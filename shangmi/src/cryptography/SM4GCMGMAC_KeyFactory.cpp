#include "travodds_dll.h"
#include <string>
#include "common/log/logger.h"
#include "accesscontrol/EndpointSecurityAttributes.h"
#include "cryptography/SM4GCMGMAC_KeyFactory.h"
#include "accesscontrol/ParticipantSecurityAttributes.h"
#include "rtps/security/common/SharedSecretHandle.h"
#include <algorithm>
#include <openssl/rand.h>
#include "auth/GMHelper.h"

USING_TRAVODDS_NAMESPACE

// 直接移植
ParticipantCryptoHandleDeleter::ParticipantCryptoHandleDeleter(
        SM4GCMGMAC_KeyFactory& factory)
{
    factory_ = std::weak_ptr<SM4GCMGMAC_KeyFactory>(factory.shared_from_this());
}

void ParticipantCryptoHandleDeleter::operator ()(
        SM4GCMGMAC_ParticipantCryptoHandle* pk)
{
    if (nullptr == pk)
    {
        return;
    }

    assert(!factory_.expired());

    SecurityException exception;
    if (nullptr == (*pk)->exception_)
    {
        (*pk)->exception_ = &exception;
    }

    factory_.lock()->release_participant(*pk);
}

void SM4GCMGMAC_KeyFactory::release_participant(
        SM4GCMGMAC_ParticipantCryptoHandle& key)
{
    using namespace std;

    assert(key->exception_ != nullptr);

    auto& exception = *key->exception_;

    {
        lock_guard<mutex> _(key->mutex_);

        release_key_id(key->ParticipantKeyMaterial.sender_key_id);
        assert([&key]() -> bool
                {
                    try
                    {
                        key.shared_from_this();
                    }
                    catch (std::bad_weak_ptr&)
                    {
                        return true;
                    }
                    return false;
                }
                    ());

        for (auto& writer : key->Writers)
        {
            unregister_datawriter(writer, exception);
        }

        for (auto& reader : key->Readers)
        {
            unregister_datareader(reader, exception);
        }

        key->Writers.clear();
        key->Readers.clear();
    }

    delete (&key);
}

SM4GCMGMAC_KeyFactory::SM4GCMGMAC_KeyFactory()
{
}

std::shared_ptr<ParticipantCryptoHandle> SM4GCMGMAC_KeyFactory::register_local_participant(
        const IdentityHandle& participant_identity,
        const PermissionsHandle& participant_permissions,
        const PropertySeq& participant_properties,
        const ParticipantSecurityAttributes& participant_security_attributes,
        SecurityException& exception)
{
    std::shared_ptr<SM4GCMGMAC_ParticipantCryptoHandle> PCrypto;

    auto ih = new SM4GCMGMAC_ParticipantCryptoHandle();
    PCrypto = std::shared_ptr<SM4GCMGMAC_ParticipantCryptoHandle>(ih, ParticipantCryptoHandleDeleter(*this));

    auto plugin_attrs = participant_security_attributes.plugin_participant_attributes;
    (*PCrypto)->ParticipantPluginAttributes = plugin_attrs;

    bool is_rtps_encrypted = (plugin_attrs & PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ENCRYPTED) != 0;
    bool is_origin_auth = (plugin_attrs & PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED) != 0;
    uint64_t maxblockspersession = 32; 

    // 文档中没有提到这个属性，暂时不处理
    // if (!participant_properties.empty())
    // {
    //     for (auto it = participant_properties.begin(); it != participant_properties.end(); ++it)
    //     {
    //         if ((it)->name().compare("dds.sec.crypto.maxblockspersession") == 0)
    //         {
    //             try
    //             {
    //                 int tmp = std::stoi((it)->value());
    //                 if (tmp > 0)
    //                 {
    //                     maxblockspersession = tmp;
    //                 }
    //             }
    //             catch (std::invalid_argument&)
    //             {
    //             }
    //         }
    //     }
    // }
    create_key((*PCrypto)->ParticipantKeyMaterial, is_rtps_encrypted);

    (*PCrypto)->max_blocks_per_session = maxblockspersession;
    (*PCrypto)->Session.session_block_counter = maxblockspersession + 1; // 强制首次使用时更新密钥

    KeyMaterial_SM4_GCM_GMAC buffer;

    buffer.transformation_kind = (*PCrypto)->ParticipantKeyMaterial.transformation_kind;
    buffer.master_salt = (*PCrypto)->ParticipantKeyMaterial.master_salt;
    buffer.master_sender_key = (*PCrypto)->ParticipantKeyMaterial.master_sender_key;
    buffer.sender_key_id = (*PCrypto)->ParticipantKeyMaterial.sender_key_id;
    if (is_origin_auth)
    {
        buffer.receiver_specific_key_id = (*PCrypto)->ParticipantKeyMaterial.sender_key_id;
        buffer.master_receiver_specific_key = (*PCrypto)->ParticipantKeyMaterial.master_sender_key;
    }
    else
    {
        buffer.receiver_specific_key_id = c_transformKeyIdZero;
        buffer.master_receiver_specific_key.fill(0);
    }

    (*PCrypto)->Participant2ParticipantKeyMaterial.push_back(buffer);
    (*PCrypto)->RemoteParticipant2ParticipantKeyMaterial.push_back(buffer);
    auto& kx_list = (*PCrypto)->Participant2ParticipantKxKeyMaterial;
    if (kx_list.empty())
    {
        kx_list.push_back(buffer);
    }
    else
    {
        kx_list[0] = buffer;  // 覆盖已有的第一个元素
    }

    return std::static_pointer_cast<ParticipantCryptoHandle>(PCrypto);
}

std::shared_ptr<ParticipantCryptoHandle> SM4GCMGMAC_KeyFactory::register_matched_remote_participant(
    const IdentityHandle& remote_participant_identity,
    const ParticipantCryptoHandle& local_participant_crypto_handle,
    const SecretHandle& sharesecret,
    const PermissionsHandle& remote_participant_permissions,
    SecurityException& exception){
    // 提取参数
    const std::vector<uint8_t>* challenge_1 = SharedSecretHelper::find_data_value(sharesecret, "Challenge1");
    const std::vector<uint8_t>* shared_secret_ss = SharedSecretHelper::find_data_value(sharesecret, "SharedSecret");
    const std::vector<uint8_t>* challenge_2 = SharedSecretHelper::find_data_value(sharesecret, "Challenge2");

    if ((challenge_1 == nullptr) || (shared_secret_ss == nullptr) || (challenge_2 == nullptr))
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed SharedSecretHandle");
        exception = SecurityException("Unable to read SharedSecret and Challenges");
        return nullptr;
    }

    const SM4GCMGMAC_ParticipantCryptoHandle& local_participant_handle =
    SM4GCMGMAC_ParticipantCryptoHandle::narrow(local_participant_crypto_handle);

    auto plugin_attrs = local_participant_handle->ParticipantPluginAttributes;
    bool is_origin_auth =
            (plugin_attrs & PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_ORIGIN_AUTHENTICATED) != 0;

    std::shared_ptr<SM4GCMGMAC_ParticipantCryptoHandle> RPCrypto;

    auto ih = new SM4GCMGMAC_ParticipantCryptoHandle();
    RPCrypto = std::shared_ptr<SM4GCMGMAC_ParticipantCryptoHandle>(ih, ParticipantCryptoHandleDeleter(*this));

    (*RPCrypto)->ParticipantPluginAttributes = plugin_attrs;

    // 填写Participant2ParticipantKeyMaterial的值,用于加密传出数据
    {
        KeyMaterial_SM4_GCM_GMAC buffer;

        buffer.transformation_kind = local_participant_handle->ParticipantKeyMaterial.transformation_kind;
        buffer.master_salt = local_participant_handle->ParticipantKeyMaterial.master_salt;
        buffer.master_sender_key = local_participant_handle->ParticipantKeyMaterial.master_sender_key;
        buffer.sender_key_id = local_participant_handle->ParticipantKeyMaterial.sender_key_id;
        //初始化接收方密钥
        buffer.master_receiver_specific_key.fill(0);
        buffer.receiver_specific_key_id = c_transformKeyIdZero;
        if (is_origin_auth)
        {
            buffer.receiver_specific_key_id = make_unique_KeyId();
            RAND_bytes(buffer.master_receiver_specific_key.data(), 16);
        }
        //将组装好的密钥材料添加到RPCrypto的参与者间密钥列表
        (*RPCrypto)->Participant2ParticipantKeyMaterial.push_back(buffer);
    }

    // 填充Participant2ParticipantKxKeyMaterial的值,用于加密 CryptoTokens
    {
        KeyMaterial_SM4_GCM_GMAC buffer; //Buffer = Participant2ParticipantKxKeyMaterial

        buffer.transformation_kind = c_transform_kind_sm4_gcm;
        buffer.sender_key_id.fill(0);
        buffer.receiver_specific_key_id.fill(0);
        buffer.master_receiver_specific_key.fill(0);

        std::array<uint8_t, 32> derived_salt_material;
        if (!GMHelper::create_kx_key(derived_salt_material, challenge_1, "keyexchange salt", challenge_2, shared_secret_ss))
        {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Error generating the keys to perform token transaction");
            exception = SecurityException("Encountered an error while creating KxKeyMaterials");
            return std::shared_ptr<ParticipantCryptoHandle>();
        }
        std::copy_n(derived_salt_material.begin(), 16, buffer.master_salt.begin());

        std::array<uint8_t, 32> derived_key_material;
        if (!GMHelper::create_kx_key(derived_key_material, challenge_2, "key exchange key", challenge_1, shared_secret_ss))
        {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Error generating the keys to perform token transaction");
            exception = SecurityException("Encountered an error while creating KxKeyMaterials");
            return std::shared_ptr<ParticipantCryptoHandle>();
        }
        std::copy_n(derived_key_material.begin(), 16, buffer.master_sender_key.begin());

        (*RPCrypto)->max_blocks_per_session = local_participant_handle->max_blocks_per_session;
        (*RPCrypto)->Session.session_block_counter = local_participant_handle->max_blocks_per_session + 1;
        (*RPCrypto)->Session.session_id = (std::numeric_limits<uint32_t>::max)();
        if ((*RPCrypto)->Session.session_id == local_participant_handle->Session.session_id)
        {
            (*RPCrypto)->Session.session_id -= 1;
        }

        auto& kx_list = (*RPCrypto)->Participant2ParticipantKxKeyMaterial;
        if (kx_list.empty())
        {
            kx_list.push_back(buffer);
        }
        else
        {
            kx_list[0] = buffer;
        }

        auto wHandle = std::dynamic_pointer_cast<SM4GCMGMAC_WriterCryptoHandle>(get_datawriter_handle());
        (*wHandle)->EndpointPluginAttributes = PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED;
        (*wHandle)->Participant_master_key_id = c_transformKeyIdZero;
        (*wHandle)->Parent_participant = std::weak_ptr<ParticipantCryptoHandle>(RPCrypto);
        (*wHandle)->EntityKeyMaterial.push_back(buffer);
        (*wHandle)->Entity2RemoteKeyMaterial.push_back(buffer);
        (*wHandle)->Remote2EntityKeyMaterial.push_back(buffer);
        (*wHandle)->Sessions[0].session_id = (*RPCrypto)->Session.session_id;
        (*wHandle)->max_blocks_per_session = (*RPCrypto)->max_blocks_per_session;
        (*wHandle)->Sessions[0].session_block_counter = (*RPCrypto)->Session.session_block_counter;
        (*RPCrypto)->Writers.push_back(wHandle);

        auto rHandle = std::dynamic_pointer_cast<SM4GCMGMAC_ReaderCryptoHandle>(get_datareader_handle());
        (*rHandle)->EndpointPluginAttributes = PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED;
        (*rHandle)->Participant_master_key_id = c_transformKeyIdZero;
        (*rHandle)->Parent_participant = std::weak_ptr<ParticipantCryptoHandle>(RPCrypto);
        (*rHandle)->EntityKeyMaterial.push_back(buffer);
        (*rHandle)->Entity2RemoteKeyMaterial.push_back(buffer);
        (*rHandle)->Remote2EntityKeyMaterial.push_back(buffer);
        (*rHandle)->Sessions[0].session_id = (*RPCrypto)->Session.session_id;
        (*rHandle)->max_blocks_per_session = (*RPCrypto)->max_blocks_per_session;
        (*rHandle)->Sessions[0].session_block_counter = (*RPCrypto)->Session.session_block_counter;
        (*RPCrypto)->Readers.push_back(rHandle);
    }

    return std::static_pointer_cast<ParticipantCryptoHandle>(RPCrypto);
}

DatareaderCryptoHandle* SM4GCMGMAC_KeyFactory::register_local_datareader(
    ParticipantCryptoHandle& participant_crypto,
    const PropertySeq& datareader_properties,
    const EndpointSecurityAttributes& datareader_security_attributes,
    SecurityException& exception)
{
    SM4GCMGMAC_ParticipantCryptoHandle& participant_handle =
        SM4GCMGMAC_ParticipantCryptoHandle::narrow(participant_crypto);

    if (participant_handle.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Invalid ParticipantCryptoHandle");
        return nullptr;
    }

    auto plugin_attrs = datareader_security_attributes.plugin_endpoint_attributes;
    bool is_sub_encrypted = (plugin_attrs & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED) != 0;

    bool use_kx_keys = false;
    
    uint64_t maxblockspersession = 32; 

    if (!datareader_properties.empty())
    {
        for (auto it = datareader_properties.begin(); it != datareader_properties.end(); ++it)
        {
            if (it->name.compare("dds.sec.builtin_endpoint_name") == 0)
            {
                if (it->value.compare("BuiltinParticipantVolatileMessageSecureReader") == 0)
                {
                    use_kx_keys = true;
                }
            }
        }
    }

    if (use_kx_keys)
    {
        if (!participant_handle->Readers.empty() && participant_handle->Readers[0])
        {
            return participant_handle->Readers[0].get();
        }
        REC_LOG_WARNING(SECURITY_CRYPTO, "KX Reader not prepared");
        exception = SecurityException("KX Reader not prepared");
        return nullptr;
    }

    auto RCrypto = std::dynamic_pointer_cast<SM4GCMGMAC_ReaderCryptoHandle>(get_datareader_handle());
    (*RCrypto)->EndpointPluginAttributes = plugin_attrs;

    {
        KeyMaterial_SM4_GCM_GMAC buffer;
        create_key(buffer, is_sub_encrypted);
        (*RCrypto)->EntityKeyMaterial.push_back(buffer);
    }

    (*RCrypto)->max_blocks_per_session = maxblockspersession;
    (*RCrypto)->Sessions[0].session_block_counter = maxblockspersession + 1;
    RAND_bytes((unsigned char*)(&((*RCrypto)->Sessions[0].session_id)), sizeof(uint32_t));

    std::unique_lock<std::mutex> lock(participant_handle->mutex_);
    (*RCrypto)->Participant_master_key_id = participant_handle->ParticipantKeyMaterial.sender_key_id;
    (*RCrypto)->Parent_participant = std::weak_ptr<ParticipantCryptoHandle>(participant_crypto.shared_from_this());
    participant_handle->Readers.push_back(RCrypto);

    return RCrypto.get();
}

DatawriterCryptoHandle* SM4GCMGMAC_KeyFactory::register_local_datawriter(
    ParticipantCryptoHandle& participant_crypto,
    const PropertySeq& datawriter_properties,
    const EndpointSecurityAttributes& datawriter_sec_attr,
    SecurityException& exception)
{
    SM4GCMGMAC_ParticipantCryptoHandle& participant_handle =
            SM4GCMGMAC_ParticipantCryptoHandle::narrow(participant_crypto);

    if (participant_handle.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Invalid ParticipantCryptoHandle");
        return nullptr;
    }

    auto plugin_attrs = datawriter_sec_attr.plugin_endpoint_attributes;
    bool is_sub_encrypted = (plugin_attrs & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED) != 0;
    bool is_payload_encrypted = (plugin_attrs & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_ENCRYPTED) != 0;
    bool use_kx_keys = false;
    uint64_t maxblockspersession = 32;
    if (!datawriter_properties.empty())
    {
        for (auto it = datawriter_properties.begin(); it != datawriter_properties.end(); ++it)
        {
            // else if (it->name().compare("dds.sec.crypto.maxblockspersession") == 0)
            // {
            //     try
            //     {
            //         int tmp = std::stoi((it)->value());
            //         if (tmp > 0)
            //         {
            //             maxblockspersession = tmp;
            //         }
            //     }
            //     catch (std::invalid_argument&)
            //     {
            //     }
            // }
            if (it->name.compare("dds.sec.builtin_endpoint_name") == 0)
            {
                if (it->value.compare("BuiltinParticipantVolatileMessageSecureWriter") == 0)
                {
                    use_kx_keys = true;
                }
            }
        }
    }

    if (use_kx_keys)
    {
        if (!participant_handle->Writers.empty() && participant_handle->Writers[0])
        {
            return participant_handle->Writers[0].get();
        }
        REC_LOG_WARNING(SECURITY_CRYPTO, "KX Writer not prepared");
        exception = SecurityException("KX Writer not prepared");
        return nullptr;
    }

    auto WCrypto = std::dynamic_pointer_cast<SM4GCMGMAC_WriterCryptoHandle>(get_datawriter_handle());
    (*WCrypto)->EndpointPluginAttributes = plugin_attrs;

    auto session = &(*WCrypto)->Sessions[0];

    if (datawriter_sec_attr.is_submessage_protected)
    {
        KeyMaterial_SM4_GCM_GMAC buffer;
        create_key(buffer, is_sub_encrypted);
        (*WCrypto)->EntityKeyMaterial.push_back(buffer);
        session->session_block_counter = maxblockspersession + 1;
        RAND_bytes((unsigned char*)(&(session->session_id)), sizeof(uint32_t));
        session++;
    }

    if (datawriter_sec_attr.is_payload_protected)
    {
        if (!datawriter_sec_attr.is_submessage_protected ||
                (is_payload_encrypted != is_sub_encrypted))
        {
            KeyMaterial_SM4_GCM_GMAC buffer;
            create_key(buffer, is_payload_encrypted);
            (*WCrypto)->EntityKeyMaterial.push_back(buffer);
            session->session_block_counter = maxblockspersession + 1;
            RAND_bytes((unsigned char*)(&(session->session_id)), sizeof(uint32_t));
        }
    }

    (*WCrypto)->max_blocks_per_session = maxblockspersession;

    std::unique_lock<std::mutex> david_loftus_lock(participant_handle->mutex_);

    (*WCrypto)->Participant_master_key_id = participant_handle->ParticipantKeyMaterial.sender_key_id;

    (*WCrypto)->Parent_participant = std::weak_ptr<ParticipantCryptoHandle>(participant_crypto.shared_from_this());

    participant_handle->Writers.push_back(WCrypto);

    return WCrypto.get();
}

DatareaderCryptoHandle* SM4GCMGMAC_KeyFactory::register_matched_remote_datareader(
    DatawriterCryptoHandle& local_datawriter_crypto_handle,
    ParticipantCryptoHandle& remote_participant_crypto,
    const SharedSecretHandle& shared_secret,
    const bool relay_only,
    SecurityException& exception)
{
    SM4GCMGMAC_WriterCryptoHandle& local_writer_handle = SM4GCMGMAC_WriterCryptoHandle::narrow(local_datawriter_crypto_handle);

    if (local_writer_handle.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed DataWriterCryptoHandle");
        return nullptr;
    }

    SM4GCMGMAC_ParticipantCryptoHandle& remote_participant = SM4GCMGMAC_ParticipantCryptoHandle::narrow(remote_participant_crypto);

    std::unique_lock<std::mutex> writer_lock(local_writer_handle->mutex_);
    auto plugin_attrs = local_writer_handle->EndpointPluginAttributes;
    bool is_origin_auth = (plugin_attrs & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED) != 0;

    // 构造并返回的远端 Reader CryptoHandle
    auto RRCrypto = std::dynamic_pointer_cast<SM4GCMGMAC_ReaderCryptoHandle>(get_datareader_handle());

    (*RRCrypto)->EndpointPluginAttributes  = plugin_attrs;
    (*RRCrypto)->Participant_master_key_id = local_writer_handle->Participant_master_key_id;

    if (local_writer_handle->EntityKeyMaterial.empty())
    {
        if (remote_participant->Participant2ParticipantKxKeyMaterial.empty())
        {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Remote participant has no KX key material");
            exception = SecurityException("Remote participant has no KX key material");
            return nullptr;
        }
        // 沿用远端 Participant 的 Kx 密钥
        (*RRCrypto)->Remote2EntityKeyMaterial.push_back(
            remote_participant->Participant2ParticipantKxKeyMaterial.at(0));
        (*RRCrypto)->Entity2RemoteKeyMaterial.push_back(
            remote_participant->Participant2ParticipantKxKeyMaterial.at(0));
    }
    else
    {
        KeyMaterial_SM4_GCM_GMAC buffer; 

        buffer.transformation_kind = local_writer_handle->EntityKeyMaterial.at(0).transformation_kind;
        buffer.master_salt = local_writer_handle->EntityKeyMaterial.at(0).master_salt;
        buffer.master_sender_key = local_writer_handle->EntityKeyMaterial.at(0).master_sender_key;
        buffer.sender_key_id = local_writer_handle->EntityKeyMaterial.at(0).sender_key_id;

        buffer.master_receiver_specific_key.fill(0);
        buffer.receiver_specific_key_id = c_transformKeyIdZero;
        if (is_origin_auth)
        {
            buffer.receiver_specific_key_id = make_unique_KeyId();
            RAND_bytes(buffer.master_receiver_specific_key.data(), 16);
        }

        (*RRCrypto)->Remote2EntityKeyMaterial.push_back(buffer);
        if (is_origin_auth)
        {
            local_writer_handle->Entity2RemoteKeyMaterial.push_back(buffer);
        }
    }

    auto session = &(*RRCrypto)->Sessions[0];
    session->session_block_counter = local_writer_handle->Sessions[0].session_block_counter;
    session->session_id = (std::numeric_limits<uint32_t>::max)();
    if (session->session_id == local_writer_handle->Sessions[0].session_id)
    {
        session->session_id -= 1;
    }

    // 如果不是 relay-only 且本地 writer 有第二套实体密钥，则为远端 reader 附加第二套共享密钥，并为其分配第二个会话槽位
    if (!relay_only && local_writer_handle->EntityKeyMaterial.size() > 1)
    {
        KeyMaterial_SM4_GCM_GMAC buffer;

        buffer.transformation_kind       = local_writer_handle->EntityKeyMaterial.at(1).transformation_kind;
        buffer.master_salt               = local_writer_handle->EntityKeyMaterial.at(1).master_salt;
        buffer.master_sender_key         = local_writer_handle->EntityKeyMaterial.at(1).master_sender_key;
        buffer.sender_key_id             = local_writer_handle->EntityKeyMaterial.at(1).sender_key_id;
        buffer.master_receiver_specific_key.fill(0);
        buffer.receiver_specific_key_id  = c_transformKeyIdZero;

        // 仅挂到远端 Reader
        (*RRCrypto)->Remote2EntityKeyMaterial.push_back(buffer);

        // 第二个会话槽位沿用同样的块计数，并确保 session_id 与 writer 不冲突
        ++session;
        session->session_block_counter = local_writer_handle->Sessions[0].session_block_counter;
        session->session_id            = (std::numeric_limits<uint32_t>::max)();
        if (session->session_id == local_writer_handle->Sessions[0].session_id)
        {
            session->session_id -= 1;
        }
    }

    // 同步每会话最大块数
    (*RRCrypto)->max_blocks_per_session = local_writer_handle->max_blocks_per_session;

    writer_lock.unlock();

    std::unique_lock<std::mutex> remote_participant_lock(remote_participant->mutex_);

    (*RRCrypto)->Parent_participant =
        std::weak_ptr<ParticipantCryptoHandle>(remote_participant_crypto.shared_from_this());

    // 将该远端 Reader handle 挂到远端 participant
    (*remote_participant)->Readers.push_back(RRCrypto);

    return RRCrypto.get();
}

DatawriterCryptoHandle* SM4GCMGMAC_KeyFactory::register_matched_remote_datawriter(
    DatareaderCryptoHandle& local_datareader_crypto_handle,
    ParticipantCryptoHandle& remote_participant_crypto,
    const SharedSecretHandle& shared_secret,
    SecurityException& exception)
{
    SM4GCMGMAC_ReaderCryptoHandle& local_reader_handle =
        SM4GCMGMAC_ReaderCryptoHandle::narrow(local_datareader_crypto_handle);

    if (local_reader_handle.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Invalid DataReaderCryptoHandle");
        return nullptr;
    }

    SM4GCMGMAC_ParticipantCryptoHandle& remote_participant =
        SM4GCMGMAC_ParticipantCryptoHandle::narrow(remote_participant_crypto);

    std::unique_lock<std::mutex> reader_lock(local_reader_handle->mutex_);
    auto plugin_attrs = local_reader_handle->EndpointPluginAttributes;
    bool is_origin_auth =
        (plugin_attrs & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED) != 0;

    auto RWCrypto = std::dynamic_pointer_cast<SM4GCMGMAC_WriterCryptoHandle>(get_datawriter_handle());

    (*RWCrypto)->Participant_master_key_id = local_reader_handle->Participant_master_key_id;
    (*RWCrypto)->EndpointPluginAttributes = local_reader_handle->EndpointPluginAttributes;

    if (local_reader_handle->EntityKeyMaterial.empty())
    {
        if (remote_participant->Participant2ParticipantKxKeyMaterial.empty())
        {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Remote participant has no KX key material");
            exception = SecurityException("Remote participant has no KX key material");
            return nullptr;
        }
        // 本地 Reader 是一个内置的 Key Exchange 端点，使用远程参与者的 Kx 密钥
        (*RWCrypto)->Remote2EntityKeyMaterial.push_back(remote_participant->Participant2ParticipantKxKeyMaterial.at(0));
        (*RWCrypto)->Entity2RemoteKeyMaterial.push_back(remote_participant->Participant2ParticipantKxKeyMaterial.at(0));
    }
    else
    {
        KeyMaterial_SM4_GCM_GMAC buffer; 

        // 从本地 Reader 的实体密钥中复制共享密钥信息
        buffer.transformation_kind = local_reader_handle->EntityKeyMaterial.at(0).transformation_kind;
        buffer.master_salt = local_reader_handle->EntityKeyMaterial.at(0).master_salt;
        buffer.master_sender_key = local_reader_handle->EntityKeyMaterial.at(0).master_sender_key;
        buffer.sender_key_id = local_reader_handle->EntityKeyMaterial.at(0).sender_key_id;
        
        // 为 receiver-specific key 生成新值
        buffer.master_receiver_specific_key.fill(0);
        buffer.receiver_specific_key_id = c_transformKeyIdZero;
        if (is_origin_auth)
        {
            buffer.receiver_specific_key_id = make_unique_KeyId();
            RAND_bytes(buffer.master_receiver_specific_key.data(), 16);
        }

        (*RWCrypto)->Remote2EntityKeyMaterial.push_back(buffer);
        if (is_origin_auth)
        {
            local_reader_handle->Entity2RemoteKeyMaterial.push_back(buffer);
        }
    }

    (*RWCrypto)->max_blocks_per_session = local_reader_handle->max_blocks_per_session;
    auto session = &(*RWCrypto)->Sessions[0];
    session->session_block_counter = local_reader_handle->Sessions[0].session_block_counter;
    session->session_id = (std::numeric_limits<uint32_t>::max)();
    if (session->session_id == local_reader_handle->Sessions[0].session_id)
    {
        session->session_id -= 1;
    }

    reader_lock.unlock();

    std::unique_lock<std::mutex> remote_participant_lock(remote_participant->mutex_);

    (*RWCrypto)->Parent_participant =
        std::weak_ptr<ParticipantCryptoHandle>(remote_participant_crypto.shared_from_this());

    // 将新创建的 Writer 句柄保存到远程参与者中
    (*remote_participant)->Writers.push_back(RWCrypto);

    return RWCrypto.get();
}

bool SM4GCMGMAC_KeyFactory::unregister_participant(
    std::shared_ptr<ParticipantCryptoHandle>& participant_crypto_handle,
    SecurityException& exception)
{
    if (!participant_crypto_handle)
    {
        return false;
    }

    SM4GCMGMAC_ParticipantCryptoHandle& handle =
            SM4GCMGMAC_ParticipantCryptoHandle::narrow(*participant_crypto_handle);

    assert(nullptr == handle->exception_);
    handle->exception_ = &exception;

    // 释放当前这份引用，并检查是否还有悬挂引用
    std::weak_ptr<ParticipantCryptoHandle> wp(participant_crypto_handle);
    participant_crypto_handle.reset();
    if (!wp.expired())
    {
        exception = SecurityException("Outstanding works on the crypto handle");
        handle->exception_ = nullptr; // 复位，避免悬挂指针
        return false;
    }

    return true;
}

bool SM4GCMGMAC_KeyFactory::unregister_datawriter(
    std::shared_ptr<DatawriterCryptoHandle>& datawriter_crypto_handle,
    SecurityException& exception)
{
    auto datawriter = std::dynamic_pointer_cast<SM4GCMGMAC_WriterCryptoHandle>(datawriter_crypto_handle);
    if (!datawriter)
    {
        exception = SecurityException("Not a valid DataWriterCryptoHandle has been passed as an argument");
        return false;
    }

    // 先释放实体 KeyId（不依赖 parent）
    for (const auto& km : (*datawriter)->EntityKeyMaterial) {
        release_key_id(km.sender_key_id);
    }

    // 若父 Participant 仍存活，则从其 Writers 容器里移除此 handle
    auto parent_participant = std::dynamic_pointer_cast<SM4GCMGMAC_ParticipantCryptoHandle>(
        (*datawriter)->Parent_participant.lock());

    if (parent_participant)
    {
        std::lock_guard<std::mutex> _((*parent_participant)->mutex_);
        auto& writers = (*parent_participant)->Writers;

        auto it = std::find(writers.begin(), writers.end(), datawriter_crypto_handle);
        if (it != writers.end())
        {
            writers.erase(it);
        }
        else
        {
            exception = SecurityException("DataWriter missing from Participant handle");
        }
    }

    // 期望此时为最后一个引用
    datawriter_crypto_handle.reset();
    return true;
}

bool SM4GCMGMAC_KeyFactory::unregister_datareader(
    std::shared_ptr<DatareaderCryptoHandle>& datareader_crypto_handle,
    SecurityException& exception)
{
    auto datareader = std::dynamic_pointer_cast<SM4GCMGMAC_ReaderCryptoHandle>(datareader_crypto_handle);
    if (!datareader)
    {
        exception = SecurityException("Not a valid DataReaderCryptoHandle has been passed as an argument");
        return false;
    }

    for (const auto& km : (*datareader)->EntityKeyMaterial) {
        release_key_id(km.sender_key_id);
    }

    // 若父 Participant 仍存活，则从其 Readers 容器里移除此 handle
    auto parent_participant = std::dynamic_pointer_cast<SM4GCMGMAC_ParticipantCryptoHandle>(
        (*datareader)->Parent_participant.lock());

    if (parent_participant)
    {
        std::lock_guard<std::mutex> _((*parent_participant)->mutex_);
        auto& readers = (*parent_participant)->Readers;

        auto it = std::find(readers.begin(), readers.end(), datareader_crypto_handle);
        if (it != readers.end())
        {
            readers.erase(it);
        }
        else
        {
            exception = SecurityException("DataReader missing from Participant handle");
        }
    }

    // 期望此时为最后一个引用
    datareader_crypto_handle.reset();
    return true;
}

CryptoTransformKeyId SM4GCMGMAC_KeyFactory::make_unique_KeyId()
{
    static std::mutex s_ids_mutex;
    std::lock_guard<std::mutex> _(s_ids_mutex);

    CryptoTransformKeyId buffer{{0, 0, 0, 0}};
    bool unique = false;

    while (!unique)
    {
        RAND_bytes(buffer.data(), 4);
        unique =
                std::find(m_CryptoTransformKeyIds.begin(), m_CryptoTransformKeyIds.end(),
                        buffer) == m_CryptoTransformKeyIds.end();
    }

    m_CryptoTransformKeyIds.push_back(buffer);

    return buffer;
}

void SM4GCMGMAC_KeyFactory::release_key_id(
        CryptoTransformKeyId key)
{
    static std::mutex s_ids_mutex;
    std::lock_guard<std::mutex> _(s_ids_mutex);

    std::vector<CryptoTransformKeyId>::iterator it;
    it = std::find(m_CryptoTransformKeyIds.begin(), m_CryptoTransformKeyIds.end(), key);
    if (it != m_CryptoTransformKeyIds.end())
    {
        m_CryptoTransformKeyIds.erase(it);
    }
}

void SM4GCMGMAC_KeyFactory::create_key(
    KeyMaterial_SM4_GCM_GMAC& key,
    bool encrypt_then_sign){
    if (encrypt_then_sign)
    {
        key.transformation_kind = c_transform_kind_sm4_gcm;
    }
    else
    {
        key.transformation_kind = c_transform_kind_sm4_gmac;
    }

    constexpr int nBytes = 16; 

    key.master_salt.fill(0);
    RAND_bytes(key.master_salt.data(), nBytes);

    key.sender_key_id = make_unique_KeyId();
    key.master_sender_key.fill(0);
    RAND_bytes(key.master_sender_key.data(), nBytes);

    key.receiver_specific_key_id = c_transformKeyIdZero;
    key.master_receiver_specific_key.fill(0);
}

std::shared_ptr<DatawriterCryptoHandle> SM4GCMGMAC_KeyFactory::get_datawriter_handle()
{
    return std::dynamic_pointer_cast<DatawriterCryptoHandle>(
        std::shared_ptr<SM4GCMGMAC_WriterCryptoHandle>(
            new SM4GCMGMAC_WriterCryptoHandle,
            [](SM4GCMGMAC_WriterCryptoHandle* p)
            {
                delete p;
            }));
}

std::shared_ptr<DatareaderCryptoHandle> SM4GCMGMAC_KeyFactory::get_datareader_handle()
{
    return std::dynamic_pointer_cast<DatareaderCryptoHandle>(
        std::shared_ptr<SM4GCMGMAC_ReaderCryptoHandle>(
            new SM4GCMGMAC_ReaderCryptoHandle,
            [](SM4GCMGMAC_ReaderCryptoHandle* p)
            {
                delete p;
            }));
}