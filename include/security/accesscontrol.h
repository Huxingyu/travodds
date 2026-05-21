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
#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H


#include "security/securitycommon.h"
#include "security/authenticationlistener.h"
#include "type/basictype.h"
#include "type/builtintopicdata.h"
#include "qos/qos.h"
#ifdef DDSSECURITY
TRAVODDS_NAMESPACE_BEGIN

typedef DDSSecHandle PermissionsHandle;
typedef DDSSecHandle IdentityHandle;
class AccessControlListener;
class Authentication;
class DataWriter;
class DataReader;

class AccessControl
{
public:
    virtual ~AccessControl() {};
    /**
     * @fn  PermissionsHandle BuiltinAccessControl::validate_local_permissions( const AuthenticationPlugin* auth_plugin, const IdentityHandle identity, const DDS_DomainId_t domain_id, const DomainParticipantQos* participant_qos, SecurityException* exception);
     *
     * @brief   确认本地域参与者的权限.
     *
     * @param   auth_plugin         授权插件，确认域参与者的权限.
     * @param   identity            本地域参与者的身份标识，来自验证插件的validate_local_identity函数.
     * @param   domain_id           域号.
     * @param   participant_qos     域参与者的QoS.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  成功则返回PermissionsHandle,否则返回空.
     */
    virtual PermissionsHandle validate_local_permissions(
        Authentication *auth_plugin,
        const IdentityHandle identity,
        const DomainId_t domain_id,
        const DomainParticipantQos *participant_qos,
        SecurityException *exception) = 0;

    /**
     * @fn  PermissionsHandle BuiltinAccessControl::validate_remote_permissions( const AuthenticationPlugin* auth_plugin, const IdentityHandle local_identity_handle, const IdentityHandle remote_identity_handle, const PermissionsToken* remote_permissions_token, const AuthenticatedPeerCredentialToken* remote_credential_token, SecurityException** exception);
     *
     * @brief  确认远端身份验证过的域参与者的权限.
     *
     * @param   auth_plugin                 验证插件，用于确认远端域参与者的身份.
     * @param   local_identity_handle       本地域参与者的身份标识.
     * @param   remote_identity_handle      远端域参与者的身份标识.
     * @param   remote_permissions_token    远端域参与者的PermissionsToken.
     * @param   remote_credential_token     远端域参与者的AuthenticatedPeerCredentialToken.
     * @param [in,out]  exception           对于异常的描述.
     *
     * @return  成功返回PermissionsHandle，否则返回空.
     */
    virtual PermissionsHandle validate_remote_permissions(
        Authentication *auth_plugin,
        const IdentityHandle local_identity_handle,
        const IdentityHandle remote_identity_handle,
        const PermissionsToken *remote_permissions_token,
        AuthenticatedPeerCredentialToken *remote_credential_token,
        SecurityException *exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::check_create_participant( PermissionsHandle permissions_handle, DDS_DomainId_t domain_id, DomainParticipantQos qos, SecurityException* exception);
     *
     * @brief   检测是否有创建满足条件的域参与者的权限.
     *
     * @param   permissions_handle  本地域参与者的权限标识.
     * @param   domain_id           域号.
     * @param   qos                 域参与者的QoS.
     * @param   exception           对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool check_create_participant(
        const PermissionsHandle permissions_handle,
        const DomainId_t domain_id,
        const DomainParticipantQos *qos,
        SecurityException *exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::check_create_datawriter( const PermissionsHandle permissions_handle, const DDS_DomainId_t domain_id, const char* topic_name, const DataWriterQos* qos, const PartitionQosPolicy* partition, const DataTag* data_tag, SecurityException* exception);
     *
     * @brief   验证本地是否有权限创建符合要求的DataWriter.
     *
     * @param   permissions_handle  本地域参与者的权限标识.
     * @param   domain_id           域号.
     * @param   topic_name          主题名.
     * @param   qos                 DataWriter的QoS.
     * @param   partition           DataWriter所属Publisher的PartitionQosPolicy。
     * @param   data_tag            和本地DataWriter的data相匹配的数据标签。
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool check_create_datawriter(
        const PermissionsHandle permissions_handle,
        const DomainId_t domain_id,
        const std::string& topic_name,
        const DataWriterQos *qos,
        const PartitionQosPolicy *partition,
        const DataTags_t *data_tag,
        SecurityException *exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::check_create_datareader( const PermissionsHandle permissions_handle, const DDS_DomainId_t domain_id, const char* topic_name, const DataReaderQos* qos, const PartitionQosPolicy* partition, const DataTag* data_tag, SecurityException* exception);
     *
     * @brief   验证本地是否权限创建符合要求的DataReader。
     *
     * @param   permissions_handle  本地域参与者的权限标识。
     * @param   domain_id           域号。
     * @param   topic_name          主题名。
     * @param   qos                 DataReader的Qos.
     * @param   partition           DataReader所属的Subscriber的PartitionQosPolicy.
     * @param   data_tag            本地DataReader正在读取的数据的标签.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool check_create_datareader(
        const PermissionsHandle permissions_handle,
        const DomainId_t domain_id,
        const std::string& topic_name,
        const DataReaderQos *qos,
        const PartitionQosPolicy *partition,
        const DataTags_t *data_tag,
        SecurityException *exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::check_create_topic( const PermissionsHandle permisssions_handle, const DDS_DomainId_t domain_id, const char* topic_name, const TopicQos* qos, SecurityException* exception);
     *
     * @brief   确认本地是否有创建满足条件的Topic的权限.
     *
     * @param   permisssions_handle 本地域参与者的权限标识.
     * @param   domain_id           域号.
     * @param   topic_name          主题名.
     * @param   qos                 Topic的QoS.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  如果成功返回true，否侧返回false.
     */
    virtual bool check_create_topic(
        const PermissionsHandle permissions_handle,
        const DomainId_t domain_id,
        const char *topic_name,
        const TopicQos *qos,
        SecurityException *exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::check_remote_participant( const PermissionsHandle permissions_handle, const DDS_DomainId_t domain_id, const ParticipantBuiltinTopicDataSecure* participant_data, SecurityException* exception);
     *
     * @brief   确认远端的域参与者是否允许加入DDS Domain.
     *
     * @param   permissions_handle  远端域参与者的权限标识.
     * @param   domain_id           域号.
     * @param   participant_data    远端域参与者的ParticipantBuiltinTopicDataSecure.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  如果成功返回true，否则返回false.
     */
    virtual bool check_remote_participant(
        const PermissionsHandle permissions_handle,
        const DomainId_t domain_id,
        const ParticipantBuiltinTopicData *participant_data, //DDS_ParticipantBuiltinTopicDataSecure
        SecurityException *exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::check_remote_datawriter( const PermissionsHandle permissions_handle, const DDS_DomainId_t domain_id, const PublicationBuiltinTopicDataSecure* publication_data, SecurityException* exception);
     *
     * @brief   检测远端的DataWriter.
     *
     * @param   permissions_handle  远端域参与者的标识.
     * @param   domain_id           域号.
     * @param   publication_data    DataWriter相关的PublicationBuiltinTopicDataSecure.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  如果成功返回true，否则返回false.
     */
    virtual bool check_remote_datawriter(
        const PermissionsHandle permissions_handle,
        const DomainId_t domain_id,
        const PublicationBuiltinTopicSecureData* publication_data,
        SecurityException* exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::check_remote_datareader( const PermissionsHandle permissions_handle, const DDS_DomainId_t domain_id, const SubscriptionBuiltinTopicDataSecure subscription_data, bool relay_only, SecurityException* exception);
     *
     * @brief   检测远端的DataReader.
     *
     * @param   permissions_handle  远端的权限标识.
     * @param   domain_id           域号.
     * @param   subscription_data   和远端DataReader相关的SubscriptionBuiltinTopicDataSecure.
     * @param [in,out]  relay_only  决定远端DataReader是否破译接收到的信息。
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  如果成功返回true，否则返回false.
     */
    virtual bool check_remote_datareader(
        const PermissionsHandle permissions_handle,
        const DomainId_t domain_id,
        const SubscriptionBuiltinTopicSecureData* subscription_data,
        bool relay_only,
        SecurityException* exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::check_remote_topic( const PermissionsHandle permissions_handle, const DDS_DomainId_t domain_id, const TopicBuiltinTopicData* topic_data, SecurityException* exception);
     *
     * @brief   检测远端的Topic.
     *
     * @param   permissions_handle  远端的权限标识.
     * @param   domain_id           域号.
     * @param   topic_data          和Topic相关联的TopicBuiltInTopicData.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool check_remote_topic(
        const PermissionsHandle permissions_handle,
        const DomainId_t domain_id,
        const TopicBuiltinTopicData* topic_data,
        SecurityException* exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::check_local_datawriter_match( const PermissionsHandle writer_permisions_handle, const PermissionsHandle reader_permissions_handle, const PartitionQosPolicy* publisher_partition, const DataTag* writer_data_tag, const DataTag* reader_data_tag, SecurityException* exception);
     *
     * @brief   检测本地DataWriter的匹配.
     *
     * @param   writer_permisions_handle    和DataWriter相关的域参与者的权限标识.
     * @param   reader_permissions_handle   和DataReader相关的域参与者的权限标识.
     * @param   publisher_partition         和DataWriter相关的Publisher的PartitionQosPolicy.
     * @param   writer_data_tag             和DataWriter相关的DataTags.
     * @param   reader_data_tag             和DataReader相关的DataTags.
     * @param [in,out]  exception           对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool check_local_datawriter_match(
        const PermissionsHandle writer_permisions_handle,
        const PermissionsHandle reader_permissions_handle,
        const PartitionQosPolicy* publisher_partition,
        const DataTags_t* writer_data_tag,
        const DataTags_t* reader_data_tag,
        SecurityException* exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::check_local_datareader_match( const PermissionsHandle reader_permissions_handle, const PermissionsHandle writer_permissions_handle, const PartitionQosPolicy* subscriber_partiton, const DataTag* reader_data_tag, const DataTag* writer_data_tag, SecurityException* exception);
     *
     * @brief   检测本地DataReader的匹配.
     *
     * @param   reader_permissions_handle   DataReader相关的域参与者的权限标识.
     * @param   writer_permissions_handle   DataWrtier相关的域参与者的权限标识.
     * @param   subscriber_partiton         和DataReader相关的Subscriber的PartitionQosPolicy.
     * @param   reader_data_tag             和DataReader相关的DataTags.
     * @param   writer_data_tag             和DataWriter相关的DataTags.
     * @param [in,out]  exception           对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool check_local_datareader_match(
        const PermissionsHandle reader_permissions_handle,
        const PermissionsHandle writer_permissions_handle,
        const PartitionQosPolicy* subscriber_partiton,
        const DataTags_t* reader_data_tag,
        const DataTags_t* writer_data_tag,
        SecurityException* exception) = 0;

    /**
     * @fn  PermissionsToken BuiltinAccessControl::get_permissions_token( const PermissionsHandle handle, SecurityException* exception);
     *
     * @brief  获得PermissionsToken.
     *
     * @param   permissions_token   保存获得PermissionsToken的信息.
     * @param   handle              域参与者的权限标识.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  The permissions token.
     */
    virtual bool get_permissions_token(
        PermissionsToken* permissions_token,
        const PermissionsHandle handle,
        SecurityException* exception) = 0;

    /**
     * @fn  PermissionsCredentialToken BuiltinAccessControl::get_permissions_credential_token( const PermissionsHandle handle, SecurityException* exception);
     *
     * @brief   获得PermissionsCredentialToken.
     *
     * @param   permissions_credential_token              保存获得PermissionsCredentialToken信息.
     * @param   handle              本地域参与者的权限标识.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  The permissions credential token.
     */
    virtual bool get_permissions_credential_token(
        PermissionsCredentialToken* permissions_credential_token,
        const PermissionsHandle handle,
        SecurityException* exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::set_listener( const AccessControlListener* listener, SecurityException* exception);
     *
     * @brief   为AccessControl插件设置监听器.
     *
     * @param   listener            监听器.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool set_listener(
        AccessControlListener* listener,
        SecurityException* exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::return_permissions_token( const PermissionsToken* token, SecurityException* exception);
     *
     * @brief   归还PermissionsToken所占的空间.
     *
     * @param   token               需要归还空间的PermissionsToken.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool return_permissions_token(
        PermissionsToken* token,
        SecurityException* exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::return_permissions_credential_token( const PermissionsCredentialToken* permissions_credential_token, SecurityException* exception);
     *
     * @brief   归还PermissionsCredentialToken所占的空间.
     *
     * @param   permissions_credential_token    需要归还空间的PermissionsCredentialToken.
     * @param [in,out]  exception               对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool return_permissions_credential_token(
        PermissionsCredentialToken* permissions_credential_token,
        SecurityException* exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::get_participant_sec_attributes( const PermissionsHandle permissions_handle, ParticipantSecurityAttributes* attributes, SecurityException* exception);
     *
     * @brief   获得ParticipantSecurityAttributes信息.
     *
     * @param   permissions_handle  本地域参与者的权限标识.
     * @param [in,out]  attributes  保存ParticipantSecurityAttributes信息.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool get_participant_sec_attributes(
        const PermissionsHandle permissions_handle,
        ParticipantSecurityAttributes* attributes,
        SecurityException* exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::get_datawriter_sec_attributes( const PermissionsHandle permissions_handle, const char* topic_name, const PartitionQosPolicy* partition, const DataTagQosPolicy* data_tag, EndpointSecurityAttributes* attributes, SecurityException* exception);
     *
     * @brief   获得DataWriter的EndpointSecurityAttributes信息.
     *
     * @param   permissions_handle  本地域参与者的权限标识.
     * @param   topic_name          主题名.
     * @param   partition           DataWriter所属的Publisher的PartitionQosPolicy.
     * @param   data_tag            DataWriter的DataTagQosPolicy.
     * @param [in,out]  attributes   保存EndpointSecurityAttributes信息。.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool get_datawriter_sec_attributes(
        const PermissionsHandle permissions_handle,
        const char* topic_name,
        const PartitionQosPolicy* partition,
        const DataTagQosPolicy* data_tag,
        EndpointSecurityAttributes* attributes,
        SecurityException* exception) = 0;

    /**
     * @fn  bool BuiltinAccessControl::get_datareader_sec_attributes( const PermissionsHandle permissions_handle, const char* topic_name, const PartitionQosPolicy* partition, const DataTagQosPolicy* data_tag, EndpointSecurityAttributes* attributes, SecurityException* exception);
     *
     * @brief   获得DataReader的EndpointSecurityAttributes.
     *
     * @param   permissions_handle  本地的域参与者的权限标识.
     * @param   topic_name          主题名.
     * @param   partition           DataReader所属的Subscriber的PartitionQosPolicy.
     * @param   data_tag            DataReader的DataTagQosPolicy.
     * @param [in,out]  attributes   保存EndpointSecurityAttributes信息.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  成功返回true，否则返回false.
     */
    virtual bool get_datareader_sec_attributes(
        const PermissionsHandle permissions_handle,
        const char* topic_name,
        const PartitionQosPolicy* partition,
        const DataTagQosPolicy* data_tag,
        EndpointSecurityAttributes* attributes,
        SecurityException* exception) = 0;

    /**
     * @fn  virtual bool AccessControl::return_permissions_handle( PermissionsHandle permissions_handle, SecurityException* exception)=0;
     *
     * @brief   释放handle所占用的空间.
     *
     * @param   permissions_handle  需要释放空间PermissionsHandle.
     * @param [in,out]  exception   对于异常的描述.
     *
     * @return  成功返回true，失败返回false.
     */
    virtual bool return_permissions_handle(
        PermissionsHandle permissions_handle,
        SecurityException* exception) = 0;

    /**
     * @fn  virtual bool AccessControl::return_participant_sec_attributes( ParticipantSecurityAttributes* attributes, SecurityException* exception) = 0;
     *
     * @brief    释放ParticipantSecurityAttributes所占用的空间.
     *
     * @param  attributes          需要释放空间的attributes.
     * @param [in,out]  exception  对于异常的描述.
     *
     * @return  成功返回true，失败放回false.
     */
    virtual bool return_participant_sec_attributes(
        ParticipantSecurityAttributes* attributes,
        SecurityException* exception) = 0;
    /**
     * @fn  virtual bool AccessControl::return_participant_sec_attributes( ParticipantSecurityAttributes* attributes, SecurityException* exception) = 0;
     *
     * @brief    释放EndpointSecurityAttributes所占用的空间.
     *
     * @param  attributes          需要释放空间的attributes.
     * @param [in,out]  exception  对于异常的描述.
     *
     * @return  成功返回true，失败放回false.
     */
    virtual bool return_datawriter_sec_attributes(
        EndpointSecurityAttributes* attributes,
        SecurityException* exception) = 0;
    /**
     * @fn  virtual bool AccessControl::return_participant_sec_attributes( ParticipantSecurityAttributes* attributes, SecurityException* exception) = 0;
     *
     * @brief    释放EndpointSecurityAttributes所占用的空间.
     *
     * @param  attributes          需要释放空间的attributes.
     * @param [in,out]  exception  对于异常的描述.
     *
     * @return  成功返回true，失败放回false.
     */
    virtual bool return_datareader_sec_attributes(
        EndpointSecurityAttributes* attributes,
        SecurityException* exception) = 0;
};

TRAVODDS_NAMESPACE_END
#endif // DDSSECURIT
#endif // ACCESS_CONTROL_H
