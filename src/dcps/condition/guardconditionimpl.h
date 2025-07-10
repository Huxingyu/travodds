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
#ifndef AGIDDS_DCPS_CONDITION_GUARDCONDITIONIMPL_H
#define AGIDDS_DCPS_CONDITION_GUARDCONDITIONIMPL_H

#include "dcps/condition/conditionimpl.h"
#include "dcps/base/retcode.h"

AGIDDS_NAMESPACE_BEGIN
class GuardCondition;
/**
 * @brief GuardConditionImpl类是ConditionImpl类子类及其具体实现， 
 * 是一个管理特定的条件的类，其trigger_value完全受应用程序的控制。
 *
 * GuardCondition的目的是为应用程序提供手动唤醒WaitSet的方法。
 * 这是通过将GuardCondition附加到WaitSet，
 * 然后通过set_trigger_value函数设置trigger_value来实现的。
 *
 */
class GuardConditionImpl : public ConditionImpl {
public:
	/**
	 * @brief 构造函数。
	 */
	GuardConditionImpl(GuardCondition* guard);;

	/**
	 * @brief 设置trigger_value。
	 *
	 * @param [in] value 触发器的新数据。
	 *
	 * @return RETCODE_OK
	 */
	ReturnCode_t set_trigger_value(bool value);

	GuardCondition* GetGuardCondition() const;
private:
	GuardCondition* guard_;
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_CONDITION_GUARDCONDITIONIMPL_H
