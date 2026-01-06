#include "sm/SMAlgorithm.hpp"

namespace dds_security_sm {
namespace SM {


bool SMAlgorithmImpl::sm4_encrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& plain, std::vector<uint8_t>& cipher){
    // TODO: 调用实际商密库实现SM4加密
    return true;
}
bool SMAlgorithmImpl::sm4_decrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& cipher, std::vector<uint8_t>& plain){
    // TODO: 调用实际商密库实现SM4解密
    return true;
}
bool SMAlgorithmImpl::sm2_sign(const std::vector<uint8_t>& privkey, const std::vector<uint8_t>& data, std::vector<uint8_t>& signature){
    // TODO: 调用实际商密库实现SM2签名
    return true;
}
bool SMAlgorithmImpl::sm2_verify(const std::vector<uint8_t>& pubkey, const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature) {
    // TODO: 调用实际商密库实现SM2验签
    return true;
}
bool SMAlgorithmImpl::sm3_hash(const std::vector<uint8_t>& data, std::vector<uint8_t>& hash){
    // 获取 SM3 算法的 EVP_MD 对象
    EVP_MD* md = EVP_MD_fetch(nullptr, "SM3", nullptr);
    if (!md) {
        std::cerr << "错误: 无法获取 SM3 算法。请检查 OpenSSL 版本和配置。" << std::endl;
        // 打印详细的 OpenSSL 错误堆栈
        ERR_print_errors_fp(stderr);
        return false;
    }

    // 准备输出缓冲区和长度变量
    hash.resize(EVP_MAX_MD_SIZE); 
    unsigned int hash_len = 0;

    // 执行哈希计算
    if (EVP_Digest(data.data(),   // 输入数据指针
                   data.size(),    // 输入数据长度
                   hash.data(),    // 输出缓冲区指针
                   &hash_len,      // 接收实际哈希长度的变量地址
                   md,             // 使用的算法 (SM3)
                   nullptr) != 1) { 
        std::cerr << "错误: EVP_Digest 执行失败。" << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_MD_free(md); 
        return false;
    }

    // 调整输出 vector 的大小为实际的哈希长度
    // 对于 SM3，hash_len 将是 32
    hash.resize(hash_len);

    EVP_MD_free(md);

    return true;
}


} // namespace SM
} // namespace dds_security_sm 
