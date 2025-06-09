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
#ifndef AGIDDS_DCPS_CONDITION_STATUSCONDITIONIMPL_H
#define AGIDDS_DCPS_CONDITION_STATUSCONDITIONIMPL_H

#include "dcps/condition/conditionimpl.h"
#include "dcps/condition/statuscondition.h"
#include "dcps/base/status.h"
#include "dcps/base/retcode.h"

AGIDDS_NAMESPACE_BEGIN

class StatusConditionImpl : public virtual StatusCondition, public ConditionImpl {
public:
	StatusConditionImpl(Entity* parent);

	bool get_trigger_value() const override;

	ReturnCode_t set_enabled_statuses(const StatusMask& mask);

	const StatusMask& get_enabled_statuses() const;

	Entity* get_entity() const;

public:
	/* 获取已触发状态字 */
	const StatusMask& get_status() const;

	/*    
	 *  topic 
	 *  INCONSISTENT_TOPIC
	 *  
	 *  subscriber
	 *  DATA_ON_READERS
	 * 
	 *  datareader
	 *  SAMPLE_REJECTED
	 *  LIVELINESS_CHANGED
	 *  REQUESTED_DEADLINE_MISSED
	 *  REQUESTED_INCOMPATIBLE_QOS
	 *  DATA_AVAILABLE
	 *  SAMPLE_LOST
	 *  SUBSCRIPTION_MATCHED
	 * 
	 *  datawriter
	 *  LIVELINESS_LOST
	 *  OFFERED_DEADLINE_MISSED
	 *  OFFERED_INCOMPATIBLE_QOS
	 *  PUBLICATION_MATCHED
	 */
	void set_status(const StatusMask& status, bool addFlag);

protected:
	Entity* entity_;
private:
	mutable std::mutex mtx_;
	/* 已触发状态字 */
	StatusMask status_;
	/* 启用的状态掩码 */
	StatusMask enabledMask_;
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_CONDITION_STATUSCONDITIONIMPL_H
