#include "QoSCompatibilityTestNegative.h"
#include "LivelinessQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

class LivelinessQosPolicyDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};

void LivelinessQosPolicyTestNegative::SetUp() {
}

void LivelinessQosPolicyTestNegative::TearDown() {
}

TEST_F(LivelinessQosPolicyTestNegative, ENABLED_LivelinessQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("LivelinessQosPolicy");

    // 2. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 3. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建3个主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic1 = participant->create_topic(
        "liveliness_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    Topic* topic2 = participant->create_topic(
        "liveliness_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic2, nullptr);
    Topic* topic3 = participant->create_topic(
        "liveliness_topic_name_3", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic3, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. DataReaderQos1: RELIABLE, AUTOMATIC, 4000ms
    DataReaderQos drqos1 = DATAREADER_QOS_DEFAULT;
    drqos1.reliability.kind = RELIABLE_RELIABILITY_QOS;
    drqos1.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;
    drqos1.liveliness.lease_duration.sec = 4;
    drqos1.liveliness.lease_duration.nanosec = 0;
    auto listener1 = new LivelinessQosPolicyDataReaderListener();
    DataReader* reader1 = subscriber->create_datareader(
        topic1, drqos1, listener1, STATUS_MASK_ALL);
    ASSERT_NE(reader1, nullptr);

    // 9. DataReaderQos2: RELIABLE, MANUAL_BY_PARTICIPANT, 4000ms
    DataReaderQos drqos2 = DATAREADER_QOS_DEFAULT;
    drqos2.reliability.kind = RELIABLE_RELIABILITY_QOS;
    drqos2.liveliness.kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    drqos2.liveliness.lease_duration.sec = 4;
    drqos2.liveliness.lease_duration.nanosec = 0;
    auto listener2 = new LivelinessQosPolicyDataReaderListener();
    DataReader* reader2 = subscriber->create_datareader(
        topic2, drqos2, listener2, STATUS_MASK_ALL);
    ASSERT_NE(reader2, nullptr);

    // 12. DataReaderQos3: RELIABLE, MANUAL_BY_TOPIC, 4000ms
    DataReaderQos drqos3 = DATAREADER_QOS_DEFAULT;
    drqos3.reliability.kind = RELIABLE_RELIABILITY_QOS;
    drqos3.liveliness.kind = MANUAL_BY_TOPIC_LIVELINESS_QOS;
    drqos3.liveliness.lease_duration.sec = 4;
    drqos3.liveliness.lease_duration.nanosec = 0;
    auto listener3 = new LivelinessQosPolicyDataReaderListener();
    DataReader* reader3 = subscriber->create_datareader(
        topic3, drqos3, listener3, STATUS_MASK_ALL);
    ASSERT_NE(reader3, nullptr);

    // 15-17. 等待匹配
    EXPECT_TRUE(TestHelper::waitmatch(reader1, 1, 5));
    EXPECT_TRUE(TestHelper::waitmatch(reader2, 1, 5));
    EXPECT_TRUE(TestHelper::waitmatch(reader3, 1, 5));

    // 18. 等待主动端写入完成
    std::string dummy;
    //EXPECT_TRUE(TestFrame::getInstance().waitForData("LivelinessQosPolicy", "write_finish", dummy, 10000));
    //发送端那边不止10s
    EXPECT_TRUE(TestFrame::getInstance().waitForData("LivelinessQosPolicy", "write_finish", dummy, 50000));

    // 19. 延迟6000ms
    std::this_thread::sleep_for(std::chrono::milliseconds(6000));

    // 20. MANUAL_BY_PARTICIPANT类型检测liveliness变化
    LivelinessChangedStatus changed1;
    EXPECT_EQ(reader2->get_liveliness_changed_status(changed1), RETCODE_OK);
    EXPECT_EQ(changed1.alive_count, 0);
    //listener中被重置为0
    //EXPECT_EQ(changed1.alive_count_change, -1);
    EXPECT_EQ(changed1.alive_count_change, 0);

    // 21. MANUAL_BY_TOPIC类型检测liveliness变化
    LivelinessChangedStatus changed3;
    EXPECT_EQ(reader3->get_liveliness_changed_status(changed3), RETCODE_OK);
    EXPECT_EQ(changed3.alive_count, 0);
    //listener中被重置为0
    //EXPECT_EQ(changed3.alive_count_change, -1);
    EXPECT_EQ(changed3.alive_count_change, 0);

    // 22. 发送检查完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("LivelinessQosPolicy","reader_check_finish"));

    // 23. 等待主动端删除域参与者
    EXPECT_TRUE(TestFrame::getInstance().waitForData("LivelinessQosPolicy", "dp_deleted", dummy, 10000));

    // 24. 延迟5000ms
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    // 25. AUTOMATIC类型检测liveliness变化
    LivelinessChangedStatus changed_auto;
    EXPECT_EQ(reader1->get_liveliness_changed_status(changed_auto), RETCODE_OK);

    EXPECT_EQ(changed_auto.alive_count, 1);
    //listener中 读取后会重置
    EXPECT_EQ(changed_auto.alive_count_change, 0);
    //EXPECT_EQ(changed_auto.alive_count_change, 1);

    // 26. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 27. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 28. finalize_instance
    factory->finalize();

    //delete listener1;
    //delete listener2;
    //delete listener3;
    DESTORY_CONNECTION("LivelinessQosPolicy");
}