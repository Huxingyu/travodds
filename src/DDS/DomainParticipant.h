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

#include "DDS/Publisher.h"
#include "DDS/Subscriber.h"
#include "DDS/ContentFilteredTopic.h"
#include "DDS/MultiTopic.h"
#include "Listener/DomainParticipantListener.h"

AGIDDS_NAMESPACE_BEGIN

class Publisher;
class TypeSupport;
class DDS_DLL DomainParticipant : public virtual Entity {
public:
    virtual ~DomainParticipant() = default;

	virtual ReturnCode_t set_qos(const DomainParticipantQos & qos) = 0;

	virtual ReturnCode_t get_qos(DomainParticipantQos & qos) const = 0;

	virtual ReturnCode_t set_listener(DomainParticipantListener* a_listener, const StatusMask& mask) = 0;

	virtual const DomainParticipantListener* get_listener() const = 0;

    virtual Publisher* create_publisher(const PublisherQos& qos, PublisherListener* a_listener, const StatusMask& mask) = 0;

    virtual ReturnCode_t delete_publisher(Publisher*& a_publisher) = 0;

    virtual Subscriber* create_subscriber(const SubscriberQos& qos, SubscriberListener* a_listener, const StatusMask& mask) = 0;

    virtual ReturnCode_t delete_subscriber(Subscriber*& a_subscriber) = 0;

    virtual Topic* create_topic(const string& topic_name, const string& type_name, const TopicQos& qos, TopicListener* a_listener, const StatusMask& mask) = 0;

    virtual ReturnCode_t delete_topic(Topic*& a_topic) = 0;

    virtual ContentFilteredTopic* create_contentfilteredtopic(const string& name, Topic* related_topic, const string& filter_expression, const StringSeq& expression_parameters) = 0;

    virtual ReturnCode_t delete_contentfilteredtopic(ContentFilteredTopic*& a_contentfilteredtopic) = 0;

    virtual MultiTopic* create_multitopic(const string& name, const string& type_name, const string& subscription_expression, const StringSeq& expression_parameters) = 0;

    virtual ReturnCode_t delete_multitopic(MultiTopic*& a_multitopic) = 0;

    virtual Topic* find_topic(const string& topic_name, const Duration_t& timeout) = 0;

    virtual TopicDescription* lookup_topicdescription(const string& name) const = 0;

    virtual const Subscriber* get_builtin_subscriber() const = 0;

    virtual ReturnCode_t ignore_participant(const InstanceHandle_t& handle) = 0;

    virtual ReturnCode_t ignore_topic(const InstanceHandle_t& handle) = 0;

    virtual ReturnCode_t ignore_publication(const InstanceHandle_t& handle) = 0;

    virtual ReturnCode_t ignore_subscription(const InstanceHandle_t& handle) = 0;

    virtual DomainId_t get_domain_id() const = 0;

    virtual ReturnCode_t delete_contained_entities() = 0;

    virtual ReturnCode_t assert_liveliness() = 0;

    virtual ReturnCode_t set_default_publisher_qos(const PublisherQos& qos) = 0;

    virtual ReturnCode_t get_default_publisher_qos(PublisherQos& qos) const = 0;

    virtual ReturnCode_t set_default_subscriber_qos(const SubscriberQos& qos) = 0;

    virtual ReturnCode_t get_default_subscriber_qos(SubscriberQos& qos) const = 0;

    virtual ReturnCode_t set_default_topic_qos(const TopicQos& qos) = 0;

    virtual ReturnCode_t get_default_topic_qos(TopicQos& qos) const = 0;

    virtual ReturnCode_t get_discovered_participants(InstanceHandle_t& participant_handles) const = 0;

    virtual ReturnCode_t get_discovered_participant_data(ParticipantBuiltinTopicData& participant_data, const InstanceHandle_t& participant_handle) const = 0;

    virtual ReturnCode_t get_discovered_topics(InstanceHandle_t& topic_handles) const = 0;

    virtual ReturnCode_t get_discovered_topic_data(TopicBuiltinTopicData& topic_data, const InstanceHandle_t& topic_handle) const = 0;

    virtual bool contains_entity(const InstanceHandle_t& a_handle) const = 0;

	static ReturnCode_t get_current_time(Time_t& current_time);

	virtual ReturnCode_t registe_type(const string& typeName, TypeSupport* typeSupport) = 0;
};

AGIDDS_NAMESPACE_END

