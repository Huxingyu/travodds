#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

/****************************** TopicDescription *********************************************/
TEST(TopicDescriptionTest, TopicDescription_topic_get) {
    DomainParticipantFactoryQos qos;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    DomainParticipant* par_get = top->get_participant();
    EXPECT_EQ(par_get, par);

    std::string type_name_get = top->get_type_name();
    EXPECT_EQ(type_name_get, type_name);

    std::string name_get = top->get_name();
    EXPECT_EQ(name_get, "topicname");

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(TopicDescriptionTest, TopicDescription_contentFiltered_get) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* related_topic = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string filter_expression = "id = 10 AND name = 'abc'";
    StringSeq expression_parameters;
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("ContentFilteredTopicname", related_topic, filter_expression, expression_parameters);
    DomainParticipant* par_get = cftop->get_participant();
    EXPECT_EQ(par_get, par);

    std::string type_name_get = cftop->get_type_name();
    EXPECT_EQ(type_name_get, type_name);

    std::string name_get = cftop->get_name();
    EXPECT_EQ(name_get, "ContentFilteredTopicname");

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(TopicDescriptionTest, TopicDescription_multiTopic_get) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    StringSeq expression_parameters;
    MultiTopic* mtopic = par->create_multitopic("MultiTopicname", type_name, "expression", expression_parameters);
    DomainParticipant* par_get = mtopic->get_participant();
    EXPECT_EQ(par_get, par);

    std::string type_name_get = mtopic->get_type_name();
    EXPECT_EQ(type_name_get, type_name);

    std::string name_get = mtopic->get_name();
    EXPECT_EQ(name_get, "MultiTopicname");

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

