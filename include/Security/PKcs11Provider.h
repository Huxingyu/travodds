#ifndef PKCS11PROVIDER_H__
#define PKCS11PROVIDER_H__

#include "Security/SecurityException.h"
#include <openssl/evp.h>
#include <openssl/engine.h>
#include <openssl/ui.h>
#include <openssl/x509.h>

TRAVODDS_NAMESPACE_BEGIN

class Pkcs11Provider
{
public:

    EVP_PKEY* load_private_key(
            X509* certificate,
            const std::string& file,
            const std::string& password,
            SecurityException& exception);

    Pkcs11Provider();

    ~Pkcs11Provider();

private:

    EVP_PKEY* load_private_key_impl(
            X509* certificate,
            const std::string& file,
            const std::string& password,
            SecurityException& exception);

    SecurityException initialization_exception_;
    bool has_initialization_error_ = false;
    ENGINE* pkcs11_ = nullptr;
    UI_METHOD* ui_method_ = nullptr;
};

TRAVODDS_NAMESPACE_END

#endif // PKCS11PROVIDER_H__