#ifndef PKIHANDSHAKEHANDLE_H
#define PKIHANDSHAKEHANDLE_H

#include <openssl/evp.h>
#include "rtps/security/common/SharedSecretHandle.h"
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

    std::shared_ptr<SharedSecretHandle> local_sharedsecret_;
    std::shared_ptr<SharedSecretHandle> remote_sharedsecret_;
};

typedef HandleImpl<GMHandshake> GMHandshakeHandle;

TRAVODDS_NAMESPACE_END
#endif // PKIHANDSHAKEHANDLE_H
