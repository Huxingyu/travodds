#include "dcps/topic/contentfilteredtopicimpl.h"
#include "dcps/topic/topicimpl.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "common/log/logger.h"
USING_TRAVODDS_NAMESPACE;
ContentFilteredTopicImpl::ContentFilteredTopicImpl(const std::string& name, Topic* related_topic, const std::string& filter_expression, const StringSeq& expression_parameters, DomainParticipant* participant)
    : TopicDescriptionImpl(name, related_topic->get_type_name(), dynamic_cast<DomainParticipantImpl*>(participant)), 
      filterExpression_(filter_expression), 
      expressionParameters_(expression_parameters),
      relatedTopic_(dynamic_cast<TopicImpl*>(related_topic))
{
    relatedTopic_->Reference();
}
ContentFilteredTopicImpl::~ContentFilteredTopicImpl()
{
    relatedTopic_->Dereference();
}
const std::string& ContentFilteredTopicImpl::get_filter_expression() const
{
    return filterExpression_;
}
Topic* ContentFilteredTopicImpl::get_related_topic() const
{
    return relatedTopic_;
}
ReturnCode_t ContentFilteredTopicImpl::get_expression_parameters(StringSeq& expression_parameters) const
{
	expression_parameters = expressionParameters_;
    return RETCODE_OK;
}

ReturnCode_t ContentFilteredTopicImpl::set_expression_parameters(const StringSeq& expression_parameters)
{
    if (expression_parameters == expressionParameters_) {
        return RETCODE_OK;
    }
    if (!ValidFilterExpression(filterExpression_, expression_parameters, participant_->find_type(typeName_)->get_dynamictype())) {
		return RETCODE_BAD_PARAMETER;
    }
	expressionParameters_ = expression_parameters;
	for (auto& reader : readers_) {
        reader->ContentFilterUpdate();
    }
    return RETCODE_OK;
}

bool ContentFilteredTopicImpl::ValidFilterExpression(const std::string& filter_expression, const StringSeq& expression_parameters, DynamicType* type)
{
	if (!type) {
   	    TRAVODDS_LOG(LOG_ERROR, "DynamicType is nullptr");
        return false;
    }
    DDSSQL::DDSSQLFilter filter(type);
    ReturnCode_t ret = filter.InputSQL(filter_expression, expression_parameters);
    return ret == RETCODE_OK ? true : false;
}

void ContentFilteredTopicImpl::AddReader(DataReaderImpl* reader)
{
	readers_.insert(reader);
}

void ContentFilteredTopicImpl::RemoveReader(DataReaderImpl* reader)
{
	readers_.erase(reader);
}
