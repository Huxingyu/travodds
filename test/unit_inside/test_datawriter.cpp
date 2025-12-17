#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/publisher.h"
#include "dcps/publish/datawriter.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

/****************************** DataWriter *********************************************/
TEST(DataWriterTest, DataWriter_get_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriterQos qos;
    DataWriter* dw = publisher->create_datawriter(top, qos, nullptr, STATUS_MASK_NONE);
    DataWriterQos qosGet;
    ReturnCode_t result = dw->get_qos(qosGet);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(qosGet.durability.kind, qos.durability.kind);
    EXPECT_EQ(qosGet.durability_service.service_cleanup_delay, qos.durability_service.service_cleanup_delay);
    EXPECT_EQ(qosGet.durability_service.history_kind, qos.durability_service.history_kind);
    EXPECT_EQ(qosGet.durability_service.history_depth, qos.durability_service.history_depth);
    EXPECT_EQ(qosGet.durability_service.max_samples, qos.durability_service.max_samples);
    EXPECT_EQ(qosGet.durability_service.max_instances, qos.durability_service.max_instances);
    EXPECT_EQ(qosGet.durability_service.max_samples_per_instance, qos.durability_service.max_samples_per_instance);
    EXPECT_EQ(qosGet.deadline.period, qos.deadline.period);
    EXPECT_EQ(qosGet.latency_budget.duration, qos.latency_budget.duration);
    EXPECT_EQ(qosGet.liveliness.kind, qos.liveliness.kind);
    EXPECT_EQ(qosGet.liveliness.lease_duration, qos.liveliness.lease_duration);
    EXPECT_EQ(qosGet.reliability.kind, qos.reliability.kind);
    EXPECT_EQ(qosGet.reliability.max_blocking_time, qos.reliability.max_blocking_time);
    EXPECT_EQ(qosGet.destination_order.kind, qos.destination_order.kind);
    EXPECT_EQ(qosGet.history.kind, qos.history.kind);
    EXPECT_EQ(qosGet.history.depth, qos.history.depth);
    EXPECT_EQ(qosGet.resource_limits.max_samples, qos.resource_limits.max_samples);
    EXPECT_EQ(qosGet.resource_limits.max_instances, qos.resource_limits.max_instances);
    EXPECT_EQ(qosGet.resource_limits.max_samples_per_instance, qos.resource_limits.max_samples_per_instance);
    EXPECT_EQ(qosGet.transport_priority.value, qos.transport_priority.value);
    EXPECT_EQ(qosGet.lifespan.duration, qos.lifespan.duration);
    EXPECT_EQ(qosGet.user_data.value, qos.user_data.value);
    EXPECT_EQ(qosGet.ownership.kind, qos.ownership.kind);
    EXPECT_EQ(qosGet.ownership_strength.value, qos.ownership_strength.value);
    EXPECT_EQ(qosGet.writer_data_lifecycle.autodispose_unregistered_instances, qos.writer_data_lifecycle.autodispose_unregistered_instances);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataWriterTest, DataWriter_set_get_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriterQos qos;
    ReturnCode_t result = dw->set_qos(qos);
    EXPECT_EQ(result, RETCODE_OK);
    DataWriterQos qosGet;
    ReturnCode_t resultGet = dw->get_qos(qosGet);
    EXPECT_EQ(resultGet, RETCODE_OK);
    EXPECT_EQ(qosGet.durability.kind, qos.durability.kind);
    EXPECT_EQ(qosGet.durability_service.service_cleanup_delay, qos.durability_service.service_cleanup_delay);
    EXPECT_EQ(qosGet.durability_service.history_kind, qos.durability_service.history_kind);
    EXPECT_EQ(qosGet.durability_service.history_depth, qos.durability_service.history_depth);
    EXPECT_EQ(qosGet.durability_service.max_samples, qos.durability_service.max_samples);
    EXPECT_EQ(qosGet.durability_service.max_instances, qos.durability_service.max_instances);
    EXPECT_EQ(qosGet.durability_service.max_samples_per_instance, qos.durability_service.max_samples_per_instance);
    EXPECT_EQ(qosGet.deadline.period, qos.deadline.period);
    EXPECT_EQ(qosGet.latency_budget.duration, qos.latency_budget.duration);
    EXPECT_EQ(qosGet.liveliness.kind, qos.liveliness.kind);
    EXPECT_EQ(qosGet.liveliness.lease_duration, qos.liveliness.lease_duration);
    EXPECT_EQ(qosGet.reliability.kind, qos.reliability.kind);
    EXPECT_EQ(qosGet.reliability.max_blocking_time, qos.reliability.max_blocking_time);
    EXPECT_EQ(qosGet.destination_order.kind, qos.destination_order.kind);
    EXPECT_EQ(qosGet.history.kind, qos.history.kind);
    EXPECT_EQ(qosGet.history.depth, qos.history.depth);
    EXPECT_EQ(qosGet.resource_limits.max_samples, qos.resource_limits.max_samples);
    EXPECT_EQ(qosGet.resource_limits.max_instances, qos.resource_limits.max_instances);
    EXPECT_EQ(qosGet.resource_limits.max_samples_per_instance, qos.resource_limits.max_samples_per_instance);
    EXPECT_EQ(qosGet.transport_priority.value, qos.transport_priority.value);
    EXPECT_EQ(qosGet.lifespan.duration, qos.lifespan.duration);
    EXPECT_EQ(qosGet.user_data.value, qos.user_data.value);
    EXPECT_EQ(qosGet.ownership.kind, qos.ownership.kind);
    EXPECT_EQ(qosGet.ownership_strength.value, qos.ownership_strength.value);
    EXPECT_EQ(qosGet.writer_data_lifecycle.autodispose_unregistered_instances, qos.writer_data_lifecycle.autodispose_unregistered_instances);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataWriterTest, DataWriter_set_qos1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriterQos qos;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    qos.history.depth = -1;
    ReturnCode_t result = dw->set_qos(qos);
    EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataWriterTest, DataWriter_set_qos2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriterQos qos;
    qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    ReturnCode_t result = dw->set_qos(qos);
    EXPECT_EQ(result, RETCODE_IMMUTABLE_POLICY);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataWriterTest, DataWriter_get_listener) {
    DataWriterListener* dwlistener = new DataWriterListener;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, dwlistener, STATUS_MASK_NONE);
    const DataWriterListener* result = dw->get_listener();
    EXPECT_EQ(result, dwlistener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DataWriterTest, DataWriter_set_get_listener) {
    DataWriterListener* dwlistener = new DataWriterListener;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t ret = dw->set_listener(dwlistener, STATUS_MASK_NONE);
    EXPECT_EQ(ret, RETCODE_OK);

    const DataWriterListener* result = dw->get_listener();
    EXPECT_EQ(result, dwlistener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DataWriterTest, DataWriter_get_topic) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Topic* result = dw->get_topic();
    EXPECT_EQ(result, top);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataWriterTest, DataWriter_get_publisher) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const Publisher* result = dw->get_publisher();
    EXPECT_EQ(result, publisher);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
// 存在key，进行注册
TEST(DataWriterTest, DataWriter_register_instance) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(publisher, nullptr);
    DataWriter* writer = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(writer, nullptr);
    TestModule::KeyData data;
    data.userId = 1;
    InstanceHandle_t handle = writer->register_instance(&data);
    handle = writer->register_instance(&data);
    EXPECT_NE(handle, HANDLE_NIL);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

//不存在key,进行注册
TEST(DataWriterTest, DataWriter_register_instance_2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(publisher, nullptr);
    DataWriter* writer = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(writer, nullptr);
    TestModule::BasicData data;
    data.id = 1;
    InstanceHandle_t handle = writer->register_instance(&data);
    EXPECT_EQ(handle, HANDLE_NIL);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

// 以注册类型，进行取消注册
TEST(DataWriterTest, DataWriter_unregister_instance_1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(publisher, nullptr);
    DataWriter* writer = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(writer, nullptr);
    TestModule::KeyData data;
    data.userId = 1;
    InstanceHandle_t handle = writer->register_instance(&data);
    EXPECT_NE(handle, HANDLE_NIL);
    ReturnCode_t ret = writer->unregister_instance(&data, handle);
    EXPECT_EQ(ret, RETCODE_OK);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}


// 没有注册Handle，取消注册 
TEST(DataWriterTest, DataWriter_unregister_instance_2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(publisher, nullptr);
    DataWriter* writer = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(writer, nullptr);
    TestModule::KeyData data;
    data.userId = 1;
    InstanceHandle_t handle = writer->register_instance(&data);
    EXPECT_NE(handle, HANDLE_NIL);
    ReturnCode_t ret = writer->unregister_instance(&data, handle);
    ret = writer->unregister_instance(&data, handle);
    EXPECT_EQ(ret, RETCODE_OK);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}



// 没有key的数据的类型，取消注册 
TEST(DataWriterTest, DataWriter_unregister_instance_3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("BasicData", TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(publisher, nullptr);
    DataWriter* writer = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(writer, nullptr);
    TestModule::KeyData data;
    data.userId = 1;
    InstanceHandle_t handle = writer->register_instance(&data);
    EXPECT_EQ(handle, HANDLE_NIL);
    ReturnCode_t ret = writer->unregister_instance(&data, handle);
    EXPECT_EQ(ret, RETCODE_ILLEGAL_OPERATION);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}


TEST(DataWriterTest, DataWriter_get_key_value) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("BasicData", TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(publisher, nullptr);
    DataWriter* writer = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(writer, nullptr);
    TestModule::KeyData data;
    data.userId = 1;
    InstanceHandle_t handle = writer->register_instance(&data);
    EXPECT_EQ(handle, HANDLE_NIL);
    ReturnCode_t ret = writer->get_key_value(&data, handle);
    EXPECT_EQ(ret, RETCODE_ILLEGAL_OPERATION);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}


TEST(DataWriterTest, TRAVODDS_DATAWRITER_lookup_instance) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    par->registe_type("BasicData", TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(top, nullptr);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(publisher, nullptr);
    DataWriter* writer = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(writer, nullptr);
    TestModule::KeyData data;
    data.userId = 1;
    InstanceHandle_t handle = writer->register_instance(&data);
    EXPECT_EQ(handle, HANDLE_NIL);
    handle = writer->lookup_instance(&data);
    EXPECT_EQ(handle, HANDLE_NIL);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

