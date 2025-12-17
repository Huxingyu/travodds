#include "QoSCompatibilityTestNegative.h"
#include "TestFrame.h"
#include "TestHelper.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/publisher.h"
//#include "types/TestModule::BasicDataTypeSupport.h"

#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE


void QoSCompatibilityTestNegative::SetUp() {
}

void QoSCompatibilityTestNegative::TearDown() {
}

TEST_F(QoSCompatibilityTestNegative, ENABLED_DurabilityMatchNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("DurabilityMatch");

    // 2. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 3. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建4个主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic1 = participant->create_topic(
        "durability_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    Topic* topic2 = participant->create_topic(
        "durability_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic2, nullptr);
    Topic* topic3 = participant->create_topic(
        "durability_match_topic_name_3", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic3, nullptr);
    Topic* topic4 = participant->create_topic(
        "durability_match_topic_name_4", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic4, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 创建DataReaderQos对象1，Durability=VOLATILE
    DataReaderQos qos_volatile = DATAREADER_QOS_DEFAULT;
    qos_volatile.durability.kind = VOLATILE_DURABILITY_QOS;

    // 7. 创建Listener1
    auto listener1 = new DurabilityMatchDataReaderListener();
    // 8. DataReader1
    DataReader* reader1 = subscriber->create_datareader(
        topic1, qos_volatile, listener1, STATUS_MASK_ALL);
    ASSERT_NE(reader1, nullptr);

    // 9. DataReaderQos对象2，Durability=TRANSIENT_LOCAL
    DataReaderQos qos_transient = DATAREADER_QOS_DEFAULT;
    qos_transient.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    // 10. 创建Listener2
    auto listener2 = new DurabilityMatchDataReaderListener();
    // 11. DataReader2
    DataReader* reader2 = subscriber->create_datareader(
        topic2, qos_transient, listener2, STATUS_MASK_ALL);
    ASSERT_NE(reader2, nullptr);

    // 12. 创建Listener3
    auto listener3 = new DurabilityMatchDataReaderListener();
    // 13. DataReader3
    DataReader* reader3 = subscriber->create_datareader(
        topic3, qos_volatile, listener3, STATUS_MASK_ALL);
    ASSERT_NE(reader3, nullptr);

    // 14. 创建Listener4
    auto listener4 = new DurabilityMatchDataReaderListener();
    // 15. DataReader4
    DataReader* reader4 = subscriber->create_datareader(
        topic4, qos_transient, listener4, STATUS_MASK_ALL);
    ASSERT_NE(reader4, nullptr);

    // 16. 发送DataReader创建完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("DurabilityMatch", "reader_created_finish"));

    // 17. VOLATILE Reader & VOLATILE Writer 匹配
    EXPECT_EQ(listener1->wait_subscription_matched(5000), 0); // 成功
    EXPECT_EQ(listener1->wait_requested_incompatible_qos(1000), 1); // 超时
    SubscriptionMatchedStatus sub_status1;
    EXPECT_EQ(reader1->get_subscription_matched_status(sub_status1), RETCODE_OK);
    EXPECT_EQ(sub_status1.total_count, 1);
    EXPECT_EQ(sub_status1.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status1;
    EXPECT_EQ(reader1->get_requested_incompatible_qos_status(rqos_status1), RETCODE_OK);
    EXPECT_EQ(rqos_status1.total_count, 0);
    EXPECT_EQ(rqos_status1.total_count_change, 0);

    // 18. TRANSIENT_LOCAL Reader & VOLATILE Writer 不匹配
    EXPECT_EQ(listener2->wait_subscription_matched(1000), 1); // 超时
    EXPECT_EQ(listener2->wait_requested_incompatible_qos(5000), 0); // 成功
    SubscriptionMatchedStatus sub_status2;
    EXPECT_EQ(reader2->get_subscription_matched_status(sub_status2), RETCODE_OK);
    EXPECT_EQ(sub_status2.total_count, 0);
    EXPECT_EQ(sub_status2.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status2;
    EXPECT_EQ(reader2->get_requested_incompatible_qos_status(rqos_status2), RETCODE_OK);
    EXPECT_EQ(rqos_status2.total_count, 1);
    EXPECT_EQ(rqos_status2.total_count_change, 0);
    EXPECT_EQ(rqos_status2.last_policy_id, DURABILITY_QOS_POLICY_ID);

    // 19. VOLATILE Reader & TRANSIENT_LOCAL Writer 匹配
    EXPECT_EQ(listener3->wait_subscription_matched(1000), 0); // 超时
    EXPECT_EQ(listener3->wait_requested_incompatible_qos(5000), 1); // 成功
    SubscriptionMatchedStatus sub_status3;
    EXPECT_EQ(reader3->get_subscription_matched_status(sub_status3), RETCODE_OK);
    EXPECT_EQ(sub_status3.total_count, 1);
    EXPECT_EQ(sub_status3.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status3;
    EXPECT_EQ(reader3->get_requested_incompatible_qos_status(rqos_status3), RETCODE_OK);
    EXPECT_EQ(rqos_status3.total_count, 0);
    EXPECT_EQ(rqos_status3.total_count_change, 0);
    //EXPECT_EQ(rqos_status3.last_policy_id, DURABILITY_QOS_POLICY_ID);

    // 20. TRANSIENT_LOCAL Reader & TRANSIENT_LOCAL Writer 匹配
    EXPECT_EQ(listener4->wait_subscription_matched(5000), 0); // 成功
    EXPECT_EQ(listener4->wait_requested_incompatible_qos(1000), 1); // 超时
    SubscriptionMatchedStatus sub_status4;
    EXPECT_EQ(reader4->get_subscription_matched_status(sub_status4), RETCODE_OK);
    EXPECT_EQ(sub_status4.total_count, 1);
    EXPECT_EQ(sub_status4.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status4;
    EXPECT_EQ(reader4->get_requested_incompatible_qos_status(rqos_status4), RETCODE_OK);
    EXPECT_EQ(rqos_status4.total_count, 0);
    EXPECT_EQ(rqos_status4.total_count_change, 0);

    // 21. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 22. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 23. finalize_instance
    factory->finalize();

    // 清理listener
    delete listener1;
    delete listener2;
    delete listener3;
    delete listener4;
    DESTORY_CONNECTION("DurabilityMatch");
}

TEST_F(QoSCompatibilityTestNegative, ENABLED_ReliabilityMatchNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("ReliabilityMatch");

    // 2. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 3. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建4个主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic1 = participant->create_topic(
        "reliability_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    Topic* topic2 = participant->create_topic(
        "reliability_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic2, nullptr);
    Topic* topic3 = participant->create_topic(
        "reliability_match_topic_name_3", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic3, nullptr);
    Topic* topic4 = participant->create_topic(
        "reliability_match_topic_name_4", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic4, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 创建DataReaderQos对象1，Reliability=BEST_EFFORT
    DataReaderQos qos_best_effort = DATAREADER_QOS_DEFAULT;
    qos_best_effort.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;

    // 7. 创建Listener1
    auto listener1 = new ReliabilityMatchDataReaderListener();
    // 8. DataReader1
    DataReader* reader1 = subscriber->create_datareader(
        topic1, qos_best_effort, listener1, STATUS_MASK_ALL);
    ASSERT_NE(reader1, nullptr);

    // 9. DataReaderQos对象2，Reliability=RELIABLE
    DataReaderQos qos_reliable = DATAREADER_QOS_DEFAULT;
    qos_reliable.reliability.kind = RELIABLE_RELIABILITY_QOS;

    // 10. 创建Listener2
    auto listener2 = new ReliabilityMatchDataReaderListener();
    // 11. DataReader2
    DataReader* reader2 = subscriber->create_datareader(
        topic2, qos_reliable, listener2, STATUS_MASK_ALL);
    ASSERT_NE(reader2, nullptr);

    // 12. 创建Listener3
    auto listener3 = new ReliabilityMatchDataReaderListener();
    // 13. DataReader3
    DataReader* reader3 = subscriber->create_datareader(
        topic3, qos_best_effort, listener3, STATUS_MASK_ALL);
    ASSERT_NE(reader3, nullptr);

    // 14. 创建Listener4
    auto listener4 = new ReliabilityMatchDataReaderListener();
    // 15. DataReader4
    DataReader* reader4 = subscriber->create_datareader(
        topic4, qos_reliable, listener4, STATUS_MASK_ALL);
    ASSERT_NE(reader4, nullptr);

    // 16. 发送DataReader创建完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("ReliabilityMatch", "reader_created_finish"));

    // 17. BEST_EFFORT Reader & BEST_EFFORT Writer 匹配
    EXPECT_EQ(listener1->wait_subscription_matched(5000), 0); // 成功
    EXPECT_EQ(listener1->wait_requested_incompatible_qos(1000), 1); // 超时
    SubscriptionMatchedStatus sub_status1;
    EXPECT_EQ(reader1->get_subscription_matched_status(sub_status1), RETCODE_OK);
    EXPECT_EQ(sub_status1.total_count, 1);
    EXPECT_EQ(sub_status1.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status1;
    EXPECT_EQ(reader1->get_requested_incompatible_qos_status(rqos_status1), RETCODE_OK);
    EXPECT_EQ(rqos_status1.total_count, 0);
    EXPECT_EQ(rqos_status1.total_count_change, 0);

    // 18. RELIABLE Reader & BEST_EFFORT Writer 不匹配
    EXPECT_EQ(listener2->wait_subscription_matched(1000), 1); // 超时
    EXPECT_EQ(listener2->wait_requested_incompatible_qos(5000), 0); // 成功
    SubscriptionMatchedStatus sub_status2;
    EXPECT_EQ(reader2->get_subscription_matched_status(sub_status2), RETCODE_OK);
    EXPECT_EQ(sub_status2.total_count, 0);
    EXPECT_EQ(sub_status2.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status2;
    EXPECT_EQ(reader2->get_requested_incompatible_qos_status(rqos_status2), RETCODE_OK);
    EXPECT_EQ(rqos_status2.total_count, 1);
    EXPECT_EQ(rqos_status2.total_count_change, 0);
    EXPECT_EQ(rqos_status2.last_policy_id, RELIABILITY_QOS_POLICY_ID);

    // 19. BEST_EFFORT Reader & RELIABLE Writer 匹配
    EXPECT_EQ(listener3->wait_subscription_matched(1000), 0); // 超时
    EXPECT_EQ(listener3->wait_requested_incompatible_qos(5000), 1); // 成功
    SubscriptionMatchedStatus sub_status3;
    EXPECT_EQ(reader3->get_subscription_matched_status(sub_status3), RETCODE_OK);
    EXPECT_EQ(sub_status3.total_count, 1);
    EXPECT_EQ(sub_status3.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status3;
    EXPECT_EQ(reader3->get_requested_incompatible_qos_status(rqos_status3), RETCODE_OK);
    EXPECT_EQ(rqos_status3.total_count, 0);
    EXPECT_EQ(rqos_status3.total_count_change, 0);
    //EXPECT_EQ(rqos_status3.last_policy_id, RELIABILITY_QOS_POLICY_ID);

    // 20. RELIABLE Reader & RELIABLE Writer 匹配
    EXPECT_EQ(listener4->wait_subscription_matched(5000), 0); // 成功
    EXPECT_EQ(listener4->wait_requested_incompatible_qos(1000), 1); // 超时
    SubscriptionMatchedStatus sub_status4;
    EXPECT_EQ(reader4->get_subscription_matched_status(sub_status4), RETCODE_OK);
    EXPECT_EQ(sub_status4.total_count, 1);
    EXPECT_EQ(sub_status4.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status4;
    EXPECT_EQ(reader4->get_requested_incompatible_qos_status(rqos_status4), RETCODE_OK);
    EXPECT_EQ(rqos_status4.total_count, 0);
    EXPECT_EQ(rqos_status4.total_count_change, 0);

    // 21. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 22. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 23. finalize_instance
    factory->finalize();

    // 清理listener
    delete listener1;
    delete listener2;
    delete listener3;
    delete listener4;
    DESTORY_CONNECTION("ReliabilityMatch");
}

TEST_F(QoSCompatibilityTestNegative, ENABLED_DeadlineMatchNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("DeadlineMatch");

    // 2. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 3. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建2个主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic1 = participant->create_topic(
        "deadline_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    Topic* topic2 = participant->create_topic(
        "deadline_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic2, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 创建DataReaderQos对象1，Deadline.period=500ms
    DataReaderQos qos1 = DATAREADER_QOS_DEFAULT;
    qos1.deadline.period.sec = 0;
    qos1.deadline.period.nanosec = 500 * 1000 * 1000; // 500ms

    // 7. 创建Listener1
    auto listener1 = new DeadlineMatchDataReaderListener();
    // 8. DataReader1
    DataReader* reader1 = subscriber->create_datareader(
        topic1, qos1, listener1, STATUS_MASK_ALL);
    ASSERT_NE(reader1, nullptr);

    // 9. 创建DataReaderQos对象2，Deadline.period=20ms
    DataReaderQos qos2 = DATAREADER_QOS_DEFAULT;
    qos2.deadline.period.sec = 0;
    qos2.deadline.period.nanosec = 20 * 1000 * 1000; // 20ms

    // 10. 创建Listener2
    auto listener2 = new DeadlineMatchDataReaderListener();
    // 11. DataReader2
    DataReader* reader2 = subscriber->create_datareader(
        topic2, qos2, listener2, STATUS_MASK_ALL);
    ASSERT_NE(reader2, nullptr);

    // 12. 发送DataReader创建完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("DeadlineMatch", "reader_created_finish"));

    // 13. DataReader1和DataWriter1匹配（period <= period）
    EXPECT_EQ(listener1->wait_subscription_matched(5000), 0); // 成功
    EXPECT_EQ(listener1->wait_requested_incompatible_qos(1000), 1); // 超时
    SubscriptionMatchedStatus sub_status1;
    EXPECT_EQ(reader1->get_subscription_matched_status(sub_status1), RETCODE_OK);
    EXPECT_EQ(sub_status1.total_count, 1);
    EXPECT_EQ(sub_status1.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status1;
    EXPECT_EQ(reader1->get_requested_incompatible_qos_status(rqos_status1), RETCODE_OK);
    EXPECT_EQ(rqos_status1.total_count, 0);
    EXPECT_EQ(rqos_status1.total_count_change, 0);

    // 14. DataReader1和DataWriter2不匹配（period > period）
    EXPECT_EQ(listener2->wait_subscription_matched(1000), 1); // 超时
    EXPECT_EQ(listener2->wait_requested_incompatible_qos(5000), 0); // 成功
    SubscriptionMatchedStatus sub_status2;
    EXPECT_EQ(reader2->get_subscription_matched_status(sub_status2), RETCODE_OK);
    EXPECT_EQ(sub_status2.total_count, 0);
    EXPECT_EQ(sub_status2.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status2;
    EXPECT_EQ(reader2->get_requested_incompatible_qos_status(rqos_status2), RETCODE_OK);
    EXPECT_EQ(rqos_status2.total_count, 1);
    EXPECT_EQ(rqos_status2.total_count_change, 0);

    // 15. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 16. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 17. finalize_instance
    factory->finalize();

    delete listener1;
    delete listener2;
    DESTORY_CONNECTION("DeadlineMatch");
}

TEST_F(QoSCompatibilityTestNegative, ENABLED_OwnershipMatchNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("OwnershipMatch");

    // 2. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 3. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建2个主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic1 = participant->create_topic(
        "ownership_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    Topic* topic2 = participant->create_topic(
        "ownership_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic2, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 创建DataReaderQos对象1，Ownership=SHARED
    DataReaderQos qos_shared = DATAREADER_QOS_DEFAULT;
    qos_shared.ownership.kind = SHARED_OWNERSHIP_QOS;

    // 7. 创建Listener1
    auto listener1 = new OwnershipMatchDataReaderListener();
    // 8. DataReader1
    DataReader* reader1 = subscriber->create_datareader(
        topic1, qos_shared, listener1, STATUS_MASK_ALL);
    ASSERT_NE(reader1, nullptr);

    // 9. 创建DataReaderQos对象2，Ownership=EXCLUSIVE，此时对方为SHARED
    DataReaderQos qos_exclusive = DATAREADER_QOS_DEFAULT;
    qos_exclusive.ownership.kind = EXCLUSIVE_OWNERSHIP_QOS;

    // 10. 创建Listener2
    auto listener2 = new OwnershipMatchDataReaderListener();
    // 11. DataReader2
    DataReader* reader2 = subscriber->create_datareader(
        topic2, qos_exclusive, listener2, STATUS_MASK_ALL);
    ASSERT_NE(reader2, nullptr);

    // 12. 发送DataReader创建完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("OwnershipMatch", "reader_created_finish"));

    // 13. DataReader1和DataWriter1匹配（kind == kind）
    EXPECT_EQ(listener1->wait_subscription_matched(5000), 0); // 成功
    EXPECT_EQ(listener1->wait_requested_incompatible_qos(1000), 1); // 超时
    SubscriptionMatchedStatus sub_status1;
    EXPECT_EQ(reader1->get_subscription_matched_status(sub_status1), RETCODE_OK);
    EXPECT_EQ(sub_status1.total_count, 1);
    EXPECT_EQ(sub_status1.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status1;
    EXPECT_EQ(reader1->get_requested_incompatible_qos_status(rqos_status1), RETCODE_OK);
    EXPECT_EQ(rqos_status1.total_count, 0);
    EXPECT_EQ(rqos_status1.total_count_change, 0);

    // 14. DataReader1和DataWriter2不匹配（kind != kind）
    EXPECT_EQ(listener2->wait_subscription_matched(1000), 1); // 超时
    EXPECT_EQ(listener2->wait_requested_incompatible_qos(5000), 0); // 成功
    SubscriptionMatchedStatus sub_status2;
    EXPECT_EQ(reader2->get_subscription_matched_status(sub_status2), RETCODE_OK);
    EXPECT_EQ(sub_status2.total_count, 0);
    EXPECT_EQ(sub_status2.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status2;
    EXPECT_EQ(reader2->get_requested_incompatible_qos_status(rqos_status2), RETCODE_OK);
    EXPECT_EQ(rqos_status2.total_count, 1);
    EXPECT_EQ(rqos_status2.total_count_change, 0);

    // 15. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 16. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 17. finalize_instance
    factory->finalize();

    delete listener1;
    delete listener2;
    DESTORY_CONNECTION("OwnershipMatch");
}

TEST_F(QoSCompatibilityTestNegative, ENABLED_LivelinessMatchNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("LivelinessMatch");

    // 2. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 3. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建2个主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic1 = participant->create_topic(
        "liveliness_match_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    Topic* topic2 = participant->create_topic(
        "liveliness_match_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic2, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 创建DataReaderQos对象1，Liveliness.kind=AUTOMATIC
    DataReaderQos qos_auto = DATAREADER_QOS_DEFAULT;
    qos_auto.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;

    // 7. 创建Listener1
    auto listener1 = new LivelinessMatchDataReaderListener();
    // 8. DataReader1
    DataReader* reader1 = subscriber->create_datareader(
        topic1, qos_auto, listener1, STATUS_MASK_ALL);
    ASSERT_NE(reader1, nullptr);

    // 9. 创建DataReaderQos对象2，Liveliness.kind=MANUAL_BY_TOPIC
    DataReaderQos qos_manual = DATAREADER_QOS_DEFAULT;
    qos_manual.liveliness.kind = MANUAL_BY_TOPIC_LIVELINESS_QOS;

    // 10. 创建Listener2
    auto listener2 = new LivelinessMatchDataReaderListener();
    // 11. DataReader2
    DataReader* reader2 = subscriber->create_datareader(
        topic2, qos_manual, listener2, STATUS_MASK_ALL);
    ASSERT_NE(reader2, nullptr);

    // 12. 发送DataReader创建完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("LivelinessMatch", "reader_created_finish"));

    // 13. DataReader1和DataWriter1匹配（kind >= kind）
    EXPECT_EQ(listener1->wait_subscription_matched(5000), 0); // 成功
    EXPECT_EQ(listener1->wait_requested_incompatible_qos(1000), 1); // 超时
    SubscriptionMatchedStatus sub_status1;
    EXPECT_EQ(reader1->get_subscription_matched_status(sub_status1), RETCODE_OK);
    EXPECT_EQ(sub_status1.total_count, 1);
    EXPECT_EQ(sub_status1.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status1;
    EXPECT_EQ(reader1->get_requested_incompatible_qos_status(rqos_status1), RETCODE_OK);
    EXPECT_EQ(rqos_status1.total_count, 0);
    EXPECT_EQ(rqos_status1.total_count_change, 0);

    // 14. DataReader1和DataWriter2不匹配（kind < kind）
    EXPECT_EQ(listener2->wait_subscription_matched(1000), 1); // 超时
    EXPECT_EQ(listener2->wait_requested_incompatible_qos(5000), 0); // 成功
    SubscriptionMatchedStatus sub_status2;
    EXPECT_EQ(reader2->get_subscription_matched_status(sub_status2), RETCODE_OK);
    EXPECT_EQ(sub_status2.total_count, 0);
    EXPECT_EQ(sub_status2.total_count_change, 0);
    RequestedIncompatibleQosStatus rqos_status2;
    EXPECT_EQ(reader2->get_requested_incompatible_qos_status(rqos_status2), RETCODE_OK);
    EXPECT_EQ(rqos_status2.total_count, 1);
    EXPECT_EQ(rqos_status2.total_count_change, 0);

    // 15. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 16. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 17. finalize_instance
    factory->finalize();

    delete listener1;
    delete listener2;
    DESTORY_CONNECTION("LivelinessMatch");
}
