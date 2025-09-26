#ifndef _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYEXCHANGE_H_
#define _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYEXCHANGE_H_

#include "Security/CryptoKeyExchange.h"
#include "cryptography/SM4GCMGMAC_Types.h"

TRAVODDS_NAMESPACE_BEGIN

enum class PskProtectionKind {
    NONE = 0,           // 不做任何PSK保护
    SIGN_ONLY,          // 仅签名/验签（HMAC-SM3）
    ENCRYPT_ONLY,       // 仅加密（SM4-GCM）
    SIGN_AND_ENCRYPT    // 先签名再加密
};

class SM4GCMGMAC_KeyExchange : public CryptoKeyExchange
{
public:
    SM4GCMGMAC_KeyExchange(
        bool is_rtps_psk_protected = false,
        bool is_rtps_axk_encrypted = false,
        PskProtectionKind psk_kind = PskProtectionKind::NONE)
        : is_rtps_psk_protected_(is_rtps_psk_protected)
        , is_rtps_axk_encrypted_(is_rtps_axk_encrypted)
        , rtps_psk_protection_kind_(psk_kind){};
        
    ~SM4GCMGMAC_KeyExchange();
    
    struct PskDeriveResult {
        std::vector<uint8_t> master_sender_key;  // 用于SM4-GCM加密/认证
        std::vector<uint8_t> master_salt;        // 用于派生IV/nonce
    };

    bool create_local_participant_crypto_tokens(
        ParticipantCryptoTokenSeq& local_participant_crypto_tokens,
        const ParticipantCryptoHandle& locat_participant_crypto,
        ParticipantCryptoHandle& remote_participant_crypto,
        SecurityException& exception) override;

    bool set_remote_participant_crypto_tokens(
        const ParticipantCryptoHandle& local_participant_crypto,
        ParticipantCryptoHandle& remote_participant_crypto,
        const ParticipantCryptoTokenSeq& remote_participant_tokens,
        SecurityException& exception) override;

    std::vector<uint8_t> KeyMaterialCDRSerialize(
            KeyMaterial_SM4_GCM_GMAC& key);

    bool KeyMaterialCDRDeserialize(
            KeyMaterial_SM4_GCM_GMAC& buffer,
            std::vector<uint8_t>* CDR);

    bool derive_psk_master_keys(
        const SM4GCMGMAC_ParticipantCryptoHandle& local_participant,
        PskDeriveResult& out_keys);
private:
    bool is_rtps_psk_protected_{false};          // 是否启用 PSK 方案
    bool is_rtps_axk_encrypted_{false};          // 是否对握手消息加密
    PskProtectionKind rtps_psk_protection_kind_{PskProtectionKind::NONE};

    static bool psk_sign_payload(
        const std::vector<uint8_t>& master_sender_key32,
        const std::vector<uint8_t>& data,
        std::vector<uint8_t>& mac_out);

    static bool psk_encrypt_payload(
        const std::vector<uint8_t>& master_sender_key,
        const std::vector<uint8_t>& master_salt,
        uint64_t /*seq*/,
        const std::vector<uint8_t>& aad,
        std::vector<uint8_t>& payload);

    static bool psk_verify_payload(
        const std::vector<uint8_t>& master_sender_key32,
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& mac_in);

    static bool psk_decrypt_payload(
        const std::vector<uint8_t>& master_sender_key,
        const std::vector<uint8_t>& master_salt,
        uint64_t /*seq*/,
        const std::vector<uint8_t>& aad,
        std::vector<uint8_t>& payload);
};
TRAVODDS_NAMESPACE_END
#endif // _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYEXCHANGE_H_