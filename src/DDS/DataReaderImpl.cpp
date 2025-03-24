#include "DDS/DataReaderImpl.h"
#include "DDS/SubscriberImpl.h"
#include "DDS/TopicDescriptionImpl.h"
#include "DDS/TypeSupport.h"
#include "RTPS/HistoryCache.h"
#include "RTPS/Reader.h"
#include "QoS/QosPolicyCheck.h"
#include "Condition/ReadConditionImpl.h"
#include "Condition/QueryConditionImpl.h"
#include "Status/StatusManager.h"
#include <iostream>
USING_AGIDDS_NAMESPACE;

ReturnCode_t DataReaderImpl::delete_readcondition(ReadCondition* a_condition)
{
    if (a_condition == nullptr) {
        return RETCODE_BAD_PARAMETER;
    }
    for (auto it = readcdts_.begin(); it != readcdts_.end(); it++) {
        if (it->get() == a_condition) {
			readcdts_.erase(it);
            return RETCODE_OK;
        }
    }
    return RETCODE_BAD_PARAMETER;
}

DataReaderImpl::DataReaderImpl(TopicDescription* topic, const DataReaderQos& qos, DataReaderListener* a_listener, StatusMask mask, SubscriberImpl* a_sub, TypeSupport* typeSupport)
    : DomainEntityImpl(mask), topic_(dynamic_cast<TopicDescriptionImpl*>(topic)), 
	  qos_(qos), listener_(a_listener), sub_(a_sub), typeSupport_(typeSupport),
	  statusMgr_(new DataReaderStatusManager(this))
{
	topic_->Reference();
}

DataReaderImpl::~DataReaderImpl()
{
	topic_->Dereference();
	readcdts_.clear();
	querycdts_.clear();
	if (statusMgr_) {
		delete statusMgr_;
		statusMgr_ = nullptr;
	}
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

	qos_ = qosTemp;

    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::get_qos(DataReaderQos& qos) const
{
    qos = qos_;
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::set_listener(DataReaderListener* a_listener, const StatusMask& mask)
{
    if (a_listener) {
        listener_ = a_listener;
        statusMask_ = mask;
        return RETCODE_OK;
    }
    return RETCODE_BAD_PARAMETER;
}

const DataReaderListener * DataReaderImpl::get_listener() const
{
    return listener_;
}

ReturnCode_t DataReaderImpl::read(DataSeq & data_values, SampleInfoSeq & sample_infos, int max_samples, SampleStateMask sample_states,
	ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq datas;
	int ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, sample_states, view_states, instance_states);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int i = 0; i < datas.length(); i++) {
		typeSupport_->deserialize_data(&datas[i], data_values[i],1);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take(DataSeq & data_values, SampleInfoSeq & sample_infos, int max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq datas;
	int ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, sample_states, view_states, instance_states, true);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int i = 0; i < datas.length(); i++) {
		typeSupport_->deserialize_data(&datas[i], data_values[i],1);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::read_w_condition(DataSeq & data_values, SampleInfoSeq & sample_infos, long max_samples, ReadCondition * a_condition)
{
	SerializedBufferSeq datas;
	int ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, a_condition->get_sample_state_mask(),
		a_condition->get_view_state_mask(), a_condition->get_instance_state_mask());
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int i = 0; i < datas.length(); i++) {
		typeSupport_->deserialize_data(&datas[i], data_values[i],1);
	}
	return ret;

}

ReturnCode_t DataReaderImpl::take_w_condition(DataSeq & data_values, SampleInfoSeq & sample_infos, long max_samples, ReadCondition * a_condition)
{
	SerializedBufferSeq datas;
	int ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, a_condition->get_sample_state_mask(),
		a_condition->get_view_state_mask(), a_condition->get_instance_state_mask(), true);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int i = 0; i < datas.length(); i++) {
		typeSupport_->deserialize_data(&datas[i], data_values[i],1);
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
		typeSupport_->deserialize_data(&datas[0], data_value, 1);
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
		typeSupport_->deserialize_data(&datas[0], data_value, 1);
	}

	return ret;
}

ReturnCode_t DataReaderImpl::read_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& a_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq  datas;
	int ret = RETCODE_OK;
	ret = reader_->ReadOrTakeInstance(datas, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int i = 0; i < datas.length(); i++) {
		typeSupport_->deserialize_data(&datas[i], data_values[i], 1);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& a_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq  datas;
	int ret = RETCODE_OK;
	ret = reader_->ReadOrTakeInstance(datas, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states,true);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int i = 0; i < datas.length(); i++) {
		 typeSupport_->deserialize_data(&datas[i], data_values[i], 1);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::read_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq  datas;
	int ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle, sample_states, view_states, instance_states);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int i = 0; i < datas.length(); i++) {
		typeSupport_->deserialize_data(&datas[i], data_values[i], 1);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::take_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	SerializedBufferSeq  datas;
	int ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle, sample_states, view_states, instance_states,true);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int i = 0; i < datas.length(); i++) {
		typeSupport_->deserialize_data(&datas[i], data_values[i], 1);
	}

	return ret;
}

ReturnCode_t DataReaderImpl::read_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, ReadCondition * a_condition)
{
	SerializedBufferSeq  datas;
	int ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle,
		a_condition->get_sample_state_mask(), a_condition->get_view_state_mask(), a_condition->get_instance_state_mask());
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int i = 0; i < datas.length(); i++) {
		ret = typeSupport_->deserialize_data(&datas[i], data_values[i], 1);
	}

	return ret;
}

ReturnCode_t DataReaderImpl::take_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, ReadCondition * a_condition)
{
	SerializedBufferSeq  datas;
	int ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle,
		a_condition->get_sample_state_mask(), a_condition->get_view_state_mask(), a_condition->get_instance_state_mask(), true);
	if (RETCODE_OK != ret) {
		return ret;
	}

	data_values.length(datas.length());
	for (int i = 0; i < datas.length(); i++) {
		typeSupport_->deserialize_data(&datas[i], data_values[i], 1);
	}
	return ret;
}

ReturnCode_t DataReaderImpl::return_loan(DataSeq& data_values, SampleInfoSeq& sample_infos)
{
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DataReaderImpl::get_key_value(void* key_holder, const InstanceHandle_t& handle)
{
	if (!typeSupport_->IsKey()) {
		std::cout << "Topic is NO_KEY, operation not permitted" << std::endl;
		return RETCODE_ILLEGAL_OPERATION;
	}

	SerializedBuffer* cacheData = history_->get_key_value(handle);
	if (nullptr == cacheData) {
		return RETCODE_BAD_PARAMETER;
	}

	typeSupport_->deserialize_data(cacheData, key_holder, 1);
	return RETCODE_OK;
}

InstanceHandle_t DataReaderImpl::lookup_instance(const void* instance_data) const
{
	InstanceHandle_t handle = HANDLE_NIL;

	if (typeSupport_->IsKey()) {
		if (typeSupport_->MakeKey(instance_data, handle, false)) {
			if (!history_->IsInstancePresent(handle)) {
				handle = HANDLE_NIL;
			}
		}
	}
	return handle;
}

ReadCondition * DataReaderImpl::create_readcondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
	ReadCondition* readcdt = nullptr;
    std::shared_ptr<ReadCondition> readcdtPtr = std::make_shared<ReadConditionImpl>(smaple_states, view_states, instance_states,this);
    if (readcdtPtr) {
        readcdts_.push_back(readcdtPtr);
		readcdt = readcdtPtr.get();
    }
    return readcdt;
}

QueryCondition* DataReaderImpl::create_querycondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states, const string & query_expression, const StringSeq& query_parameters)
{
	QueryCondition* querycdt = nullptr;
	std::shared_ptr<QueryCondition> querycdtPtr = std::make_shared<QueryConditionImpl>(smaple_states, view_states, instance_states, query_expression, query_parameters, this);
	if (querycdtPtr) {
		querycdts_.push_back(querycdtPtr);
		querycdt = querycdtPtr.get();
	}

	return querycdt;
}

ReturnCode_t DataReaderImpl::get_liveliness_changed_status(LivelinessChangedStatus &status) const
{
    return statusMgr_->get_liveliness_changed_status(status);
}

ReturnCode_t DataReaderImpl::get_requested_deadline_missed_status(RequestedDeadlineMissedStatus &status)
{
	return statusMgr_->get_requested_deadline_missed_status(status);
}

ReturnCode_t DataReaderImpl::get_requested_incompatible_qos_status(RequestedIncompatibleQosStatus &status)
{
	return statusMgr_->get_requested_incompatible_qos_status(status);
}

ReturnCode_t DataReaderImpl::get_sample_lost_status(SampleLostStatus &status) const
{
	return statusMgr_->get_sample_lost_status(status);
}

ReturnCode_t DataReaderImpl::get_sample_rejected_status(SampleRejectedStatus &status) const
{
	return statusMgr_->get_sample_rejected_status(status);
}

ReturnCode_t DataReaderImpl::get_subscription_matched_status(SubscriptionMatchedStatus &status) const
{
    return statusMgr_->get_subscription_matched_status(status);;
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
    return RETCODE_UNSUPPORTED;
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

	enable_ = true;
	return RETCODE_OK;
}

bool DataReaderImpl::Empty() const
{
	return readcdts_.empty() && querycdts_.empty();
}

