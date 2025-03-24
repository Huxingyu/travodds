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
#pragma once
#include "DDS/DataReader.h"
#include "DDS/DomainEntityImpl.h"
#include "RTPS/BasicTypeRTPS.h"

#include <memory>
AGIDDS_NAMESPACE_BEGIN

class Reader;
class SubscriberImpl;
class TopicDescriptionImpl;
class TypeSupport;
class HistoryCache;
class DataReaderStatusManager;

class DataReaderImpl : public virtual DataReader, public DomainEntityImpl {
public:
	DataReaderImpl(TopicDescription* topic, const DataReaderQos& qos, DataReaderListener* a_listener, StatusMask mask, SubscriberImpl* a_sub,TypeSupport* typeSupport);

	~DataReaderImpl();

	virtual ReturnCode_t set_qos(const DataReaderQos& qos);

	virtual ReturnCode_t get_qos(DataReaderQos& qos) const;

	virtual ReturnCode_t set_listener(DataReaderListener* a_listener, const StatusMask& mask);

	virtual const DataReaderListener* get_listener() const;

	virtual ReturnCode_t read(DataSeq& data_values, SampleInfoSeq& sample_infos, int max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t take(DataSeq& data_values, SampleInfoSeq& sample_infos, int max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t read_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, ReadCondition* a_condition);

	virtual ReturnCode_t take_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, ReadCondition* a_condition);

	virtual ReturnCode_t read_next_sample(void* data_value, SampleInfo& sample_info);

	virtual ReturnCode_t take_next_sample(void* data_value, SampleInfo& sample_info);

	virtual ReturnCode_t read_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& a_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t take_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& a_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t read_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t take_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states);

	virtual ReturnCode_t read_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, ReadCondition* a_condition);

	virtual ReturnCode_t take_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, ReadCondition* a_condition);

	virtual ReturnCode_t return_loan(DataSeq& data_values, SampleInfoSeq& sample_infos);

	virtual ReturnCode_t get_key_value(void* key_holder, const InstanceHandle_t& handle);

	virtual InstanceHandle_t lookup_instance(const void* instance_data) const;

	virtual ReadCondition* create_readcondition(SampleStateMask smaple_states, ViewStateMask  view_states, InstanceStateMask  instance_states);

	virtual QueryCondition* create_querycondition(SampleStateMask smaple_states, ViewStateMask  view_states, InstanceStateMask  instance_states, const string& query_expression, const StringSeq& query_parameters);

	virtual ReturnCode_t delete_readcondition(ReadCondition* a_condition);

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
public:
	bool Empty() const;

private:
	std::vector<std::shared_ptr<ReadCondition>> readcdts_;
	std::vector<std::shared_ptr<QueryCondition>> querycdts_;
	TopicDescriptionImpl* topic_ = nullptr;
	DataReaderQos qos_;
	DataReaderListener* listener_ = nullptr;
	Reader* reader_ = nullptr;
	SubscriberImpl* sub_ = nullptr;

	DataReaderStatusManager* statusMgr_;
protected:
	TypeSupport* typeSupport_ = nullptr;
	HistoryCache* history_ = nullptr;

};
AGIDDS_NAMESPACE_END
