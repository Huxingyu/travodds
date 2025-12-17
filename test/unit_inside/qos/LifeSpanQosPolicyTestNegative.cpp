#include "LifeSpanQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "testdataTypeSupport.h"

void LifeSpanQosPolicyTestNegative::SetUp() {
}

void LifeSpanQosPolicyTestNegative::TearDown() {
}

TEST_F(LifeSpanQosPolicyTestNegative, ENABLED_LifeSpanQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("LifeSpanQosPolicy"); 

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建lifespan_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "lifespan_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 4. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 5. 创建自定义DataReaderQos对象，Reliability=RELIABLE
    DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
    dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;

    // 6. 创建DataReader
    DataReader* reader = subscriber->create_datareader(
        topic, dr_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 7. 等待DataReader匹配完成
    EXPECT_TRUE(TestHelper::waitmatch(reader, 1, 5));

    // 8. 发送DataReader匹配完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("LifeSpanQosPolicy","reader_match_finish"));

    // 9. 等待主动端DataWriter写入完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("LifeSpanQosPolicy", "write_finish", dummy, 10000));

    // 10. 超过Lifespan时间后将取不到数据
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    TestModule::BasicData sample;
    SampleInfo info;
    EXPECT_EQ(reader->take_next_sample(&sample, info), RETCODE_NO_DATA);
    // 10.1 通知被动端接收完成
    EXPECT_TRUE(TestFrame::getInstance().sendData("LifeSpanQosPolicy", "reader_take_finish"));

    // 11. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 12. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 13. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("LifeSpanQosPolicy");
}
