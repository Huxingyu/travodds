#include "dcps/publish/publisherimpl.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/writer.h"
#include "qos/qospolicycheck.h"
#include "dcps/topic/typesupport.h"
#include "rtps/discovery/discovery.h"

USING_AGIDDS_NAMESPACE;
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
		delete listener_;
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
            return ret;
        }
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qosTemp)) {
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
    if (listener) {
        listener_ = listener;
        statusMask_ = mask;
        return RETCODE_OK;
    }
    return RETCODE_BAD_PARAMETER;
}

const PublisherListener* PublisherImpl::get_listener() const
{
    return listener_;
}

DataWriter* PublisherImpl::create_datawriter(Topic* topic, const DataWriterQos& qos, DataWriterListener* listener, const StatusMask& mask)
{
    if (!topic || topic->get_participant() != get_participant()) {
        return nullptr;
    }
	const DataWriterQos& qosT = (&qos == &DATAWRITER_QOS_DEFAULT) ? defaultDatawriterQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		return nullptr;
	}
	TypeSupport* typeSupport = participant_->find_type(topic->get_type_name());
	DataWriter* datawriter = nullptr;
	std::unique_ptr<DataWriterImpl> datawriterPtr = std::make_unique<DataWriterImpl>(topic, qosT, listener, mask, this, typeSupport);
	if (!datawriterPtr.get()) {
		return nullptr;
	}
	datawriter = datawriterPtr.get();
    bool key = false;
    if (nullptr != typeSupport && typeSupport->has_key()) {
        key = true;
    }
	std::shared_ptr<Writer>  writerPtr = participant_->GetRelatedParticipant()->CreateWriter(datawriterPtr.get(), qos, ENTITYID_UNKNOWN, key);

	if (!writerPtr.get()) {
		delete_datawriter(datawriter);
		datawriter = nullptr;
		return nullptr;
	}
    InstanceHandle_t handle = DomainParticipantImpl::GenerateInstanceHandle(writerPtr->GetGuid());
	datawriterPtr->SetInstanceHandle(handle);

	DiscoveredWriterData writerData;
	writerData.ddsPublicationData = datawriterPtr->GetBuiltinTopicData();
	std::shared_ptr<Discovery> discovery = participant_->GetDiscovery();
	discovery->WritePublicationData(writerData);

	std::lock_guard<std::mutex> lock(mtxWriters_);
	writerHandles_.insert({ datawriterPtr->get_instance_handle(), std::move(datawriterPtr) });

	if (enable_ && qos_.entity_factory.autoenable_created_entities) {
		if (RETCODE_OK != datawriter->enable()) {
			participant_->GetRelatedParticipant()->DeleteWriter(handle);
			delete_datawriter(datawriter);
			datawriter = nullptr;
			return nullptr;
		}
	}
	participant_->GetRelatedParticipant()->AddLocalWriter(writerPtr);

	return datawriter;
}

ReturnCode_t PublisherImpl::delete_datawriter(DataWriter*& datawriter)
{
    if (!datawriter) {
        return RETCODE_BAD_PARAMETER;
    }
    if (this != datawriter->get_publisher()) {
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
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t PublisherImpl::resume_publications()
{
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t PublisherImpl::begin_coherent_changes()
{
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t PublisherImpl::end_coherent_changes()
{
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t PublisherImpl::wait_for_acknowledgments(const Duration_t& max_wait)
{
    return RETCODE_UNSUPPORTED;
}

DomainParticipant* PublisherImpl::get_participant() const
{
    return participant_;
}

ReturnCode_t PublisherImpl::delete_contained_entities()
{
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
