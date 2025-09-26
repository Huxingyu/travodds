#ifndef __SECURITY_ACCESSCONTROL_ACCESSPERMISSIONSHANDLE_H__
#define __SECURITY_ACCESSCONTROL_ACCESSPERMISSIONSHANDLE_H__

#include <openssl/x509.h>
#include <string>
#include <map>

#include "rtps/security/common/Handle.h"
#include "rtps/common/Token.h"
#include "accesscontrol/Permissions.h"
#include "accesscontrol/PermissionsTypes.h"
#include "accesscontrol/ParticipantSecurityAttributes.h"

TRAVODDS_NAMESPACE_BEGIN

class AccessPermissions
{
public:

    AccessPermissions()
        : store_(nullptr)
        , there_are_crls_(false)
    {
    }

    ~AccessPermissions()
    {
        if (store_ != nullptr)
        {
            X509_STORE_free(store_);
        }
    }

    static const char* const class_id_;

    X509_STORE* store_;
    std::string sn;
    std::string algo;
    bool there_are_crls_;
    PermissionsToken permissions_token_;
    PermissionsCredentialToken permissions_credential_token_;
    ParticipantSecurityAttributes governance_rule_;
    std::vector<std::pair<std::string, EndpointSecurityAttributes>> governance_topic_rules_;
    Grant grant;
};

class Permissions;

typedef HandleImpl<AccessPermissions> AccessPermissionsHandle;

TRAVODDS_NAMESPACE_END

#endif // __SECURITY_ACCESSCONTROL_ACCESSPERMISSIONSHANDLE_H__