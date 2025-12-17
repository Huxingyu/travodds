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
#ifdef DDSSECURITY
#include "security/securitycommon.h"
#endif // DDSSECURITY

USING_TRAVODDS_NAMESPACE;

DataReaderImpl::DataReaderImpl(TopicDescription* topic, const DataReaderQos& qos, DataReaderListener* listener, StatusMask mask, SubscriberImpl* sub, TypeSupport* typeSupport)
    : DomainEntityImpl(mask), topic_(dynamic_cast<TopicDescriptionImpl*>(topic)), 
	  qos_(qos), listener_(listener), sub_(sub), typeSupport_(typeSupport),
      statusMgr_(new DataReaderStatusManager(this)),
	  deadlineTimer_(nullptr),
	  lifespanTimer_(nullptr),
	  lastInstance_(HANDLE_NIL),
      history_(nullptr)
{
	topic_->Reference();
	if(topic)
	{
		memcpy(builtinData_.key.value, &handle_.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		InstanceHandle_t handle = sub_->get_participant()->get_instance_handle();
		memcpy(builtinData_.participant_key.value, &handle.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
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
			builtinData_.topic_name = topic->get_name();
			builtinData_.topic_data = topicQos.topic_data;
		}
		ContentFilteredTopic* filteredTopic = dynamic_cast<ContentFilteredTopic*>(topic);
		if (filteredTopic) {
			TopicQos topicQos;
			filteredTopic->get_related_topic()->get_qos(topicQos);
			builtinData_.topic_name = filteredTopic->get_related_topic()->get_name();
			builtinData_.topic_data = topicQos.topic_data;
			contentFilter_.contentFilteredTopicName = filteredTopic->get_name();
			contentFilter_.relatedTopicName = filteredTopic->get_related_topic()->get_name();
			/* sql 语法 */
			contentFilter_.filterClassName = SQL_FILTER_NAME;
			contentFilter_.filterExpression = filteredTopic->get_filter_expression();
			filteredTopic->get_expression_parameters(contentFilter_.expressionParameters);
		}
#ifdef DDSSECURITY
		memcpy(builtinSecureData_.key.value, &handle_.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		memcpy(builtinSecureData_.participant_key.value, &handle.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		builtinSecureData_.type_name = topic->get_type_name();
		builtinSecureData_.durability = qos.durability;
		builtinSecureData_.deadline = qos.deadline;
		builtinSecureData_.latency_budget = qos.latency_budget;
		builtinSecureData_.liveliness = qos.liveliness;
		builtinSecureData_.reliability = qos.reliability;
		builtinSecureData_.user_data = qos.user_data;
		builtinSecureData_.ownership = qos.ownership;
		builtinSecureData_.destination_order = qos.destination_order;
		builtinSecureData_.time_based_filter = qos.time_based_filter;
		builtinSecureData_.presentation = subQos.presentation;
		builtinSecureData_.partition = subQos.partition;
		builtinSecureData_.group_data = subQos.group_data;
		if (realTopic) {
			TopicQos topicQos;
			realTopic->get_qos(topicQos);
			builtinSecureData_.topic_name = topic->get_name();
			builtinSecureData_.topic_data = topicQos.topic_data;
		}
		if (filteredTopic) {
			TopicQos topicQos;
			filteredTopic->get_related_topic()->get_qos(topicQos);
			builtinSecureData_.topic_name = filteredTopic->get_related_topic()->get_name();
			builtinSecureData_.topic_data = topicQos.topic_data;
			contentFilter_.contentFilteredTopicName = filteredTopic->get_name();
			contentFilter_.relatedTopicName = filteredTopic->get_related_topic()->get_name();
			/* sql 语法 */
			contentFilter_.filterClassName = SQL_FILTER_NAME;
			contentFilter_.filterExpression = filteredTopic->get_filter_expression();
			filteredTopic->get_expression_parameters(contentFilter_.expressionParameters);
		}
#endif // DDSSECURITY
	}
}

DataReaderImpl::~DataReaderImpl()
{
	if (deadlineTimer_) {
		deadlineTimer_->Exit();
	}

	if (lifespanTimer_) {
		lifespanTimer_->Exit();
	}
	if (enable_) {
		reader_->SetRelatedDataReader(nullptr);
	}
	topic_->Dereference();
	readcdts_.clear();
	querycdts_.clear();
    if (listener_) {
        //delete listener_;
		listener_ = nullptr;
    }
#ifdef DDSSECURITY
	for (const auto& it : preparingWriters_) {
		if (it.second != nullptr) {
			delete it.second;
			preparingWriters_[it.first] = nullptr;
		}
	}
	//安全扩展 释放crypto_key_factory相关
	if (enableSec_)
	{
		GUID_t drGuid = Conversion::ToGuid(get_instance_handle());
		if ((drGuid.IsSecureBuiltinEntity()
			&& drGuid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER)
			|| secAttributes_.is_payload_protected
			|| secAttributes_.is_submessage_protected)
		{
			if (secPlugin_ != NULL && secPlugin_->crypto_key_factory != NULL)
			{
				SecurityException secExce;
				secPlugin_->crypto_key_factory->unregister_datareader(
					cryptoHdl_, &secExce);
			}
		}
	}
#endif // DDSSECURITY
}

ReturnCode_t DataReaderImpl::set_qos(const DataReaderQos& qos)
{
    DataReaderQos defaultQos;
    sub_->get_default_datareader_qos(defaultQos);
    const DataReaderQos& qosTemp = (&qos == &DATAREADER_QOS_DEFAULT) ? defaultQos : qos;
    if (&qos != &DATAREADER_QOS_DEFAULT) {
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
	if (enable_) {
		if (qos_.deadline.period != qosTemp.deadline.period && qos_.deadline.period != Duration_t(TIME_INFINITE)) {
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
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	SerializedBufferSeq datas;
	ReturnCode_t ret = RETCODE_OK;
	data_values.unloan();
	sample_infos.unloan();

	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, sample_states, view_states, instance_states);
	if (RETCODE_OK != ret) {
		if(RETCODE_ERROR == ret) {
			TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
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

		CdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	SerializedBufferSeq datas;
	ReturnCode_t ret = RETCODE_OK;
	data_values.unloan();
	sample_infos.unloan();
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, sample_states, view_states, instance_states, true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {

		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);

		if (buffer->origin_buffer) {
			MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
		}
	}
	return ret;
}

ReturnCode_t DataReaderImpl::read_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, ReadCondition* condition)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	SerializedBufferSeq datas;
	ReturnCode_t ret = RETCODE_OK;
	data_values.unloan();
	sample_infos.unloan();
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, condition->get_sample_state_mask(),
		condition->get_view_state_mask(), condition->get_instance_state_mask());
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, ReadCondition* condition)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	SerializedBufferSeq datas;
	ReturnCode_t ret = RETCODE_OK;
	data_values.unloan();
	sample_infos.unloan();
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, condition->get_sample_state_mask(),
		condition->get_view_state_mask(), condition->get_instance_state_mask(), true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer,littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);

		if (buffer->origin_buffer) {
			MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
		}
	}
	return ret;
}

ReturnCode_t DataReaderImpl::read_next_sample(void* data_value, SampleInfo &sample_info)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
    ReturnCode_t ret = RETCODE_OK;
	SerializedBufferSeq datas;
	SampleInfoSeq sample_infos;
	ret = reader_->ReadOrTake(datas, sample_infos,1, NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
	if (RETCODE_OK == ret) {
		if (!sample_infos[0].valid_data) {
			TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
			return ret;
		}
		SerializedBuffer* buffer = &datas[0];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_value, &cdr, littleEndian);
		sample_info = sample_infos[0];
	}
	
    return ret;
}

ReturnCode_t DataReaderImpl::take_next_sample(void* data_value, SampleInfo &sample_info)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	ReturnCode_t ret = RETCODE_OK;
	SerializedBufferSeq datas;
	SampleInfoSeq sample_infos;
	ret = reader_->ReadOrTake(datas, sample_infos, 1, NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE,true);
	if (RETCODE_OK == ret) {
		if (!sample_infos[0].valid_data) {
			TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
			return ret;
		}
		SerializedBuffer* buffer = &datas[0];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer, littleEndian);
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
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeInstance(datas, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);
		
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeInstance(datas, sample_infos, max_samples, handle, sample_states, view_states, instance_states,true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTakeInstance ret = %d", ret);
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);

		if (buffer->origin_buffer) {
			MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
		}
	}
	return ret;
}

ReturnCode_t DataReaderImpl::read_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle, sample_states, view_states, instance_states);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle, sample_states, view_states, instance_states,true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);

		if (buffer->origin_buffer) {
			MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
		}
	}

	return ret;
}

ReturnCode_t DataReaderImpl::read_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, ReadCondition* condition)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle,
		condition->get_sample_state_mask(), condition->get_view_state_mask(), condition->get_instance_state_mask());
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer, littleEndian);
		typeSupport_->deserialize_data(data_values[i], &cdr, littleEndian);
	}

	return ret;
}

ReturnCode_t DataReaderImpl::take_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, ReadCondition* condition)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	SerializedBufferSeq  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle,
		condition->get_sample_state_mask(), condition->get_view_state_mask(), condition->get_instance_state_mask(), true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
		return ret;
	}

	data_values.length(datas.length());
	for (int32_t i = 0; i < datas.length(); i++) {
		if (!sample_infos[i].valid_data) {
			continue;
		}
		SerializedBuffer* buffer = &datas[i];
		bool littleEndian = GetEndianess(buffer);

		CdrDeserializer cdr(buffer, littleEndian);
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
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	if (!typeSupport_->has_key()) {
		TRAVODDS_LOG(LOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return RETCODE_ILLEGAL_OPERATION;
	}

	SerializedBuffer* cacheData = history_->get_key_value(handle);
	if (nullptr == cacheData) {
		TRAVODDS_LOG(LOG_ERROR, "cacheData is nullptr,RETCODE_BAD_PARAMETER");
		return RETCODE_BAD_PARAMETER;
	}

	CdrDeserializer cdr(cacheData);
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
		TRAVODDS_LOG(LOG_ERROR, "condition is nullptr,RETCODE_BAD_PARAMETER");
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
	TRAVODDS_LOG(LOG_ERROR, "datareder donot contain this condition");
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
	TRAVODDS_LOG(LOG_ERROR, "datareader donot support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DataReaderImpl::get_matched_publication_data(PublicationBuiltinTopicData &publication_data, const InstanceHandle_t &publication_handle) const
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	if (reader_->GetMatchedPublicationData(publication_data, publication_handle)) {
		return RETCODE_OK;
	}
	return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DataReaderImpl::get_matched_publications(InstanceHandleSeq &publication_handles) const
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
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

	/* wangyi lifespan Qos 定时器添加  */
	lifespanTimer_ = TimerManager::CreateTimer(
		TIME_INFINITE,
		[this] {
		LifespanTimeout();
		},
		"dr.lifespan"
	);

	domainParticipant->GetRelatedParticipant()->AddLocalReader(readerPtr);
	enable_ = true;

	//本地匹配完成后再发送SEPD报文，防止重复匹配
	DiscoveredReaderData readerData;
	readerData.ddsSubscriptionData = GetBuiltinTopicData();
	readerData.contentFilter = contentFilter_;
	std::shared_ptr<Discovery> discovery = domainParticipant->GetDiscovery();
	discovery->WriteSubscriptionData(readerData);
#ifdef DDSSECURITY
	//扩展了会发送DiscoveredReaderSecureData
	DiscoveredReaderSecureData readerSecureData;
	readerSecureData.ddsSubscriptionData = GetBuiltinTopicSecureData();
	discovery->WriteSubscriptionSecureData(readerSecureData);
#endif // DDSSECURITY
	return RETCODE_OK;
}

void DataReaderImpl::SetRelatedReader(std::shared_ptr<Reader> reader)
{
	reader_ = reader;
	reader->SetRelatedDataReader(this);
	handle_ = Conversion::ToInstanceHandle(reader->GetGuid());
	builtinData_.key = Conversion::ToBuiltinTopicKey(handle_);
#ifdef DDSSECURITY
	builtinSecureData_.key = Conversion::ToBuiltinTopicKey(handle_);
#endif // DDSSECURITY
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

#ifdef DDSSECURITY
const SubscriptionBuiltinTopicSecureData& DataReaderImpl::GetBuiltinTopicSecureData()
{
	return builtinSecureData_;
}

bool DataReaderImpl::GetSecAttributes()
{
	enableSec_ = false;
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(sub_->get_participant());
	if (dp->enableDDSSec_) {
		SecurityException secExce;
		GUID_t guid = Conversion::ToGuid(handle_);
		//participantstatestatelessmessageredaer的安全实体的reader 或者是用户自定义读者实体
		if (guid.IsSecureBuiltinEntity() && guid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER
			|| (guid.IsUserDefinedEntity() && guid.entityId != ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER))
		{
			// Access_Control 创建的数据读者若为内置安全相关实体或者非内置实体，需要获取安全属性
			EndpointSecurityAttributes secAttr;
			SubscriberQos qos;
			sub_->get_qos(qos);
			if (!dp->secPlugin_.access_control->get_datareader_sec_attributes(
				dp->permissionHdl_,
				builtinSecureData_.topic_name.c_str(),
				&qos.partition,
				NULL,
				&secAttr,
				&secExce)) {
				//取消初始化释放资源
				createSuccess_ = false;
				return false;;
			}
			secAttributes_.ac_endpoint_properties = secAttr.ac_endpoint_properties;
			secAttributes_.is_write_protected = secAttr.is_write_protected;
			secAttributes_.is_read_protected = secAttr.is_read_protected;
			secAttributes_.is_discovery_protected = secAttr.is_discovery_protected;
			secAttributes_.is_liveliness_protected = secAttr.is_liveliness_protected;
			secAttributes_.is_payload_protected = secAttr.is_payload_protected;
			//secAttributes_.is_key_protected = secAttr.is_key_protected;
			secAttributes_.is_submessage_protected = secAttr.is_submessage_protected;
			secAttributes_.plugin_endpoint_attributes = secAttr.plugin_endpoint_attributes;
			dp->secPlugin_.access_control->return_datawriter_sec_attributes(&secAttr, &secExce);
			if (guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) {
				rtpsMsgProtected_ = false;
			}
			else
			{
				rtpsMsgProtected_ = dp->secAttributes_.is_rtps_protected;
			}
		}
		else
		{
			// 若为内置实体，则不启用安全
			secAttributes_.is_write_protected = false;
			secAttributes_.is_read_protected = false;
			secAttributes_.is_discovery_protected = false;
			secAttributes_.is_liveliness_protected = false;
			secAttributes_.is_submessage_protected = false;
			secAttributes_.is_payload_protected = false;
			secAttributes_.is_key_protected = false;
			rtpsMsgProtected_ = false;
		}
		if (secAttributes_.is_write_protected
			|| secAttributes_.is_read_protected
			|| secAttributes_.is_discovery_protected
			|| secAttributes_.is_liveliness_protected
			|| secAttributes_.is_submessage_protected
			|| secAttributes_.is_payload_protected
			|| secAttributes_.is_key_protected)
		{
			if (dp->secPlugin_.crypto_key_factory == NULL)
			{
				TRAVODDS_LOG(LOG_ERROR, "Create DataReader failed: key factory not found.");
				createSuccess_ = false;
				return false;
			}
			if (guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) {
				unsigned int tmplen = secAttributes_.ac_endpoint_properties.size();
				Property_t tmpProperty;
				tmpProperty.name = "dds.sec.builtin_endpoint_name";
				tmpProperty.value = "BuiltinParticipantVolatileMessageSecureReader";
				secAttributes_.ac_endpoint_properties.push_back(tmpProperty);
			}
			cryptoHdl_ = dp->secPlugin_.crypto_key_factory->register_local_datareader(
				dp->cryptoHdl_,
				&secAttributes_.ac_endpoint_properties,
				&secExce);
			if (cryptoHdl_ == SEC_NIL_HANDLE)
			{
				createSuccess_ = false;
				return false;
			}
			if (!dp->secPlugin_.crypto_transform->get_datareader_submessage_encode_attributes(
				cryptoHdl_,
				&submsgEncodeAttr_))
			{
				createSuccess_ = false;
				return false;
			}
			enableSec_ = true;
			secPlugin_ = &(dp->secPlugin_);
		}
		else if (rtpsMsgProtected_)
		{
			enableSec_ = true;
			secPlugin_ = &dp->secPlugin_;
		}
		else
		{
			enableSec_ = false;
		}
	}
	createSuccess_ = false;
	return true;
}

int DataReaderImpl::SetRemoteWriterCryptoTokens(const GUID_t* writerId, const Sequence<CryptoToken>* tokens)
{
	GUID_t guid = reader_->GetGuid(); //ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER
	if (guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER)
	{
		return 0;
	}
	auto wrIter = preparingWriters_.find(*writerId);
	if (wrIter == preparingWriters_.end())
	{
		Sequence<CryptoToken>& tokenRef = remoteWriterCryptoToken_[*writerId];
		tokenRef = *tokens;
		return 0;
	}
	WriterSampleList* writer = wrIter->second;
	SecurityException secExce;
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(sub_->get_participant());
	if (!dp->secPlugin_.crypto_key_exchange->set_remote_datawriter_crypto_tokens(
		cryptoHdl_,
		writer->cryptoHdl,
		tokens,
		&secExce))
	{
		return -2;
	}
	writer->partCryptoHdl =
		GetRemoteParticipantCryptoHandle(dp->GetDiscovery()->GetSpdp().get(), &writerId->prefix);
	secWriters_.push_back(writer->cryptoHdl);
	writer->readyForSecComm = true;

	preparingWriters_.erase(wrIter);
	//DataReaderImpl存储远端写者的加密相关信息
	writerSampleList_[*writerId] = writer;
	StatefulReader* statefulReader = dynamic_cast<StatefulReader*>(reader_.get());
	if (statefulReader) {
		WriterProxyMap* writerMap = statefulReader->GetMatchedWriters();
		if (!writerMap->count(*writerId)) {
			return -1;
		}
		(*writerMap)[*writerId]->partCryptoHdl_ = writer->partCryptoHdl;
		(*writerMap)[*writerId]->cryptoHdl_ = writer->cryptoHdl;
		(*writerMap)[*writerId]->readyForSecComm_ = writer->readyForSecComm;
	}
	StatelessReader* statelessReader = dynamic_cast<StatelessReader*>(reader_.get());
	if (statelessReader) {
		std::map<GUID_t, std::shared_ptr<WriterProxy>>* writerMap = statelessReader->GetMatchWriters();
		if (!writerMap->count(*writerId)) {
			return -1;
		}
		(*writerMap)[*writerId]->partCryptoHdl_ = writer->partCryptoHdl;
		(*writerMap)[*writerId]->cryptoHdl_ = writer->cryptoHdl;
		(*writerMap)[*writerId]->readyForSecComm_ = writer->readyForSecComm;
	}
	return 0;
}

ParticipantCryptoHandle DataReaderImpl::GetRemoteParticipantCryptoHandle(Spdp* spdp, const GuidPrefix_t* remotePrefix)
{
	auto dpInfoIter = spdp->discParts_.find(*remotePrefix);
	if (dpInfoIter == spdp->discParts_.end())
	{
		return SEC_NIL_HANDLE;
	}
	if (dpInfoIter->second->enableDDSSec_)
	{
		return dpInfoIter->second->cryptoHdl_;
	}
	return SEC_NIL_HANDLE;
}

ReturnCode_t DataReaderImpl::FillSecureData()
{
	if (enableSec_) {
		builtinSecureData_.security_info.endpoint_security_attributes =
			EndpointSecurityAttributesToMask(&secAttributes_);
	}
	else
	{
		builtinSecureData_.security_info = ENDPOINT_SECURITY_ATTRIBUTES_INFO_DEFAULT;
	}
	builtinSecureData_.protected_discovery = secAttributes_.is_discovery_protected;
	return RETCODE_OK;
}


#endif // DDSSECURITY

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

	CdrSerializer cdr(&buffer);
	
	bool ret = typeSupport_->get_instancehandle((void*)data, &cdr, handle, true);
	MemoryPool::GetInstance()->releaseBuffer(buffer.origin_buffer);

	return ret;
}

void DataReaderImpl::SetLastinstanceHandle(const std::shared_ptr<CacheChange>& cache)
{
	/*  */
	// cache->kind;
	

	/* deadline */
	if (deadlineTimer_) {
		std::lock_guard<std::mutex> lock(lastInstanceMtx_);
		std::chrono::steady_clock::time_point nextTime = 
			std::chrono::steady_clock::now() + std::chrono::milliseconds(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
		history_->SetNextDeadline(cache->instanceHandle, nextTime);
		history_->GetNextDeadline(lastInstance_, nextTime);
		auto period = nextTime - std::chrono::steady_clock::now();
		deadlineTimer_->UpdatePeriod(std::chrono::duration_cast<std::chrono::milliseconds>(period).count());
	}

}

int32_t DataReaderImpl::GetInstanceLostcounts(const std::shared_ptr<CacheChange>& cache)
{
	GUID_t guid = reader_->GetGuid();
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind)
		return  0;
	return history_->GetInstanceLostcounts(cache);
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

bool DataReaderImpl::CheckLifespan(const std::shared_ptr<CacheChange>& cache)
{
	/* 非用户reader不触发判断 */
	GUID_t guid = reader_->GetGuid();
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind) {
		return  true;
	}

	PublicationBuiltinTopicData publicationData;
	if (!reader_->GetMatchedPublicationData(publicationData, Conversion::ToInstanceHandle(cache->writerGuid))) {
		return false;
	}
	 
	if (publicationData.lifespan.duration == Duration_t(TIME_INFINITE))
	{
		return true;
	}

	Duration_t expirationTime = Conversion::TimeToDuration(cache->sourceTimespace) + publicationData.lifespan.duration;
	Duration_t currentTime;
	Duration_t::now(currentTime);

	//如果新报文到达得太晚，则它可能已过期。
	//如果已过期，将其从历史记录中删除，并返回false，以避免通知监听器。
	if (expirationTime < currentTime)
	{
		reader_->GetReaderCache()->RemoveChange(cache->sequenceNumber);
		return false;
	}

	/* wangyi TODO 定时器停止后不再启动，待查 */
	//if (reader_->GetReaderCache()->GetMinSourceTimespace().get() == cache.get()) {
	//	lifespanTimer_->Stop();
	//}

	Duration_t interval = expirationTime - currentTime;
	lifespanTimer_->UpdatePeriod(interval.ToNs() * 1e-6);
	lifespanTimer_->ReStart();
	return true;
}

bool DataReaderImpl::LifespanTimeout()
{
	Duration_t currentTime;
	Duration_t::now(currentTime);
	std::shared_ptr<CacheChange> earliestChange = reader_->GetReaderCache()->GetMinSourceTimespace();
	while (earliestChange != nullptr) {

		PublicationBuiltinTopicData publicationData;
		reader_->GetMatchedPublicationData(publicationData, Conversion::ToInstanceHandle(earliestChange->writerGuid));

		if (publicationData.lifespan.duration == Duration_t(TIME_INFINITE))
		{
			break;
		}

		Duration_t expirationTime = Conversion::TimeToDuration(earliestChange->sourceTimespace) + publicationData.lifespan.duration;

		/* 检查最早的报文缓存是否已过期（启动计时器的报文缓存可能已从历史记录中删除）*/
		if (currentTime < expirationTime)
		{
			Duration_t interval = expirationTime - currentTime;
			lifespanTimer_->UpdatePeriod(interval.ToNs() * 1e-6);
			return true;
		}
		else {
			// 最早的报文缓存已过期，删除该报文。
			//reader_->GetReaderCache()->RemoveChange(earliestChange->sequenceNumber);
			//earliestChange = reader_->GetReaderCache()->GetMinSourceTimespace();
			break;
		}
	}
	return false;
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

SubscriberImpl* DataReaderImpl::getParent()
{
	return sub_;
}
