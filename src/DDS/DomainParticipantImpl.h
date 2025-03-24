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
#pragma once
#include "DDS/DomainParticipant.h"
#include "DDS/EntityImpl.h"
#include "DDS/PublisherImpl.h"
#include "DDS/SubscriberImpl.h"
#include "DDS/TopicImpl.h"
#include "DDS/ContentFilteredTopicImpl.h"
#include "DDS/MultiTopicImpl.h"
#include "Listener/TopicListener.h"
#include <condition_variable>

AGIDDS_NAMESPACE_BEGIN
class DomainParticipantImpl : public virtual DomainParticipant, public EntityImpl {
public:
    DomainParticipantImpl(DomainId_t DomainId,
        const DomainParticipantQos& QoS,
        DomainParticipantListener* a_listener,
        StatusMask mask);

    ~DomainParticipantImpl();

    ReturnCode_t set_qos(const DomainParticipantQos& qos);

    ReturnCode_t get_qos(DomainParticipantQos& qos) const;

    ReturnCode_t set_listener(DomainParticipantListener* a_listener, const StatusMask& mask);

    const DomainParticipantListener* get_listener() const;

    Publisher* create_publisher(const PublisherQos& qos, PublisherListener* a_listener, const StatusMask& mask);

    ReturnCode_t delete_publisher(Publisher*& a_publisher);

    Subscriber* create_subscriber(const SubscriberQos& qos, SubscriberListener* a_listener, const StatusMask& mask);

    ReturnCode_t delete_subscriber(Subscriber*& a_subscriber);

    Topic* create_topic(const string& topic_name, const string& type_name, const TopicQos& qos, TopicListener* a_listener, const StatusMask& mask);

    ReturnCode_t delete_topic(Topic*& a_topic);

    ContentFilteredTopic* create_contentfilteredtopic(const string& name, Topic* related_topic, const string& filter_expression, const StringSeq& expression_parameters);

    ReturnCode_t delete_contentfilteredtopic(ContentFilteredTopic*& a_contentfilteredtopic);

    MultiTopic* create_multitopic(const string& name, const string& type_name, const string& subscription_expression, const StringSeq& expression_parameters);

    ReturnCode_t delete_multitopic(MultiTopic*& a_multitopic);

    Topic* find_topic(const string& topic_name, const Duration_t& timeout);

    TopicDescription* lookup_topicdescription(const string& name) const;

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

    ReturnCode_t get_discovered_participants(InstanceHandle_t& participant_handles) const;

    ReturnCode_t get_discovered_participant_data(ParticipantBuiltinTopicData& participant_data, const InstanceHandle_t& participant_handle) const;

    ReturnCode_t get_discovered_topics(InstanceHandle_t& topic_handles) const;

    ReturnCode_t get_discovered_topic_data(TopicBuiltinTopicData& topic_data, const InstanceHandle_t& topic_handle) const;

    bool contains_entity(const InstanceHandle_t& a_handle) const;

    //static ReturnCode_t get_current_time(Time_t& current_time);

public:
	TypeSupport* find_type(const string& typeName);

	ReturnCode_t registe_type(const string& typeName, TypeSupport* typeSupport);

	ReturnCode_t enable();

    bool Empty() const;

private:
    DomainId_t domainId_ = ~0;
    DomainParticipantQos qos_;
    DomainParticipantListener* listener_ = nullptr;

    std::map<InstanceHandle_t, std::unique_ptr<PublisherImpl>> pubHandles_;
    std::map<InstanceHandle_t, std::unique_ptr<SubscriberImpl>>subHandles_;
    std::map<InstanceHandle_t, std::shared_ptr<TopicImpl>> topicHandles_;
    std::map<std::string, std::shared_ptr<MultiTopicImpl>> multiTopics_;
    std::map<std::string, std::shared_ptr<ContentFilteredTopicImpl>> contentFilteredTopics_;
    std::map<std::string, std::shared_ptr<TopicDescriptionImpl>> topics_;

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
private:


};
AGIDDS_NAMESPACE_END
