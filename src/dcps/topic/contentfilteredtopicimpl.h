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
#ifndef AGIDDS_DCPS_TOPIC_CONTENTFILTEREDTOPICIMPL_H
#define AGIDDS_DCPS_TOPIC_CONTENTFILTEREDTOPICIMPL_H

#include "dcps/topic/contentfilteredtopic.h"
#include "dcps/topic/topicdescriptionimpl.h"
AGIDDS_NAMESPACE_BEGIN
class TopicImpl;
class ContentFilteredTopicImpl : public virtual ContentFilteredTopic, public TopicDescriptionImpl {
public:
    ContentFilteredTopicImpl(const std::string& name, Topic* related_topic, const std::string& filter_expression, const StringSeq& expression_parameters, DomainParticipant* participant);
    
    virtual ~ContentFilteredTopicImpl();

    const std::string& get_filter_expression() const;

    Topic* get_related_topic() const;

    ReturnCode_t get_expression_parameters(StringSeq& expression_parameters) const;

    ReturnCode_t set_expression_parameters(const StringSeq& expression_parameters);
private:

    TopicImpl* relatedTopic_ = nullptr;
    std::string filterExpression_;
    StringSeq expressionParameters_;
};
AGIDDS_NAMESPACE_END
#endif //!AGIDDS_DCPS_TOPIC_CONTENTFILTEREDTOPICIMPL_H
