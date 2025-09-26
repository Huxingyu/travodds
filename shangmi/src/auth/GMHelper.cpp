#include "travodds_dll.h"
#include "auth/GMHelper.h"
#include "auth/PKIGM.h"
#include <openssl/cms.h>
#include <openssl/x509_vfy.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/params.h>
#include <openssl/core_names.h>

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION " (" __FILE__ ":" S2(__LINE__) ")"
#define _SecurityException_(str) SecurityException(std::string(str) + LOCATION)

USING_TRAVODDS_NAMESPACE
using namespace dds_security_sm::SM;

// 删除器: 用于释放 i2d_* 函数分配的内存
// struct DerBufferDeleter {
//     void operator()(unsigned char* ptr) const {
//         OPENSSL_free(ptr);
//     }
// };


//  用于释放 EVP_MD 对象
// struct EvpMdDeleter {
//     void operator()(EVP_MD* ptr) const {
//         EVP_MD_free(ptr);
//     }
// };

// 删除器:用于管理 EVP_PKEY_CTX 对象
// struct EvpPkeyCtxDeleter {
//     void operator()(EVP_PKEY_CTX* ptr) const {
//         EVP_PKEY_CTX_free(ptr);
//     }
// };

static std::string getOpenSSLError() {
    std::string error_msg;
    unsigned long err_code;
    char err_buf[256];

    while ((err_code = ERR_get_error()) != 0) {
        ERR_error_string_n(err_code, err_buf, sizeof(err_buf));
        if (!error_msg.empty()) {
            error_msg += "\n";
        }
        error_msg += err_buf;
        
        // 添加错误码信息
        char hex_buf[32];
        snprintf(hex_buf, sizeof(hex_buf), " (0x%lx)", err_code);
        error_msg += hex_buf;
    }
    return error_msg.empty() ? "Unknown OpenSSL error" : error_msg;
}

static int private_key_password_callback(
    char* buf,
    int bufsize,
    int /*verify*/,
    const char* password)
{
    assert(password != nullptr);

    int returnedValue = static_cast<int>(strlen(password));

    if (returnedValue > bufsize)
    {
        returnedValue = bufsize;
    }

    memcpy(buf, password, returnedValue);
    return returnedValue;
}

X509_STORE* GMHelper::load_sm2_ca(
    const std::string& identity_ca,
    bool& there_are_crls,
    std::string& ca_sn,
    std::string& ca_algo,
    SecurityException& exception){
        if (identity_ca.size() >= 7 && identity_ca.compare(0, 7, "file://") == 0)
        {
            X509_STORE* store = X509_STORE_new();

            if (store != nullptr)
            {
                BIO* in = BIO_new(BIO_s_file());
        
                if (in != nullptr)
                {
                    if (BIO_read_filename(in, identity_ca.substr(7).c_str()) > 0)
                    {
                        STACK_OF(X509_INFO) * inf = PEM_X509_INFO_read_bio(in, NULL, NULL, NULL);
                        X509* ca_cert = nullptr;
        
                        if (inf != nullptr)
                        {
                            int i, count = 0;
                            there_are_crls = false;
        
                            for (i = 0; i < sk_X509_INFO_num(inf); i++)
                            {
                                X509_INFO* itmp = sk_X509_INFO_value(inf, i);
        
                                if (itmp->x509)
                                {
                                    if (nullptr == ca_cert)
                                    {
                                        ca_cert = itmp->x509;
                                    }
        
                                    // 取出 subject name
                                    if (ca_sn.empty())
                                    {
                                        X509_NAME* ca_subject_name = X509_get_subject_name(itmp->x509);
                                        assert(ca_subject_name != nullptr);
                                        char* ca_subject_name_str = X509_NAME_oneline(ca_subject_name, 0, 0);
                                        assert(ca_subject_name_str != nullptr);
                                        ca_sn = ca_subject_name_str;
                                        OPENSSL_free(ca_subject_name_str);
                                    }
        
                                    // 取出 signature algorithm
                                    if (ca_algo.empty())
                                    {
                                        if (get_signature_algorithm(itmp->x509, ca_algo, exception))
                                        {
                                            X509_STORE_add_cert(store, itmp->x509);
                                            count++;
                                        }
                                    }
                                    else
                                    {
                                        X509_STORE_add_cert(store, itmp->x509);
                                        count++;
                                    }
                                }
                                if (itmp->crl)
                                {
                                    X509_STORE_add_crl(store, itmp->crl);
                                    there_are_crls = true;
                                }
                            }
        
                            sk_X509_INFO_pop_free(inf, X509_INFO_free);
        
                            if (count > 0)
                            {
                                // Verify CA certificate.
                                unsigned long flags = 0;
                                flags |= X509_V_FLAG_CHECK_SS_SIGNATURE | X509_V_FLAG_POLICY_CHECK;
                                flags |= X509_V_FLAG_X509_STRICT;
                                X509_STORE_CTX* ctx = X509_STORE_CTX_new();
                                if (nullptr != ctx)
                                {
                                    X509_STORE_CTX_init(ctx, store, ca_cert, NULL);
                                    X509_STORE_CTX_set_flags(ctx, flags);
                                    if (X509_verify_cert(ctx) == 1)
                                    {
                                        X509_STORE_CTX_free(ctx);
                                        BIO_free(in);
                                        return store;
                                    }
        
                                    int error_code = X509_STORE_CTX_get_error(ctx);
                                    const char* error_msg = X509_verify_cert_error_string(error_code);
        
                                    exception = _SecurityException_(
                                            "Error '" + std::to_string(error_code) + "' verifying CA certificate for " +
                                            ca_sn + ": " + error_msg);
                                    X509_STORE_CTX_free(ctx);
                                }
                                else
                                {
                                    exception = _SecurityException_("Error creating X509 store context");
                                }
                            }
                        }
                        else
                        {
                            exception = _SecurityException_(std::string("OpenSSL library cannot read X509 info in file ") + identity_ca.substr(7));
                        }
                    }
                    else
                    {
                        exception = _SecurityException_(std::string("OpenSSL library cannot read file ") + identity_ca.substr(7));
                    }
        
                    BIO_free(in);
                }
                else
                {
                    exception = _SecurityException_("OpenSSL library cannot allocate file");
                }
        
                X509_STORE_free(store);
            }
            else
            {
                exception = _SecurityException_("Creation of X509 storage");
            }
        
            return nullptr;
        }
    
        exception = _SecurityException_(std::string("Unsupported URI format ") + identity_ca);
        return nullptr;
}

 X509_CRL* GMHelper::load_sm2_crl(
    const std::string& identity_crl,
    SecurityException& exception)
{
    if (identity_crl.size() >= 7 && identity_crl.compare(0, 7, "file://") == 0)
    {
        X509_CRL* returnedValue = nullptr;

        // 创建文件BIO
        BIO* in = BIO_new(BIO_s_file());
    
        if (in != nullptr)
        {
            if (BIO_read_filename(in, identity_crl.substr(7).c_str()) > 0)
            {
                // 从PEM格式读取CRL
                returnedValue = PEM_read_bio_X509_CRL(in, NULL, NULL, NULL);
            }
            else
            {
                exception = _SecurityException_(std::string("OpenSSL library cannot read file ") + identity_crl.substr(7));
            }
            // 释放BIO资源
            BIO_free(in);
        }
        else
        {
            exception = _SecurityException_("OpenSSL library cannot allocate file");
        }
    
        return returnedValue;
    }

    exception = _SecurityException_(std::string("Unsupported URI format ") + identity_crl);
    return nullptr;
}

X509* GMHelper::load_sm2_certificate(
    const std::string& identity_cert,
    SecurityException& exception)
{
    if (identity_cert.size() >= 7 && identity_cert.compare(0, 7, "file://") == 0)
    {
        X509* returnedValue = nullptr;
        BIO* in = BIO_new(BIO_s_file());
    
        if (in != nullptr)
        {
            if (BIO_read_filename(in, identity_cert.substr(7).c_str()) > 0)
            {
                returnedValue = PEM_read_bio_X509_AUX(in, NULL, NULL, NULL);
            }
            else
            {
                exception = _SecurityException_(std::string("OpenSSL library cannot read file ") + identity_cert.substr(7));
            }
    
            BIO_free(in);
        }
        else
        {
            exception = _SecurityException_("OpenSSL library cannot allocate file");
        }
    
        return returnedValue;
    }

    exception = _SecurityException_(std::string("Unsupported URI format ") + identity_cert);
    return nullptr;
}

X509* GMHelper::load_certificate(
    const std::vector<uint8_t>& data){
        X509* returnedValue = nullptr;

        if (data.size() <= static_cast<size_t>(std::numeric_limits<int>::max()))
        {
            BIO* cid = BIO_new_mem_buf(data.data(), static_cast<int>(data.size()));
    
            if (cid != nullptr)
            {
                returnedValue = PEM_read_bio_X509_AUX(cid, NULL, NULL, NULL);
                BIO_free(cid);
            }
        }
    
        return returnedValue;
}

EVP_PKEY* GMHelper::load_sm2_private_key(
    X509* certificate,
    const std::string& file,
    const std::string& password,
    SecurityException& exception,
    PKIGM& pkigm)
{
    EVP_PKEY* key = nullptr;

    if (file.size() >= 7 && file.compare(0, 7, "file://") == 0)
    {
        EVP_PKEY* returnedValue = nullptr;
        BIO* in = BIO_new(BIO_s_file());
    
        if (in != nullptr)
        {
            if (BIO_read_filename(in, file.substr(7).c_str()) > 0)
            {
                returnedValue =
                        PEM_read_bio_PrivateKey(in, NULL, (pem_password_cb*)private_key_password_callback,
                                (void*)password.c_str());
    
                // Verify private key.
                if (nullptr == returnedValue)
                {
                    exception = _SecurityException_(std::string("Error obtaining private key ") + file.substr(7));
                }
                else if (!X509_check_private_key(certificate, returnedValue))
                {
                    exception = _SecurityException_(std::string("Error verifying private key ") + file.substr(7));
                    EVP_PKEY_free(returnedValue);
                    returnedValue = nullptr;
                }
            }
            else
            {
                exception = _SecurityException_(std::string("OpenSSL library cannot read file ") + file.substr(7));
            }
    
            BIO_free(in);
        }
        else
        {
            exception = _SecurityException_("OpenSSL library cannot allocate file");
        }
    
        return returnedValue;
    }
    else if (file.size() >= 7 && file.compare(0, 7, "pkcs11:") == 0)
    {
        if (!pkigm.pkcs11_provider)
        {
            pkigm.pkcs11_provider.reset(new Pkcs11Provider());
        }

        key = pkigm.pkcs11_provider->load_private_key(certificate, file, password, exception);
    }
    else
    {
        exception = _SecurityException_(std::string("Unsupported URI format ") + file);
    }

    return key;
}

bool GMHelper::verify_sm2_certificate(
    X509_STORE* store,
    X509* cert,
    const bool there_are_crls){
    assert(store);
    assert(cert);

    bool returnedValue = false;

    X509_STORE_CTX* ctx = X509_STORE_CTX_new();

    unsigned long flags = there_are_crls ? X509_V_FLAG_CRL_CHECK : 0;
    if (X509_STORE_CTX_init(ctx, store, cert, NULL) > 0)
    {
        X509_STORE_CTX_set_flags(ctx, flags | /*X509_V_FLAG_X509_STRICT |*/
                X509_V_FLAG_CHECK_SS_SIGNATURE | X509_V_FLAG_POLICY_CHECK);
        // TODO: 验证这个核心功能是否可以正确处理国密算法的证书，或者至少要验证openssl版本
        if (X509_verify_cert(ctx) > 0)
        {
            returnedValue = true;
        }
        else
        {
            int errorCode = X509_STORE_CTX_get_error(ctx);
            if (errorCode == X509_V_OK)
            {
                TRAVODDS_LOG(TRAVODDS::LOG_WARNING, "[SECURITY_AUTHENTICATION] Invalidation error of certificate, but no error code returned.");
            }
            else
            {
                TRAVODDS_LOG(TRAVODDS::LOG_WARNING, "[SECURITY_AUTHENTICATION] Invalidation error of certificate (%s)", X509_verify_cert_error_string(errorCode));
            }
        }

        X509_STORE_CTX_cleanup(ctx);
    }
    else
    {
        TRAVODDS_LOG(TRAVODDS::LOG_WARNING, "[SECURITY_AUTHENTICATION] Cannot init context for verifying certificate");
    }

    X509_STORE_CTX_free(ctx);

    return returnedValue;
}

EVP_PKEY* GMHelper::generate_sm2_key(
    SecurityException& exception){
    // 创建上下文，并交给 unique_ptr 管理
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(nullptr, "SM2", nullptr);
    // std::unique_ptr<EVP_PKEY_CTX, EvpPkeyCtxDeleter> ctx(
    //     EVP_PKEY_CTX_new_from_name(nullptr, "SM2", nullptr)
    // );

    if (ctx == nullptr) {
        unsigned long err_code = ERR_get_error();
        exception = _SecurityException_("EVP_PKEY_CTX_new_from_name failed. Reason: " + 
                                      std::string(ERR_reason_error_string(err_code)));
        return nullptr;
    }

    // 初始化密钥生成操作
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        unsigned long err_code = ERR_get_error();
        exception = _SecurityException_("EVP_PKEY_keygen_init failed. Reason: " + 
                                      std::string(ERR_reason_error_string(err_code)));
        return nullptr;
    }

    // 生成密钥对
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_generate(ctx, &pkey) <= 0) {
        unsigned long err_code = ERR_get_error();
        exception = _SecurityException_("EVP_PKEY_generate failed. Reason: " + 
                                      std::string(ERR_reason_error_string(err_code)));
        return nullptr;
    }

    return pkey;
}

EVP_PKEY* GMHelper::generate_sm2_peer_key(
    const std::vector<uint8_t>& buffer,
    SecurityException& exception)
{
    // EVP_PKEY_fromdata 可以直接处理多种格式：
    // 1. 压缩格式：33字节 (0x02 || X(32B)) - 来自 store_sm2_public_key
    // 2. 未压缩格式：65字节 (0x04 || X(32B) || Y(32B)) - 标准SEC1格式
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_from_name(/*libctx*/nullptr, "SM2", /*propq*/nullptr);
    if (!pctx) {
        exception = _SecurityException_("OpenSSL cannot create EVP_PKEY_CTX for SM2");
        return nullptr;
    }

    if (EVP_PKEY_fromdata_init(pctx) <= 0) {
        exception = _SecurityException_("OpenSSL cannot init fromdata for SM2");
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    // 验证输入格式
    if (buffer.size() != 33 && buffer.size() != 65) {
        EVP_PKEY_CTX_free(pctx);
        exception = _SecurityException_("Actual size: " + std::to_string(buffer.size()) + " bytes");
        return nullptr;
    }

    if (buffer.size() == 33 && buffer[0] != 0x02 && buffer[0] != 0x03) {
        EVP_PKEY_CTX_free(pctx);
        exception = _SecurityException_("Invalid compressed format. Expected 0x02 or 0x03 prefix");
        return nullptr;
    }

    if (buffer.size() == 65 && buffer[0] != 0x04) {
        EVP_PKEY_CTX_free(pctx);
        exception = _SecurityException_("Invalid uncompressed format. Expected 0x04 prefix");
        return nullptr;
    }

    // 使用EC_KEY方式
    EC_KEY* ec_key = EC_KEY_new_by_curve_name(NID_sm2);
    if (ec_key == nullptr) {
        exception = _SecurityException_("Failed to create EC_KEY for SM2: " + getOpenSSLError());
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }
    // 设置公钥点
    EC_POINT* pub_point = EC_POINT_new(EC_KEY_get0_group(ec_key));
    if (pub_point == nullptr) {
        EC_KEY_free(ec_key);
        EVP_PKEY_CTX_free(pctx);
        exception = _SecurityException_("Failed to create EC_POINT: " + getOpenSSLError());
        return nullptr;
    }
    
    BN_CTX* bn_ctx = BN_CTX_new();
    if (bn_ctx == nullptr) {
        EC_POINT_free(pub_point);
        EC_KEY_free(ec_key);
        EVP_PKEY_CTX_free(pctx);
        exception = _SecurityException_("Failed to create BN_CTX: " + getOpenSSLError());
        return nullptr;
    }
    
    if (!EC_POINT_oct2point(EC_KEY_get0_group(ec_key), pub_point, 
                           buffer.data(), buffer.size(), bn_ctx)) {
        EC_POINT_free(pub_point);
        EC_KEY_free(ec_key);
        BN_CTX_free(bn_ctx);
        EVP_PKEY_CTX_free(pctx);
        exception = _SecurityException_("EC_POINT_oct2point failed: " + getOpenSSLError());
        return nullptr;
    }
    
    if (!EC_KEY_set_public_key(ec_key, pub_point)) {
        EC_POINT_free(pub_point);
        EC_KEY_free(ec_key);
        BN_CTX_free(bn_ctx);
        EVP_PKEY_CTX_free(pctx);
        exception = _SecurityException_("EC_KEY_set_public_key failed: " + getOpenSSLError());
        return nullptr;
    }
    
    // 创建EVP_PKEY
    pkey = EVP_PKEY_new();
    if (pkey == nullptr) {
        EC_POINT_free(pub_point);
        EC_KEY_free(ec_key);
        BN_CTX_free(bn_ctx);
        EVP_PKEY_CTX_free(pctx);
        exception = _SecurityException_("Failed to create EVP_PKEY: " + getOpenSSLError());
        return nullptr;
    }
    
    if (!EVP_PKEY_set1_EC_KEY(pkey, ec_key)) {
        EVP_PKEY_free(pkey);
        EC_POINT_free(pub_point);
        EC_KEY_free(ec_key);
        BN_CTX_free(bn_ctx);
        EVP_PKEY_CTX_free(pctx);
        exception = _SecurityException_("EVP_PKEY_set1_EC_KEY failed: " + getOpenSSLError());
        return nullptr;
    }
    
    // 清理资源
    EC_POINT_free(pub_point);
    EC_KEY_free(ec_key);
    BN_CTX_free(bn_ctx);

    EVP_PKEY_CTX_free(pctx);
    return pkey;
}

bool GMHelper::store_sm2_public_key(
    EVP_PKEY* sm2_key,
    std::vector<uint8_t>& buffer,
    SecurityException& exception)
{
    size_t key_len = 0;

    // 获取公钥所需的缓冲区大小
    if (!EVP_PKEY_get_octet_string_param(sm2_key, OSSL_PKEY_PARAM_PUB_KEY, nullptr, 0, &key_len)) {
        exception = _SecurityException_("Failed to get SM2 public key length: " + getOpenSSLError());
        return false;
    }

    if (key_len == 0) {
        exception = _SecurityException_("SM2 public key length is zero.");
        return false;
    }

    // 调整输出缓冲区大小
    buffer.resize(key_len);

    // 将公钥的字节数据读入缓冲区
    if (!EVP_PKEY_get_octet_string_param(sm2_key, OSSL_PKEY_PARAM_PUB_KEY, buffer.data(), key_len, &key_len)) {
        exception = _SecurityException_("Failed to serialize SM2 public key: " + getOpenSSLError());
        buffer.clear(); // 出错时清空缓冲区
        return false;
    }

    return true;
}

int GMHelper::sm3_digest(
    const void *data,
    size_t count,
    unsigned char *out_digest,
    unsigned int *len){
    const uint8_t* p_data = static_cast<const uint8_t*>(data);
    std::vector<uint8_t> input_vec(p_data, p_data + count);

    // 接收哈希结果
    std::vector<uint8_t> hash_vec;

    SMAlgorithmImpl smalgorithimpl;
    if (!smalgorithimpl.sm3_hash(input_vec, hash_vec)) {
        return 0; 
    }

    memcpy(out_digest, hash_vec.data(), hash_vec.size());
    
    // 更新输出长度
    if (len != nullptr) {
        *len = hash_vec.size();
    }
    
    return 1; // 成功，返回 1
}

int GMHelper::sm3_digest_x509_name(
    const X509_NAME* name,
    unsigned char *out_digest,
    unsigned int *len)
{
    unsigned char* der_buf = nullptr;
    int der_len = 0;
    EVP_MD* md = nullptr;
    int result = 0; 


    if (name == nullptr || out_digest == nullptr || len == nullptr) {
        return 0;
    }

    // 将 X509_NAME 对象进行 DER 编码
    der_len = i2d_X509_NAME(name, &der_buf);
    if (der_len < 0) {
        std::cerr << "错误: i2d_X509_NAME 执行失败。" << std::endl;
        ERR_print_errors_fp(stderr);
        OPENSSL_free(der_buf);
        EVP_MD_free(md);
        return result;
    }

    // 获取 SM3 算法对象
    md = EVP_MD_fetch(nullptr, "SM3", nullptr);
    if (!md) {
        std::cerr << "错误: 无法获取 SM3 算法 (EVP_MD_fetch 失败)。" << std::endl;
        OPENSSL_free(der_buf);
        EVP_MD_free(md);
        return result;
    }

    // 对 DER 编码后的字节流计算 SM3 哈希
    if (EVP_Digest(der_buf,        // 编码后的数据
                   der_len,        // 编码后的长度
                   out_digest,     // 输出哈希的缓冲区
                   len,            // 输出哈希的长度
                   md,             // 使用的算法
                   nullptr) != 1) 
    {
        std::cerr << "错误: EVP_Digest 执行失败。" << std::endl;
        ERR_print_errors_fp(stderr);
        OPENSSL_free(der_buf);
        EVP_MD_free(md);
        return result;
    }

    result = 1;

    return result;
}

bool GMHelper::sign_sm3(
    EVP_PKEY* private_key,
    const char* user_id,
    const unsigned char* data,
    const size_t data_length,
    std::vector<uint8_t>& signature,
    SecurityException& exception){
    assert(private_key);
    assert(data);

    bool returnedValue = false;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new(); 
    EVP_MD* md = nullptr;

    if (ctx)
    {
        // 使用EVP_MD_fetch加载SM3算法
        md = EVP_MD_fetch(nullptr, "SM3", nullptr);
        if (md)
        {
            EVP_PKEY_CTX* pctx = nullptr; // 用于设置SM2特有参数
            // 初始化签名操作，使用SM3算法
            if (EVP_DigestSignInit(ctx, &pctx, md, NULL, private_key) == 1)
            {
                if (user_id && EVP_PKEY_CTX_ctrl_str(pctx, "id", user_id) <= 0)
                {
                    exception = _SecurityException_("Failed to set SM2 user ID");
                }
                else if (EVP_DigestSignUpdate(ctx, data, data_length) == 1)
                {
                    size_t length = 0;
                    // 第一次调用，获取签名所需长度
                    if (EVP_DigestSignFinal(ctx, NULL, &length) == 1 && length > 0)
                    {
                        signature.resize(length);
                        // 第二次调用，获取签名数据
                        if (EVP_DigestSignFinal(ctx, signature.data(), &length) == 1)
                        {
                            signature.resize(length); // 调整为实际长度
                            returnedValue = true;
                        }
                        else
                        {
                            exception = _SecurityException_("Cannot finalize signature");
                        }
                    }
                    else
                    {
                        exception = _SecurityException_("Cannot retrieve signature length");
                    }
                }
                else
                {
                    exception = _SecurityException_("Cannot update signature data");
                }
            }
            else
            {
                exception = _SecurityException_("Cannot initialize signature context");
            }
        }
        else
        {
            exception = _SecurityException_("Failed to fetch SM3 algorithm");
        }
    }
    else
    {
        exception = _SecurityException_("Failed to create EVP_MD_CTX");
    }

    // 统一释放所有资源
    EVP_MD_free(md);
    EVP_MD_CTX_free(ctx);

    return returnedValue;
}

bool GMHelper::check_sign_sm3(
    X509* certificate,
    const unsigned char* data,
    const size_t data_length,
    const std::vector<uint8_t>& signature,
    SecurityException& exception)
{
    assert(certificate);
    assert(data);

    bool is_valid = false;
    EVP_PKEY* public_key = nullptr;
    EVP_MD_CTX* ctx = nullptr;
    EVP_MD* md = nullptr;
    std::string user_id;

    // 从证书中提取公钥
    public_key = X509_get_pubkey(certificate);
    if (public_key == nullptr) {
        exception = _SecurityException_("Failed to get public key from certificate");
        EVP_PKEY_free(public_key);
        EVP_MD_free(md);
        EVP_MD_CTX_free(ctx);
        return is_valid;
    }

    // 从证书中提取 UserID (必须与签名时使用的ID完全一致)
    user_id = GMHelper::get_subject_name_as_user_id(certificate);
    if (user_id.empty()) {
        exception = _SecurityException_("Failed to get subject name as UserID from certificate");
        EVP_PKEY_free(public_key);
        EVP_MD_free(md);
        EVP_MD_CTX_free(ctx);
        return is_valid;
    }

    // 初始化验签上下文
    ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        exception = _SecurityException_("Failed to create EVP_MD_CTX");
        EVP_PKEY_free(public_key);
        EVP_MD_free(md);
        EVP_MD_CTX_free(ctx);
        return is_valid;
    }

    md = EVP_MD_fetch(nullptr, "SM3", nullptr);
    if (md == nullptr) {
        exception = _SecurityException_("Failed to fetch SM3 algorithm");
        EVP_PKEY_free(public_key);
        EVP_MD_free(md);
        EVP_MD_CTX_free(ctx);
        return is_valid;
    }

    EVP_PKEY_CTX* pctx = nullptr;
    if (EVP_DigestVerifyInit(ctx, &pctx, md, NULL, public_key) <= 0) {
        exception = _SecurityException_("Failed to initialize digest verification");
        EVP_PKEY_free(public_key);
        EVP_MD_free(md);
        EVP_MD_CTX_free(ctx);
        return is_valid;
    }

    // 设置与签名时相同的 UserID
    if (EVP_PKEY_CTX_ctrl_str(pctx, "id", user_id.c_str()) <= 0) {
        exception = _SecurityException_("Failed to set SM2 user ID for verification");
        EVP_PKEY_free(public_key);
        EVP_MD_free(md);
        EVP_MD_CTX_free(ctx);
        return is_valid;
    }

    // 提供原始数据进行哈希
    if (EVP_DigestVerifyUpdate(ctx, data, data_length) <= 0) {
        exception = _SecurityException_("Failed to update digest verification");
        EVP_PKEY_free(public_key);
        EVP_MD_free(md);
        EVP_MD_CTX_free(ctx);
        return is_valid;
    }

    // 提供签名并进行最终验证
    int verify_result = EVP_DigestVerifyFinal(ctx, signature.data(), signature.size());
    if (verify_result == 1) {
        is_valid = true; // 验证成功
    } else if (verify_result == 0) {
        exception = _SecurityException_("Signature is invalid");
        is_valid = false;
    } else {
        exception = _SecurityException_("Error during final verification step");
        is_valid = false;
    }

    EVP_PKEY_free(public_key);
    EVP_MD_free(md);
    EVP_MD_CTX_free(ctx);

    return is_valid;
}

bool GMHelper::hmac_sm3(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data,
    std::vector<uint8_t>& mac_out)
{
    mac_out.clear();

    if (key.empty()) {
        return false; // 不允许空密钥
    }

    bool result = false;
    EVP_MAC* mac = nullptr;
    EVP_MAC_CTX* ctx = nullptr;

    // 获取 HMAC 算法实现
    mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
    if (!mac){
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return result;
    }

    ctx = EVP_MAC_CTX_new(mac);
    if (!ctx){
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return result;
    }

    char digest_name[] = "SM3";
    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string(
        OSSL_MAC_PARAM_DIGEST,
        digest_name, 0);
    params[1] = OSSL_PARAM_construct_end();

    if (EVP_MAC_init(ctx, key.data(), key.size(), params) != 1){
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return result;
    }

    if (!data.empty()) {
        if (EVP_MAC_update(ctx, data.data(), data.size()) != 1){
            EVP_MAC_CTX_free(ctx);
            EVP_MAC_free(mac);
            return result;
        }
    }

    size_t outlen = 0;
    if (EVP_MAC_final(ctx, nullptr, &outlen, 0) != 1){
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return result;
    }

    mac_out.resize(outlen);
    if (EVP_MAC_final(ctx, mac_out.data(), &outlen, mac_out.size()) != 1) {
        mac_out.clear();
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return result;
    }
    mac_out.resize(outlen); // 正常应为 32

    result = true;
    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    return result;
}

BIO* GMHelper::verify_sm2_signed_document(
    X509_STORE* store,
    BIO* in,
    SecurityException& exception)
{
    BIO* out = nullptr;
    CMS_ContentInfo* cms = nullptr;
    BIO* indata = nullptr;

    assert(nullptr != store);
    assert(nullptr != in);

    // 使用现代化的CMS接口解析S/MIME格式的签名文件
    cms = SMIME_read_CMS(in, &indata);
    if (nullptr == cms)
    {
        exception = _SecurityException_("输入文件不是有效的 S/MIME CMS 格式");
        return nullptr;
    }

    // 输出BIO，用于接收验证成功后的原始数据
    out = BIO_new(BIO_s_mem());
    if (nullptr == out)
    {
        exception = _SecurityException_("无法为输出分配 BIO 内存");
        CMS_ContentInfo_free(cms);
        if (indata) BIO_free(indata);
        return nullptr;
    }

    // 使用CMS_verify执行验证。
    if (!CMS_verify(cms, nullptr, store, indata, out, CMS_TEXT | CMS_NO_SIGNER_CERT_VERIFY | CMS_NOINTERN))
    {
        unsigned long err_code = ERR_get_error();
        exception = _SecurityException_("国密签名验证失败 (SM2/SM3)。原因: " +
                                      std::string(ERR_reason_error_string(err_code)));
        BIO_free(out);
        out = nullptr;
    }

    // 清理资源
    CMS_ContentInfo_free(cms);
    if (indata != nullptr)
    {
        BIO_free(indata);
    }

    return out;
}

struct EvpCipherCtxDel { void operator()(EVP_CIPHER_CTX* p) const noexcept { EVP_CIPHER_CTX_free(p); } };
using EvpCipherCtxUP = std::unique_ptr<EVP_CIPHER_CTX, EvpCipherCtxDel>;

struct EvpCipherDel    { void operator()(EVP_CIPHER* p) const noexcept { EVP_CIPHER_free(p); } };
using EvpCipherUP     = std::unique_ptr<EVP_CIPHER, EvpCipherDel>;


bool GMHelper::sm4_gcm_encrypt_vector(const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& aad,
    const std::vector<uint8_t>& plaintext,
    std::vector<uint8_t>& ciphertext,
    std::vector<uint8_t>& tag)
{
    ciphertext.clear();
    tag.clear();

    if (key.size() != 16 || iv.empty()) return false;

    EvpCipherCtxUP ctx{EVP_CIPHER_CTX_new()};
    if (!ctx) return false;

    EvpCipherUP cipher{EVP_CIPHER_fetch(nullptr, "SM4-GCM", nullptr)};
    if (!cipher) return false; 

    if (EVP_EncryptInit_ex(ctx.get(), cipher.get(), nullptr, nullptr, nullptr) != 1) {
        return false;
    }

    // 非 12 字节 IV 的情况，需要显式设置 IV 长度
    if (iv.size() != 12) {
        if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN,
                                static_cast<int>(iv.size()), nullptr) != 1) {
            return false;
        }
    }

    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), iv.data()) != 1) {
        return false;
    }

    int len = 0;
    if (!aad.empty()) {
        if (EVP_EncryptUpdate(ctx.get(), nullptr, &len, aad.data(),
                                static_cast<int>(aad.size())) != 1) {
            return false;
        }
    }

    // 加密数据
    ciphertext.resize(plaintext.size());
    if (EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &len,
                            plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
        ciphertext.clear();
        return false;
    }
    const int ct_len = len;

    if (EVP_EncryptFinal_ex(ctx.get(), nullptr, &len) != 1) {
        ciphertext.clear();
        return false;
    }
    ciphertext.resize(static_cast<size_t>(ct_len));

    // 获取 16B Tag
    tag.resize(16);
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG,
                            static_cast<int>(tag.size()), tag.data()) != 1) {
                                ciphertext.clear();
                                tag.clear();
        return false;
    }

    return true;
}

// 实现（GMHelper.cpp）
bool GMHelper::sm4_gcm_decrypt_vector(const std::vector<uint8_t>& key,
                                      const std::vector<uint8_t>& iv,
                                      const std::vector<uint8_t>& aad,
                                      const std::vector<uint8_t>& ciphertext,
                                      const std::vector<uint8_t>& tag,
                                      std::vector<uint8_t>& plaintext)
{
    plaintext.clear();

    if (key.size() != 16 || iv.empty() || tag.size() != 16) {
        return false;
    }

    EvpCipherCtxUP ctx{EVP_CIPHER_CTX_new()};
    if (!ctx) return false;

    EvpCipherUP cipher{EVP_CIPHER_fetch(nullptr, "SM4-GCM", nullptr)};
    if (!cipher) return false;

    // 初始化解密上下文
    if (EVP_DecryptInit_ex(ctx.get(), cipher.get(), nullptr, nullptr, nullptr) != 1) {
        return false;
    }

    // 非 12 字节 IV 的情况，需要显式设置 IV 长度
    if (iv.size() != 12) {
        if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN,
                                static_cast<int>(iv.size()), nullptr) != 1) {
            return false;
        }
    }

    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), iv.data()) != 1) {
        return false;
    }

    int len = 0;

    if (!aad.empty()) {
        if (EVP_DecryptUpdate(ctx.get(), nullptr, &len, aad.data(),
                              static_cast<int>(aad.size())) != 1) {
            return false;
        }
    }

    // 解密密文
    plaintext.resize(ciphertext.size());
    if (!ciphertext.empty()) {
        if (EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len,
                              ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
            plaintext.clear();
            return false;
        }
    }
    const int pt_len = len;

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG,
                            static_cast<int>(tag.size()),
                            const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(tag.data()))) != 1) {
        plaintext.clear();
        return false;
    }

    if (EVP_DecryptFinal_ex(ctx.get(), nullptr, &len) != 1) {
        plaintext.clear();
        return false;  // 认证失败或数据损坏
    }

    plaintext.resize(static_cast<size_t>(pt_len));
    return true;
}

bool GMHelper::generate_challenge(
    std::vector<uint8_t>& challenge_buffer, 
    SecurityException& exception){

        bool returnedValue = false;
        BIGNUM* bn = BN_new();
    
        if (BN_rand(bn, 256, 0 /*BN_RAND_TOP_ONE*/, 0 /*BN_RAND_BOTTOM_ANY*/))
        {
            int len = BN_num_bytes(bn);
            challenge_buffer.resize(len);
    
            if (BN_bn2bin(bn, challenge_buffer.data()) == len)
            {
                returnedValue = true;
            }
            else
            {
                exception = _SecurityException_("OpenSSL library cannot store challenge");
            }
        }
    
        BN_clear_free(bn);
    
        return returnedValue;
    
}

bool GMHelper::create_kx_key(
    std::array<uint8_t, 32>& out_data,
    const std::vector<uint8_t>* first_data,
    const char* cookie,
    const std::vector<uint8_t>* second_data,
    const std::vector<uint8_t>* shared_secret)
{
    bool result = false;
    unsigned char hashed_shared_secret[SM3_DIGEST_LENGTH];
    unsigned int hashed_shared_secret_len = 0;

    if (!sm3_digest(shared_secret->data(), shared_secret->size(), hashed_shared_secret, &hashed_shared_secret_len))
    {
        return result;
    }

    size_t cookie_len = strlen(cookie);
    size_t total_data_len = first_data->size() + cookie_len + second_data->size();
    std::vector<uint8_t> data_to_hash(total_data_len);

    uint8_t* p = data_to_hash.data();
    memcpy(p, first_data->data(), first_data->size());
    p += first_data->size();
    memcpy(p, cookie, cookie_len);
    p += cookie_len;
    memcpy(p, second_data->data(), second_data->size());

    unsigned char hashed_data[SM3_DIGEST_LENGTH];
    unsigned int hashed_data_len = 0;

    if (!sm3_digest(data_to_hash.data(), data_to_hash.size(), hashed_data, &hashed_data_len))
    {
        return result;
    }

    EVP_MAC* mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
    if (!mac)
    {
        return result;
    }

    EVP_MAC_CTX* mac_ctx = EVP_MAC_CTX_new(mac);
    if (!mac_ctx)
    {
        EVP_MAC_free(mac);
        return result;
    }

    char digest_name[] = "SM3";
    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, digest_name, 0);
    params[1] = OSSL_PARAM_construct_end();

    size_t out_len = 0;

    // 执行 HMAC-SM3
    if (EVP_MAC_init(mac_ctx, hashed_shared_secret, hashed_shared_secret_len, params) &&
    EVP_MAC_update(mac_ctx, hashed_data, hashed_data_len) &&
    EVP_MAC_final(mac_ctx, out_data.data(), &out_len, out_data.size()))
    {
        if (out_len == SM3_DIGEST_LENGTH)
        {
            result = true;
        }
    }

    EVP_MAC_CTX_free(mac_ctx);
    EVP_MAC_free(mac);

    return result;
}

std::vector<uint8_t> GMHelper::derive_iv_from_salt(
    const std::vector<uint8_t>& master_salt,
    // uint64_t session_id,
    uint64_t seq)
{
    std::vector<uint8_t> in(master_salt.begin(), master_salt.end());
    uint8_t be_sid[8], be_seq[8];
    // for (int i=7;i>=0;--i){ be_sid[i]=uint8_t(session_id & 0xFF); session_id >> =8; }
    for (int i=7;i>=0;--i){ be_seq[i]=uint8_t(seq & 0xFF); seq >>= 8; }
    // in.insert(in.end(), be_sid, be_sid+8);
    in.insert(in.end(), be_seq, be_seq+8);

    // 计算 SM3 并取前 12 字节作为 GCM IV
    std::vector<uint8_t> iv(12);
    EVP_MD_CTX* md = EVP_MD_CTX_new();
    unsigned int outl=0; uint8_t digest[32];
    EVP_DigestInit_ex(md, EVP_sm3(), nullptr);
    EVP_DigestUpdate(md, in.data(), in.size());
    EVP_DigestFinal_ex(md, digest, &outl);
    EVP_MD_CTX_free(md);
    std::copy(digest, digest+12, iv.begin());
    return iv;
}

std::string GMHelper::get_subject_name_as_user_id(X509* cert)
{
    // 输入验证
    if (cert == nullptr) {
        return "";
    }

    // 从证书中获取主题名称对象
    X509_NAME* subject_name = X509_get_subject_name(cert);
    if (subject_name == nullptr) {
        return "";
    }

    std::string user_id;
    BIO* bio = BIO_new(BIO_s_mem());
    if (bio == nullptr) {
        // 内存分配失败
        return "";
    }

    // 按RFC2253标准格式，将主题名称打印到BIO中
    if (X509_NAME_print_ex(bio, subject_name, 0, XN_FLAG_RFC2253) > 0) {
        char* buffer_ptr = nullptr;
        long length = BIO_get_mem_data(bio, &buffer_ptr);
        if (length > 0 && buffer_ptr != nullptr) {
            user_id.assign(buffer_ptr, length);
        }
    }

    BIO_free(bio);

    return user_id;
}