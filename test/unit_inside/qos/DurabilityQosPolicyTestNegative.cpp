#include "DurabilityQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

void DurabilityQosPolicyTestNegative::SetUp() {
}

void DurabilityQosPolicyTestNegative::TearDown() {
}

TEST_F(DurabilityQosPolicyTestNegative, ENABLED_DurabilityQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("DurabilityQosPolicy");

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建durability_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "durability_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 4. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 5. 等待主动端写入完成信号
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("DurabilityQosPolicy", "write_finish", dummy, 10000));

    // 6. VOLATILE类型DataReader接收不到数据
    DataReaderQos dr_qos1 = DATAREADER_QOS_DEFAULT;
    dr_qos1.reliability.kind = RELIABLE_RELIABILITY_QOS;
    dr_qos1.durability.kind = VOLATILE_DURABILITY_QOS;
    DataReader* reader1 = subscriber->create_datareader(
        topic, dr_qos1, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader1, nullptr);
    EXPECT_TRUE(TestHelper::waitmatch(reader1, 1, 5));
    TestModule::BasicData sample;
    SampleInfo info;
    EXPECT_EQ(reader1->take_next_sample(&sample, info), RETCODE_NO_DATA);

    // 7. TRANSIENT_LOCAL类型DataReader接收得到数据
    DataReaderQos dr_qos2 = DATAREADER_QOS_DEFAULT;
    dr_qos2.reliability.kind = RELIABLE_RELIABILITY_QOS;
    dr_qos2.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    DataReader* reader2 = subscriber->create_datareader(
        topic, dr_qos2, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader2, nullptr);
    EXPECT_TRUE(TestHelper::waitmatch(reader2, 1, 5));
    //重传需要时间，所以需要等待一下
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(reader2->take_next_sample(&sample, info), RETCODE_OK);

    // 8. 发送检查完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("DurabilityQosPolicy","reader_check_finish"));

    // 9. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 10. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 11. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("DurabilityQosPolicy");
}