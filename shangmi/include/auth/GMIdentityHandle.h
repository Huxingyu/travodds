#ifndef _SECURITY_AUTHENTICATION_GMIDENTITYHANDLE_H_
#define _SECURITY_AUTHENTICATION_GMIDENTITYHANDLE_H_
#include <openssl/x509.h>
#include <string>
#include "Security/SecurityCommon.h"
#include "rtps/basictypertps.h"
#include "rtps/common/Token.h"
#include "rtps/security/common/Handle.h"

using std::string;
TRAVODDS_NAMESPACE_BEGIN

static const char* const RSA_SHA256 = "RSASSA-PSS-SHA256";
static const char* const ECDSA_SHA256 = "ECDSA-SHA256";
static const char* const SM2_SM3 = "SM2-with-SM3";

static const char* const RSA_SHA256_FOR_TOKENS = "RSA-2048";
static const char* const ECDSA_SHA256_FOR_TOKENS = "EC-prime256v1";
static const char* const SM2_SM3_FOR_TOKENS = "DDS:GM:SM2-sm2p256v1";

static const char* const DH_2048_256 = "DH+MODP-2048-256";
static const char* const ECDH_prime256v1 = "ECDH+prime256v1-CEUM";
static const char* const SM2_KEY_AGREEMENT = "SM2-KE+sm2p256v1";

    class PKIIdentity
    {
    public:
    
        PKIIdentity()
            : store_(nullptr)
            , cert_(nullptr)
            , pkey_(nullptr)
            , cert_content_(nullptr)
            , kagree_alg_("SM2")
            , there_are_crls_(false)
        {
        }
    
        ~PKIIdentity()
        {
            if (store_ != nullptr)
            {
                X509_STORE_free(store_);
            }
    
            if (cert_ != nullptr)
            {
                X509_free(cert_);
            }
    
            if (pkey_ != nullptr)
            {
                EVP_PKEY_free(pkey_);
            }
    
            if (cert_content_ != nullptr)
            {
                BUF_MEM_free(cert_content_);
            }
        }
    
        static const char* const class_id_;
    
        X509_STORE* store_;
        X509* cert_;
        EVP_PKEY* pkey_;
        GUID_t participant_key_;
        BUF_MEM* cert_content_;
        std::string sn;
        std::string algo;
        std::string sign_alg_;
        std::string kagree_alg_;
        std::string cert_sn_;
        std::string cert_sn_rfc2253_;
        bool there_are_crls_;
        IdentityToken identity_token_;
        PermissionsCredentialToken permissions_credential_token_;
    };
    
    class PKIDH;
    
    typedef HandleImpl<PKIIdentity> GMIdentityHandle;

TRAVODDS_NAMESPACE_END

#endif // _SECURITY_AUTHENTICATION_GMIDENTITYHANDLE_H_