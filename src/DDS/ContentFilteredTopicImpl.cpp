#include "DDS/ContentFilteredTopicImpl.h"
#include "DDS/DomainParticipantImpl.h"
#include "DDS/TopicImpl.h"
USING_AGIDDS_NAMESPACE;
ContentFilteredTopicImpl::ContentFilteredTopicImpl()
{
}
ContentFilteredTopicImpl::ContentFilteredTopicImpl(const string& name, Topic* related_topic, const string& filter_expression, const StringSeq& expression_parameters, DomainParticipant* a_participant)
    : filterExpression_(filter_expression), expressionParameters_(expression_parameters)
{
    name_ = name;
    participant_ = dynamic_cast<DomainParticipantImpl*>(a_participant);
    relatedTopic_ = dynamic_cast<TopicImpl*>(related_topic);
    relatedTopic_->Reference();
}
ContentFilteredTopicImpl::~ContentFilteredTopicImpl()
{
    relatedTopic_->Dereference();
}
Topic* ContentFilteredTopicImpl::get_related_topic() const
{
    return relatedTopic_;
}
ReturnCode_t ContentFilteredTopicImpl::get_expression_parameters(StringSeq &expression_parameters) const
{
	expression_parameters = expressionParameters_;
    return RETCODE_OK;
}

ReturnCode_t ContentFilteredTopicImpl::set_expression_parameters(const StringSeq &expression_parameters)
{
    if (expression_parameters.size()) {
        expressionParameters_ = expression_parameters;
        return RETCODE_OK;
    }
    return RETCODE_ERROR;
}