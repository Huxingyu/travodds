#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/domain/domainparticipantfactoryimpl.h"
#include "dcps/publish/publisherimpl.h"
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/topic/topicimpl.h"
#include "dcps/sql/ddssqlfilter.h"
#include "qos/qospolicycheck.h"
#include "rtps/endpoint/participant.h"
#include "rtps/discovery/discovery.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "common/log/logger.h"

TRAVODDS_NAMESPACE_BEGIN

#ifdef DDSSECURITY
DomainParticipantImpl::DomainParticipantImpl(DomainId_t domainId, 
    const DomainParticipantQos& qos, 
    DomainParticipantListener* listener, 
    StatusMask mask, 
    DDSSecurityPluginInstance* secPlugin)
    : EntityImpl(mask), domainId_(domainId), qos_(qos), listener_(listener), threadPool_(new ThreadPool(1))
{
    readerId_ = 0;
    writerId_ = 0;
    pubId_ = 0;
    subId_ = 0;
    topicId_ = 0;

    handle_ = Conversion::ToInstanceHandle(participant_->GetGuid());
    participant_ = std::make_shared<Participant>(domainId, threadPool_);
    participant_->participantImpl_ = this;
    assert(participant_.get());

    discovery_ = std::make_shared<Discovery>(this, threadPool_);
    assert(discovery_.get());

    if (secPlugin == NULL) {  // 如果不存在对应的安全插件
        enableDDSSec_ = false;
    }
    else if (!ExistSecConfig()) { // 存在对应的安全插件,但是安全qos相关配置缺失
        enableDDSSec_ = false;
    }
    else {
        enableDDSSec_ = true;
        secPlugin_.authentication = (Authentication*)secPlugin->authentication;
        secPlugin_.access_control = (AccessControl*)secPlugin->access_control;
        secPlugin_.crypto_key_factory = (CryptoKeyFactory*)secPlugin->crypto_key_factory;
        secPlugin_.crypto_key_exchange = (CryptoKeyExchange*)secPlugin->crypto_key_exchange;
        secPlugin_.crypto_transform = (CryptoTransform*)secPlugin->crypto_transform;
        secPlugin_.retry_time = secPlugin->retry_time;
        secPlugin_.max_wait_time = secPlugin->max_wait_time;
    }
}
#endif // DDSSECURITY

DomainParticipantImpl::DomainParticipantImpl(DomainId_t domainId, 
    const DomainParticipantQos& qos, 
    DomainParticipantListener* listener, 
    StatusMask mask)
    : EntityImpl(mask), domainId_(domainId), qos_(qos), listener_(listener), threadPool_(new ThreadPool(1))
{
	readerId_ = 0;
	writerId_ = 0;
	pubId_ = 0;
	subId_ = 0;
	topicId_ = 0;
	
	participant_ = std::make_shared<Participant>(domainId, threadPool_);
    handle_ = Conversion::ToInstanceHandle(participant_->GetGuid());
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
        //delete listener_;
        listener_ = nullptr;
    }
#ifdef DDSSECURITY
    if (enableDDSSec_)
    {
        SecurityException secExce;
        secPlugin_.crypto_key_factory->unregister_participant(cryptoHdl_, &secExce);
        secPlugin_.access_control->return_permissions_token(&permissionsToken_, &secExce);
        secPlugin_.access_control->return_permissions_credential_token(&permissionCredentialToken_, &secExce);
        secPlugin_.access_control->return_permissions_handle(permissionHdl_, &secExce);
        secPlugin_.authentication->return_identity_token(&identityToken_, &secExce);
        secPlugin_.authentication->return_identity_handle(identityHdl_, &secExce);
    }
#endif // DDSSECURITY
}

ReturnCode_t DomainParticipantImpl::set_qos(const DomainParticipantQos& qos)
{
	DomainParticipantQos defaultQos;
	DomainParticipantFactoryImpl::get_instance()->get_default_participant_qos(defaultQos);
    const DomainParticipantQos& qosTemp = (&qos == &PARTICIPANT_QOS_DEFAULT) ? defaultQos : qos;
    
    if (&qos != &PARTICIPANT_QOS_DEFAULT) {
        ReturnCode_t ret = QosPolicyCheck::CheckQos(qosTemp);
        if (RETCODE_OK != ret) {
			TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
            return ret;
        }
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qosTemp)) {
		TRAVODDS_LOG(LOG_ERROR, "CanQosBeUpdated Err");
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
		//delete listener_;
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
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
		return nullptr;
	}
	Publisher* pub = nullptr;
	std::unique_ptr<PublisherImpl> pubPtr = std::make_unique<PublisherImpl>(qosT, listener, mask, this);
	if (!pubPtr.get()) {
		TRAVODDS_LOG(LOG_ERROR, "publisher is nullptr");
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
		TRAVODDS_LOG(LOG_ERROR, "publisher is nullptr");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != publisher->get_participant()) {
		TRAVODDS_LOG(LOG_ERROR, "domainparticipant donot contain the publisher");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    InstanceHandle_t handle = publisher->get_instance_handle();
    std::lock_guard<std::mutex> lock(mtxPubs_);
    auto it = pubHandles_.find(handle);

    if (it == pubHandles_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "domainparticipant cant find the publisher");
        return RETCODE_ERROR;
    }
    if (!it->second->Empty()) {
		TRAVODDS_LOG(LOG_ERROR, "publisher not met");
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
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
		return nullptr;
	}
	Subscriber* sub = nullptr;
	std::unique_ptr<SubscriberImpl> subPtr = std::make_unique<SubscriberImpl>(qosT, listener, mask, this);
	if (!subPtr.get()) {
		TRAVODDS_LOG(LOG_ERROR, "subscriber is nullptr");
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
		TRAVODDS_LOG(LOG_ERROR, "subscriber is nullptr");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != subscriber->get_participant()) {
		TRAVODDS_LOG(LOG_ERROR, "domainparticipant donot contain the subscriber");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    InstanceHandle_t handle = subscriber->get_instance_handle();
    std::lock_guard<std::mutex> lock(mtxSubs_);
    auto it = subHandles_.find(handle);

    if (it == subHandles_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "domainparticipant cant find the subscriber");
        return RETCODE_ERROR;
    }
    if (!it->second->Empty()) {
		TRAVODDS_LOG(LOG_ERROR, "subscriber not met");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    subHandles_.erase(handle);
    subscriber = nullptr;
    return RETCODE_OK;
}

Topic* DomainParticipantImpl::create_topic(const std::string& topic_name, const std::string& type_name, const TopicQos& qos, TopicListener* listener, const StatusMask& mask)
{
    if (topic_name.empty() || type_name.empty()) {
        TRAVODDS_LOG(LOG_ERROR, "topic_name or type_name is empty");
		return nullptr;
    }
    if (topic_name.size() > BYTE_ORDER_LENGTH_MAX || type_name.size() > BYTE_ORDER_LENGTH_MAX) {
        TRAVODDS_LOG(LOG_ERROR, "topic_name or type_name length is too long, max length is %d", BYTE_ORDER_LENGTH_MAX);
		return nullptr;
    }
    auto it = typeSupports_.find(type_name);
    if (it == typeSupports_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "type_name[%s] not registed", type_name.c_str());
        return nullptr;
    }
	const TopicQos& qosT = (&qos == &TOPIC_QOS_DEFAULT) ? defaultTopicQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
		return nullptr;
	}
    std::lock_guard<std::mutex> lock(mtxTopics_);
    if (topics_.find(topic_name) != topics_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "topic[%s:%s] is created", topic_name.c_str(), type_name.c_str());
        return nullptr;
    }
	Topic* topic = nullptr;

#ifdef DDSSECURITY
    if (enableDDSSec_) {
        SecurityException secExce;
        if (!secPlugin_.access_control->check_create_topic(
            permissionHdl_,
            domainId_,
            topic_name.c_str(),
            &qosT,
            &secExce))
        {
            return topic;
        }
    }
#endif // DDSSECURITY

	std::shared_ptr<TopicImpl> topicPtr = std::make_shared<TopicImpl>(topic_name, type_name, qosT, listener, mask, this);
	if (!topicPtr.get()) {
		TRAVODDS_LOG(LOG_ERROR, "topic[%s:%s] create failed", topic_name.c_str(), type_name.c_str());
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
		TRAVODDS_LOG(LOG_ERROR, "topic is nullptr");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != topic->get_participant()) {
		TRAVODDS_LOG(LOG_ERROR, "domainparticipant donot contain the topic");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    InstanceHandle_t handle = topic->get_instance_handle();
    std::lock_guard<std::mutex> lock(mtxTopics_);
    auto it = topicHandles_.find(handle);
    if (it == topicHandles_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "domainparticipant canot find the topic");
        return RETCODE_ERROR;
    }
    if (it->second->IsReferenced()) {
		TRAVODDS_LOG(LOG_ERROR, "topic is used");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    topicHandles_.erase(handle);
    topics_.erase(topic->get_name());
    topic = nullptr;
    return RETCODE_OK;
}

ContentFilteredTopic* DomainParticipantImpl::create_contentfilteredtopic(const std::string& name, Topic* related_topic, const std::string& filter_expression, const StringSeq& expression_parameters)
{
    if (name.empty() || name.size() > BYTE_ORDER_LENGTH_MAX) {
        TRAVODDS_LOG(LOG_ERROR, "name is empty or length is too long, max length is %d", BYTE_ORDER_LENGTH_MAX);
		return nullptr;
    }
    if (expression_parameters.size() > EXPRESSION_PARAMETERS_MAX) {
        TRAVODDS_LOG(LOG_ERROR, "expression_parameters size is over max, max size is %d", EXPRESSION_PARAMETERS_MAX);
        return nullptr;
	}

    if (nullptr == related_topic || this != related_topic->get_participant()) {
		TRAVODDS_LOG(LOG_ERROR, "related_topic error");
        return nullptr;
    }
    std::string typeName = related_topic->get_type_name();

    if (!ContentFilteredTopicImpl::ValidFilterExpression(filter_expression, expression_parameters, typeSupports_.find(typeName)->second->get_dynamictype())) {
        return nullptr;
    }
    ContentFilteredTopic* fit = nullptr;
    std::lock_guard<std::mutex> lock(mtxTopics_);

    if (topics_.find(name) != topics_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "topic[%s] is created", name.c_str());
        return nullptr;
    }
    std::shared_ptr<ContentFilteredTopicImpl> fitPtr = std::make_shared<ContentFilteredTopicImpl>(name, related_topic, filter_expression, expression_parameters, this);
    if (!fitPtr.get()) {
		TRAVODDS_LOG(LOG_ERROR, "topic[%s] create fail", name.c_str());
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
		TRAVODDS_LOG(LOG_ERROR, "contentfilteredtopics is nullptr");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != contentfilteredtopic->get_participant()) {
		TRAVODDS_LOG(LOG_ERROR, "domainparticipant donot contain the contentfilteredtopic");
        return RETCODE_PRECONDITION_NOT_MET;
    }

    std::string name = contentfilteredtopic->get_name();

    std::lock_guard<std::mutex> lock(mtxTopics_);
    auto it = contentFilteredTopics_.find(name);
    if (contentFilteredTopics_.end() == it) {
		TRAVODDS_LOG(LOG_ERROR, "domainparticipant cant find the contentfilteredtopic");
        return RETCODE_ERROR;
    }
    if (it->second->IsReferenced()) {
		TRAVODDS_LOG(LOG_ERROR, "the contentfilteredtopic is used");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    contentFilteredTopics_.erase(name);
    topics_.erase(name);
    contentfilteredtopic = nullptr;
    return RETCODE_OK;
}

MultiTopic* DomainParticipantImpl::create_multitopic(const std::string& name, const std::string& type_name, const std::string& subscription_expression, const StringSeq& expression_parameters)
{
    if (name.empty() || type_name.empty()) {
        TRAVODDS_LOG(LOG_ERROR, "topic_name or type_name is empty");
        return nullptr;
    }
    if (name.size() > BYTE_ORDER_LENGTH_MAX || type_name.size() > BYTE_ORDER_LENGTH_MAX) {
        TRAVODDS_LOG(LOG_ERROR, "topic_name or type_name length is too long, max length is %d", BYTE_ORDER_LENGTH_MAX);
        return nullptr;
    }
    auto it = typeSupports_.find(type_name);
    if (it == typeSupports_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "type_name[%s] not regist", type_name.c_str());
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(mtxTopics_);
    if (topics_.find(name) != topics_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "topic[%s] is created", name.c_str());
        return nullptr;
    }
    MultiTopic* mult = nullptr;

    std::shared_ptr<MultiTopicImpl> mulTopic = std::make_shared<MultiTopicImpl>(name, type_name, subscription_expression, expression_parameters, this);
    if (!mulTopic.get()) {
		TRAVODDS_LOG(LOG_ERROR, "topic[%s] create failed", name.c_str());
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
		TRAVODDS_LOG(LOG_ERROR, "multitopic is nullptr ");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != multitopic->get_participant()) {
		TRAVODDS_LOG(LOG_ERROR, "domainparticipant donot contain the multitopic");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    std::string name = multitopic->get_name();
    std::lock_guard<std::mutex> lock(mtxTopics_);
    auto it = multiTopics_.find(name);
    if (multiTopics_.end() == it) {
		TRAVODDS_LOG(LOG_ERROR, "domainparticipant cant find the multitopic");
        return RETCODE_ERROR;
    }
    if (it->second->IsReferenced()) {
		TRAVODDS_LOG(LOG_ERROR, "multitopic is used");
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
		TRAVODDS_LOG(LOG_ERROR, "topic_name[%s] find Err timeout", topic_name.c_str());
		return nullptr;
    }
	
    TopicImpl* impl = dynamic_cast<TopicImpl*>(topics_[topic_name].get());

    if (nullptr == impl) {
		TRAVODDS_LOG(LOG_ERROR, "topic_name[%s] find error", topic_name.c_str());
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
	TRAVODDS_LOG(LOG_ERROR, "domainparticipant not support interface");
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
	TRAVODDS_LOG(LOG_ERROR, "domainparticipant not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DomainParticipantImpl::ignore_publication(const InstanceHandle_t& handle)
{
	TRAVODDS_LOG(LOG_ERROR, "domainparticipant not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DomainParticipantImpl::ignore_subscription(const InstanceHandle_t& handle)
{
	TRAVODDS_LOG(LOG_ERROR, "domainparticipant not support interface");
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
				TRAVODDS_LOG(LOG_ERROR, "publishers delete_contained_entities error");
                return RETCODE_PRECONDITION_NOT_MET;
            }
        }
        pubHandles_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(mtxSubs_);
        for (auto& it : subHandles_) {
            if (RETCODE_OK != it.second->delete_contained_entities()) {
				TRAVODDS_LOG(LOG_ERROR, "subscribers delete_contained_entities error");
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
		TRAVODDS_LOG(LOG_ERROR, "participant_ is nullptr");
		return RETCODE_NOT_ENABLED;
	}

	if (GetLivelinessManager() != nullptr) {
		GUID_t guid = Conversion::ToGuid(get_instance_handle());
		if (GetLivelinessManager()->AssertLiveliness(guid,true, MANUAL_BY_PARTICIPANT_LIVELINESS_QOS)) {
			return RETCODE_OK;
		}
	}
	else {
		TRAVODDS_LOG(LOG_ERROR, "Invalid WLP, cannot assert liveliness of participant");
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
		TRAVODDS_LOG(LOG_ERROR, "ICheckQos Err ret = %d", ret);
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
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err ret = %d", ret);
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
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err ret = %d", ret);
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
		TRAVODDS_LOG(LOG_ERROR, "GetParticipantData Err ret = %d", ret);
		return ret;
	}

	participant_data = data.ddsParticipantData;

	return RETCODE_OK;
}

ReturnCode_t DomainParticipantImpl::get_discovered_topics(InstanceHandleSeq& topic_handles) const
{
	TRAVODDS_LOG(LOG_ERROR, "domainparticipant not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DomainParticipantImpl::get_discovered_topic_data(TopicBuiltinTopicData& topic_data, const InstanceHandle_t& topic_handle) const
{
	TRAVODDS_LOG(LOG_ERROR, "domainparticipant not support interface");
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
    if (typeName.empty() || typeName.size() > BYTE_ORDER_LENGTH_MAX) {
		TRAVODDS_LOG(LOG_ERROR, "typeName is empty or length is too long, max length is %d", BYTE_ORDER_LENGTH_MAX);
        return RETCODE_BAD_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(mtxTypes_);
    auto it = typeSupports_.find(typeName);
    if (it != typeSupports_.end()) {
        if (it->second == typeSupport) {
            return RETCODE_OK;
        }
		TRAVODDS_LOG(LOG_ERROR, "typeName[%s] is regist, but typesupport is different", typeName.c_str());
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
    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos_);
    if (RETCODE_OK != ret) {
        TRAVODDS_LOG(LOG_ERROR, "CheckQos Err ret = %d ");
        return ret;
    }

	participant_->Init(qos_);

    handle_ = Conversion::ToInstanceHandle(participant_->GetGuid());

#ifdef DDSSECURITY
    ReturnCode_t secRet = security_check();
    securityCheckRes_ = (secRet == RETCODE_OK) ? true : false;
#endif

	discovery_->InitBuiltinEndpoint();

	participant_->InitNetwork();

	discovery_->WriteSpdPdiscoveredParticipantData();


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
#ifdef DDSSECURITY
ReturnCode_t DomainParticipantImpl::security_check()
{
    if (!enableDDSSec_) {
        TRAVODDS_LOG(LOG_DEBUG, "need not security_check");
        return VALIDATION_OK;
    }
    else {
        // validate_local_identity
        SecurityException exceptionInfo;
        BuiltinTopicKey_t originKey; //4个32位数 16字节
        memcpy(originKey.value, handle_.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
        handle_.keyHash.value[0];
        BuiltinTopicKey_t adjustedKey;
        if (secPlugin_.authentication == nullptr) {
            TRAVODDS_LOG(LOG_ERROR, "validate_local_identity failed because authentication is null!\n");
            return VALIDATION_FAILED;
        }
        ValidationResult_t validataRst = secPlugin_.authentication->validate_local_identity(
            &identityHdl_,
            adjustedKey,
            domainId_,
            &qos_,
            originKey,
            &exceptionInfo);
        while (validataRst == VALIDATION_PENDING_RETRY)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            validataRst = secPlugin_.authentication->validate_local_identity(
                &identityHdl_,
                adjustedKey,
                domainId_,
                &qos_,
                originKey,
                &exceptionInfo);
        }
        if (validataRst == VALIDATION_FAILED)
        {
            TRAVODDS_LOG(LOG_ERROR, "DomainParticipant authenticated failed."
                "\n  Code: %d"
                "\n  Minor code: %d"
                "\n  Message: %s",
                exceptionInfo.code,
                exceptionInfo.minor_code,
                exceptionInfo.message);
            return RETCODE_ERROR;
        }
        // 将哈希后的域参与者唯一标识adjusted_participant_key赋值给实体
        memcpy(handle_.keyHash.value, adjustedKey.value, 16);
        GUID_t* guid = participant_->GetGuidPtr();
        memcpy(&guid->prefix, handle_.keyHash.value, 12);
        // validate_local_permissions
        permissionHdl_ = secPlugin_.access_control->validate_local_permissions(
            secPlugin_.authentication,
            identityHdl_,
            domainId_,
            &qos_,
            &exceptionInfo);
        if (permissionHdl_ == SEC_NIL_HANDLE) {
            TRAVODDS_LOG(LOG_ERROR, "DomainParticipant validate permission failed."
                "\n  Code: %d"
                "\n  Minor code: %d"
                "\n  Message: %s",
                exceptionInfo.code,
                exceptionInfo.minor_code,
                exceptionInfo.message);
            return RETCODE_ERROR;
        }
        //check_create_participant 
        if (!secPlugin_.access_control->check_create_participant(
            permissionHdl_, domainId_, &qos_, &exceptionInfo))
        {
            TRAVODDS_LOG(LOG_ERROR, "DomainParticipant check create Participant failed."
                "\n  Code: %d"
                "\n  Minor code: %d"
                "\n  Message: %s",
                exceptionInfo.code,
                exceptionInfo.minor_code,
                exceptionInfo.message);
            return RETCODE_ERROR;
        }

        // get_identity_token 生成identityToken
        if (!secPlugin_.authentication->get_identity_token(
            &identityToken_, identityHdl_, &exceptionInfo))
        {
            TRAVODDS_LOG(LOG_ERROR, "DomainParticipant get identity token failed."
                "\n  Code: %d"
                "\n  Minor code: %d"
                "\n  Message: %s",
                exceptionInfo.code,
                exceptionInfo.minor_code,
                exceptionInfo.message);
            return RETCODE_ERROR;
        }
        // get_permissions_token
        if (!secPlugin_.access_control->get_permissions_token(
            &permissionsToken_, permissionHdl_, &exceptionInfo))
        {
            TRAVODDS_LOG(LOG_ERROR, "DomainParticipant get permission token failed."
                "\n  Code: %d"
                "\n  Minor code: %d"
                "\n  Message: %s",
                exceptionInfo.code,
                exceptionInfo.minor_code,
                exceptionInfo.message);
            return RETCODE_ERROR;
        }
        // get_permissions_credential_token
        if (!secPlugin_.access_control->get_permissions_credential_token(
            &permissionCredentialToken_, permissionHdl_, &exceptionInfo))
        {
            TRAVODDS_LOG(LOG_ERROR, "DomainParticipant get permission credential token failed."
                "\n  Code: %d"
                "\n  Minor code: %d"
                "\n  Message: %s",
                exceptionInfo.code,
                exceptionInfo.minor_code,
                exceptionInfo.message);
            return RETCODE_ERROR;
        }
        // set_permissions_credential_and_token
        if (!secPlugin_.authentication->set_permissions_credential_and_token(
            identityHdl_,
            &permissionCredentialToken_,
            &permissionsToken_,
            &exceptionInfo))
        {
            TRAVODDS_LOG(LOG_ERROR, "DomainParticipant set permission credential and token failed."
                "\n  Code: %d"
                "\n  Minor code: %d"
                "\n  Message: %s",
                exceptionInfo.code,
                exceptionInfo.minor_code,
                exceptionInfo.message);
            return RETCODE_ERROR;
        }
        // get_participant_sec_attributes 获取域参与者的安全属性
        ParticipantSecurityAttributes secAttr;
        if (!secPlugin_.access_control->get_participant_sec_attributes(
            permissionHdl_, &secAttr, &exceptionInfo))
        {
            TRAVODDS_LOG(LOG_ERROR, "DomainParticipant get security attributes failed."
                "\n  Code: %d"
                "\n  Minor code: %d"
                "\n  Message: %s",
                exceptionInfo.code,
                exceptionInfo.minor_code,
                exceptionInfo.message);
            return RETCODE_ERROR;
        }
        secAttributes_.ac_participant_properties = secAttr.ac_participant_properties;//Sequence<Property_t>
        secAttributes_.is_access_protected = secAttr.is_access_protected;
        secAttributes_.allow_unauthenticated_participants = secAttr.allow_unauthenticated_participants;
        secAttributes_.is_rtps_protected = secAttr.is_rtps_protected;
        secAttributes_.plugin_participant_attributes = secAttr.plugin_participant_attributes;
		// 目前下面几个变量未使用
        // secAttributes_.is_discovery_protected = secAttr.is_discovery_protected;
        // secAttributes_.is_liveliness_protected = secAttr.is_liveliness_protected;
        if (!secPlugin_.access_control->return_participant_sec_attributes(&secAttr, &exceptionInfo))
        {
            TRAVODDS_LOG(LOG_ERROR, "DomainParticipant return security attributes failed."
                "\n  Code: %d"
                "\n  Minor code: %d"
                "\n  Message: %s",
                exceptionInfo.code,
                exceptionInfo.minor_code,
                exceptionInfo.message);
            return RETCODE_ERROR;
        }

        if (secPlugin_.crypto_key_factory == NULL
            || secPlugin_.crypto_key_exchange == NULL
            || secPlugin_.crypto_transform == NULL)
        {
            TRAVODDS_LOG(LOG_ERROR, "No cryptographic plugin in DomainParticipant.");
            return RETCODE_ERROR;
        }

        cryptoHdl_ = secPlugin_.crypto_key_factory->register_local_participant(
            identityHdl_,
            permissionHdl_,
            &secAttributes_.ac_participant_properties,
            &exceptionInfo);
        if (cryptoHdl_ == SEC_NIL_HANDLE)
        {
            TRAVODDS_LOG(LOG_ERROR, "DomainParticipant get cryptographic handle failed."
                "\n  Code: %d"
                "\n  Minor code: %d"
                "\n  Message: %s",
                exceptionInfo.code,
                exceptionInfo.minor_code,
                exceptionInfo.message);
            return RETCODE_ERROR;
        }
        // 创建域参与者时 get_rtps_message_encode_attributes
        if (secAttributes_.is_rtps_protected)
        {
            if (!secPlugin_.crypto_transform->get_rtps_message_encode_attributes(cryptoHdl_, &rtpsCryptoAttr_))
            {
                TRAVODDS_LOG(LOG_ERROR, "DomainParticipant get cryptographic attributes failed.");
                return RETCODE_ERROR;
            }
        }
    }
    return RETCODE_OK;
}


std::shared_ptr<Discovery> DomainParticipantImpl::GetDiscovery()
{
    return discovery_;
}

DomainId_t DomainParticipantImpl::GetDomainId()
{
    return domainId_;
}

bool DomainParticipantImpl::ExistSecConfig()
{
    Property_t* prikey = NULL;  //私钥
    Property_t* prikey_password = NULL; //私钥
    Property_t* permission = NULL; //auth_permission.smime
    Property_t* governance = NULL; //auth_governance.smime
    Property_t* certificate = NULL; //用户证书
    Property_t* ca = NULL; //CA证书
    Property_t* permission_ca = NULL; //访问控制配置，例如democa.pem文件内容
    Property_t* crl = NULL;
    int ret = 0;
    //判定必须需要的配置项
    for (unsigned int i = 0; i < qos_.property.value.size(); i++)
    {
        Property_t* curProperty = &(qos_.property.value[i]);
        //用户证书
        if (curProperty->name == "dds.sec.auth.identity_certificate")
        {
            certificate = curProperty;
            continue;
        }
        //CA证书
        if (curProperty->name == "dds.sec.auth.identity_ca")
        {
            ca = curProperty;
            continue;
        }
        //私钥
        if (curProperty->name == "dds.sec.auth.private_key")
        {
            prikey = curProperty;
            continue;
        }
        //私钥
        if (curProperty->name == "dds.sec.auth.password")
        {
            prikey_password = curProperty;
            continue;
        }
        //证书注销列表
        if (curProperty->name == "dds.sec.auth.certificate_revocation_list")
        {
            crl = curProperty;
            continue;
        }
        //访问控制配置
        if (curProperty->name == "dds.sec.access.permissions_ca")
        {
            permission_ca = curProperty;
            continue;
        }
        //访问控制配置
        if (curProperty->name == "dds.sec.access.permissions")
        {
            permission = curProperty;
            continue;
        }
        //访问控制配置
        if (curProperty->name == "dds.sec.access.governance")
        {
            governance = curProperty;
            continue;
        }
    }
    //必须需要的配置项 私钥、CA证书、访问控制配置签名、授权配置签名
    if (certificate == NULL)
    {
        TRAVODDS_LOG(LOG_DEBUG, "search certificate property failed\n");
        return false;
    }
    if (ca == NULL)
    {
        TRAVODDS_LOG(LOG_DEBUG, "search ca property failed\n");
        return false;
    }
    if (prikey == NULL)
    {
        TRAVODDS_LOG(LOG_DEBUG, "search private_key property failed\n");
        return false;
    }
    if (permission == NULL)
    {
        TRAVODDS_LOG(LOG_DEBUG, "search permissions property failed\n");
        return false;
    }
    if (governance == NULL)
    {
        TRAVODDS_LOG(LOG_DEBUG, "search governance property failed\n");
        return false;
    }
    if (permission_ca == NULL)
    {
        TRAVODDS_LOG(LOG_DEBUG, "search permission ca property failed\n");
        return false;
    }
    return true;
}
#endif // DDSSECURITY
TRAVODDS_NAMESPACE_END
