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
#ifndef AGIDDS_DCPS_SUBSCRIBE_SUBSCRIBERIMPL_H
#define AGIDDS_DCPS_SUBSCRIBE_SUBSCRIBERIMPL_H

#include "dcps/domain/domainentityimpl.h"
#include "dcps/subscribe/subscriber.h"
#include "dcps/subscribe/datareaderimpl.h"

AGIDDS_NAMESPACE_BEGIN
class DomainParticipantImpl;
class SubscriberImpl : public virtual Subscriber, public DomainEntityImpl {
public:
    SubscriberImpl(const SubscriberQos& qos, SubscriberListener* listener, StatusMask mask, DomainParticipantImpl* participant);
    ~SubscriberImpl();

    ReturnCode_t set_qos(const SubscriberQos& qos);

    ReturnCode_t get_qos(SubscriberQos& qos) const;

    ReturnCode_t set_listener(SubscriberListener* listener, const StatusMask& mask);

    const SubscriberListener* get_listener() const;

	DataReader* create_datareader(TopicDescription* topic, const DataReaderQos& qos, DataReaderListener* listener, const StatusMask& mask);

    ReturnCode_t delete_datareader(DataReader*& dataReader);

    DataReader* lookup_datareader(const std::string& topic_name) const;

    ReturnCode_t begin_access();

    ReturnCode_t end_access();

    ReturnCode_t get_datareaders(DataReaderSeq& readers, const SampleStateMask& sample_states, const ViewStateMask& view_states, const InstanceStateMask& instance_states) const;

    ReturnCode_t notify_datareaders() const;

    ReturnCode_t get_sample_lost_status(SampleLostStatus& status) const;

    const DomainParticipant* get_participant() const;

    ReturnCode_t delete_contained_entities();

    ReturnCode_t set_default_datareader_qos(const DataReaderQos& qos);

    ReturnCode_t get_default_datareader_qos(DataReaderQos& qos) const;


#ifdef _XML_INTERFACE_CPP

    virtual DataReader* create_datareader_with_qos_profile(
        TopicDescription* topic,
        const char* library_name,
        const char* profile_name,
        const char* qos_name,
        DataReaderListener* listener,
        StatusMask mask);

    virtual ReturnCode_t set_default_datareader_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual ReturnCode_t set_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

#endif
public:
	ReturnCode_t enable();

    bool contains_entity(const InstanceHandle_t& handle) const;

    bool Empty() const;

    SubscriberListener* GetListener();

private:
    SubscriberQos qos_;
    SubscriberListener* listener_ = nullptr;
    std::map<InstanceHandle_t, std::unique_ptr<DataReaderImpl>> readerHandles_;
	mutable std::mutex mtxReaders_;
	DomainParticipantImpl* participant_;
    DataReaderQos defaultDatareaderQos_;

};
AGIDDS_NAMESPACE_END
#endif // ! AGIDDS_DCPS_SUBSCRIBE_SUBSCRIBERIMPL_H
