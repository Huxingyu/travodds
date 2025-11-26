#include "travodds_dll.h"
#include <cassert>
#include <fstream>


#include "accesscontrol/Permissions.h"
#include "common/log/logger.h"
#include "accesscontrol/GovernanceParser.h"
#include "accesscontrol/PermissionsParser.h"
#include "auth/GMIdentityHandle.h"
#include "auth/GMHelper.h"
#include "accesscontrol/EndpointSecurityAttributes.h"
#include "accesscontrol/DistinguishedName.h"
#include "accesscontrol/AccessPermissionsHandle.h"


#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION " (" __FILE__ ":" S2(__LINE__) ")"
#define _SecurityException_(str) SMSecurityException(std::string(str) + LOCATION)

USING_TRAVODDS_NAMESPACE

// 定义静态成员
const char* const AccessPermissions::class_id_ = "AccessPermissions";

// 实现 PluginEndpointSecurityAttributes::mask()
PluginEndpointSecurityAttributesMask PluginEndpointSecurityAttributes::mask() const
{
    PluginEndpointSecurityAttributesMask rv = PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;
    if (is_submessage_encrypted)
    {
        rv |= PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED;
    }
    if (is_payload_encrypted)
    {
        rv |= PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_ENCRYPTED;
    }
    if (is_submessage_origin_authenticated)
    {
        rv |= PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED;
    }
    return rv;
}

static BIO* load_and_verify_document(
    X509_STORE* store,
    BIO* in,
    SMSecurityException& exception)
{
    // 将验证逻辑委托给 GMHelper
    return GMHelper::verify_sm2_signed_document(store, in, exception);
}

static BIO* load_signed_file(
    X509_STORE* store,
    std::string& file,
    SMSecurityException& exception)
{
    assert(store);
    BIO* out = nullptr;

    if (file.size() >= 7 && file.compare(0, 7, "file://") == 0)
    {
        BIO* in = BIO_new_file(file.substr(7).c_str(), "r");
        if (in != nullptr)
        {
            out = load_and_verify_document(store, in, exception);
            BIO_free(in);
        }
        else
        {
            exception = _SecurityException_(std::string("Cannot read file ") + file);
        }
    }
    else
    {
        exception = _SecurityException_(std::string("Unsupported governance file format ") + file);
    }

    return out;
}

static bool load_governance_file(
    AccessPermissionsHandle& ah,
    std::string& governance_file,
    DomainAccessRules& rules,
    SMSecurityException& exception){
    bool returned_value = false;

    BIO* file_mem = load_signed_file(ah->store_, governance_file, exception);

    if (file_mem != nullptr)
    {
        BUF_MEM* ptr = nullptr;
        BIO_get_mem_ptr(file_mem, &ptr);

        if (ptr != nullptr)
        {
            GovernanceParser parser;
            if ((returned_value = parser.parse_stream(ptr->data, ptr->length)) == true)
            {
                parser.swap(rules);
            }
            else
            {
                exception = _SecurityException_(std::string("Malformed governance file ") + governance_file);
            }
        }
        else
        {
            exception = _SecurityException_(std::string("OpenSSL library cannot retrieve mem ptr from file ")
                            + governance_file);
        }

        BIO_free(file_mem);
    }

    return returned_value;   
}

static bool load_permissions_file(
    AccessPermissionsHandle& ah,
    std::string& permissions_file,
    PermissionsData& permissions,
    SMSecurityException& exception){
    bool returned_value = false;

    BIO* file_mem = load_signed_file(ah->store_, permissions_file, exception);

    if (file_mem != nullptr)
    {
        BUF_MEM* ptr = nullptr;
        BIO_get_mem_ptr(file_mem, &ptr);

        if (ptr != nullptr)
        {
            PermissionsParser parser;
            if ((returned_value = parser.parse_stream(ptr->data, ptr->length)) == true)
            {
                parser.swap(permissions);
            }
            else
            {
                exception = _SecurityException_(std::string("Malformed permissions file ") + permissions_file);
            }
        }
        else
        {
            exception = _SecurityException_(std::string("OpenSSL library cannot retrieve mem ptr from file ")
                            + permissions_file);
        }

        BIO_free(file_mem);
    }

    return returned_value;
}

static bool verify_permissions_file(
        const AccessPermissionsHandle& local_handle,
        const std::string& permissions_file,
        PermissionsData& permissions,
        SMSecurityException& exception)
{
    bool returned_value = false;

    if (permissions_file.size() <= static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        BIO* permissions_buf = BIO_new_mem_buf(permissions_file.data(), static_cast<int>(permissions_file.size()));
        if (permissions_buf != nullptr)
        {
            BIO* out = load_and_verify_document(local_handle->store_, permissions_buf, exception);
            if (nullptr != out)
            {
                BUF_MEM* ptr = nullptr;
                BIO_get_mem_ptr(out, &ptr);

                if (ptr != nullptr)
                {
                    PermissionsParser parser;
                    if ((returned_value = parser.parse_stream(ptr->data, ptr->length)) == true)
                    {
                        parser.swap(permissions);
                        returned_value = true;
                    }
                    else
                    {
                        exception = _SecurityException_(std::string(
                                            "Malformed permissions file ") + permissions_file);
                    }
                }
                else
                {
                    exception = _SecurityException_("OpenSSL library cannot retrieve mem ptr from file.");
                }
                BIO_free(out);
            }
            else
            {
                exception = _SecurityException_("Cannot read as PKCS7 the permissions file.");
            }

            BIO_free(permissions_buf);
        }
    }

    return returned_value;
}

static bool is_validation_in_time(
        const Validity& validity)
{
#if _MSC_VER != 1800
    bool returned_value = false;
    std::time_t current_time = std::time(nullptr);

    if (std::difftime(current_time, validity.not_before) >= 0)
    {
        if (std::difftime(validity.not_after, current_time) >= 0)
        {
            returned_value = true;
        }
    }

    return returned_value;
#else
    (void)validity;
    return true;
#endif // if _MSC_VER != 1800
}

static bool is_domain_in_set(
        const uint32_t domain_id,
        const Domains& domains)
{
    bool returned_value = false;

    for (auto range : domains.ranges)
    {
        if (range.second == 0)
        {
            if (domain_id == range.first)
            {
                returned_value = true;
                break;
            }
        }
        else
        {
            if (domain_id >= range.first &&
                    domain_id <= range.second)
            {
                returned_value = true;
                break;
            }
        }
    }

    return returned_value;
}

static void process_protection_kind(
        const ProtectionKind kind,
        bool& protected_flag,
        bool& encrypted_flag,
        bool& orig_auth_flag)
{
    protected_flag = kind != ProtectionKind::NONE;
    encrypted_flag = (kind == ProtectionKind::ENCRYPT) || (kind == ProtectionKind::ENCRYPT_WITH_ORIGIN_AUTHENTICATION);
    orig_auth_flag = (kind == ProtectionKind::ENCRYPT_WITH_ORIGIN_AUTHENTICATION) ||
            (kind == ProtectionKind::SIGN_WITH_ORIGIN_AUTHENTICATION);
}

static bool check_subject_name(
    const SMIdentityHandle& ih,
    AccessPermissionsHandle& ah,
    const uint32_t domain_id,
    DomainAccessRules& governance,
    PermissionsData& permissions,
    SMSecurityException& exception){
    bool returned_value = false;
    const GMIdentityHandle& lih = GMIdentityHandle::narrow(ih);

    if (!lih.nil())
    {
        for (auto grant : permissions.grants)
        {
            if (is_validation_in_time(grant.validity))
            {
                if (rfc2253_string_compare(grant.subject_name, lih->cert_sn_rfc2253_))
                {
                    ah->grant = std::move(grant);
                    returned_value = true;

                    auto iterator = grant.rules.begin();
                    while (iterator != grant.rules.end())
                    {
                        if (!is_domain_in_set(domain_id, iterator->domains))
                        {
                            iterator = grant.rules.erase(iterator);
                        }
                        else
                        {
                            ++iterator;
                        }
                    }

                    break;
                }
            }
        }

        if (returned_value)
        {
            for (auto rule : governance.rules)
            {
                if (is_domain_in_set(domain_id, rule.domains))
                {
                    ah->governance_rule_.is_access_protected = rule.enable_join_access_control;

                    PluginParticipantSecurityAttributes plug_part_attr;

                    process_protection_kind(rule.discovery_protection_kind,
                            ah->governance_rule_.is_discovery_protected,
                            plug_part_attr.is_discovery_encrypted,
                            plug_part_attr.is_discovery_origin_authenticated);

                    process_protection_kind(rule.rtps_protection_kind,
                            ah->governance_rule_.is_rtps_protected,
                            plug_part_attr.is_rtps_encrypted,
                            plug_part_attr.is_rtps_origin_authenticated);

                    process_protection_kind(rule.liveliness_protection_kind,
                            ah->governance_rule_.is_liveliness_protected,
                            plug_part_attr.is_liveliness_encrypted,
                            plug_part_attr.is_liveliness_origin_authenticated);

                    if (rule.allow_unauthenticated_participants)
                    {
                        if (ah->governance_rule_.is_rtps_protected)
                        {
                            exception = _SecurityException_(
                                "allow_unauthenticated_participants cannot be enabled if rtps_protection_kind is not none");
                            returned_value = false;
                            break;
                        }
                        else
                        {
                            ah->governance_rule_.allow_unauthenticated_participants = true;
                        }
                    }

                    ah->governance_rule_.plugin_participant_attributes = plug_part_attr.mask();

                    for (auto topic_rule : rule.topic_rules)
                    {
                        std::string topic_expression = topic_rule.topic_expression;
                        EndpointSecurityAttributes security_attributes;
                        PluginEndpointSecurityAttributes plugin_attributes;

                        security_attributes.is_discovery_protected = topic_rule.enable_discovery_protection;
                        security_attributes.is_liveliness_protected = topic_rule.enable_liveliness_protection;
                        security_attributes.is_read_protected = topic_rule.enable_read_access_control;
                        security_attributes.is_write_protected = topic_rule.enable_write_access_control;

                        bool hasEncryption =
                                (topic_rule.metadata_protection_kind == ProtectionKind::ENCRYPT) ||
                                (topic_rule.metadata_protection_kind ==
                                ProtectionKind::ENCRYPT_WITH_ORIGIN_AUTHENTICATION);
                        bool hasOriginAuth =
                                (topic_rule.metadata_protection_kind ==
                                ProtectionKind::ENCRYPT_WITH_ORIGIN_AUTHENTICATION) ||
                                (topic_rule.metadata_protection_kind ==
                                ProtectionKind::SIGN_WITH_ORIGIN_AUTHENTICATION);
                        plugin_attributes.is_submessage_encrypted = hasEncryption;
                        plugin_attributes.is_submessage_origin_authenticated = hasOriginAuth;

                        security_attributes.is_submessage_protected =
                                (topic_rule.metadata_protection_kind != ProtectionKind::NONE);

                        plugin_attributes.is_payload_encrypted =
                                security_attributes.is_key_protected =
                                (topic_rule.data_protection_kind == ProtectionKind::ENCRYPT);
                        security_attributes.is_payload_protected =
                                (topic_rule.data_protection_kind != ProtectionKind::NONE);

                        security_attributes.plugin_endpoint_attributes = plugin_attributes.mask();

                        ah->governance_topic_rules_.push_back(std::pair<std::string, EndpointSecurityAttributes>(
                                    std::move(topic_expression), std::move(security_attributes)));
                    }

                    break;
                }
            }
        }
        else
        {
            exception =
                    _SecurityException_(std::string(
                                "Not found the identity subject name in permissions file. Subject name: ") +
                            lih->cert_sn_rfc2253_);
        }
    }
    else
    {
        exception = _SecurityException_("SMIdentityHandle is not of the type PKISMIdentityHandle");
    }

    return returned_value;
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

static bool generate_permissions_token(
    AccessPermissionsHandle& handle,
    bool transmit_legacy_algorithms)
{
    Property property;
    PermissionsToken& token = handle->permissions_token_;
    token.class_id = "DDS:Access:Permissions:1.0";

    property.name = "dds.perm_ca.sn";
    property.value = handle->sn;
    property.propagate = true;
    token.properties.push_back(std::move(property));

    property.name = "dds.perm_ca.algo";
    property.value = convert_to_token_algo(handle->algo, transmit_legacy_algorithms);
    property.propagate = true;
    token.properties.push_back(std::move(property));

    return true;
}

static bool generate_credentials_token(
    AccessPermissionsHandle& handle,
    const std::string& file,
    SMSecurityException& exception)
{
    bool returned_value = false;
    // 创建 PermissionsCredentialToken;
    PermissionsCredentialToken& token = handle->permissions_credential_token_;
    token.class_id = "DDS:Access:PermissionsCredential";

    if (file.size() >= 7 && file.compare(0, 7, "file://") == 0)
    {
        try
        {
            std::ifstream ifs(file.substr(7).c_str());
            Property property;
            property.name = "dds.perm.cert";
            property.value.assign((std::istreambuf_iterator<char>(ifs)),
                    (std::istreambuf_iterator<char>()));
            property.propagate = true;
            token.properties.push_back(std::move(property));
            returned_value = true;
        }
        catch (std::exception&)
        {
            exception = _SecurityException_(std::string("Cannot find file ") + file);
        }
    }
    else
    {
        exception = _SecurityException_("Unsupported permissions_ca format");
    }

    return returned_value;
}
// 基类接口实现（适配器函数）
PermissionsHandle Permissions::validate_local_permissions(
    Authentication *auth_plugin,
    const IdentityHandle identity,
    const DomainId_t domain_id,
    const DomainParticipantQos *participant_qos,
    SecurityException *exception)
{
    if (auth_plugin == nullptr || participant_qos == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex = _SecurityException_("Invalid parameters");
            sm_ex.to_base_exception(exception);
            EMERGENCY_SECURITY_LOGGING("Permissions", "%s", sm_ex.what());
        }
        return SEC_NIL_HANDLE;
    }

    // 类型转换并调用实现函数
    const SMIdentityHandle& sm_identity = reinterpret_cast<const SMIdentityHandle&>(identity);
    SMSecurityException sm_exception;
    SMPermissionsHandle* result = validate_local_permissions_impl(*auth_plugin, sm_identity, domain_id, *participant_qos, sm_exception);
    
    // 转换返回类型
    if (result == nullptr)
    {
        sm_exception.to_base_exception(exception);
        return SEC_NIL_HANDLE;
    }
    return reinterpret_cast<PermissionsHandle>(result);
}

// 实现函数（保持原有逻辑）
SMPermissionsHandle* Permissions::validate_local_permissions_impl(
    Authentication& auth_plugin,
    const SMIdentityHandle& identity,
    const DomainId_t domain_id,
    const DomainParticipantQos& participant_qos,
    SMSecurityException& exception){
    PropertyQosPolicy access_properties = PropertyPolicyHelper::get_properties_with_prefix(participant_qos.property,
        "dds.sec.access.Access-Permissions."); // 参考设计说明中给出的配置参数
    
    if (PropertyPolicyHelper::length(access_properties) == 0)
    {
        exception = _SecurityException_("Not found any dds.sec.access.builtin.Access-Permissions property");
        EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
        return nullptr;
    }

    bool transmit_legacy_algorithms = false;
    std::string* legacy = PropertyPolicyHelper::find_property(access_properties, "transmit_algorithms_as_legacy");
    if (legacy != nullptr)
    {
        transmit_legacy_algorithms = (*legacy == "true");
    }

    std::string* permissions_ca = PropertyPolicyHelper::find_property(access_properties, "permissions_ca");

    if (permissions_ca == nullptr)
    {
        exception = _SecurityException_("Not found dds.sec.access.builtin.Access-Permissions.permissions_ca property");
        EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
        return nullptr;
    }

    std::string* governance = PropertyPolicyHelper::find_property(access_properties, "governance");

    if (governance == nullptr)
    {
        exception = _SecurityException_("Not found dds.sec.access.builtin.Access-Permissions.governance property");
        EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
        return nullptr;
    }

    std::string* permissions = PropertyPolicyHelper::find_property(access_properties, "permissions");

    if (permissions == nullptr)
    {
        exception = _SecurityException_("Not found dds.sec.access.builtin.Access-Permissions.permissions property");
        EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
        return nullptr;
    }

    AccessPermissionsHandle* ah = &AccessPermissionsHandle::narrow(*get_permissions_handle(exception));

    (*ah)->store_ = GMHelper::load_sm2_ca(*permissions_ca, (*ah)->there_are_crls_, (*ah)->sn, (*ah)->algo, exception);

    if ((*ah)->store_ != nullptr)
    {
        DomainAccessRules rules;
        if (load_governance_file(*ah, *governance, rules, exception))
        {
            PermissionsData permissions_data;
            if (load_permissions_file(*ah, *permissions, permissions_data, exception))
            {
                // Check subject name.
                if (check_subject_name(identity, *ah, domain_id, rules, permissions_data, exception))
                {
                    if (generate_permissions_token(*ah, transmit_legacy_algorithms))
                    {
                        if (generate_credentials_token(*ah, *permissions, exception))
                        {
                            return ah;
                        }
                    }
                }
            }
        }
    }

    delete ah;
    
    return nullptr;
}

SMPermissionsHandle* Permissions::get_permissions_handle(
    SMSecurityException&)
{
    return new (std::nothrow) AccessPermissionsHandle();
}

// 基类接口实现（适配器函数）
PermissionsHandle Permissions::validate_remote_permissions(
    Authentication *auth_plugin,
    const IdentityHandle local_identity_handle,
    const IdentityHandle remote_identity_handle,
    const PermissionsToken *remote_permissions_token,
    AuthenticatedPeerCredentialToken *remote_credential_token,
    SecurityException *exception)
{
    if (auth_plugin == nullptr || remote_permissions_token == nullptr || 
        remote_credential_token == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex = _SecurityException_("Invalid parameters");
            sm_ex.to_base_exception(exception);
            EMERGENCY_SECURITY_LOGGING("Permissions", "%s", sm_ex.what());
        }
        return SEC_NIL_HANDLE;
    }

    // 类型转换
    const SMIdentityHandle& local_sm_identity = reinterpret_cast<const SMIdentityHandle&>(local_identity_handle);
    const SMIdentityHandle& remote_sm_identity = reinterpret_cast<const SMIdentityHandle&>(remote_identity_handle);
    const PermissionsToken& sm_remote_permissions_token = *remote_permissions_token;
    const AuthenticatedPeerCredentialToken& sm_remote_credential_token = *remote_credential_token;

    // 获取本地权限句柄（这里可能需要从某个地方获取，暂时假设可以通过某种方式获得）
    SMSecurityException temp_ex;
    AccessPermissionsHandle* temp_handle = &AccessPermissionsHandle::narrow(*get_permissions_handle(temp_ex));
    const SMPermissionsHandle& local_permissions_handle = reinterpret_cast<const SMPermissionsHandle&>(*temp_handle);
    
    SMSecurityException sm_exception;
    // 调用实现函数
    SMPermissionsHandle* result = validate_remote_permissions_impl(*auth_plugin, local_sm_identity, local_permissions_handle, 
        remote_sm_identity, sm_remote_permissions_token, sm_remote_credential_token, sm_exception);
    
    delete temp_handle;
    
    // 转换返回类型
    if (result == nullptr)
    {
        sm_exception.to_base_exception(exception);
        return SEC_NIL_HANDLE;
    }
    return reinterpret_cast<PermissionsHandle>(result);
}

// 实现函数（保持原有逻辑）
SMPermissionsHandle* Permissions::validate_remote_permissions_impl(
        Authentication&,
        const SMIdentityHandle& local_identity_handle,
        const SMPermissionsHandle& local_permissions_handle,
        const SMIdentityHandle& remote_identity_handle,
        const PermissionsToken& remote_permissions_token,
        const AuthenticatedPeerCredentialToken& remote_credential_token,
        SMSecurityException& exception)
{
    const GMIdentityHandle& lih = GMIdentityHandle::narrow(local_identity_handle);
    const AccessPermissionsHandle& lph = AccessPermissionsHandle::narrow(local_permissions_handle);
    const GMIdentityHandle& rih = GMIdentityHandle::narrow(remote_identity_handle);

    if (lih.nil() || lph.nil() || rih.nil())
    {
        exception = _SecurityException_("Bad precondition");
        EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
        return nullptr;
    }

    // Check permissions.
    // Check c.id
    const std::string* sn = nullptr;
    for (auto it = remote_permissions_token.properties.begin(); 
         it != remote_permissions_token.properties.end(); ++it)
    {
        if (it->name.compare("dds.perm_ca.sn") == 0)
        {
            sn = &it->value;
            break;
        }
    }

    if (sn != nullptr)
    {
        if (sn->compare(lph->sn) != 0)
        {
            exception = _SecurityException_("Remote participant PermissionsCA differs from local");
            EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
            return nullptr;
        }
    }

    const std::string* algo = nullptr;
    for (auto it = remote_permissions_token.properties.begin(); 
         it != remote_permissions_token.properties.end(); ++it)
    {
        if (it->name.compare("dds.perm_ca.algo") == 0)
        {
            algo = &it->value;
            break;
        }
    }

    if (algo != nullptr)
    {
        std::string used_algo = parse_token_algo(*algo);
        if (used_algo.compare(lph->algo) != 0)
        {
            exception = _SecurityException_("Remote participant PermissionsCA algorithm differs from local");
            EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
            return nullptr;
        }
    }

    const std::string* permissions_file = nullptr;
    for (auto it = remote_credential_token.properties.begin(); 
         it != remote_credential_token.properties.end(); ++it)
    {
        if (it->name.compare("dds.perm.cert") == 0)
        {
            permissions_file = &it->value;
            break;
        }
    }

    if (permissions_file == nullptr)
    {
        exception = _SecurityException_("Remote participant doesn't sent the signed permissions file");
        EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
        return nullptr;
    }

    PermissionsData data;
    if (!verify_permissions_file(lph, *permissions_file, data, exception))
    {
        return nullptr;
    }

    Grant remote_grant;
    for (auto grant : data.grants)
    {
        if (is_validation_in_time(grant.validity))
        {
            if (rfc2253_string_compare(grant.subject_name, rih->cert_sn_rfc2253_) ||
                    strcmp(grant.subject_name.c_str(), rih->cert_sn_.c_str()) == 0)
            {
                remote_grant = std::move(grant);
                break;
            }
        }
    }

    if (remote_grant.subject_name.empty())
    {
        exception = _SecurityException_("Remote participant doesn't found in its permissions file");
        EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
        return nullptr;
    }

    AccessPermissionsHandle* handle = &AccessPermissionsHandle::narrow(*get_permissions_handle(exception));
    (*handle)->grant = std::move(remote_grant);
    (*handle)->governance_rule_ = lph->governance_rule_;
    (*handle)->governance_topic_rules_ = lph->governance_topic_rules_;

    return handle;
}

// 基类接口实现（适配器函数）
bool Permissions::check_create_participant(
    const PermissionsHandle permissions_handle,
    const DomainId_t domain_id,
    const DomainParticipantQos *qos,
    SecurityException *exception)
{
    if (qos == nullptr || exception == nullptr)
    {
        if (exception != nullptr)
        {
            SMSecurityException sm_ex = _SecurityException_("Invalid parameters");
            sm_ex.to_base_exception(exception);
            EMERGENCY_SECURITY_LOGGING("Permissions", "%s", sm_ex.what());
        }
        return false;
    }

    // 类型转换并调用实现函数
    const SMPermissionsHandle& sm_permissions_handle = reinterpret_cast<const SMPermissionsHandle&>(permissions_handle);
    SMSecurityException sm_exception;
    bool result = check_create_participant_impl(sm_permissions_handle, domain_id, *qos, sm_exception);
    
    if (!result)
    {
        sm_exception.to_base_exception(exception);
    }
    
    return result;
}

// 实现函数（保持原有逻辑）
bool Permissions::check_create_participant_impl(
    const SMPermissionsHandle& permissions_handle,
    const DomainId_t domain_id,
    const DomainParticipantQos& qos,
    SMSecurityException& exception){
        bool returned_value = false;
        const AccessPermissionsHandle& lah = AccessPermissionsHandle::narrow(permissions_handle);
    
        if (lah.nil())
        {
            exception = _SecurityException_("Bad precondition");
            EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
            return false;
        }
    
        for (auto rule : lah->grant.rules)
        {
            if (rule.allow)
            {
                returned_value = true;
                break;
            }
        }
    
        if (!returned_value)
        {
            exception = _SecurityException_("Not found a rule allowing to use the domain_id");
            EMERGENCY_SECURITY_LOGGING("Permissions", "%s", exception.what());
        }
    
        return returned_value;
}