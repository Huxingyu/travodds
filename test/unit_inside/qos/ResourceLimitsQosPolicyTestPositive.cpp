#include "ResourceLimitsQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "testdataTypeSupport.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/publisher.h"
//#include "types/TestModule::BasicDataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

void ResourceLimitsQosPolicyTestPositive::SetUp() {
}

void ResourceLimitsQosPolicyTestPositive::TearDown() {
}

TEST_F(ResourceLimitsQosPolicyTestPositive, ENABLED_ResourceLimitsQosPolicyPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("ResourceLimitsQosPolicyTestNegative.ENABLED_ResourceLimitsQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("ResourceLimitsQosPolicy"); // 创建连接

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

    // 6. 创建自定义DataWriterQos对象2
    DataWriterQos dw_qos2 = DATAWRITER_QOS_DEFAULT;
    dw_qos2.reliability.kind = RELIABLE_RELIABILITY_QOS;
    dw_qos2.reliability.max_blocking_time.sec = 2;
    dw_qos2.reliability.max_blocking_time.nanosec = 0;
    dw_qos2.history.kind = KEEP_ALL_HISTORY_QOS;
    dw_qos2.history.depth = 1;
    dw_qos2.resource_limits.max_samples = 100;
    dw_qos2.resource_limits.max_samples_per_instance = 100;
    dw_qos2.resource_limits.max_instances = 1;

    // 7. 创建DataWriter1
    DataWriter* writer1 = publisher->create_datawriter(
        topic, dw_qos1, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer1, nullptr);

    // 8. 创建DataWriter2
    DataWriter* writer2 = publisher->create_datawriter(
        topic, dw_qos2, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer2, nullptr);

    // 9. 等待被动端DataReader匹配完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("ResourceLimitsQosPolicy", "reader_match_finish", dummy, 10000));

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
    EXPECT_TRUE(TestFrame::getInstance().sendData("ResourceLimitsQosPolicy","timeout_finish"));

    // 10.4 重新写入1包，预期RETCODE_OK
    sample.id = 101;
    EXPECT_EQ(writer1->write(&sample, HANDLE_NIL), RETCODE_OK);

    // 11. DataWriter2写入50包数据，预期成功
    for (int i = 0; i < 50; ++i) {
        TestModule::BasicData s;
        s.id = i + 1000;
        EXPECT_EQ(writer2->write(&s, HANDLE_NIL), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 12. 发送DataWriter2写入完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("ResourceLimitsQosPolicy","write_finish2"));

    // 13. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 14. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 15. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("ResourceLimitsQosPolicy");
}