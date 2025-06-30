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
#ifndef AGIDDS_DCPS_CONDITION_READCONDITIONIMPL_H
#define AGIDDS_DCPS_CONDITION_READCONDITIONIMPL_H

#include "dcps/condition/readcondition.h"
#include "dcps/condition/conditionimpl.h"
#include "dcps/subscribe/states.h"

AGIDDS_NAMESPACE_BEGIN

class DataReaderImpl;
/**
 * @brief 专门用于读取操作时为DataReader附加读取条件。
 *
 * ReadCondition对象允许应用程序指定它感兴趣的数据样本（通过指定所需的SampleStateMask、ViewStateMask和InstanceStateMask）。
 * 只有在适当的信息可用时才会触发该条件。
 * 它们将作为正常条件与WaitSet一起使用。
 * 可以将多个ReadCondition附加到同一个DataReader。
 */
class ReadConditionImpl : public virtual ReadCondition, public ConditionImpl {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] smapleStates     位掩码SampleStateKind。
	 * @param [in] viewStates	    位掩码ViewStateMask。
	 * @param [in] instanceStates	位掩码InstanceStateMask。
	 * @param [in] dataReader   	DataReaderImpl类指针。
	 */
	ReadConditionImpl(SampleStateMask smapleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, DataReaderImpl* dataReader);
	/**
	 * @brief 析构函数。
	 */
    ~ReadConditionImpl();

	/**
	 * @brief 检索符合条件的触发值。
	 * 
	 * @return 如果触发器值设置为“true”，则为true，否则为“false”。
	 */
	virtual bool get_trigger_value() const;

	/**
	 * @brief 检索与ReadCondition关联的DataReader。
	 *
	 * 请注意，每个ReadCondition正好有一个DataReader关联。
	 *
	 * @return 指向与此ReadCondition关联的DataReader的指针。
	 */
	DataReader* get_datareader() const;

	/**
	 * @brief 获取SampleStateMask值。
	 * 
	 * @return SampleStateMask值。
	 */
	const SampleStateMask& get_sample_state_mask() const;

	/**
	 * @brief 获取ViewStateMask值。
	 *
	 * @return SampleStateMask值。
	 */
	const ViewStateMask& get_view_state_mask() const;

	/**
	 * @brief 获取InstanceStateMask值。
	 *
	 * @return InstanceStateMask值。
	 */
	const InstanceStateMask& get_instance_state_mask() const;
private:
    SampleStateMask sampleStateMask_;
    ViewStateMask viewStateMask_;
    InstanceStateMask instanceStateMask_;
    DataReaderImpl* dataReader_ = nullptr;
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_CONDITION_READCONDITIONIMPL_H
