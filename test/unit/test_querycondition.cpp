#ifdef TEST_QUERYCONDITION
#include "test_head.hpp"
/****************************** QueryCondition *********************************************/
TEST(DDSTest, QueryCondition_get_query_expression) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    QueryCondition* qc = dr->create_querycondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, "query_expression", strSeq);
    std::string result = qc->get_query_expression();
    EXPECT_EQ(result, "query_expression");
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, QueryCondition_get_query_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
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
TEST(DDSTest, QueryCondition_set_get_query_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    QueryCondition* qc = dr->create_querycondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, "query_expression", strSeq);
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
#endif // TEST_QUERYCONDITION
