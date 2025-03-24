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
#include "RetCode.h"

AGIDDS_NAMESPACE_BEGIN
class GuardConditionImpl;
class DDS_DLL GuardCondition : public virtual Condition {
public:
	GuardCondition();

	~GuardCondition();

	bool get_trigger_value() const;

	ReturnCode_t set_trigger_value(bool value);

private:
	GuardConditionImpl* impl_;
};

AGIDDS_NAMESPACE_END
