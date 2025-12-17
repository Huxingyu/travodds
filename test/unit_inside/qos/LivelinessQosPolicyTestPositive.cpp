#include "QoSCompatibilityTestPositive.h"
#include "LivelinessQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "testdataTypeSupport.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

class LivelinessQosPolicyDataWriterListener : public BaseDataWriterListener {
    // 直接继承BaseDataWriterListener，无需额外实现
};

void LivelinessQosPolicyTestPositive::SetUp() {
    std::cout << "LivelinessQosPolicyTestPositive::SetUp" << std::endl;
}

void LivelinessQosPolicyTestPositive::TearDown() {
    std::cout << "LivelinessQosPolicyTestPositive::TearDown" << std::endl;
}

TEST_F(LivelinessQosPolicyTestPositive, ENABLED_LivelinessQosPolicyPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("LivelinessQosPolicyTestNegative.ENABLED_LivelinessQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("LivelinessQosPolicy"); // 创建连接

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

    // 5. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 6. DataWriterQos1: AUTOMATIC, 2000ms
    DataWriterQos qos_auto = DATAWRITER_QOS_DEFAULT;
    qos_auto.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;
    qos_auto.liveliness.lease_duration.sec = 2;
    qos_auto.liveliness.lease_duration.nanosec = 0;
    auto listener1 = new LivelinessQosPolicyDataWriterListener();
    DataWriter* writer1 = publisher->create_datawriter(
        topic1, qos_auto, listener1, STATUS_MASK_ALL);
    ASSERT_NE(writer1, nullptr);

    // 9. DataWriterQos2: MANUAL_BY_PARTICIPANT, 2000ms
    DataWriterQos qos_manual_participant = DATAWRITER_QOS_DEFAULT;
    qos_manual_participant.liveliness.kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    qos_manual_participant.liveliness.lease_duration.sec = 2;
    qos_manual_participant.liveliness.lease_duration.nanosec = 0;
    auto listener2 = new LivelinessQosPolicyDataWriterListener();
    DataWriter* writer2 = publisher->create_datawriter(
        topic2, qos_manual_participant, listener2, STATUS_MASK_ALL);
    ASSERT_NE(writer2, nullptr);
    DataWriter* writer2dup = publisher->create_datawriter(
        topic2, qos_manual_participant, listener2, STATUS_MASK_ALL);
    ASSERT_NE(writer2dup, nullptr);

    // 13. DataWriterQos3: MANUAL_BY_TOPIC, 2000ms
    DataWriterQos qos_manual_topic = DATAWRITER_QOS_DEFAULT;
    qos_manual_topic.liveliness.kind = MANUAL_BY_TOPIC_LIVELINESS_QOS;
    qos_manual_topic.liveliness.lease_duration.sec = 2;
    qos_manual_topic.liveliness.lease_duration.nanosec = 0;
    auto listener3 = new LivelinessQosPolicyDataWriterListener();
    DataWriter* writer3 = publisher->create_datawriter(
        topic3, qos_manual_topic, listener3, STATUS_MASK_ALL);
    ASSERT_NE(writer3, nullptr);

    // 16. MANUAL_BY_PARTICIPANT类型需要手动调用assert_liveliness
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(writer2->assert_liveliness(), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    LivelinessLostStatus lost_status;
    EXPECT_EQ(writer2->get_liveliness_lost_status(lost_status), RETCODE_OK);
    EXPECT_EQ(lost_status.total_count, 0);
    EXPECT_EQ(lost_status.total_count_change, 0);
    EXPECT_EQ(writer2dup->get_liveliness_lost_status(lost_status), RETCODE_OK);
    EXPECT_EQ(lost_status.total_count, 0);
    EXPECT_EQ(lost_status.total_count_change, 0);
    //两次时延之间加上操作时间略大于2s。会触发liveliness_lost，不会在wait_liveliness_lost中超时 这一步的目的是什么？
    EXPECT_EQ(listener2->wait_liveliness_lost(900), 1); //超时
    //EXPECT_EQ(listener2->wait_liveliness_lost(1000), 1); // 超时

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    EXPECT_EQ(writer2->get_liveliness_lost_status(lost_status), RETCODE_OK);
    EXPECT_EQ(lost_status.total_count, 1);
    //listener通知过后 total_count_change会被重置
    EXPECT_EQ(lost_status.total_count_change, 0);
    EXPECT_EQ(writer2dup->get_liveliness_lost_status(lost_status), RETCODE_OK);
    EXPECT_EQ(lost_status.total_count, 1);
    //listener通知过后 total_count_change会被重置
    EXPECT_EQ(lost_status.total_count_change, 0);
    EXPECT_EQ(listener2->wait_liveliness_lost(1000), 0); // 成功

    // 17. MANUAL_BY_TOPIC类型需要手动调用assert_liveliness
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(writer3->assert_liveliness(), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    EXPECT_EQ(writer3->get_liveliness_lost_status(lost_status), RETCODE_OK);
    EXPECT_EQ(lost_status.total_count, 0);
    EXPECT_EQ(lost_status.total_count_change, 0);
    //两次时延之间加上操作时间略大于2s。会触发liveliness_lost，不会在wait_liveliness_lost中超时
    EXPECT_EQ(listener3->wait_liveliness_lost(900), 1); // 超时
    //EXPECT_EQ(listener3->wait_liveliness_lost(1000), 1); // 超时

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    EXPECT_EQ(writer3->get_liveliness_lost_status(lost_status), RETCODE_OK);
    EXPECT_EQ(lost_status.total_count, 1);
    //listener通知过后 total_count_change会被重置
    EXPECT_EQ(lost_status.total_count_change, 0);
    EXPECT_EQ(listener3->wait_liveliness_lost(1000), 0); // 成功

    // 18. 发送写入完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("LivelinessQosPolicy","write_finish"));

    // 19. 等待被动端检查完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("LivelinessQosPolicy", "reader_check_finish", dummy, 10000));

    // 20. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 21. 通知被动端已删除
    EXPECT_TRUE(TestFrame::getInstance().sendData("LivelinessQosPolicy","dp_deleted"));

    // 22. finalize_instance
    factory->finalize();

    delete listener1;
    delete listener2;
    delete listener3;
    DESTORY_CONNECTION("LivelinessQosPolicy");
}