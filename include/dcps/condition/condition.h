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
#ifndef TRAVODDS_DCPS_CONDITION_CONDITION_H
#define TRAVODDS_DCPS_CONDITION_CONDITION_H

#include  "travodds_dll.h"
#include  "type/basictype.h"

TRAVODDS_NAMESPACE_BEGIN

/**
 * @brief Condition类是可能附加到WaitSet的所有条件的根基类。
 */
class DDS_DLL Condition {
public:
	/**
	 * @brief 析构函数。
	 */
	virtual ~Condition() = default;

	/**
	 * @brief 检索条件的触发值。
	 * @return 如果trigger_value设置为“true”，则为true，否则为false。
	 */
    virtual bool get_trigger_value() const = 0;
};

using ConditionSeq = Sequence<Condition*>;

TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_DCPS_CONDITION_CONDITION_H
