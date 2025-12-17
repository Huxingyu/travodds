#include "ReliabilityQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/publisher.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "testdataTypeSupport.h"


USING_TRAVODDS_NAMESPACE

void ReliabilityQosPolicyTestPositive::SetUp() {
}

void ReliabilityQosPolicyTestPositive::TearDown() {
}

TEST_F(ReliabilityQosPolicyTestPositive, ENABLED__ReliabilityQosPolicyPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("ReliabilityQosPolicyTestNegative.ENABLED_ReliabilityQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("ReliabilityQosPolicy"); // 创建连接

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
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
    dw_qos.reliability.max_blocking_time.sec = 2;
    dw_qos.reliability.max_blocking_time.nanosec = 0;
    dw_qos.history.depth = 20;
    DataWriter* writer = publisher->create_datawriter(
        topic, dw_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 6. 等待被动端DataReader匹配完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("ReliabilityQosPolicy", "reader_match_finish", dummy, 10000));

    // 7. 周期为1ms发送1000包样本数据
    for (int i = 0; i < 100; ++i) {
        TestModule::BasicData sample;
        sample.id = i;
        EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    // 8. 发送DataWriter写入完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("ReliabilityQosPolicy","write_finish"));

    // 9. 等待被动端DataReader接收完成
    EXPECT_TRUE(TestFrame::getInstance().waitForData("ReliabilityQosPolicy", "reader_take_finish", dummy, 10000));

    // 10. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 11. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 12. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("ReliabilityQosPolicy");
}