#include "dcps/subscribe/datareaderimpl.h"
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/topic/topicdescriptionimpl.h"
#include "dcps/topic/typesupport.h"
#include "dcps/condition/readconditionimpl.h"
#include "dcps/condition/queryconditionimpl.h"
#include "dcps/base/statusmanager.h"
#include "dcps/listener/subscriberlistener.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "rtps/historycache/historycache.h"
#include "rtps/endpoint/statefulreader.h"
#include "rtps/endpoint/participant.h"
#include "qos/qospolicycheck.h"
#include "common/time/ddstimer.h"
#include "common/timeconversion.h"
USING_AGIDDS_NAMESPACE;

DataReaderImpl::DataReaderImpl(TopicDescription* topic, const DataReaderQos& qos, DataReaderListener* listener, StatusMask mask, SubscriberImpl* sub, TypeSupport* typeSupport)
    : DomainEntityImpl(mask), topic_(dynamic_cast<TopicDescriptionImpl*>(topic)), 
	  qos_(qos), listener_(listener), sub_(sub), typeSupport_(typeSupport),
      statusMgr_(new DataReaderStatusManager(this)),
	  deadlineTimer_(nullptr),
      history_(nullptr)
{
	topic_->Reference();
	if(topic)
	{
		memcpy(builtinData_.key.value, &handle_.keyHash.value, 16);
		InstanceHandle_t handle = sub_->get_participant()->get_instance_handle();
		memcpy(builtinData_.participant_key.value, &handle.keyHash.value, 16);
		builtinData_.topic_name = topic->get_name();
		builtinData_.type_name = topic->get_type_name();
		builtinData_.durability = qos.durability;
		builtinData_.deadline = qos.deadline;
		builtinData_.latency_budget = qos.latency_budget;
		builtinData_.liveliness = qos.liveliness;
		builtinData_.reliability = qos.reliability;
		builtinData_.user_data = qos.user_data;
		builtinData_.ownership = qos.ownership;
		builtinData_.destination_order = qos.destination_order;
		builtinData_.time_based_filter = qos.time_based_filter;
		SubscriberQos subQos;
		sub_->get_qos(subQos);
		builtinData_.presentation = subQos.presentation;
		builtinData_.partition = subQos.partition;
		builtinData_.group_data = subQos.group_data;
		Topic* realTopic = dynamic_cast<Topic*>(topic);
		if (realTopic) {
			TopicQos topicQos;
			realTopic->get_qos(topicQos);
			builtinData_.topic_data = topicQos.topic_data;
		}
	}
}

DataReaderImpl::~DataReaderImpl()
{
	reader_->SetRelatedDataReader(nullptr);
	topic_->Dereference();
	readcdts_.clear();
	querycdts_.clear();
    if (listener_) {
        delete listener_;
		listener_ = nullptr;
    }
}

ReturnCode_t DataReaderImpl::set_qos(const DataReaderQos& qos)
{
    DataReaderQos defaultQos;
    sub_->get_default_datareader_qos(defaultQos);
    const DataReaderQos& qosTemp = (&qos == &DATAREADER_QOS_DEFAULT) ? defaultQos : qos;
    if (&qos != &DATAREADER_QOS_DEFAULT) {
        ReturnCode_t ret = QosPolicyCheck::CheckQos(qosTemp);
        if (RETCODE_OK != ret) {
            return ret;
        }
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qosTemp)) {
        return RETCODE_IMMUTABLE_POLICY;
    }
	if (enable_) {
		if (qos_.deadline.period != qos.deadline.period && qos_.deadline.period != Duration_t(TIME_INFINITE)) {
			if (deadlineTimer_) {
				deadlineTimer_->UpdatePeriod(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
			}
			else {
                deadlineTimer_.reset();
                deadlineTimer_ = TimerManager::CreateTimer(
					reinterpret_cast<const Time_t&>(qosTemp.deadline.period),
					[this] {
						statusMgr_->UpdateRequestedDeadlineMissedStatus();
					},
					"dr.deadline"
                );
			}
		}
		else if (qos_.deadline.period == Duration_t(TIME_INFINITE)) {
			deadlineTimer_.reset();
		}
	}
	qos_ = qosTemp;

    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::get_qos(DataReaderQos& qos) const
{
    qos = qos_;
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::set_listener(DataReaderListener* listener, const StatusMask& mask)
{
    if (listener) {
        listener_ = listener;
        statusMask_ = mask;
        return RETCODE_OK;
    }
    return RETCODE_BAD_PARAMETER;
}

const DataReaderListener * DataReaderImpl::get_listener() const
{
    return listener_;
}

ReturnCode_t DataReaderImpl::read(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, SampleStateMask sample_states,
	ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, sample_states, view_states, instance_states);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, sample_states, view_states, instance_states, true);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {

		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);

		if (buffer->origin_buffer) {
			MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
		}
	}
	return ret;
}

ReturnCode_t DataReaderImpl::read_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, ReadCondition* condition)
{
	SerializedBufferSeq datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, condition->get_sample_state_mask(),
		condition->get_view_state_mask(), condition->get_instance_state_mask());
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, ReadCondition* condition)
{
	SerializedBufferSeq datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, condition->get_sample_state_mask(),
		condition->get_view_state_mask(), condition->get_instance_state_mask(), true);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer,littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);

		if (buffer->origin_buffer) {
			MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
		}
	}
	return ret;
}

ReturnCode_t DataReaderImpl::read_next_sample(void* data_value, SampleInfo &sample_info)
{
    ReturnCode_t ret = RETCODE_OK;
	SerializedBufferSeq datas;
	SampleInfoSeq sample_infos;

	ret = reader_->ReadOrTake(datas, sample_infos,1, NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
	if (RETCODE_OK == ret) {
		if (!sample_infos[0].valid_data) {
			return ret;
		}
		SerializedBuffer* buffer = &datas[0];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_value, &cdr, littleEndian);
		sample_info = sample_infos[0];
	}
	
    return ret;
}

ReturnCode_t DataReaderImpl::take_next_sample(void* data_value, SampleInfo &sample_info)
{
	ReturnCode_t ret = RETCODE_OK;
	SerializedBufferSeq datas;
	SampleInfoSeq sample_infos;

	ret = reader_->ReadOrTake(datas, sample_infos, 1, NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE,true);
	if (RETCODE_OK == ret) {
		if (!sample_infos[0].valid_data) {
			return ret;
		}
		SerializedBuffer* buffer = &datas[0];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_value, &cdr, littleEndian);
		sample_info = sample_infos[0];

		if (buffer->origin_buffer) {
			MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
		}
	}

	return ret;
}

ReturnCode_t DataReaderImpl::read_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& a_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeInstance(datas, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);
		
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeInstance(datas, sample_infos, max_samples, handle, sample_states, view_states, instance_states,true);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);

		if (buffer->origin_buffer) {
			MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
		}
	}
	return ret;
}

ReturnCode_t DataReaderImpl::read_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle, sample_states, view_states, instance_states);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle, sample_states, view_states, instance_states,true);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);

		if (buffer->origin_buffer) {
			MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
		}
	}

	return ret;
}

ReturnCode_t DataReaderImpl::read_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, ReadCondition* condition)
{
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle,
		condition->get_sample_state_mask(), condition->get_view_state_mask(), condition->get_instance_state_mask());
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);
	}

	return ret;
}

ReturnCode_t DataReaderImpl::take_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, ReadCondition* condition)
{
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle,
		condition->get_sample_state_mask(), condition->get_view_state_mask(), condition->get_instance_state_mask(), true);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		AGIDDSCdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);

		if (buffer->origin_buffer) {
			MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
		}
	}
	return ret;
}

ReturnCode_t DataReaderImpl::return_loan(DataSeq& data_values, SampleInfoSeq& sample_infos)
{
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DataReaderImpl::get_key_value(void* key_holder, const InstanceHandle_t& handle)
{
	if (!typeSupport_->has_key()) {
		std::cout << "Topic is NO_KEY, operation not permitted" << std::endl;
		return RETCODE_ILLEGAL_OPERATION;
	}

	SerializedBuffer* cacheData = history_->get_key_value(handle);
	if (nullptr == cacheData) {
		return RETCODE_BAD_PARAMETER;
	}

	AGIDDSCdrDeserializer cdr(cacheData);
	typeSupport_->deserialize_data(key_holder, &cdr, 1);
	return RETCODE_OK;
}

InstanceHandle_t DataReaderImpl::lookup_instance(const void* instance_data) const
{
	InstanceHandle_t handle = HANDLE_NIL;

	if (typeSupport_->has_key()) {
		if (MakeKey(instance_data, handle, true)) {
			if (!history_->IsInstancePresent(handle)) {
				handle = HANDLE_NIL;
			}
		}
	}
	return handle;
}

ReadCondition* DataReaderImpl::create_readcondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	ReadCondition* readcdt = nullptr;
    std::shared_ptr<ReadCondition> readcdtPtr = std::make_shared<ReadConditionImpl>(smaple_states, view_states, instance_states,this);
    if (readcdtPtr) {
        readcdts_.push_back(readcdtPtr);
		readcdt = readcdtPtr.get();
    }
    return readcdt;
}

QueryCondition* DataReaderImpl::create_querycondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states, const std::string & query_expression, const StringSeq& query_parameters)
{
	QueryCondition* querycdt = nullptr;
	std::shared_ptr<QueryCondition> querycdtPtr = std::make_shared<QueryConditionImpl>(smaple_states, view_states, instance_states, query_expression, query_parameters, this);
	if (querycdtPtr) {
		querycdts_.push_back(querycdtPtr);
		querycdt = querycdtPtr.get();
	}

	return querycdt;
}

ReturnCode_t DataReaderImpl::delete_readcondition(ReadCondition* condition)
{
	if (condition == nullptr) {
		return RETCODE_BAD_PARAMETER;
	}
	for (auto it = readcdts_.begin(); it != readcdts_.end(); it++) {
		if (it->get() == condition) {
			readcdts_.erase(it);
			return RETCODE_OK;
		}
	}
	for (auto it = querycdts_.begin(); it != querycdts_.end(); it++) {
		if (it->get() == condition) {
			querycdts_.erase(it);
			return RETCODE_OK;
		}
	}
	return RETCODE_PRECONDITION_NOT_MET;
}

ReturnCode_t DataReaderImpl::get_liveliness_changed_status(LivelinessChangedStatus &status) const
{
    return statusMgr_->GetLivelinessChangedStatus(status);
}

ReturnCode_t DataReaderImpl::get_requested_deadline_missed_status(RequestedDeadlineMissedStatus &status)
{
	return statusMgr_->GetRequestedDeadlineMissedStatus(status);
}

ReturnCode_t DataReaderImpl::get_requested_incompatible_qos_status(RequestedIncompatibleQosStatus &status)
{
	return statusMgr_->GetRequestedIncompatibleQosStatus(status);
}

ReturnCode_t DataReaderImpl::get_sample_lost_status(SampleLostStatus &status) const
{
	return statusMgr_->GetSampleLostStatus(status);
}

ReturnCode_t DataReaderImpl::get_sample_rejected_status(SampleRejectedStatus &status) const
{
	return statusMgr_->GetSampleRejectedStatus(status);
}

ReturnCode_t DataReaderImpl::get_subscription_matched_status(SubscriptionMatchedStatus &status) const
{
    return statusMgr_->GetSubscriptionMatchedStatus(status);;
}

const TopicDescription *DataReaderImpl::get_topicdescription() const
{
    return topic_;
}

const Subscriber *DataReaderImpl::get_subscriber() const
{
    return sub_;
}

ReturnCode_t DataReaderImpl::delete_contained_entities()
{
	readcdts_.clear();
	querycdts_.clear();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::wait_for_historical_data(const Duration_t &max_wait) const
{
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DataReaderImpl::get_matched_publication_data(PublicationBuiltinTopicData &publication_data, const InstanceHandle_t &publication_handle) const
{
	StatefulReader* reader = dynamic_cast<StatefulReader*>(reader_.get());
	if (nullptr == reader) {
		return RETCODE_ERROR;
	}
	GUID_t guid;
	memcpy(&guid, publication_handle.keyHash.value, 16);
	if (nullptr == reader->MatchedWriterLookup(guid)) {
		return RETCODE_BAD_PARAMETER;
	}
	const DomainParticipant *domain = sub_->get_participant();
	const DomainParticipantImpl* domainImpl = dynamic_cast<const DomainParticipantImpl*>(domain);
	if (domainImpl) {
		std::shared_ptr<Participant> entity = domainImpl->GetRelatedParticipant();
		return entity->GetPublicationData(publication_data, publication_handle);
	}
	return RETCODE_ERROR;
}

ReturnCode_t DataReaderImpl::get_matched_publications(InstanceHandleSeq &publication_handles) const
{
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DataReaderImpl::enable()
{
	if (enable_) {
		return RETCODE_OK;
	}
	if (!sub_->IsEnable()) {
		return RETCODE_PRECONDITION_NOT_MET;
	}
	/* yuguxu 预留 DataReader 资源申请
	 *
	 * 待实现
	 */
	history_.reset(new HistoryCache(
		qos_.history, qos_.resource_limits, { typeSupport_->has_key() ? WITH_KEY : NO_KEY }));

	if (qos_.deadline.period != Duration_t(TIME_INFINITE)) {
        deadlineTimer_.reset();
        deadlineTimer_ = TimerManager::CreateTimer(
			reinterpret_cast<Time_t&>(qos_.deadline.period),
			[this] {
				statusMgr_->UpdateRequestedDeadlineMissedStatus();
			},
			"dr.deadline"
        );
	}

	enable_ = true;
	return RETCODE_OK;
}

void DataReaderImpl::SetRelatedReader(std::shared_ptr<Reader> reader)
{
	reader_ = reader;
	reader->SetRelatedDataReader(this);
	handle_ = Conversion::ToInstanceHandle(reader->GetGuid());
	builtinData_.key = Conversion::ToBuiltinTopicKey(handle_);
}

bool DataReaderImpl::Empty() const
{
	return readcdts_.empty() && querycdts_.empty();
}

ReturnCode_t DataReaderImpl::OnDataAvaible()
{
	/* 非用户reader 不触发回调 */
	GUID_t guid = reader_->GetGuid();
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind) {
		return  RETCODE_OK;
	}

	if (qos_.deadline.period != Duration_t(TIME_INFINITE)) {
		deadlineTimer_->ReStart();
	}
	for (auto& rcdt : readcdts_) {
		// 条件等待及相关 
		//rcdt
		//TODO 
	}

	auto subListener = sub_->GetListener();
	const DomainParticipantImpl* participant = dynamic_cast<const DomainParticipantImpl*>(sub_->get_participant());
	 
	if(participant->get_listener() && (DATA_ON_READERS_STATUS & participant->get_status_mask())) {
		DomainParticipantListener * listener = const_cast<DomainParticipantListener*>(participant->get_listener());
		listener->on_data_on_readers(sub_);
		return RETCODE_OK;

	} else if (subListener && (DATA_ON_READERS_STATUS & sub_->get_status_mask())) {
		subListener->on_data_on_readers(sub_);
		return RETCODE_OK;
	} else if (listener_ && (DATA_AVAILABLE_STATUS & statusMask_)) {
		listener_->on_data_available(this);
	}

	return RETCODE_OK;
}
ReturnCode_t DataReaderImpl::OnSampleLost(int32_t lostCount)
{
	return statusMgr_->UpdateSampleLostStatus(lostCount);
}
int32_t DataReaderImpl::OnSampleRejected(const InstanceHandle_t& handle, const SampleRejectedStatusKind& reason)
{
	return statusMgr_->UpdateSampleRejectedStatus(handle, reason);
}

void DataReaderImpl::OnRequestedIncompatibleQos(std::set<QosPolicyId_t>& qosSeq)
{
	statusMgr_->UpdateRequestedIncompatibleQosStatus(qosSeq);

	if (0/*statusCondition_*/) {
		//TODO 条件等待集等待实现 
	}
}
void DataReaderImpl::OnSubscriptionMatched(InstanceHandle_t& handle, const bool& add) {
	statusMgr_->UpdateSubscriptionMatchedStatus(handle, add);

	if (0/*statusCondition_*/) {
		//TODO 条件等待集等待实现 
	}
}

const SubscriptionBuiltinTopicData& DataReaderImpl::GetBuiltinTopicData()
{
	return builtinData_;
}


bool DataReaderImpl::GetEndianess(SerializedBuffer* buffer)
{
	bool ret = false;
	ret = buffer->buffer[1] == 0x01;

	buffer->buffer += 4;
	buffer->writer_position -= 4;
	buffer->buffer_size -= 4;

	return ret;
}

bool DataReaderImpl::MakeKey(const void*  data, InstanceHandle_t& handle, bool forceMd5) const
{
	SerializedBuffer *buffer = new SerializedBuffer;
	buffer->buffer_size = typeSupport_->get_serialized_data_size((void*)data, 0);
	MemoryPool::GetInstance()->getBuffer(buffer->buffer_size, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer;

	AGIDDSCdrSerializer cdr(buffer);

	return typeSupport_->get_instancehandle((void*)data, &cdr, handle, true);;
}


const InstanceHandle_t& DataReaderImpl::GetLastinstanceHandle() const
{
	std::lock_guard<std::mutex> lock(lastInstanceMtx_);
	return lastInstance_;
}

void DataReaderImpl::SetLastinstanceHandle(const InstanceHandle_t& handle)
{
	std::lock_guard<std::mutex> lock(lastInstanceMtx_);
	lastInstance_ = handle;
}

ReturnCode_t DataReaderImpl::UpdateLivelinessChangedStatus(const InstanceHandle_t & handle, const bool & alive, const bool & ensure)
{
	return statusMgr_->UpdateLivelinessChangedStatus(handle, alive, ensure);
}


