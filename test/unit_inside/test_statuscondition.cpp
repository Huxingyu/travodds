#include "test_statuscondition.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/condition/statuscondition.h"
#include "dcps/condition/waitset.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

void StatusConditionTest::SetUp()
{
}

void StatusConditionTest::TearDown()
{
}

TEST(StatusConditionTest, StatusCondition_interface)
{
	DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, DATA_ON_READERS_STATUS);
	StatusCondition& sc = par->get_statuscondition();
	EXPECT_EQ(sc.get_enabled_statuses(), DATA_ON_READERS_STATUS);

	EXPECT_EQ(sc.set_enabled_statuses(DATA_AVAILABLE_STATUS), RETCODE_OK);

	EXPECT_EQ(sc.get_enabled_statuses(), DATA_AVAILABLE_STATUS);

	EXPECT_EQ(sc.get_entity(), par);
	TheParticipantFactory->delete_participant(par);
}

TEST(StatusConditionTest, StatusCondition_INCONSISTENT_TOPIC_STATUS)
{
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top_w = par_w->create_topic("StatusConditionTest_incon", type_name, TOPIC_QOS_DEFAULT, nullptr, INCONSISTENT_TOPIC_STATUS);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	type_name = TestModule::KeyDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::KeyDataTypeSupport::get_instance());
    Topic* top_r = par_r->create_topic("StatusConditionTest_incon", type_name, TOPIC_QOS_DEFAULT, nullptr, INCONSISTENT_TOPIC_STATUS);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	StatusCondition& sc_w = top_w->get_statuscondition();
	StatusCondition& sc_r = top_r->get_statuscondition();
	WaitSet ws;

	EXPECT_EQ(ws.attach_condition(sc_w), RETCODE_OK);
	EXPECT_EQ(ws.attach_condition(sc_r), RETCODE_OK);
	ConditionSeq active_conditions;
	bool matched_writer = false;
	bool matched_reader = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

    for (int i = 0; i < 6; i++) {
        ws.wait(active_conditions, { 10,0 });
		for (auto cond : active_conditions) {
			if (cond == &sc_w) {
				matched_writer = true;
				InconsistentTopicStatus status;
				top_w->get_inconsistent_topic_status(status);
				EXPECT_EQ(status.total_count, 1);
				EXPECT_EQ(status.total_count_change, 1);

			}
			else if (cond == &sc_r) {
				matched_reader = true;
				InconsistentTopicStatus status;
				top_r->get_inconsistent_topic_status(status);
				EXPECT_EQ(status.total_count, 1);
				EXPECT_EQ(status.total_count_change, 1);
			}
		}
		if (matched_writer && matched_reader) {
			break;
		}
	}
	EXPECT_TRUE(matched_writer);
	EXPECT_TRUE(matched_reader);
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}

TEST(StatusConditionTest, StatusCondition_OFFERED_DEADLINE_MISSED_STATUS)
{
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DataWriterQos qos;
	qos.deadline.period = { 2, 0 };
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, qos, nullptr, OFFERED_DEADLINE_MISSED_STATUS);


	StatusCondition& sc_w = dw->get_statuscondition();
	WaitSet ws;

	EXPECT_EQ(ws.attach_condition(sc_w), RETCODE_OK);
	ConditionSeq active_conditions;
	EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);

	EXPECT_EQ(active_conditions.size(), 1);
	EXPECT_EQ(active_conditions[0], &sc_w);

	OfferedDeadlineMissedStatus status;

	dw->get_offered_deadline_missed_status(status);

	EXPECT_EQ(status.total_count, 1);
	EXPECT_EQ(status.total_count_change, 1);
	EXPECT_EQ(status.last_instance_handle, HANDLE_NIL);
	par_w->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
}

TEST(StatusConditionTest, StatusCondition_REQUESTED_DEADLINE_MISSED_STATUS)
{
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReaderQos qos;
	qos.deadline.period = { 2, 0 };
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, qos, nullptr, REQUESTED_DEADLINE_MISSED_STATUS);

	StatusCondition& sc_r = dr->get_statuscondition();
	WaitSet ws;

	EXPECT_EQ(ws.attach_condition(sc_r), RETCODE_OK);
	ConditionSeq active_conditions;
	EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);

	EXPECT_EQ(active_conditions.size(), 1);
	EXPECT_EQ(active_conditions[0], &sc_r);

	RequestedDeadlineMissedStatus status;
	dr->get_requested_deadline_missed_status(status);

	EXPECT_EQ(status.total_count, 1);
	EXPECT_EQ(status.total_count_change, 1);
	EXPECT_EQ(status.last_instance_handle, HANDLE_NIL);
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_r);
}

TEST(StatusConditionTest, StatusCondition_INCOMPATIBLE_QOS_STATUS) {
	std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriterQos qos_dw;
	qos_dw.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, qos_dw, nullptr, OFFERED_INCOMPATIBLE_QOS_STATUS);
	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReaderQos qos_dr;
	qos_dr.reliability.kind = RELIABLE_RELIABILITY_QOS;
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, qos_dr, nullptr, REQUESTED_INCOMPATIBLE_QOS_STATUS);

	StatusCondition& sc_w = dw->get_statuscondition();
	StatusCondition& sc_r = dr->get_statuscondition();
	WaitSet ws;

	EXPECT_EQ(ws.attach_condition(sc_w), RETCODE_OK);
	EXPECT_EQ(ws.attach_condition(sc_r), RETCODE_OK);
	ConditionSeq active_conditions;
	bool matched_writer = false;
	bool matched_reader = false;

    for (int i = 0; i < 6; i++) {
        ws.wait(active_conditions, { 10,0 });
		for (auto cond : active_conditions) {
			if (cond == &sc_w) {
				matched_writer = true;
				OfferedIncompatibleQosStatus status;
				dw->get_offered_incompatible_qos_status(status);
				EXPECT_EQ(status.total_count, 1);
				EXPECT_EQ(status.total_count_change, 1);
				EXPECT_EQ(status.last_policy_id, RELIABILITY_QOS_POLICY_ID);
				EXPECT_EQ(status.policies.size(), 1);
				EXPECT_EQ(status.policies[0].policy_id, RELIABILITY_QOS_POLICY_ID);
				EXPECT_EQ(status.policies[0].count, 1);
			}
			else if (cond == &sc_r) {
				matched_reader = true;
				RequestedIncompatibleQosStatus status;
				dr->get_requested_incompatible_qos_status(status);
				EXPECT_EQ(status.total_count, 1);
				EXPECT_EQ(status.total_count_change, 1);
				EXPECT_EQ(status.last_policy_id, RELIABILITY_QOS_POLICY_ID);
				EXPECT_EQ(status.policies.size(), 1);
				EXPECT_EQ(status.policies[0].policy_id, RELIABILITY_QOS_POLICY_ID);
				EXPECT_EQ(status.policies[0].count, 1);
			}
		}
		if (matched_writer && matched_reader) {
			break;
		}
	}
	EXPECT_TRUE(matched_writer);
	EXPECT_TRUE(matched_reader);
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);

}
// 需要硬件模拟丢包配合测试
//TEST(StatusConditionTest, DISABLED_StatusCondition_SAMPLE_LOST_STATUS) {
//	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();

//	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
//	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
//	Topic* top_w = par_w->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
//	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
//		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

//	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
//	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
//	Topic* top_r = par_r->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
//	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
//		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, SAMPLE_LOST_STATUS);

//	StatusCondition& sc = dr->get_statuscondition();
//	WaitSet ws;
//	EXPECT_EQ(ws.attach_condition(sc), RETCODE_OK);
//	ConditionSeq attached_conditions;
//	EXPECT_EQ(ws.get_conditions(attached_conditions), RETCODE_OK);
//	EXPECT_EQ(attached_conditions.size(), 1);
//	EXPECT_EQ(attached_conditions[0], &sc);
//	ConditionSeq active_conditions;
//	std::thread th_1(
//		[&] {
//			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
//			SampleLostStatus status;
//			dr->get_sample_lost_status(status);
//			EXPECT_EQ(status.total_count, 1);
//			EXPECT_EQ(status.total_count_change, 1);
//		});
//	std::thread th_2(
//		[&] {
//			TestModule::BasicData data;
//			data.id = 1;
//			data.name = "test";
//		    for (int i= 0;i<10;i++)
//			{
//				std::this_thread::sleep_for(std::chrono::milliseconds(200));
//				EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
//			}
//		});
//	th_1.join();
//	th_2.join();
//	par_w->delete_contained_entities();
//	par_r->delete_contained_entities();
//	TheParticipantFactory->delete_participant(par_w);
//	TheParticipantFactory->delete_participant(par_r);
//}

TEST(StatusConditionTest, StatusCondition_SAMPLE_LOST_STATUS) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();

	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReaderQos qos;
	qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
	qos.history.kind = KEEP_ALL_HISTORY_QOS;
	qos.resource_limits.max_instances =1;
	qos.resource_limits.max_samples=1;
	qos.resource_limits.max_samples_per_instance = 1;
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, qos, nullptr, SAMPLE_REJECTED_STATUS);

	StatusCondition& sc = dr->get_statuscondition();
	WaitSet ws;
	EXPECT_EQ(ws.attach_condition(sc), RETCODE_OK);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
			SampleRejectedStatus status;
			dr->get_sample_rejected_status(status);
			EXPECT_EQ(status.total_count, 1);
			EXPECT_EQ(status.total_count_change, 1);
			EXPECT_EQ(status.last_reason, REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT);
			EXPECT_EQ(status.last_instance_handle, HANDLE_NIL);
		});
	std::thread th_2(
		[&] {
			TestModule::BasicData data;
			data.id = 1;
			data.name = "test";
			for (int i = 0; i < 3; i++)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
				EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
			}
		});
	th_1.join();
	th_2.join();
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}

TEST(StatusConditionTest, StatusCondition_DATA_ON_READERS_STATUS) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	Subscriber* sub = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, DATA_ON_READERS_STATUS);
	DataReader* dr = sub->create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	StatusCondition& sc_r = sub->get_statuscondition();
	WaitSet ws;

	EXPECT_EQ(ws.attach_condition(sc_r), RETCODE_OK);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
			EXPECT_EQ(active_conditions.size(), 1);
			EXPECT_EQ(active_conditions[0], &sc_r);
			DataReaderSeq drs; 
			sub->get_datareaders(drs, NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
			EXPECT_EQ(drs.size(), 1);
			EXPECT_EQ(drs[0], dr);
			SequenceData<TestModule::BasicData> data_seq;
			SampleInfoSeq info_seq;

			EXPECT_EQ(dr->take(
				data_seq, info_seq, LENGTH_UNLIMITED,
				ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE),
				RETCODE_OK);
			EXPECT_EQ(data_seq.length(), 1);
			EXPECT_EQ(data_seq[0].id, 1);
			EXPECT_EQ(data_seq[0].name, "test");
		});
	std::thread th_2(
		[&] {
			TestModule::BasicData data;
			data.id = 1;
			data.name = "test";
			std::this_thread::sleep_for(std::chrono::seconds(1));

			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}

TEST(StatusConditionTest, StatusCondition_DATA_AVAILABLE_STATUS) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
	    create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

	StatusCondition& sc_r = dr->get_statuscondition();
	WaitSet ws;

	EXPECT_EQ(ws.attach_condition(sc_r), RETCODE_OK);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
			EXPECT_EQ(active_conditions.size(), 1);
			EXPECT_EQ(active_conditions[0], &sc_r);
			SequenceData<TestModule::BasicData> data_seq;
			SampleInfoSeq info_seq;

			EXPECT_EQ(dr->take(
				data_seq, info_seq, LENGTH_UNLIMITED,
				ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE),
				RETCODE_OK);
			EXPECT_EQ(data_seq.length(), 1);
			EXPECT_EQ(data_seq[0].id, 1);
			EXPECT_EQ(data_seq[0].name, "test");
		});
	std::thread th_2(
		[&] {
			TestModule::BasicData data;
			data.id = 1;
			data.name = "test";
			std::this_thread::sleep_for(std::chrono::seconds(1));

			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}

TEST(StatusConditionTest, StatusCondition_DATA_AVAILABLE_STATUS_DATAFRAG) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

	StatusCondition& sc_r = dr->get_statuscondition();
	WaitSet ws;
	TestModule::BasicData data;
	data.id = 1;
	data.name.resize(124 * 1024, 'a');
	EXPECT_EQ(ws.attach_condition(sc_r), RETCODE_OK);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
			EXPECT_EQ(active_conditions.size(), 1);
			EXPECT_EQ(active_conditions[0], &sc_r);
			SequenceData<TestModule::BasicData> data_seq;
			SampleInfoSeq info_seq;

			EXPECT_EQ(dr->take(
				data_seq, info_seq, LENGTH_UNLIMITED,
				ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE),
				RETCODE_OK);
			EXPECT_EQ(data_seq.length(), 1);
			EXPECT_EQ(data_seq[0].id, 1);
			EXPECT_EQ(data_seq[0].name, data.name);
		});
	std::thread th_2(
		[&] {

			std::this_thread::sleep_for(std::chrono::seconds(1));

			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}

TEST(StatusConditionTest, StatusCondition_DATA_AVAILABLE_STATUS_DATAFRAG_RELIABLE) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("StatusConditionTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReaderQos qos;
	qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

	StatusCondition& sc_r = dr->get_statuscondition();
	WaitSet ws;
	TestModule::BasicData data;
	data.id = 1;
	data.name.resize(124 * 1024, 'a');
	EXPECT_EQ(ws.attach_condition(sc_r), RETCODE_OK);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
			EXPECT_EQ(active_conditions.size(), 1);
			EXPECT_EQ(active_conditions[0], &sc_r);
			SequenceData<TestModule::BasicData> data_seq;
			SampleInfoSeq info_seq;

			EXPECT_EQ(dr->take(
				data_seq, info_seq, LENGTH_UNLIMITED,
				ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE),
				RETCODE_OK);
			EXPECT_EQ(data_seq.length(), 1);
			EXPECT_EQ(data_seq[0].id, 1);
			EXPECT_EQ(data_seq[0].name, data.name);
		});
	std::thread th_2(
		[&] {

			std::this_thread::sleep_for(std::chrono::seconds(1));

			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}

TEST(StatusConditionTest, StatusCondition_MATCHED_STATUS) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top_w = par_w->create_topic("QoSTest_mas", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, PUBLICATION_MATCHED_STATUS);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top_r = par_r->create_topic("QoSTest_mas", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, SUBSCRIPTION_MATCHED_STATUS);

	StatusCondition& sc_w = dw->get_statuscondition();
	StatusCondition& sc_r = dr->get_statuscondition();
	WaitSet ws;

	EXPECT_EQ(ws.attach_condition(sc_w), RETCODE_OK);
	EXPECT_EQ(ws.attach_condition(sc_r), RETCODE_OK);
	ConditionSeq active_conditions;
	bool matched_writer = false;
    bool matched_reader = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    for (int i = 0; i < 6; i++) {
        EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
		for (auto cond : active_conditions) {
			if (cond == &sc_w) {
				matched_writer = true;
				PublicationMatchedStatus status;
				dw->get_publication_matched_status(status);
				EXPECT_EQ(status.total_count, 1);
				EXPECT_EQ(status.total_count_change, 1);
				EXPECT_EQ(status.current_count, 1);
				EXPECT_EQ(status.current_count_change, 1);
				EXPECT_EQ(status.last_subscription_handle, dr->get_instance_handle());
			}
			else if (cond == &sc_r) {
				matched_reader = true;
				SubscriptionMatchedStatus status;
				dr->get_subscription_matched_status(status);
				EXPECT_EQ(status.total_count, 1);
				EXPECT_EQ(status.total_count_change, 1);
				EXPECT_EQ(status.current_count, 1);
				EXPECT_EQ(status.current_count_change, 1);
				EXPECT_EQ(status.last_publication_handle, dw->get_instance_handle());
			}
		}
		if (matched_writer && matched_reader) {
			break;
		}
	}
	EXPECT_TRUE(matched_writer);
	EXPECT_TRUE(matched_reader);
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}
