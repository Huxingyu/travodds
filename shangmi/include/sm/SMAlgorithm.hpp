#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/err.h>


namespace dds_security_sm {
namespace SM {

class SMAlgorithm {
public:
    virtual ~SMAlgorithm() = default;
    virtual bool sm4_encrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& plain, std::vector<uint8_t>& cipher) = 0;
    virtual bool sm4_decrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& cipher, std::vector<uint8_t>& plain) = 0;
    virtual bool sm2_sign(const std::vector<uint8_t>& privkey, const std::vector<uint8_t>& data, std::vector<uint8_t>& signature) = 0;
    virtual bool sm2_verify(const std::vector<uint8_t>& pubkey, const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature) = 0;
    virtual bool sm3_hash(const std::vector<uint8_t>& data, std::vector<uint8_t>& hash) = 0;
};

class SMAlgorithmImpl : public SMAlgorithm {
    public:
        ~SMAlgorithmImpl() override = default; // 显式声明覆写基类析构函数
        // 覆写基类的纯虚函数
        bool sm4_encrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& plain, std::vector<uint8_t>& cipher) override;
        bool sm4_decrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& cipher, std::vector<uint8_t>& plain) override;
        bool sm2_sign(const std::vector<uint8_t>& privkey, const std::vector<uint8_t>& data, std::vector<uint8_t>& signature) override;
        bool sm2_verify(const std::vector<uint8_t>& pubkey, const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature) override;
        bool sm3_hash(const std::vector<uint8_t>& data, std::vector<uint8_t>& hash) override;
    };

} // namespace SM
} // namespace dds_security_sm 
