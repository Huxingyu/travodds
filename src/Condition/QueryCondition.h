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

#include "Condition/ReadCondition.h"
#include "RetCode.h"
AGIDDS_NAMESPACE_BEGIN

class DDS_DLL QueryCondition : public virtual ReadCondition {
public:
	virtual ~QueryCondition() = default;

	virtual const string& get_query_expression() const = 0;

	virtual ReturnCode_t get_query_parameters(StringSeq& query_parameters) const = 0;

	virtual ReturnCode_t set_query_parameters(const StringSeq& query_parameters) = 0;
};

AGIDDS_NAMESPACE_END
