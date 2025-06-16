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

#include <atomic>
#include "agidds_dll.h"
#include "type/basictype.h"
#include "dcps/condition/condition.h"

AGIDDS_NAMESPACE_BEGIN
class WaitSetImpl;
class ConditionImpl : public virtual Condition {
public:
	ConditionImpl();
	virtual ~ConditionImpl();

	virtual bool get_trigger_value() const;

public:
	void Notify();

	bool IsNotify();

private:
	void JoinWaitSet(WaitSetImpl* waitImpl);
	void LeaveWaitSet(WaitSetImpl* waitImpl);
	friend class WaitSetImpl;

protected:
	std::atomic_bool triggerValue_;

private:
	/* 标志是否参与了唤醒工作 */
	std::atomic_bool notify_;

	mutable std::mutex mtx_;
	std::vector<WaitSetImpl*> waitSets_;
};

AGIDDS_NAMESPACE_END

#endif // !CONDITION_IMPL_H
