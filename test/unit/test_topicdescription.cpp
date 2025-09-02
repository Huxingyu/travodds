#ifdef TEST_TOPICDESCRIPTION
#include "test_head.hpp"
/****************************** TopicDescription *********************************************/
TEST(DDSTest, TopicDescription_get_participant1) {
    DomainParticipantFactoryQos qos;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    DomainParticipant* result = top->get_participant();
    EXPECT_EQ(result, par);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, TopicDescription_get_participant2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* related_topic = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("ContentFilteredTopicname", related_topic, "expression", strSeq);
    DomainParticipant* result = cftop->get_participant();
    EXPECT_EQ(result, par);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, TopicDescription_get_participant3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    MultiTopic* mtopic = par->create_multitopic("MultiTopicname", "data", "expression", strSeq);
    DomainParticipant* result = mtopic->get_participant();
    EXPECT_EQ(result, par);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, TopicDescription_get_type_name1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    std::string name = "data";
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string nameGet = top->get_type_name();
    EXPECT_EQ(nameGet, name);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, TopicDescription_get_type_name2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    std::string name = "data";
    par->registe_type("data", type);
    Topic* related_topic = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("ContentFilteredTopicname", related_topic, "expression", strSeq);
    std::string nameGet = cftop->get_type_name();
    EXPECT_EQ(nameGet, name);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, TopicDescription_get_type_name3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    std::string name = "data";
    par->registe_type("data", type);
    MultiTopic* mtopic = par->create_multitopic("MultiTopicname", "data", "expression", strSeq);
    std::string nameGet = mtopic->get_type_name();
    EXPECT_EQ(nameGet, name);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, TopicDescription_get_name1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    std::string name = "topicname";
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string nameGet = top->get_name();
    EXPECT_EQ(nameGet, name);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, TopicDescription_get_name2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* related_topic = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string name = "ContentFilteredTopicname";
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("ContentFilteredTopicname", related_topic, "expression", strSeq);
    std::string nameGet = cftop->get_name();
    EXPECT_EQ(nameGet, name);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, TopicDescription_get_name3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    std::string name = "MultiTopicname";
    MultiTopic* mtopic = par->create_multitopic("MultiTopicname", "data", "expression", strSeq);
    std::string nameGet = mtopic->get_name();
    EXPECT_EQ(nameGet, name);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
#endif // TEST_TOPICDESCRIPTION
