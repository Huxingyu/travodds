#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/subscribe/subscriber.h"
#include "dcps/subscribe/datareader.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

/****************************** ReadCondition *********************************************/
TEST(ReadConditionTest, ReadCondition_get) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    SampleStateMask smask = 1 << 1;
    ViewStateMask vmask = 1 << 2;
    InstanceStateMask imask = 1 << 3;

    ReadCondition* rc = dr->create_readcondition(smask, vmask, imask);
    EXPECT_NE(rc, nullptr);
    DataReader* result = rc->get_datareader();
    EXPECT_EQ(result, dr);

    SampleStateMask result_smask = rc->get_sample_state_mask();
    EXPECT_EQ(result_smask, smask);

    ViewStateMask result_vmask = rc->get_view_state_mask();
    EXPECT_EQ(result_vmask, vmask);

    InstanceStateMask result_imask = rc->get_instance_state_mask();
    EXPECT_EQ(result_imask, imask);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(ReadConditionTest, ReadCondition_queryCondition_get) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    SampleStateMask smask = 1 << 1;
    ViewStateMask vmask = 1 << 2;
    InstanceStateMask imask = 1 << 3;

    StringSeq query_parameters;
    QueryCondition* qc = dr->create_querycondition(smask, vmask, imask, "query_expression", query_parameters);
    EXPECT_NE(qc, nullptr);

    DataReader* result = qc->get_datareader();
    EXPECT_EQ(result, dr);

    SampleStateMask result_smask = qc->get_sample_state_mask();
    EXPECT_EQ(result_smask, smask);

    ViewStateMask result_vmask = qc->get_view_state_mask();
    EXPECT_EQ(result_vmask, vmask);

    InstanceStateMask result_imask = qc->get_instance_state_mask();
    EXPECT_EQ(result_imask, imask);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
