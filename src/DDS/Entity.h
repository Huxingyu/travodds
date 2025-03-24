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

#include "Condition/StatusCondition.h"

AGIDDS_NAMESPACE_BEGIN

class DDS_DLL Entity {
public:
    virtual ~Entity() = default;
	// virtual ReturnCode_t set_qos(QosPolicy& qos_list) = 0;
	//
	// virtual ReturnCode_t get_qos(QosPolicy& qos_list) = 0;

    // virtual ReturnCode_t set_listener(const Listener* a_listener, StatusMask Mask) = 0;
    //
    // virtual Listener* get_listener() = 0;

    virtual StatusCondition* get_statuscondition() const = 0;

    virtual const StatusMask& get_status_change() const = 0;

    virtual ReturnCode_t enable() = 0 ;

    virtual const InstanceHandle_t& get_instance_handle() const = 0;
};

AGIDDS_NAMESPACE_END
