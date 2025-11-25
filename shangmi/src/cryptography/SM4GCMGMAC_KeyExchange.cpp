#include "cryptography/SM4GCMGMAC_KeyExchange.h"
#include "common/log/logger.h"

#include <sstream>

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "auth/GMHelper.h"
using namespace TRAVODDS;

// PSK 基本配置
static bool get_psk_params_stub(
    const SM4GCMGMAC_ParticipantCryptoHandle& /*local*/,
    /*out*/uint32_t& passphrase_id,
    /*out*/std::vector<uint8_t>& pre_shared_secret)
{
    // TODO: 从配置载入
    passphrase_id = 1;
    const char* demo = "demo_psk_secret";
    pre_shared_secret.assign(demo, demo + std::strlen(demo));
    return true;
}

// RTPS 上下文（用于 SenderKeyId/inner 消息拼装）
static bool get_rtps_context_stub(
    const SM4GCMGMAC_ParticipantCryptoHandle& /*local*/,
    /*out*/uint16_t& domain_id,
    /*out*/std::string& domain_tag,
    /*out*/uint8_t protocol_version[2],
    /*out*/uint8_t vendor_id[2],
    /*out*/uint8_t guid_prefix[12])
{
    // TODO: 从栈/发现获取真实值
    domain_id = 0;
    domain_tag.clear();
    protocol_version[0] = 2; protocol_version[1] = 4;
    vendor_id[0] = 0x01; vendor_id[1] = 0x23;
    std::memset(guid_prefix, 0x11, 12);
    return true;
}

SM4GCMGMAC_KeyExchange::~SM4GCMGMAC_KeyExchange()
{
}

bool SM4GCMGMAC_KeyExchange::protect_with_psk(
        SM4GCMGMAC_ParticipantCryptoHandle& pc,
        PskProtectionKind kind,
        const std::vector<uint8_t>& aad,
        const std::vector<uint8_t>& plaintext,
        ProtectedBlob& out,
        SMSecurityException& ex)
{
    if (pc.nil()) {
        ex = SMSecurityException("protect_with_psk: nil participant");
        return false;
    }
    if (!pc->psk_cfg_.enabled || kind == PskProtectionKind::NONE) {
        out.payload = plaintext;
        out.mac.clear();
        return true;
    }

    {
        std::lock_guard<std::mutex> lk(pc->mutex_);
        if (!pc->psk_keys_.has_value()) {
            if (!SM4GCMGMAC_KeyExchange::derive_psk_master_keys(pc, ex)) {
                return false;
            }
            if (!pc->psk_keys_.has_value()) {
                ex = SMSecurityException("protect_with_psk: PSK derivation produced no keys");
                return false;
            }
        }
    }

    const auto& psk = *pc->psk_keys_;

    const uint64_t seq = pc->psk_send_seq.fetch_add(1, std::memory_order_relaxed);

    switch (kind)
    {
        case PskProtectionKind::SIGN_ONLY:
        {
            if (!psk_sign_payload(psk.master_sender_key, out.payload, out.mac)) {
                ex = SMSecurityException("HMAC-SM3 signing failed");
                return false;
            }
            return true;
        }
        case PskProtectionKind::ENCRYPT_ONLY:
        {
            if (!psk_encrypt_payload(psk.master_sender_key, psk.master_salt, seq, aad, out.payload)) {
                ex = SMSecurityException("SM4-GCM encryption failed");
                return false;
            }
            return true;
        }
        case PskProtectionKind::SIGN_AND_ENCRYPT:
        {
            if (!psk_sign_payload(psk.master_sender_key, out.payload, out.mac)) {
                ex = SMSecurityException("HMAC-SM3 signing failed");
                return false;
            }
            if (!psk_encrypt_payload(psk.master_sender_key, psk.master_salt, seq, aad, out.payload)) {
                ex = SMSecurityException("SM4-GCM encryption failed");
                return false;
            }
            return true;
        }
        default:
            ex = SMSecurityException("Unknown PskProtectionKind");
            return false;
    }
}

bool SM4GCMGMAC_KeyExchange::unprotect_with_psk(
    SM4GCMGMAC_ParticipantCryptoHandle& pc,          // 由 derive_psk_master_keys得到
    PskProtectionKind kind,               // 与对端一致
    uint64_t seq,               // 入站报文的序列号（由调用者提供/解析）
    const std::vector<uint8_t>& aad,
    const std::vector<uint8_t>& in_payload,
    const std::vector<uint8_t>& in_mac,
    std::vector<uint8_t>& out_plaintext,
    SMSecurityException& ex)
{
    if (pc.nil()) {
        ex = SMSecurityException("protect_with_psk: nil participant");
        return false;
    }
    if (!pc->psk_cfg_.enabled || kind == PskProtectionKind::NONE) {
        return true;
    }

    {
        std::lock_guard<std::mutex> lk(pc->mutex_);
        if (!pc->psk_keys_.has_value()) {
            if (!SM4GCMGMAC_KeyExchange::derive_psk_master_keys(pc, ex)) {
                return false;
            }
            if (!pc->psk_keys_.has_value()) {
                ex = SMSecurityException("protect_with_psk: PSK derivation produced no keys");
                return false;
            }
        }
    }

    const auto& psk = *pc->psk_keys_;

    out_plaintext = in_payload;

    switch (kind)
    {
        case PskProtectionKind::SIGN_ONLY:
            if (in_mac.empty()) {
                ex = SMSecurityException("Missing MAC for SIGN_ONLY");
                return false;
            }
            if (!psk_verify_payload(psk.master_sender_key, out_plaintext, in_mac)) {
                ex = SMSecurityException("HMAC-SM3 verification failed (SIGN_ONLY)");
                return false;
            }
            return true;

        case PskProtectionKind::ENCRYPT_ONLY:
            if (!psk_decrypt_payload(psk.master_sender_key, psk.master_salt, seq, aad, out_plaintext)) {
                ex = SMSecurityException("SM4-GCM decryption failed (ENCRYPT_ONLY)");
                return false;
            }
            return true;

        case PskProtectionKind::SIGN_AND_ENCRYPT:
            if (in_mac.empty()) {
                ex = SMSecurityException("Missing MAC for SIGN_AND_ENCRYPT");
                return false;
            }
            // 先解密
            if (!psk_decrypt_payload(psk.master_sender_key, psk.master_salt, seq, aad, out_plaintext)) {
                ex = SMSecurityException("SM4-GCM decryption failed (SIGN_AND_ENCRYPT)");
                return false;
            }
            // 再验签
            if (!psk_verify_payload(psk.master_sender_key, out_plaintext, in_mac)) {
                ex = SMSecurityException("HMAC-SM3 verification failed (SIGN_AND_ENCRYPT)");
                return false;
            }
            return true;

        default:
            ex = SMSecurityException("Unknown PskProtectionKind");
            return false;
    }
}

bool SM4GCMGMAC_KeyExchange::derive_psk_master_keys(
    SM4GCMGMAC_ParticipantCryptoHandle& local_participant,
    SMSecurityException& ex)
{
    SM4GCMGMAC_ParticipantCryptoHandle& pc =
    SM4GCMGMAC_ParticipantCryptoHandle::narrow(local_participant);

    if (pc.nil()) {
        ex = SMSecurityException("derive_psk_master_keys: nil participant");
        return false;
    }

    // 后续访问仍用 pc-> 成员
    if (!pc->psk_cfg_.enabled || pc->psk_cfg_.kind == PskProtectionKind::NONE) {
        std::lock_guard<std::mutex> lk(pc->mutex_);
        pc->psk_keys_.reset();
        return true;
    }

    // 取 PSK 与 RTPS 上下文
    // TODO： 改为从配置或栈获取
    uint32_t passphrase_id = 0;
    std::vector<uint8_t> psk; // PreSharedSecret
    if (!get_psk_params_stub(local_participant, passphrase_id, psk)) {
        ex = SMSecurityException("derive_psk_master_keys: missing PSK params");
        return false;
    }
    uint16_t domain_id = 0; 
    std::string domain_tag;
    uint8_t pv[2]{}, vid[2]{}, gp[12]{};
    if (!get_rtps_context_stub(local_participant, domain_id, domain_tag, pv, vid, gp)) {
        ex = SMSecurityException("derive_psk_master_keys: missing RTPS context");
        return false;
    }

    // 按照文档要求进行拼接和计算
    // SenderKeyId = SM3("DomainId="|DomainId|";DomainTag="|DomainTag)[0..2] | (PassphraseId & 0xFF)
    std::string sid_input = std::string("DomainId=") + std::to_string(domain_id)
                          + std::string(";DomainTag=") + domain_tag;
    uint8_t sid_sm3[32]; 
    unsigned int sid_len = 0;
    if (!GMHelper::sm3_digest(sid_input.data(), sid_input.size(), sid_sm3, &sid_len) || sid_len != 32) {
        ex = SMSecurityException("derive_psk_master_keys: SM3(SenderKeyId) failed");
        return false;
    }
    const uint8_t passphrase_key_id = static_cast<uint8_t>(passphrase_id & 0xFF);
    std::array<uint8_t, 4> sender_key_id{ sid_sm3[0], sid_sm3[1], sid_sm3[2], passphrase_key_id };

    // inner = "<PSK-SKEY/PSK-SALT>" | SenderKeyId | "RTPS" | PV(2) | VID(2) | GP(12)
    auto build_inner = [&](const char* label){
        std::vector<uint8_t> v;
        static constexpr char kRTPS[] = "RTPS";
        v.insert(v.end(), label, label + std::strlen(label));
        v.insert(v.end(), sender_key_id.begin(), sender_key_id.end());
        v.insert(v.end(), kRTPS, kRTPS + sizeof(kRTPS)-1);
        v.insert(v.end(), pv, pv+2);
        v.insert(v.end(), vid, vid+2);
        v.insert(v.end(), gp, gp+12);
        return v;
    };
    std::vector<uint8_t> inner_skey = build_inner("PSK-SKEY");
    std::vector<uint8_t> inner_salt = build_inner("PSK-SALT");

    // inner_mac = HMAC-SM3(inner, PreSharedSecret)
    std::vector<uint8_t> inner_skey_mac, inner_salt_mac;
    if (!GMHelper::hmac_sm3(psk, inner_skey, inner_skey_mac)) return false;
    if (!GMHelper::hmac_sm3(psk, inner_salt, inner_salt_mac)) return false;

    // 派生：HMAC-SM3(inner_mac, "<label>" | 0x01) -> 32B
    auto derive_outer = [&](const std::vector<uint8_t>& inner_mac,
                            const char* label, 
                            std::vector<uint8_t>& out) {
        std::vector<uint8_t> msg; 
        msg.insert(msg.end(), label, label + std::strlen(label));
        msg.push_back(0x01);
        out.clear();
        return GMHelper::hmac_sm3(inner_mac, msg, out) && out.size()==32;
    };
    ParticipantKeyHandle::PskDeriveResult derived;
    if (!derive_outer(inner_skey_mac, "master sender key derivation", derived.master_sender_key)) {
        ex = SMSecurityException("derive_psk_master_keys: KDF(master_sender_key) failed");
        return false;
    }
    if (!derive_outer(inner_salt_mac, "master salt derivation", derived.master_salt)) {
        ex = SMSecurityException("derive_psk_master_keys: KDF(master_salt) failed");
        return false;
    }
    if (derived.master_sender_key.size() < 16) {
        ex = SMSecurityException("derive_psk_master_keys: master_sender_key too short");
        return false;
    }
    derived.master_sender_key.resize(16);
    {
        std::lock_guard<std::mutex> lk(pc->mutex_);
        pc->psk_keys_ = std::move(derived);
    }
    return true;
}

// 基类接口实现（适配器函数）
bool SM4GCMGMAC_KeyExchange::create_local_participant_crypto_tokens(
        ParticipantCryptoTokenSeq* local_participant_crypto_tokens,
        ParticipantCryptoHandle locat_participant,
        ParticipantCryptoHandle remote_participant,
        SecurityException* exception)
{
    if (local_participant_crypto_tokens == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex("Invalid parameters");
            sm_ex.to_base_exception(exception);
        }
        return false;
    }

    // 类型转换
    SMParticipantCryptoHandle& sm_local = reinterpret_cast<SMParticipantCryptoHandle&>(locat_participant);
    SMParticipantCryptoHandle& sm_remote = reinterpret_cast<SMParticipantCryptoHandle&>(remote_participant);
    ParticipantCryptoTokenSeq& sm_tokens = reinterpret_cast<ParticipantCryptoTokenSeq&>(*local_participant_crypto_tokens);
    
    SMSecurityException temp_exception;
    
    // 调用实现函数
    bool result = create_local_participant_crypto_tokens_impl(
        sm_tokens,
        sm_local,
        sm_remote,
        temp_exception);
    
    if (!result)
    {
        temp_exception.to_base_exception(exception);
    }
    
    return result;
}

// 实现函数（保持原有逻辑）
bool SM4GCMGMAC_KeyExchange::create_local_participant_crypto_tokens_impl(
        ParticipantCryptoTokenSeq& local_participant_crypto_tokens,
        SMParticipantCryptoHandle& locat_participant_crypto,
        SMParticipantCryptoHandle& remote_participant_crypto,
        SMSecurityException& exception)
{
    SM4GCMGMAC_ParticipantCryptoHandle& local_participant = SM4GCMGMAC_ParticipantCryptoHandle::narrow(locat_participant_crypto);
    SM4GCMGMAC_ParticipantCryptoHandle& remote_participant = SM4GCMGMAC_ParticipantCryptoHandle::narrow(remote_participant_crypto);

    if (local_participant.nil() || remote_participant.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Not a valid SMParticipantCryptoHandle received");
        return false;
    }

    //清空此前的 CryptoTokens
    local_participant_crypto_tokens.clear();

    try
    {
        std::vector<uint8_t> serialized =
            KeyMaterialCDRSerialize(remote_participant->Participant2ParticipantKeyMaterial.at(0));
        if (serialized.empty())
        {
            exception = SMSecurityException("KeyMaterial serialization returned empty buffer");
            return false;
        }

        ProtectedBlob pb;
        pb.payload = serialized;  
        ParticipantCryptoToken temp;
        temp.class_id = std::string("DDS:Crypto:SM4_GCM_GMAC");

        std::vector<uint8_t> seq_prop_value;
        if (local_participant->psk_cfg_.enabled &&
            local_participant->psk_cfg_.kind != PskProtectionKind::NONE)
        {
            if (!SM4GCMGMAC_KeyExchange::derive_psk_master_keys(local_participant, exception)) {
                return false;
            }

            const std::vector<uint8_t> aad;
            if (!SM4GCMGMAC_KeyExchange::protect_with_psk(local_participant, local_participant->psk_cfg_.kind, aad,
                pb.payload, pb, exception))
            {
                return false;
            }
            uint64_t seq_used;
            {
                std::lock_guard<std::mutex> lk(local_participant->mutex_);
                uint64_t cur = local_participant->psk_send_seq.load(std::memory_order_relaxed);
                seq_used = (cur == 0) ? 0 : (cur - 1);
            }
        
            seq_prop_value.resize(sizeof(uint64_t));
            std::memcpy(seq_prop_value.data(), &seq_used, sizeof(uint64_t));
        }

        BinaryProperty prop;
        prop.name = std::string("dds.cryp.keymat");
        // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
        prop.value = reinterpret_cast<const Sequence<unsigned char>&>(pb.payload);
        prop.propagate = true;
        if (prop.value.empty())
        {
            exception = SMSecurityException("Empty key material payload");
            return false;
        }
        temp.binary_properties.push_back(std::move(prop));
        
        if (!seq_prop_value.empty()) {
            BinaryProperty prop_seq;
            prop_seq.name  = "dds.cryp.keymat.seq";
            // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
            prop_seq.value = reinterpret_cast<const Sequence<unsigned char>&>(seq_prop_value);
            prop_seq.propagate = true;
            temp.binary_properties.push_back(std::move(prop_seq));
        }

        if (!pb.mac.empty())
        {
            BinaryProperty prop_mac;
            prop_mac.name   = std::string("dds.cryp.keymat.mac");
            // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
            prop_mac.value = reinterpret_cast<const Sequence<unsigned char>&>(pb.mac);
            prop_mac.propagate = true;
            temp.binary_properties.push_back(std::move(prop_mac));
        }

        local_participant_crypto_tokens.push_back(std::move(temp));
    }
    catch (const std::exception& e)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Exception while creating participant crypto tokens: " << e.what());
        exception = SMSecurityException(std::string("create_local_participant_crypto_tokens failed: ") + e.what());
        return false;
    }
    catch (...)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Unknown exception while creating participant crypto tokens");
        exception = SMSecurityException("create_local_participant_crypto_tokens failed: unknown error");
        return false;
    }

    return true;
}

// 基类接口实现（适配器函数）
bool SM4GCMGMAC_KeyExchange::set_remote_participant_crypto_tokens(
        ParticipantCryptoHandle local_participant_crypto,
        ParticipantCryptoHandle remote_participant_crypto,
        const ParticipantCryptoTokenSeq* remote_participant_tokens,
        SecurityException* exception)
{
    if (remote_participant_tokens == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex("Invalid parameters");
            sm_ex.to_base_exception(exception);
        }
        return false;
    }

    // 类型转换
    SMParticipantCryptoHandle& sm_local = reinterpret_cast<SMParticipantCryptoHandle&>(local_participant_crypto);
    SMParticipantCryptoHandle& sm_remote = reinterpret_cast<SMParticipantCryptoHandle&>(remote_participant_crypto);
    const ParticipantCryptoTokenSeq& sm_tokens = reinterpret_cast<const ParticipantCryptoTokenSeq&>(*remote_participant_tokens);
    
    SMSecurityException temp_exception;
    
    // 调用实现函数
    bool result = set_remote_participant_crypto_tokens_impl(
        sm_local,
        sm_remote,
        sm_tokens,
        temp_exception);
    
    if (!result)
    {
        temp_exception.to_base_exception(exception);
    }
    
    return result;
}

// 实现函数（保持原有逻辑）
bool SM4GCMGMAC_KeyExchange::set_remote_participant_crypto_tokens_impl(
        SMParticipantCryptoHandle& local_participant_crypto,
        SMParticipantCryptoHandle& remote_participant_crypto,
        const ParticipantCryptoTokenSeq& remote_participant_tokens,
        SMSecurityException& exception)
{
    SM4GCMGMAC_ParticipantCryptoHandle& local_participant = SM4GCMGMAC_ParticipantCryptoHandle::narrow(local_participant_crypto);
    SM4GCMGMAC_ParticipantCryptoHandle& remote_participant = SM4GCMGMAC_ParticipantCryptoHandle::narrow(remote_participant_crypto);

    if (local_participant.nil() || remote_participant.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Not a valid SMParticipantCryptoHandle received");
        exception = SMSecurityException("Invalid SMParticipantCryptoHandle");
        return false;
    }

    if (remote_participant_tokens.size() != 1)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoTokenSequence");
        exception = SMSecurityException("Incorrect remote CryptoSequence length");
        return false;
    }

    const auto& token = remote_participant_tokens.front();

    // 类型校验
    if (token.class_id != std::string("DDS:Crypto:SM4_GCM_GMAC"))
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: incorrect class_id");
        exception = SMSecurityException("Incorrect token type received");
        return false;
    }

    // 不接受 string 属性
    if (!token.properties.empty())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: unexpected string properties");
        exception = SMSecurityException("Malformed CryptoToken");
        return false;
    }

    const BinaryProperty* keymat_prop = nullptr;
    const BinaryProperty* mac_prop    = nullptr;
    const BinaryProperty* seq_prop    = nullptr;

    for (const auto& p : token.binary_properties)
    {
        if (p.name == std::string("dds.cryp.keymat"))
        {
            keymat_prop = &p;
        }
        else if (p.name == std::string("dds.cryp.keymat.mac"))
        {
            mac_prop = &p;
        }
        else if (p.name == std::string("dds.cryp.keymat.seq"))
        {
            seq_prop = &p;
        }
    }

    if (!keymat_prop)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: missing dds.cryp.keymat");
        exception = SMSecurityException("Missing key material");
        return false;
    }

    // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
    std::vector<uint8_t> payload = reinterpret_cast<const std::vector<uint8_t>&>(keymat_prop->value);
    if (payload.empty())
    {
        exception = SMSecurityException("Empty key material payload");
        return false;
    }

    // PSK 保护
    if (!local_participant.nil() && local_participant->psk_cfg_.enabled && local_participant->psk_cfg_.kind != PskProtectionKind::NONE)
    {
        if (!SM4GCMGMAC_KeyExchange::derive_psk_master_keys(local_participant, exception))
        {
            return false;
        }


        uint64_t seq = 0;
        if (seq_prop && seq_prop->value.size() == sizeof(uint64_t))
        {
            std::memcpy(&seq, seq_prop->value.data(), sizeof(uint64_t));
        }

        const std::vector<uint8_t> aad;
        // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
        std::vector<uint8_t> mac_in = mac_prop ? reinterpret_cast<const std::vector<uint8_t>&>(mac_prop->value) : std::vector<uint8_t>{};
        std::vector<uint8_t> plain;

        if (!unprotect_with_psk(local_participant, local_participant->psk_cfg_.kind, seq, aad,
                                payload, mac_in, plain, exception))
        {
            return false;
        }
        payload.swap(plain);
    }

    // CDR 反序列化
    KeyMaterial_SM4_GCM_GMAC keymat{};
    if (!KeyMaterialCDRDeserialize(keymat, &payload))
    {
        exception = SMSecurityException("KeyMaterial CDR deserialize failed");
        return false;
    }

    {
        std::unique_lock<std::mutex> lk(remote_participant->mutex_);

        remote_participant->RemoteParticipant2ParticipantKeyMaterial.push_back(keymat);
    }

    return true;
}

// 基类接口实现（适配器函数）
bool SM4GCMGMAC_KeyExchange::return_crypto_tokens(
    CryptoTokenSeq* crypto_tokens,
    SecurityException* exception)
{
    if (crypto_tokens == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex("Invalid parameters");
            sm_ex.to_base_exception(exception);
        }
        return false;
    }

    // 类型转换
    CryptoTokenSeq& sm_tokens = reinterpret_cast<CryptoTokenSeq&>(*crypto_tokens);
    
    // 直接调用实现函数
    SMSecurityException temp_exception;
    bool result = return_crypto_tokens_impl(&sm_tokens, temp_exception);
    
    if (!result)
    {
        temp_exception.to_base_exception(exception);
    }
    
    return result;
}

// 实现函数（保持原有逻辑）
bool SM4GCMGMAC_KeyExchange::return_crypto_tokens_impl(
    CryptoTokenSeq* crypto_tokens,
    SMSecurityException& exception)
{
    if (crypto_tokens==nullptr)
    {
        exception = SMSecurityException("The tokenSeq is NULL.");
        return false;
    }
    //清空seq
    crypto_tokens->clear();
    return true;
}

// 基类接口实现（适配器函数）
bool SM4GCMGMAC_KeyExchange::create_local_datawriter_crypto_tokens(
        DatawriterCryptoTokenSeq* local_datawriter_crypto_tokens,
        DatawriterCryptoHandle local_datawriter_crypto,
        DatareaderCryptoHandle remote_datareader_crypto,
        SecurityException* exception)
{
    if (local_datawriter_crypto_tokens == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex("Invalid parameters");
            sm_ex.to_base_exception(exception);
        }
        return false;
    }

    // 类型转换
    SMDatawriterCryptoHandle& sm_local_writer = reinterpret_cast<SMDatawriterCryptoHandle&>(local_datawriter_crypto);
    SMDatareaderCryptoHandle& sm_remote_reader = reinterpret_cast<SMDatareaderCryptoHandle&>(remote_datareader_crypto);
    DatawriterCryptoTokenSeq& sm_tokens = reinterpret_cast<DatawriterCryptoTokenSeq&>(*local_datawriter_crypto_tokens);
    
    SMSecurityException temp_exception;
    
    // 调用实现函数
    bool result = create_local_datawriter_crypto_tokens_impl(
        sm_tokens,
        sm_local_writer,
        sm_remote_reader,
        temp_exception);
    
    if (!result)
    {
        temp_exception.to_base_exception(exception);
    }
    
    return result;
}

// 实现函数（保持原有逻辑）
bool SM4GCMGMAC_KeyExchange::create_local_datawriter_crypto_tokens_impl(
        DatawriterCryptoTokenSeq& local_datawriter_crypto_tokens,
        SMDatawriterCryptoHandle& local_datawriter_crypto,
        SMDatareaderCryptoHandle& remote_datareader_crypto,
        SMSecurityException& exception)
{
    SM4GCMGMAC_WriterCryptoHandle&  local_writer  =
        SM4GCMGMAC_WriterCryptoHandle::narrow(local_datawriter_crypto);
    SM4GCMGMAC_ReaderCryptoHandle&  remote_reader =
        SM4GCMGMAC_ReaderCryptoHandle::narrow(remote_datareader_crypto);

    if (local_writer.nil() || remote_reader.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Invalid CryptoHandle received (writer/reader)");
        exception = SMSecurityException("Invalid CryptoHandle");
        return false;
    }

    try
    {
        auto sp_part = local_writer->Parent_participant.lock();
        if (!sp_part) {
            exception = SMSecurityException("PSK enabled but Parent_participant expired (writer not bound to participant)");
            return false;
        }
        auto& local_participant = SM4GCMGMAC_ParticipantCryptoHandle::narrow(*sp_part);
        if (local_participant.nil()) {
            exception = SMSecurityException("Invalid local participant handle after narrow()");
            return false;
        }
        std::unique_lock<std::mutex> lk(remote_reader->mutex_);

        // 刷新旧 Token
        local_datawriter_crypto_tokens.clear();

        for (auto& km : remote_reader->Remote2EntityKeyMaterial)
        {
            // CDR 序列化 KeyMaterial
            std::vector<uint8_t> serialized = KeyMaterialCDRSerialize(km);
            if (serialized.empty())
            {
                exception = SMSecurityException("KeyMaterial serialization returned empty buffer");
                return false;
            }

            ProtectedBlob pb;
            pb.payload = std::move(serialized);
            std::vector<uint8_t> seq_prop_value; // 若启用PSK则写入此属性

            if (local_participant->psk_cfg_.enabled && local_participant->psk_cfg_.kind != PskProtectionKind::NONE)
            {
                if (!derive_psk_master_keys(local_participant, exception))
                {
                    exception = SMSecurityException("PSK master key derivation failed");
                    return false;
                }

                const std::vector<uint8_t> aad; // TODO: 填充 AAD
                // Writer 侧也维护独立的发送序列号（与 Participant 类似）

                if (!protect_with_psk(local_participant, local_participant->psk_cfg_.kind, aad, pb.payload, pb, exception))
                {
                    return false;
                }
                uint64_t seq_used;
                {
                    std::lock_guard<std::mutex> lk(local_participant->mutex_);
                    uint64_t cur = local_participant->psk_send_seq.load(std::memory_order_relaxed);
                    seq_used = (cur == 0) ? 0 : (cur - 1);  // protect_with_psk 内部 fetch_add(1)
                }
                seq_prop_value.resize(sizeof(uint64_t));
                std::memcpy(seq_prop_value.data(), &seq_used, sizeof(uint64_t));
            }

            // 组装 DatawriterCryptoToken
            DatawriterCryptoToken token;
            token.class_id = std::string("DDS:Crypto:SM4_GCM_GMAC");

            // 主负载：可能是明文（未启PSK）或密文/签名标签（已启PSK）
            {
                BinaryProperty prop;
                prop.name  = std::string("dds.cryp.keymat");
                // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
                prop.value = reinterpret_cast<const Sequence<unsigned char>&>(pb.payload);
                prop.propagate = true;
                if (prop.value.empty())
                {
                    exception = SMSecurityException("Empty key material payload");
                    return false;
                }
                token.binary_properties.push_back(std::move(prop));
            }

            // 仅在 PSK 启用时携带序列号
            if (!seq_prop_value.empty())
            {
                BinaryProperty prop_seq;
                prop_seq.name  = std::string("dds.cryp.keymat.seq");
                // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
                prop_seq.value = reinterpret_cast<const Sequence<unsigned char>&>(seq_prop_value);
                prop_seq.propagate = true;
                token.binary_properties.push_back(std::move(prop_seq));
            }

            // 仅在 SIGN_ONLY / AUTH_ENC 等产生 MAC 时携带
            if (!pb.mac.empty())
            {
                BinaryProperty prop_mac;
                prop_mac.name  = std::string("dds.cryp.keymat.mac");
                // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
                prop_mac.value = reinterpret_cast<const Sequence<unsigned char>&>(pb.mac);
                prop_mac.propagate = true;
                token.binary_properties.push_back(std::move(prop_mac));
            }

            local_datawriter_crypto_tokens.push_back(std::move(token));
        }

        return true;
    }
    catch (const std::exception& e)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Exception in create_local_datawriter_crypto_tokens: " << e.what());
        exception = SMSecurityException(std::string("create_local_datawriter_crypto_tokens failed: ") + e.what());
        return false;
    }
    catch (...)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Unknown exception in create_local_datawriter_crypto_tokens");
        exception = SMSecurityException("create_local_datawriter_crypto_tokens failed: unknown error");
        return false;
    }
}

// 基类接口实现（适配器函数）
bool SM4GCMGMAC_KeyExchange::set_remote_datawriter_crypto_tokens(
        DatareaderCryptoHandle local_datareader_crypto,
        DatawriterCryptoHandle remote_datawriter_crypto,
        const DatawriterCryptoTokenSeq* remote_datawriter_token,
        SecurityException* exception)
{
    if (remote_datawriter_token == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex("Invalid parameters");
            sm_ex.to_base_exception(exception);
        }
        return false;
    }

    // 类型转换
    SMDatareaderCryptoHandle& sm_local_reader = reinterpret_cast<SMDatareaderCryptoHandle&>(local_datareader_crypto);
    SMDatawriterCryptoHandle& sm_remote_writer = reinterpret_cast<SMDatawriterCryptoHandle&>(remote_datawriter_crypto);
    const DatawriterCryptoTokenSeq& sm_tokens = reinterpret_cast<const DatawriterCryptoTokenSeq&>(*remote_datawriter_token);
    
    SMSecurityException temp_exception;
    
    // 调用实现函数
    bool result = set_remote_datawriter_crypto_tokens_impl(
        sm_local_reader,
        sm_remote_writer,
        sm_tokens,
        temp_exception);
    
    if (!result)
    {
        temp_exception.to_base_exception(exception);
    }
    
    return result;
}

// 实现函数（保持原有逻辑）
bool SM4GCMGMAC_KeyExchange::set_remote_datawriter_crypto_tokens_impl(
        SMDatareaderCryptoHandle& local_datareader_crypto,
        SMDatawriterCryptoHandle& remote_datawriter_crypto,
        const DatawriterCryptoTokenSeq& remote_datawriter_tokens,
        SMSecurityException& exception)
{
    SM4GCMGMAC_ReaderCryptoHandle& local_reader = SM4GCMGMAC_ReaderCryptoHandle::narrow(local_datareader_crypto);
    SM4GCMGMAC_WriterCryptoHandle& remote_writer = SM4GCMGMAC_WriterCryptoHandle::narrow(remote_datawriter_crypto);

    if (local_reader.nil() || remote_writer.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Invalid CryptoHandle");
        exception = SMSecurityException("Invalid CryptoHandle");
        return false;
    }

    const size_t nTokens = remote_datawriter_tokens.size();
    if (nTokens != 1 && nTokens != 2)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoTokenSequence");
        exception = SMSecurityException("Incorrect remote CryptoSequence length");
        return false;
    }

    for (size_t i = 0; i < nTokens; ++i)
    {
        const auto& token = remote_datawriter_tokens.at(i);

        if (token.class_id != std::string("DDS:Crypto:SM4_GCM_GMAC"))
        {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: incorrect class_id");
            exception = SMSecurityException("Incorrect token type received");
            return false;
        }
        if (!token.properties.empty())
        {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: unexpected string properties");
            exception = SMSecurityException("Malformed CryptoToken");
            return false;
        }

        // 提取二进制属性
        const BinaryProperty* keymat_prop = nullptr;
        const BinaryProperty* mac_prop = nullptr;
        const BinaryProperty* seq_prop = nullptr;

        for (const auto& p : token.binary_properties)
        {
            if (p.name == std::string("dds.cryp.keymat"))
                keymat_prop = &p;
            else if (p.name == std::string("dds.cryp.keymat.mac"))
                mac_prop = &p;
            else if (p.name == std::string("dds.cryp.keymat.seq"))
                seq_prop = &p;
        }

        if (!keymat_prop)
        {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: missing dds.cryp.keymat");
            exception = SMSecurityException("Missing key material");
            return false;
        }

        // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
    std::vector<uint8_t> payload = reinterpret_cast<const std::vector<uint8_t>&>(keymat_prop->value);
        if (payload.empty())
        {
            exception = SMSecurityException("Empty key material payload");
            return false;
        }
        auto sp_local_part = local_reader->Parent_participant.lock();
        if (!sp_local_part)
        {
            exception = SMSecurityException("Parent participant expired (reader)");
            return false;
        }
        SM4GCMGMAC_ParticipantCryptoHandle& local_participant = SM4GCMGMAC_ParticipantCryptoHandle::narrow(*sp_local_part);
        // PSK 解封装
        if (local_participant->psk_cfg_.enabled && local_participant->psk_cfg_.kind != PskProtectionKind::NONE)
        {
            if (local_participant.nil())
            {
                exception = SMSecurityException("Invalid local participant handle");
                return false;
            }
            if (!derive_psk_master_keys(local_participant, exception))
            {
                exception = SMSecurityException("PSK master key derivation failed");
                return false;
            }

            uint64_t seq = 0;
            if (seq_prop && seq_prop->value.size() == sizeof(uint64_t))
            {
                // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接使用
                std::memcpy(&seq, seq_prop->value.data(), sizeof(uint64_t));
            }

            const std::vector<uint8_t> aad; // 与发送侧保持一致
            // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
        std::vector<uint8_t> mac_in = mac_prop ? reinterpret_cast<const std::vector<uint8_t>&>(mac_prop->value) : std::vector<uint8_t>{};
            std::vector<uint8_t> plain;

            if (!unprotect_with_psk(local_participant, local_participant->psk_cfg_.kind, seq, aad,
                                    payload, mac_in, plain, exception))
            {
                return false;
            }
            payload.swap(plain);
        }

        // CDR 反序列化
        KeyMaterial_SM4_GCM_GMAC keymat{};
        if (!KeyMaterialCDRDeserialize(keymat, &payload))
        {
            exception = SMSecurityException("KeyMaterial CDR deserialize failed");
            return false;
        }

        {
            std::unique_lock<std::mutex> remote_writer_lock(remote_writer->mutex_);
            remote_writer->Entity2RemoteKeyMaterial.push_back(keymat);
        }
        {
            std::unique_lock<std::mutex> local_reader_lock(local_reader->mutex_);
            local_reader->Remote2EntityKeyMaterial.push_back(keymat);
        }
    }

    return true;
}

// 基类接口实现（适配器函数）
bool SM4GCMGMAC_KeyExchange::create_local_datareader_crypto_tokens(
        DatareaderCryptoTokenSeq* local_datareader_crypto_tokens,
        DatareaderCryptoHandle local_datareader_crypto,
        DatawriterCryptoHandle remote_datawriter_crypto,
        SecurityException* exception)
{
    if (local_datareader_crypto_tokens == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex("Invalid parameters");
            sm_ex.to_base_exception(exception);
        }
        return false;
    }

    // 类型转换
    SMDatareaderCryptoHandle& sm_local_reader = reinterpret_cast<SMDatareaderCryptoHandle&>(local_datareader_crypto);
    SMDatawriterCryptoHandle& sm_remote_writer = reinterpret_cast<SMDatawriterCryptoHandle&>(remote_datawriter_crypto);
    DatareaderCryptoTokenSeq& sm_tokens = reinterpret_cast<DatareaderCryptoTokenSeq&>(*local_datareader_crypto_tokens);
    
    SMSecurityException temp_exception;
    
    // 调用实现函数
    bool result = create_local_datareader_crypto_tokens_impl(
        sm_tokens,
        sm_local_reader,
        sm_remote_writer,
        temp_exception);
    
    if (!result)
    {
        temp_exception.to_base_exception(exception);
    }
    
    return result;
}

// 实现函数（保持原有逻辑）
bool SM4GCMGMAC_KeyExchange::create_local_datareader_crypto_tokens_impl(
        DatareaderCryptoTokenSeq& local_datareader_crypto_tokens,
        SMDatareaderCryptoHandle& local_datareader_crypto,
        SMDatawriterCryptoHandle& remote_datawriter_crypto,
        SMSecurityException& exception)
{
    SM4GCMGMAC_ReaderCryptoHandle&  local_reader  =
        SM4GCMGMAC_ReaderCryptoHandle::narrow(local_datareader_crypto);
    SM4GCMGMAC_WriterCryptoHandle&  remote_writer =
        SM4GCMGMAC_WriterCryptoHandle::narrow(remote_datawriter_crypto);

    if (local_reader.nil() || remote_writer.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Invalid CryptoHandle received (reader/writer)");
        exception = SMSecurityException("Invalid CryptoHandle");
        return false;
    }

    try
    {
        auto sp_part = local_reader->Parent_participant.lock();
        if (!sp_part)
        {
            exception = SMSecurityException("Parent participant expired (reader)");
            return false;
        }

        auto& local_participant = SM4GCMGMAC_ParticipantCryptoHandle::narrow(*sp_part);


        if (local_participant->psk_cfg_.enabled && local_participant->psk_cfg_.kind != PskProtectionKind::NONE)
        {
            if (local_participant.nil())
            {
                exception = SMSecurityException("Invalid local participant handle after narrow()");
                return false;
            }
        }

        std::unique_lock<std::mutex> lk(remote_writer->mutex_);

        // 清空旧 token
        local_datareader_crypto_tokens.clear();

        for (auto& km : remote_writer->Remote2EntityKeyMaterial)
        {
            // CDR 序列化
            std::vector<uint8_t> serialized = KeyMaterialCDRSerialize(km);
            if (serialized.empty())
            {
                exception = SMSecurityException("KeyMaterial serialization returned empty buffer");
                return false;
            }

            ProtectedBlob pb;
            pb.payload = std::move(serialized);
            std::vector<uint8_t> seq_prop_value; 

            if (local_participant->psk_cfg_.enabled && local_participant->psk_cfg_.kind != PskProtectionKind::NONE)
            {
                // 派生 PSK 主密钥
                if (!derive_psk_master_keys(local_participant, exception))
                {
                    exception = SMSecurityException("PSK master key derivation failed");
                    return false;
                }

                // 保护负载
                const std::vector<uint8_t> aad;

                std::lock_guard<std::mutex> lk2(local_reader->mutex_);
                uint64_t seq_used = local_participant->psk_send_seq.load(std::memory_order_relaxed);

                if (!protect_with_psk(local_participant, local_participant->psk_cfg_.kind, aad, pb.payload, pb, exception))
                {
                    return false;
                }
                seq_prop_value.resize(sizeof(uint64_t));
                std::memcpy(seq_prop_value.data(), &seq_used, sizeof(uint64_t));
            }

            // 构造 DatareaderCryptoToken
            DatareaderCryptoToken token;
            token.class_id = std::string("DDS:Crypto:SM4_GCM_GMAC");

            // 主负载（明文或密文/密文+tag）
            {
                BinaryProperty prop;
                prop.name  = std::string("dds.cryp.keymat");
                // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
                prop.value = reinterpret_cast<const Sequence<unsigned char>&>(pb.payload);
                prop.propagate = true;
                if (prop.value.empty())
                {
                    exception = SMSecurityException("Empty key material payload");
                    return false;
                }
                token.binary_properties.push_back(std::move(prop));
            }

            // 仅 PSK 时携带序列号
            if (!seq_prop_value.empty())
            {
                BinaryProperty prop_seq;
                prop_seq.name  = std::string("dds.cryp.keymat.seq");
                // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
                prop_seq.value = reinterpret_cast<const Sequence<unsigned char>&>(seq_prop_value);
                prop_seq.propagate = true;
                token.binary_properties.push_back(std::move(prop_seq));
            }

            // 若使用签名，携带 MAC
            if (!pb.mac.empty())
            {
                BinaryProperty prop_mac;
                prop_mac.name  = std::string("dds.cryp.keymat.mac");
                // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
                prop_mac.value = reinterpret_cast<const Sequence<unsigned char>&>(pb.mac);
                prop_mac.propagate = true;
                token.binary_properties.push_back(std::move(prop_mac));
            }

            // 写入序列
            local_datareader_crypto_tokens.push_back(std::move(token));
        }

        return true;
    }
    catch (const std::exception& e)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Exception in create_local_datareader_crypto_tokens: " << e.what());
        exception = SMSecurityException(std::string("create_local_datareader_crypto_tokens failed: ") + e.what());
        return false;
    }
    catch (...)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Unknown exception in create_local_datareader_crypto_tokens");
        exception = SMSecurityException("create_local_datareader_crypto_tokens failed: unknown error");
        return false;
    }
}

// 基类接口实现（适配器函数）
bool SM4GCMGMAC_KeyExchange::set_remote_datareader_crypto_tokens(
        DatawriterCryptoHandle local_datawriter_crypto,
        DatareaderCryptoHandle remote_datareader_crypto,
        const DatareaderCryptoTokenSeq* remote_datareader_tokens,
        SecurityException* exception)
{
    if (remote_datareader_tokens == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex("Invalid parameters");
            sm_ex.to_base_exception(exception);
        }
        return false;
    }

    // 类型转换
    SMDatawriterCryptoHandle& sm_local_writer = reinterpret_cast<SMDatawriterCryptoHandle&>(local_datawriter_crypto);
    SMDatareaderCryptoHandle& sm_remote_reader = reinterpret_cast<SMDatareaderCryptoHandle&>(remote_datareader_crypto);
    const DatareaderCryptoTokenSeq& sm_tokens = reinterpret_cast<const DatareaderCryptoTokenSeq&>(*remote_datareader_tokens);
    
    SMSecurityException temp_exception;
    
    // 调用实现函数
    bool result = set_remote_datareader_crypto_tokens_impl(
        sm_local_writer,
        sm_remote_reader,
        sm_tokens,
        temp_exception);
    
    if (!result)
    {
        temp_exception.to_base_exception(exception);
    }
    
    return result;
}

// 实现函数（保持原有逻辑）
bool SM4GCMGMAC_KeyExchange::set_remote_datareader_crypto_tokens_impl(
        SMDatawriterCryptoHandle&                 local_datawriter_crypto,
        SMDatareaderCryptoHandle&                 remote_datareader_crypto,
        const DatareaderCryptoTokenSeq&         remote_datareader_tokens,
        SMSecurityException&                      exception)
{
    SM4GCMGMAC_WriterCryptoHandle& local_writer = SM4GCMGMAC_WriterCryptoHandle::narrow(local_datawriter_crypto);
    SM4GCMGMAC_ReaderCryptoHandle& remote_reader = SM4GCMGMAC_ReaderCryptoHandle::narrow(remote_datareader_crypto);

    if (local_writer.nil() || remote_reader.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Invalid CryptoHandle received");
        exception = SMSecurityException("Invalid CryptoHandle");
        return false;
    }

    const size_t nTokens = remote_datareader_tokens.size();
    if (nTokens != 1 && nTokens != 2)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoTokenSequence");
        exception = SMSecurityException("Incorrect remote CryptoSequence length");
        return false;
    }

    for (size_t i = 0; i < nTokens; ++i)
    {
        const auto& token = remote_datareader_tokens.at(i);

        // class_id 校验
        if (token.class_id != std::string("DDS:Crypto:SM4_GCM_GMAC"))
        {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: incorrect class_id");
            exception = SMSecurityException("Incorrect token type received");
            return false;
        }

        if (!token.properties.empty())
        {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: unexpected string properties");
            exception = SMSecurityException("Malformed CryptoToken");
            return false;
        }

        // 提取二进制属性
        const BinaryProperty* keymat_prop = nullptr;
        const BinaryProperty* mac_prop = nullptr;
        const BinaryProperty* seq_prop = nullptr;

        for (const auto& p : token.binary_properties)
        {
            if (p.name == std::string("dds.cryp.keymat"))
                keymat_prop = &p;
            else if (p.name == std::string("dds.cryp.keymat.mac"))
                mac_prop = &p;
            else if (p.name == std::string("dds.cryp.keymat.seq"))
                seq_prop = &p;
        }

        if (!keymat_prop)
        {
            REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: missing dds.cryp.keymat");
            exception = SMSecurityException("Missing key material");
            return false;
        }

        // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
    std::vector<uint8_t> payload = reinterpret_cast<const std::vector<uint8_t>&>(keymat_prop->value);
        if (payload.empty())
        {
            exception = SMSecurityException("Empty key material payload");
            return false;
        }
        // 用本地 writer 的父 participant 派生接收侧主密钥
        auto sp_local_part = local_writer->Parent_participant.lock();
        if (!sp_local_part)
        {
            exception = SMSecurityException("Parent participant expired (writer)");
            return false;
        }

        SM4GCMGMAC_ParticipantCryptoHandle& local_participant =
            SM4GCMGMAC_ParticipantCryptoHandle::narrow(*sp_local_part);
        // PSK 解封装
        if (local_participant->psk_cfg_.enabled && local_participant->psk_cfg_.kind != PskProtectionKind::NONE)
        {

            if (local_participant.nil())
            {
                exception = SMSecurityException("Invalid local participant handle");
                return false;
            }

            if (!derive_psk_master_keys(local_participant, exception))
            {
                exception = SMSecurityException("PSK master key derivation failed");
                return false;
            }

            uint64_t seq = 0;
            if (seq_prop && seq_prop->value.size() == sizeof(uint64_t))
            {
                // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接使用
                std::memcpy(&seq, seq_prop->value.data(), sizeof(uint64_t));
            }

            const std::vector<uint8_t> aad;
            // Sequence<unsigned char> 就是 std::vector<unsigned char>，可以直接转换
        std::vector<uint8_t> mac_in = mac_prop ? reinterpret_cast<const std::vector<uint8_t>&>(mac_prop->value) : std::vector<uint8_t>{};
            std::vector<uint8_t> plain;

            if (!unprotect_with_psk(local_participant, local_participant->psk_cfg_.kind, seq, aad,
                                    payload, mac_in, plain, exception))
            {
                return false;
            }
            payload.swap(plain);
        }

        // CDR 反序列化
        KeyMaterial_SM4_GCM_GMAC keymat{};
        if (!KeyMaterialCDRDeserialize(keymat, &payload))
        {
            exception = SMSecurityException("KeyMaterial CDR deserialize failed");
            return false;
        }

        {
            std::unique_lock<std::mutex> remote_reader_lock(remote_reader->mutex_);
            remote_reader->Entity2RemoteKeyMaterial.push_back(keymat);
        }
        {
            std::unique_lock<std::mutex> local_writer_lock(local_writer->mutex_);
            local_writer->Remote2EntityKeyMaterial.push_back(keymat);
        }
    }

    return true;
}

std::vector<uint8_t> SM4GCMGMAC_KeyExchange::KeyMaterialCDRSerialize(
    KeyMaterial_SM4_GCM_GMAC& key)
{
    std::vector<uint8_t> buffer;
    buffer.reserve(128); 

    // 序列化 transformation_kind (4 字节)
    buffer.insert(buffer.end(), key.transformation_kind.begin(), key.transformation_kind.end());

    // 对于 SM4，密钥和盐值的长度始终是 128 位 (16 字节)
    const uint8_t sm4_key_len = 16;

    // 序列化 master_salt (sequence<octet>)
    buffer.insert(buffer.end(), 3, 0x00);      // 长度字段的前3个字节为0
    buffer.push_back(sm4_key_len);             // 长度字段的最后1个字节
    buffer.insert(buffer.end(), key.master_salt.begin(), key.master_salt.end());

    // 序列化 sender_key_id (4 字节)
    buffer.insert(buffer.end(), key.sender_key_id.begin(), key.sender_key_id.end());

    // 序列化 master_sender_key (sequence<octet>)
    buffer.insert(buffer.end(), 3, 0x00);
    buffer.push_back(sm4_key_len);
    buffer.insert(buffer.end(), key.master_sender_key.begin(), key.master_sender_key.end());

    // 序列化 receiver_specific_key_id (4 字节)
    buffer.insert(buffer.end(), key.receiver_specific_key_id.begin(), key.receiver_specific_key_id.end());

    // 检查是否存在 receiver_specific_key
    bool has_specific_key = (key.receiver_specific_key_id != c_transform_kind_none);

    if (has_specific_key)
    {
        // 如果存在，序列化 master_receiver_specific_key (sequence<octet>)
        buffer.insert(buffer.end(), 3, 0x00);
        buffer.push_back(sm4_key_len);
        buffer.insert(buffer.end(), key.master_receiver_specific_key.begin(), key.master_receiver_specific_key.end());
    }
    else
    {
        // 如果不存在，写入一个长度为 0 的空序列
        buffer.insert(buffer.end(), 4, 0x00);
    }

    return buffer;
}


/**
* @brief 从 CDR 字节流中反序列化出 KeyMaterial_SM4_GCM_GMAC 结构体。
* @param key [out] 用于存储反序列化结果的结构体。
* @param cdr_buffer [in] 包含序列化数据的字节流。
* @return 如果反序列化成功则返回 true，否则返回 false (例如数据长度不足)。
*/
bool SM4GCMGMAC_KeyExchange::KeyMaterialCDRDeserialize(
    KeyMaterial_SM4_GCM_GMAC& key,
    std::vector<uint8_t>* cdr_buffer)
{
    if (nullptr == cdr_buffer || cdr_buffer->empty())
    {
        return false;
    }

    const uint8_t* data = cdr_buffer->data();
    size_t buffer_size = cdr_buffer->size();   
    size_t pos = 0;

    const uint8_t expected_key_len = 16;

    // 反序列化 transformation_kind
    if (pos + 4 > buffer_size) return false;
    memcpy(key.transformation_kind.data(), &data[pos], 4);
    pos += 4;

    // 反序列化 master_salt
    if (pos + 4 > buffer_size) return false;
    uint8_t salt_len = data[pos + 3];
    pos += 4;
    if (salt_len != expected_key_len || pos + salt_len > buffer_size) return false;
    memcpy(key.master_salt.data(), &data[pos], salt_len);
    pos += salt_len;

    // 反序列化 sender_key_id
    if (pos + 4 > buffer_size) return false;
    memcpy(key.sender_key_id.data(), &data[pos], 4);
    pos += 4;

    // 反序列化 master_sender_key
    if (pos + 4 > buffer_size) return false;
    uint8_t sender_key_len = data[pos + 3];
    pos += 4;
    if (sender_key_len != expected_key_len || pos + sender_key_len > buffer_size) return false;
    memcpy(key.master_sender_key.data(), &data[pos], sender_key_len);
    pos += sender_key_len;

    // 反序列化 receiver_specific_key_id
    if (pos + 4 > buffer_size) return false;
    memcpy(key.receiver_specific_key_id.data(), &data[pos], 4);
    pos += 4;

    // 检查是否存在 receiver_specific_key
    bool has_specific_key = (key.receiver_specific_key_id != c_transform_kind_none);

    if (has_specific_key)
    {
        if (pos + 4 > buffer_size) return false;
        uint8_t specific_key_len = data[pos + 3];
        pos += 4;
        if (specific_key_len != expected_key_len || pos + specific_key_len > buffer_size) return false;
        memcpy(key.master_receiver_specific_key.data(), &data[pos], specific_key_len);
        pos += specific_key_len;
    }

    return true;
}

bool SM4GCMGMAC_KeyExchange::psk_sign_payload(const std::vector<uint8_t>& master_sender_key32,
                                const std::vector<uint8_t>& data,
                                std::vector<uint8_t>& mac_out)
{
    if (master_sender_key32.size() < 16) return false;
    return GMHelper::hmac_sm3(master_sender_key32, data, mac_out);
}

bool SM4GCMGMAC_KeyExchange::psk_encrypt_payload(const std::vector<uint8_t>& master_sender_key,
    const std::vector<uint8_t>& master_salt,
    uint64_t seq,
    const std::vector<uint8_t>& aad,
    std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> iv = GMHelper::derive_iv_from_salt(master_salt, seq);
    if (iv.size() != 12) { // 固定 12B IV
        return false;
    }

    std::vector<uint8_t> sm4_key(master_sender_key.begin(),
                                 master_sender_key.begin()+16);

    std::vector<uint8_t> ct, tag;
    if (!GMHelper::sm4_gcm_encrypt_vector(sm4_key, iv, aad, payload, ct, tag)) {
        return false;
    }
    payload.clear();
    payload.insert(payload.end(), ct.begin(), ct.end());
    payload.insert(payload.end(), tag.begin(), tag.end());
    return true;
}

bool SM4GCMGMAC_KeyExchange::psk_verify_payload(const std::vector<uint8_t>& master_sender_key32,
                                                 const std::vector<uint8_t>& data,
                                                 const std::vector<uint8_t>& mac_in)
{
    if (master_sender_key32.size() < 16) return false;
    if (mac_in.empty()) return false;

    std::vector<uint8_t> mac_calc;
    if (!GMHelper::hmac_sm3(master_sender_key32, data, mac_calc)) {
        return false;
    }

    // 常量时间比较,避免时序侧信道攻击
    if (mac_calc.size() != mac_in.size()) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < mac_calc.size(); ++i) {
        diff |= static_cast<unsigned char>(mac_calc[i] ^ mac_in[i]);
    }
    return diff == 0;
}

bool SM4GCMGMAC_KeyExchange::psk_decrypt_payload(const std::vector<uint8_t>& master_sender_key,
                                                  const std::vector<uint8_t>& master_salt,
                                                  uint64_t seq,
                                                  const std::vector<uint8_t>& aad,
                                                  std::vector<uint8_t>& payload)
{
    if (master_sender_key.size() < 16) return false;
    if (payload.size() < 16) return false; 

    std::vector<uint8_t> iv = GMHelper::derive_iv_from_salt(master_salt, seq);
    if (iv.size() != 12) {
        return false;
    }

    // SM4 16B key 取 master_sender_key 前16字节
    std::vector<uint8_t> sm4_key(master_sender_key.begin(),
                                 master_sender_key.begin() + 16);

    // 拆分出 ct 与 tag
    const size_t n = payload.size();
    std::vector<uint8_t> ct(payload.begin(), payload.begin() + (n - 16));
    std::vector<uint8_t> tag(payload.begin() + (n - 16), payload.end());

    std::vector<uint8_t> plain;
    if (!GMHelper::sm4_gcm_decrypt_vector(sm4_key, iv, aad, ct, tag, plain)) {
        return false;
    }

    payload.swap(plain);
    return true;
}
