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
#ifndef _SECURITY_ACCESSCONTROL_PERMISSIONS_H_
#define _SECURITY_ACCESSCONTROL_PERMISSIONS_H_

#include "type/basictype.h"
#include "security/securitycommon.h"
#include "PropertyPolicy.h"
#include "Handle.h"
#include "security/accesscontrol.h"
#include "SecurityException.h"


TRAVODDS_NAMESPACE_BEGIN

class Permissions : public AccessControl
{
    public:

    virtual ~Permissions() = default;

    // 基类接口实现（带 override，匹配基类签名）
    PermissionsHandle validate_local_permissions(
        Authentication *auth_plugin,
        const IdentityHandle identity,
        const DomainId_t domain_id,
        const DomainParticipantQos *participant_qos,
        SecurityException *exception) override;

    PermissionsHandle validate_remote_permissions(
        Authentication *auth_plugin,
        const IdentityHandle local_identity_handle,
        const IdentityHandle remote_identity_handle,
        const PermissionsToken *remote_permissions_token,
        AuthenticatedPeerCredentialToken *remote_credential_token,
        SecurityException *exception) override;

    bool check_create_participant(
        const PermissionsHandle permissions_handle,
        const DomainId_t domain_id,
        const DomainParticipantQos *qos,
        SecurityException *exception) override;

    // 派生类自己的接口（用于内部调用，保持原有签名）
    SMPermissionsHandle* validate_local_permissions_impl(
        Authentication& auth_plugin,
        const SMIdentityHandle& identity,
        const DomainId_t domain_id,
        const DomainParticipantQos& participant_qos,
        SMSecurityException& exception);

    SMPermissionsHandle* get_permissions_handle(
        SMSecurityException& exception);

    SMPermissionsHandle* validate_remote_permissions_impl(
        Authentication& auth_plugin,
        const SMIdentityHandle& local_identity_handle,
        const SMPermissionsHandle& local_permissions_handle,
        const SMIdentityHandle& remote_identity_handle,
        const PermissionsToken& remote_permissions_token,
        const AuthenticatedPeerCredentialToken& remote_credential_token,
        SMSecurityException& exception);

    bool check_create_participant_impl(
        const SMPermissionsHandle& permissions_handle,
        const DomainId_t domain_id,
        const DomainParticipantQos& qos,
        SMSecurityException& exception);
};
TRAVODDS_NAMESPACE_END
#endif // _SECURITY_ACCESSCONTROL_PERMISSIONS_H_