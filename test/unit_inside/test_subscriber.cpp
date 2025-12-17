#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/subscribe/subscriber.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

/****************************** Subscriber *********************************************/
TEST(SubscriberTest, Subscriber_get_qos) {
    SubscriberQos qos;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(qos, nullptr, STATUS_MASK_NONE);
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

TEST(SubscriberTest, Subscriber_set_get_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
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

TEST(SubscriberTest, Subscriber_set_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    SubscriberQos qos;
    qos.presentation.access_scope = TOPIC_PRESENTATION_QOS;
    ReturnCode_t result = subscriber->set_qos(qos);
    EXPECT_EQ(result, RETCODE_IMMUTABLE_POLICY);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(SubscriberTest, Subscriber_get_listener) {
    SubscriberListener* listener = new SubscriberListener();
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, listener, STATUS_MASK_NONE);
    const SubscriberListener* result = subscriber->get_listener();
    EXPECT_EQ(result, listener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(SubscriberTest, Subscriber_set_get_listener) {
    SubscriberListener* listener = new SubscriberListener();
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t ret = subscriber->set_listener(listener, STATUS_MASK_ALL);
    EXPECT_EQ(ret, RETCODE_OK);
    const SubscriberListener* result = subscriber->get_listener();
    EXPECT_EQ(result, listener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    delete listener;
}
TEST(SubscriberTest, Subscriber_create_datareader)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* result = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(SubscriberTest, Subscriber_create_datareader1)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReaderQos qos;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    qos.history.depth = -1;
    DataReader* result = subscriber->create_datareader(top, qos, nullptr, STATUS_MASK_NONE);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(SubscriberTest, Subscriber_create_datareader2)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* topic = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DomainParticipant* par_ = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* topic_ = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* result = subscriber->create_datareader(topic_, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
    par_->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_);
}

TEST(SubscriberTest, Subscriber_create_datareader3)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* result = subscriber->create_datareader(nullptr, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(SubscriberTest, Subscriber_delete_datareader)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = subscriber->delete_datareader(dr);
    EXPECT_EQ(result, RETCODE_OK);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(SubscriberTest, Subscriber_delete_datareader1)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = nullptr;
    ReturnCode_t result = subscriber->delete_datareader(dr);
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(SubscriberTest, Subscriber_delete_datareader2)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* sub_ = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = sub->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, 0);
    ReturnCode_t result = sub_->delete_datareader(dr);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(SubscriberTest, Subscriber_lookup_datareader)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* result = subscriber->lookup_datareader("topicname");
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(result, dr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(SubscriberTest, Subscriber_lookup_datareader1)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* result = subscriber->lookup_datareader("A");
    EXPECT_EQ(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(SubscriberTest, Subscriber_get_participant)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const DomainParticipant* result = subscriber->get_participant();
    EXPECT_EQ(result, par);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(SubscriberTest, Subscriber_delete_contained_entities)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t result = subscriber->delete_contained_entities();
    EXPECT_EQ(result, RETCODE_OK);
    result = par->delete_subscriber(subscriber);
    EXPECT_NE(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(SubscriberTest, Subscriber_set_get_default_datareader_qos)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
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

TEST(SubscriberTest, Subscriber_set_default_datareader_qos1)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReaderQos qos;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    qos.history.depth = -1;
    ReturnCode_t result = subscriber->set_default_datareader_qos(qos);
    EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(SubscriberTest, Subscriber_copy_from_topic_qos)
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

TEST(SubscriberTest, Subscriber_create_datareader_k) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(subscriber, nullptr);
    DataReader* reader = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(reader, nullptr);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

//
TEST(SubscriberTest, Subscriber_set_qos_k) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, 0);
    EXPECT_NE(top, nullptr);
    SubscriberQos sub_qos;
    sub_qos.partition.name.resize(11);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(subscriber, nullptr);
    subscriber->set_qos(sub_qos);
    DataReader* reader = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(reader, nullptr);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(SubscriberTest, Subscriber_get_datareaders_k) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, 0);
    EXPECT_NE(top, nullptr);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(subscriber, nullptr);
    DataReader* reader = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(reader, nullptr);

    DataReaderSeq readers;
    SampleStateMask sample_states = 1;
    const ViewStateMask view_states = 1;
    const InstanceStateMask instance_states = 1;
    subscriber->get_datareaders(readers, sample_states, view_states, instance_states);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}


TEST(SubscriberTest, Subscriber_notify_datareaders) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(subscriber, nullptr);
    DataReader* reader = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(reader, nullptr);

    subscriber->notify_datareaders();

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}


TEST(SubscriberTest, Subscriber_GetListener) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(subscriber, nullptr);
    subscriber->enable();

    //SubscriberImpl* pImpl = dynamic_cast<SubscriberImpl*> (subscriber);
    //if (pImpl)
    //{
    //    const SubscriberListener* reader = pImpl->GetListener();
    //}

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(SubscriberTest, Subscriber_begin_end) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    SubscriberListener* pSubscriberListener = new SubscriberListener();
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, pSubscriberListener, STATUS_MASK_ALL);
    EXPECT_NE(subscriber, nullptr);
    subscriber->enable();

    //SubscriberImpl* pImpl = dynamic_cast<SubscriberImpl*> (subscriber);
    //if (pImpl)
    //{
    //    const SubscriberListener* reader = pImpl->GetListener();
    //}

    pSubscriberListener->on_data_on_readers(subscriber);

    subscriber->begin_access();
    subscriber->end_access();

    subscriber->set_default_datareader_qos(DATAREADER_QOS_DEFAULT);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
