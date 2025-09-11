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
#ifndef TRAVODDS_DCPS_CONDITION_READCONDITION_H
#define TRAVODDS_DCPS_CONDITION_READCONDITION_H

#include "dcps/condition/condition.h"
#include "dcps/subscribe/states.h"

TRAVODDS_NAMESPACE_BEGIN

class DataReader;
/**
 * @brief 专门用于读取操作时为DataReader附加读取条件。
 *
 * ReadCondition对象允许应用程序指定它感兴趣的数据样本（通过指定所需的SampleStateMask、ViewStateMask和InstanceStateMask）。
 * 只有在适当的信息可用时才会触发该条件。
 * 它们将作为正常条件与WaitSet一起使用。
 * 可以将多个ReadCondition附加到同一个DataReader。
 */
class DDS_DLL ReadCondition :public virtual Condition {
public:
	/**
	 * @brief 析构函数。
	 */
	virtual ~ReadCondition() = default;
	
	/**
	 * @brief 获取DataReader句柄。
	 * 
	 * @return DataReader句柄。
	 */
	virtual DataReader* get_datareader() const = 0;

	/**
	 * @brief 获取SampleStateMask值。
	 * 
	 * @return SampleStateMask值。
	 */
	virtual const SampleStateMask& get_sample_state_mask() const = 0;

	/**
	 * @brief 获取ViewStateMask值。
	 * 
	 * @return ViewStateMask值。
	 */
	virtual const ViewStateMask& get_view_state_mask() const = 0;

	/**
	 * @brief 获取InstanceStateMask值。
	 * 
	 * @return InstanceStateMask值。
	 */
	virtual const InstanceStateMask& get_instance_state_mask() const = 0;
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_CONDITION_READCONDITION_H