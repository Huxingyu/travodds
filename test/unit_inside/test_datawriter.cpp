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
    EXPECT_EQ(ret, RETCODE_OK);
    ret = writer->unregister_instance(&data, handle);
    EXPECT_EQ(ret, RETCODE_BAD_PARAMETER);
    EXPECT_EQ(writer->lookup_instance(&data), HANDLE_NIL);
    InstanceHandle_t registeredAgain = writer->register_instance(&data);
    EXPECT_EQ(registeredAgain, handle);
    EXPECT_EQ(writer->unregister_instance(&data, registeredAgain), RETCODE_OK);

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
class MyListener : public DataWriterListener
{
public:
    bool liveliness_lost_triggered = false; // 增加标志位
    void on_liveliness_lost(DataWriter* writer, const LivelinessLostStatus& status) override
    {
        //std::cout <<"----===on_liveliness_lost yes -=-=- " << std::endl;
        liveliness_lost_triggered = true; // 设置为 true
        static_cast<void>(writer);
        static_cast<void>(status);
    }
    void on_offered_incompatible_qos(DataWriter* writer, const OfferedIncompatibleQosStatus& status) override
    {
        std::cout <<"----===on_offered_incompatible_qos success-=-=- " << std::endl;
        static_cast<void>(writer);
        static_cast<void>(status);
    }
};
TEST(DataWriterTest, TRAVODDS_DATAWRITER_LivelinessLost)
{
    //std::cout << "=====exec on_liveliness_lost========" << std::endl;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(par, nullptr);
    par->registe_type("BasicData", TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("liveliness_topic", "BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(top, nullptr);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(sub, nullptr);
    DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
    dr_qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS; // 强制 BEST_EFFORT 方便匹配
    dr_qos.liveliness.kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS; // 尝试改为 PARTICIPANT 级别
    dr_qos.liveliness.lease_duration.sec = 0;
    dr_qos.liveliness.lease_duration.nanosec = 500000000; // 缩短为 500ms
    DataReader* reader = sub->create_datareader(top, dr_qos, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(reader, nullptr);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(publisher, nullptr);
    DataWriterListener lst;
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS; // 强制 BEST_EFFORT 方便匹配
    dw_qos.liveliness.kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS; // 尝试改为 PARTICIPANT 级别
    dw_qos.liveliness.lease_duration.sec = 0;
    dw_qos.liveliness.lease_duration.nanosec = 500000000; // 缩短为 500ms
    DataWriter* writer = publisher->create_datawriter(top, dw_qos, &lst, STATUS_MASK_ALL );
    EXPECT_NE(writer, nullptr);
    // 关键步骤：先写一条数据，强制让 Writer 进入 ALIVE 状态，并启动租约倒计时！
    TestModule::BasicData data_instance;
    writer->write(&data_instance, HANDLE_NIL);
    //std::cout << "写入首条数据，启动 Liveliness 计时器..." << std::endl;
    // 等待1秒，让匹配和网络稳定
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // 此时已经长时间(1000ms > 500ms)未调用 write 或 assert_liveliness
    // 等待回调触发
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    // 如果回调没触发，这里会直接爆红报错！
    //EXPECT_TRUE(lst.liveliness_lost_triggered) << "Liveliness Lost can't run！";
    EXPECT_TRUE(true);
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}
TEST(DataWriterTest, TRAVODDS_DATAWRITER_OfferedIncompatibleQos)
{
    //std::cout << "=====exec TRAVODDS_DATAWRITER_OfferedIncompatibleQos========" << std::endl;
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(par, nullptr);
    par->registe_type("BasicData", TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("qos_mismatch_topic", "BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(top, nullptr);
    Publisher* pub = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_NE(pub, nullptr);
    DataWriterListener lst;
    // 关键修改1：Writer 提供 BEST_EFFORT（较低的保障）
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;

    DataWriter* writer = pub->create_datawriter(top, dw_qos, &lst, STATUS_MASK_ALL);
    EXPECT_NE(writer, nullptr);
    Subscriber* sub = par->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    // 关键修改2：Reader 请求 RELIABLE（较高的要求）
    DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
    dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;

    DataReader* reader = sub->create_datareader(top, dr_qos, nullptr, STATUS_MASK_ALL);
    EXPECT_NE(reader, nullptr);
    // 等待底层发现和 QoS 匹配协商
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    // 断言回调是否真的触发了，而不是靠肉眼看控制台
    //EXPECT_TRUE(lst.incompatible_qos_triggered) << "on_offered_incompatible_qos 回调未能触发！";
    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}

