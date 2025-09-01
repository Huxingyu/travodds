#include "dcps/topic/multitopicimpl.h"

USING_TRAVODDS_NAMESPACE;

MultiTopicImpl::MultiTopicImpl(const std::string& name, const std::string& type_name, const std::string& subscription_expression, const StringSeq& expression_parameters, DomainParticipantImpl* participant)
    : TopicDescriptionImpl(name, type_name, participant), subscriptionExpression_(subscription_expression), expressionParameters_(expression_parameters)
{
}

MultiTopicImpl::~MultiTopicImpl()
{
}

const std::string& MultiTopicImpl::get_subscription_expression() const
{
    return subscriptionExpression_;
}

ReturnCode_t MultiTopicImpl::get_expression_parameters(StringSeq& expression_parameters) const
{
    expression_parameters = expressionParameters_;
    return RETCODE_OK;

}

ReturnCode_t MultiTopicImpl::set_expression_parameters(const StringSeq& expression_parameters)
{
    expressionParameters_ = expression_parameters;
    return RETCODE_OK;
}
