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
#pragma once

#include "Condition/ReadConditionImpl.h"
#include "Condition/QueryCondition.h"
#include "RetCode.h"
AGIDDS_NAMESPACE_BEGIN

class QueryConditionImpl : public virtual QueryCondition, public  ReadConditionImpl {
public:
	QueryConditionImpl();
	QueryConditionImpl(SampleStateMask smapleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, const string& queryExpression, StringSeq queryParameters, DataReader* dataReader);

	virtual const std::string& get_query_expression() const;

	virtual ReturnCode_t get_query_parameters(StringSeq& query_parameters) const;

	virtual ReturnCode_t set_query_parameters(const StringSeq& query_parameters);
private:
    std::string queryExpression_;
    StringSeq queryParameters_;
};

AGIDDS_NAMESPACE_END
