#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/subscribe/subscriber.h"
#include "dcps/subscribe/datareader.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

/****************************** QueryCondition *********************************************/
TEST(QueryConditionTest, QueryCondition_get_query_expression) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    StringSeq query_parameters;
    QueryCondition* qc = dr->create_querycondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, "query_expression", query_parameters);
    std::string result = qc->get_query_expression();
    EXPECT_EQ(result, "query_expression");
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(QueryConditionTest, QueryCondition_get_query_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    StringSeq seq;
    seq.push_back("query_parameters");
    QueryCondition* qc = dr->create_querycondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, "query_expression", seq);
    StringSeq seqGet;
    ReturnCode_t result = qc->get_query_parameters(seqGet);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(seqGet, seq);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(QueryConditionTest, QueryCondition_set_get_query_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    StringSeq query_parameters;
    QueryCondition* qc = dr->create_querycondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, "query_expression", query_parameters);
    StringSeq seq;
    seq.push_back("query_parameters");
    ReturnCode_t ret = qc->set_query_parameters(seq);
    EXPECT_EQ(ret, RETCODE_OK);
    StringSeq seqGet;
    ReturnCode_t result = qc->get_query_parameters(seqGet);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(seqGet, seq);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
