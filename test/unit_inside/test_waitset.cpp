#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/condition/guardcondition.h"
#include "dcps/condition/statuscondition.h"
#include "dcps/condition/readcondition.h"
#include "dcps/condition/waitset.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

TEST(WaitSetTest, WaitSet_guardcondition) {

	WaitSet ws;
	GuardCondition gc;
	EXPECT_EQ(ws.attach_condition(gc), RETCODE_OK);
	ConditionSeq attached_conditions;
	EXPECT_EQ(ws.get_conditions(attached_conditions), RETCODE_OK);
	EXPECT_EQ(attached_conditions.size(), 1);
	EXPECT_EQ(attached_conditions[0], &gc);
	ConditionSeq active_conditions;

	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);

		});

	std::thread th_2(
		[&] {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			gc.set_trigger_value(true);
		});
	th_1.join();
	th_2.join();

	EXPECT_EQ(active_conditions.size(), 1);
	EXPECT_EQ(active_conditions[0], &gc);
	gc.set_trigger_value(false);
	EXPECT_EQ(ws.detach_condition(gc), RETCODE_OK);
	EXPECT_EQ(ws.get_conditions(attached_conditions), RETCODE_OK);
	EXPECT_EQ(attached_conditions.size(), 0);
}


TEST(WaitSetTest, WaitSet_statuscondition) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();

	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

	StatusCondition& sc = dr->get_statuscondition();
	WaitSet ws;
	EXPECT_EQ(ws.attach_condition(sc), RETCODE_OK);
	ConditionSeq attached_conditions;
	EXPECT_EQ(ws.get_conditions(attached_conditions), RETCODE_OK);
	EXPECT_EQ(attached_conditions.size(), 1);
	EXPECT_EQ(attached_conditions[0], &sc);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
		});
	std::thread th_2(
		[&] {
			std::this_thread::sleep_for(std::chrono::seconds(1));

			TestModule::BasicData data;
			data.id = 1;
			data.name = "test";
			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	EXPECT_EQ(active_conditions.size(), 1);
	EXPECT_EQ(active_conditions[0], &sc);
	EXPECT_EQ(sc.get_entity(), dr);
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}

TEST(WaitSetTest, WaitSet_readcondition) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("WaitSetTest_readcondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("WaitSetTest_readcondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ReadCondition* rc = dr->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
	WaitSet ws;
	EXPECT_EQ(ws.attach_condition(*rc), RETCODE_OK);
	ConditionSeq attached_conditions;
	EXPECT_EQ(ws.get_conditions(attached_conditions), RETCODE_OK);
	EXPECT_EQ(attached_conditions.size(), 1);
	EXPECT_EQ(attached_conditions[0], rc);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
		});
	std::thread th_2(
		[&] {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			TestModule::BasicData data;
			data.id = 1;
			data.name = "test";
			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	EXPECT_EQ(active_conditions.size(), 1);
	EXPECT_EQ(active_conditions[0], rc);
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}