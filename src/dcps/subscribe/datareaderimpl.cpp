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
#include "rtps/discovery/discovery.h"
#include "qos/qospolicycheck.h"
#include "common/time/ddstimer.h"
#include "common/timeconversion.h"
#include "common/log/logger.h"
USING_AGIDDS_NAMESPACE;

DataReaderImpl::DataReaderImpl(TopicDescription* topic, const DataReaderQos& qos, DataReaderListener* listener, StatusMask mask, SubscriberImpl* sub, TypeSupport* typeSupport)
    : DomainEntityImpl(mask), topic_(dynamic_cast<TopicDescriptionImpl*>(topic)), 
	  qos_(qos), listener_(listener), sub_(sub), typeSupport_(typeSupport),
      statusMgr_(new DataReaderStatusManager(this)),
	  deadlineTimer_(nullptr),
	  lastInstance_(HANDLE_NIL),
      history_(nullptr)
{
	topic_->Reference();
	if(topic)
	{
		memcpy(builtinData_.key.value, &handle_.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		InstanceHandle_t handle = sub_->get_participant()->get_instance_handle();
		memcpy(builtinData_.participant_key.value, &handle.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
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
	if (deadlineTimer_) {
		deadlineTimer_->Exit();
	}

	reader_->SetRelatedDataReader(nullptr);
	topic_->Dereference();
	readcdts_.clear();
	querycdts_.clear();
    if (listener_) {
        //delete listener_;
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
			AGILOG(AGILOG_ERROR, "CheckQos Err");
            return ret;
        }
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qosTemp)) {
		AGILOG(AGILOG_ERROR, "CanQosBeUpdated Err");
        return RETCODE_IMMUTABLE_POLICY;
    }
	if (enable_) {
		if (qos_.deadline.period != qos.deadline.period && qos_.deadline.period != Duration_t(TIME_INFINITE)) {
			std::chrono::steady_clock::time_point nextTime =
				std::chrono::steady_clock::now() + std::chrono::milliseconds(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
			history_->UpAllDeadline(nextTime);
			if (deadlineTimer_) {
				deadlineTimer_->UpdatePeriod(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
			} else {
                deadlineTimer_ = TimerManager::CreateTimer(
					reinterpret_cast<const Time_t&>(qosTemp.deadline.period),
					[this] {
						DeadlineTimeout();
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
	//TODO JD 需要添加互斥锁  //
	if (listener_) {
		//delete listener_;
		listener_ = nullptr;
	}

	listener_ = listener;
	statusMask_ = mask;

    return RETCODE_OK;
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
	data_values.unloan();
	sample_infos.unloan();

	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, sample_states, view_states, instance_states);
	if (RETCODE_OK != ret) {
		if(RETCODE_ERROR == ret) {
			AGILOG(AGILOG_ERROR, "ReadOrTake ret = %d", ret);
		}
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
	data_values.unloan();
	sample_infos.unloan();
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, sample_states, view_states, instance_states, true);
	if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "ReadOrTake ret = %d", ret);
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
	data_values.unloan();
	sample_infos.unloan();
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, condition->get_sample_state_mask(),
		condition->get_view_state_mask(), condition->get_instance_state_mask());
	if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "ReadOrTake ret = %d", ret);
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
	data_values.unloan();
	sample_infos.unloan();
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, condition->get_sample_state_mask(),
		condition->get_view_state_mask(), condition->get_instance_state_mask(), true);
	if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "ReadOrTake ret = %d", ret);
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
			AGILOG(AGILOG_ERROR, "ReadOrTake ret = %d", ret);
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
			AGILOG(AGILOG_ERROR, "ReadOrTake ret = %d", ret);
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
		AGILOG(AGILOG_ERROR, "ReadOrTake ret = %d", ret);
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
		AGILOG(AGILOG_ERROR, "ReadOrTakeInstance ret = %d", ret);
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
		AGILOG(AGILOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
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
		AGILOG(AGILOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
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
		AGILOG(AGILOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
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
		AGILOG(AGILOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
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
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::get_key_value(void* key_holder, const InstanceHandle_t& handle)
{
	if (!typeSupport_->has_key()) {
		AGILOG(AGILOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return RETCODE_ILLEGAL_OPERATION;
	}

	SerializedBuffer* cacheData = history_->get_key_value(handle);
	if (nullptr == cacheData) {
		AGILOG(AGILOG_ERROR, "cacheData is nullptr,RETCODE_BAD_PARAMETER");
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
    std::shared_ptr<ReadConditionImpl> readcdtPtr = std::make_shared<ReadConditionImpl>(smaple_states, view_states, instance_states,this);
    if (readcdtPtr) {
        readcdts_.push_back(readcdtPtr);
		readcdt = readcdtPtr.get();
    }
    return readcdt;
}

QueryCondition* DataReaderImpl::create_querycondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states, const std::string & query_expression, const StringSeq& query_parameters)
{
	QueryCondition* querycdt = nullptr;
	std::shared_ptr<QueryConditionImpl> querycdtPtr = std::make_shared<QueryConditionImpl>(smaple_states, view_states, instance_states, query_expression, query_parameters, this);
	if (querycdtPtr) {
		querycdts_.push_back(querycdtPtr);
		querycdt = querycdtPtr.get();
	}

	return querycdt;
}

ReturnCode_t DataReaderImpl::delete_readcondition(ReadCondition* condition)
{
	if (condition == nullptr) {
		AGILOG(AGILOG_ERROR, "condition is nullptr,RETCODE_BAD_PARAMETER");
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
	AGILOG(AGILOG_ERROR, "datareder donot contain this condition");
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
	AGILOG(AGILOG_ERROR, "datareader donot support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DataReaderImpl::get_matched_publication_data(PublicationBuiltinTopicData &publication_data, const InstanceHandle_t &publication_handle) const
{
	if (nullptr == reader_) {
		return RETCODE_NOT_ENABLED;
	}
	if (reader_->GetMatchedPublicationData(publication_data, publication_handle)) {
		return RETCODE_OK;
	}
	return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DataReaderImpl::get_matched_publications(InstanceHandleSeq &publication_handles) const
{
    if(!reader_) {
		AGILOG(AGILOG_ERROR, "datareader releated reader is nullptr");
        return RETCODE_NOT_ENABLED;
    }
    reader_->GetMatchedPublications(publication_handles);
    return RETCODE_OK;
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
	DomainParticipantImpl* domainParticipant = dynamic_cast<DomainParticipantImpl*>(const_cast<DomainParticipant*>(sub_->get_participant()));
	if (nullptr == domainParticipant) {
		return RETCODE_ERROR;
	}
	std::shared_ptr<Reader> readerPtr = domainParticipant->GetRelatedParticipant()->CreateReader(this, qos_, Conversion::ToGuid(handle_), typeSupport_->has_key());
	if (!readerPtr.get()) {
		return RETCODE_ERROR;
	}
	history_ = readerPtr->readerCache_;

	if (qos_.deadline.period != Duration_t(TIME_INFINITE)) {
        deadlineTimer_ = TimerManager::CreateTimer(
			reinterpret_cast<Time_t&>(qos_.deadline.period),
			[this] {
				DeadlineTimeout();
			},
			"dr.deadline"
        );
	}

	domainParticipant->GetRelatedParticipant()->AddLocalReader(readerPtr);
	enable_ = true;

	//本地匹配完成后再发送SEPD报文，防止重复匹配
	DiscoveredReaderData readerData;
	readerData.ddsSubscriptionData = GetBuiltinTopicData();
	std::shared_ptr<Discovery> discovery = domainParticipant->GetDiscovery();
	discovery->WriteSubscriptionData(readerData);

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

	NotifyReadConditions();

	auto subListener = sub_->GetListener();
	const DomainParticipantImpl* participant = dynamic_cast<const DomainParticipantImpl*>(sub_->get_participant());
	 
	if(participant->get_listener() && (DATA_ON_READERS_STATUS & participant->get_status_mask())) {
		DomainParticipantListener * listener = const_cast<DomainParticipantListener*>(participant->get_listener());
		listener->on_data_on_readers(sub_);

	} else if (subListener && (DATA_ON_READERS_STATUS & sub_->get_status_mask())) {
		subListener->on_data_on_readers(sub_);
	} else if (listener_ && (DATA_AVAILABLE_STATUS & statusMask_)) {
		listener_->on_data_available(this);
	}

	SetStatus(true);
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
}
void DataReaderImpl::OnSubscriptionMatched(InstanceHandle_t& handle, const bool& add) {
	statusMgr_->UpdateSubscriptionMatchedStatus(handle, add);
}

const SubscriptionBuiltinTopicData& DataReaderImpl::GetBuiltinTopicData()
{
	return builtinData_;
}


bool DataReaderImpl::GetEndianess(SerializedBuffer* buffer)
{
	bool ret = false;
	ret = buffer->buffer[1] == uint32_t(1);

	buffer->buffer += CDR_ENDIANNESS_LEN;
	buffer->writer_position -= CDR_ENDIANNESS_LEN;
	buffer->buffer_size -= CDR_ENDIANNESS_LEN;

	return ret;
}

bool DataReaderImpl::MakeKey(const void*  data, InstanceHandle_t& handle, bool forceMd5) const
{
	SerializedBuffer buffer;
	buffer.buffer_size = typeSupport_->get_serialized_data_size((void*)data, 0);
	MemoryPool::GetInstance()->getBuffer(buffer.buffer_size, buffer.origin_buffer);
	buffer.buffer = buffer.origin_buffer;

	AGIDDSCdrSerializer cdr(&buffer);
	
	bool ret = typeSupport_->get_instancehandle((void*)data, &cdr, handle, true);
	MemoryPool::GetInstance()->releaseBuffer(buffer.origin_buffer);

	return ret;
}

void DataReaderImpl::SetLastinstanceHandle(const std::shared_ptr<CacheChange>& cache)
{
	/*  */
	cache->kind;
	

	/* deadline */
	if (deadlineTimer_) {
		std::lock_guard<std::mutex> lock(lastInstanceMtx_);
		std::chrono::steady_clock::time_point nextTime = 
			std::chrono::steady_clock::now() + std::chrono::milliseconds(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
		history_->SetNextDeadline(lastInstance_, nextTime);
		history_->GetNextDeadline(lastInstance_, nextTime);
		auto period = nextTime - std::chrono::steady_clock::now();
		deadlineTimer_->UpdatePeriod(std::chrono::duration_cast<std::chrono::milliseconds>(period).count());
	}

}

void DataReaderImpl::DeadlineTimeout()
{
	InstanceHandle_t instance;
	{
		std::lock_guard<std::mutex> lock(lastInstanceMtx_);
		instance = lastInstance_;
		std::chrono::steady_clock::time_point nextTime =
			std::chrono::steady_clock::now() + std::chrono::milliseconds(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
		history_->SetNextDeadline(lastInstance_, nextTime,true);
		history_->GetNextDeadline(lastInstance_, nextTime);
		auto period = nextTime - std::chrono::steady_clock::now();
		deadlineTimer_->UpdatePeriod(std::chrono::duration_cast<std::chrono::milliseconds>(period).count());
	}
	
	statusMgr_->UpdateRequestedDeadlineMissedStatus(instance);
}

ReturnCode_t DataReaderImpl::UpdateLivelinessChangedStatus(const InstanceHandle_t & handle, const bool & alive, const bool & ensure)
{
	if (!alive) {
		history_->RemoveRemoteWriter(Conversion::ToGuid(handle));
	}
	return statusMgr_->UpdateLivelinessChangedStatus(handle, alive, ensure);
}

void DataReaderImpl::SetStatus(bool add)
{
	dynamic_cast<StatusConditionImpl*>(&sub_->get_statuscondition())->set_status(DATA_ON_READERS_STATUS, add);
	dynamic_cast<StatusConditionImpl*>(&get_statuscondition())->set_status(DATA_AVAILABLE_STATUS, add);
}

SampleCounters DataReaderImpl::GetSampleCounters(bool trigger)
{
	return history_->GetSampleCounters(trigger);
}

void DataReaderImpl::NotifyReadConditions()
{
	for (auto& it : readcdts_) {
		if (it->get_trigger_value()) {
			it->Notify();
		}
	}
}

