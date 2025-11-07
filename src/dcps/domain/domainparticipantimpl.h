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
#ifndef TRAVODDS_DCPS_DOMAIN_DOMAINPARTICIPANTIMPL_H
#define TRAVODDS_DCPS_DOMAIN_DOMAINPARTICIPANTIMPL_H

#include "dcps/domain/domainparticipant.h"
#include "dcps/base/entityimpl.h"
#include "dcps/publish/publisherimpl.h"
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/topic/topicimpl.h"
#include "dcps/topic/contentfilteredtopicimpl.h"
#include "dcps/topic/multitopicimpl.h"
#include "dcps/listener/topiclistener.h"
#include "common/threads/threadpool.h"
#include <condition_variable>

#include "security/securitycommon.h"
#include "security/authenticationlistener.h"
#include "security/cryptotransform.h"
#include "security/accesscontrol.h"
#include "security/crycommonsource.h"
#include "security/cryptokeyfactory.h"
#include "security/cryptokeyexchange.h"

TRAVODDS_NAMESPACE_BEGIN

class Participant;
class Discovery;
class LivelinessManager;

#ifdef DDSSECURITY

struct SecurityPluginInstanceImpl
{
    Authentication* authentication = nullptr;
    AccessControl* access_control; //访问控制
    CryptoKeyFactory* crypto_key_factory;
    CryptoKeyExchange* crypto_key_exchange;
    CryptoTransform* crypto_transform;
    Time_t retry_time;
    Time_t max_wait_time;
    unsigned int max_retry_number;
};

#endif // DDSSECURITY

/**
 * @brief DomainParticipantImpl类是DomainParticipant类的子类和功能具体实现，
 * 代表域中应用程序的本地成员资格。
 */
class DomainParticipantImpl : public virtual DomainParticipant, public EntityImpl {
public:
    /**
     * @brief 构造函数。
     *
     * @param [in] domainId    域ID。
     * @param [in] qos	       DomainParticipant Qos。
     * @param [in] listener	   DomainParticipant监听器。
     * @param [in] mask		   StatusMask位掩码。
     * @param [in] secPlugin   安全插件实例。
     */
#ifdef DDSSECURITY
    DomainParticipantImpl(DomainId_t domainId,
        const DomainParticipantQos& qos,
        DomainParticipantListener* listener,
        StatusMask mask,
        DDSSecurityPluginInstance* secPlugin);
#endif // DDSSECURITY

    DomainParticipantImpl(DomainId_t domainId,
        const DomainParticipantQos& qos,
        DomainParticipantListener* listener,
        StatusMask mask);

    /**
     * @brief 析构函数。
     */
    virtual ~DomainParticipantImpl();

    /**
     * @brief 为此实例设置DomainParticipantQos。
     *
     * @param [in] qos DomainParticipantQos
     */
    ReturnCode_t set_qos(const DomainParticipantQos& qos);

    /**
     * @brief 获取此实例的DomainParticipantQos设置。
     *
     * @return qos DomainParticipantQos。
     */
    ReturnCode_t get_qos(DomainParticipantQos& qos) const;

    /**
     * @brief 在DomainParticipant中注册一个监听器。
     *
     * 监听器收到的通知取决于它注册时使用的状态掩码。
     *
     * 通过将监听器设置为NULL来执行监听器取消注册。
     *
     * @param [in] listener 监听器句柄。
     *
     * @param mask 定义将通知监听器的StatusMask位掩码。
     */
    ReturnCode_t set_listener(DomainParticipantListener* listener, const StatusMask& mask);

    /**
     * @brief 获取此DomainParticipant的监听器。
     *
     * @return 监听器句柄。
     */
    const DomainParticipantListener* get_listener() const;

    /**
     * @brief 在此参与者中创建发布者。
     *
     * @param [in] qos 发布者的QoS
     * @param [in] listenerer 指向监听器的指针。
     * @param [in] mask StatusMask位掩码，用于指定哪些特定事件会触发监听器的回调。
     *
     * @return 指向已创建发布者的指针。
     */
    Publisher* create_publisher(const PublisherQos& qos, PublisherListener* listener, const StatusMask& mask);

    /**
     * @brief 删除现有发布者。
     *
     * @param [in] publisher 要删除的发布者句柄。
     *
     * @return 如果发布者不属于此参与者，或者如果它有活动的数据写入器，
     * 则返回RETCODE_PRECONDITIONNOT_MET，如果正确删除了RETCODE_OK，否则返回RETCODE_ERROR。
     */
    ReturnCode_t delete_publisher(Publisher*& publisher);

    /**
     * @brief 在此参与者中创建订阅者。
     *
     * @param [in] qos 订阅者的QoS
     * @param [in] listenerer 指向监听器的指针。
     * @param [in] mask StatusMask位掩码。
     *
     * @return 指向已创建订阅者的指针。
     */
    Subscriber* create_subscriber(const SubscriberQos& qos, SubscriberListener* listener, const StatusMask& mask);

    /**
     * @brief 删除现有订阅者。
     *
     * @param [in] publisher 要删除的订阅者句柄。
     *
     * @return 如果订阅者不属于此参与者，或者如果它有活动的数据写入器，
     * 则返回RETCODE_PRECONDITIONNOT_MET，如果正确删除了RETCODE_OK，否则返回RETCODE_ERROR。
     */
    ReturnCode_t delete_subscriber(Subscriber*& subscriber);

    /**
     * @brief 在此参与者中创建主题。
     *
     * @param [in] topic_name 主题名.
     * @param [in] type_name  主题的数据类型。
     * @param [in] qos        主题的Qos.
     * @param [in] listener   指向监听器的指针。
     * @param [in] mask       StatusMask位掩码，保存监听器响应的状态。
     *
     * @return 指向已创建主题的指针。
     */
    Topic* create_topic(const std::string& topic_name, const std::string& type_name, const TopicQos& qos, TopicListener* listener, const StatusMask& mask);

    /**
     * @brief 删除现有主题。
     *
     * @param [in] topic 要删除的主题。
     *
     * @return 如果传递的主题为nullptr，则返回RETCODE_BAD_MEMETER；
     * 如果主题不属于此参与者或被任何实体引用，则返回RETCODE_PRECONDITIONNOT_MET；
     * 如果主题被删除，则返回RETCODE_OK。
     */
    ReturnCode_t delete_topic(Topic*& topic);

    /**
     * @brief 在此参与者中创建ContentFilteredTopic。
     *
     * @param [in] name                  内容筛选主题的名称。
     * @param [in] related_topic         与订阅相关的主题。
     * @param [in] filter_expression     创建筛选器的逻辑表达式。
     * @param [in] expression_parameters 筛选内容的参数。
     *
     * @return 指向创建的ContentFilteredTopic的指针。
     */
    ContentFilteredTopic* create_contentfilteredtopic(const std::string& name, Topic* related_topic, const std::string& filter_expression, const StringSeq& expression_parameters);

    /**
     * @brief 删除现有的ContentFilteredTopic。
     *
     * @param [in] contentfilteredtopic 要删除的内容筛选主题。
     *
     * @return 如果传递的主题为空ptr，则返回RETCODE_BAD_MEMETER；
     * 如果主题不属于此参与者或被任何实体引用，则返回RETCODE_PRECONDITIONNOT_MET；
     * 如果删除了ContentFilteredTopic，则返回RETCODE_OK。
     */
    ReturnCode_t delete_contentfilteredtopic(ContentFilteredTopic*& contentfilteredtopic);

    /**
     * @brief 在此参与者中创建多主题。
     *
     * @param [in] name                    多主题名称。
     * @param [in] type_name               多主题的结果类型。
     * @param [in] subscription_expression 组合过滤器的逻辑表达式。
     * @param [in] expression_parameters   订阅内容的参数。
     *
     * @return 指向创建的ContentFilteredTopic的指针，在错误情况下为nullptr。
     */
    MultiTopic* create_multitopic(const std::string& name, const std::string& type_name, const std::string& subscription_expression, const StringSeq& expression_parameters);

    /**
     * @brief 删除现有的多主题。
     *
     * @param [in] multitopic 要删除的多主题。
     *
     * @return 如果传递的主题为nullptr，则返回RETCODE_BAD_MEMETER；
     * 如果主题不属于此参与者或被任何实体引用，则返回RETCODE_PRECONDITIONNOT_MET；
     * 如果主题被删除，则返回RETCODE_OK。
     */
    ReturnCode_t delete_multitopic(MultiTopic*& multitopic);

    /**
     * @brief 允许访问已启用（或准备启用）的主题。
     * 通过此方法获得的主题必须被delete_topic销毁。
     *
     * @param [in] topic_name 要获取的主题的主题名。
     * @param [in] timeout    等待主题的最长时间。
     *
     * @return 指向现有主题的指针，在错误情况下为nullptr。
     */
    Topic* find_topic(const std::string& topic_name, const Duration_t& timeout);

    /**
     * @brief 允许访问已启用（或准备启用）的主题。
     * 通过此方法获得的主题必须被delete_topic销毁。
     *
     * @param [in] topic_name 要获取的主题的主题名。
     * @param [in] timeout    等待主题的最长时间。
     *
     * @return 指向现有主题的指针，在错误情况下为nullptr。
     */
    TopicDescription* lookup_topicdescription(const std::string& name) const;

    /**
     * @brief 允许访问内置订阅者。
     *
     * @return 指向内置订阅者的指针，在错误情况下为nullptr
     */
    const Subscriber* get_builtin_subscriber() const;

    /**
     * @brief 在本地忽略一个远程域参与者。
     *
     * @note 此函数不需要是可逆的。
     *
     * @param [in] handle 要忽略的远程参与者的标识符。
     *
     * @return 如果一切正常，返回RETCODE_OK代码，否则返回错误代码。
     */
    ReturnCode_t ignore_participant(const InstanceHandle_t& handle);

    /**
     * @brief 在本地忽略一个主题。
     *
     * @note 此函数不需要是可逆的。
     *
     * @param [in] handle 要忽略的主题的标识符。
     *
     * @return 如果一切正常，返回RETCODE_OK代码，否则返回错误代码。
     */
    ReturnCode_t ignore_topic(const InstanceHandle_t& handle);

    /**
     * @brief 在本地忽略数据写者。
     *
     * @note 此函数不需要是可逆的。
     *
     * @param [in] handle 要忽略的数据写者的标识符。
     *
     * @return 如果一切正常，返回RETCODE_OK代码，否则返回错误代码。
     */
    ReturnCode_t ignore_publication(const InstanceHandle_t& handle);

    /**
     * @brief 在本地忽略数据读者。
     *
     * @note 此函数不需要是可逆的。
     *
     * @param [in] handle 要忽略的数据读者的标识符
     *
     * @return 如果一切正常，返回RETCODE_OK代码，否则返回错误代码。
     */
    ReturnCode_t ignore_subscription(const InstanceHandle_t& handle);

    /**
     * @brief 此函数检索用于创建DomainParticipant的domain_id。
     * domain_id为标识DomainParticipant所属的DDS域。
     *
     * @return 参与者的domain_id。
     */
	DomainId_t get_domain_id() const;

    /**
     * @brief 删除通过“create”方法创建的所有实体
     *
     * @return 如果一切正常，返回RETCODE_OK代码，否则返回错误代码
     */
    ReturnCode_t delete_contained_entities();

    /**
     * @brief 此函数手动声明DomainParticipant的活力。
     * 这与LIVELINESS QoS策略结合使用，向LIVELINESS服务标示实体正保持活跃。
     *
     * 仅当DomainParticipant包含DataWriter实体时，才需要使用此函数
     * LIVELINESS设置为MANUAL_BY_PARTICIPANT，它只影响这些DataWriter实体的活跃度。
     * 否则，它没有任何效果。
     *
     * @note 通过DataWriter上的数据发送函数发送数据，可以保证DataWriter本身及其DomainParticipant的活力。
     * 因此，只有当应用程序没有定期发送数据时，才需要使用assert_liveliness。
     *
     * @return 如果liveliness激活成功返回RETCODE_OK, 否则返回RETCODE_ERROR.
     */
    ReturnCode_t assert_liveliness();

    /**
     * @brief 此函数设置发布者的QoS策略的默认值，
     * 在create_Publisher函数中默认QoS策略的情况下，
     * 该值将用于新创建的发布者实体。
     *
     * 此函数将检查生成的策略是否自洽；
     * 如果不是，则函数将无效并返回false。
     *
     * 可以将特殊值PUBLISHER_QOS_DEFAULT传递给此函数，
     * 以指示默认QOS应重置回工厂将使用的初始值，
     * 即如果从未调用set_default_publisher_qos函数时将使用的值。
     *
     * @param [in] qos 待设置的PublisherQos。
     *
     * @return 如果Qos不是自一致的，则返回RETCODE_INCONSISTENT_POLICY；
     * 如果Qos正确更改，则返回RETCODE_OK。
     */
    ReturnCode_t set_default_publisher_qos(const PublisherQos& qos);

    /**
     * @brief 此函数检索发布者QoS的默认值，
     * 即在create_publisher函数中默认QoS策略的情况下，
     * 将用于新创建的发布服务器实体的QoS策略。
     *
     * 检索到的get_default_publisher_qos值将与上次成功调用
     * set_default_ppublisher_qos时指定的值集匹配，
     * 否则，如果从未进行过调用，则使用默认值。
     *
     * @param [in] qos 返回default_publisher_qos的PublisherQos引用。
     *
     * @return RETCODE_OK
     */
    ReturnCode_t get_default_publisher_qos(PublisherQos& qos) const;

    /**
     * @brief 此函数设置将用于新创建的订阅服务器QoS策略的默认值
     * 在create_subscriber函数中默认QoS策略的情况下的订阅者实体。
     *
     * 此函数将检查生成的策略是否自洽；如果不是，则函数将无效并返回false。
     *
     * 可以将特殊值SUBSCRIBER_QOS_DEFAULT传递给此函数，
     * 以指示默认QOS应重置回工厂将使用的初始值，
     * 即如果从未调用set_default_subscriber_qos函数时将使用的值。
     *
     * @param [in] qos 要设置的订阅者QoS。
     *
     * @return 如果Qos不是自一致的，则返回RETCODE_INCONSISTENT_POLICY；
     * 如果Qos正确更改，则返回RETCODE_OK。
     */
    ReturnCode_t set_default_subscriber_qos(const SubscriberQos& qos);

    /**
     * @brief 此函数检索订阅者QoS的默认值，
     * 即在create_subscriber函数中默认QoS策略的情况下，
     * 将用于新创建的发布服务器实体的QoS策略。
     *
     * 检索到的get_default_subscriber_qos值将与上次成功调用
     * set_default_subscriber_qos时指定的值集匹配，
     * 否则，如果从未进行过调用，则使用默认值。
     *
     * @param [in] qos 返回default_subscriber_qos的SubscriberQos引用。
     *
     * @return RETCODE_OK
     */
    ReturnCode_t get_default_subscriber_qos(SubscriberQos& qos) const;

    /**
     * @brief 此函数设置Topic QoS策略的默认值，
     * 在create_topic函数中默认QoS策略的情况下，
     * 该默认值将用于新创建的Topic实体。
     *
     * 此函数将检查生成的策略是否自洽；如果不是，函数将无效，并返回RETCODE_INCONSISTENT_POLICY。
     *
     * 可以将特殊值TOPIC_QOS_DEFAULT传递给此函数，
     * 以指示默认QOS应重置回工厂将使用的初始值，
     * 即如果从未调用set_default_topic_qos函数时将使用的值。
     *
     * @param [in] qos 要设置的TopicQos
     *
     * @return 如果Qos不是自一致的，则返回RETCODE_INCONSISTENT_POLICY；
     * 如果Qos正确更改，则返回RETCODE_OK。
     */
    ReturnCode_t set_default_topic_qos(const TopicQos& qos);

    /**
     * @brief 此函数检索主题QoS的默认值，即将使用的QoS策略
     * 在create_topic函数中默认QoS策略的情况下，用于新创建的Topic实体。
     *
     * 检索到的get_default_topic_qos值将与上次成功调用
     * set_default_tpic_qos时指定的值集匹配，
     * 否则，如果从未进行过调用，则与TOPIC_QOS_DEFAULT匹配。
     *
     * @param [in] qos 返回default_topic_qos的TopicQos引用。
     *
     * @return RETCODE_OK
     */
    ReturnCode_t get_default_topic_qos(TopicQos& qos) const;

    /**
     * @brief 检索在域中发现且未被“忽略”的DomainParticits列表。
     *
     * @param [out] participant_handles 指向将返回已发现参与者的向量。
     *
     * @return 如果一切正常返回RETCODE_OK，否则返回错误代码。
     */
    ReturnCode_t get_discovered_participants(InstanceHandleSeq& participant_handles) const;

    /**
     * @brief 检索已发现的未被忽略的参与者的DomainParticipant数据。
     *
     * @param [out] participant_data   引用ParticipantBiltinTopicData对象以返回数据。
     * @param [in]  participant_handle DomainParticipant的InstanceHandle，用于从中检索数据。
     *
     * @return 如果一切正常，则返回RETCODE_OK，如果参与者不存在，则返回PRECONDITION_NOT_MET。
     */
    ReturnCode_t get_discovered_participant_data(ParticipantBuiltinTopicData& participant_data, const InstanceHandle_t& participant_handle) const;

    /**
     * @brief 检索在域中发现且未被“忽略”的主题列表。
     *
     * @param [out] topic_handles 指向返回已发现主题的向量。
     *
     * @return 如果一切正常RETCODE_OK，则返回错误代码。
     */
    ReturnCode_t get_discovered_topics(InstanceHandleSeq& topic_handles) const;

    /**
     * @brief 检索已发现的未忽略主题的主题数据。
     *
     * @param [out] topic_data   引用TopicBuiltinTopicData对象以返回数据。
     * @param [in]  topic_handle 用于从中检索数据的Topic的InstanceHandle。
     *
     * @return 如果一切正常，则返回RETCODE_OK，如果主题不存在，则返回PRECONDITION_NOT_MET
     */
    ReturnCode_t get_discovered_topic_data(TopicBuiltinTopicData& topic_data, const InstanceHandle_t& topic_handle) const;

    /**
     * @brief 获取已发现的DataWriter句柄列表。
     *
     * @param [out] publication_handles        已发现的DataWriter句柄列表。
     * @param [in]  include_contained_entities 是否包括包含的实体标识。
     *
     * @return RETCODE_OK。
     */
    ReturnCode_t get_publications(InstanceHandleSeq& publication_handles, bool include_contained_entities) const;

    /**
     * @brief 获取指定的DataWriter信息。
     *
     * @param [out] publication_data    指定的Datawriter信息。
     * @param [in]  publication_handle  指定的Datawriter句柄。
     *
     * @return 获取成功返回RETCODE_OK，否则返回RETCODE_BAD_PARAMETER。
     */
    ReturnCode_t get_publication_data(struct PublicationBuiltinTopicData& publication_data, const InstanceHandle_t& publication_handle) const;

    /**
     * @brief 获取已发现的DataReader句柄列表。
     *
     * @param [out] subscription_handles        已发现的DataReader句柄列表。
     * @param [in]  include_contained_entities  是否包括包含的实体标识。
     *
     * @return RETCODE_OK。
     */
    ReturnCode_t get_subscriptions(InstanceHandleSeq& subscription_handles, bool include_contained_entities) const;

    /**
     * @brief 获取指定的DataReader信息。
     *
     * @param [out] subscription_data   指定的DataReader信息。
     * @param [in]  subscription_handle 指定的DataReader句柄。
     *
     * @return 获取成功返回RETCODE_OK，否则返回RETCODE_BAD_PARAMETER。
     */
    ReturnCode_t get_subscription_data(struct SubscriptionBuiltinTopicData& subscription_data, const InstanceHandle_t& subscription_handle) const;

    /**
     * @brief 此函数检查给定的句柄是否表示从DomainParticipant创建的实体。
     *
     * @param [in] handle 要查找的实体的InstanceHandle。
     *
     * @return 如果包含实体，则为True。否则为False。
     */
    bool contains_entity(const InstanceHandle_t& handle) const;

#ifdef _XML_INTERFACE_CPP
    /**
     * @brief 指定QoS仓库的Qos配置，并创建发布者。
     *
     * @param [in] library_name QoS库的名字，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为NULL，转化为空字符串。
     * @param [in] qos_name     QoS名称，允许为NULL，转化为空字符串。
     * @param [in] listener     发布者监听器。
     * @param [in] mask         设置应用程序感兴趣的状态，只有应用程序感兴趣的状态发生变化时，
     *                          才会通知应用程序。
     *
     * @return 创建成功返回发布者指针，否则返回NULL。
     */
    virtual Publisher* create_publisher_with_qos_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name,
        PublisherListener* listener,
        StatusMask mask);

    /**
     * @brief 指定Qos仓库中的Qos设置为默认发布者Qos。
     *
     * @param [in] library_name QoS库的名字，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为NULL，转化为空字符串。
     * @param [in] qos_name     QoS名称，允许为NULL，转化为空字符串。
     *
     * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t set_default_publisher_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @brief   指定QoS仓库的Qos配置，并创建订阅者。
     *
     * @param [in] library_name QoS库的名字，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为NULL，转化为空字符串。
     * @param [in] qos_name     QoS名称，允许为NULL，转化为空字符串。
     * @param [in] listener     订阅者监听器。
     * @param [in] mask         设置应用程序感兴趣的状态，只有应用程序感兴趣的状态发生变化时，才会通知应用程序。
     *
     * @return 创建成功返回订阅者指针，否则返回NULL。
     */
    virtual Subscriber* create_subscriber_with_qos_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name,
        SubscriberListener* listener,
        StatusMask mask);

    /**
     * @brief 指定Qos仓库中的Qos设置为默认订阅Qos。
     *
     * @param [in] library_name QoS库的名字，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为NULL，转化为空字符串。
     * @param [in] qos_name     QoS名称，允许为NULL，转化为空字符串。
     *
     * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t set_default_subscriber_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @brief   指定QoS仓库的Qos配置，并创建主题。
     *
     * @param [in] topic_name   主题名称。
     * @param [in] type_name    类型名称。
     * @param [in] library_name QoS库的名字，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为NULL，转化为空字符串。
     * @param [in] qos_name     QoS名称，允许为NULL，转化为空字符串。
     * @param [in] listener     主题监听器。
     * @param [in] mask         设置应用程序感兴趣的状态，只有应用程序感兴趣的状态发生变化时，才会通知应用程序。
     *
     * @return 创建成功返回主题指针，否则返回NULL。
     */
    virtual Topic* create_topic_with_qos_profile(
        const std::string& topic_name,
        const std::string& type_name,
        const char* library_name,
        const char* profile_name,
        const char* qos_name,
        TopicListener* listener,
        StatusMask mask);

    /**
     * @brief 指定Qos仓库中的Qos设置为默认发布者Qos。
     *
     * @param [in] library_name QoS库的名字，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为NULL，转化为空字符串。
     * @param [in] qos_name     QoS名称，允许为NULL，转化为空字符串。
     *
     * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t set_default_topic_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @brief   设置QoS为QoS仓库中指定Qos配置。
     *
     * @param [in] library_name QoS库的名字，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为NULL，转化为空字符串。
     * @param [in] qos_name     QoS名称，允许为NULL，转化为空字符串。
     *
     * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t set_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

#endif // _XML_INTERFACE_CPP
public:
    /**
     * @brief 根据数据类型名从typeSupports_容器中获取对应的类型样本。
     *
     * @param [in] typeName 数据类型名。
     *
     * @return 数据类型对应的样本。
     */
	TypeSupport* find_type(const std::string& typeName);

    /**
     * @brief 注册当前数据类型的样本。
     *
     * @param [in] typeName    数据类型名。
     * @param [in] typeSupport 数据类型样本。
     *
     * @return 注册成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
	ReturnCode_t registe_type(const std::string& typeName, TypeSupport* typeSupport);

    /**
     * @brief 此函数用于启用实体
     *
     * @return 启用成功返回RETCODE_OK，
     *         不满足重新编码前提条件时返回RETCODE_PRECONDITION_NOT_MET，
     *         启用失败返回RETCODE_ERROR。
     */
	ReturnCode_t enable();

    /**
     * @brief 判断pubHandles_、subHandles_和topics_参数是否同时为空。
     *
     * @return 为空则返回true，否则返回false。
     */
    bool Empty() const;

    /**
     * @brief 获取相关参与者。
     *
     * @return 相关参与者指针。
     */
	std::shared_ptr<Participant> GetRelatedParticipant() const;

    /**
     * @brief 获取相关参与者。
     *
     * @return 相关参与者指针。
     */
	std::shared_ptr<Discovery> GetDiscovery() const;

    /**
     * @brief 根据GUID生成实例句柄。
     * 
     * @param [in] guid  参与者GUID。
     *
     * @return 参与者对应的实例句柄。
     */
	static InstanceHandle_t GenerateInstanceHandle(const GUID_t &guid);

    /**
     * @brief 获取参与者活跃管理类指针。
     *
     * @return 参与者活跃管理类指针。
     */
    std::shared_ptr<LivelinessManager> GetLivelinessManager();

#ifdef DDSSECURITY

    /**
     * @brief 安全检测。
     *
     * @return 检测失败则返回RETCODE_OK 失败返回RETCODE_ERROR。
     */
    ReturnCode_t security_check();

    std::shared_ptr<Discovery> GetDiscovery();

    DomainId_t GetDomainId();
#endif // DDSSECURITY


private:
    DomainId_t domainId_ = ~0;
    DomainParticipantQos qos_;


	mutable std::mutex mtxPubs_;
	mutable std::mutex mtxSubs_;
	mutable std::mutex mtxTopics_;

    std::condition_variable condTopics_;

	/* 已经注册了的数据类型 key值为数据类型名称 */
	std::map<std::string, TypeSupport*> typeSupports_;
    mutable std::mutex mtxTypes_;

	PublisherQos          defaultPubQos_;
	SubscriberQos         defaultSubQos_;
	TopicQos              defaultTopicQos_;
    
	uint32_t readerId_;
	uint32_t writerId_;
	uint32_t pubId_;
	uint32_t subId_;
	uint32_t topicId_;
private:
    DomainParticipantListener* listener_ = nullptr;

    std::shared_ptr<Participant> participant_;
    std::shared_ptr<Discovery> discovery_;
    /* 与participant_和discovery_共用一个线程池 */
    std::shared_ptr<ThreadPool> threadPool_;

    std::map<InstanceHandle_t, std::shared_ptr<TopicImpl>> topicHandles_;
    std::map<std::string, std::shared_ptr<MultiTopicImpl>> multiTopics_;
    std::map<std::string, std::shared_ptr<ContentFilteredTopicImpl>> contentFilteredTopics_;
    std::map<std::string, std::shared_ptr<TopicDescriptionImpl>> topics_;

    std::map<InstanceHandle_t, std::unique_ptr<SubscriberImpl>>subHandles_;
    std::map<InstanceHandle_t, std::unique_ptr<PublisherImpl>> pubHandles_;
public:
#ifdef DDSSECURITY

    /** @brief 指明该DomainParticipant是否启用了安全插件 */
    bool enableDDSSec_;
    /** @brief 安全插件实例 */
    SecurityPluginInstanceImpl secPlugin_;
    /** @brief 身份验证相关句柄 唯一标识域参与者认证状态 通过插件接口validate_local_identity生成*/
    IdentityHandle identityHdl_;
    /** @brief 访问控制相关句柄 */
    PermissionsHandle permissionHdl_ = NULL;
    /** @brief 加密相关句柄 */
    ParticipantCryptoHandle cryptoHdl_;
    /** @brief 身份验证令牌 包含身份摘要信息(初步身份标识)*/
    IdentityToken identityToken_;
    /** @brief 访问控制令牌 */
    PermissionsToken permissionsToken_;
    /** @brief 访问控制令牌 */
    PermissionsCredentialToken permissionCredentialToken_;
    /** @brief 安全相关属性 */
    ParticipantSecurityAttributes secAttributes_;
    typedef std::vector<ParticipantCryptoHandle> SecParticipantCryptoHandleList;
    /** @brief 该DomainParticipant发现的启用安全的其他DomainParticipant列表 */
    SecParticipantCryptoHandleList secParticipants_;
    /** @brief RTPS消息加密属性 */
    CryptoRtpsMessageAttributes rtpsCryptoAttr_;
    bool securityCheckRes_;

#endif // DDSSECURITY

};
TRAVODDS_NAMESPACE_END
#endif
