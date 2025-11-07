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
#ifndef __SECURITY_ACCESSCONTROL_ACCESSPERMISSIONSHANDLE_H__
#define __SECURITY_ACCESSCONTROL_ACCESSPERMISSIONSHANDLE_H__

#include <openssl/x509.h>
#include <string>
#include <map>

#include "Handle.h"
#include "security/securitycommon.h"
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