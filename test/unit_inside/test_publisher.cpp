#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/publisher.h"
#include "testdataTypeSupport.h"
USING_TRAVODDS_NAMESPACE

/****************************** Publisher *********************************************/
TEST(PublisherTest, Publisher_get_qos) {
    PublisherQos qos;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(qos, nullptr, STATUS_MASK_NONE);
    PublisherQos qosGet;
    ReturnCode_t result = publisher->get_qos(qosGet);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(qosGet.presentation.access_scope, qos.presentation.access_scope);
    EXPECT_EQ(qosGet.presentation.coherent_access, qos.presentation.coherent_access);
    EXPECT_EQ(qosGet.presentation.ordered_access, qos.presentation.ordered_access);
    EXPECT_EQ(qosGet.partition.name, qos.partition.name);
    EXPECT_EQ(qosGet.group_data.value, qos.group_data.value);
    EXPECT_EQ(qosGet.entity_factory.autoenable_created_entities, qos.entity_factory.autoenable_created_entities);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_set_get_qos) {

    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    PublisherQos qos;
    ReturnCode_t result = publisher->set_qos(qos);
    EXPECT_EQ(result, RETCODE_OK);

    PublisherQos qosGet;
    ReturnCode_t resultGet = publisher->get_qos(qosGet);
    EXPECT_EQ(resultGet, RETCODE_OK);
    EXPECT_EQ(qosGet.presentation.access_scope, qos.presentation.access_scope);
    EXPECT_EQ(qosGet.presentation.coherent_access, qos.presentation.coherent_access);
    EXPECT_EQ(qosGet.presentation.ordered_access, qos.presentation.ordered_access);
    EXPECT_EQ(qosGet.partition.name, qos.partition.name);
    EXPECT_EQ(qosGet.group_data.value, qos.group_data.value);
    EXPECT_EQ(qosGet.entity_factory.autoenable_created_entities, qos.entity_factory.autoenable_created_entities);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_set_qos) {

    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    PublisherQos qos;
    qos.presentation.access_scope = TOPIC_PRESENTATION_QOS;
    ReturnCode_t result = publisher->set_qos(qos);
    EXPECT_EQ(result, RETCODE_IMMUTABLE_POLICY);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_get_listener) {
    PublisherListener* listener = new PublisherListener();
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, listener, STATUS_MASK_NONE);
    const PublisherListener* result = publisher->get_listener();
    EXPECT_EQ(result, listener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_set_get_listener) {
    PublisherListener* listener = new PublisherListener();
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t ret = publisher->set_listener(listener, 0);
    EXPECT_EQ(ret, RETCODE_OK);
    const PublisherListener* result = publisher->get_listener();
    EXPECT_EQ(result, listener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_create_datawriter1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* result = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_create_datawriter2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* result = publisher->create_datawriter(nullptr, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_create_datawriter3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* topic = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* topic_ = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* result = publisher->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    par_->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_);
}
TEST(PublisherTest, Publisher_create_datawriter4) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* topic = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriterQos qos;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    qos.history.depth = -1;
    DataWriter* result = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_delete_datawriter1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = publisher->delete_datawriter(dw);
    EXPECT_EQ(result, RETCODE_OK);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_delete_datawriter2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = nullptr;
    ReturnCode_t result = publisher->delete_datawriter(dw);
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_delete_datawriter3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* pub_ = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = pub->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = pub_->delete_datawriter(dw);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_lookup_datawriter1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* result = publisher->lookup_datawriter("topicname");
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(result, dw);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_lookup_datawriter2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* result = publisher->lookup_datawriter("A");
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisheTest, Publisher_suspend_publications) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
	// 暂时未支持该接口
    ReturnCode_t result = publisher->suspend_publications();
    EXPECT_EQ(result, RETCODE_UNSUPPORTED);
    par->delete_contained_entities();
	TheParticipantFactory->delete_participant(par);
}
TEST(PublisheTest, Publisher_resume_publications) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    // 暂时未支持该接口
    ReturnCode_t result = publisher->resume_publications();
    EXPECT_EQ(result, RETCODE_UNSUPPORTED);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisheTest, Publisher_begin_coherent_changes) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    // 暂时未支持该接口
    ReturnCode_t result = publisher->begin_coherent_changes();
    EXPECT_EQ(result, RETCODE_UNSUPPORTED);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisheTest, Publisher_end_coherent_changes) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    // 暂时未支持该接口
    ReturnCode_t result = publisher->end_coherent_changes();
    EXPECT_EQ(result, RETCODE_UNSUPPORTED);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
// TODO: 可靠和不可靠的通信测试
TEST(PublisheTest, Publisher_wait_for_acknowledgments) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = publisher->wait_for_acknowledgments({5,0});
    EXPECT_EQ(result, RETCODE_OK);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_get_participant) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const DomainParticipant* result = publisher->get_participant();
    EXPECT_EQ(par, result);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_delete_contained_entities) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = publisher->delete_contained_entities();
    EXPECT_EQ(result, RETCODE_OK);
    result = par->delete_publisher(publisher);
    EXPECT_NE(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(PublisherTest, Publisher_set_get_default_datawriter_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriterQos qos;
    ReturnCode_t result = publisher->set_default_datawriter_qos(qos);
    EXPECT_EQ(result, RETCODE_OK);

    DataWriterQos qosGet;
    ReturnCode_t resultGet = publisher->get_default_datawriter_qos(qosGet);
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
TEST(PublisherTest, Publisher_set_default_datawriter_qos2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriterQos qos;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    qos.history.depth = -1;
    ReturnCode_t result = publisher->set_default_datawriter_qos(qos);
    EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(PublisherTest, Publisher_copy_from_topic_qos) {
    DataWriterQos dwqos;
    TopicQos tqos;
    ReturnCode_t result = Publisher::copy_from_topic_qos(dwqos, tqos);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(dwqos.durability.kind, tqos.durability.kind);
    EXPECT_EQ(dwqos.durability_service.service_cleanup_delay, tqos.durability_service.service_cleanup_delay);
    EXPECT_EQ(dwqos.durability_service.history_kind, tqos.durability_service.history_kind);
    EXPECT_EQ(dwqos.durability_service.history_depth, tqos.durability_service.history_depth);
    EXPECT_EQ(dwqos.durability_service.max_samples, tqos.durability_service.max_samples);
    EXPECT_EQ(dwqos.durability_service.max_instances, tqos.durability_service.max_instances);
    EXPECT_EQ(dwqos.durability_service.max_samples_per_instance, tqos.durability_service.max_samples_per_instance);
    EXPECT_EQ(dwqos.deadline.period, tqos.deadline.period);
    EXPECT_EQ(dwqos.latency_budget.duration, tqos.latency_budget.duration);
    EXPECT_EQ(dwqos.liveliness.kind, tqos.liveliness.kind);
    EXPECT_EQ(dwqos.liveliness.lease_duration, tqos.liveliness.lease_duration);
    EXPECT_EQ(dwqos.reliability.kind, tqos.reliability.kind);
    EXPECT_EQ(dwqos.reliability.max_blocking_time, tqos.reliability.max_blocking_time);
    EXPECT_EQ(dwqos.destination_order.kind, tqos.destination_order.kind);
    EXPECT_EQ(dwqos.history.kind, tqos.history.kind);
    EXPECT_EQ(dwqos.history.depth, tqos.history.depth);
    EXPECT_EQ(dwqos.resource_limits.max_samples, tqos.resource_limits.max_samples);
    EXPECT_EQ(dwqos.resource_limits.max_instances, tqos.resource_limits.max_instances);
    EXPECT_EQ(dwqos.resource_limits.max_samples_per_instance, tqos.resource_limits.max_samples_per_instance);
    EXPECT_EQ(dwqos.transport_priority.value, tqos.transport_priority.value);
    EXPECT_EQ(dwqos.lifespan.duration, tqos.lifespan.duration);
    EXPECT_EQ(dwqos.ownership.kind, tqos.ownership.kind);
}