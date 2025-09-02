#ifdef TEST_CONTENtFILTEREDTOPIC
#include "test_head.hpp"

TEST(DDSTest, ContentFilteredTopic_get_filter_expression) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    std::string filter_expression = "filter_expression";
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, filter_expression, strSeq);
    std::string result = cftop->get_filter_expression();
    EXPECT_EQ(result, filter_expression);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTest, ContentFilteredTopic_get_related_topic) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, "filter_expression", strSeq);
    Topic* result = cftop->get_related_topic();
    EXPECT_EQ(result, top);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTest, ContentFilteredTopic_get_expression_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    StringSeq seq;
    seq.push_back("expression_parameters");
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, "filter_expression", seq);
    StringSeq seqGet;
    ReturnCode_t result = cftop->get_expression_parameters(seqGet);
    EXPECT_EQ(result, RETCODE_OK);

    EXPECT_EQ(seqGet, seq);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTest, ContentFilteredTopic_set_get_expression_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    ContentFilteredTopic* cftop = par->create_contentfilteredtopic("cftop", top, "filter_expression", strSeq);
    StringSeq seq;
    seq.push_back("expression_parameters");
    ReturnCode_t result = cftop->set_expression_parameters(seq);
    EXPECT_EQ(result, RETCODE_OK);

    StringSeq seqGet;
    ReturnCode_t resultGet = cftop->get_expression_parameters(seqGet);
    EXPECT_EQ(resultGet, RETCODE_OK);

    EXPECT_EQ(seqGet, seq);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
#endif // TEST_CONTENtFILTEREDTOPIC
