//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef SECURITY_COMMON_H
#define SECURITY_COMMON_H

#include "cdr/travoddscdr.h"
#include "dcps/topic/typesupport.h"
#include "type/basictype.h"
#include <string>
#ifdef DDSSECURITY
TRAVODDS_NAMESPACE_BEGIN

typedef struct DDSSecurityPluginInstance
{
    void* authentication; //认证服务
    void* access_control; //访问控制
    void* crypto_key_factory;
    void* crypto_key_exchange;
    void* crypto_transform;
    Time_t retry_time;
    Time_t max_wait_time;
    unsigned int max_retry_number;
}DDSSecurityPluginInstance;

typedef bool(*GetSecurityPlugInstance)(DDSSecurityPluginInstance*);
typedef void(*FinalizeSecurityPlugInstance)();

typedef void* DDSSecHandle;

extern DDS_DLL const DDSSecHandle SEC_NIL_HANDLE;

extern std::string GMCLASSID_SECURITY_AUTH_HANDSHAKE;
extern std::string GMCLASSID_SECURITY_PARTICIPANT_CRYPTO_TOKENS;
extern std::string GMCLASSID_SECURITY_DATAWRITER_CRYPTO_TOKENS;
extern std::string GMCLASSID_SECURITY_DATAREADER_CRYPTO_TOKENS;

struct SecurityException
{
    /** @brief   异常代码。 */
    char code;
    /** @brief   异常子码。 */
    char minor_code;
    /** @brief   异常信息。 */
    char message[256];
};

typedef struct DataHolder
{
    /** @brief   当作为Token类型时，该域表示Token类型，dds.sec.为保留字符串,实现厂商使用域名的逆序。 */
    std::string class_id;
    /** @brief   具体配置。 */
	Sequence<Property_t> properties;
    /** @brief   具体配置。 */
	Sequence<BinaryProperty_t> binary_properties;
} DataHolder;

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

typedef unsigned int EndpointSecurityAttributesMask;
typedef unsigned int PluginEndpointSecurityAttributesMask;

typedef struct EndpointSecurityInfo
{
    EndpointSecurityAttributesMask endpoint_security_attributes = 0;
    PluginEndpointSecurityAttributesMask plugin_endpoint_security_attributes = 0;
}EndpointSecurityInfo;

extern const EndpointSecurityInfo ENDPOINT_SECURITY_ATTRIBUTES_INFO_DEFAULT;
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
typedef struct ParticipantSecurityAttributes
{
    /** @brief   是否允许与未验证身份的DomainParticipant匹配 */
    bool allow_unauthenticated_participants;
    /** @brief   两个DomainParticipant匹配是否需要AccessControl插件的授权 */
    bool is_access_protected;
    /** @brief   整个RTPS Message是否需要被CryptoTransform的encode_rtps_message操作进行转换. */
    bool is_rtps_protected;
    /** @brief   发现消息是否需要被保护. */
    bool is_discovery_protected;
    /** @brief   Liveliness消息是否需要被保护. */
    bool is_liveliness_protected;
    /** @brief   为CryptoKeyFactory的register_local_participant操作中的参数participant_properties附加的属性. */
    PropertySeq ac_participant_properties;
    /** @brief The plugin participant attributes */
    PluginParticipantSecurityAttributesMask plugin_participant_attributes;
}ParticipantSecurityAttributes;

/**
 * @typedef struct EndpointSecurityAttributes
 *
 * @brief   实体相关的安全配置.
 */
typedef struct EndpointSecurityAttributes
{
    /* **从 TopicSecurityAttributes 继承的属性 */
    /** @brief   是否需要使用访问控制插件对本地Reader创建和远端Reader匹配进行检查 */
    bool is_read_protected;
    /** @brief   是否需要使用访问控制插件对本地Writer创建和远端Writer匹配进行检查 */
    bool is_write_protected;
    /** @brief   对实体的发现信息发送时，是使用安全的内建的topics还是使用常规的topics. */
    bool is_discovery_protected;
    /** @brief   Liveliness消息是否需要被保护. */
    bool is_liveliness_protected;
    /* **继承自 TopicSecurityAttributes 的属性结束 */

    /** @brief   是否需要对子消息进行保护. */
    bool is_submessage_protected;
    /** @brief   是否需要对数据负载进行保护. */
    bool is_payload_protected;
    /** @brief   是否需要使用安全插件计算数据实例句柄. */
    bool is_key_protected;
    /** @brief   附加的属性. */
    PropertySeq ac_endpoint_properties;
    /** @brief The plugin endpoint attributes */
    PluginEndpointSecurityAttributesMask plugin_endpoint_attributes;
}EndpointSecurityAttributes;

struct CryptoTransformIdentifier_t
{
	char transformationKind[4];
	char transformationKeyId[4];
};

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

typedef struct Tag_t
{
	std::string name;
	std::string value;
}Tag_t;

typedef struct DataTags_t
{
	Sequence<Tag_t> tags;
}DataTags_t;

typedef DataTags_t DataTagQosPolicy;

typedef std::vector<DataHolder> DataHolderSeq;

typedef DataHolder Credential;
typedef Credential IdentityCredential;
typedef Credential PermissionsCredential;

typedef DataHolder Token;

typedef Token HandshakeMessageToken;
typedef Token IdentityToken; //身份令牌
typedef Token PermissionsToken; //权限令牌
typedef Token AuthenticatedPeerCredentialToken;
typedef Token PermissionsCredentialToken;

typedef Token CryptoToken;
typedef Token ParticipantCryptoToken;
typedef Token DatawriterCryptoToken;
typedef Token DatareaderCryptoToken;

typedef DataHolderSeq HandshakeMessageTokenSeq;
typedef DataHolderSeq CryptoTokenSeq;
typedef DataHolderSeq ParticipantCryptoTokenSeq;
typedef DataHolderSeq DatawriterCryptoTokenSeq;
typedef DataHolderSeq DatareaderCryptoTokenSeq;

//一些Qos属性成员

//适配DataHolder的序列化和反序列化
class DataHolderTypeSupport : public TRAVODDS::TypeSupport {
public:
    static DataHolderTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, CdrSerializer* cdr, int endian);

private:
    DataHolderTypeSupport() = default;
};

ParticipantSecurityAttributesMask ParticipantSecurityAttributesToMask(ParticipantSecurityAttributes* secAttr);

//EndpointSecurityAttributesMask TopicSecurityAttributesToMask(TopicSecurityAttributes* secAttr);

EndpointSecurityAttributesMask EndpointSecurityAttributesToMask(EndpointSecurityAttributes* secAttr);

TRAVODDS_NAMESPACE_END
#endif // DDSSECURITY
#endif // SECURITY_COMMON_H
