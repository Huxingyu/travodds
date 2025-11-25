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
#ifndef _SECURITY_AUTHENTICATION_PKIGM_H_
#define _SECURITY_AUTHENTICATION_PKIGM_H_

#include "auth/GMIdentityHandle.h"
#include "auth/GMHandshakeHandle.h"
#include "auth/GMHelper.h"
#include "SecurityException.h"
#include "sm/SMAlgorithm.hpp"
#include "security/authentication.h"
#include "PropertyPolicy.h"
#include "security/propertyqospolicy.h"
#include "common/log/logger.h"

using dds_security_sm::SM::SMAlgorithmImpl;

TRAVODDS_NAMESPACE_BEGIN

class PKIGM : public Authentication
    {
    public:
    
        std::shared_ptr<SMSecretHandle> generate_sharedsecret(
            EVP_PKEY* private_key,
            EVP_PKEY* public_key,
            SMSecurityException& exception) const;

        // 基类接口实现（适配器函数，带 override）
        ValidationResult_t validate_local_identity(
                IdentityHandle *local_identity_handle,
                BuiltinTopicKey_t &adjusted_participant_key,
                const DomainId_t domain_id,
                DomainParticipantQos *participant_qos,
                const BuiltinTopicKey_t candidate_participant_key,
                SecurityException *exception) override;

        // 实现函数（保持原有签名）
        ValidationResult_t validate_local_identity_impl(
                SMIdentityHandle **local_identity_handle,
                BuiltinTopicKey_t adjusted_participant_key,
                const DomainId_t domain_id,
                const DomainParticipantQos *participant_qos,
                const BuiltinTopicKey_t candidate_participant_key,
                SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        ValidationResult_t validate_remote_identity(
            IdentityHandle *remote_identity_handle,
            const IdentityHandle local_identity_handle,
            IdentityToken &remote_identity_token,
            const BuiltinTopicKey_t remote_participant_key,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        ValidationResult_t validate_remote_identity_impl(
            SMIdentityHandle **remote_identity_handle,
            const SMIdentityHandle& local_identity_handle,
            IdentityToken *remote_identity_token,
            const BuiltinTopicKey_t remote_participant_key,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        ValidationResult_t begin_handshake_request(
            HandshakeHandle *handshake_handle,
            HandshakeMessageToken *handshake_message_token,
            const IdentityHandle initiator_identity_handle,
            const IdentityHandle replier_identity_handle,
            const char* builtin_topic_data,
            unsigned int builtin_topic_data_length,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        ValidationResult_t begin_handshake_request_impl(
            SMHandshakeHandle **handshake_handle,
            HandshakeMessageToken **handshake_message_token,
            const SMIdentityHandle &initiator_identity_handle,
            SMIdentityHandle &replier_identity_handle,
            const char* builtin_topic_data,
            unsigned int builtin_topic_data_length,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        ValidationResult_t begin_handshake_reply(
            HandshakeHandle *handshake_handle,
            HandshakeMessageToken *handshake_message_out,
            HandshakeMessageToken *handshake_message_in,
            const IdentityHandle initiator_identity_handle,
            const IdentityHandle replier_identity_handle,
            const char* builtin_topic_data,
            unsigned int builtin_topic_data_length,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        ValidationResult_t begin_handshake_reply_impl(
            SMHandshakeHandle **handshake_handle,
            HandshakeMessageToken **handshake_message_out,
            HandshakeMessageToken &&handshake_message_in,
            SMIdentityHandle &initiator_identity_handle,
            const SMIdentityHandle &replier_identity_handle,
            const char* builtin_topic_data,
            unsigned int builtin_topic_data_length,
            SMSecurityException& exception);

        // 基类接口实现（适配器函数，带 override）
        ValidationResult_t process_handshake(
            HandshakeMessageToken *handshake_message_out,
            HandshakeMessageToken *handshake_message_in,
            const HandshakeHandle handshake_handle,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        ValidationResult_t process_handshake_impl(
            HandshakeMessageToken **handshake_message_out,
            HandshakeMessageToken &&handshake_message_in,
            SMHandshakeHandle &handshake_handle,
            SMSecurityException &exception);

        ValidationResult_t process_handshake_request(
            HandshakeMessageToken** handshake_message_out,
            HandshakeMessageToken&& handshake_message_in,
            GMHandshakeHandle& handshake_handle,
            SMSecurityException& exception);

        ValidationResult_t process_handshake_reply(
            HandshakeMessageToken** handshake_message_out,
            HandshakeMessageToken&& handshake_message_in,
            GMHandshakeHandle& handshake_handle,
            SMSecurityException& exception);

        // 基类接口实现（适配器函数，带 override）
        SharedSecretHandle get_shared_secret(
            const HandshakeHandle handshake_handle,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        std::shared_ptr<SMSecretHandle> get_shared_secret_impl(
            const SMHandshakeHandle& handshake_handle,
            SMSecurityException& exception) const;
        // bool return_identity_handle(
        //     const SMIdentityHandle identity_handle,
        //     SecurityException *exception) override;

        SMIdentityHandle* get_identity_handle(
            SMSecurityException&);
        
        // 基类接口实现（适配器函数，带 override）
        bool get_authenticated_peer_credential_token(
            AuthenticatedPeerCredentialToken *peer_credential_token,
            const HandshakeHandle handshake_handle,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        bool get_authenticated_peer_credential_token_impl(
            AuthenticatedPeerCredentialToken **peer_credential_token,
            const SMHandshakeHandle &handshake_handle,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        bool get_identity_token(
            IdentityToken *identity_token,
            const IdentityHandle handle,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        bool get_identity_token_impl(
            IdentityToken **identity_token,
            const SMIdentityHandle &handle,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        bool set_permissions_credential_and_token(
            IdentityHandle handle,
            const PermissionsCredentialToken *permissions_credential_token,
            const PermissionsToken *permissions_token,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        bool set_permissions_credential_and_token_impl(
            SMIdentityHandle &handle,
            const PermissionsCredentialToken &permissions_credential_token,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        bool return_identity_token(
            IdentityToken *token,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        bool return_identity_token_impl(
            IdentityToken *token,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        bool return_authenticated_peer_credential_token(
            AuthenticatedPeerCredentialToken *peer_credential_token,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        bool return_authenticated_peer_credential_token_impl(
            AuthenticatedPeerCredentialToken *peer_credential_token,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        bool return_handshake_handle(
            const HandshakeHandle handshake_handle,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        bool return_handshake_handle_impl(
            SMHandshakeHandle *handshake_handle,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        bool return_identity_handle(
            const IdentityHandle identity_handle,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        bool return_identity_handle_impl(
            SMIdentityHandle *identity_handle,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        bool return_sharedsecret_handle(
            const SharedSecretHandle sharedsecret_handle,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        bool return_sharedsecret_handle_impl(
            std::shared_ptr<SMSecretHandle>& sharedsecret_handle,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        bool return_handshake_token(
            HandshakeMessageToken *handshake_token,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        bool return_handshake_token_impl(
            HandshakeMessageToken *handshake_token,
            SMSecurityException &exception);

        // 基类接口实现（适配器函数，带 override）
        bool set_listener(
            AuthenticationListener *listener,
            SecurityException *exception) override;

        // 实现函数（保持原有签名）
        bool set_listener_impl(
            AuthenticationListener *listener,
            SMSecurityException &exception);

        bool return_identity_status_token(
            IdentityToken *token,
            SMSecurityException &exception);

        std::unique_ptr<Pkcs11Provider> pkcs11_provider;
    };

TRAVODDS_NAMESPACE_END

#endif // _SECURITY_AUTHENTICATION_PKIGM_H_