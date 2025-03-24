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
#include "MultiTopic.h"
#include "TopicDescriptionImpl.h"
AGIDDS_NAMESPACE_BEGIN
class MultiTopicImpl : public virtual MultiTopic, public TopicDescriptionImpl {
public:
    MultiTopicImpl();
    MultiTopicImpl(const string& name, const string& type_name, const string& subscription_expression, const StringSeq& expression_parameters);
    ~MultiTopicImpl();

    const std::string&  get_subscription_expression() const;

    ReturnCode_t get_expression_parameters(StringSeq& expression_parameters) const;

    ReturnCode_t set_expression_parameters(const StringSeq& expression_parameters);
private:
    std::string subscriptionExpression_;
    StringSeq expressionParameters_;
};
AGIDDS_NAMESPACE_END
