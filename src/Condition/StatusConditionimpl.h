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

#include "Condition/ConditionImpl.h"
#include "Condition/StatusCondition.h"
#include "Status.h"
#include "RetCode.h"

AGIDDS_NAMESPACE_BEGIN

class DDS_DLL StatusConditionImpl :public virtual StatusCondition, public ConditionImpl {
public:
	StatusConditionImpl() {};
	StatusConditionImpl(Entity* parent);

	ReturnCode_t set_enabled_statuses(const StatusMask& mask);

	const StatusMask& get_enabled_statuses() const;

	Entity* get_entity() const;

public:
	const StatusMask& get_status() const;


protected:
	Entity* entity_ = nullptr;
private:
	StatusMask status_{};
};

AGIDDS_NAMESPACE_END
