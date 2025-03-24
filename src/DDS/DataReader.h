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

#include "BuiltinTopicData.h"
#include "Listener/DataReaderListener.h"
#include "DDS/DomainEntity.h"
#include "DDS/TopicDescription.h"
#include "Condition/ReadCondition.h"
#include "Condition/QueryCondition.h"

AGIDDS_NAMESPACE_BEGIN

class Subscriber;

typedef SequenceBase DataSeq;

class DDS_DLL DataReader : public virtual  DomainEntity {
public:
    virtual ~DataReader() = default;

	virtual ReturnCode_t set_qos(const DataReaderQos & qos) = 0;

	virtual ReturnCode_t get_qos(DataReaderQos & qos) const = 0;

	virtual ReturnCode_t set_listener(DataReaderListener* a_listener, const StatusMask& mask) = 0;

	virtual const DataReaderListener* get_listener() const = 0;

    virtual ReturnCode_t read(DataSeq& data_values, SampleInfoSeq& sample_infos, int max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states) = 0;

    virtual ReturnCode_t take(DataSeq& data_values, SampleInfoSeq& sample_infos, int max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states) = 0;

	virtual ReturnCode_t read_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples,  ReadCondition* readcondition) = 0;

    virtual ReturnCode_t take_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples,  ReadCondition* readcondition) = 0;

    virtual ReturnCode_t read_next_sample(void* data_value, SampleInfo& sample_info) = 0;

    virtual ReturnCode_t take_next_sample(void* data_value, SampleInfo& sample_info) = 0;

    virtual ReturnCode_t read_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& handle, SampleStateMask sample_states, ViewStateMask samole_states, InstanceStateMask instance_states) = 0;

    virtual ReturnCode_t take_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& handle, SampleStateMask sample_states, ViewStateMask samole_states, InstanceStateMask instance_states) = 0;

    virtual ReturnCode_t read_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask samole_states, InstanceStateMask instance_states) = 0;

    virtual ReturnCode_t take_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples,  const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask samole_states, InstanceStateMask instance_states) = 0;

    virtual ReturnCode_t read_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, ReadCondition* pReadCondition) = 0;

    virtual ReturnCode_t take_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, long max_samples, const InstanceHandle_t& previous_handle, ReadCondition* pReadCondition) = 0;

    virtual ReturnCode_t return_loan(DataSeq& data_values, SampleInfoSeq& sample_infos) = 0;

	virtual ReturnCode_t get_key_value(void* key_holder, const InstanceHandle_t& handle) = 0;

	virtual InstanceHandle_t lookup_instance(const void* instance) const = 0;

	virtual ReadCondition* create_readcondition(SampleStateMask smaple_states, ViewStateMask  view_states, InstanceStateMask  instance_states) = 0;

	virtual QueryCondition* create_querycondition(SampleStateMask smaple_states, ViewStateMask  view_states, InstanceStateMask  instance_states, const string& query_expression, const StringSeq& query_parameters) = 0;

	virtual ReturnCode_t delete_readcondition(ReadCondition* condition) = 0;

    virtual ReturnCode_t get_liveliness_changed_status(LivelinessChangedStatus& status) const = 0;

    virtual ReturnCode_t get_requested_deadline_missed_status(RequestedDeadlineMissedStatus& status) = 0;

    virtual ReturnCode_t get_requested_incompatible_qos_status(RequestedIncompatibleQosStatus& status) = 0;

    virtual ReturnCode_t get_sample_lost_status(SampleLostStatus& status) const = 0;

    virtual ReturnCode_t get_sample_rejected_status(SampleRejectedStatus& status) const = 0;

    virtual ReturnCode_t get_subscription_matched_status(SubscriptionMatchedStatus& status) const = 0;

    virtual const TopicDescription* get_topicdescription() const = 0;

    virtual const Subscriber* get_subscriber() const = 0;

    virtual ReturnCode_t delete_contained_entities() = 0;

    virtual ReturnCode_t wait_for_historical_data(const Duration_t& max_wait) const = 0;

    virtual ReturnCode_t get_matched_publication_data(PublicationBuiltinTopicData& publication_data, const InstanceHandle_t& publication_handle) const = 0;

    virtual ReturnCode_t get_matched_publications(InstanceHandleSeq& publication_handles) const = 0;

};

using DataReaderSeq = Sequence<DataReader*>;

AGIDDS_NAMESPACE_END

