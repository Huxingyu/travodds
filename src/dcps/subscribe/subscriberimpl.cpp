#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "dcps/topic/typesupport.h"
#include "qos/qospolicycheck.h"
#include "rtps/endpoint/participant.h"
#include "rtps/discovery/discovery.h"
#include "common/log/logger.h"
USING_TRAVODDS_NAMESPACE;

SubscriberImpl::SubscriberImpl(const SubscriberQos& qos, SubscriberListener* a_listener, StatusMask mask, DomainParticipantImpl* a_participant)
    : DomainEntityImpl(mask), qos_(qos), listener_(a_listener), participant_(a_participant)
{
    GUID_t guid = participant_->GetRelatedParticipant()->GenerateGuid(ENTITYID_UNKNOWN, false, ENTITY_SUB);
    handle_ = Conversion::ToInstanceHandle(guid);
}

SubscriberImpl::~SubscriberImpl()
{
	for (auto& it : readerHandles_) {
		participant_->GetRelatedParticipant()->DeleteReader(it.second->get_instance_handle());
	}
    readerHandles_.clear();
    if (listener_) {
        //delete listener_;
        listener_ = nullptr;
    }
}

ReturnCode_t SubscriberImpl::set_qos(const SubscriberQos& qos)
{
	SubscriberQos defaultQos;
	participant_->get_default_subscriber_qos(defaultQos);
    const SubscriberQos& qosTemp = (&qos == &SUBSCRIBER_QOS_DEFAULT) ? defaultQos : qos;

    if (&qos != &SUBSCRIBER_QOS_DEFAULT) {
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

ReturnCode_t SubscriberImpl::get_qos(SubscriberQos& qos) const
{
	qos = qos_;
    return RETCODE_OK;
}

ReturnCode_t SubscriberImpl::set_listener(SubscriberListener* a_listener, const StatusMask& mask)
{
	//TODO 线程安全
    if (listener_) {
		//delete listener_;
		listener_ = nullptr;
    }
	listener_ = a_listener;
	statusMask_ = mask;
	return RETCODE_OK;
}

const SubscriberListener* SubscriberImpl::get_listener() const
{
    return listener_;
}

DataReader *SubscriberImpl::create_datareader(TopicDescription* a_topic, const DataReaderQos& qos, DataReaderListener* a_listener, const StatusMask& mask)
{
    if (!a_topic || a_topic->get_participant() != get_participant()) {
		TRAVODDS_LOG(LOG_ERROR, "topic is nullptr || topic and subscriber not same domainparticipant");
        return nullptr;
    }
	const DataReaderQos& qosT = (&qos == &DATAREADER_QOS_DEFAULT) ? defaultDatareaderQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
		return nullptr;
	}
	TypeSupport* typeSupport = participant_->find_type(a_topic->get_type_name());
	if (nullptr == typeSupport) {
		TRAVODDS_LOG(LOG_ERROR, "typeSupport[%s] is nullptr", a_topic->get_type_name());
		return nullptr;
	}
#ifdef DDSSECURITY
    if (participant_->enableDDSSec_) {
        SecurityException secExce;
        if (!participant_->secPlugin_.access_control->check_create_datareader(
            participant_->permissionHdl_,
            participant_->GetDomainId(),
            a_topic->get_name(),
            &qosT,
            &qos_.partition,
            NULL,
            &secExce))
        {
            TRAVODDS_LOG(LOG_ERROR, "Create DataReader failed: rejected by access control plugin.");
            return nullptr;
        }
    }
#endif // DDSSCURITY

	DataReader* reader = nullptr;
	std::unique_ptr<DataReaderImpl> datareaderPtr = std::make_unique<DataReaderImpl>(a_topic, qosT, a_listener, mask, this, typeSupport);

    if (!datareaderPtr.get()) {
		TRAVODDS_LOG(LOG_ERROR, "create datareader[%s] fail", a_topic->get_type_name());
		return nullptr;
	}
    GUID_t guid = participant_->GetRelatedParticipant()->GenerateGuid(ENTITYID_UNKNOWN, typeSupport->has_key(), ENTITY_READER);
	//InstanceHandle_t handle = DomainParticipantImpl::GenerateInstanceHandle(readrPtr->GetGuid());
    datareaderPtr->SetInstanceHandle(Conversion::ToInstanceHandle(guid));
#ifdef DDSSECURITY
    //数据读者获取安全属性并填充掩码
    datareaderPtr->GetSecAttributes();
    datareaderPtr->FillSecureData();
#endif // DDSSCURITY
	reader = datareaderPtr.get();
    {
        std::lock_guard<std::mutex> lock(mtxReaders_);
        readerHandles_.insert({ datareaderPtr->get_instance_handle(), std::move(datareaderPtr) });
    }

	if (enable_ && qos_.entity_factory.autoenable_created_entities) {
		if (RETCODE_OK != reader->enable()) {
			delete_datareader(reader);
			reader = nullptr;
			TRAVODDS_LOG(LOG_ERROR, "enable Err");
			return nullptr;
		}
	}
	return reader;
}

ReturnCode_t SubscriberImpl::delete_datareader(DataReader*& a_dataReader)
{
    if (!a_dataReader) {
		TRAVODDS_LOG(LOG_ERROR, "datareader is nullptr");
        return RETCODE_BAD_PARAMETER;
    }
    if (this != a_dataReader->get_subscriber()) {
		TRAVODDS_LOG(LOG_ERROR, "subscriber donot contain the dateader");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    InstanceHandle_t handle = a_dataReader->get_instance_handle();
    if (dynamic_cast<DataReaderImpl*>(a_dataReader)->IsEnable()) {
        participant_->GetRelatedParticipant()->DeleteReader(handle);
        std::shared_ptr<Discovery> discovery = participant_->GetDiscovery();
        discovery->RemoveSubscriptionData(handle);
    }

    std::lock_guard<std::mutex> lock(mtxReaders_);
    auto it = readerHandles_.find(handle);
    if (it == readerHandles_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "subscriber cant find the dateader");
        return RETCODE_ERROR;
    }
    if (!it->second->Empty()) {
		TRAVODDS_LOG(LOG_ERROR, "the dateader is used");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    readerHandles_.erase(handle);
    a_dataReader = nullptr;
    return RETCODE_OK;
}

DataReader *SubscriberImpl::lookup_datareader(const std::string& topic_name) const
{
    DataReader* reader = nullptr;

    std::lock_guard<std::mutex> lock(mtxReaders_);
    for (auto& it : readerHandles_) {
        if (it.second->get_topicdescription()->get_name() == topic_name) {
            reader =  it.second.get();
            break;
        }
    }
    return reader;
}

ReturnCode_t SubscriberImpl::begin_access()
{
	TRAVODDS_LOG(LOG_ERROR, "subscriber not spupport interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t SubscriberImpl::end_access()
{
	TRAVODDS_LOG(LOG_ERROR, "subscriber not spupport interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t SubscriberImpl::get_datareaders(DataReaderSeq& readers, const SampleStateMask& sample_states, const ViewStateMask& view_states, const InstanceStateMask& instance_states) const
{
    for (auto& it : readerHandles_) {
        SampleCounters counters = it.second->GetSampleCounters(false);
        SampleStateMask smapleStates = (counters.sampleRead ? READ_SAMPLE_STATE : 0) |
            (counters.sampleNotRead ? NOT_READ_SAMPLE_STATE : 0);
        ViewStateMask viewStates = (counters.viewNew ? NEW_VIEW_STATE : 0) |
            (counters.viewNotNew ? NOT_NEW_VIEW_STATE : 0);
        InstanceStateMask instanceStates = (counters.instanceAlive ? ALIVE_INSTANCE_STATE : 0) |
            (counters.instanceDisposed ? NOT_ALIVE_DISPOSED_INSTANCE_STATE : 0) |
            (counters.instanceNoWriters ? NOT_ALIVE_NO_WRITERS_INSTANCE_STATE : 0);
        if (sample_states & smapleStates &&
            view_states & viewStates &&
            instance_states & instanceStates) {
            readers.push_back(it.second.get());
		}
    }
    return RETCODE_OK;
}

ReturnCode_t SubscriberImpl::notify_datareaders() const
{
    for (auto& it : readerHandles_) {
        DataReaderListener* listener = const_cast<DataReaderListener*>(it.second->get_listener());
        if (nullptr != listener) {
            listener->on_data_available(it.second.get());
        }
    }
    return RETCODE_OK;
}

DomainParticipant *SubscriberImpl::get_participant()
{
    return participant_;
}

ReturnCode_t SubscriberImpl::delete_contained_entities()
{

	std::lock_guard<std::mutex> lock(mtxReaders_);

	for (auto itor = readerHandles_.begin(); itor != readerHandles_.end(); itor++) {
		DataReader* datareader = itor->second.get();
        if (dynamic_cast<DataReaderImpl*>(datareader)->IsEnable()) {
            InstanceHandle_t handle = datareader->get_instance_handle();
            participant_->GetRelatedParticipant()->DeleteReader(handle);
            std::shared_ptr<Discovery> discovery = participant_->GetDiscovery();
            discovery->RemoveSubscriptionData(handle);
        }
		if (RETCODE_OK != datareader->delete_contained_entities()) {
			TRAVODDS_LOG(LOG_ERROR, "delete_contained_entities Err")
			return RETCODE_PRECONDITION_NOT_MET;
		}
	}

    readerHandles_.clear();
    return RETCODE_OK;
}

ReturnCode_t SubscriberImpl::set_default_datareader_qos(const DataReaderQos& qos)
{
    if (&qos == &DATAREADER_QOS_DEFAULT) {
        /* yuguxu ?预留操作，默认的QoS具体是指配置文件中设置，还是默认宏所指 */
        defaultDatareaderQos_ = DATAREADER_QOS_DEFAULT;
        return RETCODE_OK;
    }
    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos Err ret = %d", ret)
        return ret;
    }
    defaultDatareaderQos_ = qos;
    return RETCODE_OK;
}

ReturnCode_t SubscriberImpl::get_default_datareader_qos(DataReaderQos& qos) const
{
    qos = defaultDatareaderQos_;
    return RETCODE_OK;
}

ReturnCode_t Subscriber::copy_from_topic_qos(DataReaderQos& a_datareader_qos, const TopicQos& a_topic_qos)
{
    a_datareader_qos.durability = a_topic_qos.durability;
    a_datareader_qos.deadline = a_topic_qos.deadline;
    a_datareader_qos.latency_budget = a_topic_qos.latency_budget;
    a_datareader_qos.liveliness = a_topic_qos.liveliness;
    a_datareader_qos.reliability = a_topic_qos.reliability;
    a_datareader_qos.destination_order = a_topic_qos.destination_order;
    a_datareader_qos.history = a_topic_qos.history;
    a_datareader_qos.resource_limits = a_topic_qos.resource_limits;
    a_datareader_qos.ownership = a_topic_qos.ownership;

    return RETCODE_OK;
}

ReturnCode_t SubscriberImpl::enable()
{
	if (enable_) {
		return RETCODE_OK;
	}
	if (!participant_->IsEnable()) {
		TRAVODDS_LOG(LOG_ERROR, "IsEnable Err")
		return RETCODE_PRECONDITION_NOT_MET;
	}
	{
		std::lock_guard<std::mutex> lock(mtxReaders_);
		for (auto& reader : readerHandles_) {
			reader.second->enable();
		}
	}
	enable_ = true;
	return RETCODE_OK;
}

bool SubscriberImpl::contains_entity(const InstanceHandle_t& a_handle) const
{
    std::lock_guard<std::mutex> lock(mtxReaders_);
    if (readerHandles_.find(a_handle) != readerHandles_.end()) {
        return true;
    }
    return false;
}

bool SubscriberImpl::Empty() const
{
    return readerHandles_.empty();
}
SubscriberListener* SubscriberImpl::GetListener()
{
	return listener_;
}
