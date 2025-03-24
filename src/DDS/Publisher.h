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

#include "DDS/DataWriter.h"
#include "Listener/PublisherListener.h"

AGIDDS_NAMESPACE_BEGIN

class DomainParticipant;

class DDS_DLL Publisher : public DomainEntity {
public:
    virtual ~Publisher() = default;

	virtual ReturnCode_t set_qos(const PublisherQos& qos) = 0;

	virtual ReturnCode_t get_qos(PublisherQos& qos) const = 0;

	virtual ReturnCode_t set_listener(PublisherListener* a_listener, const StatusMask& mask) = 0;

	virtual const PublisherListener* get_listener() const = 0;

    virtual	DataWriter* create_datawriter(Topic* a_topic, const DataWriterQos& qos, DataWriterListener * a_listener, const StatusMask& mask) = 0;

    virtual	ReturnCode_t delete_datawriter(DataWriter*& a_datawriter) = 0;

    virtual	DataWriter*  lookup_datawriter(const string& topic_name) const = 0;

    virtual	ReturnCode_t suspend_publications() = 0;

    virtual	ReturnCode_t resume_publications() = 0;

    virtual	ReturnCode_t begin_coherent_changes() = 0;

    virtual	ReturnCode_t end_coherent_changes() = 0;

    virtual	ReturnCode_t wait_for_acknowledgments(const Duration_t& max_wait) = 0;

    virtual	const DomainParticipant* get_participant() const = 0;

    virtual	ReturnCode_t delete_contained_entities() = 0;

    virtual	ReturnCode_t set_default_datawriter_qos(const DataWriterQos& Qos) = 0;

    virtual	ReturnCode_t get_default_datawriter_qos(DataWriterQos& Qos) const = 0;

    static	ReturnCode_t copy_from_topic_qos(DataWriterQos& datawriter_qos, const TopicQos& topic_qos);
};

AGIDDS_NAMESPACE_END
