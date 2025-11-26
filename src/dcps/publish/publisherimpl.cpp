#include "dcps/publish/publisherimpl.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/writer.h"
#include "qos/qospolicycheck.h"
#include "dcps/topic/typesupport.h"
#include "rtps/discovery/discovery.h"
#include "common/log/logger.h"

USING_TRAVODDS_NAMESPACE;
PublisherImpl::PublisherImpl(PublisherQos qos, PublisherListener* listener, StatusMask mask, DomainParticipantImpl* participant)
    : DomainEntityImpl(mask), qos_(qos), listener_(listener), participant_(participant)
{
    GUID_t guid =  participant_->GetRelatedParticipant()->GenerateGuid(ENTITYID_UNKNOWN,false, ENTITY_PUB);
    handle_ = Conversion::ToInstanceHandle(guid);
}

PublisherImpl::~PublisherImpl()
{
    for (auto& it : writerHandles_) {
        participant_->GetRelatedParticipant()->DeleteWriter(it.second->get_instance_handle());
    }
    writerHandles_.clear();
	if (listener_) {
		//delete listener_;
		listener_ = nullptr;
	}
}

ReturnCode_t PublisherImpl::set_qos(const PublisherQos& qos)
{

	PublisherQos defaultQos;
	participant_->get_default_publisher_qos(defaultQos);
    const PublisherQos& qosTemp = (&qos == &PUBLISHER_QOS_DEFAULT) ? defaultQos : qos;

    if (&qos != &PUBLISHER_QOS_DEFAULT) {
        ReturnCode_t ret = QosPolicyCheck::CheckQos(qosTemp);
        if (RETCODE_OK != ret) {
			TRAVODDS_LOG(LOG_ERROR, "CheckQos Err ret = %d ");
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

ReturnCode_t PublisherImpl::get_qos(PublisherQos& qos) const
{
	qos = qos_;
    return RETCODE_OK;
}

ReturnCode_t PublisherImpl::set_listener(PublisherListener* listener, const StatusMask& mask)
{
    if (listener_) {
		//delete listener_;
		listener_ = nullptr;
    }
	listener_ = listener;
	statusMask_ = mask;
	return RETCODE_OK;
}

const PublisherListener* PublisherImpl::get_listener() const
{
    return listener_;
}

DataWriter* PublisherImpl::create_datawriter(Topic* topic, const DataWriterQos& qos, DataWriterListener* listener, const StatusMask& mask)
{
    if (!topic || topic->get_participant() != get_participant()) {
		TRAVODDS_LOG(LOG_ERROR, "topic is nullptr || publisher and topic not same domparticipant");
        return nullptr;
    }
	const DataWriterQos& qosT = (&qos == &DATAWRITER_QOS_DEFAULT) ? defaultDatawriterQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
		return nullptr;
	}
	TypeSupport* typeSupport = participant_->find_type(topic->get_type_name());
	if (nullptr == typeSupport) {
		TRAVODDS_LOG(LOG_ERROR, "typeSupport[%s] is nullptr", topic->get_type_name());
		return nullptr;
	}

#ifdef DDSSECURITY
	SecurityException secExce;
	if (participant_->enableDDSSec_) {
		if (!participant_->secPlugin_.access_control->check_create_datawriter(
			participant_->permissionHdl_,
			participant_->GetDomainId(),
			topic->get_name(),
			&qosT,
			&qos_.partition,
			NULL,
			&secExce)) {
			TRAVODDS_LOG(LOG_ERROR, "Create DataWrier failed on Publisher %s: rejected by access control plugin.");
			return nullptr;
		}
	}
#endif // DDSSECURITY

	DataWriter* datawriter = nullptr;
	std::unique_ptr<DataWriterImpl> datawriterPtr = std::make_unique<DataWriterImpl>(topic, qosT, listener, mask, this, typeSupport);
#ifdef DDSSECURITY
	//数据读者获取安全属性
	datawriterPtr->GetSecAttributes();
	//填充缓存的builtinSecureData_
	datawriterPtr->FillSecureData();
#endif
	if (!datawriterPtr.get()) {
		TRAVODDS_LOG(LOG_ERROR, "create datawriter Err");
		return nullptr;
	}
	GUID_t guid = participant_->GetRelatedParticipant()->GenerateGuid(ENTITYID_UNKNOWN, typeSupport->has_key(), ENTITY_WRITER);
	datawriterPtr->SetInstanceHandle(Conversion::ToInstanceHandle(guid));
	datawriter = datawriterPtr.get();

	std::lock_guard<std::mutex> lock(mtxWriters_);
	writerHandles_.insert({ datawriterPtr->get_instance_handle(), std::move(datawriterPtr) });

	if (enable_ && qos_.entity_factory.autoenable_created_entities) {
		if (RETCODE_OK != datawriter->enable()) {
			delete_datawriter(datawriter);
			datawriter = nullptr;
			TRAVODDS_LOG(LOG_ERROR, "datawriter->enable Err");
			return nullptr;
		}
	}
	return datawriter;
}

ReturnCode_t PublisherImpl::delete_datawriter(DataWriter*& datawriter)
{
    if (!datawriter) {
		TRAVODDS_LOG(LOG_ERROR, "datawriter is nullptr");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != datawriter->get_publisher()) {
		TRAVODDS_LOG(LOG_ERROR, "publisher donot contain the datawriter");
        return RETCODE_PRECONDITION_NOT_MET;
    }
	
    InstanceHandle_t handle = datawriter->get_instance_handle();

	participant_->GetRelatedParticipant()->DeleteWriter(handle);
	std::shared_ptr<Discovery> discovery = participant_->GetDiscovery();
	discovery->RemovePublicationData(handle);

    std::lock_guard<std::mutex> lock(mtxWriters_);
    auto it = writerHandles_.find(handle);
    if (it != writerHandles_.end()) {
        writerHandles_.erase(handle);
        datawriter = nullptr;
        return RETCODE_OK;
    }
	TRAVODDS_LOG(LOG_ERROR, "publisher cont find the datawriter");
	return RETCODE_ERROR;
}

DataWriter* PublisherImpl::lookup_datawriter(const std::string& topic_name) const
{
    DataWriter* writer = nullptr;
    std::lock_guard<std::mutex> lock(mtxWriters_);
    for (auto& it : writerHandles_) {
        if (it.second.get()->get_topic()->get_name() == topic_name) {
            writer =  it.second.get();
            break;
        }
    }
    return writer;
}

ReturnCode_t PublisherImpl::suspend_publications()
{
	TRAVODDS_LOG(LOG_ERROR, "Publisher not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t PublisherImpl::resume_publications()
{
	TRAVODDS_LOG(LOG_ERROR, "Publisher not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t PublisherImpl::begin_coherent_changes()
{
	TRAVODDS_LOG(LOG_ERROR, "Publisher not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t PublisherImpl::end_coherent_changes()
{
	TRAVODDS_LOG(LOG_ERROR, "Publisher not support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t PublisherImpl::wait_for_acknowledgments(const Duration_t& max_wait)
{
	ReturnCode_t ret;
	Duration_t begin, end;
	Duration_t currentWait = max_wait;
	std::lock_guard<std::mutex> lock(mtxWriters_);
	for (auto& it : writerHandles_) {
		DataWriterImpl* datawriter = it.second.get();
		Duration_t::now(begin);
		ret = datawriter->wait_for_acknowledgments(currentWait);
		if (RETCODE_OK != ret) {
			return ret;
		}
		Duration_t::now(end);
		currentWait = currentWait - (end - begin);
		if( currentWait < Duration_t(TIME_ZERO)) {
			return RETCODE_TIMEOUT;
		}
	}
    return RETCODE_OK;
}

DomainParticipant* PublisherImpl::get_participant() const
{
    return participant_;
}

ReturnCode_t PublisherImpl::delete_contained_entities()
{
	
	std::lock_guard<std::mutex> lock(mtxWriters_);

	for (auto itor = writerHandles_.begin(); itor != writerHandles_.end(); itor++) {
		DataWriter* datawriter = itor->second.get();

		InstanceHandle_t handle = datawriter->get_instance_handle();
		participant_->GetRelatedParticipant()->DeleteWriter(handle);
		std::shared_ptr<Discovery> discovery = participant_->GetDiscovery();
		discovery->RemovePublicationData(handle);
	}
	
	writerHandles_.clear();

    return RETCODE_OK;
}

ReturnCode_t PublisherImpl::set_default_datawriter_qos(const DataWriterQos &qos)
{
    if (&qos == &DATAWRITER_QOS_DEFAULT) {
        /* yuguxu ?预留操作，默认的QoS具体是指配置文件中设置，还是默认宏所指 */
        defaultDatawriterQos_ = DATAWRITER_QOS_DEFAULT;
        return RETCODE_OK;
    }
    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
		return ret;
    }
    defaultDatawriterQos_ = qos;
    return RETCODE_OK;
}

ReturnCode_t PublisherImpl::get_default_datawriter_qos(DataWriterQos &qos) const
{
    qos = defaultDatawriterQos_;
    return RETCODE_OK;
}

ReturnCode_t Publisher::copy_from_topic_qos(DataWriterQos& datawriter_qos, const TopicQos& topic_qos)
{
    datawriter_qos.durability = topic_qos.durability;
    datawriter_qos.durability_service = topic_qos.durability_service;
    datawriter_qos.deadline = topic_qos.deadline;
    datawriter_qos.latency_budget = topic_qos.latency_budget;
    datawriter_qos.liveliness = topic_qos.liveliness;
    datawriter_qos.reliability = topic_qos.reliability;
    datawriter_qos.destination_order = topic_qos.destination_order;
    datawriter_qos.history = topic_qos.history;
    datawriter_qos.resource_limits = topic_qos.resource_limits;
    datawriter_qos.transport_priority = topic_qos.transport_priority;
    datawriter_qos.lifespan = topic_qos.lifespan;
    datawriter_qos.ownership = topic_qos.ownership;

    return RETCODE_OK;
}

ReturnCode_t PublisherImpl::enable()
{
	if (enable_) {
		return RETCODE_OK;
	}
	if (!participant_->IsEnable()) {
		TRAVODDS_LOG(LOG_ERROR, "IsEnable Err");
		return RETCODE_PRECONDITION_NOT_MET;
	}
	{
		std::lock_guard<std::mutex> lock(mtxWriters_);
		for (auto& writer : writerHandles_) {
			writer.second->enable();
		}
	}
	enable_ = true;
	return RETCODE_OK;
}

bool PublisherImpl::contains_entity(const InstanceHandle_t& handle) const
{    
    std::lock_guard<std::mutex> lock(mtxWriters_);
    if (writerHandles_.find(handle) != writerHandles_.end()) {
        return true;
    }
    return false;
}

bool PublisherImpl::Empty() const
{
    return writerHandles_.empty();
}
