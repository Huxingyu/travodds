#include "DDS/PublisherImpl.h"
#include "DDS/DomainParticipantImpl.h"
#include "DDS/DataWriterImpl.h"
#include "QoS/QosPolicyCheck.h"
#include "TypeSupport.h"

USING_AGIDDS_NAMESPACE;
PublisherImpl::PublisherImpl(PublisherQos qos, PublisherListener* a_listener, StatusMask mask, DomainParticipantImpl* a_participant)
    : DomainEntityImpl(mask), qos_(qos), listener_(a_listener), participant_(a_participant)
{

}

PublisherImpl::~PublisherImpl()
{
    writerHandles_.clear();
	if (listener_) {
		delete listener_;
		listener_ = nullptr;
	}
}

ReturnCode_t PublisherImpl::set_qos(const PublisherQos & qos)
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

ReturnCode_t PublisherImpl::get_qos(PublisherQos & qos) const
{
	qos = qos_;
    return RETCODE_OK;
}

ReturnCode_t PublisherImpl::set_listener(PublisherListener * a_listener, const StatusMask& mask)
{
    if (a_listener) {
        listener_ = a_listener;
        statusMask_ = mask;
        return RETCODE_OK;
    }
    return RETCODE_BAD_PARAMETER;
}

const PublisherListener * PublisherImpl::get_listener() const
{
    return listener_;
}

DataWriter *PublisherImpl::create_datawriter(Topic* a_topic, const DataWriterQos& qos, DataWriterListener* a_listener, const StatusMask& mask)
{
    if (!a_topic ||a_topic->get_participant() != get_participant()) {
        return nullptr;
    }
	const DataWriterQos& qosT = (&qos == &DATAWRITER_QOS_DEFAULT) ? defaultDatawriterQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		return nullptr;
	}
	TypeSupport* typeSupport = participant_->find_type(a_topic->get_type_name());
	DataWriter* writer = nullptr;
	std::unique_ptr<DataWriterImpl> writerPtr = std::make_unique<DataWriterImpl>(a_topic, qosT, a_listener, mask, this, typeSupport);
	if (!writerPtr.get()) {
		return nullptr;
	}
    writer = writerPtr.get();
	{
		std::lock_guard<std::mutex> lock(mtxWriters_);
        writerHandles_.insert({ writerPtr->get_instance_handle(), std::move(writerPtr) });
	}
	if (enable_ && qos_.entity_factory.autoenable_created_entities) {
		if (RETCODE_OK != writer->enable()) {
			delete_datawriter(writer);
			writer = nullptr;
			return nullptr;
		}
	}
	return writer;
}

ReturnCode_t PublisherImpl::delete_datawriter(DataWriter*& a_datawriter)
{
    if (!a_datawriter) {
        return RETCODE_BAD_PARAMETER;
    }
    if (this != a_datawriter->get_publisher()) {
        return RETCODE_PRECONDITION_NOT_MET;
    }
    InstanceHandle_t handle = a_datawriter->get_instance_handle();
    std::lock_guard<std::mutex> lock(mtxWriters_);
    auto it = writerHandles_.find(handle);
    if (it != writerHandles_.end()) {
        writerHandles_.erase(handle);
        a_datawriter = nullptr;
        return RETCODE_OK;
    }
}

DataWriter* PublisherImpl::lookup_datawriter(const string &topic_name) const
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

const DomainParticipant *PublisherImpl::get_participant() const
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

ReturnCode_t Publisher::copy_from_topic_qos(DataWriterQos & a_datawriter_qos, const TopicQos & a_topic_qos)
{
    a_datawriter_qos.durability = a_topic_qos.durability;
    a_datawriter_qos.durability_service = a_topic_qos.durability_service;
    a_datawriter_qos.deadline = a_topic_qos.deadline;
    a_datawriter_qos.latency_budget = a_topic_qos.latency_budget;
    a_datawriter_qos.liveliness = a_topic_qos.liveliness;
    a_datawriter_qos.reliability = a_topic_qos.reliability;
    a_datawriter_qos.destination_order = a_topic_qos.destination_order;
    a_datawriter_qos.history = a_topic_qos.history;
    a_datawriter_qos.resource_limits = a_topic_qos.resource_limits;
    a_datawriter_qos.transport_priority = a_topic_qos.transport_priority;
    a_datawriter_qos.lifespan = a_topic_qos.lifespan;
    a_datawriter_qos.ownership = a_topic_qos.ownership;

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

bool PublisherImpl::contains_entity(const InstanceHandle_t& a_handle) const
{    
    std::lock_guard<std::mutex> lock(mtxWriters_);
    if (writerHandles_.find(a_handle) != writerHandles_.end()) {
        return true;
    }
    return false;
}

bool PublisherImpl::Empty() const
{
    return writerHandles_.empty();
}
