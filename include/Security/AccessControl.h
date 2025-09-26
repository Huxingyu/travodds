#ifndef AccessControl_h__
#define AccessControl_h__

#include "travodds_dll.h"
#include "SecurityCommon.h"
#include "rtps/common/Token.h"
#include "qos/qos.h"
#include "Security/SecurityException.h"

TRAVODDS_NAMESPACE_BEGIN

class DataWriter;
class DataReader;
class DynamicData;

// typedef DDSSecHandle PermissionsHandle;
// typedef DDSSecHandle IdentityHandle;
typedef struct TopicBuiltinTopicData TopicBuiltinTopicData;
class AccessControlListener;
class Authentication;
class ParticipantBuiltinTopicDataSecure;
class PublicationBuiltinTopicDataSecure;
class SubscriptionBuiltinTopicDataSecure;
struct ParticipantSecurityAttributes;
struct EndpointSecurityAttributes;

class AccessControl
{
public:
    virtual ~AccessControl() = default;
    /**
    * @fn PermissionsHandle* validate_local_permissions(Authentication& auth_plugin, const IdentityHandle& identity, const DomainId_t domain_id, const DomainParticipantQos& participant_qos, SecurityException& exception)
    *
    * @brief 确认本地域参与者的权限.
    *
    * @param auth_plugin     授权插件，确认域参与者的权限.
    * @param identity        本地域参与者的身份标识，来自验证插件的validate_local_identity函数.
    * @param domain_id       域号.
    * @param participant_qos 域参与者的QoS.
    * @param[in,out] exception 对于异常的描述.
    *
    * @return 成功则返回PermissionsHandle指针,否则返回nullptr.
    */

    virtual PermissionsHandle* validate_local_permissions(
        Authentication& auth_plugin,
        const IdentityHandle& identity,
        const DomainId_t domain_id,
        const DomainParticipantQos& participant_qos,
        SecurityException& exception) = 0;

    /**
    * @fn PermissionsHandle* validate_remote_permissions(Authentication& auth_plugin, const IdentityHandle& local_identity_handle, const PermissionsHandle& local_permissions_handle, const IdentityHandle& remote_identity_handle, const PermissionsToken& remote_permissions_token, const AuthenticatedPeerCredentialToken& remote_credential_token, SecurityException& exception)
    *
    * @brief 确认远端身份验证过的域参与者的权限.
    *
    * @param auth_plugin             验证插件，用于确认远端域参与者的身份.
    * @param local_identity_handle   本地域参与者的身份标识.
    * @param local_permissions_handle 本地域参与者的权限标识.
    * @param remote_identity_handle  远端域参与者的身份标识.
    * @param remote_permissions_token 远端域参与者的PermissionsToken.
    * @param remote_credential_token 远端域参与者的AuthenticatedPeerCredentialToken.
    * @param[in,out] exception       对于异常的描述.
    *
    * @return 成功返回PermissionsHandle指针，否则返回nullptr.
    */

    virtual PermissionsHandle* validate_remote_permissions(
        Authentication& auth_plugin,
        const IdentityHandle& local_identity_handle,
        const PermissionsHandle& local_permissions_handle,
        const IdentityHandle& remote_identity_handle,
        const PermissionsToken& remote_permissions_token,
        const AuthenticatedPeerCredentialToken& remote_credential_token,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_create_participant(const PermissionsHandle& permissions_handle, const DomainId_t domain_id, const DomainParticipantQos& qos, SecurityException& exception)
    *
    * @brief 检测是否有创建满足条件的域参与者的权限.
    *
    * @param permissions_handle 本地域参与者的权限标识.
    * @param domain_id          域号.
    * @param qos                域参与者的QoS.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 有权限则返回true，否则返回false.
    */

    virtual bool check_create_participant(
        const PermissionsHandle& permissions_handle,
        const DomainId_t domain_id,
        const DomainParticipantQos& qos,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_create_datawriter(const PermissionsHandle& permissions_handle, const DomainId_t domain_id, const std::string& topic_name, const DataWriterQos& qos, const PartitionQosPolicy& partition, const DataTags& data_tag, SecurityException& exception)
    *
    * @brief 验证本地是否有权限创建符合要求的DataWriter.
    *
    * @param permissions_handle 本地域参与者的权限标识.
    * @param domain_id          域号.
    * @param topic_name         主题名.
    * @param qos                DataWriter的QoS.
    * @param partition          DataWriter所属Publisher的PartitionQosPolicy.
    * @param data_tag           和本地DataWriter的data相匹配的数据标签.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 有权限则返回true，否则返回false.
    */

    virtual bool check_create_datawriter(
        const PermissionsHandle& permissions_handle,
        const DomainId_t domain_id,
        const std::string& topic_name,
        const DataWriterQos& qos,
        const PartitionQosPolicy& partition,
        const DataTags& data_tag,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_create_datareader(const PermissionsHandle& permissions_handle, const DomainId_t domain_id, const std::string& topic_name, const DataReaderQos& qos, const PartitionQosPolicy& partition, const DataTags& data_tag, SecurityException& exception)
    *
    * @brief 验证本地是否有权限创建符合要求的DataReader.
    *
    * @param permissions_handle 本地域参与者的权限标识.
    * @param domain_id          域号.
    * @param topic_name         主题名.
    * @param qos                DataReader的QoS.
    * @param partition          DataReader所属Subscriber的PartitionQosPolicy.
    * @param data_tag           本地DataReader正在读取的数据的标签.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 有权限则返回true，否则返回false.
    */

    virtual bool check_create_datareader(
        const PermissionsHandle& permissions_handle,
        const DomainId_t domain_id,
        const std::string& topic_name,
        const DataReaderQos& qos,
        const PartitionQosPolicy& partition,
        const DataTags& data_tag,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_create_topic(const PermissionsHandle& permissions_handle, const DomainId_t domain_id, const std::string& topic_name, const TopicQos& qos, SecurityException& exception)
    *
    * @brief 确认本地是否有创建满足条件的Topic的权限.
    *
    * @param permissions_handle 本地域参与者的权限标识.
    * @param domain_id          域号.
    * @param topic_name         主题名.
    * @param qos                Topic的QoS.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 有权限则返回true，否则返回false.
    */

    virtual bool check_create_topic(
        const PermissionsHandle& permissions_handle,
        const DomainId_t domain_id,
        const std::string& topic_name,
        const TopicQos& qos,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_local_datawriter_register_instance(const PermissionsHandle& permissions_handle, const DataWriter& writer, const DynamicData& key, SecurityException& exception)
    *
    * @brief 验证本地的DataWriter是否有注册实例的权限.
    *
    * @param permissions_handle 本地域参与者的权限标识.
    * @param writer             本地的DataWriter.
    * @param key                实例的key.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 有权限则返回true，否则返回false.
    */

    virtual bool check_local_datawriter_register_instance(
        const PermissionsHandle& permissions_handle,
        const DataWriter& writer,
        const DynamicData& key,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_local_datawriter_dispose_instance(const PermissionsHandle& permissions_handle, const DataWriter& writer, const DynamicData& key, SecurityException& exception)
    *
    * @brief 确认本地DataWriter是否有处理实例的权限.
    *
    * @param permissions_handle 本地的权限标识.
    * @param writer             本地的DataWriter.
    * @param key                实例的key.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 有权限则返回true，否则返回false.
    */

    virtual bool check_local_datawriter_dispose_instance(
        const PermissionsHandle& permissions_handle,
        const DataWriter& writer,
        const DynamicData& key,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_remote_participant(const PermissionsHandle& permissions_handle, const DomainId_t domain_id, const ParticipantBuiltinTopicDataSecure& participant_data, SecurityException& exception)
    *
    * @brief 确认远端的域参与者是否允许加入DDS Domain.
    *
    * @param permissions_handle 远端域参与者的权限标识.
    * @param domain_id          域号.
    * @param participant_data   远端域参与者的ParticipantBuiltinTopicDataSecure.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 允许则返回true，否则返回false.
    */

    virtual bool check_remote_participant(
        const PermissionsHandle& permissions_handle,
        const DomainId_t domain_id,
        const ParticipantBuiltinTopicDataSecure& participant_data,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_remote_datawriter(const PermissionsHandle& permissions_handle, const DomainId_t domain_id, const PublicationBuiltinTopicDataSecure& publication_data, SecurityException& exception)
    *
    * @brief 检测远端的DataWriter.
    *
    * @param permissions_handle 远端域参与者的标识.
    * @param domain_id          域号.
    * @param publication_data   DataWriter相关的PublicationBuiltinTopicDataSecure.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 检查通过则返回true，否则返回false.
    */

    virtual bool check_remote_datawriter(
        const PermissionsHandle& permissions_handle,
        const DomainId_t domain_id,
        const PublicationBuiltinTopicDataSecure& publication_data,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_remote_datareader(const PermissionsHandle& permissions_handle, const DomainId_t domain_id, const SubscriptionBuiltinTopicDataSecure& subscription_data, bool& relay_only, SecurityException& exception)
    *
    * @brief 检测远端的DataReader.
    *
    * @param permissions_handle  远端的权限标识.
    * @param domain_id           域号.
    * @param subscription_data   和远端DataReader相关的SubscriptionBuiltinTopicDataSecure.
    * @param[in,out] relay_only  [out] 决定远端DataReader是否破译接收到的信息.
    * @param[in,out] exception   对于异常的描述.
    *
    * @return 检查通过则返回true，否则返回false.
    */

    virtual bool check_remote_datareader(
        const PermissionsHandle& permissions_handle,
        const DomainId_t domain_id,
        const SubscriptionBuiltinTopicDataSecure& subscription_data,
        bool& relay_only,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_remote_topic(const PermissionsHandle& permissions_handle, const DomainId_t domain_id, const TopicBuiltinTopicData& topic_data, SecurityException& exception)
    *
    * @brief 检测远端的Topic.
    *
    * @param permissions_handle 远端的权限标识.
    * @param domain_id          域号.
    * @param topic_data         和Topic相关联的TopicBuiltInTopicData.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 检查通过则返回true，否则返回false.
    */

    virtual bool check_remote_topic(
        const PermissionsHandle& permissions_handle,
        const DomainId_t domain_id,
        const TopicBuiltinTopicData& topic_data,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_local_datawriter_match(const PermissionsHandle& writer_permissions_handle, const PermissionsHandle& reader_permissions_handle, const PartitionQosPolicy& publisher_partition, const DataTags& writer_data_tag, const DataTags& reader_data_tag, SecurityException& exception)
    *
    * @brief 检测本地DataWriter与远端DataReader的匹配.
    *
    * @param writer_permissions_handle 和DataWriter相关的域参与者的权限标识.
    * @param reader_permissions_handle 和DataReader相关的域参与者的权限标识.
    * @param publisher_partition       和DataWriter相关的Publisher的PartitionQosPolicy.
    * @param writer_data_tag           和DataWriter相关的DataTags.
    * @param reader_data_tag           和DataReader相关的DataTags.
    * @param[in,out] exception         对于异常的描述.
    *
    * @return 允许匹配则返回true，否则返回false.
    */

    virtual bool check_local_datawriter_match(
        const PermissionsHandle& writer_permissions_handle,
        const PermissionsHandle& reader_permissions_handle,
        const PartitionQosPolicy& publisher_partition,
        const DataTags& writer_data_tag,
        const DataTags& reader_data_tag,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_local_datareader_match(const PermissionsHandle& reader_permissions_handle, const PermissionsHandle& writer_permissions_handle, const PartitionQosPolicy& subscriber_partition, const DataTags& reader_data_tag, const DataTags& writer_data_tag, SecurityException& exception)
    *
    * @brief 检测本地DataReader与远端DataWriter的匹配.
    *
    * @param reader_permissions_handle 和DataReader相关的域参与者的权限标识.
    * @param writer_permissions_handle 和DataWriter相关的域参与者的权限标识.
    * @param subscriber_partition      和DataReader相关的Subscriber的PartitionQosPolicy.
    * @param reader_data_tag           和DataReader相关的DataTags.
    * @param writer_data_tag           和DataWriter相关的DataTags.
    * @param[in,out] exception         对于异常的描述.
    *
    * @return 允许匹配则返回true，否则返回false.
    */

    virtual bool check_local_datareader_match(
        const PermissionsHandle& reader_permissions_handle,
        const PermissionsHandle& writer_permissions_handle,
        const PartitionQosPolicy& subscriber_partition,
        const DataTags& reader_data_tag,
        const DataTags& writer_data_tag,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_remote_datawriter_register_instance(const PermissionsHandle& permissions_handle, const DataReader& reader, const InstanceHandle_t& publication_handle, const DynamicData& key, const InstanceHandle_t& instance_handle, SecurityException& exception)
    *
    * @brief 检测远端DataWriter注册实例的权限.
    *
    * @param permissions_handle 远端域参与者的权限标识.
    * @param reader             和远端DataWriter相匹配的DataReader.
    * @param publication_handle 远端DataWriter的唯一标识.
    * @param key                实例的key，该实例需要去匹配注册实例所需的permissions.
    * @param instance_handle    实例的唯一标识.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 有权限则返回true，否则返回false.
    */

    virtual bool check_remote_datawriter_register_instance(
        const PermissionsHandle& permissions_handle,
        const DataReader& reader,
        const InstanceHandle_t& publication_handle,
        const DynamicData& key,
        const InstanceHandle_t& instance_handle,
        SecurityException& exception) = 0;

    /**
    * @fn bool check_remote_datawriter_dispose_instance(const PermissionsHandle& permissions_handle, const DataReader& reader, const InstanceHandle_t& publication_handle, const DynamicData& key, SecurityException& exception)
    *
    * @brief 检测远端DataWriter处理实例的权限.
    *
    * @param permissions_handle 远端域参与者的权限标识.
    * @param reader             和DataWriter匹配的本地的DataReader.
    * @param publication_handle DataWriter的唯一标识.
    * @param key                实例的key.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 有权限则返回true，否则返回false.
    */

    virtual bool check_remote_datawriter_dispose_instance(
        const PermissionsHandle& permissions_handle,
        const DataReader& reader,
        const InstanceHandle_t& publication_handle,
        const DynamicData& key,
        SecurityException& exception) = 0;

    /**
    * @fn bool get_permissions_token(PermissionsToken** permissions_token, const PermissionsHandle& handle, SecurityException& exception)
    *
    * @brief 创建并获取PermissionsToken.
    *
    * @param[out] permissions_token 用于接收新创建的PermissionsToken对象的指针地址.
    * @param handle              本地域参与者的权限标识.
    * @param[in,out] exception   对于异常的描述.
    *
    * @return 成功返回true，否则返回false.
    */

    virtual bool get_permissions_token(
        PermissionsToken** permissions_token,
        const PermissionsHandle& handle,
        SecurityException& exception) = 0;

    /**
    * @fn bool get_permissions_credential_token(PermissionsCredentialToken** permissions_credential_token, const PermissionsHandle& handle, SecurityException& exception)
    *
    * @brief 创建并获取PermissionsCredentialToken.
    *
    * @param[out] permissions_credential_token 用于接收新创建的PermissionsCredentialToken对象的指针地址.
    * @param handle                           本地域参与者的权限标识.
    * @param[in,out] exception                对于异常的描述.
    *
    * @return 成功返回true，否则返回false.
    */

    virtual bool get_permissions_credential_token(
        PermissionsCredentialToken** permissions_credential_token,
        const PermissionsHandle& handle,
        SecurityException& exception) = 0;

    /**
    * @fn bool set_listener(AccessControlListener* listener, SecurityException& exception)
    *
    * @brief 为AccessControl插件设置监听器.
    *
    * @param listener          要设置的监听器.
    * @param[in,out] exception 对于异常的描述.
    *
    * @return 成功返回true，否则返回false.
    */

    virtual bool set_listener(
        AccessControlListener* listener,
        SecurityException& exception) = 0;

    /**
    * @fn bool return_permissions_token(PermissionsToken* token, SecurityException& exception)
    *
    * @brief 归还并释放PermissionsToken所占用的资源.
    *
    * @param token             需要释放的PermissionsToken指针.
    * @param[in,out] exception 对于异常的描述.
    *
    * @return 成功返回true，否则返回false.
    */

    virtual bool return_permissions_token(
        PermissionsToken* token,
        SecurityException& exception) = 0;

    /**
    * @fn bool return_permissions_credential_token(PermissionsCredentialToken* permissions_credential_token, SecurityException& exception)
    *
    * @brief 归还并释放PermissionsCredentialToken所占用的资源.
    *
    * @param permissions_credential_token 需要释放的PermissionsCredentialToken指针.
    * @param[in,out] exception            对于异常的描述.
    *
    * @return 成功返回true，否则返回false.
    */

    virtual bool return_permissions_credential_token(
        PermissionsCredentialToken* permissions_credential_token,
        SecurityException& exception) = 0;

    /**
    * @fn bool get_participant_sec_attributes(const PermissionsHandle& permissions_handle, ParticipantSecurityAttributes& attributes, SecurityException& exception)
    *
    * @brief 获取并填充ParticipantSecurityAttributes信息.
    *
    * @param permissions_handle 本地域参与者的权限标识.
    * @param[out] attributes    用于接收并填充ParticipantSecurityAttributes信息的对象.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 成功返回true，否则返回false.
    */

    virtual bool get_participant_sec_attributes(
        const PermissionsHandle& permissions_handle,
        ParticipantSecurityAttributes& attributes,
        SecurityException& exception) = 0;

    /**
    * @fn bool get_datawriter_sec_attributes(const PermissionsHandle& permissions_handle, const std::string& topic_name, const PartitionQosPolicy& partition, const DataTagQosPolicy& data_tag, EndpointSecurityAttributes& attributes, SecurityException& exception)
    *
    * @brief 获取并填充DataWriter的EndpointSecurityAttributes信息.
    *
    * @param permissions_handle 本地域参与者的权限标识.
    * @param topic_name         主题名.
    * @param partition          DataWriter所属的Publisher的PartitionQosPolicy.
    * @param data_tag           DataWriter的DataTagQosPolicy.
    * @param[out] attributes    用于接收并填充EndpointSecurityAttributes信息的对象.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 成功返回true，否则返回false.
    */

    virtual bool get_datawriter_sec_attributes(
        const PermissionsHandle& permissions_handle,
        const std::string& topic_name,
        const PartitionQosPolicy& partition,
        const DataTagQosPolicy& data_tag,
        EndpointSecurityAttributes& attributes,
        SecurityException& exception) = 0;

    /**
    * @fn bool get_datareader_sec_attributes(const PermissionsHandle& permissions_handle, const std::string& topic_name, const PartitionQosPolicy& partition, const DataTagQosPolicy& data_tag, EndpointSecurityAttributes& attributes, SecurityException& exception)
    *
    * @brief 获取并填充DataReader的EndpointSecurityAttributes信息.
    *
    * @param permissions_handle 本地域参与者的权限标识.
    * @param topic_name         主题名.
    * @param partition          DataReader所属的Subscriber的PartitionQosPolicy.
    * @param data_tag           DataReader的DataTagQosPolicy.
    * @param[out] attributes    用于接收并填充EndpointSecurityAttributes信息的对象.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 成功返回true，否则返回false.
    */

    virtual bool get_datareader_sec_attributes(
        const PermissionsHandle& permissions_handle,
        const std::string& topic_name,
        const PartitionQosPolicy& partition,
        const DataTagQosPolicy& data_tag,
        EndpointSecurityAttributes& attributes,
        SecurityException& exception) = 0;

    /**
    * @fn bool return_permissions_handle(PermissionsHandle* permissions_handle, SecurityException& exception)
    *
    * @brief 归还并释放PermissionsHandle所占用的资源.
    *
    * @param permissions_handle 需要释放的PermissionsHandle指针.
    * @param[in,out] exception  对于异常的描述.
    *
    * @return 成功返回true，否则返回false.
    */

    virtual bool return_permissions_handle(
        PermissionsHandle* permissions_handle,
        SecurityException& exception) = 0;

    // /**
    //  * @fn  virtual bool AccessControl::return_participant_sec_attributes( ParticipantSecurityAttributes* attributes, SecurityException& exception) = 0;
    //  *
    //  * @brief    释放ParticipantSecurityAttributes所占用的空间.
    //  *
    //  * @date     2018/5/21
    //  *
    //  * @param  attributes      需要释放空间的attributes.
    //  * @param [in,out]  exception  对于异常的描述.
    //  *
    //  * @return   成功返回true，失败放回false.
    //  */
    //
    // virtual bool return_participant_sec_attributes(
    //      ParticipantSecurityAttributes* attributes,
    //      SecurityException& exception) = 0;
    // /**
    // * @fn  virtual bool AccessControl::return_participant_sec_attributes( ParticipantSecurityAttributes* attributes, SecurityException& exception) = 0;
    // *
    // * @brief    释放EndpointSecurityAttributes所占用的空间.
    // *
    // * @date     2018/5/21
    // *
    // * @param  attributes      需要释放空间的attributes.
    // * @param [in,out]  exception  对于异常的描述.
    // *
    // * @return   成功返回true，失败放回false.
    // */
    // virtual bool return_datawriter_sec_attributes(
    //      EndpointSecurityAttributes* attributes,
    //      SecurityException& exception) = 0;
    // /**
    // * @fn  virtual bool AccessControl::return_participant_sec_attributes( ParticipantSecurityAttributes* attributes, SecurityException& exception) = 0;
    // *
    // * @brief    释放EndpointSecurityAttributes所占用的空间.
    // *
    // * @date     2018/5/21
    // *
    // * @param  attributes      需要释放空间的attributes.
    // * @param [in,out]  exception  对于异常的描述.
    // *
    // * @return   成功返回true，失败放回false.
    // */
    // virtual bool return_datareader_sec_attributes(
    //      EndpointSecurityAttributes* attributes,
    //      SecurityException& exception) = 0;
};

TRAVODDS_NAMESPACE_END

#endif // AccessControlPlugin_h__