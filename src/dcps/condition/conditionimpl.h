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
#ifndef AGIDDS_DCPS_CONDITION_CONDITIONIMPL_H
#define AGIDDS_DCPS_CONDITION_CONDITIONIMPL_H

#include  "agidds_dll.h"
#include  "type/basictype.h"
#include  "dcps/condition/condition.h"


AGIDDS_NAMESPACE_BEGIN

class ConditionImpl : public virtual Condition {
public:
	ConditionImpl() : triggerValue_(false) {};

	bool get_trigger_value() const;

protected:
	bool triggerValue_;

};

AGIDDS_NAMESPACE_END

#endif // !CONDITION_IMPL_H
