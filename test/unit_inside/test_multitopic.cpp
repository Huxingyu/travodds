#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

TEST(MultiTopicTest, MultiTopic_get_subscription_expression) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    std::string subscription_expression = "subscription_expression";
    StringSeq seq;
    MultiTopic* mltop = par->create_multitopic("mltop", type_name, subscription_expression, seq);
    std::string result = mltop->get_subscription_expression();
    EXPECT_EQ(result, subscription_expression);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}


TEST(MultiTopicTest, MultiTopic_get_expression_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    StringSeq seq;
    seq.push_back("expression_parameters");
    MultiTopic* mltop = par->create_multitopic("mltop", type_name, "subscription_expression", seq);
    StringSeq seqGet;
    ReturnCode_t resultGet = mltop->get_expression_parameters(seqGet);
    EXPECT_EQ(resultGet, RETCODE_OK);

    EXPECT_EQ(seqGet, seq);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}


TEST(MultiTopicTest, MultiTopic_set_get_expression_parameters) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    StringSeq strSeq;
    MultiTopic* mltop = par->create_multitopic("mltop", type_name, "subscription_expression", strSeq);
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

