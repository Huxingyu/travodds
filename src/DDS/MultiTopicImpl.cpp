#include "MultiTopicImpl.h"

USING_AGIDDS_NAMESPACE;

MultiTopicImpl::MultiTopicImpl()
{

}

MultiTopicImpl::MultiTopicImpl(const string& name, const string& type_name, const string& subscription_expression, const StringSeq& expression_parameters)
    : subscriptionExpression_(subscription_expression), expressionParameters_(expression_parameters)
{
}

MultiTopicImpl::~MultiTopicImpl()
{
}

const std::string& MultiTopicImpl::get_subscription_expression() const
{
    return subscriptionExpression_;
}

ReturnCode_t MultiTopicImpl::get_expression_parameters(StringSeq &expression_parameters) const
{

    expression_parameters = expressionParameters_;
    return RETCODE_OK;

}

ReturnCode_t MultiTopicImpl::set_expression_parameters(const StringSeq &expression_parameters)
{

    expressionParameters_ = expression_parameters;
    return RETCODE_OK;

}