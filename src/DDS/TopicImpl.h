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
#include "DDS/Topic.h"
#include "DDS/DomainEntityImpl.h"
#include "DDS/TopicDescriptionImpl.h"
AGIDDS_NAMESPACE_BEGIN
class DomainParticipantImpl;
class TopicImpl : public virtual Topic, public DomainEntityImpl, public TopicDescriptionImpl {
public:
    TopicImpl();
    ~TopicImpl();
    TopicImpl(const string& name, const string& type_name, const TopicQos& qos, TopicListener* a_listener, StatusMask mask,DomainParticipant* a_participant);

    ReturnCode_t set_qos(const TopicQos& qos);

    ReturnCode_t get_qos(TopicQos& qos) const;

    ReturnCode_t set_listener(TopicListener* a_listener, const StatusMask& mask);

    const TopicListener* get_listener() const;

    ReturnCode_t get_inconsistent_topic_status(InconsistentTopicStatus& a_status) const;

public:
	ReturnCode_t enable();

private:
    TopicQos qos_;
    TopicListener* listener_ = nullptr;
};
AGIDDS_NAMESPACE_END    
