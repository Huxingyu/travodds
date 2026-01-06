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
#ifndef _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYFACTORY_H_
#define _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYFACTORY_H_

#include "security/cryptokeyfactory.h"
#include "cryptography/SM4GCMGMAC_Types.h"
#include "PropertyPolicy.h"

TRAVODDS_NAMESPACE_BEGIN

struct ParticipantCryptoHandleDeleter;

class SM4GCMGMAC_KeyFactory : public CryptoKeyFactory, public std::enable_shared_from_this<SM4GCMGMAC_KeyFactory>
{
    friend ParticipantCryptoHandleDeleter;

    void release_participant(SM4GCMGMAC_ParticipantCryptoHandle& key);

    std::shared_ptr<SMDatawriterCryptoHandle> get_datawriter_handle();
    std::shared_ptr<SMDatareaderCryptoHandle> get_datareader_handle();
public:
    SM4GCMGMAC_KeyFactory();

    // 基类接口实现（适配器函数，带 override）
    ParticipantCryptoHandle register_local_participant(
        IdentityHandle participant_identity,
        PermissionsHandle participant_permissions,
        const PropertySeq* participant_properties,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    std::shared_ptr<SMParticipantCryptoHandle> register_local_participant_impl(
        const SMIdentityHandle& participant_identity,
        const SMPermissionsHandle& participant_permissions,
        const PropertySeq& participant_properties,
        const ParticipantSecurityAttributes& participant_security_attributes,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    ParticipantCryptoHandle register_matched_remote_participant(
        IdentityHandle remote_participant_identity,
        ParticipantCryptoHandle local_participant_crypto_handle,
        SharedSecretHandle sharesecret,
        PermissionsHandle remote_participant_permissions,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    std::shared_ptr<SMParticipantCryptoHandle> register_matched_remote_participant_impl(
        const SMIdentityHandle& remote_participant_identity,
        const SMParticipantCryptoHandle& local_participant_crypto_handle,
        const SMSecretHandle& sharesecret,
        const SMPermissionsHandle& remote_participant_permissions,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    DatareaderCryptoHandle register_local_datareader(
        ParticipantCryptoHandle participant_crypto,
        const PropertySeq* datareader_properties,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    SMDatareaderCryptoHandle* register_local_datareader_impl(
        SMParticipantCryptoHandle& participant_crypto,
        const PropertySeq& datareader_properties,
        const EndpointSecurityAttributes& datareader_security_attributes,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    DatawriterCryptoHandle register_local_datawriter(
        ParticipantCryptoHandle participant_crypto,
        const PropertySeq* datawriter_properties,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    SMDatawriterCryptoHandle* register_local_datawriter_impl(
        SMParticipantCryptoHandle& participant_crypto,
        const PropertySeq& datawriter_properties,
        const EndpointSecurityAttributes& datawriter_sec_attr,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    DatareaderCryptoHandle register_matched_remote_datareader(
        DatawriterCryptoHandle local_datawriter_crypto_handle,
        ParticipantCryptoHandle remote_participant_crypto,
        SharedSecretHandle shared_secret,
        bool relay_only,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    SMDatareaderCryptoHandle* register_matched_remote_datareader_impl(
        SMDatawriterCryptoHandle& local_datawriter_crypto_handle,
        SMParticipantCryptoHandle& remote_participant_crypto,
        const SMSharedSecretHandle& shared_secret,
        const bool relay_only,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    DatawriterCryptoHandle register_matched_remote_datawriter(
        DatareaderCryptoHandle local_datareader_crypto_handle,
        ParticipantCryptoHandle remote_participant_crypto,
        SharedSecretHandle shared_secret,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    SMDatawriterCryptoHandle* register_matched_remote_datawriter_impl(
        SMDatareaderCryptoHandle& local_datareader_crypto_handle,
        SMParticipantCryptoHandle& remote_participant_crypto,
        const SMSharedSecretHandle& shared_secret,
        SMSecurityException& exception);

    // 基类接口实现（适配器函数，带 override）
    bool unregister_participant(
        ParticipantCryptoHandle participant_crypto_handle,
        SecurityException* exception) override;

    bool unregister_datawriter(
        DatawriterCryptoHandle datawriter_crypto_handle,
        SecurityException* exception) override;

    bool unregister_datareader(
        DatareaderCryptoHandle datareader_crypto_handle,
        SecurityException* exception) override;

    // 实现函数（保持原有签名）
    bool unregister_participant_impl(
        std::shared_ptr<SMParticipantCryptoHandle>& participant_crypto_handle,
        SMSecurityException& exception);

    bool unregister_datawriter_impl(
        std::shared_ptr<SMDatawriterCryptoHandle>& datawriter_crypto_handle,
        SMSecurityException& exception);

    bool unregister_datareader_impl(
        std::shared_ptr<SMDatareaderCryptoHandle>& datareader_crypto_handle,
        SMSecurityException& exception);
        
private:
    void create_key(
            KeyMaterial_SM4_GCM_GMAC& key,
            bool encrypt_then_sign);

    CryptoTransformKeyId make_unique_KeyId();

    void release_key_id(CryptoTransformKeyId id);

    std::vector<CryptoTransformKeyId> m_CryptoTransformKeyIds;
};

struct ParticipantCryptoHandleDeleter
{
    ParticipantCryptoHandleDeleter(
            SM4GCMGMAC_KeyFactory& factory);

    void operator ()(
            SM4GCMGMAC_ParticipantCryptoHandle* pk);

    std::weak_ptr<SM4GCMGMAC_KeyFactory> factory_;
};

TRAVODDS_NAMESPACE_END

#endif // _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYFACTORY_H_