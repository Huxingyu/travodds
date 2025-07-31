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
#ifndef AGIDDS_DCPS_CONDITION_WAITSETIMPL_H
#define AGIDDS_DCPS_CONDITION_WAITSETIMPL_H

#include <condition_variable>
#include "dcps/condition/conditionimpl.h"
#include "dcps/base/retcode.h"

AGIDDS_NAMESPACE_BEGIN

class GuardConditionImpl;

class WaitSetImpl
{
public:
	~WaitSetImpl();

	ReturnCode_t attach_condition(const Condition& condition);

	ReturnCode_t detach_condition(const Condition& condition);

	ReturnCode_t wait(ConditionSeq& active_conditions, const Duration_t& timeout);

	ReturnCode_t get_conditions(ConditionSeq& attached_conditions) const;

public:
	uint32_t FindTrigger(ConditionSeq& active_conditions);

	void Notify();
protected:
	GuardConditionImpl* IsGuardCondition(Condition* cond);
	Condition* IsGuardConditionImpl(ConditionImpl* cond) const;
private:
	mutable std::mutex mtx_;
	mutable std::mutex mtxCv_;
	std::condition_variable cv_;
	std::vector<ConditionImpl*> conditions_;
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_CONDITION_WAITSETIMPL_H
