//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef AGIDDS_DCPS_SUBSCRIBE_DATAREADERIMPL_H
#define AGIDDS_DCPS_SUBSCRIBE_DATAREADERIMPL_H

#include "dcps/subscribe/datareader.h"
#include "dcps/domain/domainentityimpl.h"
#include "rtps/basictypertps.h"
#include "rtps/historycache/historycache.h"

class SerializedBuffer;

AGIDDS_NAMESPACE_BEGIN

class Reader;
class SubscriberImpl;
class TopicDescriptionImpl;
class TypeSupport;
class DataReaderStatusManager;
class Timer;
class ReadConditionImpl;
class QueryConditionImpl;

class DataReaderImpl : public virtual DataReader, public DomainEntityImpl {
public:
	DataReaderImpl(TopicDescription* topic, const DataReaderQos& qos, DataReaderListener* listener, StatusMask mask, SubscriberImpl* sub, TypeSupport* typeSupport);

	~DataReaderImpl();

	virtual ReturnCode_t set_qos(const DataReaderQos& qos);

	virtual ReturnCode_t get_qos(DataReaderQos& qos) const;

	virtual ReturnCode_t set_listener(DataReaderListener* listener, const StatusMask& mask);

	virtual const DataReaderListener* get_listener() const;

	virtual ReturnCode_t read(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t take(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t read_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, ReadCondition* condition);

	virtual ReturnCode_t take_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, ReadCondition* condition);

	virtual ReturnCode_t read_next_sample(void* data_value, SampleInfo& sample_info);

	virtual ReturnCode_t take_next_sample(void* data_value, SampleInfo& sample_info);

	virtual ReturnCode_t read_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t take_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t read_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t take_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t read_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, ReadCondition* condition);

	virtual ReturnCode_t take_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, ReadCondition* condition);

	virtual ReturnCode_t return_loan(DataSeq& data_values, SampleInfoSeq& sample_infos);

	virtual ReturnCode_t get_key_value(void* key_holder, const InstanceHandle_t& handle);

	virtual InstanceHandle_t lookup_instance(const void* instance_data) const;

	virtual ReadCondition* create_readcondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual QueryCondition* create_querycondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states, const std::string& query_expression, const StringSeq& query_parameters);

	virtual ReturnCode_t delete_readcondition(ReadCondition* condition);

	virtual ReturnCode_t get_liveliness_changed_status(LivelinessChangedStatus& status) const;

	virtual ReturnCode_t get_requested_deadline_missed_status(RequestedDeadlineMissedStatus& status);

	virtual ReturnCode_t get_requested_incompatible_qos_status(RequestedIncompatibleQosStatus& status);

	virtual ReturnCode_t get_sample_lost_status(SampleLostStatus& status) const;

	virtual ReturnCode_t get_sample_rejected_status(SampleRejectedStatus& status) const;

	virtual ReturnCode_t get_subscription_matched_status(SubscriptionMatchedStatus& status) const;

	virtual const TopicDescription* get_topicdescription() const;

	virtual const Subscriber* get_subscriber() const;

	virtual ReturnCode_t delete_contained_entities();

	virtual ReturnCode_t wait_for_historical_data(const Duration_t& max_wait) const;

	virtual ReturnCode_t get_matched_publication_data(PublicationBuiltinTopicData& publication_data, const InstanceHandle_t& publication_handle) const;

	virtual ReturnCode_t get_matched_publications(InstanceHandleSeq& publication_handles) const;

	virtual ReturnCode_t enable();
#ifdef _XML_INTERFACE_CPP

	virtual ReturnCode_t set_qos_with_profile(
		const char* library_name,
		const char* profile_name,
		const char* qos_name);

#endif // _XML_INTERFACE_CPP
public:

	void SetRelatedReader(std::shared_ptr<Reader> reader);

	bool Empty() const;

	ReturnCode_t OnDataAvaible();

	ReturnCode_t OnSampleLost(int32_t lostCount);

	ReturnCode_t OnSampleRejected(const InstanceHandle_t& handle, const SampleRejectedStatusKind& reason);

	void OnLivelinessChanged(DataReader* the_reader, const LivelinessChangedStatus& status);

	void OnRequestedDeadlineMissed(DataReader* the_reader, const RequestedDeadlineMissedStatus& status);

	//void OnRequestedIncompatibleQos(DataReader* the_reader, const RequestedIncompatibleQosStatus& status);
	void OnRequestedIncompatibleQos(std::set<QosPolicyId_t>& qosSeq);

	void OnSubscriptionMatched(InstanceHandle_t& handle, const bool& add = true);

	void OnSampleLost(DataReader* the_reader, const SampleLostStatus& status);

	const SubscriptionBuiltinTopicData& GetBuiltinTopicData();

	//bool GetEndianess(SerializedBuffer* buffer);
	bool GetEndianess(SerializedBuffer* buffer);
	
	bool MakeKey(const void*  data, InstanceHandle_t& handle, bool forceMd5) const;


	/* 最后接收的样本实例 */
	void SetLastinstanceHandle(const std::shared_ptr<CacheChange>& cache);

	void DeadlineTimeout();

	/* wangyi   更新LIVELINESS_CHANGED通信状态
	 * bool ensure : 明确节点失活（通知了），或者先前明确失活了，现又成功活了为  true，否则 false
	 */
	ReturnCode_t UpdateLivelinessChangedStatus(const InstanceHandle_t& handle,const bool& alive,const bool& ensure = true);

	void SetStatus(bool add);

	SampleCounters GetSampleCounters();

	void NotifyReadConditions();

private:
	std::vector<std::shared_ptr<ReadConditionImpl>> readcdts_;
	std::vector<std::shared_ptr<QueryConditionImpl>> querycdts_;
	TopicDescriptionImpl* topic_ = nullptr;
	DataReaderQos qos_;
	DataReaderListener* listener_ = nullptr;
	std::shared_ptr<Reader> reader_ = nullptr;
	SubscriberImpl* sub_ = nullptr;
	std::unique_ptr<DataReaderStatusManager> statusMgr_;
	std::shared_ptr<Timer> deadlineTimer_;
	/* 当前deadline计时器所属实例 */
	InstanceHandle_t lastInstance_;
	mutable std::mutex lastInstanceMtx_;
	SubscriptionBuiltinTopicData builtinData_;
protected:
	TypeSupport* typeSupport_ = nullptr;
    std::shared_ptr<HistoryCache> history_;

};
AGIDDS_NAMESPACE_END

#endif // !DATA_READER_IMPL_H
