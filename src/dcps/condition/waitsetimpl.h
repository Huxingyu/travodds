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
class WaitSetImpl
{
public:
	/**
	 * @brief 析构函数。
	 */
	~WaitSetImpl();

	/**
	 * @brief 为等待集附加条件。
	 *
	 * @param [in] condition 条件。
	 *
	 * @return RETCODE_OK（如果连接正确），否则为其他错误代码。
	 */
	ReturnCode_t attach_condition(const Condition& condition);

	/**
	 * @brief 从WaitSet中分离条件。
	 *
	 * @param [in] condition 条件。
	 *
	 * @return 如果正确分离，则返回RETCODE_OK；如果未附加条件，则返回PRECONDITION_NOT_MET。
	 */
	ReturnCode_t detach_condition(const Condition& condition);

	/**
	 * @brief 允许应用程序线程等待某些条件的发生。
	 *
	 * 如果附加到WaitSet的条件都没有trigger_value为true，则等待函数将阻止挂起调用线程。
	 *
	 * @param [in] active_conditions 引用trigger_value为真的条件集合。
	 * @param [in] timeout			 最长等待时间。
	 *
	 * @return 如果一切正常，则返回RETCODE_OK，如果WaitSet已经在等待，
	 * 则返回PRECONDITION_NOT_MET，如果最大值，则返回TIMEOUT时间已过，否则为错误代码
	 */
	ReturnCode_t wait(ConditionSeq& active_conditions, const Duration_t& timeout);

	/**
	 * @brief 检索获取附加条件列表。
	 *
	 * @param [out] attached_conditions 附加条件参考附加条件的集合。
	 *
	 * @return 如果一切正常则返回RETCODE_OK，否则返回错误代码。
	 */
	ReturnCode_t get_conditions(ConditionSeq& attached_conditions) const;

public:

	/**
	 * @brief 检索获取活跃附加条件列表。
	 *
	 * @param [out] active_conditions 活跃的附加条件的集合。
	 *
	 * @return 活跃附件条件列表大小。
	 */
	uint32_t FindTrigger(ConditionSeq& active_conditions);
	
	/**
	 * @brief 唤醒正在等待该条件变量的线程。
	 */
	void Notify();

private:
	mutable std::mutex mtx_;
	mutable std::mutex mtxCv_;
	std::condition_variable cv_;
	std::vector<ConditionImpl*> conditions_;
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_CONDITION_WAITSETIMPL_H
