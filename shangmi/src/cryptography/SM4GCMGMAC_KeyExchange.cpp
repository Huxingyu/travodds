#include "travodds_dll.h"
#include "cryptography/SM4GCMGMAC_KeyExchange.h"
#include "common/log/logger.h"

#include <cstring>
#include <sstream>

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "auth/GMHelper.h"
USING_TRAVODDS_NAMESPACE

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

bool SM4GCMGMAC_KeyExchange::derive_psk_master_keys(
    const SM4GCMGMAC_ParticipantCryptoHandle& local_participant,
    PskDeriveResult& out_keys)
{
    out_keys = {};
    if (!is_rtps_psk_protected_ || rtps_psk_protection_kind_ == PskProtectionKind::NONE) {
        return false; // 未启用PSK时，不派生
    }

    // 取 PSK 与 RTPS 上下文
    // TODO： 改为从配置或栈获取
    uint32_t passphrase_id = 0;
    std::vector<uint8_t> psk; // PreSharedSecret
    if (!get_psk_params_stub(local_participant, passphrase_id, psk)) {
        return false;
    }
    uint16_t domain_id = 0; 
    std::string domain_tag;
    uint8_t pv[2]{}, vid[2]{}, gp[12]{};
    if (!get_rtps_context_stub(local_participant, domain_id, domain_tag, pv, vid, gp)) {
        return false;
    }

    // 按照文档要求进行拼接和计算
    // SenderKeyId = SM3("DomainId="|DomainId|";DomainTag="|DomainTag)[0..2] | (PassphraseId & 0xFF)
    std::string sid_input = std::string("DomainId=") + std::to_string(domain_id)
                          + std::string(";DomainTag=") + domain_tag;
    uint8_t sid_sm3[32]; 
    unsigned int sid_len=0;
    GMHelper::sm3_digest(sid_input.data(), sid_input.size(), sid_sm3, &sid_len);
    const uint8_t passphrase_key_id = static_cast<uint8_t>(passphrase_id & 0xFF);
    std::array<uint8_t,4> sender_key_id{sid_sm3[0], sid_sm3[1], sid_sm3[2], passphrase_key_id};

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
    if (!derive_outer(inner_skey_mac, "master sender key derivation", out_keys.master_sender_key)) return false;
    if (!derive_outer(inner_salt_mac, "master salt derivation", out_keys.master_salt)) return false;

    return true;
}

bool SM4GCMGMAC_KeyExchange::create_local_participant_crypto_tokens(
    ParticipantCryptoTokenSeq& local_participant_crypto_tokens,
    const ParticipantCryptoHandle& locat_participant_crypto,
    ParticipantCryptoHandle& remote_participant_crypto,
    SecurityException& exception)
{
    const SM4GCMGMAC_ParticipantCryptoHandle& local_participant = SM4GCMGMAC_ParticipantCryptoHandle::narrow(locat_participant_crypto);
    SM4GCMGMAC_ParticipantCryptoHandle& remote_participant = SM4GCMGMAC_ParticipantCryptoHandle::narrow(remote_participant_crypto);

    if (local_participant.nil() || remote_participant.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Not a valid ParticipantCryptoHandle received");
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
            exception = SecurityException("KeyMaterial serialization returned empty buffer");
            return false;
        }

        std::vector<uint8_t> payload = serialized;  // 默认明文
        std::vector<uint8_t> mac;                   // 仅在 SIGN_* 时使用

        if (is_rtps_psk_protected_ && rtps_psk_protection_kind_ != PskProtectionKind::NONE)
        {
            PskDeriveResult psk{};
            if (!derive_psk_master_keys(local_participant, psk)) {
                exception = SecurityException("PSK master key derivation failed");
                return false;
            }

            const std::vector<uint8_t> aad;
            uint64_t seq = 0;
            // auto do_sign = [&](const std::vector<uint8_t>& data)->bool {
            //     mac.clear();
            //     return GMHelper::hmac_sm3(psk.master_sender_key, data, mac);
            // };

            // auto do_encrypt = [&]()->bool {
            //     // 12B IV：由 master_salt(32B) + 序号派生；现阶段用 seq=0
            //     // TODO: 换成真实的会话/消息序号
            //     std::vector<uint8_t> iv = GMHelper::derive_iv_from_salt(psk.master_salt, /*seq=*/0);

            //     // SM4 16B密钥：取 master_sender_key 前 16 字节
            //     std::vector<uint8_t> sm4_key(psk.master_sender_key.begin(),
            //                                  psk.master_sender_key.begin()+16);

            //     std::vector<uint8_t> ct, tag;
            //     if (!GMHelper::sm4_gcm_encrypt_vector(sm4_key, iv, aad, payload, ct, tag)) {
            //         return false;
            //     }
            //     payload.clear();
            //     payload.reserve(ct.size() + tag.size());
            //     payload.insert(payload.end(), ct.begin(), ct.end());
            //     payload.insert(payload.end(), tag.begin(), tag.end());
            //     return true;
            // };

            switch (rtps_psk_protection_kind_) {
                case PskProtectionKind::SIGN_ONLY:
                    if (!psk_sign_payload(psk.master_sender_key, payload, mac)) {
                        exception = SecurityException("HMAC-SM3 signing failed");
                        return false;
                    }
                    break;
                case PskProtectionKind::ENCRYPT_ONLY:
                    if (!psk_encrypt_payload(psk.master_sender_key, psk.master_salt, seq, aad, payload)) {
                        exception = SecurityException("SM4-GCM encryption failed");
                        return false;
                    }
                    break;
                case PskProtectionKind::SIGN_AND_ENCRYPT:
                    if (!psk_sign_payload(psk.master_sender_key, payload, mac)) {
                        exception = SecurityException("HMAC-SM3 signing failed");
                        return false;
                    }
                    if (!psk_encrypt_payload(psk.master_sender_key, psk.master_salt, seq, aad, payload)) {
                        exception = SecurityException("SM4-GCM encryption failed");
                        return false;
                    }
                    break;
                case PskProtectionKind::NONE:
                default:
                    break;
            }
        }

        ParticipantCryptoToken temp;
        temp.GetClassID() = std::string("DDS:Crypto:SM4_GCM_GMAC");

        BinaryProperty prop;
        prop.name()   = std::string("dds.cryp.keymat");
        prop.value()  = std::move(payload);   // 明文或密文||tag
        prop.propagate(true);
        temp.GetBinaryProperties().push_back(std::move(prop));

        if (!mac.empty())
        {
            BinaryProperty prop_mac;
            prop_mac.name()   = std::string("dds.cryp.keymat.mac");
            prop_mac.value()  = std::move(mac);
            prop_mac.propagate(true);
            temp.GetBinaryProperties().push_back(std::move(prop_mac));
        }

        local_participant_crypto_tokens.push_back(std::move(temp));
    }
    catch (const std::exception& e)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Exception while creating participant crypto tokens: " << e.what());
        exception = SecurityException(std::string("create_local_participant_crypto_tokens failed: ") + e.what());
        return false;
    }
    catch (...)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Unknown exception while creating participant crypto tokens");
        exception = SecurityException("create_local_participant_crypto_tokens failed: unknown error");
        return false;
    }

    return true;
}

bool SM4GCMGMAC_KeyExchange::set_remote_participant_crypto_tokens(
        const ParticipantCryptoHandle& local_participant_crypto,
        ParticipantCryptoHandle& remote_participant_crypto,
        const ParticipantCryptoTokenSeq& remote_participant_tokens,
        SecurityException& exception)
{
    const SM4GCMGMAC_ParticipantCryptoHandle& local_participant =
        SM4GCMGMAC_ParticipantCryptoHandle::narrow(local_participant_crypto);
    SM4GCMGMAC_ParticipantCryptoHandle& remote_participant =
        SM4GCMGMAC_ParticipantCryptoHandle::narrow(remote_participant_crypto);

    if (local_participant.nil() || remote_participant.nil())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Not a valid ParticipantCryptoHandle received");
        exception = SecurityException("Invalid ParticipantCryptoHandle");
        return false;
    }

    // Token 基本结构校验
    if (remote_participant_tokens.size() != 1)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Invalid CryptoTokenSeq length");
        exception = SecurityException("Incorrect remote CryptoSequence length");
        return false;
    }

    const ParticipantCryptoToken& token = remote_participant_tokens.at(0);
    if (token.GetClassID() != std::string("DDS:Crypto:SM4_GCM_GMAC"))
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: unexpected class_id");
        exception = SecurityException("Incorrect token type received");
        return false;
    }

    if (!token.GetProperties().empty())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: unexpected string properties");
        exception = SecurityException("Malformed CryptoToken");
        return false;
    }

    // 提取二进制属性
    const BinaryProperty* keymat_prop = nullptr;
    const BinaryProperty* mac_prop    = nullptr;

    for (const auto& p : token.GetBinaryProperties())
    {
        if (p.name() == std::string("dds.cryp.keymat")) {
            keymat_prop = &p;
        } else if (p.name() == std::string("dds.cryp.keymat.mac")) {
            mac_prop = &p;
        }
    }

    if (!keymat_prop)
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Malformed CryptoToken: missing dds.cryp.keymat");
        exception = SecurityException("Missing key material");
        return false;
    }

    std::vector<uint8_t> payload = keymat_prop->value();
    std::vector<uint8_t> mac_recv;
    if (mac_prop) {
        mac_recv = mac_prop->value(); // HMAC-SM3
    }

    if (payload.empty())
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "Empty key material payload");
        exception = SecurityException("Empty key material payload");
        return false;
    }

    if (is_rtps_psk_protected_ && rtps_psk_protection_kind_ != PskProtectionKind::NONE)
    {
        PskDeriveResult psk{};
        if (!derive_psk_master_keys(local_participant, psk))
        {
            exception = SecurityException("PSK master key derivation failed");
            return false;
        }

        const std::vector<uint8_t> aad;
        // uint64_t seq = 0;           

        switch (rtps_psk_protection_kind_)
        {
            case PskProtectionKind::SIGN_ONLY:
            {
                if (mac_recv.empty())
                {
                    exception = SecurityException("Missing MAC for SIGN_ONLY");
                    return false;
                }
                if (!psk_verify_payload(psk.master_sender_key, payload, mac_recv))
                {
                    exception = SecurityException("HMAC-SM3 verification failed");
                    return false;
                }
                break;
            }

            case PskProtectionKind::ENCRYPT_ONLY:
            {
                if (!psk_decrypt_payload(psk.master_sender_key, psk.master_salt, /*seq =*/ 0, aad, payload))
                {
                    exception = SecurityException("SM4-GCM decryption failed");
                    return false;
                }
                break;
            }

            case PskProtectionKind::SIGN_AND_ENCRYPT:
            {
                if (mac_recv.empty())
                {
                    exception = SecurityException("Missing MAC for SIGN_AND_ENCRYPT");
                    return false;
                }
                // 解密
                if (!psk_decrypt_payload(psk.master_sender_key, psk.master_salt, /*seq =*/ 0, aad, payload))
                {
                    exception = SecurityException("SM4-GCM decryption failed");
                    return false;
                }
                // 验签
                if (!psk_verify_payload(psk.master_sender_key, payload, mac_recv))
                {
                    exception = SecurityException("HMAC-SM3 verification failed");
                    return false;
                }
                break;
            }

            case PskProtectionKind::NONE:
            default:
                break;
        }
    }
    else
    {
    }

    // 反序列化 KeyMaterial
    KeyMaterial_SM4_GCM_GMAC keymat{};
    if (!KeyMaterialCDRDeserialize(keymat, &payload))
    {
        REC_LOG_WARNING(SECURITY_CRYPTO, "KeyMaterial CDR deserialize failed");
        exception = SecurityException("KeyMaterial CDR deserialize failed");
        return false;
    }

    remote_participant->RemoteParticipant2ParticipantKeyMaterial.push_back(keymat);
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
    uint64_t /*seq*/,
    const std::vector<uint8_t>& aad,
    std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> iv = GMHelper::derive_iv_from_salt(master_salt, /*seq =*/ 0);
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
                                                  uint64_t /*seq*/,
                                                  const std::vector<uint8_t>& aad,
                                                  std::vector<uint8_t>& payload)
{
    if (master_sender_key.size() < 16) return false;
    if (payload.size() < 16) return false; 

    std::vector<uint8_t> iv = GMHelper::derive_iv_from_salt(master_salt, /*seq=*/0);
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
