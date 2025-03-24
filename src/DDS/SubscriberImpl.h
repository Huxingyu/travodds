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
#include "Subscriber.h"
#include "DomainEntityImpl.h"
#include "DataReaderImpl.h"
AGIDDS_NAMESPACE_BEGIN
class DomainParticipantImpl;
class SubscriberImpl:public virtual Subscriber,public DomainEntityImpl {
public:
    SubscriberImpl();
    SubscriberImpl(const SubscriberQos& qos, SubscriberListener* a_listener, StatusMask mask, DomainParticipantImpl* a_participant);
    ~SubscriberImpl();

    ReturnCode_t set_qos(const SubscriberQos& qos);

    ReturnCode_t get_qos(SubscriberQos& qos) const;

    ReturnCode_t set_listener(SubscriberListener* a_listener, const StatusMask& mask);

    const SubscriberListener* get_listener() const;

	DataReader* create_datareader(TopicDescription* a_topic, const DataReaderQos& qos, DataReaderListener* a_listener, const StatusMask& mask);

    ReturnCode_t delete_datareader(DataReader*& a_dataReader);

    DataReader* lookup_datareader(const string& topic_name) const;

    ReturnCode_t begin_access();

    ReturnCode_t end_access();

    ReturnCode_t get_datareaders(DataReaderSeq& readers, const SampleStateMask& sample_states, const ViewStateMask& view_states, const InstanceStateMask& instance_states) const;

    ReturnCode_t notify_datareaders() const;

    ReturnCode_t get_sample_lost_status(SampleLostStatus& status) const;

    const DomainParticipant* get_participant() const;

    ReturnCode_t delete_contained_entities();

    ReturnCode_t set_default_datareader_qos(const DataReaderQos& qos);

    ReturnCode_t get_default_datareader_qos(DataReaderQos& qos) const;

    //ReturnCode_t copy_from_topic_qos(DataReaderQos& a_datareader_qos, const TopicQos& a_topic_qos);

public:
	ReturnCode_t enable();

    bool contains_entity(const InstanceHandle_t& a_handle) const;

    bool Empty() const;
private:
    SubscriberQos qos_;
    SubscriberListener* listener_ = nullptr;
    std::map<InstanceHandle_t, std::unique_ptr<DataReaderImpl>> readerHandles_;
	mutable std::mutex mtxReaders_;
	DomainParticipantImpl* participant_;
    DataReaderQos defaultDatareaderQos_;

};
AGIDDS_NAMESPACE_END
