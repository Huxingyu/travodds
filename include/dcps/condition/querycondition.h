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
#ifndef AGIDDS_DCPS_CONDITION_QUERYCONDITION_H
#define AGIDDS_DCPS_CONDITION_QUERYCONDITION_H

#include "dcps/condition/readcondition.h"
#include "dcps/base/retcode.h"

AGIDDS_NAMESPACE_BEGIN

class DDS_DLL QueryCondition : public virtual ReadCondition {
public:
	/**
	 * @brief 析构函数。
	 */
	virtual ~QueryCondition() = default;

	/**
	 * @brief 获取query_expression。
	 * @return 查询到的数据。
	 */
	virtual const std::string& get_query_expression() const = 0;

	/**
	 * @brief 获取query_parameters。
	 * @param value 返回查询到的数据。
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t get_query_parameters(StringSeq& query_parameters) const = 0;

	/**
	 * @brief 设置query_parameters。
	 * @param value 要设置的新数据。
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t set_query_parameters(const StringSeq& query_parameters) = 0;
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_CONDITION_QUERYCONDITION_H
