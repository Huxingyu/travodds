#ifndef SecurityCommon_h__
#define SecurityCommon_h__

#include <string>
#include <vector>

#include "travodds_dll.h"
// #include "rtps/common/Property.h"
#include "rtps/basictypertps.h"

TRAVODDS_NAMESPACE_BEGIN

// class Property
// {
// public:

//     Property()
//     {
//     }

//     Property(
//             const Property& property)
//         : name_(property.name_)
//         , value_(property.value_)
//         , propagate_(property.propagate_)
//     {
//     }

//     Property(
//             Property&& property)
//         : name_(std::move(property.name_))
//         , value_(std::move(property.value_))
//         , propagate_(property.propagate_)
//     {
//     }

//     Property(
//             const std::string& name,
//             const std::string& value,
//             bool propagate = false)
//         : name_(name)
//         , value_(value)
//         , propagate_(propagate)
//     {
//     }

//     Property(
//             std::string&& name,
//             std::string&& value,
//             bool propagate = false)
//         : name_(std::move(name))
//         , value_(std::move(value))
//         , propagate_(propagate)
//     {
//     }

//     Property& operator =(
//             const Property& property)
//     {
//         name_ = property.name_;
//         value_ = property.value_;
//         propagate_ = property.propagate_;
//         return *this;
//     }

//     Property& operator =(
//             Property&& property)
//     {
//         name_ = std::move(property.name_);
//         value_ = std::move(property.value_);
//         propagate_ = property.propagate_;
//         return *this;
//     }

//     bool operator ==(
//             const Property& b) const
//     {
//         return (this->name_ == b.name_) &&
//                (this->value_ == b.value_);
//     }

//     void name(
//             const std::string& name)
//     {
//         name_ = name;
//     }

//     void name(
//             std::string&& name)
//     {
//         name_ = std::move(name);
//     }

//     const std::string& name() const
//     {
//         return name_;
//     }

//     std::string& name()
//     {
//         return name_;
//     }

//     void value(
//             const std::string& value)
//     {
//         value_ = value;
//     }

//     void value(
//             std::string&& value)
//     {
//         value_ = std::move(value);
//     }

//     const std::string& value() const
//     {
//         return value_;
//     }

//     std::string& value()
//     {
//         return value_;
//     }

//     void propagate(
//             bool propagate)
//     {
//         propagate_ = propagate;
//     }

//     bool propagate() const
//     {
//         return propagate_;
//     }

//     bool& propagate()
//     {
//         return propagate_;
//     }

// private:

//     std::string name_;

//     std::string value_;

//     bool propagate_ = false;
// };

// class BinaryProperty
// {
// public:

//     BinaryProperty()
//         : propagate_(false)
//     {
//     }

//     BinaryProperty(
//             const BinaryProperty& property)
//         : name_(property.name_)
//         , value_(property.value_)
//         , propagate_(property.propagate_)
//     {
//     }

//     BinaryProperty(
//             BinaryProperty&& property)
//         : name_(std::move(property.name_))
//         , value_(std::move(property.value_))
//         , propagate_(property.propagate_)
//     {
//     }

//     BinaryProperty(
//             const std::string& name,
//             const std::vector<uint8_t>& value)
//         : name_(name)
//         , value_(value)
//     {
//     }

//     BinaryProperty(
//             std::string&& name,
//             std::vector<uint8_t>&& value)
//         : name_(std::move(name))
//         , value_(std::move(value))
//     {
//     }

//     BinaryProperty& operator =(
//             const BinaryProperty& property)
//     {
//         name_ = property.name_;
//         value_ = property.value_;
//         propagate_ = property.propagate_;
//         return *this;
//     }

//     BinaryProperty& operator =(
//             BinaryProperty&& property)
//     {
//         name_ = std::move(property.name_);
//         value_ = std::move(property.value_);
//         propagate_ = property.propagate_;
//         return *this;
//     }

//     bool operator ==(
//             const BinaryProperty& b) const
//     {
//         return (this->name_ == b.name_) &&
//                (this->value_ == b.value_);
//     }

//     void name(
//             const std::string& name)
//     {
//         name_ = name;
//     }

//     void name(
//             std::string&& name)
//     {
//         name_ = std::move(name);
//     }

//     const std::string& name() const
//     {
//         return name_;
//     }

//     std::string& name()
//     {
//         return name_;
//     }

//     void value(
//             const std::vector<uint8_t>& value)
//     {
//         value_ = value;
//     }

//     void value(
//             std::vector<uint8_t>&& value)
//     {
//         value_ = std::move(value);
//     }

//     const std::vector<uint8_t>& value() const
//     {
//         return value_;
//     }

//     std::vector<uint8_t>& value()
//     {
//         return value_;
//     }

//     void propagate(
//             bool propagate)
//     {
//         propagate_ = propagate;
//     }

//     bool propagate() const
//     {
//         return propagate_;
//     }

//     bool& propagate()
//     {
//         return propagate_;
//     }

// private:

//     std::string name_;

//     std::vector<uint8_t> value_;

//     bool propagate_;
// };

// //typedef std::vector<Property> PropertySeq;
// typedef std::vector<BinaryProperty> BinaryPropertySeq;

typedef struct SecurityPluginInstance
{
    void* authentication;
    void* access_control;
    void* crypto_key_factory;
    void* crypto_key_exchange;
    void* crypto_transform;
}SecurityPluginInstance;

typedef bool(*GetSecurityPlugInstance)(SecurityPluginInstance*);
typedef void(*FinalizeSecurityPlugInstance)();

typedef void* DDSSecHandle;

extern const DDSSecHandle SEC_NIL_HANDLE;

extern char* GMCLASSID_SECURITY_AUTH_HANDSHAKE;
extern char* GMCLASSID_SECURITY_PARTICIPANT_CRYPTO_TOKENS;
extern char* GMCLASSID_SECURITY_DATAWRITER_CRYPTO_TOKENS;
extern char* GMCLASSID_SECURITY_DATAREADER_CRYPTO_TOKENS;

/**
 * @typedef int SecurityExceptionCode
 *
 * @brief   表示安全插件异常代码。
 */

typedef int SecurityExceptionCode;

/**
 * @struct  SecurityException
 *
 * @brief   表示安全插件异常信息。
 */

// struct SecurityException
// {
//     /** @brief   异常代码。 */
//     SecurityExceptionCode code;
//     /** @brief   异常子码。 */
//     int minor_code;
//     /** @brief   异常信息。 */
//     char message[256];
// };

/**
 * @struct  DataHolder
 *
 * @brief   用来存储一般性的数据，包括多样的属性来存储不同类型与格式的数据，作为其他类型的组成部分，例如Token、GenericMessageData
 */

// typedef struct DataHolder
// {
//     /** @brief   当作为Token类型时，该域表示Token类型，dds.sec.为保留字符串,实现厂商使用域名的逆序。 */
//     char* class_id;
//     /** @brief   具体配置。 */
//     PropertySeq properties;
//     /** @brief   具体配置。 */
//     BinaryPropertySeq binary_properties;
// } DataHolder;

typedef unsigned int ParticipantSecurityAttributesMask;
typedef unsigned int PluginParticipantSecurityAttributesMask;

typedef struct ParticipantSecurityInfo
{
    ParticipantSecurityAttributesMask  participant_security_atributes;
    PluginParticipantSecurityAttributesMask plugin_participant_security_attributes;
}ParticipantSecurityInfo;

extern const ParticipantSecurityInfo PARTICIPANT_SECURITY_ATTRIBUTES_INFO_DEFAULT;
#define PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID (0x1 << 31)
#define PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_PROTECTED (0x00000001 << 0)
#define PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED (0x00000001 << 1)
#define PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED (0x00000001 << 2)

// typedef unsigned int EndpointSecurityAttributesMask;
// typedef unsigned int PluginEndpointSecurityAttributesMask;

// typedef struct EndpointSecurityInfo
// {
//     EndpointSecurityAttributesMask endpoint_security_attributes;
//     PluginEndpointSecurityAttributesMask plugin_endpoint_security_attributes;
// }EndpointSecurityInfo;

// extern const EndpointSecurityInfo ENDPOINT_SECURITY_ATTRIBUTES_INFO_DEFAULT;
#define ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID (0x1 << 31)
#define ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_READ_PROTECTED (0x00000001 << 0)
#define ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_WRITE_PROTECTED (0x00000001 << 1)
#define ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED (0x00000001 << 2)
#define ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_PROTECTED (0x00000001 << 3)
#define ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_PROTECTED (0x00000001 << 4)
#define ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_KEY_PROTECTED (0x00000001 << 5)
#define ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED (0x00000001 << 6)

/**
 * @typedef struct ParticipantSecurityAttributes
 *
 * @brief   DomainParticipant相关的安全配置.
 */
// typedef struct ParticipantSecurityAttributes
// {
//     /** @brief   是否允许与未验证身份的DomainParticipant匹配 */
//     bool allow_unauthenticated_participants;
//     /** @brief   两个DomainParticipant匹配是否需要AccessControl插件的授权 */
//     bool is_access_protected;
//     /** @brief   整个RTPS Message是否需要被CryptoTransform的encode_rtps_message操作进行转换. */
//     bool is_rtps_protected;
//     /** @brief   发现消息是否需要被保护. */
//     bool is_discovery_protected;
//     /** @brief   Liveliness消息是否需要被保护. */
//     bool is_liveliness_protected;
//     /** @brief   为CryptoKeyFactory的register_local_participant操作中的参数participant_properties附加的属性. */
//     PropertySeq ac_participant_properties;
//     /** @brief The plugin participant attributes */
//     PluginParticipantSecurityAttributesMask plugin_participant_attributes;
// }ParticipantSecurityAttributes;

/**
 * @typedef struct TopicSecurityAttributes
 *
 * @brief 主题相关安全配置.
 */
typedef struct TopicSecurityAttributes
{
    /** @brief   是否需要使用访问控制插件对本地Reader创建和远端Reader匹配进行检查 */
    bool is_read_protected;
    /** @brief   是否需要使用访问控制插件对本地Writer创建和远端Writer匹配进行检查 */
    bool is_write_protected;
    /** @brief   对实体的发现信息发送时，是使用安全的内建的topics还是使用常规的topics. */
    bool is_discovery_protected;
    /** @brief   Liveliness消息是否需要被保护. */
    bool is_liveliness_protected;
}TopicSecurityAttributes;

/**
 * @typedef struct EndpointSecurityAttributes
 *
 * @brief   实体相关的安全配置.
//  */
// typedef struct EndpointSecurityAttributes
// {
//     /* **从 TopicSecurityAttributes 继承的属性 */
//     /** @brief   是否需要使用访问控制插件对本地Reader创建和远端Reader匹配进行检查 */
//     bool is_read_protected;
//     /** @brief   是否需要使用访问控制插件对本地Writer创建和远端Writer匹配进行检查 */
//     bool is_write_protected;
//     /** @brief   对实体的发现信息发送时，是使用安全的内建的topics还是使用常规的topics. */
//     bool is_discovery_protected;
//     /** @brief   Liveliness消息是否需要被保护. */
//     bool is_liveliness_protected;
//     /* **继承自 TopicSecurityAttributes 的属性结束 */

//     /** @brief   是否需要对子消息进行保护. */
//     bool is_submessage_protected;
//     /** @brief   是否需要对数据负载进行保护. */
//     bool is_payload_protected;
//     /** @brief   是否需要使用安全插件计算数据实例句柄. */
//     bool is_key_protected;
//     /** @brief   附加的属性. */
//     PropertySeq ac_endpoint_properties;
//     /** @brief The plugin endpoint attributes */
//     PluginEndpointSecurityAttributesMask plugin_endpoint_attributes;
// }EndpointSecurityAttributes;

typedef struct CryptoPayloadEncodeAttributes
{
    unsigned int headerFixLength;
    unsigned int encodeDataLengthPercentage;
    unsigned int tailFixLength;
}CryptoPayloadEncodeAttributes;

typedef struct CryptoWriterSubmessageAttributes
{
    unsigned int headerFixLength;
    unsigned int headerMutableLengthPerReader;
    unsigned int encodeDataLengthPercentage;
    unsigned int tailFixLength;
    unsigned int tailMutableLengthPerReader;
}CryptoWriterSubmessageAttributes;

typedef struct CryptoReaderSubmessageAttributes
{
    unsigned int headerFixLength;
    unsigned int headerMutableLengthPerWriter;
    unsigned int encodeDataLengthPercentage;
    unsigned int tailFixLength;
    unsigned int tailMutableLengthPerWriter;
}CryptoReaderSubmessageAttributes;

typedef struct CryptoRtpsMessageAttributes
{
    unsigned int headerFixLength;
    unsigned int headerMutableLengthPerParticipant;
    unsigned int encodeDataLengthPercentage;
    unsigned int tailFixLength;
    unsigned int tailMutableLengthPerParticipant;
}CryptoRtpsMessageAttributes;

// typedef std::vector<DataHolder> DataHolderSeq;

// typedef DataHolder Credential;
// typedef Credential IdentityCredential;
// typedef Credential PermissionsCredential;

// typedef DataHolder Token;

// typedef Token HandshakeMessageToken;
// typedef Token IdentityToken;
// typedef Token PermissionsToken;
// typedef Token AuthenticatedPeerCredentialToken;
// typedef Token PermissionsCredentialToken;

// typedef Token CryptoToken;
// typedef Token ParticipantCryptoToken;
// typedef Token DatawriterCryptoToken;
// typedef Token DatareaderCryptoToken;

// typedef DataHolderSeq HandshakeMessageTokenSeq;
// typedef DataHolderSeq CryptoTokenSeq;
// typedef DataHolderSeq ParticipantCryptoTokenSeq;
// typedef DataHolderSeq DatawriterCryptoTokenSeq;
// typedef DataHolderSeq DatareaderCryptoTokenSeq;

typedef struct Tag
{
    std::string name;
    std::string value;
}Tag;

typedef std::vector<Tag> TagSeq;

typedef struct DataTags
{
    TagSeq tags;
}DataTags;

typedef DataTags DataTagQosPolicy;

// ParticipantSecurityAttributesMask ParticipantSecurityAttributesToMask(ParticipantSecurityAttributes* secAttr);

// EndpointSecurityAttributesMask TopicSecurityAttributesToMask(TopicSecurityAttributes* secAttr);

// EndpointSecurityAttributesMask EndpointSecurityAttributesToMask(EndpointSecurityAttributes* secAttr);

TRAVODDS_NAMESPACE_END

#endif /* SecurityCommon_h__ */
