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

#include "Condition/Condition.h"
#include "Status.h"
#include "RetCode.h"

AGIDDS_NAMESPACE_BEGIN

class Entity;

class DDS_DLL StatusCondition : public virtual Condition {
public:
	virtual ~StatusCondition() = default;

	virtual ReturnCode_t set_enabled_statuses(const StatusMask& mask) = 0;

	virtual const StatusMask& get_enabled_statuses() const = 0;

	virtual Entity* get_entity() const = 0;
};

AGIDDS_NAMESPACE_END
