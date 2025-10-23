#include "travodds_dll.h"
#include "auth/PKIGM.h"
#include "cdr/travoddscdr.h"
#include "rtps/messages/parameter.h"

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
#define IS_OPENSSL_1_1 1
#define OPENSSL_CONST const
#else
#define IS_OPENSSL_1_1 0
#define OPENSSL_CONST
#endif // if OPENSSL_VERSION_NUMBER >= 0x10100000L

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION " (" __FILE__ ":" S2(__LINE__) ")"
#define _SecurityException_(str) SecurityException(std::string(str) + LOCATION)

USING_TRAVODDS_NAMESPACE

// 定义静态成员
const char* const PKIIdentity::class_id_ = "PKIIdentity";
const char* const GMHandshake::class_id_ = "GMHandshake";

static bool serializeBinaryPropertySeq(
    CdrSerializer& serializer,
    const std::vector<BinaryProperty>& properties)
{
    std::vector<const BinaryProperty*> filtered;
    filtered.reserve(properties.size());
    for (const auto& p : properties)
    {
        if (p.propagate())
        {
            filtered.push_back(&p);
        }
    }
    if (!serializer.serializeBaseType<uint32_t>(static_cast<uint32_t>(properties.size())))
    {
        return false;
    }

    for (size_t i = 0; i < filtered.size(); ++i)
    {
        const BinaryProperty& prop = *filtered[i];
        if (!serializer.serializeString(prop.name()))
        {
            return false;
        }
        // 序列化 value (字节序列)，需要先写入长度，再写入数据
        const auto& val = prop.value();
        const uint32_t vlen = static_cast<uint32_t>(val.size());
        if (!serializer.serializeBaseType<uint32_t>(vlen))
        {
            return false;
        }
        if (vlen > 0)
        {
            if (!serializer.serializeBaseTypeArray<uint8_t>(val.data(), vlen))
        {
            return false;
            }
        }
    }
    return true;
}

static bool serializeBinaryPropertySeq(
    CdrSerializer& serializer,
    const std::vector<BinaryProperty>& properties,
    const std::string& name_start)
{
    std::vector<const BinaryProperty*> filtered;
    filtered.reserve(properties.size());
    for (const auto& p : properties)
    {
        const std::string& n = p.name();
        if (n.size() >= name_start.size() &&
            n.compare(0, name_start.size(), name_start) == 0)
        {
            filtered.push_back(&p);
        }
    }
    if (!serializer.serializeBaseType<uint32_t>(static_cast<uint32_t>(filtered.size())))
    {
        return false;
    }

    for (size_t i = 0; i < filtered.size(); ++i)
    {
        const BinaryProperty& prop = *filtered[i];
        if (!serializer.serializeString(prop.name()))
        {
            return false;
        }
        // 序列化 value (字节序列)
        const auto& val = prop.value();
        const uint32_t vlen = static_cast<uint32_t>(val.size());
        if (!serializer.serializeBaseType<uint32_t>(vlen))
        {
            return false;
        }
        if (vlen > 0)
        {
            if (!serializer.serializeBaseTypeArray<uint8_t>(val.data(), vlen))
        {
            return false;
            }
        }
    }
    return true;
}

static bool serializeBinaryProperty(
    CdrSerializer& serializer,
    const BinaryProperty& prop)
{
    // 序列化 name
    if (!serializer.serializeString(prop.name()))
    {
        return false;
    }

    // 序列化 value (字节序列)
    const auto& prop_value = prop.value();
    if (!serializer.serializeBaseType<uint32_t>(static_cast<uint32_t>(prop_value.size())))
    {
        return false;
    }
    if (!serializer.serializeBaseTypeArray<uint8_t>(prop_value.data(), static_cast<uint32_t>(prop_value.size())))
    {
        return false;
    }
    return true;
}

bool read_guid_from_cdr(
    CdrDeserializer& cdr,
    uint16_t search_pid,
    GUID_t& guid)
{
    // 循环条件：只要缓冲区中还有数据就继续
    while (cdr.getBufferLength() > 0)
    {
        uint32_t memberId = 0;
        uint32_t length = 0;
        bool useExtended = false;
        bool mustUnderstand = false;

        // 读取参数头 (PID 和 Length)
        if (!cdr.deserializeParameterHeader(memberId, length, useExtended, mustUnderstand))
        {
            return false;
        }

        // 检查是否是列表结束标记
        if (memberId == PID_SENTINEL)
        {
            break;
        }

        // 3. 检查PID是否匹配
        if (memberId == search_pid)
        {
            // 确认长度是否正确 (GUID是16字节)
            if (length != (sizeof(guid.prefix.value) + sizeof(guid.entityId)))
            {
                return false; // 长度不匹配，数据有误
            }

            // 读取GUID数据
            bool success = true;
            success &= cdr.deserializeBaseTypeArray<uint8_t>(guid.prefix.value, sizeof(guid.prefix.value));
            success &= cdr.deserializeBaseTypeArray<uint8_t>((uint8_t*)&guid.entityId, sizeof(guid.entityId));
            return success;
        }
        else
        {
            // 如果PID不匹配，则跳过这个参数的数据
            uint32_t aligned_length = (length + 3) & ~3;
            if (!cdr.aheadReaderPosition(cdr.getBufferLength(), aligned_length))
            {
                return false; // 跳过失败
            }
        }
    }

    // 循环结束仍未找到
    return false;
}

static std::string convert_to_token_algo(
    const std::string& algorithm,
    bool use_legacy)
{
    // 如果使用旧版格式，直接返回内部名称
    if (use_legacy)
    {
        return algorithm;
    }
    // --- 对国密SM2/SM3算法的转换 ---
    if (algorithm == SM2_SM3)
    {
        return SM2_SM3_FOR_TOKENS;
    }
    // --- 转换为DDS-SEC 1.1规定的令牌格式(理论上用不到) ---
    else if (algorithm == RSA_SHA256)
    {
        return RSA_SHA256_FOR_TOKENS;
    }
    else if (algorithm == ECDSA_SHA256)
    {
        return ECDSA_SHA256_FOR_TOKENS;
    }
    return algorithm;
}

static std::string parse_token_algo(
    const std::string& algorithm)
{
    // --- 对国密SM2/SM3算法的解析 ---
    if (algorithm == SM2_SM3_FOR_TOKENS)
    {
        return SM2_SM3;
    }
    // --- 将令牌格式转换为内部格式(理论上用不到)  ---
    else if (algorithm == RSA_SHA256_FOR_TOKENS)
    {
        return RSA_SHA256;
    }
    else if (algorithm == ECDSA_SHA256_FOR_TOKENS)
    {
        return ECDSA_SHA256;
    }

    // 如果本身就是内部格式或旧格式，直接返回
    return algorithm;
}

std::shared_ptr<SecretHandle> PKIGM::generate_sharedsecret(
    EVP_PKEY* private_key,
    EVP_PKEY* public_key,
    SecurityException& exception) const
    {
        assert(private_key);
        assert(public_key);

        std::shared_ptr<SecretHandle> handle = nullptr;
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, NULL);

        if (ctx != nullptr)
        {
            if (EVP_PKEY_derive_init(ctx) > 0)
            {
                if (EVP_PKEY_derive_set_peer(ctx, public_key) > 0)
                {
                    size_t derived_len = 0;
                    // 获取长度
                    if (EVP_PKEY_derive(ctx, NULL, &derived_len) > 0)
                    {
                        std::vector<unsigned char> derived_secret(derived_len);
                        // 获取数据
                        if (EVP_PKEY_derive(ctx, derived_secret.data(), &derived_len) > 0)
                        {
                            unsigned char sm3_hash[32];
                            unsigned int sm3_len = 0;
                            // 使用 SM3 计算最终摘要
                            if (GMHelper::sm3_digest(derived_secret.data(), derived_len, sm3_hash, &sm3_len) == 1)
                            {
                                auto temp_handle = std::dynamic_pointer_cast<SharedSecretHandle>(
                                    get_shared_secret(SharedSecretHandle::nil_handle, exception));

                                if(temp_handle) {
                                    SharedSecret::BinaryData data;
                                    data.name("SharedSecret");
                                    data.value().assign(sm3_hash, sm3_hash + sm3_len);
                                    (*temp_handle)->data_.push_back(std::move(data));
                                    handle = temp_handle;
                                }
                            }
                            else
                            {
                                exception = _SecurityException_("GMHelper: sm3_digest failed");
                            }
                        }
                        else
                        {
                            exception = _SecurityException_("OpenSSL: Cannot derive key");
                        }
                    }
                    else
                    {
                        exception = _SecurityException_("OpenSSL: Cannot get derived key length");
                    }
                }
                else
                {
                    exception = _SecurityException_("OpenSSL: Cannot set peer key");
                }
            }
            else
            {
                exception = _SecurityException_("OpenSSL: Cannot init derive");
            }

            // 在所有操作完成后，释放上下文
            EVP_PKEY_CTX_free(ctx);
        }
        else
        {
            exception = _SecurityException_("OpenSSL: Cannot allocate context");
        }

        return handle;
};


bool TRAVODDS::get_signature_algorithm(
    X509* certificate,
    std::string& signature_algorithm,
    SecurityException& exception)
{
    bool returnedValue = false;
    BUF_MEM* ptr = nullptr;
    OPENSSL_CONST X509_ALGOR* sigalg = nullptr;
    OPENSSL_CONST ASN1_BIT_STRING* sig = nullptr;

    BIO* out = BIO_new(BIO_s_mem());

    if (out != nullptr)
    {
        X509_get0_signature(&sig, &sigalg, certificate);

        if (sigalg != nullptr)
        {
            if (i2a_ASN1_OBJECT(out, sigalg->algorithm) > 0)
            {
                BIO_get_mem_ptr(out, &ptr);

                if (ptr != nullptr)
                {
                    if (strncmp(ptr->data, "SM2-with-SM3", ptr->length) == 0)
                    {
                        signature_algorithm = SM2_SM3;
                        returnedValue = true;
                    }
                    else
                    {
                        // 如果证书使用的不是sm2-with-sm3，则认为是无效的，并设置异常信息
                        std::string found_alg(ptr->data, ptr->length);
                        exception = _SecurityException_("不支持的签名算法。期望 'sm2-with-sm3'，但实际为: " + found_alg);
                        returnedValue = false;
                    }
                }
                else
                {
                    exception = _SecurityException_("OpenSSL: 无法获取内存指针");
                }
            }
        }
        else
        {
            exception = _SecurityException_("OpenSSL: 无法从证书中写入算法信息");
        }

        BIO_free(out);
    }
    else
    {
        exception = _SecurityException_("OpenSSL: 无法分配内存");
    }

    return returnedValue;
}

static bool store_certificate_in_buffer(
    X509* certificate,
    BUF_MEM** ptr,
    SecurityException& exception)
{
    bool returnedValue = false;

    BIO* out = BIO_new(BIO_s_mem());

    if (out != nullptr)
    {
        if (PEM_write_bio_X509(out, certificate) > 0 )
        {
            BIO_get_mem_ptr(out, ptr);

            if (*ptr != nullptr)
            {
                (void)BIO_set_close(out, BIO_NOCLOSE);
                returnedValue = true;
            }
            else
            {
                exception = _SecurityException_("OpenSSL library cannot retrieve mem ptr");
            }
        }
        else
        {
            exception = _SecurityException_("OpenSSL library cannot write cert");
        }

        BIO_free(out);
    }
    else
    {
        exception = _SecurityException_("OpenSSL library cannot allocate mem");
    }

    return returnedValue;
}


static bool adjust_participant_key_sm3(
    X509* cert,
    const BuiltinTopicKey_t candidate_participant_key,
    BuiltinTopicKey_t& adjusted_participant_key,
    SecurityException& exception)
{
    assert(cert != nullptr);

    GUID_t* adjusted_guid_view = reinterpret_cast<GUID_t*>(&adjusted_participant_key);
    const GUID_t* candidate_guid_view = reinterpret_cast<const GUID_t*>(&candidate_participant_key);

    X509_NAME* cert_sn = X509_get_subject_name(cert);
    assert(cert_sn != nullptr);

    unsigned char md[SM3_DIGEST_LENGTH];
    unsigned int length = 0;

    if (!GMHelper::sm3_digest_x509_name(cert_sn, md, &length) || length != SM3_DIGEST_LENGTH)
    {
        exception = _SecurityException_("OpenSSL library cannot hash sm3");
        return false;
    }

    // 位操作逻辑保持完全不变
    adjusted_guid_view->prefix.value[0] = 0x80 | (md[0] >> 1);
    adjusted_guid_view->prefix.value[1] = (md[0] << 7) | (md[1] >> 1);
    adjusted_guid_view->prefix.value[2] = (md[1] << 7) | (md[2] >> 1);
    adjusted_guid_view->prefix.value[3] = (md[2] << 7) | (md[3] >> 1);
    adjusted_guid_view->prefix.value[4] = (md[3] << 7) | (md[4] >> 1);
    adjusted_guid_view->prefix.value[5] = (md[4] << 7) | (md[5] >> 1);

    unsigned char key[16] = {
        candidate_guid_view->prefix.value[0],
        candidate_guid_view->prefix.value[1],
        candidate_guid_view->prefix.value[2],
        candidate_guid_view->prefix.value[3],
        candidate_guid_view->prefix.value[4],
        candidate_guid_view->prefix.value[5],
        candidate_guid_view->prefix.value[6],
        candidate_guid_view->prefix.value[7],
        candidate_guid_view->prefix.value[8],
        candidate_guid_view->prefix.value[9],
        candidate_guid_view->prefix.value[10],
        candidate_guid_view->prefix.value[11],
        candidate_guid_view->entityId.entityKey[0],
        candidate_guid_view->entityId.entityKey[1],
        candidate_guid_view->entityId.entityKey[2],
        candidate_guid_view->entityId.entityKind
    };
    if (!GMHelper::sm3_digest(&key, 16, md, NULL))
    {
        exception = _SecurityException_("OpenSSL library cannot hash sm3");
        return false;
    }

    // 数据赋值逻辑保持完全不变
    adjusted_guid_view->prefix.value[6] = md[0];
    adjusted_guid_view->prefix.value[7] = md[1];
    adjusted_guid_view->prefix.value[8] = md[2];
    adjusted_guid_view->prefix.value[9] = md[3];
    adjusted_guid_view->prefix.value[10] = md[4];
    adjusted_guid_view->prefix.value[11] = md[5];

    adjusted_guid_view->entityId.entityKey[0] = candidate_guid_view->entityId.entityKey[0];
    adjusted_guid_view->entityId.entityKey[1] = candidate_guid_view->entityId.entityKey[1];
    adjusted_guid_view->entityId.entityKey[2] = candidate_guid_view->entityId.entityKey[2];
    adjusted_guid_view->entityId.entityKind = candidate_guid_view->entityId.entityKind;

    return true;
}

// static int get_dh_type(
//     const std::string& algorithm)
// {
//     auto raw_alg = algorithm.c_str();
//     if (strcmp(SM2_KEY_AGREEMENT, raw_alg) == 0)
//     {
//         return EVP_PKEY_SM2;
//     }
//     else if (strcmp(DH_2048_256, raw_alg) == 0)
//     {
//         return EVP_PKEY_DH;
//     }
//     else if (strcmp(ECDH_prime256v1, raw_alg) == 0)
//     {
//         return EVP_PKEY_EC;
//     }

//     return 0;
// }

// static EVP_PKEY* generate_dh_peer_key(
//     const std::vector<uint8_t>& buffer,
//     SecurityException& exception)
// {
//     // TODO :具体实现，需适配国密算法的密钥交换
//     return nullptr;
// }

// static EVP_PKEY* generate_dh_key(
//     int type,
//     SecurityException& exception)
// {
//     // TODO :具体实现，需适配国密算法的密钥交换
//     return nullptr;
// }

// static bool store_dh_public_key(
//     EVP_PKEY* dhkey,
//     int type,
//     std::vector<uint8_t>& buffer,
//     SecurityException& exception)
// {
//     bool returnedValue = false;

//     if (type == EVP_PKEY_DH)
//     {
//         const DH* dh =
//     #if IS_OPENSSL_1_1
//                 EVP_PKEY_get0_DH(dhkey);
//     #else
//                 dhkey->pkey.dh;
//     #endif // if IS_OPENSSL_1_1

//         if (dh != nullptr)
//         {
//     #if IS_OPENSSL_1_1
//             const BIGNUM* pub_key = nullptr;
//             const BIGNUM* priv_key = nullptr;
//             DH_get0_key(dh, &pub_key, &priv_key);

//     #else
//             const BIGNUM* pub_key = dh->pub_key;
//     #endif // if IS_OPENSSL_1_1

//             int len = BN_num_bytes(pub_key);
//             buffer.resize(len);
//             unsigned char* pointer = buffer.data();
//             if (BN_bn2bin(pub_key, pointer) == len)
//             {
//                 returnedValue = true;
//             }
//             else
//             {
//                 exception = _SecurityException_("Cannot serialize public key");
//             }
//         }
//         else
//         {
//             exception = _SecurityException_("OpenSSL library doesn't retrieve DH");
//         }
//     }
//     else if (type == EVP_PKEY_EC)
//     {
//         const EC_KEY* ec =
//     #if IS_OPENSSL_1_1
//                 EVP_PKEY_get0_EC_KEY(dhkey);
//     #else
//                 dhkey->pkey.ec;
//     #endif // if IS_OPENSSL_1_1
//         if (ec != nullptr)
//         {
//             auto grp = EC_KEY_get0_group(ec);
//             auto pub_key = EC_KEY_get0_public_key(ec);
//             auto len = EC_POINT_point2oct(grp, pub_key, EC_KEY_get_conv_form(ec), NULL, 0, NULL);
//             buffer.resize(len);
//             if (EC_POINT_point2oct(grp, pub_key, EC_KEY_get_conv_form(ec), buffer.data(), len, NULL) == len)
//             {
//                 returnedValue = true;
//             }
//             else
//             {
//                 exception = _SecurityException_("Cannot serialize public key");
//             }
//         }
//         else
//         {
//             exception = _SecurityException_("OpenSSL library doesn't retrieve DH");
//         }
//     }
//     else
//     {
//         exception = _SecurityException_("Wrong DH kind");
//     }

//     return returnedValue;
// }

// static bool generate_challenge(
//     std::vector<uint8_t>& vector,
//     SecurityException& exception)
// {
//     bool returnedValue = false;
//     // TODO :根据国密要求具体实现
//     return returnedValue;
// }

static bool generate_identity_token(
    GMIdentityHandle& handle,
    bool transmit_legacy_algorithms)
{
    Property property;
    IdentityToken& token = handle->identity_token_;
    token.SetClassID("DDS:Auth:PKI-GM:1.0");

    property.name = "dds.cert.sn";
    property.value = handle->cert_sn_;
    property.propagate = true;
    token.GetProperties().push_back(std::move(property));

    property.name = "dds.cert.algo";
    property.value = convert_to_token_algo(handle->sign_alg_, transmit_legacy_algorithms);
    property.propagate = true;
    token.GetProperties().push_back(std::move(property));

    property.name = "dds.ca.sn";
    property.value = handle->sn;
    property.propagate = true;
    token.GetProperties().push_back(std::move(property));

    property.name = "dds.ca.algo";
    property.value = convert_to_token_algo(handle->algo, transmit_legacy_algorithms);
    property.propagate = true;
    token.GetProperties().push_back(std::move(property));

    return true;
}

ValidationResult_t PKIGM::validate_local_identity(
    IdentityHandle **local_identity_handle,
    BuiltinTopicKey_t adjusted_participant_key,
    const DomainId_t domain_id,
    const DomainParticipantQos *participant_qos,
    const BuiltinTopicKey_t candidate_participant_key,
    SecurityException &exception){
    assert(local_identity_handle);

    PropertyQosPolicy auth_properties = PropertyPolicyHelper::get_properties_with_prefix(participant_qos->property,
                    "dds.sec.auth.builtin.PKI-GM.");

    if (PropertyPolicyHelper::length(auth_properties) == 0)
    {
        exception = _SecurityException_("未找到任何 dds.sec.auth.builtin.PKI-GM 属性");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    bool transmit_legacy_algorithms = false;
    std::string* legacy = PropertyPolicyHelper::find_property(auth_properties, "transmit_algorithms_as_legacy");
    if (legacy != nullptr)
    {
        transmit_legacy_algorithms = (*legacy == "true");
    }

    std::string* identity_ca = PropertyPolicyHelper::find_property(auth_properties, "identity_ca");

    if (identity_ca == nullptr)
    {
        exception = _SecurityException_("未找到 PKI-GM.identity_ca 属性");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    std::string* identity_cert = PropertyPolicyHelper::find_property(auth_properties, "identity_certificate");

    if (identity_cert == nullptr)
    {
        exception = _SecurityException_("未找到 PKI-GM.identity_certificate 属性");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    std::string* identity_crl = PropertyPolicyHelper::find_property(auth_properties, "identity_crl");

    std::string* private_key = PropertyPolicyHelper::find_property(auth_properties, "private_key");

    if (private_key == nullptr)
    {
        exception = _SecurityException_("未找到 PKI-GM.private_key 属性");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    std::string* password = PropertyPolicyHelper::find_property(auth_properties, "password");
    std::string empty_password;

    if (password == nullptr)
    {
        password = &empty_password;
    }

    std::string key_agreement_algorithm = "AUTO";
    std::string* key_agreement_property =
            PropertyPolicyHelper::find_property(auth_properties, "preferred_key_agreement");
    if (nullptr != key_agreement_property)
    {
        const std::pair<std::string, std::string> key_agreement_allowed_values[] = {
            {"SM2", "SM2"}, // 国密标准
            {"AUTO", "AUTO"}
        };

        key_agreement_algorithm = "";
        for (const auto& allowed_value : key_agreement_allowed_values)
        {
            if (key_agreement_property->compare(allowed_value.first) == 0)
            {
                key_agreement_algorithm = allowed_value.second;
                break;
            }
        }

        if (key_agreement_algorithm.empty())
        {
            exception = _SecurityException_("Invalid key agreement algorithm '" + *key_agreement_property + "'");
            EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
            return ValidationResult_t::VALIDATION_FAILED;
        }
    }

    std::unique_ptr<GMIdentityHandle> ih(new GMIdentityHandle());
    (*ih)->store_ = GMHelper::load_sm2_ca(*identity_ca, (*ih)->there_are_crls_, (*ih)->sn, (*ih)->algo, exception);

    if ((*ih)->store_ != nullptr)
    {
        if (key_agreement_algorithm == "AUTO")
        {
            key_agreement_algorithm = SM2_KEY_AGREEMENT;
        }
        (*ih)->kagree_alg_ = key_agreement_algorithm;

        if (identity_crl != nullptr)
        {
            X509_CRL* crl = GMHelper::load_sm2_crl(*identity_crl, exception);
            if (crl != nullptr)
            {
                X509_STORE_add_crl((*ih)->store_, crl);
                X509_CRL_free(crl);
                (*ih)->there_are_crls_ = true;
            }
            else
            {
                return ValidationResult_t::VALIDATION_FAILED;
            }
        }

        (*ih)->cert_ = GMHelper::load_sm2_certificate(*identity_cert, exception);

        if ((*ih)->cert_ != nullptr)
        {
            X509_NAME* cert_sn = X509_get_subject_name((*ih)->cert_);
            assert(cert_sn != nullptr);
            char* cert_sn_str = X509_NAME_oneline(cert_sn, 0, 0);
            assert(cert_sn_str != nullptr);
            (*ih)->cert_sn_ = cert_sn_str;
            OPENSSL_free(cert_sn_str);
            BIO* cert_sn_rfc2253_str = BIO_new(BIO_s_mem());
            X509_NAME_print_ex(cert_sn_rfc2253_str, cert_sn, 0, XN_FLAG_RFC2253 & ~ASN1_STRFLGS_ESC_MSB);
            const int bufsize = 1024;
            char buffer[bufsize];
            int length = BIO_read(cert_sn_rfc2253_str, buffer, bufsize);
            BIO_free(cert_sn_rfc2253_str);
            (*ih)->cert_sn_rfc2253_.assign(buffer, length);

            if (GMHelper::verify_sm2_certificate((*ih)->store_, (*ih)->cert_, (*ih)->there_are_crls_))
            {
                if (store_certificate_in_buffer((*ih)->cert_, &(*ih)->cert_content_, exception))
                {
                    if (get_signature_algorithm((*ih)->cert_, (*ih)->sign_alg_, exception))
                    {
                        (*ih)->pkey_ = GMHelper::load_sm2_private_key((*ih)->cert_, *private_key, *password, exception, *this);
                        if ((*ih)->pkey_ != nullptr)
                        {
                            if (adjust_participant_key_sm3((*ih)->cert_, candidate_participant_key,
                                    adjusted_participant_key, exception))
                            {
                                if (generate_identity_token(*ih, transmit_legacy_algorithms))
                                {
                                    (*ih)->participant_key_ = adjusted_participant_key;
                                    *local_identity_handle = ih.release();
                                    return ValidationResult_t::VALIDATION_OK;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return ValidationResult_t::VALIDATION_FAILED;
}

ValidationResult_t PKIGM::validate_remote_identity(
    IdentityHandle **remote_identity_handle,
    const IdentityHandle& local_identity_handle,
    IdentityToken *remote_identity_token,
    const BuiltinTopicKey_t remote_participant_key,
    SecurityException &exception){
        assert(remote_identity_handle);
        assert(local_identity_handle.nil() == false);

        *remote_identity_handle = nullptr;

        ValidationResult_t returnedValue = ValidationResult_t::VALIDATION_FAILED;

        const GMIdentityHandle& lih = GMIdentityHandle::narrow(local_identity_handle);

        if (!lih.nil() && !remote_identity_token->is_nil())
        {
            // dds.ca.sn
            const std::string* ca_sn = remote_identity_token->FindPropertyValue("dds.ca.sn");

            // dds.cert.sn
            // const std::string* cert_sn = DataHolderHelper::find_property_value(remote_identity_token, "dds.cert.sn");

            // dds.cert.algo
            const std::string* cert_algo = remote_identity_token->FindPropertyValue("dds.cert.algo");

            // 直接在栈上创建句柄对象
            std::unique_ptr<GMIdentityHandle> rih(new GMIdentityHandle());

            (*rih)->sn = ca_sn ? *ca_sn : "";
            (*rih)->cert_sn_ = ""; // cert_sn ? *cert_sn : "";
            (*rih)->algo = cert_algo ? parse_token_algo(*cert_algo) : "";
            (*rih)->participant_key_ = remote_participant_key;

            if (lih->participant_key_ < remote_participant_key)
            {
                returnedValue = ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_REQUEST;
            }
            else
            {
                returnedValue = ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
            }
            *remote_identity_handle = rih.release();
        }

        return returnedValue;
}

ValidationResult_t PKIGM::begin_handshake_request(
    HandshakeHandle **handshake_handle,
    HandshakeMessageToken **handshake_message_token,
    const IdentityHandle& initiator_identity_handle,
    IdentityHandle& replier_identity_handle,
    const char* builtin_topic_data,
    unsigned int builtin_topic_data_length,
    SecurityException &exception)
{
    // 前置条件检查与句柄转换
    assert(handshake_handle);
    assert(handshake_message_token);
    assert(initiator_identity_handle.nil() == false);
    assert(replier_identity_handle.nil() == false);

    const GMIdentityHandle& lih = GMIdentityHandle::narrow(initiator_identity_handle);
    GMIdentityHandle& rih = GMIdentityHandle::narrow(replier_identity_handle);

    if (lih.nil() || rih.nil())
    {
        exception = _SecurityException_("Bad precondition: Invalid IdentityHandle");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if (builtin_topic_data == nullptr || builtin_topic_data_length == 0)
    {
        exception = _SecurityException_("Bad precondition: Empty participant data");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 为本次握手会话创建状态句柄
    GMHandshakeHandle* handshake_handle_aux = new GMHandshakeHandle();
    // (*handshake_handle_aux)->kagree_alg_ = lih->kagree_alg_;
    (*handshake_handle_aux)->handshake_message_.SetClassID("DDS:Auth:PKI-GM:1.0+Req");

    BinaryProperty bproperty;

    // 添加 c.id (发起方SM2证书)
    bproperty.name("c.id");
    bproperty.value().assign(lih->cert_content_->data,
            lih->cert_content_->data + lih->cert_content_->length);
    bproperty.propagate(true);
    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

    // 添加 c.perm (权限文件)
    if (lih->permissions_credential_token_.GetClassID().compare("DDS:Access:PermissionsCredential") == 0)
    {
        const Property* permissions_file = lih->permissions_credential_token_.FindProperty("dds.perm.cert");
        if (permissions_file != nullptr)
        {
            bproperty.name("c.perm");
            bproperty.value().assign(permissions_file->value.begin(), permissions_file->value.end());
            bproperty.propagate(true);
            (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));
        }
        else
        {
            exception = _SecurityException_("Cannot find permissions file in permissions credential token");
            delete handshake_handle_aux;
            return ValidationResult_t::VALIDATION_FAILED;
        }
    }

    // 添加 c.pdata (参与者数据)
    bproperty.name("c.pdata");
    bproperty.value().assign(reinterpret_cast<const unsigned char*>(builtin_topic_data),
            reinterpret_cast<const unsigned char*>(builtin_topic_data) + builtin_topic_data_length);
    bproperty.propagate(true);
    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

    // 添加 c.dsign_algo 和 c.kagree_algo
    bproperty.name("c.dsign_algo");
    bproperty.value().assign(lih->sign_alg_.begin(), lih->sign_alg_.end());
    bproperty.propagate(true);
    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

    bproperty.name("c.kagree_algo");
    bproperty.value().assign(lih->kagree_alg_.begin(), lih->kagree_alg_.end());
    bproperty.propagate(true);
    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

    // 计算前面所有内容的哈希值(hash_c1)，用于完整性校验
    // CDRMessage_t message(static_cast<uint32_t>(BinaryPropertyHelper::serialized_size(
    //                 (*handshake_handle_aux)->handshake_message_.GetBinaryProperties())));
    // message.msg_endian = BIGEND;
    // CDRMessage::addBinaryPropertySeq(&message, (*handshake_handle_aux)->handshake_message_.GetBinaryProperties(), false);
    size_t required_size = BinaryPropertyHelper::serialized_size(
        (*handshake_handle_aux)->handshake_message_.GetBinaryProperties());
    std::vector<char> buffer(required_size);

    SerializedBuffer serialized_buffer;
    serialized_buffer.buffer = buffer.data();
    serialized_buffer.buffer_size = static_cast<uint32_t>(buffer.size());


    CdrSerializer serializer(&serialized_buffer, false);

    serializeBinaryPropertySeq(serializer,
        (*handshake_handle_aux)->handshake_message_.GetBinaryProperties());

    // 类型转换（正确性未知）
    // std::vector<uint8_t> message_data(message.buffer, message.buffer + message.length);
    unsigned char hashResult[SM3_DIGEST_LENGTH];

    //SMAlgorithmImpl sm_algorithm;
    bool hashSuccess = GMHelper::sm3_digest(serialized_buffer.buffer, serialized_buffer.writer_position, hashResult, NULL);
    if (!hashSuccess)
    {
        exception = _SecurityException_("SM3 hash calculation failed");
        delete handshake_handle_aux;
        return ValidationResult_t::VALIDATION_FAILED;
    }
    bproperty.name("hash_c1");
    bproperty.value().assign(hashResult, hashResult + SM3_DIGEST_LENGTH);
    bproperty.propagate(true);
    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

    // int kagree_kind = get_dh_type((*handshake_handle_aux)->kagree_alg_);

    // SM2密钥交换：生成并添加本次会话的临时公钥
    (*handshake_handle_aux)->sm2keys_ = GMHelper::generate_sm2_key(exception);
    if ((*handshake_handle_aux)->sm2keys_ != nullptr)
    {
        bproperty.name("sm2_pub1");
        bproperty.propagate(true);

        if (GMHelper::store_sm2_public_key((*handshake_handle_aux)->sm2keys_, bproperty.value(), exception))
        {
            (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

            // 生成一个随机挑战数
            bproperty.name("challenge1");
            bproperty.propagate(true);
            if (GMHelper::generate_challenge(bproperty.value(), exception))
            {
                (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

                // 转移句柄所有权，并设置好消息出参
                (*handshake_handle_aux)->local_identity_handle_ = &lih;
                (*handshake_handle_aux)->remote_identity_handle_ = &rih;
                *handshake_handle = handshake_handle_aux;
                *handshake_message_token = &(*handshake_handle_aux)->handshake_message_;
                return ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
            }
        }
    }

    delete handshake_handle_aux;
    return ValidationResult_t::VALIDATION_FAILED;
}

    // bool PKIGM::return_identity_handle(
    //         const IdentityHandle identity_handle,
    //         SecurityException* exception)
    // {
    //     // 检查传入的句柄是否为空
    //     if (nullptr == identity_handle)
    //     {
    //         // 如果为空，直接成功返回
    //         return true;
    //     }
    //     // 删除和释放之前由 get_identity_handle 或类似函数创建的句柄对象
    //     delete identity_handle;
    //     return true;
    // }

ValidationResult_t PKIGM::begin_handshake_reply(
    HandshakeHandle **handshake_handle,
    HandshakeMessageToken **handshake_message_out,
    HandshakeMessageToken &&handshake_message_in,
    IdentityHandle &initiator_identity_handle,
    const IdentityHandle &replier_identity_handle,
    const char* builtin_topic_data,
    unsigned int builtin_topic_data_length,
    SecurityException& exception){
    assert(handshake_handle);
    assert(handshake_message_out);
    assert(initiator_identity_handle.nil() == false);
    assert(replier_identity_handle.nil() == false);

    const GMIdentityHandle& lih = GMIdentityHandle::narrow(replier_identity_handle);
    GMIdentityHandle& rih = GMIdentityHandle::narrow(initiator_identity_handle);


    if (lih.nil() || rih.nil())
    {
        exception = _SecurityException_("Bad precondition");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if (builtin_topic_data_length == 0)
    {
        exception = _SecurityException_("Bad precondition");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 检查TokenMessage
    if (handshake_message_in.GetClassID().compare("DDS:Auth:PKI-GM:1.0+Req") != 0)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", (std::string("Bad HandshakeMessageToken (") +
                handshake_message_in.GetClassID() + ")").c_str());
        return ValidationResult_t::VALIDATION_FAILED;
    }
    // 检查传入的握手请求
    // 检查c.id
    const std::vector<uint8_t>* cid = handshake_message_in.FindBinaryPropertyValue( "c.id");
    if (cid == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property c.id");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 远端若有旧证书先释放
    if (rih->cert_ != nullptr)
    {
        X509_free(rih->cert_);
        rih->cert_ = nullptr;
    }

    rih->cert_ = GMHelper::load_certificate(*cid);
    if (rih->cert_ == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot load certificate");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if (rih->cert_ == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s",  "Cannot load certificate");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    X509_NAME* cert_sn = X509_get_subject_name(rih->cert_);
    assert(cert_sn != nullptr);
    char* cert_sn_str = X509_NAME_oneline(cert_sn, 0, 0);
    assert(cert_sn_str != nullptr);
    if (!rih->cert_sn_.empty() && rih->cert_sn_.compare(cert_sn_str) != 0)
    {
        OPENSSL_free(cert_sn_str);
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Certificated subject name invalid");
        return ValidationResult_t::VALIDATION_FAILED;
    }
    // 格式转化和存储
    rih->cert_sn_.assign(cert_sn_str);
    OPENSSL_free(cert_sn_str);
    BIO* cert_sn_rfc2253_str = BIO_new(BIO_s_mem());
    X509_NAME_print_ex(cert_sn_rfc2253_str, cert_sn, 0, XN_FLAG_RFC2253 & ~ASN1_STRFLGS_ESC_MSB);
    const int bufsize = 1024;
    char buffer[bufsize];
    int str_length = BIO_read(cert_sn_rfc2253_str, buffer, bufsize);
    BIO_free(cert_sn_rfc2253_str);
    rih->cert_sn_rfc2253_.assign(buffer, str_length);

    // 提取并存储权限数据
    if (!GMHelper::verify_sm2_certificate(lih->store_, rih->cert_, lih->there_are_crls_))
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Error verifying certificate");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if (lih->permissions_credential_token_.GetClassID().compare("DDS:Access:PermissionsCredential") == 0)
    {
        const std::vector<uint8_t>* perm = handshake_message_in.FindBinaryPropertyValue("c.perm");

        if (perm == nullptr)
        {
            WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property c.perm");
            return ValidationResult_t::VALIDATION_FAILED;
        }

        rih->permissions_credential_token_.SetClassID("DDS:Access:PermissionsCredential");
        Property permission_file;
        permission_file.name = "dds.perm.cert";
        permission_file.value.assign(perm->begin(), perm->end());
        rih->permissions_credential_token_.GetProperties().push_back(std::move(permission_file));
    }

    const std::vector<uint8_t>* pdata = handshake_message_in.FindBinaryPropertyValue("c.pdata");

    if (pdata == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property c.pdata");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    GUID_t participant_guid;

    // 使用 SerializedBuffer 来包装 pdata 的现有内存
    SerializedBuffer sbuffer_pdata;
    sbuffer_pdata.buffer = (char*)pdata->data();
    sbuffer_pdata.buffer_size = (uint32_t)pdata->size();
    // 将 writer_position 设置为缓冲区大小，表示缓冲区已填满数据可供读取
    sbuffer_pdata.writer_position = (uint32_t)pdata->size();
    sbuffer_pdata.reader_position = 0;

    // 创建一个反序列化器来读取 sbuffer_pdata
    CdrDeserializer cdr_deserializer(&sbuffer_pdata, false);

    if (!read_guid_from_cdr(cdr_deserializer, PID_PARTICIPANT_GUID, participant_guid))
    {
        WARNING_SECURITY_LOGGING("PKIGM", "Cannot deserialize ParticipantProxyData in property c.pdata");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 后续的验证逻辑保持不变
    if ((participant_guid.prefix.value[0] & 0x80) != 0x80)
    {
        WARNING_SECURITY_LOGGING("PKIGN", "Bad participant_key's first bit in c.pdata");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    unsigned char md[SM3_DIGEST_LENGTH];
    unsigned char hash_c1[SM3_DIGEST_LENGTH];
    unsigned int length = 0;

    if (!GMHelper().sm3_digest_x509_name(cert_sn, md, &length) || length != SM3_DIGEST_LENGTH)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot generate SM3 of subject name");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    md[5] &= 0xFE;
    unsigned char bytes[6]{
        static_cast<unsigned char>((participant_guid.prefix.value[0] << 1) |
        (participant_guid.prefix.value[1] >> 7)),
        static_cast<unsigned char>((participant_guid.prefix.value[1] << 1) |
        (participant_guid.prefix.value[2] >> 7)),
        static_cast<unsigned char>((participant_guid.prefix.value[2] << 1) |
        (participant_guid.prefix.value[3] >> 7)),
        static_cast<unsigned char>((participant_guid.prefix.value[3] << 1) |
        (participant_guid.prefix.value[4] >> 7)),
        static_cast<unsigned char>((participant_guid.prefix.value[4] << 1) |
        (participant_guid.prefix.value[5] >> 7)),
        static_cast<unsigned char>(participant_guid.prefix.value[5] << 1)
    };

    if (memcmp(md, bytes, 6) != 0)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Bad participant_key's 47bits in c.pdata");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // c.dsign_algo
    const std::vector<uint8_t>* dsign_algo = handshake_message_in.FindBinaryPropertyValue("c.dsign_algo");

    if (dsign_algo == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property c.dsign_algo");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 检查签名算法
    std::string s_dsign_algo(dsign_algo->begin(), dsign_algo->end());
    if (strcmp(SM2_SM3, s_dsign_algo.c_str()) != 0 && strcmp(RSA_SHA256, s_dsign_algo.c_str()) != 0 && strcmp(ECDSA_SHA256, s_dsign_algo.c_str()) != 0)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", ("Unsupported signature algorithm: " + s_dsign_algo).c_str());
        return ValidationResult_t::VALIDATION_FAILED;
    }
    rih->sign_alg_ = std::move(s_dsign_algo);

    // c.kagree_algo
    const std::vector<uint8_t>* kagree_algo = handshake_message_in.FindBinaryPropertyValue("c.kagree_algo");
    if (kagree_algo == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property c.kagree_algo");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 检查密钥协商算法
    std::string s_kagree_algo(kagree_algo->begin(), kagree_algo->end());
    if (strcmp(SM2_KEY_AGREEMENT, s_kagree_algo.c_str()))
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", ("Unsupported key agreement algorithm: " + s_kagree_algo).c_str());
        return ValidationResult_t::VALIDATION_FAILED;
    }
    rih->kagree_alg_ = std::move(s_kagree_algo);

    // 准备要序列化的数据
    const auto& binary_properties = handshake_message_in.GetBinaryProperties();

    // 计算所需空间大小并创建缓冲区
    size_t required_size = BinaryPropertyHelper::serialized_size(binary_properties);
    std::vector<char> property_buffer(required_size);

    // 将缓冲区与 SerializedBuffer 关联
    SerializedBuffer serialized_buffer;
    serialized_buffer.buffer = property_buffer.data();
    serialized_buffer.buffer_size = static_cast<uint32_t>(property_buffer.size());

    // 创建序列化器，并设置字节序为大端
    CdrSerializer serializer(&serialized_buffer, false);

    // 调用辅助函数完成序列化，传入前缀 "c."
    serializeBinaryPropertySeq(serializer, binary_properties, "c.");

    // 对序列化后的整个缓冲区内容进行哈希计算
    if (!GMHelper::sm3_digest(serialized_buffer.buffer, serialized_buffer.writer_position, hash_c1, NULL)) {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot generate SM3 of request");
        return ValidationResult_t::VALIDATION_FAILED;
    }
    else
    {
        // hash_c1
        std::vector<uint8_t>* hash_c1_vec =handshake_message_in.FindBinaryPropertyValue("hash_c1");

        if (hash_c1_vec != nullptr)
        {
            if ((hash_c1_vec->size() == SM3_DIGEST_LENGTH) &&
                    (memcmp(hash_c1, hash_c1_vec->data(), SM3_DIGEST_LENGTH) != 0))
            {
                WARNING_SECURITY_LOGGING("PKIGM", "%s", "Wrong hash_c1");
            }
        }
    }

    // sm2_pub1
    std::vector<uint8_t>* sm2_pub1 = handshake_message_in.FindBinaryPropertyValue("sm2_pub1");

    if (sm2_pub1 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property sm2_pub1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // challenge1
    std::vector<uint8_t>* challenge1 = handshake_message_in.FindBinaryPropertyValue("challenge1");

    if (challenge1 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property challenge1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 产生handshake reply message token
    GMHandshakeHandle* handshake_handle_aux = new GMHandshakeHandle();
    // (*handshake_handle_aux)->kagree_alg_ = rih->kagree_alg_;
    (*handshake_handle_aux)->handshake_message_.SetClassID("DDS:Auth:PKI-GM:1.0+Reply");

    // int kagree_kind = get_dh_type((*handshake_handle_aux)->kagree_alg_);

    // 存储sm2_pub1
    if (((*handshake_handle_aux)->peerkeys = GMHelper::generate_sm2_peer_key(*sm2_pub1, exception)) == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot store peer key from sm2_pub1");
        delete handshake_handle_aux;
        return ValidationResult_t::VALIDATION_FAILED;
    }

    BinaryProperty bproperty;

    // c.id
    bproperty.name("c.id");
    bproperty.value().assign(lih->cert_content_->data,
            lih->cert_content_->data + lih->cert_content_->length);
    bproperty.propagate(true);
    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

    // c.perm
    if (lih->permissions_credential_token_.GetClassID().compare("DDS:Access:PermissionsCredential") == 0)
    {
        const Property* permissions_file = lih->permissions_credential_token_.FindProperty("dds.perm.cert");

        if (permissions_file != nullptr)
        {
            bproperty.name("c.perm");
            bproperty.value().assign(permissions_file->value.begin(), permissions_file->value.end());
            bproperty.propagate(true);
            (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));
        }
        else
        {
            exception = _SecurityException_("Cannot find permissions file in permissions credential token");
            EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
            return ValidationResult_t::VALIDATION_FAILED;
        }
    }

    // c.pdata
    bproperty.name("c.pdata");
    bproperty.value().assign(builtin_topic_data, builtin_topic_data + builtin_topic_data_length);
    bproperty.propagate(true);
    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

    // c.dsign_algo.
    bproperty.name("c.dsign_algo");
    bproperty.value().assign(lih->sign_alg_.begin(),
            lih->sign_alg_.end());
    bproperty.propagate(true);
    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

    // c.kagree_algo.
    bproperty.name("c.kagree_algo");
    bproperty.value().assign((*handshake_handle_aux)->kagree_alg_.begin(),
            (*handshake_handle_aux)->kagree_alg_.end());
    bproperty.propagate(true);
    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

    // hash_c2
    // 准备要序列化的数据
    const auto& binary_properties2 = (*handshake_handle_aux)->handshake_message_.GetBinaryProperties();

    // 计算所需空间并创建缓冲区
    size_t required_size2 = BinaryPropertyHelper::serialized_size(binary_properties2);
    std::vector<char> property_buffer2(required_size2);

    // 将缓冲区与 SerializedBuffer 关联
    SerializedBuffer serialized_buffer2;
    serialized_buffer2.buffer = property_buffer2.data();
    serialized_buffer2.buffer_size = static_cast<uint32_t>(property_buffer2.size());

    // 创建序列化器，并设置字节序为大端
    CdrSerializer serializer2(&serialized_buffer2, false);

    // 调用辅助函数完成序列化
    serializeBinaryPropertySeq(serializer2, binary_properties2);

    // 对序列化后的数据进行哈希计算
    if (!GMHelper::sm3_digest(serialized_buffer2.buffer, serialized_buffer2.writer_position, md, NULL))
    {
    exception = _SecurityException_("OpenSSL library cannot hash sm3");
    EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
    delete handshake_handle_aux;
    return ValidationResult_t::VALIDATION_FAILED;
    }
    bproperty.name("hash_c2");
    bproperty.value().assign(md, md + SM3_DIGEST_LENGTH);
    bproperty.propagate(true);
    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

    // sm2_pub2
    if (((*handshake_handle_aux)->sm2keys_ = GMHelper::generate_sm2_key(exception)) != nullptr)
    {
        bproperty.name("sm2_pub2");
        bproperty.propagate(true);

        if (GMHelper::store_sm2_public_key((*handshake_handle_aux)->sm2keys_, bproperty.value(), exception))
        {
            (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

            // hash_c1
            bproperty.name("hash_c1");
            bproperty.value().assign(hash_c1, hash_c1 + SM3_DIGEST_LENGTH);
            bproperty.propagate(true);
            (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

            // sm2_pub1
            bproperty.name("sm2_pub1");
            bproperty.value(std::move(*sm2_pub1));
            bproperty.propagate(true);
            (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

            // challenge1
            bproperty.name("challenge1");
            bproperty.value(std::move(*challenge1));
            bproperty.propagate(true);
            (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

            // challenge2
            bproperty.name("challenge2");
            bproperty.propagate(true);
            if (GMHelper::generate_challenge(bproperty.value(), exception))
            {
                (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

                size_t required_size_sig = BinaryPropertyHelper::serialized_size(
                    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties());
                std::vector<char> signature_buffer(required_size_sig);

                SerializedBuffer serialized_buffer_sig;
                serialized_buffer_sig.buffer = signature_buffer.data();
                serialized_buffer_sig.buffer_size = static_cast<uint32_t>(signature_buffer.size());

                // 创建序列化器
                CdrSerializer serializer_sig(&serialized_buffer_sig, false); // BIGEND

                // 写入序列长度，手动指定为6
                serializer_sig.serializeBaseType<uint32_t>(6);

                // 依次查找并序列化6个指定的 BinaryProperty
                serializeBinaryProperty(serializer_sig,
                    *(*handshake_handle_aux)->handshake_message_.FindBinaryProperty("hash_c2"));
                serializeBinaryProperty(serializer_sig,
                    *(*handshake_handle_aux)->handshake_message_.FindBinaryProperty("challenge2"));
                serializeBinaryProperty(serializer_sig,
                    *(*handshake_handle_aux)->handshake_message_.FindBinaryProperty("sm2_pub2"));
                serializeBinaryProperty(serializer_sig,
                    *(*handshake_handle_aux)->handshake_message_.FindBinaryProperty("challenge1"));
                serializeBinaryProperty(serializer_sig,
                    *(*handshake_handle_aux)->handshake_message_.FindBinaryProperty("sm2_pub1"));
                serializeBinaryProperty(serializer_sig,
                    *(*handshake_handle_aux)->handshake_message_.FindBinaryProperty("hash_c1"));

                bproperty.name("signature");
                bproperty.propagate(true);
                if (GMHelper::sign_sm3(lih->pkey_, nullptr, reinterpret_cast<const unsigned char*>(serialized_buffer_sig.buffer), serialized_buffer_sig.writer_position, bproperty.value(), exception))
                {
                    (*handshake_handle_aux)->handshake_message_.GetBinaryProperties().push_back(std::move(bproperty));

                    (*handshake_handle_aux)->local_identity_handle_ = &lih;
                    (*handshake_handle_aux)->remote_identity_handle_ = &rih;
                    *handshake_handle = handshake_handle_aux;
                    *handshake_message_out = &(*handshake_handle_aux)->handshake_message_;

                    return ValidationResult_t::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
                }
            }
        }
    }
    delete handshake_handle_aux;

    // ERR_clear_error();

    return ValidationResult_t::VALIDATION_FAILED;
}

ValidationResult_t PKIGM::process_handshake(
    HandshakeMessageToken** handshake_message_out,
    HandshakeMessageToken&& handshake_message_in,
    HandshakeHandle& handshake_handle,
    SecurityException& exception)
{
    ValidationResult_t returnedValue = ValidationResult_t::VALIDATION_FAILED;

    GMHandshakeHandle& handshake = GMHandshakeHandle::narrow(handshake_handle);

    if (!handshake.nil())
    {
        if (handshake->handshake_message_.GetClassID().compare("DDS:Auth:PKI-GM:1.0+Req") == 0)
        {
            returnedValue = process_handshake_request(handshake_message_out, std::move(handshake_message_in),
                            handshake, exception);
        }
        else if (handshake->handshake_message_.GetClassID().compare("DDS:Auth:PKI-GM:1.0+Reply") == 0)
        {
            returnedValue = process_handshake_reply(handshake_message_out, std::move(handshake_message_in),
                            handshake, exception);
        }
        else
        {
            WARNING_SECURITY_LOGGING("PKIGM", "%s",
                    (std::string("Handshake message not supported (") + handshake->handshake_message_.GetClassID() + ")").c_str());
        }
    }

    return returnedValue;
}

ValidationResult_t PKIGM::process_handshake_request(
    HandshakeMessageToken** handshake_message_out,
    HandshakeMessageToken&& handshake_message_in,
    GMHandshakeHandle& handshake_handle,
    SecurityException& exception)
{
    const GMIdentityHandle& lih = *handshake_handle->local_identity_handle_;
    GMIdentityHandle& rih = *handshake_handle->remote_identity_handle_;

    // 检查 Token message
    if (handshake_message_in.GetClassID().compare("DDS:Auth:PKI-GM:1.0+Reply") != 0)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", (std::string("Bad HandshakeMessageToken (") +
                handshake_message_in.GetClassID() + ")").c_str());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 检查 incoming handshake.
    // 检查 c.id
    const std::vector<uint8_t>* cid = handshake_message_in.FindBinaryPropertyValue("c.id");
    if (cid == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property c.id");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    rih->cert_ = GMHelper::load_certificate(*cid);

    if (rih->cert_ == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot load certificate");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    X509_NAME* cert_sn = X509_get_subject_name(rih->cert_);
    assert(cert_sn != nullptr);
    char* cert_sn_str = X509_NAME_oneline(cert_sn, 0, 0);
    assert(cert_sn_str != nullptr);
    if (!rih->cert_sn_.empty() && rih->cert_sn_.compare(cert_sn_str) != 0)
    {
        OPENSSL_free(cert_sn_str);
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Certificated subject name invalid");
        return ValidationResult_t::VALIDATION_FAILED;
    }
    OPENSSL_free(cert_sn_str);
    BIO* cert_sn_rfc2253_str = BIO_new(BIO_s_mem());
    X509_NAME_print_ex(cert_sn_rfc2253_str, cert_sn, 0, XN_FLAG_RFC2253 & ~ASN1_STRFLGS_ESC_MSB);
    const int bufsize = 1024;
    char buffer[bufsize];
    int str_length = BIO_read(cert_sn_rfc2253_str, buffer, bufsize);
    BIO_free(cert_sn_rfc2253_str);
    rih->cert_sn_rfc2253_.assign(buffer, str_length);

    if (!GMHelper::verify_sm2_certificate(lih->store_, rih->cert_, lih->there_are_crls_))
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Error verifying certificate");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // c.perm
    if (lih->permissions_credential_token_.GetClassID().compare("DDS:Access:PermissionsCredential") == 0)
    {
        const std::vector<uint8_t>* perm = handshake_message_in.FindBinaryPropertyValue("c.perm");

        if (perm == nullptr)
        {
            WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property c.perm");
            return ValidationResult_t::VALIDATION_FAILED;
        }

        rih->permissions_credential_token_.SetClassID("DDS:Access:PermissionsCredential");
        Property permission_file;
        permission_file.name = "dds.perm.cert";
        permission_file.value.assign(perm->begin(), perm->end());
        rih->permissions_credential_token_.GetProperties().push_back(std::move(permission_file));
    }

    const std::vector<uint8_t>* pdata = handshake_message_in.FindBinaryPropertyValue("c.pdata");

    if (pdata == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property c.pdata");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    GUID_t participant_guid;

    // 使用 SerializedBuffer 来包装 pdata 的现有内存
    SerializedBuffer sbuffer_pdata;
    sbuffer_pdata.buffer = (char*)pdata->data();
    sbuffer_pdata.buffer_size = (uint32_t)pdata->size();
    // 将 writer_position 设置为缓冲区大小，表示缓冲区已填满数据可供读取
    sbuffer_pdata.writer_position = (uint32_t)pdata->size();
    sbuffer_pdata.reader_position = 0;

    // 创建一个反序列化器来读取 sbuffer_pdata，并设置为大端字节序
    CdrDeserializer cdr_deserializer(&sbuffer_pdata, false); // false for BIGEND

    if (!read_guid_from_cdr(cdr_deserializer, PID_PARTICIPANT_GUID, participant_guid))
    {
        WARNING_SECURITY_LOGGING("PKIDH", "Cannot deserialize ParticipantProxyData in property c.pdata");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if ((participant_guid.prefix.value[0] & 0x80) != 0x80)
    {
        WARNING_SECURITY_LOGGING("PKIDH", "Bad participant_key's first bit in c.pdata");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    unsigned char md[SM3_DIGEST_LENGTH];
    unsigned int length = 0;

    if (!GMHelper().sm3_digest_x509_name(cert_sn, md, &length) || length != SM3_DIGEST_LENGTH)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot generate SM3 of subject name");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    md[5] &= 0xFE;
    unsigned char bytes[6]{
        static_cast<unsigned char>((participant_guid.prefix.value[0] << 1) |
        (participant_guid.prefix.value[1] >> 7)),
        static_cast<unsigned char>((participant_guid.prefix.value[1] << 1) |
        (participant_guid.prefix.value[2] >> 7)),
        static_cast<unsigned char>((participant_guid.prefix.value[2] << 1) |
        (participant_guid.prefix.value[3] >> 7)),
        static_cast<unsigned char>((participant_guid.prefix.value[3] << 1) |
        (participant_guid.prefix.value[4] >> 7)),
        static_cast<unsigned char>((participant_guid.prefix.value[4] << 1) |
        (participant_guid.prefix.value[5] >> 7)),
        static_cast<unsigned char>(participant_guid.prefix.value[5] << 1)
    };

    if (memcmp(md, bytes, 6) != 0)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Bad participant_key's 47bits in c.pdata");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // c.dsign_algo
    const std::vector<uint8_t>* dsign_algo = handshake_message_in.FindBinaryPropertyValue("c.dsign_algo");
    if (dsign_algo == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property c.dsign_algo");
        return ValidationResult_t::VALIDATION_FAILED;
    }
    // 检查签名算法
    std::string s_dsign_algo(dsign_algo->begin(), dsign_algo->end());
    if (s_dsign_algo.compare(SM2_SM3) != 0 &&
        s_dsign_algo.compare(RSA_SHA256) != 0 &&
        s_dsign_algo.compare(ECDSA_SHA256) != 0)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", ("Unsupported signature algorithm: " + s_dsign_algo).c_str());
        return ValidationResult_t::VALIDATION_FAILED;
    }
    rih->sign_alg_ = std::move(s_dsign_algo);

    // c.kagree_algo
    const std::vector<uint8_t>* kagree_algo = handshake_message_in.FindBinaryPropertyValue("c.kagree_algo");
    if (kagree_algo == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property c.kagree_algo");
        return ValidationResult_t::VALIDATION_FAILED;
    }
    // 检查密钥协商算法
    std::string s_kagree_algo(kagree_algo->begin(), kagree_algo->end());
    if (s_kagree_algo.compare(handshake_handle->kagree_alg_) != 0)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", std::string("Invalid key agreement algorithm. Received ") +
                s_kagree_algo + ", expected " + handshake_handle->kagree_alg_);
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // hash_c2
    BinaryProperty* hash_c2 = handshake_message_in.FindBinaryProperty("hash_c2");
    if (hash_c2 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property hash_c2");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if (hash_c2->value().size() != SM3_DIGEST_LENGTH)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Wrong size of hash_c2");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 计算序列化所需的精确大小
    size_t required_size = BinaryPropertyHelper::serialized_size(
        handshake_message_in.GetBinaryProperties());

    // 创建缓冲区并将其与 SerializedBuffer 关联
    std::vector<char> property_buffer(required_size);
    SerializedBuffer serialized_buffer;
    serialized_buffer.buffer = property_buffer.data();
    serialized_buffer.buffer_size = static_cast<uint32_t>(property_buffer.size());

    // 创建序列化器，并设置为大端字节序
    CdrSerializer serializer(&serialized_buffer, false); // false for BIGEND

    // 序列化属性并添加 "c." 前缀
    serializeBinaryPropertySeq(serializer, handshake_message_in.GetBinaryProperties(), "c.");

    // 对序列化后的数据进行哈希计算
    if (!GMHelper::sm3_digest(serialized_buffer.buffer, serialized_buffer.writer_position, md, NULL))
    {
        exception = _SecurityException_("Cannot generate SM3 of request");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if (memcmp(md, hash_c2->value().data(), SM3_DIGEST_LENGTH) != 0)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Wrong hash_c2");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // sm2_pub2
    BinaryProperty* sm2_pub2 = handshake_message_in.FindBinaryProperty("sm2_pub2");
    if (sm2_pub2 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property sm2_pub2");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // int kagree_kind = get_dh_type(s_kagree_algo);

    if ((handshake_handle->peerkeys = GMHelper::generate_sm2_peer_key(sm2_pub2->value(), exception)) == nullptr)
    {
        exception = _SecurityException_("Cannot store peer key from sm2_pub2");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    BinaryProperty* challenge2 = handshake_message_in.FindBinaryProperty("challenge2");

    if (challenge2 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property challenge2");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // hash_c1
    BinaryProperty* hash_c1 = handshake_message_in.FindBinaryProperty("hash_c1");

    if (hash_c1 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property hash_c1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    const std::vector<uint8_t>* hash_c1_request = handshake_handle->handshake_message_.FindBinaryPropertyValue("hash_c1");

    if (hash_c1_request == nullptr)
    {
        exception = _SecurityException_("Cannot find property hash_c1 in request message");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if (hash_c1->value() != *hash_c1_request)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Invalid property hash_c1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // sm2_pub1
    BinaryProperty* sm2_pub1 = handshake_message_in.FindBinaryProperty("sm2_pub1");

    if (sm2_pub1 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property sm2_pub1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    const std::vector<uint8_t>* sm2_pub1_request = handshake_handle->handshake_message_.FindBinaryPropertyValue("sm2_pub1");

    if (sm2_pub1_request == nullptr)
    {
        exception = _SecurityException_("Cannot find property sm2_pub1 in request message");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if (sm2_pub1->value() != *sm2_pub1_request)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Invalid property sm2_pub1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    BinaryProperty* challenge1 = handshake_message_in.FindBinaryProperty("challenge1");

    if (challenge1 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s",  "Cannot find property challenge1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    const std::vector<uint8_t>* challenge1_request = handshake_handle->handshake_message_.FindBinaryPropertyValue("challenge1");

    if (challenge1_request == nullptr)
    {
        exception = _SecurityException_("Cannot find property challenge1 in request message");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s",  exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if (challenge1->value() != *challenge1_request)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s",  "Invalid property challenge1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    const std::vector<uint8_t>* signature = handshake_message_in.FindBinaryPropertyValue("signature");

    if (signature == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s",  "Cannot find property signature");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 为签名数据准备缓冲区
    size_t required_size_2 = BinaryPropertyHelper::serialized_size(
        handshake_message_in.GetBinaryProperties());
    std::vector<char> property_buffer_2(required_size_2);

    SerializedBuffer serialized_buffer_2;
    serialized_buffer_2.buffer = property_buffer_2.data();
    serialized_buffer_2.buffer_size = static_cast<uint32_t>(property_buffer_2.size());

    // 创建唯一的序列化器实例
    CdrSerializer serializer_2(&serialized_buffer_2, false); // BIGEND

    // 写入序列长度
    serializer_2.serializeBaseType<uint32_t>(6);

    // 依次序列化6个指定的 BinaryProperty
    serializeBinaryProperty(serializer_2, *hash_c2);
    serializeBinaryProperty(serializer_2, *challenge2);
    serializeBinaryProperty(serializer_2, *sm2_pub2);
    serializeBinaryProperty(serializer_2, *challenge1);
    serializeBinaryProperty(serializer_2, *sm2_pub1);
    serializeBinaryProperty(serializer_2, *hash_c1);

    // 使用新的变量调用签名检查函数
    if (!GMHelper::check_sign_sm3(rih->cert_, reinterpret_cast<const unsigned char*>(serialized_buffer_2.buffer),
        serialized_buffer_2.writer_position, *signature, exception)){
        WARNING_SECURITY_LOGGING("PKIGM", "%s",  "Error verifying signature");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // Generate handshake final message token.
    HandshakeMessageToken final_message;
    final_message.GetBinaryProperties().clear();
    final_message.SetClassID("DDS:Auth:PKI-GM:1.0+Final");

    BinaryProperty bproperty;

    // hash_c1
    bproperty.name("hash_c1");
    bproperty.value(std::move(hash_c1->value()));
    bproperty.propagate(true);
    final_message.GetBinaryProperties().push_back(std::move(bproperty));

    // hash_c2
    bproperty.name("hash_c2");
    bproperty.value(std::move(hash_c2->value()));
    bproperty.propagate(true);
    final_message.GetBinaryProperties().push_back(std::move(bproperty));

    // sm2_pub1
    bproperty.name("sm2_pub1");
    bproperty.value(std::move(sm2_pub1->value()));
    bproperty.propagate(true);
    final_message.GetBinaryProperties().push_back(std::move(bproperty));

    // sm2_pub2
    bproperty.name("sm2_pub2");
    bproperty.value(std::move(sm2_pub2->value()));
    bproperty.propagate(true);
    final_message.GetBinaryProperties().push_back(std::move(bproperty));

    // challenge1
    bproperty.name("challenge1");
    bproperty.value(std::move(challenge1->value()));
    bproperty.propagate(true);
    final_message.GetBinaryProperties().push_back(std::move(bproperty));

    // challenge2
    bproperty.name("challenge2");
    bproperty.value(std::move(challenge2->value()));
    bproperty.propagate(true);
    final_message.GetBinaryProperties().push_back(std::move(bproperty));

    serialized_buffer_2.writer_position = 0;
    serialized_buffer_2.reader_position = 0;

    // 为重置后的缓冲区创建一个新的、干净的序列化器实例
    CdrSerializer serializer_3(&serialized_buffer_2, false); // BIGEND

    // 写入序列长度
    serializer_3.serializeBaseType<uint32_t>(6);

    // 依次序列化6个指定的 BinaryProperty
    serializeBinaryProperty(serializer_3, *final_message.FindBinaryProperty("hash_c1"));
    serializeBinaryProperty(serializer_3, *final_message.FindBinaryProperty("challenge1"));
    serializeBinaryProperty(serializer_3, *final_message.FindBinaryProperty("sm2_pub1"));
    serializeBinaryProperty(serializer_3, *final_message.FindBinaryProperty("challenge2"));
    serializeBinaryProperty(serializer_3, *final_message.FindBinaryProperty("sm2_pub2"));
    serializeBinaryProperty(serializer_3, *final_message.FindBinaryProperty("hash_c2"));

    bproperty.name("signature");
    bproperty.propagate(true);
    if (GMHelper::sign_sm3(lih->pkey_, lih->cert_sn_rfc2253_.c_str(),
    reinterpret_cast<const unsigned char*>(serialized_buffer_2.buffer), serialized_buffer_2.writer_position,
        bproperty.value(), exception))
    {
        final_message.GetBinaryProperties().push_back(std::move(bproperty));

        auto base_secret_handle = this->generate_sharedsecret(
            handshake_handle->sm2keys_,
            handshake_handle->peerkeys,
            exception);

        auto shared_secret = std::dynamic_pointer_cast<SharedSecretHandle>(base_secret_handle);
        if (shared_secret != nullptr)
        {
            handshake_handle->local_sharedsecret_ = shared_secret;
            handshake_handle->remote_sharedsecret_ = shared_secret;

            // Save challenge1 y challenge2 in sharedsecret
            (*shared_secret)->data_.emplace_back(SharedSecret::BinaryData("Challenge1",
                *final_message.FindBinaryPropertyValue("challenge1")));
            (*shared_secret)->data_.emplace_back(SharedSecret::BinaryData("Challenge2",
                *final_message.FindBinaryPropertyValue("challenge2")));

            handshake_handle->handshake_message_ = std::move(final_message);
            *handshake_message_out = &handshake_handle->handshake_message_;

            return ValidationResult_t::VALIDATION_OK_WITH_FINAL_MESSAGE;
        }
    }

    return ValidationResult_t::VALIDATION_FAILED;
}



ValidationResult_t PKIGM::process_handshake_reply(
    HandshakeMessageToken** handshake_message_out,
    HandshakeMessageToken&& handshake_message_in,
    GMHandshakeHandle& handshake_handle,
    SecurityException& exception)
{
    GMIdentityHandle& rih = *handshake_handle->remote_identity_handle_;

    // 检查 Token message
    if (handshake_message_in.GetClassID().compare("DDS:Auth:PKI-GM:1.0+Final") != 0)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", (std::string("Bad HandshakeMessageToken (") +
                handshake_message_in.GetClassID() + ")").c_str());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // 检查 incoming handshake.
    // challenge1
    BinaryProperty* challenge1 = handshake_message_in.FindBinaryProperty("challenge1");
    if (challenge1 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property challenge1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    std::vector<uint8_t>* challenge1_reply = handshake_handle->handshake_message_.FindBinaryPropertyValue("challenge1");
    if (challenge1_reply == nullptr)
    {
        exception = _SecurityException_("Cannot find property challenge1 in request message");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    if (challenge1->value() != *challenge1_reply)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Invalid property challenge1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // challenge2
    BinaryProperty* challenge2 = handshake_message_in.FindBinaryProperty("challenge2");
    if (challenge2 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property challenge2");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    std::vector<uint8_t>* challenge2_reply = handshake_handle->handshake_message_.FindBinaryPropertyValue("challenge2");
    if (challenge2_reply == nullptr)
    {
        exception = _SecurityException_("Cannot find property challenge2 in request message");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }
    if (challenge2->value() != *challenge2_reply)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Invalid property challenge2");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // signature
    const std::vector<uint8_t>* signature = handshake_message_in.FindBinaryPropertyValue("signature");
    if (signature == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property signature");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // hash_c1
    BinaryProperty* hash_c1_reply = handshake_handle->handshake_message_.FindBinaryProperty("hash_c1");
    if (hash_c1_reply == nullptr)
    {
        exception = _SecurityException_("Cannot find property hash_c1 in request message");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    BinaryProperty* hash_c1 = handshake_message_in.FindBinaryProperty("hash_c1");
    if (hash_c1 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property hash_c1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // hash_c2
    BinaryProperty* hash_c2_reply = handshake_handle->handshake_message_.FindBinaryProperty("hash_c2");
    if (hash_c2_reply == nullptr)
    {
        exception = _SecurityException_("Cannot find property hash_c2 in request message");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    BinaryProperty* hash_c2 = handshake_message_in.FindBinaryProperty("hash_c2");
    if (hash_c2 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property hash_c2");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // sm2_pub1
    BinaryProperty* sm2_pub1_reply = handshake_handle->handshake_message_.FindBinaryProperty("sm2_pub1");
    if (sm2_pub1_reply == nullptr)
    {
        exception = _SecurityException_("Cannot find property sm2_pub1 in request message");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    BinaryProperty* sm2_pub1 = handshake_message_in.FindBinaryProperty("sm2_pub1");
    if (sm2_pub1 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property sm2_pub1");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // sm2_pub2
    BinaryProperty* sm2_pub2_reply = handshake_handle->handshake_message_.FindBinaryProperty("sm2_pub2");
    if (sm2_pub2_reply == nullptr)
    {
        exception = _SecurityException_("Cannot find property sm2_pub2 in request message");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
        return ValidationResult_t::VALIDATION_FAILED;
    }

    BinaryProperty* sm2_pub2 = handshake_message_in.FindBinaryProperty("sm2_pub2");
    if (sm2_pub2 == nullptr)
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Cannot find property sm2_pub2");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    size_t required_size = BinaryPropertyHelper::serialized_size(
        handshake_message_in.GetBinaryProperties());
    std::vector<char> property_buffer(required_size);

    SerializedBuffer serialized_buffer;
    serialized_buffer.buffer = property_buffer.data();
    serialized_buffer.buffer_size = static_cast<uint32_t>(property_buffer.size());

    // 创建序列化器实例
    CdrSerializer serializer(&serialized_buffer, false); // BIGEND

    // 写入序列长度
    serializer.serializeBaseType<uint32_t>(6);

    // 依次序列化6个指定的 BinaryProperty
    serializeBinaryProperty(serializer, *hash_c1_reply);
    serializeBinaryProperty(serializer, *challenge1);
    serializeBinaryProperty(serializer, *sm2_pub1_reply);
    serializeBinaryProperty(serializer, *challenge2);
    serializeBinaryProperty(serializer, *sm2_pub2_reply);
    serializeBinaryProperty(serializer, *hash_c2_reply);

    // 调用签名检查函数，并进行类型转换
    if (!GMHelper::check_sign_sm3(rih->cert_,
        reinterpret_cast<const unsigned char*>(serialized_buffer.buffer),
        serialized_buffer.writer_position, *signature, exception))
    {
        WARNING_SECURITY_LOGGING("PKIGM", "%s", "Error verifying signature");
        return ValidationResult_t::VALIDATION_FAILED;
    }

    // Generate shared secret
    auto base_secret_handle = this->generate_sharedsecret(
        handshake_handle->sm2keys_,
        handshake_handle->peerkeys,
        exception);
    auto shared_secret = std::dynamic_pointer_cast<SharedSecretHandle>(base_secret_handle);

    if (shared_secret != nullptr)
    {
        handshake_handle->local_sharedsecret_ = shared_secret;
        handshake_handle->remote_sharedsecret_ = shared_secret;

        // Save challenge1 y challenge2 in sharedsecret
        (*shared_secret)->data_.emplace_back(SharedSecret::BinaryData("Challenge1",
            *handshake_message_in.FindBinaryPropertyValue("challenge1")));
        (*shared_secret)->data_.emplace_back(SharedSecret::BinaryData("Challenge2",
            *handshake_message_in.FindBinaryPropertyValue("challenge2")));

        handshake_handle->handshake_message_ = std::move(handshake_message_in);
        *handshake_message_out = &handshake_handle->handshake_message_;

        return ValidationResult_t::VALIDATION_OK;
    }

    return ValidationResult_t::VALIDATION_FAILED;
}

std::shared_ptr<SecretHandle> PKIGM::get_shared_secret(
    const HandshakeHandle& handshake_handle,
    SecurityException& exception) const
{
    const GMHandshakeHandle& handshake = GMHandshakeHandle::narrow(handshake_handle);

    if (!handshake.nil())
    {
        auto secret = get_shared_secret(SharedSecretHandle::nil_handle, exception);
        auto sharedsecret = std::dynamic_pointer_cast<SharedSecretHandle>(secret);
        (*sharedsecret)->data_ = (*handshake->local_sharedsecret_)->data_;
        return secret;
    }

    auto p = new (std::nothrow) SharedSecretHandle;
    return std::dynamic_pointer_cast<SecretHandle>(
        std::shared_ptr<SharedSecretHandle>(p, [](SharedSecretHandle* p)
        {
            delete p;
        }));
}

IdentityHandle* PKIGM::get_identity_handle(
        SecurityException&)
{
    return new (std::nothrow) GMIdentityHandle();
}

bool PKIGM::get_authenticated_peer_credential_token(
    AuthenticatedPeerCredentialToken **peer_credential_token,
    const HandshakeHandle &handshake_handle,
    SecurityException &exception)
{
    const GMIdentityHandle& handle = GMIdentityHandle::narrow(handshake_handle);

    if (!handle.nil())
    {
        *peer_credential_token = new AuthenticatedPeerCredentialToken(handle->permissions_credential_token_);
        return true;
    }
    else
    {
        exception = _SecurityException_("Invalid handshake handle");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
    }

    return false;
}

bool PKIGM::get_identity_token(
    IdentityToken **identity_token,
    const IdentityHandle &handle,
    SecurityException &exception){
    const GMIdentityHandle& ihandle = GMIdentityHandle::narrow(handle);

    if (!ihandle.nil())
    {
        *identity_token = new IdentityToken(ihandle->identity_token_);
        return true;
    }

    return false;
}

bool PKIGM::set_permissions_credential_and_token(
    IdentityHandle &handle,
    const PermissionsCredentialToken &permissions_credential_token,
    SecurityException &exception){
    GMIdentityHandle& ihandle = GMIdentityHandle::narrow(handle);

    if (!ihandle.nil())
    {
        ihandle->permissions_credential_token_ = std::move(permissions_credential_token);
        return true;
    }
    else
    {
        exception = _SecurityException_("Invalid identity handle");
        EMERGENCY_SECURITY_LOGGING("PKIGM", "%s", exception.what());
    }

    return false;
}

bool PKIGM::return_identity_token(
    IdentityToken *token,
    SecurityException &exception){
    delete token;
    token = nullptr;
    return true;
}

bool PKIGM::return_authenticated_peer_credential_token(
    AuthenticatedPeerCredentialToken *peer_credential_token,
    SecurityException &exception){
    delete peer_credential_token;
    peer_credential_token = nullptr;
    return true;
}

bool PKIGM::return_handshake_handle(
    HandshakeHandle *handshake_handle,
    SecurityException &exception){
    GMHandshakeHandle* handle = &GMHandshakeHandle::narrow(*handshake_handle);

    if (!handle->nil())
    {
        delete handle;
        return true;
    }

    return false;
}

bool PKIGM::return_identity_handle(
    IdentityHandle *identity_handle,
    SecurityException &exception){
    GMIdentityHandle* handle = &GMIdentityHandle::narrow(*identity_handle);

    if (!handle->nil())
    {
        delete handle;
        return true;
    }

    return false;
}

bool PKIGM::return_sharedsecret_handle(
    std::shared_ptr<SecretHandle>& sharedsecret_handle,
    SecurityException &exception){
    sharedsecret_handle.reset();
    return true;
}

bool PKIGM::return_handshake_token(
    HandshakeMessageToken *handshake_token,
    SecurityException &exception){
    delete handshake_token;
    handshake_token = nullptr;
    return true;
}

bool PKIGM::set_listener(
    AuthenticationListener *listener,
    SecurityException &exception){
    // 简单的实现，暂时忽略listener参数
    // 在实际实现中，这里应该存储listener指针以便后续使用
    (void)listener;  // 避免未使用参数警告
    (void)exception; // 避免未使用参数警告
    return true;
}

bool PKIGM::return_identity_status_token(
    IdentityStatusToken *token,
    SecurityException &exception){
    delete token;
    token = nullptr;
    return true;
}