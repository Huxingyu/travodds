#ifdef TEST_SUBSCRIBER
#include "test_head.hpp"
/****************************** Subscriber *********************************************/
TEST(DDSTest, Subscriber_get_qos) {
    SubscriberQos qos;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(qos, nullptr, 0);
    SubscriberQos qosGet;
    ReturnCode_t result = subscriber->get_qos(qosGet);
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

TEST(DDSTest, Subscriber_set_get_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    SubscriberQos qos;
    ReturnCode_t result = subscriber->set_qos(qos);
    EXPECT_EQ(result, RETCODE_OK);

    SubscriberQos qosGet;
    ReturnCode_t resultGet = subscriber->get_qos(qosGet);
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

TEST(DDSTest, Subscriber_se_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    SubscriberQos qos;
    qos.presentation.access_scope = TOPIC_PRESENTATION_QOS;
    ReturnCode_t result = subscriber->set_qos(qos);
    EXPECT_EQ(result, RETCODE_IMMUTABLE_POLICY);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTest, Subscriber_get_listener) {
    SubscriberListener* listener = new SubscriberListener();
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, listener, 0);
    const SubscriberListener* result = subscriber->get_listener();
    EXPECT_EQ(result, listener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTest, Subscriber_set_get_listener) {
    SubscriberListener* listener = new SubscriberListener();
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    ReturnCode_t ret = subscriber->set_listener(listener, 0);
    EXPECT_EQ(ret, RETCODE_OK);
    const SubscriberListener* result = subscriber->get_listener();
    EXPECT_EQ(result, listener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTEST, Subscriber_create_datareader)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* result = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    EXPECT_NE(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTEST, Subscriber_create_datareader1)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReaderQos qos;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    qos.history.depth = -1;
    DataReader* result = subscriber->create_datareader(top, qos, nullptr, 0);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTEST, Subscriber_create_datareader2)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* topic = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par_->registe_type("data", type);
    Topic* topic_ = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* result = subscriber->create_datareader(topic_, DATAREADER_QOS_DEFAULT, nullptr, 0);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    par_->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_);
}

TEST(DDSTEST, Subscriber_create_datareader3)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* result = subscriber->create_datareader(nullptr, DATAREADER_QOS_DEFAULT, nullptr, 0);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTEST, Subscriber_delete_datareader)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    ReturnCode_t result = subscriber->delete_datareader(dr);
    EXPECT_EQ(result, RETCODE_OK);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTEST, Subscriber_delete_datareader1)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = nullptr;
    ReturnCode_t result = subscriber->delete_datareader(dr);
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTEST, Subscriber_delete_datareader2)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    Subscriber* sub_ = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = sub->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    ReturnCode_t result = sub_->delete_datareader(dr);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTEST, Subscriber_lookup_datareader)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    DataReader* result = subscriber->lookup_datareader("topicname");
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(result, dr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTEST, Subscriber_lookup_datareader1)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    DataReader* result = subscriber->lookup_datareader("A");
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTEST, Subscriber_get_participant)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    const DomainParticipant* result = subscriber->get_participant();
    EXPECT_EQ(result, par);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DDSTEST, Subscriber_delete_contained_entities)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    ReturnCode_t result = subscriber->delete_contained_entities();
    EXPECT_EQ(result, RETCODE_OK);
    result = par->delete_subscriber(subscriber);
    EXPECT_NE(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTEST, Subscriber_set_get_default_datareader_qos)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReaderQos qos;
    ReturnCode_t result = subscriber->set_default_datareader_qos(qos);
    EXPECT_EQ(result, RETCODE_OK);

    DataReaderQos qosGet;
    ReturnCode_t resultGet = subscriber->get_default_datareader_qos(qosGet);
    EXPECT_EQ(resultGet, RETCODE_OK);
    EXPECT_EQ(qosGet.durability.kind, qos.durability.kind);
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
    EXPECT_EQ(qosGet.user_data.value, qos.user_data.value);
    EXPECT_EQ(qosGet.ownership.kind, qos.ownership.kind);
    EXPECT_EQ(qosGet.time_based_filter.minimum_separation, qos.time_based_filter.minimum_separation);
    EXPECT_EQ(qosGet.reader_data_lifecycle.autopurge_nowriter_samples_delay, qos.reader_data_lifecycle.autopurge_nowriter_samples_delay);
    EXPECT_EQ(qosGet.reader_data_lifecycle.autopurge_disposed_samples_delay, qos.reader_data_lifecycle.autopurge_disposed_samples_delay);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTEST, Subscriber_set_default_datareader_qos1)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    DataReaderQos qos;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    qos.history.depth = -1;
    ReturnCode_t result = subscriber->set_default_datareader_qos(qos);
    EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DDSTEST, Subscriber_copy_from_topic_qos)
{
    DataReaderQos datareader_qos;
    TopicQos topic_qos;
    ReturnCode_t result = Subscriber::copy_from_topic_qos(datareader_qos, topic_qos);
    EXPECT_EQ(result, RETCODE_OK);
    EXPECT_EQ(datareader_qos.durability.kind, topic_qos.durability.kind);
    EXPECT_EQ(datareader_qos.deadline.period, topic_qos.deadline.period);
    EXPECT_EQ(datareader_qos.latency_budget.duration, topic_qos.latency_budget.duration);
    EXPECT_EQ(datareader_qos.liveliness.kind, topic_qos.liveliness.kind);
    EXPECT_EQ(datareader_qos.liveliness.lease_duration, topic_qos.liveliness.lease_duration);
    EXPECT_EQ(datareader_qos.reliability.kind, topic_qos.reliability.kind);
    EXPECT_EQ(datareader_qos.reliability.max_blocking_time, topic_qos.reliability.max_blocking_time);
    EXPECT_EQ(datareader_qos.destination_order.kind, topic_qos.destination_order.kind);
    EXPECT_EQ(datareader_qos.history.kind, topic_qos.history.kind);
    EXPECT_EQ(datareader_qos.history.depth, topic_qos.history.depth);
    EXPECT_EQ(datareader_qos.resource_limits.max_samples, topic_qos.resource_limits.max_samples);
    EXPECT_EQ(datareader_qos.resource_limits.max_instances, topic_qos.resource_limits.max_instances);
    EXPECT_EQ(datareader_qos.resource_limits.max_samples_per_instance, topic_qos.resource_limits.max_samples_per_instance);
    EXPECT_EQ(datareader_qos.ownership.kind, topic_qos.ownership.kind);
}
TEST(DDSTEST, Subscriber_get_instance_handle)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("data", type);
    Topic* top = par->create_topic("topicname", "data", TOPIC_QOS_DEFAULT, nullptr, 0);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, 0);
    InstanceHandle_t handle = subscriber->get_instance_handle();
    bool result = par->contains_entity(handle);
    EXPECT_EQ(result, true);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
#endif // TEST_SUBSCRIBER
