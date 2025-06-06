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
#ifndef AGIDDS_DCPS_DOMAIN_DOMAINPARTICIPANTIMPL_H
#define AGIDDS_DCPS_DOMAIN_DOMAINPARTICIPANTIMPL_H

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

AGIDDS_NAMESPACE_BEGIN

class Participant;
class Discovery;
class LivelinessManager;

class DomainParticipantImpl : public virtual DomainParticipant, public EntityImpl {
public:
    DomainParticipantImpl(DomainId_t domainId,
        const DomainParticipantQos& qos,
        DomainParticipantListener* listener,
        StatusMask mask);

    virtual ~DomainParticipantImpl();

    ReturnCode_t set_qos(const DomainParticipantQos& qos);

    ReturnCode_t get_qos(DomainParticipantQos& qos) const;

    ReturnCode_t set_listener(DomainParticipantListener* listener, const StatusMask& mask);

    const DomainParticipantListener* get_listener() const;

    Publisher* create_publisher(const PublisherQos& qos, PublisherListener* listener, const StatusMask& mask);

    ReturnCode_t delete_publisher(Publisher*& publisher);

    Subscriber* create_subscriber(const SubscriberQos& qos, SubscriberListener* listener, const StatusMask& mask);

    ReturnCode_t delete_subscriber(Subscriber*& subscriber);

    Topic* create_topic(const std::string& topic_name, const std::string& type_name, const TopicQos& qos, TopicListener* listener, const StatusMask& mask);

    ReturnCode_t delete_topic(Topic*& topic);

    ContentFilteredTopic* create_contentfilteredtopic(const std::string& name, Topic* related_topic, const std::string& filter_expression, const StringSeq& expression_parameters);

    ReturnCode_t delete_contentfilteredtopic(ContentFilteredTopic*& contentfilteredtopic);

    MultiTopic* create_multitopic(const std::string& name, const std::string& type_name, const std::string& subscription_expression, const StringSeq& expression_parameters);

    ReturnCode_t delete_multitopic(MultiTopic*& multitopic);

    Topic* find_topic(const std::string& topic_name, const Duration_t& timeout);

    TopicDescription* lookup_topicdescription(const std::string& name) const;

    const Subscriber* get_builtin_subscriber() const;

    ReturnCode_t ignore_participant(const InstanceHandle_t& handle);

    ReturnCode_t ignore_topic(const InstanceHandle_t& handle);

    ReturnCode_t ignore_publication(const InstanceHandle_t& handle);

    ReturnCode_t ignore_subscription(const InstanceHandle_t& handle);

	DomainId_t get_domain_id() const;

    ReturnCode_t delete_contained_entities();

    ReturnCode_t assert_liveliness();

    ReturnCode_t set_default_publisher_qos(const PublisherQos& qos);

    ReturnCode_t get_default_publisher_qos(PublisherQos& qos) const;

    ReturnCode_t set_default_subscriber_qos(const SubscriberQos& qos);

    ReturnCode_t get_default_subscriber_qos(SubscriberQos& qos) const;

    ReturnCode_t set_default_topic_qos(const TopicQos& qos);

    ReturnCode_t get_default_topic_qos(TopicQos& qos) const;

    ReturnCode_t get_discovered_participants(InstanceHandleSeq& participant_handles) const;

    ReturnCode_t get_discovered_participant_data(ParticipantBuiltinTopicData& participant_data, const InstanceHandle_t& participant_handle) const;

    ReturnCode_t get_discovered_topics(InstanceHandleSeq& topic_handles) const;

    ReturnCode_t get_discovered_topic_data(TopicBuiltinTopicData& topic_data, const InstanceHandle_t& topic_handle) const;

    /* 获取已发现的DataWriter列表 */
    ReturnCode_t get_publications(InstanceHandleSeq& publication_handles, bool include_contained_entities) const;

    /* 获取指定的DataWriter信息 */
    ReturnCode_t get_publication_data(struct PublicationBuiltinTopicData& publication_data, const InstanceHandle_t& publication_handle) const;

    /* 获取已发现的DataReader列表 */
    ReturnCode_t get_subscriptions(InstanceHandleSeq& subscription_handles, bool include_contained_entities) const;

    /* 获取指定的DataReader信息 */
    ReturnCode_t get_subscription_data(struct SubscriptionBuiltinTopicData& subscription_data, const InstanceHandle_t& subscription_handle) const;

    bool contains_entity(const InstanceHandle_t& handle) const;

#ifdef _XML_INTERFACE_CPP

    virtual Publisher* create_publisher_with_qos_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name,
        PublisherListener* listener,
        StatusMask mask);

    virtual ReturnCode_t set_default_publisher_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual Subscriber* create_subscriber_with_qos_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name,
        SubscriberListener* listener,
        StatusMask mask);

    virtual ReturnCode_t set_default_subscriber_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual Topic* create_topic_with_qos_profile(
        const std::string& topic_name,
        const std::string& type_name,
        const char* library_name,
        const char* profile_name,
        const char* qos_name,
        TopicListener* listener,
        StatusMask mask);

    virtual ReturnCode_t set_default_topic_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual ReturnCode_t set_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

#endif // _XML_INTERFACE_CPP
public:
	TypeSupport* find_type(const std::string& typeName);

	ReturnCode_t registe_type(const std::string& typeName, TypeSupport* typeSupport);

	ReturnCode_t enable();

    bool Empty() const;

	std::shared_ptr<Participant> GetRelatedParticipant() const;

	std::shared_ptr<Discovery> GetDiscovery() const;

	static InstanceHandle_t GenerateInstanceHandle(const GUID_t &guid);

    std::shared_ptr<LivelinessManager> GetLivelinessManager();

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

    std::map<InstanceHandle_t, std::shared_ptr<TopicImpl>> topicHandles_;
    std::map<std::string, std::shared_ptr<MultiTopicImpl>> multiTopics_;
    std::map<std::string, std::shared_ptr<ContentFilteredTopicImpl>> contentFilteredTopics_;
    std::map<std::string, std::shared_ptr<TopicDescriptionImpl>> topics_;

    std::map<InstanceHandle_t, std::unique_ptr<SubscriberImpl>>subHandles_;
    std::map<InstanceHandle_t, std::unique_ptr<PublisherImpl>> pubHandles_;
    std::shared_ptr<Participant> participant_;
    std::shared_ptr<Discovery> discovery_;
    /* 与participant_和discovery_共用一个线程池 */
    std::shared_ptr<ThreadPool> threadPool_;
};
AGIDDS_NAMESPACE_END
#endif
