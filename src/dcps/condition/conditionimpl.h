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
#ifndef TRAVODDS_DCPS_CONDITION_CONDITIONIMPL_H
#define TRAVODDS_DCPS_CONDITION_CONDITIONIMPL_H

#include <atomic>
#include "travodds_dll.h"
#include "type/basictype.h"
#include "dcps/condition/condition.h"

TRAVODDS_NAMESPACE_BEGIN
class WaitSetImpl;

/**
 * @brief ConditionImpl，Condition类功能的具体实现。
 */
class ConditionImpl : public virtual Condition {
public:
	/**
	 * @brief 构造函数。
	 */
	ConditionImpl();

	/**
	 * @brief 析构函数。
	 */
	virtual ~ConditionImpl();

	/**
	 * @brief 检索条件的触发值。
	 * 
	 * @return 如果trigger_value设置为“true”，则为true，否则为false。
	 */
	virtual bool get_trigger_value() const;

public:
	/**
	 * @brief 唤醒附加到此通知程序的所有WaitSet实现。
	 */
	void Notify();

	/**
	 * @brief 判断是否已唤醒附加到此通知程序的所有WaitSet实现。
	 * 
	 * @return 以唤醒则返回true，否则返回false。
	 */
	bool IsNotify();

private:
	/**
	 * @brief 加入等待集。
	 * 
	 * @param [in] waitImpl 要加入的条件。
	 */
	void JoinWaitSet(WaitSetImpl* waitImpl);

	/**
	 * @brief 从等待集中删除。
	 * 
	 * @param [in] waitImpl 要删除的条件。
	 */
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

TRAVODDS_NAMESPACE_END

#endif // !CONDITION_IMPL_H
