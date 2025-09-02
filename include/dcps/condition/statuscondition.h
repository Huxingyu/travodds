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
#ifndef TRAVODDS_DCPS_CONDITION_STATUSCONDITION_H
#define TRAVODDS_DCPS_CONDITION_STATUSCONDITION_H

#include "dcps/condition/condition.h"
#include "dcps/base/status.h"
#include "dcps/base/retcode.h"

TRAVODDS_NAMESPACE_BEGIN

class Entity;
/**
 * @brief StatusCondition类是与每个实体关联的特定条件。
 *
 */
class DDS_DLL StatusCondition : public virtual Condition {
public:
	/**
	 * @brief 析构函数。
	 */
	virtual ~StatusCondition() = default;

	/**
	 * @brief 设置StatusMask位掩码值。
	 *
	 * @param value StatusKind位掩码值。
	 *
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t set_enabled_statuses(const StatusMask& mask) = 0;

	/**
	 * @brief 获取StatusMask位掩码值。
	 *
	 * @return  StatusKind位掩码值。
	 */
	virtual const StatusMask& get_enabled_statuses() const = 0;

	/**
	 * @brief 获取Entity 类对象。
	 * @return Entity 类对象
	 */
	virtual Entity* get_entity() const = 0;
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_CONDITION_STATUSCONDITION_H
