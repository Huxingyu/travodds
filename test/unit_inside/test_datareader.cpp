#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/subscribe/subscriber.h"
#include "dcps/subscribe/datareader.h"
#include "testdataTypeSupport.h"

#include <chrono>

USING_TRAVODDS_NAMESPACE

/****************************** DataReader *********************************************/
TEST(DataReaderTest, DataReader_get_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReaderQos qos;
    DataReader* dr = subscriber->create_datareader(top, qos, nullptr, STATUS_MASK_NONE);
    DataReaderQos qosGet;
    ReturnCode_t result = dr->get_qos(qosGet);
    EXPECT_EQ(result, RETCODE_OK);
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

TEST(DataReaderTest, DataReader_set_get_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReaderQos qos;
    ReturnCode_t result = dr->set_qos(qos);
    EXPECT_EQ(result, RETCODE_OK);
    DataReaderQos qosGet;
    ReturnCode_t resultGet = dr->get_qos(qosGet);
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
TEST(DataReaderTest, DataReader_set_qos) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReaderQos qos;
    qos.history.kind = KEEP_LAST_HISTORY_QOS;
    qos.history.depth = -1;
    ReturnCode_t result = dr->set_qos(qos);
    EXPECT_EQ(result, RETCODE_INCONSISTENT_POLICY);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataReaderTest, DataReader_set_qos1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReaderQos qos;
    qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    ReturnCode_t result = dr->set_qos(qos);
    EXPECT_EQ(result, RETCODE_IMMUTABLE_POLICY);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DataReaderTest, DataReader_set_qos2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    TopicQos TOPIC_QOS_DEFAULT;
    Topic* top = par->create_topic("topicname", "KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(subscriber, nullptr);
    DataReader* reader = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(reader, nullptr);

    DataReaderQos dr_qos;
    dr_qos.deadline.period = { 5,0 };
    EXPECT_EQ(reader->set_qos(dr_qos), RETCODE_OK);
    dr_qos.deadline.period = { 10,0 };
    EXPECT_EQ(reader->set_qos(dr_qos), RETCODE_OK);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DataReaderTest, DataReader_get_listener) {
    DataReaderListener* dwlistener = new DataReaderListener;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, dwlistener, STATUS_MASK_NONE);
    const DataReaderListener* result = dr->get_listener();
    EXPECT_EQ(result, dwlistener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DataReaderTest, DataReader_set_get_listener) {
    DataReaderListener* dwlistener = new DataReaderListener;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReturnCode_t ret = dr->set_listener(dwlistener, 0);
    EXPECT_EQ(ret, RETCODE_OK);

    const DataReaderListener* result = dr->get_listener();
    EXPECT_EQ(result, dwlistener);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DataReaderTest, DataReader_create_readcondition) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReadCondition* result = dr->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    EXPECT_NE(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DataReaderTest, DataReader_create_querycondition) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    StringSeq query_parameters;
    QueryCondition* result = dr->create_querycondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, "query_expression", query_parameters);
    EXPECT_NE(result, nullptr);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DataReaderTest, DataReader_delete_readcondition) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReadCondition* rc = dr->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    ReturnCode_t result = dr->delete_readcondition(rc);
    EXPECT_EQ(result, RETCODE_OK);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataReaderTest, DataReader_delete_readcondition1) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReadCondition* rc = nullptr;
    ReturnCode_t result = dr->delete_readcondition(rc);
    EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataReaderTest, DataReader_delete_readcondition2) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr_ = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReadCondition* rc = dr->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    ReturnCode_t result = dr_->delete_readcondition(rc);
    EXPECT_EQ(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataReaderTest, DataReader_delete_readcondition3) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    StringSeq query_parameters;
    QueryCondition* qc = dr->create_querycondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, "query_expression", query_parameters);
    ReturnCode_t result = dr->delete_readcondition(qc);
    EXPECT_EQ(result, RETCODE_OK);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DataReaderTest, DataReader_get_topicdescription) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const TopicDescription* result = dr->get_topicdescription();
    EXPECT_EQ(result, top);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

TEST(DataReaderTest, DataReader_get_subscriber) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const Subscriber* result = dr->get_subscriber();
    EXPECT_EQ(result, subscriber);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataReaderTest, DataReader_delete_contained_entities) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ReadCondition* rc = dr->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    ReturnCode_t result = dr->delete_contained_entities();
    EXPECT_EQ(result, RETCODE_OK);
    result = subscriber->delete_datareader(dr);
    EXPECT_NE(result, RETCODE_PRECONDITION_NOT_MET);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
//// 存在key，进行注册
TEST(DataReaderTest, DataReader_ContentFilteredTopic) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    TopicQos qos;
    Topic* top = par->create_topic("topicname", "KeyData", qos, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(top, nullptr);
    std::string filter_expression = "userId > 5";
    StringSeq expression_parameters;
    ContentFilteredTopic* filterTop = par->create_contentfilteredtopic("CFTopicname", top, filter_expression, expression_parameters);
    EXPECT_NE(top, nullptr);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(subscriber, nullptr);
    DataReader* reader = subscriber->create_datareader(filterTop, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(reader, nullptr);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}


TEST(DataReaderTest, DataReader_read_take) {
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    TopicQos qos;
    Topic* top = par->create_topic("topicname", "KeyData", qos, nullptr, 0);
    EXPECT_NE(top, nullptr);

    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, new SubscriberListener(), STATUS_MASK_ALL);
    EXPECT_NE(subscriber, nullptr);
    DataReaderListener* pDataReaderListener = new DataReaderListener();
    DataReader* reader = subscriber->create_datareader(top, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(reader, nullptr);

    pDataReaderListener->on_data_available(nullptr);
    LivelinessChangedStatus status;
    pDataReaderListener->on_liveliness_changed(nullptr, status);

    RequestedIncompatibleQosStatus _RequestedIncompatibleQosStatus;
    pDataReaderListener->on_requested_incompatible_qos(nullptr, _RequestedIncompatibleQosStatus);

    RequestedDeadlineMissedStatus _RequestedDeadlineMissedStatus;
    pDataReaderListener->on_requested_deadline_missed(nullptr, _RequestedDeadlineMissedStatus);

    SampleRejectedStatus _SampleRejectedStatus;
	pDataReaderListener->on_sample_rejected(nullptr, _SampleRejectedStatus);

	SampleLostStatus _SampleLostStatus;
	pDataReaderListener->on_sample_lost(nullptr, _SampleLostStatus);

    TRAVODDS_SEQUENCE(KeyDataSeq, TestModule::KeyData);

    KeyDataSeq dataSeq;
    TestModule::KeyData data;
    SampleInfoSeq sample_infos;
    SampleInfo sampleinfo;
    //int32_t max_samples, SampleStateMask sample_states,
    //	ViewStateMask view_states, InstanceStateMask instance_states
    reader->read(dataSeq, sample_infos, 10, 0xffff, 0xffff, 0xffff);

    ReadCondition* cond = reader->create_readcondition(0xffff, 0xffff, 0xffff);
    reader->read(dataSeq, sample_infos, 10, 0xffff, 0xffff, 0xffff);
    reader->read_next_sample(&data, sampleinfo);
    reader->read_w_condition(dataSeq, sample_infos, 10, cond);
    reader->read_instance(dataSeq, sample_infos, 10, HANDLE_NIL, 0xffff, 0xffff, 0xffff);
    reader->read_next_instance(dataSeq, sample_infos, 10, HANDLE_NIL, 0xffff, 0xffff, 0xffff);
    reader->read_next_instance_w_condition(dataSeq, sample_infos, 10, HANDLE_NIL, cond);


    reader->take(dataSeq, sample_infos, 10, 0xffff, 0xffff, 0xffff);
    reader->take_next_sample(&data, sampleinfo);
    reader->take_w_condition(dataSeq, sample_infos, 10, cond);
    reader->take_instance(dataSeq, sample_infos, 10, HANDLE_NIL, 0xffff, 0xffff, 0xffff);
    reader->take_next_instance(dataSeq, sample_infos, 10, HANDLE_NIL, 0xffff, 0xffff, 0xffff);
    reader->take_next_instance_w_condition(dataSeq, sample_infos, 10, HANDLE_NIL, cond);


    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataReaderTest, DataReader_return_load) {

    TRAVODDS_SEQUENCE(KeyDataSeq, TestModule::KeyData);


    DomainParticipant* par2 = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par2->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    TopicQos qos;
    Topic* top2 = par2->create_topic("topicname_KeyData", "KeyData", qos, nullptr, 0);
    EXPECT_NE(top2, nullptr);
    Publisher* pub = par2->create_publisher(PUBLISHER_QOS_DEFAULT, new PublisherListener(), STATUS_MASK_ALL);
    EXPECT_NE(pub, nullptr);
    DataWriterQos dw_qos;
    dw_qos.deadline.period = { 3,0 };
    dw_qos.lifespan.duration = { 1, 0 };
    DataWriter* writer = pub->create_datawriter(top2, dw_qos, new DataWriterListener, STATUS_MASK_ALL);
    EXPECT_NE(writer, nullptr);


    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, 0);
    par->registe_type("KeyData", TestModule::KeyDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname_KeyData", "KeyData", qos, nullptr, 0);
    EXPECT_NE(top, nullptr);
    Subscriber* subscriber = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, new SubscriberListener(), STATUS_MASK_ALL);
    EXPECT_NE(subscriber, nullptr);
    DataReaderQos dr_qos;
    dr_qos.deadline.period = { 5,0 };
    DataReader* reader = subscriber->create_datareader(top, dr_qos, new DataReaderListener, STATUS_MASK_ALL);
    EXPECT_NE(reader, nullptr);


    std::this_thread::sleep_for(std::chrono::seconds(1));

    KeyDataSeq data_values;
    SampleInfoSeq sample_infos;
    reader->return_loan(data_values, sample_infos);

    TestModule::KeyData data;
    reader->lookup_instance(&data);

    writer->write(&data, HANDLE_NIL);

    InstanceHandle_t handle;
    reader->get_key_value(&data, handle);

    reader->lookup_instance(&data);

    LivelinessChangedStatus _LivelinessChangedStatus;
    reader->get_liveliness_changed_status(_LivelinessChangedStatus);

    RequestedDeadlineMissedStatus _RequestedDeadlineMissedStatus;
    reader->get_requested_deadline_missed_status(_RequestedDeadlineMissedStatus);

    RequestedIncompatibleQosStatus _RequestedIncompatibleQosStatus;
    reader->get_requested_incompatible_qos_status(_RequestedIncompatibleQosStatus);

    SampleLostStatus _SampleLostStatus;
    reader->get_sample_lost_status(_SampleLostStatus);

    SampleRejectedStatus _SampleRejectedStatus;
    reader->get_sample_rejected_status(_SampleRejectedStatus);

    SubscriptionMatchedStatus _SubscriptionMatchedStatus;
    reader->get_subscription_matched_status(_SubscriptionMatchedStatus);

    Duration_t time = { 1,0 };
    reader->wait_for_historical_data(time);

    InstanceHandleSeq handleSeq;

    reader->get_matched_publications(handleSeq);
    //reader-

    if (handleSeq.size() > 0)
    {
        PublicationBuiltinTopicData dw_data;
        reader->get_matched_publication_data(dw_data, handleSeq[0]);
    }

	par->delete_contained_entities();
	par2->delete_contained_entities();

    TheParticipantFactory->delete_participant(par);
    TheParticipantFactory->delete_participant(par2);
}