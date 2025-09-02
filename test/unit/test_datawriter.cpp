#ifdef TEST_DATAWRITER
#include "test_head.hpp"
/****************************** DataWriter *********************************************/
TEST(DDSTest, DataWriter_get_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    DataWriterQos qos;
    DataWriter* dw = publisher->create_datawriter(top, qos, nullptr, 0);
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
TEST(DDSTest, DataWriter_set_get_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, 0);
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
TEST(DDSTest, DataWriter_set_qos1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, 0);
    DataWriterQos qos;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    qos.history.depth = -1;
    ReturnCode_t result = dw->set_qos(qos);
    EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, DataWriter_set_qos2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, 0);
    DataWriterQos qos;
    qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    ReturnCode_t result = dw->set_qos(qos);
    EXPECT_EQ(result, RETCODE_IMMUTABLE_POLICY);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, DataWriter_get_listener) {
    DataWriterListener* dwlistener = new DataWriterListener;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, dwlistener, 0);
    const DataWriterListener* result = dw->get_listener();
    EXPECT_EQ(result, dwlistener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTest, DataWriter_set_get_listener) {
    DataWriterListener* dwlistener = new DataWriterListener;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, 0);
    ReturnCode_t ret = dw->set_listener(dwlistener, 0);
    EXPECT_EQ(ret, RETCODE_OK);

    const DataWriterListener* result = dw->get_listener();
    EXPECT_EQ(result, dwlistener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTest, DataWriter_get_topic) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, 0);
    Topic* result = dw->get_topic();
    EXPECT_EQ(result, top);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, DataWriter_get_publisher) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, 0);
    const Publisher* result = dw->get_publisher();
    EXPECT_EQ(result, publisher);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, DataWriter_get_instance_handle) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, 0);
    DataWriter* dw = publisher->create_datawriter(top, DATAWRITER_QOS_DEFAULT, nullptr, 0);
    InstanceHandle_t handle = dw->get_instance_handle();
    bool result = par->contains_entity(handle);
    EXPECT_EQ(result, true);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
#endif // TEST_DATAWRITER
