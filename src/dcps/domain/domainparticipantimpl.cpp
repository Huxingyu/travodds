#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/domain/domainparticipantfactoryimpl.h"
#include "dcps/publish/publisherimpl.h"
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/topic/topicimpl.h"
#include "qos/qospolicycheck.h"
#include "rtps/endpoint/participant.h"
#include "rtps/discovery/discovery.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "common/log/logger.h"
AGIDDS_NAMESPACE_BEGIN;

DomainParticipantImpl::DomainParticipantImpl(DomainId_t domainId, const DomainParticipantQos& qos, DomainParticipantListener* listener, StatusMask mask)
    : EntityImpl(mask), domainId_(domainId), qos_(qos), listener_(listener), threadPool_(new ThreadPool(1))
{
	readerId_ = 0;
	writerId_ = 0;
	pubId_ = 0;
	subId_ = 0;
	topicId_ = 0;
	
	participant_ = std::make_shared<Participant>(domainId, threadPool_);
	assert(participant_.get());

	discovery_ = std::make_shared<Discovery>(this, threadPool_);
	assert(discovery_.get());

}

DomainParticipantImpl::~DomainParticipantImpl()
{
	threadPool_->shutdown();
	discovery_->RemoveSpdPdiscoveredParticipantData();
    participant_->StopReceiveThread();
    if (listener_) {
        delete listener_;
        listener_ = nullptr;
    }
}

ReturnCode_t DomainParticipantImpl::set_qos(const DomainParticipantQos& qos)
{
	DomainParticipantQos defaultQos;
	DomainParticipantFactoryImpl::get_instance()->get_default_participant_qos(defaultQos);
    const DomainParticipantQos& qosTemp = (&qos == &PARTICIPANT_QOS_DEFAULT) ? defaultQos : qos;
    
    if (&qos != &PARTICIPANT_QOS_DEFAULT) {
        ReturnCode_t ret = QosPolicyCheck::CheckQos(qosTemp);
        if (RETCODE_OK != ret) {
			AGILOG(AGILOG_ERROR, "CheckQos Err");
            return ret;
        }
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qosTemp)) {
		AGILOG(AGILOG_ERROR, "CanQosBeUpdated Err");
        return RETCODE_IMMUTABLE_POLICY;
    }
    qos_ = qosTemp;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_qos(DomainParticipantQos& qos) const
{
    qos = qos_;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::set_listener(DomainParticipantListener* listener, const StatusMask& mask)
{
    if (listener_) {
		delete listener_;
		listener_ = nullptr;
    }
	listener_ = listener;
	statusMask_ = mask;

    return RETCODE_OK;
}

const DomainParticipantListener* DomainParticipantImpl::get_listener() const
{
    return listener_;
}

Publisher* DomainParticipantImpl::create_publisher(const PublisherQos& qos, PublisherListener* listener, const StatusMask& mask)
{
	const PublisherQos& qosT = (&qos == &PUBLISHER_QOS_DEFAULT) ? defaultPubQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		AGILOG(AGILOG_ERROR, "CheckQos Err");
		return nullptr;
	}
	Publisher* pub = nullptr;
	std::unique_ptr<PublisherImpl> pubPtr = std::make_unique<PublisherImpl>(qosT, listener, mask, this);
	if (!pubPtr.get()) {
		AGILOG(AGILOG_ERROR, "publisher is nullptr");
		return nullptr;
	}
    pub = pubPtr.get();
    {
        std::lock_guard<std::mutex> lock(mtxPubs_);
        pubHandles_.insert({ pubPtr->get_instance_handle(), std::move(pubPtr) });
    }
    if (enable_ && qos_.entity_factory.autoenable_created_entities) {
        ReturnCode_t ret = pub->enable();
    }
	return pub;
}

ReturnCode_t DomainParticipantImpl::delete_publisher(Publisher*& publisher)
{
    if (!publisher) {
		AGILOG(AGILOG_ERROR, "publisher is nullptr");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != publisher->get_participant()) {
		AGILOG(AGILOG_ERROR, "domainparticipant donot contain the publisher");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    InstanceHandle_t handle = publisher->get_instance_handle();
    std::lock_guard<std::mutex> lock(mtxPubs_);
    auto it = pubHandles_.find(handle);

    if (it == pubHandles_.end()) {
		AGILOG(AGILOG_ERROR, "domainparticipant cant find the publisher");
        return RETCODE_ERROR;
    }
    if (!it->second->Empty()) {
		AGILOG(AGILOG_ERROR, "publisher not met");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    pubHandles_.erase(handle);
    publisher = nullptr;
    return RETCODE_OK;
}

Subscriber* DomainParticipantImpl::create_subscriber(const SubscriberQos& qos, SubscriberListener* listener, const StatusMask& mask)
{
	const SubscriberQos& qosT = (&qos == &SUBSCRIBER_QOS_DEFAULT) ? defaultSubQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		AGILOG(AGILOG_ERROR, "CheckQos Err");
		return nullptr;
	}
	Subscriber* sub = nullptr;
	std::unique_ptr<SubscriberImpl> subPtr = std::make_unique<SubscriberImpl>(qosT, listener, mask, this);
	if (!subPtr.get()) {
		AGILOG(AGILOG_ERROR, "subscriber is nullptr");
		return nullptr;
	}
    sub = subPtr.get();
    {
        std::lock_guard<std::mutex> lock(mtxSubs_);
        subHandles_.insert({ subPtr->get_instance_handle(), std::move(subPtr) });
    }
    if (enable_ && qos_.entity_factory.autoenable_created_entities) {
        ReturnCode_t ret = sub->enable();
    }
	return sub;
}

ReturnCode_t DomainParticipantImpl::delete_subscriber(Subscriber*& subscriber)
{
    if (!subscriber) {
		AGILOG(AGILOG_ERROR, "subscriber is nullptr");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != subscriber->get_participant()) {
		AGILOG(AGILOG_ERROR, "domainparticipant donot contain the subscriber");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    InstanceHandle_t handle = subscriber->get_instance_handle();
    std::lock_guard<std::mutex> lock(mtxSubs_);
    auto it = subHandles_.find(handle);

    if (it == subHandles_.end()) {
		AGILOG(AGILOG_ERROR, "domainparticipant cant find the subscriber");
        return RETCODE_ERROR;
    }
    if (!it->second->Empty()) {
		AGILOG(AGILOG_ERROR, "subscriber not met");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    subHandles_.erase(handle);
    subscriber = nullptr;
    return RETCODE_OK;
}

Topic* DomainParticipantImpl::create_topic(const std::string& topic_name, const std::string& type_name, const TopicQos& qos, TopicListener* listener, const StatusMask& mask)
{
    auto it = typeSupports_.find(type_name);
    if (it == typeSupports_.end()) {
		AGILOG(AGILOG_ERROR, "type_name[%s] not registed", type_name.c_str());
        return nullptr;
    }
	const TopicQos& qosT = (&qos == &TOPIC_QOS_DEFAULT) ? defaultTopicQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		AGILOG(AGILOG_ERROR, "CheckQos Err");
		return nullptr;
	}
    std::lock_guard<std::mutex> lock(mtxTopics_);
    if (topics_.find(topic_name) != topics_.end()) {
		AGILOG(AGILOG_ERROR, "topic[%s:%s] is created", topic_name.c_str(), type_name.c_str());
        return nullptr;
    }
	Topic* topic = nullptr;
	std::shared_ptr<TopicImpl> topicPtr = std::make_shared<TopicImpl>(topic_name, type_name, qosT, listener, mask, this);
	if (!topicPtr.get()) {
		AGILOG(AGILOG_ERROR, "topic[%s:%s] create failed", topic_name.c_str(), type_name.c_str());
		return nullptr;
	}
    topic = topicPtr.get();
    
    topicHandles_.insert({ topicPtr->get_instance_handle(), topicPtr });
    topics_.insert({ topic_name, topicPtr });

	if (enable_ && qos_.entity_factory.autoenable_created_entities) {
		ReturnCode_t ret = topic->enable();
	}
    condTopics_.notify_all();
	return topic;
}

ReturnCode_t DomainParticipantImpl::delete_topic(Topic*& topic)
{
    if (!topic) {
		AGILOG(AGILOG_ERROR, "topic is nullptr");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != topic->get_participant()) {
		AGILOG(AGILOG_ERROR, "domainparticipant donot contain the topic");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    InstanceHandle_t handle = topic->get_instance_handle();
    std::lock_guard<std::mutex> lock(mtxTopics_);
    auto it = topicHandles_.find(handle);
    if (it == topicHandles_.end()) {
		AGILOG(AGILOG_ERROR, "domainparticipant canot find the topic");
        return RETCODE_ERROR;
    }
    if (it->second->IsReferenced()) {
		AGILOG(AGILOG_ERROR, "topic is used");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    topicHandles_.erase(handle);
    topics_.erase(topic->get_name());
    topic = nullptr;
    return RETCODE_OK;
}

ContentFilteredTopic* DomainParticipantImpl::create_contentfilteredtopic(const std::string& name, Topic* related_topic, const std::string& filter_expression, const StringSeq& expression_parameters)
{
    ContentFilteredTopic* fit = nullptr;
    if (nullptr == related_topic || this != related_topic->get_participant()) {
		AGILOG(AGILOG_ERROR, "related_topic error");
        return fit;
    }
    std::lock_guard<std::mutex> lock(mtxTopics_);

    if (topics_.find(name) != topics_.end()) {
		AGILOG(AGILOG_ERROR, "topic[%s] is created", name.c_str());
        return nullptr;
    }
    std::shared_ptr<ContentFilteredTopicImpl> fitPtr = std::make_shared<ContentFilteredTopicImpl>(name, related_topic, filter_expression, expression_parameters, this);
    if (!fitPtr.get()) {
		AGILOG(AGILOG_ERROR, "topic[%s] create fail", name.c_str());
        return nullptr;
    }
    fit = fitPtr.get();
    contentFilteredTopics_.insert({ name, fitPtr });
    topics_.insert({ name, fitPtr });
    return fit;
}

ReturnCode_t DomainParticipantImpl::delete_contentfilteredtopic(ContentFilteredTopic*& contentfilteredtopic)
{
    if (!contentfilteredtopic) {
		AGILOG(AGILOG_ERROR, "contentfilteredtopics is nullptr");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != contentfilteredtopic->get_participant()) {
		AGILOG(AGILOG_ERROR, "domainparticipant donot contain the contentfilteredtopic");
        return RETCODE_PRECONDITION_NOT_MET;
    }

    std::string name = contentfilteredtopic->get_name();

    std::lock_guard<std::mutex> lock(mtxTopics_);
    auto it = contentFilteredTopics_.find(name);
    if (contentFilteredTopics_.end() == it) {
		AGILOG(AGILOG_ERROR, "domainparticipant cant find the contentfilteredtopic");
        return RETCODE_ERROR;
    }
    if (it->second->IsReferenced()) {
		AGILOG(AGILOG_ERROR, "the contentfilteredtopic is used");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    contentFilteredTopics_.erase(name);
    topics_.erase(name);
    contentfilteredtopic = nullptr;
    return RETCODE_OK;
}

MultiTopic* DomainParticipantImpl::create_multitopic(const std::string& name, const std::string& type_name, const std::string& subscription_expression, const StringSeq& expression_parameters)
{
    auto it = typeSupports_.find(type_name);
    if (it == typeSupports_.end()) {
		AGILOG(AGILOG_ERROR, "type_name[%s] not regist", type_name.c_str());
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(mtxTopics_);
    if (topics_.find(name) != topics_.end()) {
		AGILOG(AGILOG_ERROR, "topic[%s] is created", name.c_str());
        return nullptr;
    }
    MultiTopic* mult = nullptr;

    std::shared_ptr<MultiTopicImpl> mulTopic = std::make_shared<MultiTopicImpl>(name, type_name, subscription_expression, expression_parameters, this);
    if (!mulTopic.get()) {
		AGILOG(AGILOG_ERROR, "topic[%s] create failed", name.c_str());
        return nullptr;
    }
    mult = mulTopic.get();
    multiTopics_.insert({ name, mulTopic });
    topics_.insert({ name, mulTopic });
    return mult;
}

ReturnCode_t DomainParticipantImpl::delete_multitopic(MultiTopic*& multitopic)
{
    if (!multitopic) {
		AGILOG(AGILOG_ERROR, "multitopic is nullptr ");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != multitopic->get_participant()) {
		AGILOG(AGILOG_ERROR, "domainparticipant donot contain the multitopic");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    std::string name = multitopic->get_name();
    std::lock_guard<std::mutex> lock(mtxTopics_);
    auto it = multiTopics_.find(name);
    if (multiTopics_.end() == it) {
		AGILOG(AGILOG_ERROR, "domainparticipant cant find the multitopic");
        return RETCODE_ERROR;
    }
    if (it->second->IsReferenced()) {
		AGILOG(AGILOG_ERROR, "multitopic is used");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    multiTopics_.erase(name);
    topics_.erase(name);
    multitopic = nullptr;
    return RETCODE_OK;
}

Topic* DomainParticipantImpl::find_topic(const std::string& topic_name, const Duration_t& timeout)
{
    auto duration = std::chrono::seconds(timeout.sec) + std::chrono::nanoseconds(timeout.nanosec);

    std::unique_lock<std::mutex> lock(mtxTopics_);
    bool findRet = topics_.count(topic_name) > 0;
	if (findRet) {
		TopicImpl* impl = dynamic_cast<TopicImpl*>(topics_[topic_name].get());
		if (impl) {
			return impl;
		}
	}

    if (!condTopics_.wait_for(lock, duration, [this, &topic_name]() { return topics_.count(topic_name) > 0; })){
		AGILOG(AGILOG_ERROR, "topic_name[%s] find Err timeout", topic_name.c_str());
		return nullptr;
    }
	
    TopicImpl* impl = dynamic_cast<TopicImpl*>(topics_[topic_name].get());

    if (nullptr == impl) {
		AGILOG(AGILOG_ERROR, "topic_name[%s] find error", topic_name.c_str());
        return nullptr;
    }
    return impl;
}

TopicDescription* DomainParticipantImpl::lookup_topicdescription(const std::string& name) const
{
    TopicDescription* topic = nullptr;
    std::lock_guard<std::mutex> lock(mtxTopics_);
    auto it = topics_.find(name);
    if (it != topics_.end()) {
        topic = it->second.get();
    }
    return topic;
}

const Subscriber* DomainParticipantImpl::get_builtin_subscriber() const
{
	AGILOG(AGILOG_ERROR, "domainparticipant not support interface");
    return nullptr;
}

ReturnCode_t DomainParticipantImpl::ignore_participant(const InstanceHandle_t& handle)
{
	GUID_t guid = Conversion::ToGuid(handle);
	if (guid == participant_->GetGuid()) {
		/* TODO LOG */
		return RETCODE_ERROR;
	}
	discovery_->SetIgnoredParticipants(guid);
	discovery_->RemoveProxy(guid);
	return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::ignore_topic(const InstanceHandle_t& handle)
{
	AGILOG(AGILOG_ERROR, "domainparticipant not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DomainParticipantImpl::ignore_publication(const InstanceHandle_t& handle)
{
	AGILOG(AGILOG_ERROR, "domainparticipant not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DomainParticipantImpl::ignore_subscription(const InstanceHandle_t& handle)
{
	AGILOG(AGILOG_ERROR, "domainparticipant not support interface");
    return RETCODE_UNSUPPORTED;
}

DomainId_t DomainParticipantImpl::get_domain_id() const
{
    return domainId_;
}

ReturnCode_t DomainParticipantImpl::delete_contained_entities()
{
    {
        std::lock_guard<std::mutex> lock(mtxPubs_);
        for (auto& it : pubHandles_) {
            if (RETCODE_OK != it.second->delete_contained_entities()) {
				AGILOG(AGILOG_ERROR, "publishers delete_contained_entities error");
                return RETCODE_PRECONDITION_NOT_MET;
            }
        }
        pubHandles_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(mtxSubs_);
        for (auto& it : subHandles_) {
            if (RETCODE_OK != it.second->delete_contained_entities()) {
				AGILOG(AGILOG_ERROR, "subscribers delete_contained_entities error");
                return RETCODE_PRECONDITION_NOT_MET;
            }
        }
        subHandles_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(mtxTopics_);
        topics_.clear();
        contentFilteredTopics_.clear();
        multiTopics_.clear();
        topicHandles_.clear();
    }
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::assert_liveliness()
{
	if (participant_ == nullptr) {
		AGILOG(AGILOG_ERROR, "participant_ is nullptr");
		return RETCODE_NOT_ENABLED;
	}

	if (GetLivelinessManager() != nullptr) {
		GUID_t guid = Conversion::ToGuid(get_instance_handle());
		if (GetLivelinessManager()->AssertLiveliness(guid,true, MANUAL_BY_PARTICIPANT_LIVELINESS_QOS)) {
			return RETCODE_OK;
		}
	}
	else {
		AGILOG(AGILOG_ERROR, "Invalid WLP, cannot assert liveliness of participant");
	}
	return RETCODE_ERROR;
}

/* wangyi 获取livelinessManager */
std::shared_ptr<LivelinessManager> DomainParticipantImpl::GetLivelinessManager()
{
    return participant_->GetLivelinessManager();
}

ReturnCode_t DomainParticipantImpl::set_default_publisher_qos(const PublisherQos& qos)
{
    if (&qos == &PUBLISHER_QOS_DEFAULT) {
        /* yuguxu ?预留操作，默认的QoS具体是指配置文件中设置，还是默认宏所指 */
        defaultPubQos_ = PUBLISHER_QOS_DEFAULT;
        return RETCODE_OK;
    }
    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "ICheckQos Err ret = %d", ret);
        return ret;
    }
    defaultPubQos_ = qos;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_default_publisher_qos(PublisherQos& qos) const
{
    qos = defaultPubQos_;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::set_default_subscriber_qos(const SubscriberQos& qos)
{
    if (&qos == &SUBSCRIBER_QOS_DEFAULT) {
        /* yuguxu ?预留操作，默认的QoS具体是指配置文件中设置，还是默认宏所指 */
        defaultSubQos_ = SUBSCRIBER_QOS_DEFAULT;

        return RETCODE_OK;
    }
    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "CheckQos Err ret = %d", ret);
        return ret;
    }
    defaultSubQos_ = qos;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_default_subscriber_qos(SubscriberQos& qos) const
{
    qos = defaultSubQos_;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::set_default_topic_qos(const TopicQos& qos)
{
    if (&qos == &TOPIC_QOS_DEFAULT) {
        /* yuguxu ?预留操作，默认的QoS具体是指配置文件中设置，还是默认宏所指 */
        defaultTopicQos_ = TOPIC_QOS_DEFAULT;

        return RETCODE_OK;
    }
    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "CheckQos Err ret = %d", ret);
        return ret;
    }
    defaultTopicQos_ = qos;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_default_topic_qos(TopicQos& qos) const
{
    qos = defaultTopicQos_;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_discovered_participants(InstanceHandleSeq& participant_handles) const
{
	discovery_->GetDiscoveredParticipants(participant_handles);
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_discovered_participant_data(ParticipantBuiltinTopicData& participant_data, const InstanceHandle_t& participant_handle) const
{
	BuiltinTopicKey_t key = Conversion::ToBuiltinTopicKey(participant_handle);
	SPDPdiscoveredParticipantData data;

    ReturnCode_t ret =  discovery_->GetParticipantData(key, data);
	if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "GetParticipantData Err ret = %d", ret);
		return ret;
	}

	participant_data = data.ddsParticipantData;

	return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_discovered_topics(InstanceHandleSeq& topic_handles) const
{
	AGILOG(AGILOG_ERROR, "domainparticipant not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DomainParticipantImpl::get_discovered_topic_data(TopicBuiltinTopicData& topic_data, const InstanceHandle_t& topic_handle) const
{
	AGILOG(AGILOG_ERROR, "domainparticipant not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DomainParticipantImpl::get_publications(InstanceHandleSeq& publication_handles, bool include_contained_entities) const 
{
	return participant_->GetPublications(publication_handles, include_contained_entities);
}

ReturnCode_t DomainParticipantImpl::get_publication_data(PublicationBuiltinTopicData & publication_data, const InstanceHandle_t & publication_handle) const
{
	return participant_->GetPublicationData(publication_data, publication_handle);
}

ReturnCode_t DomainParticipantImpl::get_subscriptions(InstanceHandleSeq & subscription_handles, bool include_contained_entities) const
{
	return participant_->GetSublications(subscription_handles, include_contained_entities);
}

ReturnCode_t DomainParticipantImpl::get_subscription_data(SubscriptionBuiltinTopicData & subscription_data, const InstanceHandle_t & subscription_handle) const
{
	return participant_->GetSublicationData(subscription_data, subscription_handle);
}

bool DomainParticipantImpl::contains_entity(const InstanceHandle_t& handle) const
{
    {
        std::lock_guard<std::mutex> lock(mtxPubs_);
        if (pubHandles_.find(handle) != pubHandles_.end()) {
            return true;
        }
    }
    {
        std::lock_guard<std::mutex> lock(mtxSubs_);
        if (subHandles_.find(handle) != subHandles_.end()) {
            return true;
        }
    }
    {
        std::lock_guard<std::mutex> lock(mtxTopics_);
        if (topicHandles_.find(handle) != topicHandles_.end()) {
            return true;
        }
    }
    {
        std::lock_guard<std::mutex> lock(mtxPubs_);
        for (auto& it : pubHandles_) {
            if (it.second->contains_entity(handle)) {
                return true;
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(mtxSubs_);
        for (auto& it : subHandles_) {
            if (it.second->contains_entity(handle)) {
                return true;
            }
        }
    }
    return false;
}

ReturnCode_t DomainParticipant::get_current_time(Time_t& current_time)
{
    // 获取当前时间点,自纪元以来的时间间隔
    auto since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
    since_epoch -= sec;
    auto nanosec = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch);

    current_time.sec = static_cast<int32_t>(sec.count());
    current_time.nanosec = static_cast<uint32_t>(nanosec.count());
    return RETCODE_OK;
}


TypeSupport* DomainParticipantImpl::find_type(const std::string& typeName)
{
    std::lock_guard<std::mutex> lock(mtxTypes_);
    auto it = typeSupports_.find(typeName);
    if (it != typeSupports_.end()) {
        return it->second;
    }
	return nullptr;
}

ReturnCode_t DomainParticipantImpl::registe_type(const std::string& typeName, TypeSupport* typeSupport)
{
    if (typeName.empty()) {
		AGILOG(AGILOG_ERROR, "typeName is empty");
        return RETCODE_BAD_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(mtxTypes_);
    auto it = typeSupports_.find(typeName);
    if (it != typeSupports_.end()) {
        if (it->second == typeSupport) {
            return RETCODE_OK;
        }
		AGILOG(AGILOG_ERROR, "typeName[%s] is regist, but typesupport is different", typeName.c_str());
        return RETCODE_PRECONDITION_NOT_MET;
    }
    typeSupports_.insert({ typeName, typeSupport });
	return RETCODE_OK;
}
ReturnCode_t DomainParticipantImpl::enable()
{
	if (enable_) {
		return RETCODE_OK;
	}
	participant_->Init();

    handle_ = Conversion::ToInstanceHandle(participant_->GetGuid());

	discovery_->InitBuiltinEndpoint();

	discovery_->WriteSpdPdiscoveredParticipantData();

	ReturnCode_t ret = QosPolicyCheck::CheckQos(qos_);
	if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "CheckQos Err ret = %d ");
		return ret;
	}
	/* yuguxu 预留 DomainParticipant 资源申请
	 *
	 * 待实现
	 */
	if (qos_.entity_factory.autoenable_created_entities) {
		/* topic 优先enble */
		{
			std::lock_guard<std::mutex> lock(mtxTopics_);
			for (auto topic : topicHandles_) {
				topic.second->enable();
			}
		}
		{
			std::lock_guard<std::mutex> lock(mtxPubs_);
			for (auto& pub : pubHandles_) {
				pub.second->enable();
			}
		}
		{
			std::lock_guard<std::mutex> lock(mtxSubs_);
			for (auto& sub : subHandles_) {
				sub.second->enable();
			}
		}
	}
	enable_ = true;
	return RETCODE_OK;
}
bool DomainParticipantImpl::Empty() const
{
    return pubHandles_.empty() && subHandles_.empty() && topics_.empty();
}

std::shared_ptr<Participant> DomainParticipantImpl::GetRelatedParticipant() const
{
	return participant_;
}

std::shared_ptr<Discovery> DomainParticipantImpl::GetDiscovery() const
{
	return discovery_;
}


InstanceHandle_t DomainParticipantImpl::GenerateInstanceHandle(const GUID_t& guid)
{
	InstanceHandle_t handle;

	memcpy(handle.keyHash.value, &guid, sizeof(GUID_t));

	return handle;
}


AGIDDS_NAMESPACE_END
