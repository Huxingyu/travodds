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
#include "Listener/SubscriberListener.h"

AGIDDS_NAMESPACE_BEGIN
class DomainParticipant;

class DDS_DLL Subscriber : public virtual DomainEntity {
public:
    virtual ~Subscriber() = default;

	virtual ReturnCode_t set_qos(const SubscriberQos& qos) = 0;

	virtual ReturnCode_t get_qos(SubscriberQos& qos) const = 0;

	virtual ReturnCode_t set_listener(SubscriberListener* a_listener, const StatusMask& mask) = 0;

	virtual const SubscriberListener* get_listener() const = 0;

    virtual DataReader* create_datareader(TopicDescription* a_topic, const DataReaderQos& qos, DataReaderListener* a_listener, const StatusMask& mask) = 0;

    virtual ReturnCode_t delete_datareader(DataReader*& a_dataReader) = 0;

    virtual DataReader* lookup_datareader(const string& topic_name) const = 0;

    virtual ReturnCode_t begin_access() = 0;

    virtual ReturnCode_t end_access() = 0;

    virtual ReturnCode_t get_datareaders(DataReaderSeq& readers, const SampleStateMask& sample_states, const ViewStateMask& view_states, const InstanceStateMask& instance_states) const = 0;

    virtual ReturnCode_t notify_datareaders() const = 0;

    virtual ReturnCode_t get_sample_lost_status(SampleLostStatus& status) const = 0;

    virtual const DomainParticipant* get_participant() const = 0;

    virtual ReturnCode_t delete_contained_entities() = 0;

    virtual ReturnCode_t set_default_datareader_qos(const DataReaderQos& Qos) = 0;

    virtual ReturnCode_t get_default_datareader_qos(DataReaderQos& Qos) const = 0;

    static ReturnCode_t copy_from_topic_qos(DataReaderQos& datareader_qos, const TopicQos& topic_qos);
};

AGIDDS_NAMESPACE_END
