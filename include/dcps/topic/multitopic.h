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
#ifndef AGIDDS_DCPS_TOPIC_MULTITOPIC_H
#define AGIDDS_DCPS_TOPIC_MULTITOPIC_H

#include "dcps/topic/topicdescription.h"
#include "dcps/base/retcode.h"
#include "type/basictype.h"

AGIDDS_NAMESPACE_BEGIN

class MultiTopic : public virtual TopicDescription {
public:
    virtual ~MultiTopic() = default;

	virtual const std::string&  get_subscription_expression() const = 0;

    virtual ReturnCode_t get_expression_parameters(StringSeq& expression_parameters) const = 0;

    virtual ReturnCode_t set_expression_parameters(const StringSeq& expression_parameters) = 0;
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_TOPIC_MULTITOPIC_H
