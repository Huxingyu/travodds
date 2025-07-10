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
#ifndef AGIDDS_DCPS_CONDITION_GUARDCONDITION_H
#define AGIDDS_DCPS_CONDITION_GUARDCONDITION_H

#include "dcps/condition/condition.h"
#include "dcps/base/retcode.h"

AGIDDS_NAMESPACE_BEGIN
class GuardConditionImpl;
class WaitSetImpl;
/**
 * @brief GuardCondition类是一个特定的条件，其trigger_value完全受应用程序的控制。
 *
 * GuardCondition的目的是为应用程序提供手动唤醒WaitSet的方法。
 * 这是通过将GuardCondition附加到WaitSet，
 * 然后通过set_trigger_value函数设置trigger_value来实现的。
 *
 */
class DDS_DLL GuardCondition : public virtual Condition {
public:
	/**
	 * @brief 构造函数。
	 */
	GuardCondition();

	/**
	 * @brief 析构函数。
	 */
	~GuardCondition();

	/**
	 * @brief 检索条件的触发值。
	 * 
	 * @return 如果trigger_value设置为“true”，则为true，否则为false。
	 */
	bool get_trigger_value() const;

	/**
	 * @brief 设置trigger_value。
	 * 
	 * @param [in] value 触发器的新数据。
	 * 
	 * @return RETCODE_OK
	 */
	ReturnCode_t set_trigger_value(bool value);

private:
	GuardConditionImpl* impl_;
	friend class WaitSetImpl;
};

AGIDDS_NAMESPACE_END
#endif
