#ifndef _SECURITY_AUTHENTICATION_PKIGM_H_
#define _SECURITY_AUTHENTICATION_PKIGM_H_

#include "auth/GMIdentityHandle.h"
#include "auth/GMHandshakeHandle.h"
#include "auth/GMHelper.h"
#include "Security/SecurityException.h"
#include "sm/SMAlgorithm.hpp"
#include "Security/Authentication.h"
#include "PropertyPolicy.h"
#include "security/propertyqospolicy.h"
#include "common/log/logger.h"

using dds_security_sm::SM::SMAlgorithmImpl;

TRAVODDS_NAMESPACE_BEGIN

class PKIGM : public Authentication
    {
    public:
    
        std::shared_ptr<SecretHandle> generate_sharedsecret(
            EVP_PKEY* private_key,
            EVP_PKEY* public_key,
            SecurityException& exception) const;

        ValidationResult_t validate_local_identity(
                IdentityHandle **local_identity_handle,
                BuiltinTopicKey_t adjusted_participant_key,
                const DomainId_t domain_id,
                const DomainParticipantQos *participant_qos,
                const BuiltinTopicKey_t candidate_participant_key,
                SecurityException &exception) override;

        ValidationResult_t validate_remote_identity(
            IdentityHandle **remote_identity_handle,
            const IdentityHandle& local_identity_handle,
            IdentityToken *remote_identity_token,
            const BuiltinTopicKey_t remote_participant_key,
            SecurityException &exception) override;

        ValidationResult_t begin_handshake_request(
            HandshakeHandle **handshake_handle,
            HandshakeMessageToken **handshake_message_token,
            const IdentityHandle &initiator_identity_handle,
            IdentityHandle &replier_identity_handle,
            const char* builtin_topic_data,
            unsigned int builtin_topic_data_length,
            SecurityException &exception) override;

        ValidationResult_t begin_handshake_reply(
            HandshakeHandle **handshake_handle,
            HandshakeMessageToken **handshake_message_out,
            HandshakeMessageToken &&handshake_message_in,
            IdentityHandle &initiator_identity_handle,
            const IdentityHandle &replier_identity_handle,
            const char* builtin_topic_data,
            unsigned int builtin_topic_data_length,
            SecurityException& exception) override;

        ValidationResult_t process_handshake(
            HandshakeMessageToken **handshake_message_out,
            HandshakeMessageToken &&handshake_message_in,
            HandshakeHandle &handshake_handle,
            SecurityException &exception) override;

        ValidationResult_t process_handshake_request(
            HandshakeMessageToken** handshake_message_out,
            HandshakeMessageToken&& handshake_message_in,
            GMHandshakeHandle& handshake_handle,
            SecurityException& exception);

        ValidationResult_t process_handshake_reply(
            HandshakeMessageToken** handshake_message_out,
            HandshakeMessageToken&& handshake_message_in,
            GMHandshakeHandle& handshake_handle,
            SecurityException& exception);

        std::shared_ptr<SecretHandle> get_shared_secret(
            const HandshakeHandle& handshake_handle,
            SecurityException& exception) const override;
        // bool return_identity_handle(
        //     const IdentityHandle identity_handle,
        //     SecurityException *exception) override;

        IdentityHandle* get_identity_handle(
            SecurityException&);
        
        bool get_authenticated_peer_credential_token(
            AuthenticatedPeerCredentialToken **peer_credential_token,
            const HandshakeHandle &handshake_handle,
            SecurityException &exception) override;

        bool get_identity_token(
            IdentityToken **identity_token,
            const IdentityHandle &handle,
            SecurityException &exception) override;

        bool set_permissions_credential_and_token(
            IdentityHandle &handle,
            const PermissionsCredentialToken &permissions_credential_token,
            SecurityException &exception) override;

        bool return_identity_token(
            IdentityToken *token,
            SecurityException &exception) override;

        bool return_authenticated_peer_credential_token(
            AuthenticatedPeerCredentialToken *peer_credential_token,
            SecurityException &exception) override;

        bool return_handshake_handle(
            HandshakeHandle *handshake_handle,
            SecurityException &exception) override;

        bool return_identity_handle(
            IdentityHandle *identity_handle,
            SecurityException &exception) override;

        bool return_sharedsecret_handle(
            std::shared_ptr<SecretHandle>& sharedsecret_handle,
            SecurityException &exception) override;

        bool return_handshake_token(
            HandshakeMessageToken *handshake_token,
            SecurityException &exception) override;

        bool set_listener(
            AuthenticationListener *listener,
            SecurityException &exception) override;

        std::unique_ptr<Pkcs11Provider> pkcs11_provider;
    };

TRAVODDS_NAMESPACE_END

#endif // _SECURITY_AUTHENTICATION_PKIGM_H_