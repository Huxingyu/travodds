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
#include "Publisher.h"
#include "DomainEntityImpl.h"
#include "DataWriterImpl.h"
AGIDDS_NAMESPACE_BEGIN

class DomainParticipantImpl;

class PublisherImpl : public virtual Publisher,public  DomainEntityImpl {
public:
    PublisherImpl(PublisherQos qos, PublisherListener* a_listener, StatusMask mask, DomainParticipantImpl* a_participant);

    ~PublisherImpl();
       
    ReturnCode_t set_qos(const PublisherQos& qos);

    ReturnCode_t get_qos(PublisherQos& qos) const;

    ReturnCode_t set_listener(PublisherListener* a_listener, const StatusMask& mask) ;

    const PublisherListener* get_listener() const;

    DataWriter* create_datawriter(Topic* a_topic, const DataWriterQos& qos, DataWriterListener * a_listener, const StatusMask& mask);

    ReturnCode_t delete_datawriter(DataWriter*& a_datawriter);

	DataWriter*  lookup_datawriter(const string& topic_name) const;

    ReturnCode_t suspend_publications();

    ReturnCode_t resume_publications();

    ReturnCode_t begin_coherent_changes();

    ReturnCode_t end_coherent_changes();

    ReturnCode_t wait_for_acknowledgments(const Duration_t& max_wait);

    const DomainParticipant* get_participant() const;

    ReturnCode_t delete_contained_entities();

    ReturnCode_t set_default_datawriter_qos(const DataWriterQos& qos);

    ReturnCode_t get_default_datawriter_qos(DataWriterQos& qos) const;

    //ReturnCode_t copy_from_topic_qos(DataWriterQos& a_datawriter_qos, const TopicQos& a_topic_qos);

public:
	ReturnCode_t enable();

    bool contains_entity(const InstanceHandle_t& a_handle) const;

    bool Empty() const;
private:
    PublisherQos qos_;
    PublisherListener* listener_ = nullptr;
    std::map<InstanceHandle_t, std::unique_ptr<DataWriterImpl>> writerHandles_;
	mutable std::mutex mtxWriters_;
	DomainParticipantImpl* participant_ = nullptr;
	DataWriterQos defaultDatawriterQos_;

};
AGIDDS_NAMESPACE_END

