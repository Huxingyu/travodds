#ifndef _SECURITY_ACCESSCONTROL_PERMISSIONS_H_
#define _SECURITY_ACCESSCONTROL_PERMISSIONS_H_

#include "type/basictype.h"
#include "rtps/common/Token.h"
#include "PropertyPolicy.h"
#include "rtps/security/common/Handle.h"
#include "Security/AccessControl.h"


TRAVODDS_NAMESPACE_BEGIN

class Permissions : public AccessControl
{
    public:

    virtual ~Permissions() = default;

    PermissionsHandle* validate_local_permissions(
        Authentication& auth_plugin,
        const IdentityHandle& identity,
        const DomainId_t domain_id,
        const DomainParticipantQos& participant_qos,
        SecurityException& exception) override;

    PermissionsHandle* get_permissions_handle(
        SecurityException& exception);

    PermissionsHandle* validate_remote_permissions(
        Authentication& auth_plugin,
        const IdentityHandle& local_identity_handle,
        const PermissionsHandle& local_permissions_handle,
        const IdentityHandle& remote_identity_handle,
        const PermissionsToken& remote_permissions_token,
        const AuthenticatedPeerCredentialToken& remote_credential_token,
        SecurityException& exception) override;

    bool check_create_participant(
        const PermissionsHandle& permissions_handle,
        const DomainId_t domain_id,
        const DomainParticipantQos& qos,
        SecurityException& exception) override;
};
TRAVODDS_NAMESPACE_END
#endif // _SECURITY_ACCESSCONTROL_PERMISSIONS_H_