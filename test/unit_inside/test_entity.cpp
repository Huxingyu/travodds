#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/publisher.h"
#include "dcps/publish/datawriter.h"
#include "dcps/subscribe/subscriber.h"
#include "dcps/subscribe/datareader.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE


TEST(EntityTest, Entity_domainParticipant_get) {

    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(par, nullptr);

    StatusCondition* condition = &par->get_statuscondition();
    ASSERT_NE(condition, nullptr);
	EXPECT_EQ(condition->get_entity(), par);

	StatusMask status_changes = par->get_status_change();
	EXPECT_EQ(status_changes, StatusMask(0));
	EXPECT_EQ(par->get_status_change(), StatusMask(0));
	EXPECT_EQ(par->enable(), RETCODE_OK);
	EXPECT_NE(par->get_instance_handle(), HANDLE_NIL);

	int data = 42;
	par->set_object_data(&data);

	void* retrieved_data = par->get_object_data();
	EXPECT_EQ(retrieved_data, &data);

	TheParticipantFactory->delete_participant(par);
}
TEST(EntityTest, Entity_publisher_get) {
	DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(par, nullptr);
	Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(publisher, nullptr);
	StatusCondition* condition = &publisher->get_statuscondition();
	ASSERT_NE(condition, nullptr);
	EXPECT_EQ(condition->get_entity(), publisher);
	StatusMask status_changes = publisher->get_status_change();
	EXPECT_EQ(status_changes, StatusMask(0));
	EXPECT_EQ(publisher->get_status_change(), StatusMask(0));
	EXPECT_EQ(publisher->enable(), RETCODE_OK);
	EXPECT_NE(publisher->get_instance_handle(), HANDLE_NIL);
	int data = 84;
	publisher->set_object_data(&data);
	void* retrieved_data = publisher->get_object_data();
	EXPECT_EQ(retrieved_data, &data);
	par->delete_publisher(publisher);
	TheParticipantFactory->delete_participant(par);
}
TEST(EntityTest, Entity_subscriber_get) {
	DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(par, nullptr);
	Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(subscriber, nullptr);
	StatusCondition* condition = &subscriber->get_statuscondition();
	ASSERT_NE(condition, nullptr);
	EXPECT_EQ(condition->get_entity(), subscriber);
	StatusMask status_changes = subscriber->get_status_change();
	EXPECT_EQ(status_changes, StatusMask(0));
	EXPECT_EQ(subscriber->get_status_change(), StatusMask(0));
	EXPECT_EQ(subscriber->enable(), RETCODE_OK);
	EXPECT_NE(subscriber->get_instance_handle(), HANDLE_NIL);
	int data = 168;
	subscriber->set_object_data(&data);
	void* retrieved_data = subscriber->get_object_data();
	EXPECT_EQ(retrieved_data, &data);
	par->delete_subscriber(subscriber);
	TheParticipantFactory->delete_participant(par);
}
TEST(EntityTest, Entity_datawriter_get) {
	DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(par, nullptr);
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(top, nullptr);
	Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(publisher, nullptr);
	DataWriter* writer = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(writer, nullptr);
	StatusCondition* condition = &writer->get_statuscondition();
	ASSERT_NE(condition, nullptr);
	EXPECT_EQ(condition->get_entity(), writer);
	StatusMask status_changes = writer->get_status_change();
	EXPECT_EQ(status_changes, StatusMask(0));
	EXPECT_EQ(writer->get_status_change(), StatusMask(0));
	EXPECT_EQ(writer->enable(), RETCODE_OK);
	EXPECT_NE(writer->get_instance_handle(), HANDLE_NIL);
	int data = 336;
	writer->set_object_data(&data);
	void* retrieved_data = writer->get_object_data();
	EXPECT_EQ(retrieved_data, &data);
	publisher->delete_datawriter(writer);
	par->delete_publisher(publisher);
	par->delete_topic(top);
	TheParticipantFactory->delete_participant(par);
}
TEST(EntityTest, Entity_datareader_get) {
	DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(par, nullptr);
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(top, nullptr);
	Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(subscriber, nullptr);
	DataReader* reader = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(reader, nullptr);
	StatusCondition* condition = &reader->get_statuscondition();
	ASSERT_NE(condition, nullptr);
	EXPECT_EQ(condition->get_entity(), reader);
	StatusMask status_changes = reader->get_status_change();
	EXPECT_EQ(status_changes, StatusMask(0));
	EXPECT_EQ(reader->get_status_change(), StatusMask(0));
	EXPECT_EQ(reader->enable(), RETCODE_OK);
	EXPECT_NE(reader->get_instance_handle(), HANDLE_NIL);
	int data = 672;
	reader->set_object_data(&data);
	void* retrieved_data = reader->get_object_data();
	EXPECT_EQ(retrieved_data, &data);
	subscriber->delete_datareader(reader);
	par->delete_subscriber(subscriber);
	par->delete_topic(top);
	TheParticipantFactory->delete_participant(par);
}
TEST(EntityTest, Entity_topic_get) {
	DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(par, nullptr);
	const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
	par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
	Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	EXPECT_NE(top, nullptr);
	StatusCondition* condition = &top->get_statuscondition();
	ASSERT_NE(condition, nullptr);
	EXPECT_EQ(condition->get_entity(), top);
	StatusMask status_changes = top->get_status_change();
	EXPECT_EQ(status_changes, StatusMask(0));
	EXPECT_EQ(top->get_status_change(), StatusMask(0));
	EXPECT_EQ(top->enable(), RETCODE_OK);
	EXPECT_NE(top->get_instance_handle(), HANDLE_NIL);
	int data = 1344;
	top->set_object_data(&data);
	void* retrieved_data = top->get_object_data();
	EXPECT_EQ(retrieved_data, &data);
	par->delete_topic(top);
	TheParticipantFactory->delete_participant(par);
}