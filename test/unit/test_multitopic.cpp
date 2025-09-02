#ifdef TEST_MULTITOPIC
#include "test_head.hpp"

TEST(DDSTest, MultiTopic_get_subscription_expression) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    std::string subscription_expression = "subscription_expression";
    MultiTopic* mltop = par->create_multitopic("mltop", "data", subscription_expression, strSeq);
    std::string result = mltop->get_subscription_expression();
    EXPECT_EQ(result, subscription_expression);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}


TEST(DDSTest, MultiTopic_get_expression_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    StringSeq seq;
    seq.push_back("expression_parameters");
    MultiTopic* mltop = par->create_multitopic("mltop", "data", "subscription_expression", seq);
    StringSeq seqGet;
    ReturnCode_t resultGet = mltop->get_expression_parameters(seqGet);
    EXPECT_EQ(resultGet, RETCODE_OK);

    EXPECT_EQ(seqGet, seq);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}


TEST(DDSTest, MultiTopic_set_get_expression_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    MultiTopic* mltop = par->create_multitopic("mltop", "data", "subscription_expression", strSeq);
    StringSeq seq;
    seq.push_back("expression_parameters");
    ReturnCode_t result = mltop->set_expression_parameters(seq);
    EXPECT_EQ(result, RETCODE_OK);
    StringSeq seqGet;
    ReturnCode_t resultGet = mltop->get_expression_parameters(seqGet);
    EXPECT_EQ(resultGet, RETCODE_OK);
    EXPECT_EQ(seqGet, seq);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
#endif // TEST_MULTITOPIC
