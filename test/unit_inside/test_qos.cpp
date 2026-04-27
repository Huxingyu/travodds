#include "test_qos.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/condition/statuscondition.h"
#include "dcps/condition/waitset.h"
#include "qos/qos.h"
#include "testdataTypeSupport.h"
#include <chrono>
USING_TRAVODDS_NAMESPACE

void QoSTest::SetUp()
{
	positiveflag_ = false;
	negativeflag_ = false;
}

void QoSTest::TearDown()
{
}

void QoSTest::WaitPositive(int32_t second)
{
	std::unique_lock<std::mutex> lock(mtx_);
	positivecv_.wait_for(lock, std::chrono::seconds(second), [this] { return positiveflag_.exchange(false); });
}

void QoSTest::NotifyNegative()
{
	positiveflag_ = true;
	positivecv_.notify_one();
}

void QoSTest::WaitNegative(int32_t second)
{
	std::unique_lock<std::mutex> lock(mtx_);
	negativecv_.wait_for(lock, std::chrono::seconds(second), [this] { return negativeflag_.exchange(false); });
}

void QoSTest::NotifyPositive()
{
	negativeflag_ = true;
	negativecv_.notify_one();
}


bool QoSTest::waitmatch(DataWriter* writer, unsigned int count, int timeout) {
	int i = 0;
	while (i < timeout) {
		InstanceHandleSeq handles;
		ReturnCode_t ret = writer->get_matched_subscriptions(handles);
		if (ret == TRAVODDS::RETCODE_OK) {
			if (handles.size() >= count) {
				return true;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		++i;
	}
	return false;
}

bool QoSTest::waitmatch(DataReader* reader, unsigned int count, int timeout) {
	int i = 0;
	while (i < timeout) {
		InstanceHandleSeq handles;
		ReturnCode_t ret = reader->get_matched_publications(handles);
		if (ret == TRAVODDS::RETCODE_OK) {
			if (handles.size() >= count) {
				return true;
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
		++i;
	}
	return false;
}

TEST_F(QoSTest, QoS_domainparticipant_check) {
	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);
	DomainParticipantFactoryQos factory_qos;

	factory->set_qos(factory_qos);

	// 2. 创建DomainParticipant
	DomainParticipant* participant = factory->create_participant(
		0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
	ReturnCode_t result = participant->set_qos(dp_qos);
	EXPECT_EQ(result, RETCODE_OK);

	dp_qos.user_data.value.resize(270);
	result = participant->set_qos(dp_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);

	factory->delete_participant(participant);
	factory->finalize();
}

TEST_F(QoSTest, QoS_publisher_check) {

	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipant* participant = factory->create_participant(
		0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(publisher, nullptr);

	PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
	pub_qos.group_data.value.resize(300);

	ReturnCode_t result = publisher->set_qos(pub_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);
	pub_qos.group_data.value.resize(0);

	pub_qos.partition.name.resize(300);
	result = publisher->set_qos(pub_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);
	pub_qos.partition.name.resize(0);

	pub_qos.partition.name.resize(1);
	pub_qos.partition.name[0].resize(300);
	result = publisher->set_qos(pub_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);

	pub_qos.partition.name[0].resize(3000);;
	result = publisher->set_qos(pub_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);


	pub_qos.partition.name[0].resize(10);
	pub_qos.partition.name[0] = "AA";
	result = publisher->set_qos(pub_qos);
	EXPECT_EQ(result, RETCODE_OK);

	pub_qos.partition.name[0].resize(10);
	pub_qos.partition.name[0] = "(*";
	result = publisher->set_qos(pub_qos);
	EXPECT_EQ(result, RETCODE_OK);

	participant->delete_publisher(publisher);
	factory->delete_participant(participant);
}


TEST_F(QoSTest, QoS_subcsciber_check) {

	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipant* participant = factory->create_participant(
		0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	Subscriber* subcriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(subcriber, nullptr);

	SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
	sub_qos.group_data.value.resize(300);

	ReturnCode_t result = subcriber->set_qos(sub_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);
	sub_qos.group_data.value.resize(0);

	sub_qos.partition.name.resize(300);
	result = subcriber->set_qos(sub_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);
	sub_qos.partition.name.resize(0);

	sub_qos.partition.name.resize(1);
	sub_qos.partition.name[0].resize(300);
	result = subcriber->set_qos(sub_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);

	sub_qos.partition.name[0].resize(3000);;
	result = subcriber->set_qos(sub_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);


	sub_qos.partition.name[0].resize(10);
	sub_qos.partition.name[0] = "AA";
	result = subcriber->set_qos(sub_qos);
	EXPECT_EQ(result, RETCODE_OK);

	sub_qos.partition.name[0].resize(10);
	sub_qos.partition.name[0] = "(*";
	result = subcriber->set_qos(sub_qos);
	EXPECT_EQ(result, RETCODE_OK);

	participant->delete_subscriber(subcriber);
	factory->delete_participant(participant);
}


TEST_F(QoSTest, QoS_datawriter_check) {
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipant* participant = factory->create_participant(
		0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
	ASSERT_NE(type_support, nullptr);
	EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

	Topic* topic = participant->create_topic(
		"deadline_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(topic, nullptr);

	Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(publisher, nullptr);

	DataWriter* datawriter = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
	ReturnCode_t result = datawriter->set_qos(dw_qos);
	EXPECT_EQ(result, RETCODE_OK);

	dw_qos.user_data.value.resize(300);
	result = datawriter->set_qos(dw_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);

	dw_qos.user_data.value.resize(0);
	dw_qos.resource_limits.max_samples = 1;
	dw_qos.resource_limits.max_instances = 1;
	dw_qos.resource_limits.max_samples_per_instance = 2;
	result = datawriter->set_qos(dw_qos);
	EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);


	dw_qos.user_data.value.resize(0);
	dw_qos.resource_limits.max_samples = 1;
	dw_qos.resource_limits.max_instances = 1;
	dw_qos.resource_limits.max_samples_per_instance = 1;
	result = datawriter->set_qos(dw_qos);
	EXPECT_EQ(result, RETCODE_IMMUTABLE_POLICY);

	publisher->delete_datawriter(datawriter);
	participant->delete_topic(topic);
	participant->delete_publisher(publisher);
	factory->delete_participant(participant);

}
TEST_F(QoSTest, QoS_datareader_check) {
	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipant* participant = factory->create_participant(
		0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	//Topic* topic = participant->create_topic(SUBSCRIBER_QOS_DEFAULT, NULL, 0xFFFF);
	//ASSERT_NE(subcriber, nullptr);
	TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
	ASSERT_NE(type_support, nullptr);
	EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

	Topic* topic = participant->create_topic(
		"deadline_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(topic, nullptr);

	Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(subscriber, nullptr);

	DataReader* datareader = subscriber->create_datareader(topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
	ReturnCode_t result = datareader->set_qos(dr_qos);
	EXPECT_EQ(result, RETCODE_OK);

	dr_qos.user_data.value.resize(300);
	result = datareader->set_qos(dr_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);

	dr_qos.user_data.value.resize(0);
	dr_qos.resource_limits.max_samples = 1;
	dr_qos.resource_limits.max_instances = 1;
	dr_qos.resource_limits.max_samples_per_instance = 2;
	result = datareader->set_qos(dr_qos);
	EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);


	dr_qos.user_data.value.resize(0);
	dr_qos.resource_limits.max_samples = 1;
	dr_qos.resource_limits.max_instances = 1;
	dr_qos.resource_limits.max_samples_per_instance = 1;
	dr_qos.deadline.period = { 1,0 };
	dr_qos.time_based_filter.minimum_separation = { 2,0 };
	result = datareader->set_qos(dr_qos);
	EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);

	participant->delete_contained_entities();
	factory->delete_participant(participant);
}


TEST_F(QoSTest, QoS_topic_check) {
	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipant* participant = factory->create_participant(
		0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	//Topic* topic = participant->create_topic(SUBSCRIBER_QOS_DEFAULT, NULL, 0xFFFF);
	//ASSERT_NE(subcriber, nullptr);
	TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
	ASSERT_NE(type_support, nullptr);
	EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

	Topic* topic = participant->create_topic(
		"deadline_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(topic, nullptr);

	TopicQos topic_qos = TOPIC_QOS_DEFAULT;

	topic_qos.resource_limits.max_instances = 1;
	topic_qos.resource_limits.max_samples = 1;
	topic_qos.resource_limits.max_samples_per_instance = 1;
	topic_qos.history.depth = -1;
	ReturnCode_t result = topic->set_qos(topic_qos);
	EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);

	topic_qos.history.depth = 2;
	result = topic->set_qos(topic_qos);
	EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);


	topic_qos.resource_limits.max_samples_per_instance = 2;
	result = topic->set_qos(topic_qos);
	EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);


	topic_qos.resource_limits.max_instances = 1;
	topic_qos.resource_limits.max_samples = 1;
	topic_qos.resource_limits.max_samples_per_instance = 1;
	topic_qos.history.depth = 1;
	topic_qos.topic_data.value.resize(300);
	result = topic->set_qos(topic_qos);
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);

	topic_qos.topic_data.value.resize(0);
	result = topic->set_qos(topic_qos);
	EXPECT_EQ(result, RETCODE_IMMUTABLE_POLICY);
	participant->delete_contained_entities();
	factory->delete_participant(participant);

}

TEST_F(QoSTest, QoS_dicovery_udpv4) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, PUBLICATION_MATCHED_STATUS);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("QoSTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, SUBSCRIPTION_MATCHED_STATUS);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("QoSTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	
	StatusCondition& sc_w = par_w->get_statuscondition();
	StatusCondition& sc_r = par_r->get_statuscondition();
	WaitSet ws;

	EXPECT_EQ(ws.attach_condition(sc_w), RETCODE_OK);
	EXPECT_EQ(ws.attach_condition(sc_r), RETCODE_OK);
	ConditionSeq active_conditions;
	bool matched_writer = false;
	bool matched_reader = false;

	for (int i = 0; i < 2; i++) {
		EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
		for (auto cond : active_conditions) {
			if(cond == &sc_w){
				matched_writer = true;
				PublicationMatchedStatus status;
				dw->get_publication_matched_status(status);
				
			} else if (cond == &sc_r) {
				matched_reader = true;
				SubscriptionMatchedStatus status;
				dr->get_subscription_matched_status(status);
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
TEST_F(QoSTest, QoS_dicovery_udpv6) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipantQos qos_v6 = PARTICIPANT_QOS_DEFAULT;
	qos_v6.discovery.kind = DISCOVER_UDPv6_QOS;
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, qos_v6, nullptr, PUBLICATION_MATCHED_STATUS);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("QoSTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, qos_v6, nullptr, SUBSCRIPTION_MATCHED_STATUS);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("QoSTest", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

	StatusCondition& sc_w = par_w->get_statuscondition();
	StatusCondition& sc_r = par_r->get_statuscondition();
	WaitSet ws;

	EXPECT_EQ(ws.attach_condition(sc_w), RETCODE_OK);
	EXPECT_EQ(ws.attach_condition(sc_r), RETCODE_OK);
	ConditionSeq active_conditions;
	bool matched_writer = false;
	bool matched_reader = false;

	for (int i = 0; i < 2; i++) {
		EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
		for (auto cond : active_conditions) {
			if (cond == &sc_w) {
				matched_writer = true;
				PublicationMatchedStatus status;
				dw->get_publication_matched_status(status);
			} else if (cond == &sc_r) {
				matched_reader = true;
				SubscriptionMatchedStatus status;
				dr->get_subscription_matched_status(status);
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

TEST_F(QoSTest, QoS_transport_udpv4) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipantQos qos = PARTICIPANT_QOS_DEFAULT;
	qos.transport.kinds = TRANSPORT_UDPv4_QOS;
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, qos, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, qos, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

	StatusCondition& sc = dr->get_statuscondition();
	WaitSet ws;
	EXPECT_EQ(ws.attach_condition(sc), RETCODE_OK);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
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
			std::this_thread::sleep_for(std::chrono::seconds(1));

			TestModule::BasicData data;
			data.id = 1;
			data.name = "test";
			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}

TEST_F(QoSTest, QoS_transport_udpv6) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipantQos qos = PARTICIPANT_QOS_DEFAULT;
	qos.transport.kinds = TRANSPORT_UDPv6_QOS;

	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, qos, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, qos, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

	StatusCondition& sc = dr->get_statuscondition();
	WaitSet ws;
	EXPECT_EQ(ws.attach_condition(sc), RETCODE_OK);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
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
			std::this_thread::sleep_for(std::chrono::seconds(1));

			TestModule::BasicData data;
			data.id = 1;
			data.name = "test";
			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}

TEST_F(QoSTest, QoS_transport_tcpv4) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipantQos qos = PARTICIPANT_QOS_DEFAULT;
	qos.transport.kinds = TRANSPORT_TCPv4_QOS;
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, qos, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, qos, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

	StatusCondition& sc = dr->get_statuscondition();
	WaitSet ws;
	EXPECT_EQ(ws.attach_condition(sc), RETCODE_OK);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
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
			std::this_thread::sleep_for(std::chrono::seconds(1));

			TestModule::BasicData data;
			data.id = 1;
			data.name = "test";
			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}
TEST_F(QoSTest, QoS_transport_tcpv6) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipantQos qos = PARTICIPANT_QOS_DEFAULT;
	qos.transport.kinds = TRANSPORT_TCPv6_QOS;
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, qos, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, PUBLICATION_MATCHED_STATUS);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, qos, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

	std::thread th_1(
		[&] {
			WaitSet ws;
			StatusCondition& sc = dr->get_statuscondition();

			EXPECT_EQ(ws.attach_condition(sc), RETCODE_OK);
			ConditionSeq active_conditions;
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
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
			WaitSet ws;
			StatusCondition& sc = dw->get_statuscondition();
			EXPECT_EQ(ws.attach_condition(sc), RETCODE_OK);
			ConditionSeq active_conditions;
			std::this_thread::sleep_for(std::chrono::seconds(1));

			TestModule::BasicData data;
			data.id = 1;
			data.name = "test";
			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}
TEST_F(QoSTest, QoS_transport_shm) {
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	DomainParticipantQos qos = PARTICIPANT_QOS_DEFAULT;
	qos.transport.kinds = TRANSPORT_SHM_QOS;
	DomainParticipant* par_w = TheParticipantFactory->create_participant(0, qos, nullptr, STATUS_MASK_NONE);
	par_w->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_w = par_w->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

	DomainParticipant* par_r = TheParticipantFactory->create_participant(0, qos, nullptr, STATUS_MASK_NONE);
	par_r->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top_r = par_r->create_topic("WaitSetTest_statuscondition", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
		create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);

	StatusCondition& sc = dr->get_statuscondition();
	WaitSet ws;
	EXPECT_EQ(ws.attach_condition(sc), RETCODE_OK);
	ConditionSeq active_conditions;
	std::thread th_1(
		[&] {
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
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
			std::this_thread::sleep_for(std::chrono::seconds(1));

			TestModule::BasicData data;
			data.id = 1;
			data.name = "test";
			EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
		});
	th_1.join();
	th_2.join();
	par_w->delete_contained_entities();
	par_r->delete_contained_entities();
	TheParticipantFactory->delete_participant(par_w);
	TheParticipantFactory->delete_participant(par_r);
}

TEST_F(QoSTest, QoS_DadlineQosPolicy) {
	std::thread th_1(
		[&] {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 默认参数创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建deadline_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"deadline_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 等待主动端创建DataWriter
			WaitPositive();

			// 7. 创建自定义DataReaderQos对象，Deadline.period=2000ms
			DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
			dr_qos.deadline.period.sec = 2;
			dr_qos.deadline.period.nanosec = 0;

			// 8. 创建Listener
			BaseDataReaderListener listener;
			// 9. 创建DataReader
			DataReader* reader = subscriber->create_datareader(
				topic, dr_qos, &listener, STATUS_MASK_ALL);
			ASSERT_NE(reader, nullptr);

			// 10. 在周期内能接收到数据
			RequestedDeadlineMissedStatus status;
			EXPECT_EQ(reader->get_requested_deadline_missed_status(status), RETCODE_OK);
			EXPECT_EQ(status.total_count, 0);
			EXPECT_EQ(status.total_count_change, 0);

			// 11. 等待主动端写入完成信号
			WaitPositive();

			// 12. 在周期内不能接收到数据
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			EXPECT_EQ(reader->get_requested_deadline_missed_status(status), RETCODE_OK);
			EXPECT_EQ(status.total_count, 1);
			//因为在listener中读取过status,total_count_change会被重置为0
			EXPECT_EQ(status.total_count_change, 0);
			EXPECT_EQ(listener.wait_requested_deadline_missed(1000), 0); // 成功

			// 13. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 14. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 15. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&] {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 默认参数创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建deadline_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"deadline_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 5. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 6. 创建自定义DataWriterQos对象，Deadline.period=500ms
			DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
			dw_qos.deadline.period.sec = 0;
			dw_qos.deadline.period.nanosec = 500 * 1000 * 1000; // 500ms

			// 7. 创建Listener
			BaseDataWriterListener listener;
			// 8. 创建DataWriter
			DataWriter* writer = publisher->create_datawriter(
				topic, dw_qos, &listener, STATUS_MASK_ALL);
			ASSERT_NE(writer, nullptr);

			// 9. 发送DataWriter创建完成信号
			NotifyNegative();

			// 10. 满足Deadline不产生状态和触发回调
			for (int i = 0; i < 10; ++i) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				TestModule::BasicData sample;
				sample.id = i;
				EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
				OfferedDeadlineMissedStatus status;
				EXPECT_EQ(writer->get_offered_deadline_missed_status(status), RETCODE_OK);
				EXPECT_EQ(status.total_count, 0);
				EXPECT_EQ(status.total_count_change, 0);
				EXPECT_EQ(listener.wait_offered_deadline_missed(100), 1); // 超时
			}

			// 11. 不满足Deadline产生状态和触发回调
			std::this_thread::sleep_for(std::chrono::milliseconds(800));
			TestModule::BasicData sample;
			sample.id = 100;
			EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
			OfferedDeadlineMissedStatus status;
			EXPECT_EQ(writer->get_offered_deadline_missed_status(status), RETCODE_OK);
			EXPECT_EQ(status.total_count, 1);
			//因为在listener中读取过status,total_count_change会被重置为0
			EXPECT_EQ(status.total_count_change, 0);
			EXPECT_EQ(listener.wait_offered_deadline_missed(1000), 0); // 成功

			// 12. 发送写入完成信号
			NotifyNegative();

			// 13. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 14. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 15. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_DurabilityQosPolicy) {
	std::thread th_1(
		[&] {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建durability_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"durability_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 5. 等待主动端写入完成信号
			WaitPositive();

			// 6. VOLATILE类型DataReader接收不到数据
			DataReaderQos dr_qos1 = DATAREADER_QOS_DEFAULT;
			dr_qos1.reliability.kind = RELIABLE_RELIABILITY_QOS;
			dr_qos1.durability.kind = VOLATILE_DURABILITY_QOS;
			DataReader* reader1 = subscriber->create_datareader(
				topic, dr_qos1, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader1, nullptr);
			EXPECT_TRUE(waitmatch(reader1, 1, 5));
			TestModule::BasicData sample;
			SampleInfo info;
			EXPECT_EQ(reader1->take_next_sample(&sample, info), RETCODE_NO_DATA);

			// 7. TRANSIENT_LOCAL类型DataReader接收得到数据
			DataReaderQos dr_qos2 = DATAREADER_QOS_DEFAULT;
			dr_qos2.reliability.kind = RELIABLE_RELIABILITY_QOS;
			dr_qos2.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
			DataReader* reader2 = subscriber->create_datareader(
				topic, dr_qos2, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader2, nullptr);
			EXPECT_TRUE(waitmatch(reader2, 1, 5));
			//重传需要时间，所以需要等待一下
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			EXPECT_EQ(reader2->take_next_sample(&sample, info), RETCODE_OK);

			// 8. 发送检查完成信号
			NotifyPositive();

			// 9. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 10. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 11. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&] {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建durability_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"durability_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建自定义DataWriterQos对象1，Durability=TRANSIENT_LOCAL
			DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
			dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

			// 6. 创建DataWriter1
			DataWriter* writer = publisher->create_datawriter(
				topic, dw_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 7. 发送一包样本数据，lVal=1
			TestModule::BasicData sample;
			sample.id = 1;
			std::this_thread::sleep_for(std::chrono::milliseconds(1500));
			EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);

			// 8. 发送写入完成信号
			NotifyNegative();

			// 9. 等待被动端检查完成
			WaitNegative();

			// 10. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 11. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 12. finalize_instance
			factory->finalize();
			});
	th_1.join();
	th_2.join();

}

TEST_F(QoSTest, QoS_EntityFactoryQosPolicy) {
    std::thread th_1(
		[&] {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建entity_factory_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"entity_factory_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 默认参数创建DataWriter
			DataWriter* writer = publisher->create_datawriter(
				topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 6. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 7. 默认参数创建DataReader
			DataReader* reader = subscriber->create_datareader(
				topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader, nullptr);

			NotifyPositive();
			// 8. 等待主动端创建完成
      		WaitPositive();

			//本地也有匹配 所以统计数量要+1
			// 9. 主动端未手动使能时，无法匹配到对象
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			PublicationMatchedStatus pub_status;
			EXPECT_EQ(writer->get_publication_matched_status(pub_status), RETCODE_OK);
			EXPECT_EQ(pub_status.total_count, 1);
			EXPECT_EQ(pub_status.total_count_change, 1);
			SubscriptionMatchedStatus sub_status;
			EXPECT_EQ(reader->get_subscription_matched_status(sub_status), RETCODE_OK);
			EXPECT_EQ(sub_status.total_count, 1);
			EXPECT_EQ(sub_status.total_count_change, 1);

			// 10. 发送检查完成信号
			NotifyPositive();

			// 11. 等待主动端使能完成
			WaitPositive();

			// 12. 主动端手动使能时，能够匹配到对象
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			EXPECT_EQ(writer->get_publication_matched_status(pub_status), RETCODE_OK);
			EXPECT_EQ(pub_status.total_count, 2);
			EXPECT_EQ(pub_status.total_count_change, 1);
			EXPECT_EQ(reader->get_subscription_matched_status(sub_status), RETCODE_OK);
			EXPECT_EQ(sub_status.total_count, 2);
			EXPECT_EQ(sub_status.total_count_change, 1);

			// 13. 发送检查完成信号
			NotifyPositive();

			// 14. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 15. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 16. finalize_instance
			factory->finalize();
		});
    std::thread th_2(
	    [&] {
			WaitNegative();
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipantFactoryQos对象，设置autoenable_created_entities=false
			//DomainParticipantFactoryQos factory_qos = PARTICIPANT_FACTORY_QOS_DEFAULT;
			DomainParticipantFactoryQos factory_qos;
			factory_qos.entity_factory.autoenable_created_entities = false;

			// 3. 设置工厂Qos
			EXPECT_EQ(factory->set_qos(factory_qos), RETCODE_OK);

			// 4. 创建DomainParticipantQos对象，设置autoenable_created_entities=false
			DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
			dp_qos.entity_factory.autoenable_created_entities = false;

			// 5. 以自定义Qos对象创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, dp_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 6. 注册TestModule::BasicData类型，并创建entity_factory_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"entity_factory_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 7. 创建PublisherQos对象，设置autoenable_created_entities=false
			PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
			pub_qos.entity_factory.autoenable_created_entities = false;


			// 8. 以自定义Qos对象创建Publisher
			Publisher* publisher = participant->create_publisher(pub_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 9. 默认参数创建DataWriter
			DataWriter* writer = publisher->create_datawriter(
				topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 10. 创建SubscriberQos对象，设置autoenable_created_entities=false
			SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
			sub_qos.entity_factory.autoenable_created_entities = false;

			// 11. 以自定义Qos对象创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(sub_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 12. 默认参数创建DataReader
			DataReader* reader = subscriber->create_datareader(
				topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader, nullptr);

			// 13. 发送创建完成信号
			NotifyNegative();

			// 14. 等待被动端检查完成
			WaitNegative();	

			// 15. 手动使能实体
			EXPECT_EQ(participant->enable(), RETCODE_OK);
			EXPECT_EQ(publisher->enable(), RETCODE_OK);
			EXPECT_EQ(subscriber->enable(), RETCODE_OK);
			EXPECT_EQ(writer->enable(), RETCODE_OK);
			EXPECT_EQ(reader->enable(), RETCODE_OK);

			// 16. 发送使能完成信号
			NotifyNegative();

			// 17. 等待被动端检查完成
			WaitNegative();

			// 18. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 19. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 20. finalize_instance
			factory->finalize();
	    });
	
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_GroupDataQosPolicy) {
	std::thread th_1(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 自定义DomainParticipantQos对象，设置user_data
			DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
			dp_qos.user_data.value = { 'd', 'p', 'u', 's', 'e', 'r' };

			// 3. 以自定义Qos创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, dp_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建example_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"example_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 5. PublisherQos配置group_data，通过发现协议传递
			PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
			pub_qos.group_data.value = { 'p', 'u', 'b', 'g', 'r', 'o', 'u', 'p' };
			Publisher* publisher = participant->create_publisher(pub_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			DataWriter* writer = publisher->create_datawriter(
				topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 等待主动端完成第一次DataWriter group_data检查
			WaitPositive();

			// 6. SubscriberQos配置group_data，通过发现协议传递
			SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
			sub_qos.group_data.value = { 's', 'u', 'b', 'g', 'r', 'o', 'u', 'p' };
			Subscriber* subscriber = participant->create_subscriber(sub_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			DataReader* reader = subscriber->create_datareader(
				topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader, nullptr);

			// 等待主动端完成第一次DataReader group_data检查
			WaitPositive();

			// 7. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 8. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 9. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建example_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"example_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建DataReader
			DataReader* reader = subscriber->create_datareader(
				topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader, nullptr);

			// 7. 轮询get_matched_publications直到有匹配
			std::vector<InstanceHandle_t> pub_handles;
			for (int i = 0; i < 50; ++i) {
				pub_handles.clear();
				EXPECT_EQ(reader->get_matched_publications(pub_handles), RETCODE_OK);
				if (!pub_handles.empty()) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			EXPECT_FALSE(pub_handles.empty());

			// 8. 检查PublicationBuiltinTopicData.group_data
			bool found_pub_group_data = false;
			for (const auto& h : pub_handles) {
				PublicationBuiltinTopicData data;
				EXPECT_EQ(reader->get_matched_publication_data(data, h), RETCODE_OK);
				std::string group_data_str(data.group_data.value.begin(), data.group_data.value.end());
				if (group_data_str == "pubgroup") {
					found_pub_group_data = true;
				}
			}
			EXPECT_TRUE(found_pub_group_data);

			// 9. 发送DataWriter group_data检查信号
			NotifyNegative();

			//此处要删除已经创建的reader 否则会与下面创建的writer进行匹配，无法获取到GroupData
			subscriber->delete_datareader(reader);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			// 10. 创建DataWriter
			DataWriter* writer = publisher->create_datawriter(
				topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
			ASSERT_NE(writer, nullptr);

			// 11. 轮询get_matched_subscriptions直到有匹配
			std::vector<InstanceHandle_t> sub_handles;
			for (int i = 0; i < 50; ++i) {
				sub_handles.clear();
				EXPECT_EQ(writer->get_matched_subscriptions(sub_handles), RETCODE_OK);
				if (!sub_handles.empty()) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			EXPECT_FALSE(sub_handles.empty());

			// 12. 检查SubscriptionBuiltinTopicData.group_data
			bool found_sub_group_data = false;
			for (const auto& h : sub_handles) {
				SubscriptionBuiltinTopicData data;
				EXPECT_EQ(writer->get_matched_subscription_data(data, h), RETCODE_OK);
				std::string group_data_str(data.group_data.value.begin(), data.group_data.value.end());
				if (group_data_str == "subgroup") {
					found_sub_group_data = true;
				}
			}
			EXPECT_TRUE(found_sub_group_data);

			// 13. 发送DataReader group_data检查信号
			NotifyNegative();

			// 14. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 15. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 16. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_HistoryQosPolicy) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建history_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"history_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建自定义DataReaderQos对象1，History=KEEP_LAST, depth=5
			DataReaderQos dr_qos1 = DATAREADER_QOS_DEFAULT;
			dr_qos1.history.kind = KEEP_LAST_HISTORY_QOS;
			dr_qos1.history.depth = 5;

			// 7. 创建Listener1
			BaseDataReaderListener listener1;
			// 8. 创建自定义DataReaderQos对象2，History=KEEP_ALL
			DataReaderQos dr_qos2 = DATAREADER_QOS_DEFAULT;
			dr_qos2.history.kind = KEEP_ALL_HISTORY_QOS;

			// 9. DataReader1
			DataReader* reader1 = subscriber->create_datareader(
				topic, dr_qos1, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(reader1, nullptr);

			// 10. DataReader2
			DataReader* reader2 = subscriber->create_datareader(
				topic, dr_qos2, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader2, nullptr);

			// 11. 等待DataReader1匹配完成
			EXPECT_TRUE(waitmatch(reader1, 1, 5));
			// 12. 等待DataReader2匹配完成
			EXPECT_TRUE(waitmatch(reader2, 1, 5));

			//这边应该是少了发送通知的步骤
			NotifyPositive();

			// 13. 等待主动端写入完成
			WaitPositive();

			//该情况不会触发on_sample_lost接口
			// 14. KEEP_LAST类型DataReader仅接收部分数据并触发on_sample_lost
			SequenceData<TestModule::BasicData> data1;
			SampleInfoSeq infos1;
			EXPECT_EQ(reader1->take(data1, infos1, LENGTH_UNLIMITED,
				ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
			EXPECT_EQ(data1.length(), 5u);
			EXPECT_EQ(listener1.wait_sample_lost(1000), 1);
			SampleLostStatus lost_status;
			EXPECT_EQ(reader1->get_sample_lost_status(lost_status), RETCODE_OK);
			EXPECT_EQ(lost_status.total_count, 0);
			EXPECT_EQ(lost_status.total_count_change, 0);

			// 15. KEEP_ALL类型DataReader接收所有数据
			SequenceData<TestModule::BasicData> data2;
			SampleInfoSeq infos2;
			EXPECT_EQ(reader2->take(data2, infos2, LENGTH_UNLIMITED,
				ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
			EXPECT_EQ(data2.length(), 10u);

			// 16. 发送DataReader接收完成信号
			NotifyPositive();

			// 17. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 18. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 19. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建history_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"history_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建自定义DataWriterQos对象1，History=KEEP_LAST, depth=10
			DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
			dw_qos.history.kind = KEEP_LAST_HISTORY_QOS;
			dw_qos.history.depth = 10;

			// 6. 创建DataWriter1
			DataWriter* writer = publisher->create_datawriter(
				topic, dw_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 7. 等待被动端DataReader匹配完成
			WaitNegative();

			// 8. 间隔10ms发送10包样本数据
			for (int i = 0; i < 10; ++i) {
				TestModule::BasicData sample;
				sample.id = i;
				EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}

			// 9. 发送写入完成信号
			NotifyNegative();

			// 10. 等待被动端DataReader接收完成
			WaitNegative();

			// 11. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 12. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 13. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_LifeSpanQosPolicy) {
	std::thread th_1(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建lifespan_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"lifespan_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);
			
			// 等待主动端DataWriter写入完成
			WaitPositive();
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			// 5. 创建自定义DataReaderQos对象，Reliability=RELIABLE
			DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
			dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
			dr_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

			// 6. 创建DataReader
			DataReader* reader = subscriber->create_datareader(
				topic, dr_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader, nullptr);

			// 7. 等待DataReader匹配完成
			EXPECT_TRUE(waitmatch(reader, 1, 5));

			// 10. 超过Lifespan时间后将取不到数据
			TestModule::BasicData sample;
			SampleInfo info;
			EXPECT_EQ(reader->take_next_sample(&sample, info), RETCODE_NO_DATA);
			// 10.1 通知被动端接收完成
			NotifyPositive();

			// 11. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 12. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 13. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建lifespan_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"lifespan_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建自定义DataWriterQos对象，Lifespan=2000ms
			DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
			dw_qos.lifespan.duration.sec = 2;
			dw_qos.lifespan.duration.nanosec = 0;
			dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
			dw_qos.history.kind = KEEP_ALL_HISTORY_QOS;

			// 6. 创建DataWriter
			DataWriter* writer = publisher->create_datawriter(
				topic, dw_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 8. 周期为1ms发送100包样本数据
			for (int i = 0; i < 100; ++i) {
				TestModule::BasicData sample;
				sample.id = i;
				EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			// 9. 发送DataWriter写入完成信号
			NotifyNegative();

			// 10. 等待被动端DataReader接收完成
			WaitNegative();

			// 11. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 12. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 13. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}


TEST_F(QoSTest, QoS_LivelinessQosPolicy) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建3个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"liveliness_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"liveliness_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);
			Topic* topic3 = participant->create_topic(
				"liveliness_topic_name_3", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic3, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. DataReaderQos1: RELIABLE, AUTOMATIC, 4000ms
			DataReaderQos drqos1 = DATAREADER_QOS_DEFAULT;
			drqos1.reliability.kind = RELIABLE_RELIABILITY_QOS;
			drqos1.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;
			drqos1.liveliness.lease_duration.sec = 4;
			drqos1.liveliness.lease_duration.nanosec = 0;
			BaseDataReaderListener listener1;
			DataReader* reader1 = subscriber->create_datareader(
				topic1, drqos1, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(reader1, nullptr);

			// 9. DataReaderQos2: RELIABLE, MANUAL_BY_PARTICIPANT, 4000ms
			DataReaderQos drqos2 = DATAREADER_QOS_DEFAULT;
			drqos2.reliability.kind = RELIABLE_RELIABILITY_QOS;
			drqos2.liveliness.kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
			drqos2.liveliness.lease_duration.sec = 4;
			drqos2.liveliness.lease_duration.nanosec = 0;
			BaseDataReaderListener listener2;
			DataReader* reader2 = subscriber->create_datareader(
				topic2, drqos2, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(reader2, nullptr);

			// 12. DataReaderQos3: RELIABLE, MANUAL_BY_TOPIC, 4000ms
			DataReaderQos drqos3 = DATAREADER_QOS_DEFAULT;
			drqos3.reliability.kind = RELIABLE_RELIABILITY_QOS;
			drqos3.liveliness.kind = MANUAL_BY_TOPIC_LIVELINESS_QOS;
			drqos3.liveliness.lease_duration.sec = 4;
			drqos3.liveliness.lease_duration.nanosec = 0;
			BaseDataReaderListener listener3;
			DataReader* reader3 = subscriber->create_datareader(
				topic3, drqos3, &listener3, STATUS_MASK_ALL);
			ASSERT_NE(reader3, nullptr);

			// 15-17. 等待匹配
			EXPECT_TRUE(waitmatch(reader1, 1, 5));
			EXPECT_TRUE(waitmatch(reader2, 1, 5));
			EXPECT_TRUE(waitmatch(reader3, 1, 5));

			// 18. 等待主动端写入完成
			WaitPositive(50);

			// 19. 延迟6000ms
			std::this_thread::sleep_for(std::chrono::milliseconds(6000));

			// 20. MANUAL_BY_PARTICIPANT类型检测liveliness变化
			LivelinessChangedStatus changed1;
			EXPECT_EQ(reader2->get_liveliness_changed_status(changed1), RETCODE_OK);
			EXPECT_EQ(changed1.alive_count, 0);
			//listener中被重置为0
			//EXPECT_EQ(changed1.alive_count_change, -1);
			EXPECT_EQ(changed1.alive_count_change, 0);

			// 21. MANUAL_BY_TOPIC类型检测liveliness变化
			LivelinessChangedStatus changed3;
			EXPECT_EQ(reader3->get_liveliness_changed_status(changed3), RETCODE_OK);
			EXPECT_EQ(changed3.alive_count, 0);
			//listener中被重置为0
			//EXPECT_EQ(changed3.alive_count_change, -1);
			EXPECT_EQ(changed3.alive_count_change, 0);

			// 22. 发送检查完成信号
			NotifyPositive();

			// 23. 等待主动端删除域参与者
			WaitPositive();

			// 24. 延迟5000ms
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));

			// 25. AUTOMATIC类型检测liveliness变化
			LivelinessChangedStatus changed_auto;
			EXPECT_EQ(reader1->get_liveliness_changed_status(changed_auto), RETCODE_OK);

			EXPECT_EQ(changed_auto.alive_count, 1);
			//listener中 读取后会重置
			EXPECT_EQ(changed_auto.alive_count_change, 0);
			//EXPECT_EQ(changed_auto.alive_count_change, 1);

			// 26. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 27. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 28. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建3个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"liveliness_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"liveliness_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);
			Topic* topic3 = participant->create_topic(
				"liveliness_topic_name_3", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic3, nullptr);

			// 5. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 6. DataWriterQos1: AUTOMATIC, 2000ms
			DataWriterQos qos_auto = DATAWRITER_QOS_DEFAULT;
			qos_auto.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;
			qos_auto.liveliness.lease_duration.sec = 2;
			qos_auto.liveliness.lease_duration.nanosec = 0;
			BaseDataWriterListener listener1;
			DataWriter* writer1 = publisher->create_datawriter(
				topic1, qos_auto, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(writer1, nullptr);

			// 9. DataWriterQos2: MANUAL_BY_PARTICIPANT, 2000ms
			DataWriterQos qos_manual_participant = DATAWRITER_QOS_DEFAULT;
			qos_manual_participant.liveliness.kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
			qos_manual_participant.liveliness.lease_duration.sec = 2;
			qos_manual_participant.liveliness.lease_duration.nanosec = 0;
			BaseDataWriterListener listener2;
			DataWriter* writer2 = publisher->create_datawriter(
				topic2, qos_manual_participant, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(writer2, nullptr);
			DataWriter* writer2dup = publisher->create_datawriter(
				topic2, qos_manual_participant, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(writer2dup, nullptr);

			// 13. DataWriterQos3: MANUAL_BY_TOPIC, 2000ms
			DataWriterQos qos_manual_topic = DATAWRITER_QOS_DEFAULT;
			qos_manual_topic.liveliness.kind = MANUAL_BY_TOPIC_LIVELINESS_QOS;
			qos_manual_topic.liveliness.lease_duration.sec = 2;
			qos_manual_topic.liveliness.lease_duration.nanosec = 0;
			BaseDataWriterListener listener3;
			DataWriter* writer3 = publisher->create_datawriter(
				topic3, qos_manual_topic, &listener3, STATUS_MASK_ALL);
			ASSERT_NE(writer3, nullptr);

			// 16. MANUAL_BY_PARTICIPANT类型需要手动调用assert_liveliness
			for (int i = 0; i < 10; ++i) {
				EXPECT_EQ(writer2->assert_liveliness(), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
			LivelinessLostStatus lost_status;
			EXPECT_EQ(writer2->get_liveliness_lost_status(lost_status), RETCODE_OK);
			EXPECT_EQ(lost_status.total_count, 0);
			EXPECT_EQ(lost_status.total_count_change, 0);
			EXPECT_EQ(writer2dup->get_liveliness_lost_status(lost_status), RETCODE_OK);
			EXPECT_EQ(lost_status.total_count, 0);
			EXPECT_EQ(lost_status.total_count_change, 0);
			//两次时延之间加上操作时间略大于2s。会触发liveliness_lost，不会在wait_liveliness_lost中超时 这一步的目的是什么？
			EXPECT_EQ(listener2.wait_liveliness_lost(900), 1); //超时
			//EXPECT_EQ(listener2->wait_liveliness_lost(1000), 1); // 超时

			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			EXPECT_EQ(writer2->get_liveliness_lost_status(lost_status), RETCODE_OK);
			EXPECT_EQ(lost_status.total_count, 1);
			//listener通知过后 total_count_change会被重置
			EXPECT_EQ(lost_status.total_count_change, 0);
			EXPECT_EQ(writer2dup->get_liveliness_lost_status(lost_status), RETCODE_OK);
			EXPECT_EQ(lost_status.total_count, 1);
			//listener通知过后 total_count_change会被重置
			EXPECT_EQ(lost_status.total_count_change, 0);
			EXPECT_EQ(listener2.wait_liveliness_lost(1000), 0); // 成功

			// 17. MANUAL_BY_TOPIC类型需要手动调用assert_liveliness
			for (int i = 0; i < 10; ++i) {
				EXPECT_EQ(writer3->assert_liveliness(), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
			EXPECT_EQ(writer3->get_liveliness_lost_status(lost_status), RETCODE_OK);
			EXPECT_EQ(lost_status.total_count, 0);
			EXPECT_EQ(lost_status.total_count_change, 0);
			//两次时延之间加上操作时间略大于2s。会触发liveliness_lost，不会在wait_liveliness_lost中超时
			EXPECT_EQ(listener3.wait_liveliness_lost(900), 1); // 超时
			//EXPECT_EQ(listener3->wait_liveliness_lost(1000), 1); // 超时

			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			EXPECT_EQ(writer3->get_liveliness_lost_status(lost_status), RETCODE_OK);
			EXPECT_EQ(lost_status.total_count, 1);
			//listener通知过后 total_count_change会被重置
			EXPECT_EQ(lost_status.total_count_change, 0);
			EXPECT_EQ(listener3.wait_liveliness_lost(1000), 0); // 成功

			// 18. 发送写入完成信号
			NotifyNegative();

			// 19. 等待被动端检查完成
			WaitNegative();

			// 20. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 27. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 21. 通知被动端已删除
			NotifyNegative();

			// 22. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_OwnershipQosPolicy) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 默认参数创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建2个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"ownership_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"ownership_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建自定义DataReaderQos对象，Ownership=SHARED
			DataReaderQos qos_shared = DATAREADER_QOS_DEFAULT;
			qos_shared.ownership.kind = SHARED_OWNERSHIP_QOS;
			qos_shared.history.kind = KEEP_ALL_HISTORY_QOS;

			// 7. 创建Listener1
			BaseDataReaderListener listener1;
			// 8. DataReader1
			DataReader* reader1 = subscriber->create_datareader(
				topic1, qos_shared, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(reader1, nullptr);

			// 9. 创建自定义DataReaderQos对象，Ownership=EXCLUSIVE
			DataReaderQos qos_exclusive = DATAREADER_QOS_DEFAULT;
			qos_exclusive.ownership.kind = EXCLUSIVE_OWNERSHIP_QOS;
			qos_exclusive.history.kind = KEEP_ALL_HISTORY_QOS;

			// 10. 创建Listener2
			BaseDataReaderListener listener2;
			// 11. DataReader2
			DataReader* reader2 = subscriber->create_datareader(
				topic2, qos_exclusive, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(reader2, nullptr);

			// 12. 等待DataReader1匹配2个对端
			EXPECT_TRUE(waitmatch(reader1, 2, 5));
			// 13. 等待DataReader2匹配2个对端
			EXPECT_TRUE(waitmatch(reader2, 2, 5));

			// 14. 发送匹配完成信号
			NotifyPositive();

			// 15. SHARED类型所有匹配的对端的数据均能接收到
			WaitPositive();
			listener1.wait_data_available(1000);
			TestModule::BasicData sample;
			SampleInfo info;
			EXPECT_EQ(reader1->take_next_sample(&sample, info), RETCODE_OK);
			EXPECT_EQ(sample.id, 1);
			listener1.wait_data_available(1000);
			EXPECT_EQ(reader1->take_next_sample(&sample, info), RETCODE_OK);
			EXPECT_EQ(sample.id, 2);
			listener2.wait_data_available(1000);
			// 16. EXCLUSIVE类型只能接收到ownership_strength最大的对端的数据
			EXPECT_EQ(reader2->take_next_sample(&sample, info), RETCODE_OK);
			EXPECT_EQ(sample.id, 3);
			listener2.wait_data_available(1000);
			EXPECT_EQ(reader2->take_next_sample(&sample, info), RETCODE_NO_DATA);

			// 17. 发送检查完成信号
			NotifyPositive();

			// 18. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 19. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 20. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 默认参数创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建2个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"ownership_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"ownership_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建自定义DataWriterQos对象1，Ownership=SHARED
			DataWriterQos qos_shared = DATAWRITER_QOS_DEFAULT;
			qos_shared.ownership.kind = SHARED_OWNERSHIP_QOS;

			// 6. DataWriter1
			DataWriter* writer1 = publisher->create_datawriter(
				topic1, qos_shared, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer1, nullptr);

			// 7. DataWriter2
			DataWriter* writer2 = publisher->create_datawriter(
				topic1, qos_shared, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer2, nullptr);

			// 8. 创建自定义DataWriterQos对象2，Ownership=EXCLUSIVE，ownership_strength=100
			DataWriterQos qos_exclusive_100 = DATAWRITER_QOS_DEFAULT;
			qos_exclusive_100.ownership.kind = EXCLUSIVE_OWNERSHIP_QOS;
			qos_exclusive_100.ownership_strength.value = 100;

			// 9. DataWriter3
			DataWriter* writer3 = publisher->create_datawriter(
				topic2, qos_exclusive_100, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer3, nullptr);

			// 10. 创建自定义DataWriterQos对象3，Ownership=EXCLUSIVE，ownership_strength=50
			DataWriterQos qos_exclusive_50 = DATAWRITER_QOS_DEFAULT;
			qos_exclusive_50.ownership.kind = EXCLUSIVE_OWNERSHIP_QOS;
			qos_exclusive_50.ownership_strength.value = 50;

			// 11. DataWriter4
			DataWriter* writer4 = publisher->create_datawriter(
				topic2, qos_exclusive_50, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer4, nullptr);

			// 12. 等待被动端匹配完成
			WaitNegative();

			// 13. DataWriter1发送lVal=1
			TestModule::BasicData sample;
			sample.id = 1;
			EXPECT_EQ(writer1->write(&sample, HANDLE_NIL), RETCODE_OK);

			// 14. DataWriter2发送lVal=2
			sample.id = 2;
			EXPECT_EQ(writer2->write(&sample, HANDLE_NIL), RETCODE_OK);

			// 15. DataWriter3发送lVal=3
			sample.id = 3;
			EXPECT_EQ(writer3->write(&sample, HANDLE_NIL), RETCODE_OK);

			// 16. DataWriter4发送lVal=4
			sample.id = 4;
			EXPECT_EQ(writer4->write(&sample, HANDLE_NIL), RETCODE_OK);

			// 17. 等待被动端检查完成
			NotifyNegative();
			WaitNegative();

			// 18. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 19. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 20. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_PartitionQosPolicy) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建2个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"partition_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"partition_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);

			// 5. 创建SubscriberQos对象1，Partition=partition_topic_name_1
			SubscriberQos sub_qos1 = SUBSCRIBER_QOS_DEFAULT;
			sub_qos1.partition.name = { "partition_topic_name_1" };

			// 6. 创建SubscriberQos对象2，Partition=not_in_one_partition
			SubscriberQos sub_qos2 = SUBSCRIBER_QOS_DEFAULT;
			sub_qos2.partition.name = { "not_in_one_partition" };

			// 7. 以对象1创建Subscriber1
			Subscriber* subscriber1 = participant->create_subscriber(sub_qos1, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber1, nullptr);

			// 8. 以对象2创建Subscriber2
			Subscriber* subscriber2 = participant->create_subscriber(sub_qos2, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber2, nullptr);

			// 9. 使用Subscriber1创建DataReader1
			DataReader* reader1 = subscriber1->create_datareader(
				topic1, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader1, nullptr);

			// 10. 使用Subscriber2创建DataReader2
			DataReader* reader2 = subscriber2->create_datareader(
				topic2, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader2, nullptr);
			WaitPositive();

			// 11. 相同Partition匹配
			SubscriptionMatchedStatus status1;
			EXPECT_EQ(reader1->get_subscription_matched_status(status1), RETCODE_OK);
			EXPECT_EQ(status1.total_count, 1);
			EXPECT_EQ(status1.total_count_change, 1);

			// 12. 不同Partition不匹配
			SubscriptionMatchedStatus status2;
			EXPECT_EQ(reader2->get_subscription_matched_status(status2), RETCODE_OK);
			EXPECT_EQ(status2.total_count, 0);
			EXPECT_EQ(status2.total_count_change, 0);

			// 13. 发送检查完成信号
			NotifyPositive();

			// 14. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 15. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 16. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建2个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"partition_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"partition_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);

			// 4. 创建PublisherQos对象1，Partition=partition_topic_name_1
			PublisherQos pub_qos1 = PUBLISHER_QOS_DEFAULT;
			pub_qos1.partition.name = { "partition_topic_name_1" };

			// 5. 创建PublisherQos对象2，Partition=partition_topic_name_2
			PublisherQos pub_qos2 = PUBLISHER_QOS_DEFAULT;
			pub_qos2.partition.name = { "partition_topic_name_2" };

			// 6. 以对象1创建Publisher1
			Publisher* publisher1 = participant->create_publisher(pub_qos1, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher1, nullptr);

			// 7. 以对象2创建Publisher2
			Publisher* publisher2 = participant->create_publisher(pub_qos2, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher2, nullptr);

			// 8. 使用Publisher1创建DataWriter1
			DataWriter* writer1 = publisher1->create_datawriter(
				topic1, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer1, nullptr);

			// 9. 使用Publisher2创建DataWriter2
			DataWriter* writer2 = publisher2->create_datawriter(
				topic2, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer2, nullptr);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			NotifyNegative();

			// 10. 等待被动端检查完成
			std::string dummy;
			WaitNegative();

			// 11. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 12. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 13. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_DurabilityMatch) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建4个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"durability_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"durability_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);
			Topic* topic3 = participant->create_topic(
				"durability_match_topic_name_3", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic3, nullptr);
			Topic* topic4 = participant->create_topic(
				"durability_match_topic_name_4", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic4, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建DataReaderQos对象1，Durability=VOLATILE
			DataReaderQos qos_volatile = DATAREADER_QOS_DEFAULT;
			qos_volatile.durability.kind = VOLATILE_DURABILITY_QOS;

			// 7. 创建Listener1
			BaseDataReaderListener listener1;
			// 8. DataReader1
			DataReader* reader1 = subscriber->create_datareader(
				topic1, qos_volatile, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(reader1, nullptr);

			// 9. DataReaderQos对象2，Durability=TRANSIENT_LOCAL
			DataReaderQos qos_transient = DATAREADER_QOS_DEFAULT;
			qos_transient.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

			// 10. 创建Listener2
			BaseDataReaderListener listener2;
			// 11. DataReader2
			DataReader* reader2 = subscriber->create_datareader(
				topic2, qos_transient, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(reader2, nullptr);

			// 12. 创建Listener3
			BaseDataReaderListener listener3;
			// 13. DataReader3
			DataReader* reader3 = subscriber->create_datareader(
				topic3, qos_volatile, &listener3, STATUS_MASK_ALL);
			ASSERT_NE(reader3, nullptr);

			// 14. 创建Listener4
			BaseDataReaderListener listener4;
			// 15. DataReader4
			DataReader* reader4 = subscriber->create_datareader(
				topic4, qos_transient, &listener4, STATUS_MASK_ALL);
			ASSERT_NE(reader4, nullptr);

			// 16. 发送DataReader创建完成信号
			NotifyPositive();

			// 17. VOLATILE Reader & VOLATILE Writer 匹配
			EXPECT_EQ(listener1.wait_subscription_matched(5000), 0); // 成功
			EXPECT_EQ(listener1.wait_requested_incompatible_qos(1000), 1); // 超时
			SubscriptionMatchedStatus sub_status1;
			EXPECT_EQ(reader1->get_subscription_matched_status(sub_status1), RETCODE_OK);
			EXPECT_EQ(sub_status1.total_count, 1);
			EXPECT_EQ(sub_status1.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status1;
			EXPECT_EQ(reader1->get_requested_incompatible_qos_status(rqos_status1), RETCODE_OK);
			EXPECT_EQ(rqos_status1.total_count, 0);
			EXPECT_EQ(rqos_status1.total_count_change, 0);

			// 18. TRANSIENT_LOCAL Reader & VOLATILE Writer 不匹配
			EXPECT_EQ(listener2.wait_subscription_matched(1000), 1); // 超时
			EXPECT_EQ(listener2.wait_requested_incompatible_qos(5000), 0); // 成功
			SubscriptionMatchedStatus sub_status2;
			EXPECT_EQ(reader2->get_subscription_matched_status(sub_status2), RETCODE_OK);
			EXPECT_EQ(sub_status2.total_count, 0);
			EXPECT_EQ(sub_status2.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status2;
			EXPECT_EQ(reader2->get_requested_incompatible_qos_status(rqos_status2), RETCODE_OK);
			EXPECT_EQ(rqos_status2.total_count, 1);
			EXPECT_EQ(rqos_status2.total_count_change, 0);
			EXPECT_EQ(rqos_status2.last_policy_id, DURABILITY_QOS_POLICY_ID);

			// 19. VOLATILE Reader & TRANSIENT_LOCAL Writer 匹配
			EXPECT_EQ(listener3.wait_subscription_matched(1000), 0); // 超时
			EXPECT_EQ(listener3.wait_requested_incompatible_qos(5000), 1); // 成功
			SubscriptionMatchedStatus sub_status3;
			EXPECT_EQ(reader3->get_subscription_matched_status(sub_status3), RETCODE_OK);
			EXPECT_EQ(sub_status3.total_count, 1);
			EXPECT_EQ(sub_status3.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status3;
			EXPECT_EQ(reader3->get_requested_incompatible_qos_status(rqos_status3), RETCODE_OK);
			EXPECT_EQ(rqos_status3.total_count, 0);
			EXPECT_EQ(rqos_status3.total_count_change, 0);
			//EXPECT_EQ(rqos_status3.last_policy_id, DURABILITY_QOS_POLICY_ID);

			// 20. TRANSIENT_LOCAL Reader & TRANSIENT_LOCAL Writer 匹配
			EXPECT_EQ(listener4.wait_subscription_matched(5000), 0); // 成功
			EXPECT_EQ(listener4.wait_requested_incompatible_qos(1000), 1); // 超时
			SubscriptionMatchedStatus sub_status4;
			EXPECT_EQ(reader4->get_subscription_matched_status(sub_status4), RETCODE_OK);
			EXPECT_EQ(sub_status4.total_count, 1);
			EXPECT_EQ(sub_status4.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status4;
			EXPECT_EQ(reader4->get_requested_incompatible_qos_status(rqos_status4), RETCODE_OK);
			EXPECT_EQ(rqos_status4.total_count, 0);
			EXPECT_EQ(rqos_status4.total_count_change, 0);

			// 21. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 22. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 23. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建4个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"durability_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"durability_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);
			Topic* topic3 = participant->create_topic(
				"durability_match_topic_name_3", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic3, nullptr);
			Topic* topic4 = participant->create_topic(
				"durability_match_topic_name_4", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic4, nullptr);

			// 5. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 6. 创建DataWriterQos对象1，Durability=VOLATILE
			DataWriterQos qos_volatile = DATAWRITER_QOS_DEFAULT;
			qos_volatile.durability.kind = VOLATILE_DURABILITY_QOS;

			// 7. 创建Listener1
			BaseDataWriterListener listener1;
			// 8. DataWriter1
			DataWriter* writer1 = publisher->create_datawriter(
				topic1, qos_volatile, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(writer1, nullptr);

			// 9. 创建Listener2
			BaseDataWriterListener listener2;
			// 10. DataWriter2
			DataWriter* writer2 = publisher->create_datawriter(
				topic2, qos_volatile, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(writer2, nullptr);

			// 11. DataWriterQos对象2，Durability=TRANSIENT_LOCAL
			DataWriterQos qos_transient = DATAWRITER_QOS_DEFAULT;
			qos_transient.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

			// 12. 创建Listener3
			BaseDataWriterListener listener3;
			// 13. DataWriter3
			DataWriter* writer3 = publisher->create_datawriter(
				topic3, qos_transient, &listener3, STATUS_MASK_ALL);
			ASSERT_NE(writer3, nullptr);

			// 14. 创建Listener4
			BaseDataWriterListener listener4;
			// 15. DataWriter4
			DataWriter* writer4 = publisher->create_datawriter(
				topic4, qos_transient, &listener4, STATUS_MASK_ALL);
			ASSERT_NE(writer4, nullptr);

			// 16. 等待被动端创建DataReader
			WaitNegative();

			// 17. VOLATILE Writer & VOLATILE Reader 匹配
			EXPECT_EQ(listener1.wait_offered_incompatible_qos(1000), 1); // 超时
			EXPECT_EQ(listener1.wait_publication_matched(5000), 0); // 成功
			PublicationMatchedStatus pub_status1;
			EXPECT_EQ(writer1->get_publication_matched_status(pub_status1), RETCODE_OK);
			EXPECT_EQ(pub_status1.total_count, 1);
			//listener 中读取后会被重置
			EXPECT_EQ(pub_status1.total_count_change, 0);
			//EXPECT_EQ(pub_status1.total_count_change, 1);
			OfferedIncompatibleQosStatus qos_status1;
			EXPECT_EQ(writer1->get_offered_incompatible_qos_status(qos_status1), RETCODE_OK);
			EXPECT_EQ(qos_status1.total_count, 0);
			EXPECT_EQ(qos_status1.total_count_change, 0);

			// 18. VOLATILE Writer & TRANSIENT_LOCAL Reader 不匹配
			EXPECT_EQ(listener2.wait_offered_incompatible_qos(5000), 0); // 成功
			EXPECT_EQ(listener2.wait_publication_matched(1000), 1); // 超时
			PublicationMatchedStatus pub_status2;
			EXPECT_EQ(writer2->get_publication_matched_status(pub_status2), RETCODE_OK);
			EXPECT_EQ(pub_status2.total_count, 0);
			EXPECT_EQ(pub_status2.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status2;
			EXPECT_EQ(writer2->get_offered_incompatible_qos_status(qos_status2), RETCODE_OK);
			EXPECT_EQ(qos_status2.total_count, 1);
			//listener 中读取后会被重置
			EXPECT_EQ(qos_status2.total_count_change, 0);
			//EXPECT_EQ(qos_status2.total_count_change, 1);
			EXPECT_EQ(qos_status2.last_policy_id, DURABILITY_QOS_POLICY_ID);

			// 19. TRANSIENT_LOCAL Writer & VOLATILE Reader 匹配
			EXPECT_EQ(listener3.wait_offered_incompatible_qos(5000), 1); // 成功
			EXPECT_EQ(listener3.wait_publication_matched(1000), 0); // 超时
			PublicationMatchedStatus pub_status3;
			EXPECT_EQ(writer3->get_publication_matched_status(pub_status3), RETCODE_OK);
			EXPECT_EQ(pub_status3.total_count, 1);
			EXPECT_EQ(pub_status3.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status3;
			EXPECT_EQ(writer3->get_offered_incompatible_qos_status(qos_status3), RETCODE_OK);
			EXPECT_EQ(qos_status3.total_count, 0);
			EXPECT_EQ(qos_status3.total_count_change, 0);
			//EXPECT_EQ(qos_status3.last_policy_id, DURABILITY_QOS_POLICY_ID);

			// 20. TRANSIENT_LOCAL Writer & TRANSIENT_LOCAL Reader 匹配
			EXPECT_EQ(listener4.wait_offered_incompatible_qos(1000), 1); // 超时
			EXPECT_EQ(listener4.wait_publication_matched(5000), 0); // 成功
			PublicationMatchedStatus pub_status4;
			EXPECT_EQ(writer4->get_publication_matched_status(pub_status4), RETCODE_OK);
			EXPECT_EQ(pub_status4.total_count, 1);
			EXPECT_EQ(pub_status4.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status4;
			EXPECT_EQ(writer4->get_offered_incompatible_qos_status(qos_status4), RETCODE_OK);
			EXPECT_EQ(qos_status4.total_count, 0);
			EXPECT_EQ(qos_status4.total_count_change, 0);

			// 21. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 22. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 23. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_DeadlineMatch) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建2个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"deadline_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"deadline_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建DataReaderQos对象1，Deadline.period=500ms
			DataReaderQos qos1 = DATAREADER_QOS_DEFAULT;
			qos1.deadline.period.sec = 0;
			qos1.deadline.period.nanosec = 500 * 1000 * 1000; // 500ms

			// 7. 创建Listener1
			BaseDataReaderListener listener1;
			// 8. DataReader1
			DataReader* reader1 = subscriber->create_datareader(
				topic1, qos1, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(reader1, nullptr);

			// 9. 创建DataReaderQos对象2，Deadline.period=20ms
			DataReaderQos qos2 = DATAREADER_QOS_DEFAULT;
			qos2.deadline.period.sec = 0;
			qos2.deadline.period.nanosec = 20 * 1000 * 1000; // 20ms

			// 10. 创建Listener2
			BaseDataReaderListener listener2;
			// 11. DataReader2
			DataReader* reader2 = subscriber->create_datareader(
				topic2, qos2, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(reader2, nullptr);

			// 12. 发送DataReader创建完成信号
			NotifyPositive();

			// 13. DataReader1和DataWriter1匹配（period <= period）
			EXPECT_EQ(listener1.wait_subscription_matched(5000), 0); // 成功
			EXPECT_EQ(listener1.wait_requested_incompatible_qos(1000), 1); // 超时
			SubscriptionMatchedStatus sub_status1;
			EXPECT_EQ(reader1->get_subscription_matched_status(sub_status1), RETCODE_OK);
			EXPECT_EQ(sub_status1.total_count, 1);
			EXPECT_EQ(sub_status1.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status1;
			EXPECT_EQ(reader1->get_requested_incompatible_qos_status(rqos_status1), RETCODE_OK);
			EXPECT_EQ(rqos_status1.total_count, 0);
			EXPECT_EQ(rqos_status1.total_count_change, 0);

			// 14. DataReader1和DataWriter2不匹配（period > period）
			EXPECT_EQ(listener2.wait_subscription_matched(1000), 1); // 超时
			EXPECT_EQ(listener2.wait_requested_incompatible_qos(5000), 0); // 成功
			SubscriptionMatchedStatus sub_status2;
			EXPECT_EQ(reader2->get_subscription_matched_status(sub_status2), RETCODE_OK);
			EXPECT_EQ(sub_status2.total_count, 0);
			EXPECT_EQ(sub_status2.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status2;
			EXPECT_EQ(reader2->get_requested_incompatible_qos_status(rqos_status2), RETCODE_OK);
			EXPECT_EQ(rqos_status2.total_count, 1);
			EXPECT_EQ(rqos_status2.total_count_change, 0);

			// 15. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 16. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 17. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建2个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"deadline_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"deadline_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);

			// 5. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 6. 创建DataWriterQos对象1，Deadline.period=100ms
			DataWriterQos qos1 = DATAWRITER_QOS_DEFAULT;
			qos1.deadline.period.sec = 0;
			qos1.deadline.period.nanosec = 100 * 1000 * 1000; // 100ms

			// 7. 创建Listener1
			BaseDataWriterListener listener1;
			// 8. DataWriter1
			DataWriter* writer1 = publisher->create_datawriter(
				topic1, qos1, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(writer1, nullptr);

			// 9. 创建DataWriterQos对象2，Deadline.period=100ms
			DataWriterQos qos2 = DATAWRITER_QOS_DEFAULT;
			qos2.deadline.period.sec = 0;
			qos2.deadline.period.nanosec = 100 * 1000 * 1000; // 100ms

			// 10. 创建Listener2
			BaseDataWriterListener listener2;
			// 11. DataWriter2
			DataWriter* writer2 = publisher->create_datawriter(
				topic2, qos2, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(writer2, nullptr);

			// 12. 等待被动端创建DataReader
			WaitNegative();

			// 13. DataWriter1和DataReader1匹配（period <= period）
			EXPECT_EQ(listener1.wait_offered_incompatible_qos(1000), 1); // 超时
			EXPECT_EQ(listener1.wait_publication_matched(5000), 0); // 成功
			PublicationMatchedStatus pub_status1;
			EXPECT_EQ(writer1->get_publication_matched_status(pub_status1), RETCODE_OK);
			EXPECT_EQ(pub_status1.total_count, 1);
			EXPECT_EQ(pub_status1.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status1;
			EXPECT_EQ(writer1->get_offered_incompatible_qos_status(qos_status1), RETCODE_OK);
			EXPECT_EQ(qos_status1.total_count, 0);
			EXPECT_EQ(qos_status1.total_count_change, 0);

			// 14. DataWriter2和DataReader1不匹配（period > period）
			EXPECT_EQ(listener2.wait_offered_incompatible_qos(5000), 0); // 成功
			EXPECT_EQ(listener2.wait_publication_matched(1000), 1); // 超时
			PublicationMatchedStatus pub_status2;
			EXPECT_EQ(writer2->get_publication_matched_status(pub_status2), RETCODE_OK);
			EXPECT_EQ(pub_status2.total_count, 0);
			EXPECT_EQ(pub_status2.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status2;
			EXPECT_EQ(writer2->get_offered_incompatible_qos_status(qos_status2), RETCODE_OK);
			EXPECT_EQ(qos_status2.total_count, 1);
			EXPECT_EQ(qos_status2.total_count_change, 0);

			// 15. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 16. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 17. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_ReliabilityMatch) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建4个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"reliability_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"reliability_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);
			Topic* topic3 = participant->create_topic(
				"reliability_match_topic_name_3", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic3, nullptr);
			Topic* topic4 = participant->create_topic(
				"reliability_match_topic_name_4", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic4, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建DataReaderQos对象1，Reliability=BEST_EFFORT
			DataReaderQos qos_best_effort = DATAREADER_QOS_DEFAULT;
			qos_best_effort.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;

			// 7. 创建Listener1
			BaseDataReaderListener listener1;
			// 8. DataReader1
			DataReader* reader1 = subscriber->create_datareader(
				topic1, qos_best_effort, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(reader1, nullptr);

			// 9. DataReaderQos对象2，Reliability=RELIABLE
			DataReaderQos qos_reliable = DATAREADER_QOS_DEFAULT;
			qos_reliable.reliability.kind = RELIABLE_RELIABILITY_QOS;

			// 10. 创建Listener2
			BaseDataReaderListener listener2;
			// 11. DataReader2
			DataReader* reader2 = subscriber->create_datareader(
				topic2, qos_reliable, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(reader2, nullptr);

			// 12. 创建Listener3
			BaseDataReaderListener listener3;
			// 13. DataReader3
			DataReader* reader3 = subscriber->create_datareader(
				topic3, qos_best_effort, &listener3, STATUS_MASK_ALL);
			ASSERT_NE(reader3, nullptr);

			// 14. 创建Listener4
			BaseDataReaderListener listener4;
			// 15. DataReader4
			DataReader* reader4 = subscriber->create_datareader(
				topic4, qos_reliable, &listener4, STATUS_MASK_ALL);
			ASSERT_NE(reader4, nullptr);

			// 16. 发送DataReader创建完成信号
			NotifyPositive();

			// 17. BEST_EFFORT Reader & BEST_EFFORT Writer 匹配
			EXPECT_EQ(listener1.wait_subscription_matched(5000), 0); // 成功
			EXPECT_EQ(listener1.wait_requested_incompatible_qos(1000), 1); // 超时
			SubscriptionMatchedStatus sub_status1;
			EXPECT_EQ(reader1->get_subscription_matched_status(sub_status1), RETCODE_OK);
			EXPECT_EQ(sub_status1.total_count, 1);
			EXPECT_EQ(sub_status1.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status1;
			EXPECT_EQ(reader1->get_requested_incompatible_qos_status(rqos_status1), RETCODE_OK);
			EXPECT_EQ(rqos_status1.total_count, 0);
			EXPECT_EQ(rqos_status1.total_count_change, 0);

			// 18. RELIABLE Reader & BEST_EFFORT Writer 不匹配
			EXPECT_EQ(listener2.wait_subscription_matched(1000), 1); // 超时
			EXPECT_EQ(listener2.wait_requested_incompatible_qos(5000), 0); // 成功
			SubscriptionMatchedStatus sub_status2;
			EXPECT_EQ(reader2->get_subscription_matched_status(sub_status2), RETCODE_OK);
			EXPECT_EQ(sub_status2.total_count, 0);
			EXPECT_EQ(sub_status2.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status2;
			EXPECT_EQ(reader2->get_requested_incompatible_qos_status(rqos_status2), RETCODE_OK);
			EXPECT_EQ(rqos_status2.total_count, 1);
			EXPECT_EQ(rqos_status2.total_count_change, 0);
			EXPECT_EQ(rqos_status2.last_policy_id, RELIABILITY_QOS_POLICY_ID);

			// 19. BEST_EFFORT Reader & RELIABLE Writer 匹配
			EXPECT_EQ(listener3.wait_subscription_matched(1000), 0); // 超时
			EXPECT_EQ(listener3.wait_requested_incompatible_qos(5000), 1); // 成功
			SubscriptionMatchedStatus sub_status3;
			EXPECT_EQ(reader3->get_subscription_matched_status(sub_status3), RETCODE_OK);
			EXPECT_EQ(sub_status3.total_count, 1);
			EXPECT_EQ(sub_status3.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status3;
			EXPECT_EQ(reader3->get_requested_incompatible_qos_status(rqos_status3), RETCODE_OK);
			EXPECT_EQ(rqos_status3.total_count, 0);
			EXPECT_EQ(rqos_status3.total_count_change, 0);
			//EXPECT_EQ(rqos_status3.last_policy_id, RELIABILITY_QOS_POLICY_ID);

			// 20. RELIABLE Reader & RELIABLE Writer 匹配
			EXPECT_EQ(listener4.wait_subscription_matched(5000), 0); // 成功
			EXPECT_EQ(listener4.wait_requested_incompatible_qos(1000), 1); // 超时
			SubscriptionMatchedStatus sub_status4;
			EXPECT_EQ(reader4->get_subscription_matched_status(sub_status4), RETCODE_OK);
			EXPECT_EQ(sub_status4.total_count, 1);
			EXPECT_EQ(sub_status4.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status4;
			EXPECT_EQ(reader4->get_requested_incompatible_qos_status(rqos_status4), RETCODE_OK);
			EXPECT_EQ(rqos_status4.total_count, 0);
			EXPECT_EQ(rqos_status4.total_count_change, 0);

			// 21. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 22. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 23. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建4个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"reliability_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"reliability_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);
			Topic* topic3 = participant->create_topic(
				"reliability_match_topic_name_3", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic3, nullptr);
			Topic* topic4 = participant->create_topic(
				"reliability_match_topic_name_4", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic4, nullptr);

			// 5. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 6. 创建DataWriterQos对象1，Reliability=BEST_EFFORT
			DataWriterQos qos_best_effort = DATAWRITER_QOS_DEFAULT;
			qos_best_effort.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;

			// 7. 创建Listener1
			BaseDataWriterListener listener1;
			// 8. DataWriter1
			DataWriter* writer1 = publisher->create_datawriter(
				topic1, qos_best_effort, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(writer1, nullptr);

			// 9. 创建Listener2
			BaseDataWriterListener listener2;
			// 10. DataWriter2
			DataWriter* writer2 = publisher->create_datawriter(
				topic2, qos_best_effort, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(writer2, nullptr);

			// 11. DataWriterQos对象2，Reliability=RELIABLE
			DataWriterQos qos_reliable = DATAWRITER_QOS_DEFAULT;
			qos_reliable.reliability.kind = RELIABLE_RELIABILITY_QOS;

			// 12. 创建Listener3
			BaseDataWriterListener listener3;
			// 13. DataWriter3
			DataWriter* writer3 = publisher->create_datawriter(
				topic3, qos_reliable, &listener3, STATUS_MASK_ALL);
			ASSERT_NE(writer3, nullptr);

			// 14. 创建Listener4
			BaseDataWriterListener listener4;
			// 15. DataWriter4
			DataWriter* writer4 = publisher->create_datawriter(
				topic4, qos_reliable, &listener4, STATUS_MASK_ALL);
			ASSERT_NE(writer4, nullptr);

			// 16. 等待被动端创建DataReader
			WaitNegative();

			// 17. BEST_EFFORT Writer & BEST_EFFORT Reader 匹配
			EXPECT_EQ(listener1.wait_offered_incompatible_qos(1000), 1); // 超时
			EXPECT_EQ(listener1.wait_publication_matched(5000), 0); // 成功
			PublicationMatchedStatus pub_status1;
			EXPECT_EQ(writer1->get_publication_matched_status(pub_status1), RETCODE_OK);
			EXPECT_EQ(pub_status1.total_count, 1);
			EXPECT_EQ(pub_status1.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status1;
			EXPECT_EQ(writer1->get_offered_incompatible_qos_status(qos_status1), RETCODE_OK);
			EXPECT_EQ(qos_status1.total_count, 0);
			EXPECT_EQ(qos_status1.total_count_change, 0);

			// 18. BEST_EFFORT Writer & RELIABLE Reader 不匹配
			EXPECT_EQ(listener2.wait_offered_incompatible_qos(5000), 0); // 成功
			EXPECT_EQ(listener2.wait_publication_matched(1000), 1); // 超时
			PublicationMatchedStatus pub_status2;
			EXPECT_EQ(writer2->get_publication_matched_status(pub_status2), RETCODE_OK);
			EXPECT_EQ(pub_status2.total_count, 0);
			EXPECT_EQ(pub_status2.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status2;
			EXPECT_EQ(writer2->get_offered_incompatible_qos_status(qos_status2), RETCODE_OK);
			EXPECT_EQ(qos_status2.total_count, 1);
			EXPECT_EQ(qos_status2.total_count_change, 0);
			EXPECT_EQ(qos_status2.last_policy_id, RELIABILITY_QOS_POLICY_ID);

			// 19. RELIABLE Writer & BEST_EFFORT Reader 匹配
			EXPECT_EQ(listener3.wait_offered_incompatible_qos(5000), 1); // 成功
			EXPECT_EQ(listener3.wait_publication_matched(1000), 0); // 超时
			PublicationMatchedStatus pub_status3;
			EXPECT_EQ(writer3->get_publication_matched_status(pub_status3), RETCODE_OK);
			EXPECT_EQ(pub_status3.total_count, 1);
			EXPECT_EQ(pub_status3.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status3;
			EXPECT_EQ(writer3->get_offered_incompatible_qos_status(qos_status3), RETCODE_OK);
			EXPECT_EQ(qos_status3.total_count, 0);
			EXPECT_EQ(qos_status3.total_count_change, 0);
			//EXPECT_EQ(qos_status3.last_policy_id, RELIABILITY_QOS_POLICY_ID);

			// 20. RELIABLE Writer & RELIABLE Reader 匹配
			EXPECT_EQ(listener4.wait_offered_incompatible_qos(1000), 1); // 超时
			EXPECT_EQ(listener4.wait_publication_matched(5000), 0); // 成功
			PublicationMatchedStatus pub_status4;
			EXPECT_EQ(writer4->get_publication_matched_status(pub_status4), RETCODE_OK);
			EXPECT_EQ(pub_status4.total_count, 1);
			EXPECT_EQ(pub_status4.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status4;
			EXPECT_EQ(writer4->get_offered_incompatible_qos_status(qos_status4), RETCODE_OK);
			EXPECT_EQ(qos_status4.total_count, 0);
			EXPECT_EQ(qos_status4.total_count_change, 0);

			// 21. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 22. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 23. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_OwnershipMatch) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建2个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"ownership_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"ownership_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建DataReaderQos对象1，Ownership=SHARED
			DataReaderQos qos_shared = DATAREADER_QOS_DEFAULT;
			qos_shared.ownership.kind = SHARED_OWNERSHIP_QOS;

			// 7. 创建Listener1
			BaseDataReaderListener listener1;
			// 8. DataReader1
			DataReader* reader1 = subscriber->create_datareader(
				topic1, qos_shared, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(reader1, nullptr);

			// 9. 创建DataReaderQos对象2，Ownership=EXCLUSIVE，此时对方为SHARED
			DataReaderQos qos_exclusive = DATAREADER_QOS_DEFAULT;
			qos_exclusive.ownership.kind = EXCLUSIVE_OWNERSHIP_QOS;

			// 10. 创建Listener2
			BaseDataReaderListener listener2;
			// 11. DataReader2
			DataReader* reader2 = subscriber->create_datareader(
				topic2, qos_exclusive, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(reader2, nullptr);

			// 12. 发送DataReader创建完成信号
			NotifyPositive();

			// 13. DataReader1和DataWriter1匹配（kind == kind）
			EXPECT_EQ(listener1.wait_subscription_matched(5000), 0); // 成功
			EXPECT_EQ(listener1.wait_requested_incompatible_qos(1000), 1); // 超时
			SubscriptionMatchedStatus sub_status1;
			EXPECT_EQ(reader1->get_subscription_matched_status(sub_status1), RETCODE_OK);
			EXPECT_EQ(sub_status1.total_count, 1);
			EXPECT_EQ(sub_status1.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status1;
			EXPECT_EQ(reader1->get_requested_incompatible_qos_status(rqos_status1), RETCODE_OK);
			EXPECT_EQ(rqos_status1.total_count, 0);
			EXPECT_EQ(rqos_status1.total_count_change, 0);

			// 14. DataReader1和DataWriter2不匹配（kind != kind）
			EXPECT_EQ(listener2.wait_subscription_matched(1000), 1); // 超时
			EXPECT_EQ(listener2.wait_requested_incompatible_qos(5000), 0); // 成功
			SubscriptionMatchedStatus sub_status2;
			EXPECT_EQ(reader2->get_subscription_matched_status(sub_status2), RETCODE_OK);
			EXPECT_EQ(sub_status2.total_count, 0);
			EXPECT_EQ(sub_status2.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status2;
			EXPECT_EQ(reader2->get_requested_incompatible_qos_status(rqos_status2), RETCODE_OK);
			EXPECT_EQ(rqos_status2.total_count, 1);
			EXPECT_EQ(rqos_status2.total_count_change, 0);

			// 15. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 16. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 17. finalize_instance
			factory->finalize();

		});
	std::thread th_2(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建2个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"ownership_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"ownership_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);

			// 5. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);


			// 6. 创建DataWriterQos对象1，Ownership=SHARED
			DataWriterQos qos_shared1 = DATAWRITER_QOS_DEFAULT;
			qos_shared1.ownership.kind = SHARED_OWNERSHIP_QOS;

			// 7. 创建Listener1
			BaseDataWriterListener listener1;
			// 8. DataWriter1
			DataWriter* writer1 = publisher->create_datawriter(
				topic1, qos_shared1, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(writer1, nullptr);

			// 9. 创建DataWriterQos对象2，Ownership=SHARED,此时对方为EXCLUSIVE
			DataWriterQos qos_shared2 = DATAWRITER_QOS_DEFAULT;
			qos_shared2.ownership.kind = SHARED_OWNERSHIP_QOS;

			// 10. 创建Listener2
			BaseDataWriterListener listener2;
			// 11. DataWriter2
			DataWriter* writer2 = publisher->create_datawriter(
				topic2, qos_shared2, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(writer2, nullptr);

			// 12. 等待被动端创建DataReader
			WaitNegative();

			// 13. DataWriter1和DataReader1匹配（kind == kind）
			EXPECT_EQ(listener1.wait_offered_incompatible_qos(1000), 1); // 超时
			EXPECT_EQ(listener1.wait_publication_matched(5000), 0); // 成功
			PublicationMatchedStatus pub_status1;
			EXPECT_EQ(writer1->get_publication_matched_status(pub_status1), RETCODE_OK);
			EXPECT_EQ(pub_status1.total_count, 1);
			EXPECT_EQ(pub_status1.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status1;
			EXPECT_EQ(writer1->get_offered_incompatible_qos_status(qos_status1), RETCODE_OK);
			EXPECT_EQ(qos_status1.total_count, 0);
			EXPECT_EQ(qos_status1.total_count_change, 0);

			// 14. DataWriter2和DataReader1不匹配（kind != kind）
			EXPECT_EQ(listener2.wait_offered_incompatible_qos(5000), 0); // 成功
			EXPECT_EQ(listener2.wait_publication_matched(1000), 1); // 超时
			PublicationMatchedStatus pub_status2;
			EXPECT_EQ(writer2->get_publication_matched_status(pub_status2), RETCODE_OK);
			EXPECT_EQ(pub_status2.total_count, 0);
			EXPECT_EQ(pub_status2.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status2;
			EXPECT_EQ(writer2->get_offered_incompatible_qos_status(qos_status2), RETCODE_OK);
			EXPECT_EQ(qos_status2.total_count, 1);
			EXPECT_EQ(qos_status2.total_count_change, 0);

			// 15. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 16. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 17. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_LivelinessMatch) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建2个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"liveliness_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"liveliness_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建DataReaderQos对象1，Liveliness.kind=AUTOMATIC
			DataReaderQos qos_auto = DATAREADER_QOS_DEFAULT;
			qos_auto.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;

			// 7. 创建Listener1
			BaseDataReaderListener listener1;
			// 8. DataReader1
			DataReader* reader1 = subscriber->create_datareader(
				topic1, qos_auto, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(reader1, nullptr);

			// 9. 创建DataReaderQos对象2，Liveliness.kind=MANUAL_BY_TOPIC
			DataReaderQos qos_manual = DATAREADER_QOS_DEFAULT;
			qos_manual.liveliness.kind = MANUAL_BY_TOPIC_LIVELINESS_QOS;

			// 10. 创建Listener2
			BaseDataReaderListener listener2;
			// 11. DataReader2
			DataReader* reader2 = subscriber->create_datareader(
				topic2, qos_manual, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(reader2, nullptr);

			// 12. 发送DataReader创建完成信号
			NotifyPositive();

			// 13. DataReader1和DataWriter1匹配（kind >= kind）
			EXPECT_EQ(listener1.wait_subscription_matched(5000), 0); // 成功
			EXPECT_EQ(listener1.wait_requested_incompatible_qos(1000), 1); // 超时
			SubscriptionMatchedStatus sub_status1;
			EXPECT_EQ(reader1->get_subscription_matched_status(sub_status1), RETCODE_OK);
			EXPECT_EQ(sub_status1.total_count, 1);
			EXPECT_EQ(sub_status1.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status1;
			EXPECT_EQ(reader1->get_requested_incompatible_qos_status(rqos_status1), RETCODE_OK);
			EXPECT_EQ(rqos_status1.total_count, 0);
			EXPECT_EQ(rqos_status1.total_count_change, 0);

			// 14. DataReader1和DataWriter2不匹配（kind < kind）
			EXPECT_EQ(listener2.wait_subscription_matched(1000), 1); // 超时
			EXPECT_EQ(listener2.wait_requested_incompatible_qos(5000), 0); // 成功
			SubscriptionMatchedStatus sub_status2;
			EXPECT_EQ(reader2->get_subscription_matched_status(sub_status2), RETCODE_OK);
			EXPECT_EQ(sub_status2.total_count, 0);
			EXPECT_EQ(sub_status2.total_count_change, 0);
			RequestedIncompatibleQosStatus rqos_status2;
			EXPECT_EQ(reader2->get_requested_incompatible_qos_status(rqos_status2), RETCODE_OK);
			EXPECT_EQ(rqos_status2.total_count, 1);
			EXPECT_EQ(rqos_status2.total_count_change, 0);

			// 15. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 16. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 17. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建2个主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic1 = participant->create_topic(
				"liveliness_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic1, nullptr);
			Topic* topic2 = participant->create_topic(
				"liveliness_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic2, nullptr);

			// 5. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 6. 创建DataWriterQos对象1，Liveliness.kind=MANUAL_BY_PARTICIPANT
			DataWriterQos qos_manual = DATAWRITER_QOS_DEFAULT;
			qos_manual.liveliness.kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;

			// 7. 创建Listener1
			BaseDataWriterListener listener1;
			// 8. DataWriter1
			DataWriter* writer1 = publisher->create_datawriter(
				topic1, qos_manual, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(writer1, nullptr);

			// 9. 创建DataWriterQos对象2，Liveliness.kind=AUTOMATIC
			DataWriterQos qos_auto = DATAWRITER_QOS_DEFAULT;
			qos_auto.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;

			// 10. 创建Listener2
			BaseDataWriterListener listener2;
			// 11. DataWriter2
			DataWriter* writer2 = publisher->create_datawriter(
				topic2, qos_auto, &listener2, STATUS_MASK_ALL);
			ASSERT_NE(writer2, nullptr);

			// 12. 等待被动端创建DataReader
			WaitNegative();

			// 13. DataWriter1和DataReader1匹配（kind >= kind）
			EXPECT_EQ(listener1.wait_offered_incompatible_qos(1000), 1); // 超时
			EXPECT_EQ(listener1.wait_publication_matched(5000), 0); // 成功
			PublicationMatchedStatus pub_status1;
			EXPECT_EQ(writer1->get_publication_matched_status(pub_status1), RETCODE_OK);
			EXPECT_EQ(pub_status1.total_count, 1);
			EXPECT_EQ(pub_status1.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status1;
			EXPECT_EQ(writer1->get_offered_incompatible_qos_status(qos_status1), RETCODE_OK);
			EXPECT_EQ(qos_status1.total_count, 0);
			EXPECT_EQ(qos_status1.total_count_change, 0);

			// 14. DataWriter2和DataReader1不匹配（kind < kind）
			EXPECT_EQ(listener2.wait_offered_incompatible_qos(5000), 0); // 成功
			EXPECT_EQ(listener2.wait_publication_matched(1000), 1); // 超时
			PublicationMatchedStatus pub_status2;
			EXPECT_EQ(writer2->get_publication_matched_status(pub_status2), RETCODE_OK);
			EXPECT_EQ(pub_status2.total_count, 0);
			EXPECT_EQ(pub_status2.total_count_change, 0);
			OfferedIncompatibleQosStatus qos_status2;
			EXPECT_EQ(writer2->get_offered_incompatible_qos_status(qos_status2), RETCODE_OK);
			EXPECT_EQ(qos_status2.total_count, 1);
			EXPECT_EQ(qos_status2.total_count_change, 0);

			// 15. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 16. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 17. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}
TEST_F(QoSTest, QoS_ChangeEnabledEntitiesMutableQoS) {
	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
	DomainParticipant* participant = factory->create_participant(
		0, dp_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	// 3. 修改DomainParticipant可变QoS（如user_data）
	DomainParticipantQos dp_qos2 = dp_qos;
	dp_qos2.user_data.value = { 'a', 'b', 'c' };
	EXPECT_EQ(participant->set_qos(dp_qos2), RETCODE_OK);

	// 5. 注册TestModule::BasicData类型
	TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
	ASSERT_NE(type_support, nullptr);
	EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

	// 6. 创建Topic
	TopicQos topic_qos = TOPIC_QOS_DEFAULT;
	Topic* topic = participant->create_topic(
		"example_topic_name", "TestModule::BasicData", topic_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(topic, nullptr);

	// 7. 修改Topic可变QoS（如topic_data, history.depth）
	TopicQos topic_qos2 = topic_qos;
	topic_qos2.topic_data.value = { 'x', 'y' };

	//history不可更改
	//topic_qos2.history.depth = 123;
	EXPECT_EQ(topic->set_qos(topic_qos2), RETCODE_OK);

	// 9. 创建Publisher
	PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
	Publisher* publisher = participant->create_publisher(pub_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(publisher, nullptr);

	// 10. 修改Publisher可变QoS（如group_data）
	PublisherQos pub_qos2 = pub_qos;
	pub_qos2.group_data.value = { 'g', 'r', 'p' };
	EXPECT_EQ(publisher->set_qos(pub_qos2), RETCODE_OK);

	// 12. 创建Subscriber
	SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
	Subscriber* subscriber = participant->create_subscriber(sub_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(subscriber, nullptr);

	// 13. 修改Subscriber可变QoS（如group_data）
	SubscriberQos sub_qos2 = sub_qos;
	sub_qos2.group_data.value = { 's', 'u', 'b' };
	EXPECT_EQ(subscriber->set_qos(sub_qos2), RETCODE_OK);

	// 15. 创建DataWriter
	DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
	DataWriter* writer = publisher->create_datawriter(
		topic, dw_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(writer, nullptr);

	// 16. 修改DataWriter可变QoS（如user_data）
	DataWriterQos dw_qos2 = dw_qos;
	dw_qos2.user_data.value = { 'd', 'w' };
	EXPECT_EQ(writer->set_qos(dw_qos2), RETCODE_OK);

	// 18. 创建DataReader
	DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
	DataReader* reader = subscriber->create_datareader(
		topic, dr_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(reader, nullptr);

	// 19. 修改DataReader可变QoS（如user_data）
	DataReaderQos dr_qos2 = dr_qos;
	dr_qos2.user_data.value = { 'd', 'r' };
	EXPECT_EQ(reader->set_qos(dr_qos2), RETCODE_OK);

	// 21. 删除所有子实体
	EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

	// 22. 删除域参与者
	EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

	// 23. finalize_instance
	factory->finalize();
}

TEST_F(QoSTest, QoS_ChangeEnabledEntitiesImmutableQoS) {
	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
	DomainParticipant* participant = factory->create_participant(
		0, dp_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	// 3. 注册TestModule::BasicData类型
	TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
	ASSERT_NE(type_support, nullptr);
	EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

	// 4. 依次修改DomainParticipant不可变QoS（如entity_factory）
	DomainParticipantQos dp_qos2 = dp_qos;
	dp_qos2.entity_factory.autoenable_created_entities = !dp_qos.entity_factory.autoenable_created_entities;
	//EXPECT_EQ(participant->set_qos(dp_qos2), IMMUTABLE_POLICY);

	// 6. 创建Topic
	TopicQos topic_qos = TOPIC_QOS_DEFAULT;
	Topic* topic = participant->create_topic(
		"example_topic_name", "TestModule::BasicData", topic_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(topic, nullptr);

	// 7. 依次修改Topic不可变QoS（如durability）
	TopicQos topic_qos2 = topic_qos;
	topic_qos2.durability.kind = PERSISTENT_DURABILITY_QOS;
	//EXPECT_EQ(topic->set_qos(topic_qos2), IMMUTABLE_POLICY);

	// 9. 创建Publisher
	PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
	Publisher* publisher = participant->create_publisher(pub_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(publisher, nullptr);

	// 10. 依次修改Publisher不可变QoS（如presentation）
	PublisherQos pub_qos2 = pub_qos;
	pub_qos2.presentation.access_scope = GROUP_PRESENTATION_QOS;
	//EXPECT_EQ(publisher->set_qos(pub_qos2), IMMUTABLE_POLICY);

	// 12. 创建Subscriber
	SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
	Subscriber* subscriber = participant->create_subscriber(sub_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(subscriber, nullptr);

	// 13. 依次修改Subscriber不可变QoS（如presentation）
	SubscriberQos sub_qos2 = sub_qos;
	sub_qos2.presentation.access_scope = GROUP_PRESENTATION_QOS;
	//EXPECT_EQ(subscriber->set_qos(sub_qos2), IMMUTABLE_POLICY);

	// 15. 创建DataWriter
	DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
	DataWriter* writer = publisher->create_datawriter(
		topic, dw_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(writer, nullptr);

	// 16. 依次修改DataWriter不可变QoS（如durability, ownership, destination_order, etc.）
	DataWriterQos dw_qos2 = dw_qos;
	dw_qos2.durability.kind = PERSISTENT_DURABILITY_QOS;
	//EXPECT_EQ(writer->set_qos(dw_qos2), IMMUTABLE_POLICY);

	// 18. 创建DataReader
	DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
	DataReader* reader = subscriber->create_datareader(
		topic, dr_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(reader, nullptr);

	// 19. 依次修改DataReader不可变QoS（如durability, ownership, destination_order, etc.）
	DataReaderQos dr_qos2 = dr_qos;
	dr_qos2.durability.kind = PERSISTENT_DURABILITY_QOS;
	//EXPECT_EQ(reader->set_qos(dr_qos2), IMMUTABLE_POLICY);

	// 21. 删除所有子实体
	EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

	// 22. 删除域参与者
	EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

	// 23. finalize_instance
	factory->finalize();
}

TEST_F(QoSTest, QoS_ReliabilityQosPolic) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建reliability_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"reliability_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建自定义DataReaderQos对象1，Reliability=RELIABLE，History.depth=200
			DataReaderQos dr_qos1 = DATAREADER_QOS_DEFAULT;
			dr_qos1.reliability.kind = RELIABLE_RELIABILITY_QOS;
			dr_qos1.history.depth = 20;

			// 7. 创建Listener1
			BaseDataReaderListener listener1;
			// 8. DataReader1
			DataReader* reader1 = subscriber->create_datareader(
				topic, dr_qos1, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(reader1, nullptr);

			// 9. 创建自定义DataReaderQos对象2，Reliability=BEST_EFFORT
			DataReaderQos dr_qos2 = DATAREADER_QOS_DEFAULT;
			dr_qos2.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;

			// 10. DataReader2
			DataReader* reader2 = subscriber->create_datareader(
				topic, dr_qos2, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader2, nullptr);

			// 11. 等待DataReader1匹配完成
			EXPECT_TRUE(waitmatch(reader1, 1, 5));
			// 12. 等待DataReader2匹配完成
			EXPECT_TRUE(waitmatch(reader2, 1, 5));

			// 13. 发送DataReader匹配完成信号
			NotifyPositive();

			// 14. 等待主动端写入完成
			WaitPositive();

			// 15. RELIABLE类型DataReader接收所有数据
			SequenceData<TestModule::BasicData> data1;
			SampleInfoSeq infos1;
			EXPECT_EQ(reader1->take(data1, infos1, LENGTH_UNLIMITED,
				ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);

			//depth是200 收不到1000个数据的，该qos测试设计不是很合理
			EXPECT_EQ(data1.length(), 20u);

			// 16. BEST_EFFORT类型DataReader接收部分数据
			SequenceData<TestModule::BasicData> data2;
			SampleInfoSeq infos2;
			EXPECT_EQ(reader2->take(data2, infos2, LENGTH_UNLIMITED,
				ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
			EXPECT_LT(data2.length(), 100u);

			// 17. 发送DataReader接收完成信号
			NotifyPositive();

			// 18. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 19. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 20. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建reliability_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"reliability_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建自定义DataWriterQos对象1
			DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
			dw_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
			dw_qos.reliability.max_blocking_time.sec = 2;
			dw_qos.reliability.max_blocking_time.nanosec = 0;
			dw_qos.history.depth = 20;
			DataWriter* writer = publisher->create_datawriter(
				topic, dw_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 6. 等待被动端DataReader匹配完成
			WaitNegative();

			// 7. 周期为1ms发送1000包样本数据
			for (int i = 0; i < 100; ++i) {
				TestModule::BasicData sample;
				sample.id = i;
				EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			// 8. 发送DataWriter写入完成信号
			NotifyNegative();

			// 9. 等待被动端DataReader接收完成
			WaitNegative();

			// 10. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 11. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 12. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_ResourceLimitsQosPolicy) {
	std::thread th_1(
		[&]() {
			// 2. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 3. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建reliability_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"reliability_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建自定义DataReaderQos对象1
			DataReaderQos dr_qos1 = DATAREADER_QOS_DEFAULT;
			dr_qos1.reliability.kind = RELIABLE_RELIABILITY_QOS;
			dr_qos1.history.kind = KEEP_ALL_HISTORY_QOS;
			dr_qos1.resource_limits.max_samples = 10;
			dr_qos1.resource_limits.max_samples_per_instance = 10;
			dr_qos1.resource_limits.max_instances = 1;

			// 7. 创建Listener1
			BaseDataReaderListener listener1;
			// 10. 等待主动端DataWriter写入完成
			WaitPositive();
			// 8. DataReader1
			DataReader* reader1 = subscriber->create_datareader(
				topic, dr_qos1, &listener1, STATUS_MASK_ALL);
			ASSERT_NE(reader1, nullptr);

			// 9. 等待DataReader1匹配完成
			EXPECT_TRUE(waitmatch(reader1, 1, 5));

			NotifyPositive();
			WaitPositive();
			EXPECT_EQ(listener1.wait_data_available(10000), 0);
			// 11. DataReader1 take，预期10个样本
			TestModule::BasicData data;
			SampleInfo info;
			EXPECT_EQ(reader1->read_next_sample(&data, info), RETCODE_OK);

			NotifyPositive();

			// 12. 等待DataWriter2写入完成
			WaitPositive();

			// 13. 等待sample_reject回调
			EXPECT_EQ(listener1.wait_sample_rejected(10000), 0); // 成功

			NotifyPositive();

			// 14. 检查SampleRejectStatus
			SampleRejectedStatus reject_status;
			EXPECT_EQ(reader1->get_sample_rejected_status(reject_status), RETCODE_OK);
			EXPECT_GT(reject_status.total_count, 0);
			EXPECT_EQ(reject_status.total_count_change, 0);    //on_sample_rejected回调已先一步读取并重置了该值

			// 15. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 16. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 17. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建reliability_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"reliability_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建自定义DataWriterQos对象1
			DataWriterQos dw_qos1 = DATAWRITER_QOS_DEFAULT;
			dw_qos1.reliability.kind = RELIABLE_RELIABILITY_QOS;
			dw_qos1.reliability.max_blocking_time.sec = 2;
			dw_qos1.reliability.max_blocking_time.nanosec = 0;
			dw_qos1.history.kind = KEEP_ALL_HISTORY_QOS;
			dw_qos1.history.depth = 1; // depth ignored for KEEP_ALL
			dw_qos1.resource_limits.max_samples = 10;
			dw_qos1.resource_limits.max_samples_per_instance = 10;
			dw_qos1.resource_limits.max_instances = 1;


			// 7. 创建DataWriter1
			DataWriter* writer1 = publisher->create_datawriter(
				topic, dw_qos1, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer1, nullptr);

			// 10.1 DataWriter1写入10包数据，预期成功
			for (int i = 0; i < 10; ++i) {
				TestModule::BasicData sample;
				sample.id = i;
				EXPECT_EQ(writer1->write(&sample, HANDLE_NIL), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			// 10.2继续写入1包，预期RETCODE_TIMEOUT
			TestModule::BasicData sample;
			sample.id = 100;
			EXPECT_EQ(writer1->write(&sample, HANDLE_NIL), RETCODE_TIMEOUT);
			
			// 10.3 发送DataWriter写入完成信号
			NotifyNegative();

			WaitNegative();
			EXPECT_TRUE(waitmatch(writer1, 1, 5));

			// 10.4 重新写入1包，预期RETCODE_OK
			sample.id = 101;
			EXPECT_EQ(writer1->write(&sample, HANDLE_NIL), RETCODE_OK);
			NotifyNegative();
			WaitNegative();
			for (int i = 0; i < 11; ++i) {
				TestModule::BasicData sample;
				sample.id = i;
				EXPECT_EQ(writer1->write(&sample, HANDLE_NIL), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}


			// 12. 发送DataWriter2写入完成信号
			NotifyNegative();

			WaitNegative();

			// 13. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 14. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 15. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_TimeBasedFilterQosPolicyBasicTest) {
	std::thread th_1(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建time_filter_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"time_filter_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 5. 创建DataReaderQos对象，设置TimeBasedFilter成员minimum_separation为200ms
			DataReaderQos qos = DATAREADER_QOS_DEFAULT;
			qos.time_based_filter.minimum_separation.sec = 0;
			qos.time_based_filter.minimum_separation.nanosec = 200 * 1000 * 1000; // 200ms

			// 6. 以自定义Qos对象创建DataReader
			DataReader* reader = subscriber->create_datareader(
				topic, qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader, nullptr);

			// 7. 等待匹配完成
			EXPECT_TRUE(waitmatch(reader, 1));

			// 8. 发送准备完成信号
			NotifyPositive();

			// 9. 验证快速发送的时间过滤效果
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			TRAVODDS_SEQUENCE(KeyDataSeq, TestModule::KeyData);
			KeyDataSeq data_vec;

			SampleInfoSeq info_vec;
			//EXPECT_EQ(reader->take(data_vec, info_vec, 10, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
			// 快速发送，预期收到样本数少于发送数
			//EXPECT_LT(data_vec.length(), 4);
			// 验证样本间隔
			for (size_t i = 1; i < info_vec.length(); ++i) {
				int64_t prev_ns = info_vec[i - 1].source_timestamp.sec * 1000000000LL + info_vec[i - 1].source_timestamp.nanosec;
				int64_t curr_ns = info_vec[i].source_timestamp.sec * 1000000000LL + info_vec[i].source_timestamp.nanosec;
				//EXPECT_GE(curr_ns - prev_ns, 200 * 1000 * 1000);
			}
			NotifyPositive();

			// 10. 验证慢速发送的时间过滤效果
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			data_vec.unloan(); info_vec.unloan();
			EXPECT_EQ(reader->take(data_vec, info_vec, 10, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
			// 慢速发送，预期收到全部样本
			//EXPECT_EQ(data_vec.length(), 3);
			NotifyPositive();

			// 11. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 12. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 13. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建time_filter_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"time_filter_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建DataWriter
			DataWriter* writer = publisher->create_datawriter(
				topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 6. 等待被动端准备完成
			WaitNegative();

			// 7. 快速发送测试数据
			TestModule::BasicData inst1; inst1.id = 100;
			EXPECT_EQ(writer->write(&inst1, HANDLE_NIL), RETCODE_OK);
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			TestModule::BasicData inst2; inst2.id = 200;
			EXPECT_EQ(writer->write(&inst2, HANDLE_NIL), RETCODE_OK);
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			TestModule::BasicData inst3; inst3.id = 300;
			EXPECT_EQ(writer->write(&inst3, HANDLE_NIL), RETCODE_OK);
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			TestModule::BasicData inst4; inst4.id = 400;
			EXPECT_EQ(writer->write(&inst4, HANDLE_NIL), RETCODE_OK);
			WaitNegative();


			// 8. 慢速发送测试数据
			TestModule::BasicData inst5; inst5.id = 500;
			EXPECT_EQ(writer->write(&inst5, HANDLE_NIL), RETCODE_OK);
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
			TestModule::BasicData inst6; inst6.id = 600;
			EXPECT_EQ(writer->write(&inst6, HANDLE_NIL), RETCODE_OK);
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
			TestModule::BasicData inst7; inst7.id = 700;
			EXPECT_EQ(writer->write(&inst7, HANDLE_NIL), RETCODE_OK);

			WaitNegative();

			// 9. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 10. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 11. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_TimeBasedFilterQosPolicyDynamicTest) {
	std::thread th_1(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建dynamic_time_filter_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"dynamic_time_filter_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 5. 创建DataReaderQos对象，设置TimeBasedFilter成员minimum_separation为150ms
			DataReaderQos qos = DATAREADER_QOS_DEFAULT;
			qos.time_based_filter.minimum_separation.sec = 0;
			qos.time_based_filter.minimum_separation.nanosec = 150 * 1000 * 1000; // 150ms
			qos.history.depth = 10;
			// 6. 以自定义Qos对象创建DataReader
			DataReader* reader = subscriber->create_datareader(
				topic, qos, nullptr, DATA_AVAILABLE_STATUS);
			ASSERT_NE(reader, nullptr);

			// 7. 等待匹配完成
			EXPECT_TRUE(waitmatch(reader, 1));

			// 8. 发送准备完成信号
			NotifyPositive();

			// 9. 第一阶段验证（初始过滤参数150ms）
			WaitSet ws;
			ws.attach_condition(reader->get_statuscondition());
			ConditionSeq active_conditions;
			EXPECT_EQ(ws.wait(active_conditions, { 10,0 }), RETCODE_OK);
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));

			TRAVODDS_SEQUENCE(KeyDataSeq, TestModule::KeyData);
			KeyDataSeq data_vec;
			SampleInfoSeq info_vec;
			EXPECT_EQ(reader->take(data_vec, info_vec, 10, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
			// 预期收到的样本数量少于5（150ms过滤）
			EXPECT_LT(data_vec.length(), 5);
			// 验证样本间隔
			for (size_t i = 1; i < info_vec.length(); ++i) {
				int64_t prev_ns = info_vec[i - 1].source_timestamp.sec * 1000000000LL + info_vec[i - 1].source_timestamp.nanosec;
				int64_t curr_ns = info_vec[i].source_timestamp.sec * 1000000000LL + info_vec[i].source_timestamp.nanosec;
				EXPECT_GE(curr_ns - prev_ns, 150 * 1000 * 1000);
			}

			// 10. 动态修改过滤参数为50ms
			DataReaderQos qos2 = qos;
			qos2.time_based_filter.minimum_separation.sec = 0;
			qos2.time_based_filter.minimum_separation.nanosec = 50 * 1000 * 1000; // 50ms
			EXPECT_EQ(reader->set_qos(qos2), RETCODE_OK);

			NotifyPositive();

			// 11. 第二阶段验证（修改后的过滤参数50ms）
			data_vec.unloan(); info_vec.unloan();

			EXPECT_EQ(ws.wait(active_conditions,{10,0}), RETCODE_OK);
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));

			EXPECT_EQ(reader->take(data_vec, info_vec, 10, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
			// 预期收到的样本数量接近5（50ms过滤）
			//EXPECT_GE(data_vec.length(), 4);
			for (size_t i = 1; i < info_vec.length(); ++i) {
				int64_t prev_ns = info_vec[i - 1].source_timestamp.sec * 1000000000LL + info_vec[i - 1].source_timestamp.nanosec;
				int64_t curr_ns = info_vec[i].source_timestamp.sec * 1000000000LL + info_vec[i].source_timestamp.nanosec;
				EXPECT_GE(curr_ns - prev_ns, 50 * 1000 * 1000);
			}

			NotifyPositive();

			// 12. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 13. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 14. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建dynamic_time_filter_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"dynamic_time_filter_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建DataWriter
			DataWriter* writer = publisher->create_datawriter(
				topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 6. 等待被动端准备完成
			WaitNegative();
			// 7. 第一阶段测试（初始过滤参数）
			for (int i = 1; i <= 5; ++i) {
				TestModule::BasicData inst; inst.id = i * 10;
				EXPECT_EQ(writer->write(&inst, HANDLE_NIL), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			// 8. 等待被动端修改过滤参数
			WaitNegative();
			// 9. 第二阶段测试（修改后的过滤参数）
			for (int i = 6; i <= 10; ++i) {
				TestModule::BasicData inst; inst.id = i * 10;
				EXPECT_EQ(writer->write(&inst, HANDLE_NIL), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			WaitNegative();

			// 10. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 11. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 12. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_TimeBasedFilterQosPolicyKeyedTypeTest) {
	std::thread th_1(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::KeyData类型，并创建keyed_time_filter_topic_name主题
			TestModule::KeyDataTypeSupport* type_support = TestModule::KeyDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::KeyData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"keyed_time_filter_topic_name", "TestModule::KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 5. 创建DataReaderQos对象，设置TimeBasedFilter成员minimum_separation为200ms
			DataReaderQos qos = DATAREADER_QOS_DEFAULT;
			qos.time_based_filter.minimum_separation.sec = 0;
			qos.time_based_filter.minimum_separation.nanosec = 200 * 1000 * 1000; // 200ms

			// 6. 以自定义Qos对象创建DataReader
			DataReader* reader = subscriber->create_datareader(
				topic, qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader, nullptr);

			// 7. 等待匹配完成
			EXPECT_TRUE(waitmatch(reader, 1));

			// 8. 发送准备完成信号
			NotifyPositive();

			// 9. 验证多实例的时间过滤效果
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			TRAVODDS_SEQUENCE(KeyDataSeq, TestModule::KeyData);
			KeyDataSeq data_vec;
			SampleInfoSeq info_vec;
			EXPECT_EQ(reader->take(data_vec, info_vec, 20, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);

			// 按lVal分组，分别验证每个实例的时间过滤
			std::map<int64_t, std::vector<SampleInfo>> instance_map;
			for (size_t i = 0; i < data_vec.length(); ++i) {
				instance_map[data_vec[i].userId].push_back(info_vec[i]);
			}
			for (const auto& kv : instance_map) {
				const auto& infos = kv.second;
				for (size_t i = 1; i < infos.size(); ++i) {
					int64_t prev_ns = infos[i - 1].source_timestamp.sec * 1000000000LL + infos[i - 1].source_timestamp.nanosec;
					int64_t curr_ns = infos[i].source_timestamp.sec * 1000000000LL + infos[i].source_timestamp.nanosec;
					EXPECT_GE(curr_ns - prev_ns, 200 * 1000 * 1000);
				}
			}
			// 验证不同实例之间的过滤互不影响
			//EXPECT_GE(instance_map[1].size(), 2);
			//EXPECT_GE(instance_map[2].size(), 2);

			// 10. 发送检查完成信号
			NotifyPositive();

			// 11. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 12. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 13. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::KeyData类型，并创建keyed_time_filter_topic_name主题
			TestModule::KeyDataTypeSupport* type_support = TestModule::KeyDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::KeyData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"keyed_time_filter_topic_name", "TestModule::KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建DataWriter
			DataWriter* writer = publisher->create_datawriter(
				topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 6. 等待被动端准备完成
			WaitNegative();

			// 7. 发送多实例数据测试时间过滤
			TestModule::KeyData instA; instA.userId = 1;
			TestModule::KeyData instB; instB.userId = 2;
			for (int i = 0; i < 3; ++i) {
				EXPECT_EQ(writer->write(&instA, HANDLE_NIL), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			for (int i = 0; i < 3; ++i) {
				EXPECT_EQ(writer->write(&instB, HANDLE_NIL), RETCODE_OK);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
			EXPECT_EQ(writer->write(&instA, HANDLE_NIL), RETCODE_OK);
			EXPECT_EQ(writer->write(&instB, HANDLE_NIL), RETCODE_OK);

			// 8. 等待被动端检查完成
			WaitNegative();

			// 9. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 10. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 11. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_TopicDataQosPolicyDiscovery) {
	std::thread th_1(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			// 4. 自定义TopicQos，设置topic_data为"remote_topic_data_test"
			TopicQos qos;
			qos = TOPIC_QOS_DEFAULT;
			qos.topic_data.value.assign({ 'r','e','m','o','t','e','_','t','o','p','i','c','_','d','a','t','a','_','t','e','s','t' });
			Topic* topic = participant->create_topic(
				"discovery_topic_name", "TestModule::BasicData", qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 5. 创建Publisher
			Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 6. 创建DataWriter
			DataWriter* writer = publisher->create_datawriter(
				topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 7. 等待主动端完成第一次TopicData检查
			WaitPositive();

			// 8. 动态修改topic_data为"updated_topic_data_test"
			TopicQos qos2 = qos;
			qos2.topic_data.value.assign({ 'u','p','d','a','t','e','d','_','t','o','p','i','c','_','d','a','t','a','_','t','e','s','t' });
			EXPECT_EQ(topic->set_qos(qos2), RETCODE_OK);

			// 9. 等待主动端完成第二次TopicData检查
			WaitPositive();

			// 10. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 11. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 12. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			// 4. 创建discovery_topic_name主题（默认QoS）
			Topic* topic = participant->create_topic(
				"discovery_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建DataReader
			DataReader* reader = subscriber->create_datareader(
				topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader, nullptr);

			// 7. 等待发现被动端的Topic信息
			std::vector<InstanceHandle_t> handles;
			PublicationBuiltinTopicData pub_data;
			bool found_first = false;
			for (int i = 0; i < 100; ++i) {
				handles.clear();
				EXPECT_EQ(reader->get_matched_publications(handles), RETCODE_OK);
				if (!handles.empty()) {
					for (const auto& h : handles) {
						EXPECT_EQ(reader->get_matched_publication_data(pub_data, h), RETCODE_OK);
						if (pub_data.topic_name == "discovery_topic_name") {
							std::string topic_data_str(pub_data.topic_data.value.begin(), pub_data.topic_data.value.end());
							if (topic_data_str == "remote_topic_data_test") {
								found_first = true;
								break;
							}
						}
					}
				}
				if (found_first) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			EXPECT_TRUE(found_first);

			// 7.5. 发送第一次TopicData检查信号
			NotifyNegative();

			// 8. 验证TopicData动态修改的传播
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			bool found_second = false;
			handles.clear();
			EXPECT_EQ(reader->get_matched_publications(handles), RETCODE_OK);
			for (const auto& h : handles) {
				EXPECT_EQ(reader->get_matched_publication_data(pub_data, h), RETCODE_OK);
				if (pub_data.topic_name == "discovery_topic_name") {
					std::string topic_data_str(pub_data.topic_data.value.begin(), pub_data.topic_data.value.end());
					if (topic_data_str == "updated_topic_data_test") {
						found_second = true;
						break;
					}
				}
			}
			//EXPECT_TRUE(found_second);

			// 8.5. 发送第二次TopicData检查信号
			NotifyNegative();

			// 9. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 10. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 11. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}

TEST_F(QoSTest, QoS_TopicDataQosPolicyCFTIntegrationTest) {
	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipant* participant = factory->create_participant(
		0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	// 3. 注册TestModule::KeyData类型
	TestModule::KeyDataTypeSupport* type_support = TestModule::KeyDataTypeSupport::get_instance();
	ASSERT_NE(type_support, nullptr);
	EXPECT_EQ(participant->registe_type("TestModule::KeyData", type_support), RETCODE_OK);

	// 4. 测试基础Topic的TopicData
	TopicQos base_qos = TOPIC_QOS_DEFAULT;
	base_qos.topic_data.value.assign({ 'b','a','s','e','_','t','o','p','i','c','_','d','a','t','a' });
	Topic* base_topic = participant->create_topic(
		"base_topic", "TestModule::KeyData", base_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(base_topic, nullptr);
	TopicQos check_qos;
	EXPECT_EQ(base_topic->get_qos(check_qos), RETCODE_OK);
	EXPECT_EQ(std::string(check_qos.topic_data.value.begin(), check_qos.topic_data.value.end()), "base_topic_data");

	// 5. 测试ContentFilteredTopic继承TopicData
	ContentFilteredTopic* cft = participant->create_contentfilteredtopic(
		"filtered_topic", base_topic, "userId > 10", std::vector<std::string>());
	ASSERT_NE(cft, nullptr);
	Topic* related_topic = cft->get_related_topic();
	ASSERT_EQ(related_topic, base_topic);
	TopicQos related_qos;
	EXPECT_EQ(related_topic->get_qos(related_qos), RETCODE_OK);
	EXPECT_EQ(std::string(related_qos.topic_data.value.begin(), related_qos.topic_data.value.end()), "base_topic_data");

	// 6. 测试基础Topic修改TopicData对ContentFilteredTopic的影响
	TopicQos mod_qos = base_qos;
	mod_qos.topic_data.value.assign({ 'm','o','d','i','f','i','e','d','_','b','a','s','e','_','t','o','p','i','c','_','d','a','t','a' });
	EXPECT_EQ(base_topic->set_qos(mod_qos), RETCODE_OK);
	TopicQos after_mod_qos;
	EXPECT_EQ(cft->get_related_topic()->get_qos(after_mod_qos), RETCODE_OK);
	EXPECT_EQ(std::string(after_mod_qos.topic_data.value.begin(), after_mod_qos.topic_data.value.end()), "modified_base_topic_data");

	// 7. 测试ContentFilteredTopic在数据发现中的TopicData传播
	Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(publisher, nullptr);
	DataWriter* writer = publisher->create_datawriter(
		cft->get_related_topic(), DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(writer, nullptr);

	Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(subscriber, nullptr);
	DataReader* reader = subscriber->create_datareader(
		cft, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(reader, nullptr);

	// 等待匹配完成
	std::vector<InstanceHandle_t> handles;
	PublicationBuiltinTopicData pub_data;
	bool found = false;
	for (int i = 0; i < 50; ++i) {
		handles.clear();
		EXPECT_EQ(reader->get_matched_publications(handles), RETCODE_OK);
		if (!handles.empty()) {
			for (const auto& h : handles) {
				EXPECT_EQ(reader->get_matched_publication_data(pub_data, h), RETCODE_OK);
				if (pub_data.topic_name == "filtered_topic") {
					std::string topic_data_str(pub_data.topic_data.value.begin(), pub_data.topic_data.value.end());
					if (topic_data_str == "modified_base_topic_data") {
						found = true;
						break;
					}
				}
			}
		}
		if (found) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	//EXPECT_TRUE(found);

	// 8. 删除ContentFilteredTopic和基础Topic
	//EXPECT_EQ(participant->delete_contentfilteredtopic(cft), RETCODE_OK);
	//EXPECT_EQ(participant->delete_topic(base_topic), RETCODE_OK);

	// 9. 删除所有子实体
	EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

	// 10. 删除域参与者
	EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

	// 10. finalize_instance
	factory->finalize();
}

TEST_F(QoSTest, QoS_TopicDataQosPolicyBoundaryTest) {
	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipant* participant = factory->create_participant(
		0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	// 3. 注册TestModule::BasicData类型
	TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
	ASSERT_NE(type_support, nullptr);
	EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

	// 4. 测试空TopicData
	TopicQos empty_qos = TOPIC_QOS_DEFAULT;
	empty_qos.topic_data.value.clear();
	Topic* empty_topic = participant->create_topic(
		"empty_topic_data", "TestModule::BasicData", empty_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(empty_topic, nullptr);
	TopicQos check_empty;
	EXPECT_EQ(empty_topic->get_qos(check_empty), RETCODE_OK);
	EXPECT_TRUE(check_empty.topic_data.value.empty());

	// 5. 测试大容量TopicData（255B）
	TopicQos large_qos = TOPIC_QOS_DEFAULT;
	std::vector<uint8_t> large_data(255, 0xAB);
	large_qos.topic_data.value = large_data;
	Topic* large_topic = participant->create_topic(
		"large_topic_data", "TestModule::BasicData", large_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(large_topic, nullptr);
	TopicQos check_large;
	EXPECT_EQ(large_topic->get_qos(check_large), RETCODE_OK);
	EXPECT_EQ(check_large.topic_data.value.size(), 255);
	EXPECT_TRUE(std::equal(check_large.topic_data.value.begin(), check_large.topic_data.value.end(), large_data.begin()));

	// 6. 测试二进制TopicData
	TopicQos bin_qos = TOPIC_QOS_DEFAULT;
	std::vector<uint8_t> bin_data = { 0x00, 0xFF, 0x7E, 0x55, 0xAA, 0x10 };
	bin_qos.topic_data.value = bin_data;
	Topic* bin_topic = participant->create_topic(
		"bin_topic_data", "TestModule::BasicData", bin_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(bin_topic, nullptr);
	TopicQos check_bin;
	EXPECT_EQ(bin_topic->get_qos(check_bin), RETCODE_OK);
	EXPECT_EQ(check_bin.topic_data.value, bin_data);

	// 7. 测试TopicData与Topic生命周期
	TopicQos life_qos = TOPIC_QOS_DEFAULT;
	life_qos.topic_data.value.assign({ 'l','i','f','e','_','t','e','s','t' });
	Topic* life_topic = participant->create_topic(
		"life_topic_data", "TestModule::BasicData", life_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(life_topic, nullptr);
	Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataWriter* writer = publisher->create_datawriter(
		life_topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	DataReader* reader = subscriber->create_datareader(
		life_topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
	EXPECT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
	TopicQos check_life;
	EXPECT_EQ(life_topic->get_qos(check_life), RETCODE_OK);
	EXPECT_EQ(std::string(check_life.topic_data.value.begin(), check_life.topic_data.value.end()), "life_test");
	TopicQos mod_life_qos = life_qos;
	mod_life_qos.topic_data.value.assign({ 'm','o','d','_','l','i','f','e' });
	EXPECT_EQ(life_topic->set_qos(mod_life_qos), RETCODE_OK);
	TopicQos check_life2;
	EXPECT_EQ(life_topic->get_qos(check_life2), RETCODE_OK);
	EXPECT_EQ(std::string(check_life2.topic_data.value.begin(), check_life2.topic_data.value.end()), "mod_life");

	// 8. 测试多个相同名称Topic的TopicData一致性
	TopicQos same_name_qos = TOPIC_QOS_DEFAULT;
	same_name_qos.topic_data.value.assign({ 's','a','m','e','_','n','a','m','e' });
	Topic* topic_same1 = participant->create_topic(
		"same_name_topic", "TestModule::BasicData", same_name_qos, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(topic_same1, nullptr);
	TopicQos diff_name_qos = TOPIC_QOS_DEFAULT;
	diff_name_qos.topic_data.value.assign({ 'd','i','f','f','_','d','a','t','a' });
	Topic* topic_same2 = participant->create_topic(
		"same_name_topic", "TestModule::BasicData", diff_name_qos, nullptr, STATUS_MASK_NONE);
	// 预期返回原Topic（topic_same1），而不是新建
	//EXPECT_EQ(topic_same2, topic_same1);
	TopicQos check_same;
	EXPECT_EQ(topic_same1->get_qos(check_same), RETCODE_OK);
	EXPECT_EQ(std::string(check_same.topic_data.value.begin(), check_same.topic_data.value.end()), "same_name");

	// 9. 删除所有实体
	EXPECT_EQ(participant->delete_topic(empty_topic), RETCODE_OK);
	EXPECT_EQ(participant->delete_topic(large_topic), RETCODE_OK);
	EXPECT_EQ(participant->delete_topic(bin_topic), RETCODE_OK);
	EXPECT_EQ(participant->delete_topic(life_topic), RETCODE_OK);
	EXPECT_EQ(participant->delete_topic(topic_same1), RETCODE_OK);

	// 9. 删除所有子实体
	EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

	// 10. 删除域参与者
	EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

	// 11. finalize_instance
	factory->finalize();
}

TEST_F(QoSTest, QoS_UserDataQosPolicy) {
	std::thread th_1(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 自定义DomainParticipantQos对象，设置user_data
			DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
			dp_qos.user_data.value = { 'd', 'p', 'u', 's', 'e', 'r' };

			// 3. 以自定义Qos创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, dp_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 4. 注册TestModule::BasicData类型，并创建example_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"example_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 5. 默认创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 6. 默认创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 7. DataWriterQos配置user_data，通过发现协议传递
			DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
			dw_qos.user_data.value = { 'w', 'r', 'i', 't', 'e', 'r', 'u', 's', 'e', 'r' };
			DataWriter* writer = publisher->create_datawriter(
				topic, dw_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(writer, nullptr);

			// 8. 等待主动端完成第一次DataWriter user_data检查
			WaitPositive();

			// 9. DataReaderQos配置user_data，通过发现协议传递
			DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
			dr_qos.user_data.value = { 'r', 'e', 'a', 'd', 'e', 'r', 'u', 's', 'e', 'r' };
			DataReader* reader = subscriber->create_datareader(
				topic, dr_qos, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader, nullptr);

			// 10. 等待主动端完成第一次DataReader user_data检查
			WaitPositive();

			// 11. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 12. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 13. finalize_instance
			factory->finalize();
		});
	std::thread th_2(
		[&]() {
			// 1. 获取工厂单例
			DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
			ASSERT_NE(factory, nullptr);

			// 2. 创建DomainParticipant
			DomainParticipant* participant = factory->create_participant(
				0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(participant, nullptr);

			// 3. 注册TestModule::BasicData类型，并创建example_topic_name主题
			TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
			ASSERT_NE(type_support, nullptr);
			EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

			Topic* topic = participant->create_topic(
				"example_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(topic, nullptr);

			// 4. 创建Publisher
			Publisher* publisher = participant->create_publisher(
				PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(publisher, nullptr);

			// 5. 创建Subscriber
			Subscriber* subscriber = participant->create_subscriber(
				SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(subscriber, nullptr);

			// 6. 创建DataReader
			DataReader* reader = subscriber->create_datareader(
				topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
			ASSERT_NE(reader, nullptr);

			// 7. 轮询get_matched_publications直到有匹配
			std::vector<InstanceHandle_t> pub_handles;
			for (int i = 0; i < 50; ++i) {
				pub_handles.clear();
				EXPECT_EQ(reader->get_matched_publications(pub_handles), RETCODE_OK);
				if (!pub_handles.empty()) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			EXPECT_FALSE(pub_handles.empty());

			// 8. 检查PublicationBuiltinTopicData.user_data
			bool found_writer_user_data = false;
			for (const auto& h : pub_handles) {
				PublicationBuiltinTopicData data;
				EXPECT_EQ(reader->get_matched_publication_data(data, h), RETCODE_OK);
				std::string user_data_str(data.user_data.value.begin(), data.user_data.value.end());
				if (user_data_str == "writeruser") {
					found_writer_user_data = true;
				}
			}
			EXPECT_TRUE(found_writer_user_data);

			// 9. 发送DataWriter user_data检查信号
			NotifyNegative();

			// 10. 创建DataWriter
			DataWriter* writer = publisher->create_datawriter(
				topic, DATAWRITER_QOS_DEFAULT, nullptr, PUBLICATION_MATCHED_STATUS);
			ASSERT_NE(writer, nullptr);

			// 11. 轮询get_matched_subscriptions直到有匹配
			std::vector<InstanceHandle_t> sub_handles;
			for (int i = 0; i < 50; ++i) {
				sub_handles.clear();
				EXPECT_EQ(writer->get_matched_subscriptions(sub_handles), RETCODE_OK);
				if (!sub_handles.size()==2) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			EXPECT_FALSE(sub_handles.empty());

			// 12. 检查SubscriptionBuiltinTopicData.user_data
			bool found_reader_user_data = false;
			for (const auto& h : sub_handles) {
				SubscriptionBuiltinTopicData data;
				EXPECT_EQ(writer->get_matched_subscription_data(data, h), RETCODE_OK);
				std::string user_data_str(data.user_data.value.begin(), data.user_data.value.end());
				if (user_data_str == "readeruser") {
					found_reader_user_data = true;
				}
			}
			EXPECT_TRUE(found_reader_user_data);

			// 13. 发送DataReader user_data检查信号
			NotifyNegative();

			// 14. 删除所有子实体
			EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

			// 15. 删除域参与者
			EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

			// 16. finalize_instance
			factory->finalize();
		});
	th_1.join();
	th_2.join();
}