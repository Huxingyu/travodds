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

#include "DDS/DomainParticipant.h"
#include "Listener/DomainParticipantListener.h"

AGIDDS_NAMESPACE_BEGIN

#define DDSParticipantFactory DomainParticipantFactory::get_instance()

class DDS_DLL DomainParticipantFactory {
public:
    virtual ~DomainParticipantFactory() = default;

    virtual DomainParticipant* create_participant(DomainId_t domain_id,
                                                  const DomainParticipantQos& qos,
                                                  DomainParticipantListener* a_listener,
                                                  const StatusMask& mask) = 0;

    virtual ReturnCode_t delete_participant(DomainParticipant*& a_participant) = 0;

    virtual DomainParticipant* lookup_participant(DomainId_t domain_id) const = 0;

    virtual ReturnCode_t set_default_participant_qos(const DomainParticipantQos& qos) = 0;

    virtual ReturnCode_t get_default_participant_qos(DomainParticipantQos& qos) const = 0;

    virtual ReturnCode_t set_qos(const DomainParticipantFactoryQos& qos) = 0;

    virtual ReturnCode_t get_qos(DomainParticipantFactoryQos& qos) const = 0;

    static DomainParticipantFactory* get_instance();

};

AGIDDS_NAMESPACE_END
