//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYEXCHANGE_H_
#define _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYEXCHANGE_H_

#include <cstring>
#include "security/cryptokeyexchange.h"
#include "cryptography/SM4GCMGMAC_Types.h"

TRAVODDS_NAMESPACE_BEGIN

struct SeqEntry {
    GUID_t guid;
    std::atomic<uint64_t> seq;

    SeqEntry() : guid{}, seq(0) {}
    SeqEntry(const GUID_t& g, uint64_t init = 0) : guid(g), seq(0) {
        seq.store(init, std::memory_order_relaxed);
    }

    // 自定义移动构造
    SeqEntry(SeqEntry&& other) noexcept : guid(other.guid), seq(0)
    {
        seq.store(other.seq.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    // 自定义移动赋值
    SeqEntry& operator=(SeqEntry&& other) noexcept
    {
        if (this != &other) {
            guid = other.guid;
            seq.store(other.seq.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }

    // 禁用拷贝
    SeqEntry(const SeqEntry&) = delete;
    SeqEntry& operator=(const SeqEntry&) = delete;
};

class SM4GCMGMAC_KeyExchange : public CryptoKeyExchange
{
public:
    SM4GCMGMAC_KeyExchange(){};
        
    ~SM4GCMGMAC_KeyExchange();

    // 基类接口实现（适配器函数，带 override）
    bool create_local_participant_crypto_tokens(
        ParticipantCryptoTokenSeq* local_participant_crypto_tokens,
        ParticipantCryptoHandle locat_participant,
        ParticipantCryptoHandle remote_participant,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool create_local_participant_crypto_tokens_impl(
        SMParticipantCryptoTokenSeq& local_participant_crypto_tokens,
        SMParticipantCryptoHandle& locat_participant_crypto,
        SMParticipantCryptoHandle& remote_participant_crypto,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool set_remote_participant_crypto_tokens(
        ParticipantCryptoHandle local_participant_crypto,
        ParticipantCryptoHandle remote_participant_crypto,
        const ParticipantCryptoTokenSeq* remote_participant_tokens,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool set_remote_participant_crypto_tokens_impl(
        SMParticipantCryptoHandle& local_participant_crypto,
        SMParticipantCryptoHandle& remote_participant_crypto,
        const SMParticipantCryptoTokenSeq& remote_participant_tokens,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool create_local_datawriter_crypto_tokens(
        DatawriterCryptoTokenSeq* local_datawriter_crypto_tokens,
        DatawriterCryptoHandle local_datawriter_crypto,
        DatareaderCryptoHandle remote_datareader_crypto,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool create_local_datawriter_crypto_tokens_impl(
        SMDatawriterCryptoTokenSeq& local_datawriter_crypto_tokens,
        SMDatawriterCryptoHandle& local_datawriter_crypto,
        SMDatareaderCryptoHandle& remote_datareader_crypto,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool set_remote_datawriter_crypto_tokens(
        DatareaderCryptoHandle local_datareader_crypto,
        DatawriterCryptoHandle remote_datawriter_crypto,
        const DatawriterCryptoTokenSeq* remote_datawriter_token,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool set_remote_datawriter_crypto_tokens_impl(
        SMDatareaderCryptoHandle& local_datareader_crypto,
        SMDatawriterCryptoHandle& remote_datawriter_crypto,
        const SMDatawriterCryptoTokenSeq& remote_datawriter_tokens,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool create_local_datareader_crypto_tokens(
        DatareaderCryptoTokenSeq* local_datareader_crypto_tokens,
        DatareaderCryptoHandle local_datareader_crypto,
        DatawriterCryptoHandle remote_datawriter_crypto,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool create_local_datareader_crypto_tokens_impl(
        SMDatareaderCryptoTokenSeq& local_datareader_crypto_tokens,
        SMDatareaderCryptoHandle& local_datareader_crypto,
        SMDatawriterCryptoHandle& remote_datawriter_crypto,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool set_remote_datareader_crypto_tokens(
        DatawriterCryptoHandle local_datawriter_crypto,
        DatareaderCryptoHandle remote_datareader_crypto,
        const DatareaderCryptoTokenSeq* remote_datareader_tokens,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool set_remote_datareader_crypto_tokens_impl(
        SMDatawriterCryptoHandle& local_datawriter_crypto,
        SMDatareaderCryptoHandle& remote_datareader_crypto,
        const SMDatareaderCryptoTokenSeq& remote_datareader_tokens,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool return_crypto_tokens(
        CryptoTokenSeq* crypto_tokens,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool return_crypto_tokens_impl(
        SMCryptoTokenSeq* crypto_tokens,
        SMSecurityException& exception);

    std::vector<uint8_t> KeyMaterialCDRSerialize(
            KeyMaterial_SM4_GCM_GMAC& key);

    bool KeyMaterialCDRDeserialize(
            KeyMaterial_SM4_GCM_GMAC& buffer,
            std::vector<uint8_t>* CDR);

    bool derive_psk_master_keys(
        SM4GCMGMAC_ParticipantCryptoHandle& local_participant,
        SMSecurityException& ex);

private:
    static bool psk_sign_payload(
        const std::vector<uint8_t>& master_sender_key32,
        const std::vector<uint8_t>& data,
        std::vector<uint8_t>& mac_out);

    static bool psk_encrypt_payload(
        const std::vector<uint8_t>& master_sender_key,
        const std::vector<uint8_t>& master_salt,
        uint64_t seq,
        const std::vector<uint8_t>& aad,
        std::vector<uint8_t>& payload);

    static bool psk_verify_payload(
        const std::vector<uint8_t>& master_sender_key32,
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& mac_in);

    static bool psk_decrypt_payload(
        const std::vector<uint8_t>& master_sender_key,
        const std::vector<uint8_t>& master_salt,
        uint64_t seq,
        const std::vector<uint8_t>& aad,
        std::vector<uint8_t>& payload);

    struct PskContext {
        std::vector<uint8_t> master_sender_key;
        std::vector<uint8_t> master_salt;
        std::atomic<uint64_t>& next_seq; // 由调用方维护的序列
    };

    struct ProtectedBlob {
        std::vector<uint8_t> payload;
        std::vector<uint8_t> mac;
    };


    bool protect_with_psk(
        SM4GCMGMAC_ParticipantCryptoHandle& pc,
        PskProtectionKind kind,
        const std::vector<uint8_t>& aad,
        const std::vector<uint8_t>& plaintext,
        ProtectedBlob& out,
        SMSecurityException& ex);

    bool unprotect_with_psk(
        SM4GCMGMAC_ParticipantCryptoHandle& pc,
        PskProtectionKind kind,
        uint64_t seq,
        const std::vector<uint8_t>& aad,
        const std::vector<uint8_t>& in_payload,
        const std::vector<uint8_t>& in_mac,
        std::vector<uint8_t>& out_plaintext,
        SMSecurityException& ex);
};
TRAVODDS_NAMESPACE_END
#endif // _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYEXCHANGE_H_