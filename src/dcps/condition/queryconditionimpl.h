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
#ifndef TRAVODDS_DCPS_CONDITION_QUERYCONDITIONIMPL_H
#define TRAVODDS_DCPS_CONDITION_QUERYCONDITIONIMPL_H

#include "dcps/condition/readconditionimpl.h"
#include "dcps/condition/querycondition.h"
#include "dcps/base/retcode.h"

TRAVODDS_NAMESPACE_BEGIN

class QueryConditionImpl : public virtual QueryCondition, public  ReadConditionImpl {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] smapleStates     位掩码SampleStateKind。
	 * @param [in] viewStates	    位掩码ViewStateMask。
	 * @param [in] instanceStates	位掩码InstanceStateMask。
	 * @param [in] queryExpression	查询表达式。	
	 * @param [in] queryParameters	查询参数。
	 * @param [in] dataReader   	DataReaderImpl类指针。
	 */
	QueryConditionImpl(SampleStateMask smapleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, const std::string& queryExpression, StringSeq queryParameters, DataReaderImpl* dataReader);

	/**
	 * @brief 获取query_expression。
	 * @return 查询到的数据。
	 */
	virtual const std::string& get_query_expression() const;

	/**
	 * @brief 获取query_parameters。
	 * @param [in] value 返回查询到的数据。
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t get_query_parameters(StringSeq& query_parameters) const;

	/**
	 * @brief 设置query_parameters。
	 * 
	 * @param [in] value 要设置的新数据。
	 * 
	 * @return RETCODE_OK
	 */
	virtual ReturnCode_t set_query_parameters(const StringSeq& query_parameters);
private:
    std::string queryExpression_;
    StringSeq queryParameters_;
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_CONDITION_QUERYCONDITIONIMPL_H
