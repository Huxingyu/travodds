

#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"
#include "TestFrame.h"

USING_TRAVODDS_NAMESPACE


TEST(QOSCheck, DomainParticipant_QOSCHECK) {
	// 等待连接
	//WAIT_AND_SYNC_CONNECTION("EntityFactoryQosPolicy");

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

	//participant->delete_subscriber(subcriber);
	factory->delete_participant(participant);
}

TEST(QOSCheck, Publisher_QOSCHECK) {
	// 等待连接
	//WAIT_AND_SYNC_CONNECTION("EntityFactoryQosPolicy");

	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipant* participant = factory->create_participant(
		0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT,NULL,0xFFFF);
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
	EXPECT_EQ(result, RETCODE_OK);


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


TEST(QOSCheck, Subcsciber_QOSCHECK) {
	// 等待连接
	//WAIT_AND_SYNC_CONNECTION("EntityFactoryQosPolicy");

	// 1. 获取工厂单例
	DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
	ASSERT_NE(factory, nullptr);

	// 2. 创建DomainParticipant
	DomainParticipant* participant = factory->create_participant(
		0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	ASSERT_NE(participant, nullptr);

	Subscriber* subcriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, NULL, 0xFFFF);
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
	EXPECT_EQ(result, RETCODE_OK);


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


TEST(QOSCheck, DataWriter_QOSCHECK  ) {
	// 等待连接
	//WAIT_AND_SYNC_CONNECTION("EntityFactoryQosPolicy");

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

	Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, NULL, 0xFFFF);
	ASSERT_NE(publisher, nullptr);

	DataWriter* datawriter = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, 0xffff);

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
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);


	dw_qos.user_data.value.resize(0);
	dw_qos.resource_limits.max_samples = 1;
	dw_qos.resource_limits.max_instances = 1;
	dw_qos.resource_limits.max_samples_per_instance = 1;
	result = datawriter->set_qos(dw_qos);
	EXPECT_EQ(result, RETCODE_OK);

	publisher->delete_datawriter(datawriter);
	participant->delete_topic(topic);
	participant->delete_publisher(publisher);
	factory->delete_participant(participant);

}
TEST(QOSCheck, DataReader_QOSCHECK) {
	// 等待连接
	//WAIT_AND_SYNC_CONNECTION("EntityFactoryQosPolicy");

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

	Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, NULL, 0xFFFF);
	ASSERT_NE(subscriber, nullptr);

	DataReader* datareader= subscriber->create_datareader(topic, DATAREADER_QOS_DEFAULT, nullptr, 0xffff);

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
	EXPECT_EQ(result, RETCODE_OUT_OF_RESOURCES);


	dr_qos.user_data.value.resize(0);
	dr_qos.resource_limits.max_samples = 1;
	dr_qos.resource_limits.max_instances = 1;
	dr_qos.resource_limits.max_samples_per_instance = 1;
	dr_qos.deadline.period = { 1,0 };
	dr_qos.time_based_filter.minimum_separation = { 2,0 };
	result = datareader->set_qos(dr_qos);
	EXPECT_EQ(result, RETCODE_OK);


	subscriber->delete_datareader(datareader);
	participant->delete_topic(topic);
	participant->delete_subscriber(subscriber);
	factory->delete_participant(participant);
}


TEST(QOSCheck, TOPIC_QOSCHECK) {
	// 等待连接
	//WAIT_AND_SYNC_CONNECTION("EntityFactoryQosPolicy");

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
	EXPECT_EQ(result, RETCODE_OK);

	topic_qos.topic_data.value.resize(0);
	result = topic->set_qos(topic_qos);
	EXPECT_EQ(result, RETCODE_OK);

}
