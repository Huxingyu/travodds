#include "travodds_dll.h"
#include "PKcs11Provider.h"
#include <openssl/ssl.h>
#include <openssl/engine.h>
#include <openssl/ui.h>
#include <openssl/err.h>

USING_TRAVODDS_NAMESPACE

#define _SecurityException_(str) SMSecurityException(std::string(str) + " (" __FILE__ ":" + std::to_string(__LINE__) + ")")

Pkcs11Provider::Pkcs11Provider()
{
    // 加载OpenSSL错误信息字符串，便于后续输出可读的错误提示
    SSL_load_error_strings();                
    // 初始化OpenSSL加密库，加载算法等核心组件
    SSL_library_init();                      

    // 创建用户交互方法，用于在需要时获取PIN码
    // 当环境变量和URI中均未提供PIN时，将使用此方法获取
    ui_method_ = UI_create_method("OpenSSL application user interface");
    // 设置UI方法的回调函数：打开UI交互
    UI_method_set_opener(ui_method_, nullptr);
    // 设置UI方法的回调函数：读取用户输入（如PIN码）
    UI_method_set_reader(ui_method_, nullptr);
    // 设置UI方法的回调函数：关闭UI交互
    UI_method_set_closer(ui_method_, nullptr);

    // 加载OpenSSL内置引擎
    ENGINE_load_builtin_engines();
    // 获取PKCS#11引擎实例
    pkcs11_ = ENGINE_by_id("pkcs11");
    if (!pkcs11_)  // 若获取引擎失败
    {
        has_initialization_error_ = true;  // 标记初始化错误
        initialization_exception_ = _SecurityException_("获取'pkcs11'引擎失败");
        return;
    }

    // 使用PIN码初始化PKCS#11引擎
    if (!ENGINE_init(pkcs11_))
    {
        has_initialization_error_ = true;
        initialization_exception_ = _SecurityException_("初始化HSM提供程序库失败");
        ENGINE_free(pkcs11_);  // 释放引擎资源
        return;
    }
}

Pkcs11Provider::~Pkcs11Provider()
{
    ENGINE_finish(pkcs11_);
    ENGINE_free(pkcs11_);

    if (ui_method_)
    {
        UI_destroy_method(ui_method_);
    }
}

EVP_PKEY* Pkcs11Provider::load_private_key(
    X509* certificate,
    const std::string& pkey,
    const std::string& /*password*/,
    SMSecurityException& exception)
{
    if (has_initialization_error_)
    {
        exception = initialization_exception_;
        return nullptr;
    }

    EVP_PKEY* returnedValue = ENGINE_load_private_key(pkcs11_, pkey.c_str(), ui_method_, nullptr);
    if (!returnedValue)
    {
        exception = _SecurityException_(std::string("Error opening the private key ") + pkey.substr(7));
        return returnedValue;
    }

    // Verify private key.
    if (!X509_check_private_key(certificate, returnedValue))
    {
        exception = _SecurityException_(std::string("Error verifying private key ") + pkey.substr(7)
                        + "\n ERROR: " + ERR_error_string(ERR_get_error(), nullptr));
        EVP_PKEY_free(returnedValue);
        returnedValue = nullptr;
    }

    return returnedValue;
}