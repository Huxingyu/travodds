#ifndef _SECURITY_AUTHENTICATION_GMHELEPER_H_
#define _SECURITY_AUTHENTICATION_GMHELEPER_H_



#include <string>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/crypto.h>
#include <openssl/core_names.h>
#include "travodds_dll.h"
//#include "sm/SMAlgorithm.hpp"
#include "Security/SecurityCommon.h"
#include "Security/SecurityException.h"
#include "Security/PKcs11Provider.h"


#define SM3_DIGEST_LENGTH 32

TRAVODDS_NAMESPACE_BEGIN

// 前向声明
bool get_signature_algorithm(
    X509* certificate,
    std::string& signature_algorithm,
    SecurityException& exception);
class PKIGM;

/**
 * @brief 国密密码库辅助类
 * 封装所有与底层密码库的直接交互。
 */
class GMHelper
{
public:
    /**
     * @brief 加载SM2格式的CA证书
     * @return 返回一个包含受信任CA的证书存储区
     */
    static X509_STORE* load_sm2_ca(
        const std::string& identity_ca,
        bool& there_are_crls,
        std::string& ca_sn,
        std::string& ca_algo,
        SecurityException& exception);

    /**
     * @brief 加载SM2格式的证书吊销列表 (CRL)
     * @return 返回解析后的CRL对象
     */
    static X509_CRL* load_sm2_crl(
        const std::string& identity_crl,
        SecurityException& exception);

    /**
     * @brief 加载SM2身份证书
     * @return 返回解析后的证书对象
     */
    static X509* load_sm2_certificate(
        const std::string& cert_path,
        SecurityException& exception);

    /**
     * @brief 加载内存中的证书数据
     * @return 返回解析结果
     */
    static X509* load_certificate(
        const std::vector<uint8_t>& data);
    /**
     * @brief 加载SM2私钥
     * @return 返回解析后的私钥对象
     */
    static EVP_PKEY* load_sm2_private_key(
        X509* cert,
        const std::string& key_path,
        const std::string& password,
        SecurityException& exception,
        PKIGM& pkigm);

    /**
     * @brief 验证SM2证书的有效性
     * @return 验证成功返回true，否则返回false
     */
    static bool verify_sm2_certificate(
        X509_STORE* store,
        X509* cert,
        const bool there_are_crls);

    /**
     * @brief 为SM2密钥交换（Key Exchange）生成一对临时的SM2密钥。
     * @param[out] exception 如果发生错误，将填充异常信息。
     * @return 成功则返回一个代表临时密钥对的 EVP_PKEY* 指针，失败则返回 nullptr。
     */
    static EVP_PKEY* generate_sm2_key(
        SecurityException& exception);
    
    static EVP_PKEY* generate_sm2_peer_key(
        const std::vector<uint8_t>& buffer,
        SecurityException& exception);
    /**
     * @brief 将一个SM2密钥对中的公钥部分，编码为标准的字节序列以便传输。
     * @param[in] key_pair 包含公钥的密钥对。
     * @param[out] out_buffer 用于存储编码后公钥的字节缓冲区。
     * @param[out] exception 如果发生错误，将填充异常信息。
     * @return 编码成功返回 true，否则返回 false。
     */
    static bool store_sm2_public_key(
        EVP_PKEY* dhkey,
        std::vector<uint8_t>& buffer,
        SecurityException& exception);

    /**
     * @brief 计算 X509_NAME 对象的 SM3 哈希值。
     * 该函数首先将 X509_NAME 对象进行 DER 编码，然后对编码后的字节流计算哈希。
     * @param[in] name 要进行哈希的 X509_NAME 对象指针。
     * @param[out] out_digest 用于存储32字节哈希结果的 vector。
     * @return 成功返回 true，失败返回 false。
     */
    static int sm3_digest_x509_name(
        const X509_NAME* name,
        unsigned char *out_digest,
        unsigned int *len);
    
    /**
     * @brief 计算 任意 对象的 SM3 哈希值。
     * @param[in] name 要进行哈希的象指针。
     * @param[out] out_digest 用于存储32字节哈希结果的 vector。
     * @return 成功返回 true，失败返回 false。
     */
    static int sm3_digest(
        const void *data,
        size_t count,
        unsigned char *out_digest,
        unsigned int *len);
    
    /**
     * @brief 使用SM2私钥和SM3哈希算法对数据进行签名。
     * @param private_key SM2私钥 (EVP_PKEY*)。
     * @param user_id SM2签名所需的用户ID，若为nullptr则使用OpenSSL默认值。
     * @param data 待签名的数据。
     * @param data_length 待签名数据的长度。
     * @param signature 用于存放签名结果的字节向量。
     * @param exception 如果发生错误，将填充异常信息。
     * @return 成功返回true，失败返回false。
     */
    static bool sign_sm3(
        EVP_PKEY* private_key,
        const char* user_id,
        const unsigned char* data,
        const size_t data_length,
        std::vector<uint8_t>& signature,
        SecurityException& exception);

    /**
     * @brief 使用SM2公钥和SM3哈希算法对签名进行验证。
     * @param certificate 签名者的X509证书。
     * @param data 原始签名数据。
     * @param data_length 原始数据的长度。
     * @param signature 待验证的签名。
     * @param exception 如果发生内部错误，将填充异常信息。
     * @return 签名有效返回true，签名无效或发生错误返回false。
     */
    static bool check_sign_sm3(
        X509* certificate,
        const unsigned char* data,
        const size_t data_length,
        const std::vector<uint8_t>& signature,
        SecurityException& exception);

    /**
     * @brief 使用 OpenSSL 3.0+ EVP_MAC API 计算 HMAC-SM3
     *
     * @param key     HMAC 密钥
     * @param data    要计算摘要的数据
     * @param mac_out [输出] 计算出的 32 字节 MAC (Message Authentication Code)
     * @return        成功时返回 true, 失败时返回 false
     */
    static bool hmac_sm3(
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& data,
        std::vector<uint8_t>& mac_out);
    /**
     * @brief 使用现代化的CMS接口验证一个国密SM2/SM3签名的文档。
     * @param store 包含受信任CA证书的X509_STORE。必须由 load_sm2_ca 初始化。
     * @param in 包含签名文档数据的BIO。
     * @param exception 如果发生错误，将填充异常信息。
     * @return 验证成功则返回一个包含原始文件内容的BIO*，失败则返回nullptr。
     */
    static BIO* verify_sm2_signed_document(
        X509_STORE* store,
        BIO* in,
        SecurityException& exception);
    
    /**
     * @brief 使用 OpenSSL 3.0+ 的 EVP API 和 C++ vector 实现 SM4-GCM 加密
     *
     * @param key        16字节 (128位) 的密钥
     * @param iv         初始化向量 (IV)
     * @param aad        附加认证数据 (AAD)
     * @param plaintext  明文数据
     * @param ciphertext [输出] 加密后的密文
     * @param tag        [输出] 16字节的 GCM 认证标签
     * @return           成功时返回 true, 失败时返回 false
     */
    static bool sm4_gcm_encrypt_vector(const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv,
        const std::vector<uint8_t>& aad,
        const std::vector<uint8_t>& plaintext,
        std::vector<uint8_t>& ciphertext,
        std::vector<uint8_t>& tag);
    
    /**
     * @brief 使用 OpenSSL 3.0+ 的 EVP API 和 C++ vector 实现 SM4-GCM 解密
     *
     * @param key        16字节 (128位) 的密钥
     * @param iv         初始化向量 (IV)
     * @param aad        附加认证数据 (AAD)
     * @param ciphertext 密文数据
     * @param tag        16字节的 GCM 认证标签
     * @param plaintext  [输出] 解密得到的明文
     * @return           成功且认证通过返回 true, 否则返回 false
     */
    static bool sm4_gcm_decrypt_vector(const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv,
        const std::vector<uint8_t>& aad,
        const std::vector<uint8_t>& ciphertext,
        const std::vector<uint8_t>& tag,
        std::vector<uint8_t>& plaintext);

    /**
     * @brief 生成一个指定长度的随机字节序列，用作挑战数。
     * @param[out] challenge_buffer 用于存储生成的随机数的缓冲区，函数会根据需要调整其大小。
     * @param[out] exception 如果发生错误，将填充异常信息。
     * @return 成功返回 true，否则返回 false。
     */
    static bool generate_challenge(
        std::vector<uint8_t>& challenge_buffer, 
        SecurityException& exception);

    /**
     * @brief 根据共享密钥和握手挑战数派生出密钥材料。
     * @param[out] out_data 用于存储派生出的 32 字节密钥材料的缓冲区。
     * @param[in]  first_data 指向第一个输入数据的指针。
     * @param[in]  cookie 一个固定的字符串常量（“Cookie”），用于区分派生密钥的类型。
     * @param[in]  second_data 指向第二个输入数据的指针（通常是 Challenge2 或 Challenge1）。
     * @param[in]  shared_secret 指向共享密钥的指针，作为 HMAC-SM3 的运算密钥。
     * @return 成功派生密钥则返回 true，否则返回 false。
     */
    static bool create_kx_key(
        std::array<uint8_t, 32>& out_data,
        const std::vector<uint8_t>* first_data,
        const char* cookie,
        const std::vector<uint8_t>* second_data,
        const std::vector<uint8_t>* shared_secret);

    /**
     * @brief 根据主盐、会话ID和序列号派生出用于加密的 IV
     * 内部使用 SHA-256 哈希算法
     * @param master_salt   基础盐值
     * @param session_id    会话 ID（暂时不用）
     * @param seq           消息序列号
     * @return std::vector<uint8_t> 派生出的 12 字节 IV
     */
    static std::vector<uint8_t> derive_iv_from_salt(
        const std::vector<uint8_t>& master_salt,
        // uint64_t session_id,
        uint64_t seq);

    /**
     * @brief 从X509证书中提取RFC2253格式的主题名称作为UserID。
     * @param cert X509证书对象。
     * @return 成功则返回主题名称字符串，失败或输入无效则返回空字符串。
     */
    static std::string get_subject_name_as_user_id(X509* cert);
};

TRAVODDS_NAMESPACE_END

#endif // _SECURITY_AUTHENTICATION_GMHELEPER_H_