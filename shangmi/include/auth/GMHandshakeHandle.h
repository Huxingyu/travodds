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
#ifndef PKIHANDSHAKEHANDLE_H
#define PKIHANDSHAKEHANDLE_H

#include <openssl/evp.h>
#include "SharedSecretHandle.h"
#include "GMIdentityHandle.h"
#include <string>
#include <map>

TRAVODDS_NAMESPACE_BEGIN
class GMHandshake
{
public:

    GMHandshake() = default;

    ~GMHandshake()
    {
        if (sm2keys_ != nullptr)
        {
            EVP_PKEY_free(sm2keys_);
        }

        if (peerkeys != nullptr)
        {
            EVP_PKEY_free(peerkeys);
        }
    }

    static const char* const class_id_;

    string kagree_alg_;
    EVP_PKEY* sm2keys_ = { nullptr };
    EVP_PKEY* peerkeys = { nullptr };

    EVP_PKEY* rsm2keys = { nullptr };
    EVP_PKEY* rPeerkeys = { nullptr };

    //map<GUID_t, EVP_PKEY*> remotePeerKeyList;

    //const PKIIdentityHandle* local_identity_handle  = { nullptr };
    const GMIdentityHandle* local_identity_handle_  = { nullptr };
    GMIdentityHandle* remote_identity_handle_ = { nullptr };
    HandshakeMessageToken handshake_message_;

    std::shared_ptr<SMSharedSecretHandle> local_sharedsecret_;
    std::shared_ptr<SMSharedSecretHandle> remote_sharedsecret_;
};

typedef HandleImpl<GMHandshake> GMHandshakeHandle;

TRAVODDS_NAMESPACE_END
#endif // PKIHANDSHAKEHANDLE_H
