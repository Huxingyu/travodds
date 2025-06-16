#include "dcps/condition/queryconditionimpl.h"
USING_AGIDDS_NAMESPACE;
QueryConditionImpl::QueryConditionImpl(SampleStateMask smapleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, const std::string& queryExpression, StringSeq queryParameters, DataReaderImpl* dataReader)
    : ReadConditionImpl(smapleStates, viewStates, instanceStates, dataReader), queryExpression_(queryExpression), queryParameters_(queryParameters)
{
}

const std::string& QueryConditionImpl::get_query_expression() const
{
    return queryExpression_;
}

ReturnCode_t QueryConditionImpl::get_query_parameters(StringSeq& query_parameters) const
{
    query_parameters = queryParameters_;
    return RETCODE_OK;
}

ReturnCode_t QueryConditionImpl::set_query_parameters(const StringSeq& query_parameters)
{
    queryParameters_ = query_parameters;
    return RETCODE_OK;
}
