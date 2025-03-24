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
#include "DDS/DomainParticipantFactory.h"
#include "DDS/DomainParticipantImpl.h"
AGIDDS_NAMESPACE_BEGIN

class DomainParticipantFactoryImpl : public virtual DomainParticipantFactory {
public:
 
	~DomainParticipantFactoryImpl();

    DomainParticipant* create_participant(DomainId_t domain_id,
        const DomainParticipantQos& qos,
        DomainParticipantListener* a_listener,
        const StatusMask& mask);

    ReturnCode_t delete_participant(DomainParticipant* &a_participant);

    DomainParticipant* lookup_participant(DomainId_t domain_id) const;

    ReturnCode_t set_default_participant_qos(const DomainParticipantQos& qos);

    ReturnCode_t get_default_participant_qos(DomainParticipantQos& qos) const;

    ReturnCode_t set_qos(const DomainParticipantFactoryQos& qos);

    ReturnCode_t get_qos(DomainParticipantFactoryQos& qos) const;

public:
    static DomainParticipantFactoryImpl* get_instance();
private:
	DomainParticipantFactoryImpl();
	std::map<InstanceHandle_t, std::unique_ptr<DomainParticipantImpl>> participants_;
	mutable std::mutex mtxParticipants_;
    DomainParticipantFactoryQos qos_ = {};
    DomainParticipantQos defaultParticipantQos_ = {};
};

AGIDDS_NAMESPACE_END
