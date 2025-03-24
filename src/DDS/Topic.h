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

#include "DDS/TopicDescription.h"
#include "DDS/DomainEntity.h"
#include "Listener/TopicListener.h"
#include "Qos.h"

AGIDDS_NAMESPACE_BEGIN

class DDS_DLL Topic : public virtual DomainEntity, public virtual TopicDescription {
public:
    virtual ~Topic() = default;

	virtual ReturnCode_t set_qos(const TopicQos& qos) = 0;

	virtual ReturnCode_t get_qos(TopicQos& qos) const = 0;

	virtual ReturnCode_t set_listener(TopicListener* a_listener, const StatusMask& mask) = 0;

	virtual const TopicListener* get_listener() const = 0;

    virtual ReturnCode_t get_inconsistent_topic_status(InconsistentTopicStatus& status) const = 0;
};

AGIDDS_NAMESPACE_END

