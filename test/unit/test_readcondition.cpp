#ifdef TEST_READCONDITION
#include "test_head.hpp"
/****************************** ReadCondition *********************************************/
TEST(DDSTest, ReadCondition_get_datareader) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    ReadCondition* rc = dr->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    DataReader* result = rc->get_datareader();
    EXPECT_EQ(result, dr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, ReadCondition_get_datareader1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    QueryCondition* qc = dr->create_querycondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, "query_expression", strSeq);
    DataReader* result = qc->get_datareader();
    EXPECT_EQ(result, dr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, ReadCondition_get_sample_state_mask) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    SampleStateMask smask = 1 << 1;
    ReadCondition* rc = dr->create_readcondition(smask, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    SampleStateMask result = rc->get_sample_state_mask();
    EXPECT_EQ(result, smask);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, ReadCondition_get_sample_state_mask1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    SampleStateMask smask = 1 << 1;
    QueryCondition* qc = dr->create_querycondition(smask, ANY_VIEW_STATE, ANY_INSTANCE_STATE, "query_expression", strSeq);
    SampleStateMask result = qc->get_sample_state_mask();
    EXPECT_EQ(result, smask);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, ReadCondition_get_view_state_mask) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    ViewStateMask vmask = 1 << 2;
    ReadCondition* rc = dr->create_readcondition(ANY_SAMPLE_STATE, vmask, ANY_INSTANCE_STATE);
    ViewStateMask result = rc->get_view_state_mask();
    EXPECT_EQ(result, vmask);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, ReadCondition_get_view_state_mask1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    ViewStateMask vmask = 1 << 3;
    QueryCondition* qc = dr->create_querycondition(ANY_SAMPLE_STATE, vmask, ANY_INSTANCE_STATE, "query_expression", strSeq);
    ViewStateMask result = qc->get_view_state_mask();
    EXPECT_EQ(result, vmask);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, ReadCondition_get_instance_state_mask) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    InstanceStateMask imask = 1 << 3;
    QueryCondition* qc = dr->create_querycondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, imask, "query_expression", strSeq);
    InstanceStateMask result = qc->get_instance_state_mask();
    EXPECT_EQ(result, imask);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
#endif // TEST_READCONDITION
