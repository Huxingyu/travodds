#ifndef _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYFACTORY_H_
#define _SECURITY_AUTHENTICATION_SM4GCMGMAC_KEYFACTORY_H_

#include "Security/CryptoKeyFactory.h"
#include "cryptography/SM4GCMGMAC_Types.h"
#include "security/propertyqospolicy.h"

TRAVODDS_NAMESPACE_BEGIN

struct ParticipantCryptoHandleDeleter;

class SM4GCMGMAC_KeyFactory : public CryptoKeyFactory, public std::enable_shared_from_this<SM4GCMGMAC_KeyFactory>
{
    friend ParticipantCryptoHandleDeleter;

    void release_participant(SM4GCMGMAC_ParticipantCryptoHandle& key);

    std::shared_ptr<DatawriterCryptoHandle> get_datawriter_handle();
    std::shared_ptr<DatareaderCryptoHandle> get_datareader_handle();
public:
    SM4GCMGMAC_KeyFactory();

    std::shared_ptr<ParticipantCryptoHandle> register_local_participant(
        const IdentityHandle& participant_identity,
        const PermissionsHandle& participant_permissions,
        const PropertySeq& participant_properties,
        const ParticipantSecurityAttributes& participant_security_attributes,
        SecurityException& exception) override;

    std::shared_ptr<ParticipantCryptoHandle> register_matched_remote_participant(
        const IdentityHandle& remote_participant_identity,
        const ParticipantCryptoHandle& local_participant_crypto_handle,
        const SecretHandle& sharesecret,
        const PermissionsHandle& remote_participant_permissions,
        SecurityException& exception) override;

    DatareaderCryptoHandle* register_local_datareader(
        ParticipantCryptoHandle& participant_crypto,
        const PropertySeq& datareader_properties,
        const EndpointSecurityAttributes& datareader_security_attributes,
        SecurityException& exception) override;

    DatawriterCryptoHandle* register_local_datawriter(
        ParticipantCryptoHandle& participant_crypto,
        const PropertySeq& datawriter_properties,
        const EndpointSecurityAttributes& datawriter_sec_attr,
        SecurityException& exception) override;

    DatareaderCryptoHandle* register_matched_remote_datareader(
        DatawriterCryptoHandle& local_datawriter_crypto_handle,
        ParticipantCryptoHandle& remote_participant_crypto,
        const SharedSecretHandle& shared_secret,
        const bool relay_only,
        SecurityException& exception) override;

    DatawriterCryptoHandle* register_matched_remote_datawriter(
        DatareaderCryptoHandle& local_datareader_crypto_handle,
        ParticipantCryptoHandle& remote_participant_crypto,
        const SharedSecretHandle& shared_secret,
        SecurityException& exception) override;

    bool unregister_participant(
        std::shared_ptr<ParticipantCryptoHandle>& participant_crypto_handle,
        SecurityException& exception) override;

    bool unregister_datawriter(
        std::shared_ptr<DatawriterCryptoHandle>& datawriter_crypto_handle,
        SecurityException& exception) override;

    bool unregister_datareader(
        std::shared_ptr<DatareaderCryptoHandle>& datareader_crypto_handle,
        SecurityException& exception) override;
        
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