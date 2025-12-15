#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

TEST(ContentFilteredTopicTest, ContentFilteredTopic_get_filter_expression) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string filter_expression = "id = 10 AND name = 'abc'";
	StringSeq expression_parameters;
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, filter_expression, expression_parameters);
    EXPECT_NE(cftop, nullptr);
    std::string result = cftop->get_filter_expression();
    EXPECT_EQ(result, filter_expression);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(ContentFilteredTopicTest, ContentFilteredTopic_get_related_topic) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string filter_expression = "id = 10 AND name = 'abc'";
    StringSeq expression_parameters;
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, filter_expression, expression_parameters);
    Topic* result = cftop->get_related_topic();
    EXPECT_EQ(result, top);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(ContentFilteredTopicTest, ContentFilteredTopic_get_expression_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string filter_expression = "id = %0 AND name = 'abc'";
    StringSeq expression_parameters;
	expression_parameters.push_back("10");
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, filter_expression, expression_parameters);
    StringSeq seqGet;
    ReturnCode_t result = cftop->get_expression_parameters(seqGet);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(seqGet, expression_parameters);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(ContentFilteredTopicTest, ContentFilteredTopic_set_get_expression_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string filter_expression = "id = %0 AND name = 'abc'";
    StringSeq expression_parameters;
    expression_parameters.push_back("10");
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, filter_expression, expression_parameters);
    StringSeq seq;
    seq.push_back("20");
    ReturnCode_t result = cftop->set_expression_parameters(seq);
    EXPECT_EQ(result, RETCODE_OK);

    StringSeq seqGet;
    ReturnCode_t resultGet = cftop->get_expression_parameters(seqGet);
    EXPECT_EQ(resultGet, RETCODE_OK);

    EXPECT_EQ(seqGet, seq);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
