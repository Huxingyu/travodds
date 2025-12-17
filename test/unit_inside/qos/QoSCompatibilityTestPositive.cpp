#include "QoSCompatibilityTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/publisher.h"
#include "testdataTypeSupport.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include <condition_variable>
#include <mutex>
#include <chrono>

USING_TRAVODDS_NAMESPACE


void QoSCompatibilityTestPositive::SetUp() {
}

void QoSCompatibilityTestPositive::TearDown() {
}

TEST_F(QoSCompatibilityTestPositive, ENABLED_DurabilityMatchPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("QoSCompatibilityTestNegative.ENABLED_DurabilityMatchNegative");
    CREATE_AND_SYNC_CONNECTION("DurabilityMatch"); // 创建连接

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

    // 5. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 6. 创建DataWriterQos对象1，Durability=VOLATILE
    DataWriterQos qos_volatile = DATAWRITER_QOS_DEFAULT;
    qos_volatile.durability.kind = VOLATILE_DURABILITY_QOS;

    // 7. 创建Listener1
    auto listener1 = new DurabilityMatchDataWriterListener();
    // 8. DataWriter1
    DataWriter* writer1 = publisher->create_datawriter(
        topic1, qos_volatile, listener1, STATUS_MASK_ALL);
    ASSERT_NE(writer1, nullptr);

    // 9. 创建Listener2
    auto listener2 = new DurabilityMatchDataWriterListener();
    // 10. DataWriter2
    DataWriter* writer2 = publisher->create_datawriter(
        topic2, qos_volatile, listener2, STATUS_MASK_ALL);
    ASSERT_NE(writer2, nullptr);

    // 11. DataWriterQos对象2，Durability=TRANSIENT_LOCAL
    DataWriterQos qos_transient = DATAWRITER_QOS_DEFAULT;
    qos_transient.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    // 12. 创建Listener3
    auto listener3 = new DurabilityMatchDataWriterListener();
    // 13. DataWriter3
    DataWriter* writer3 = publisher->create_datawriter(
        topic3, qos_transient, listener3, STATUS_MASK_ALL);
    ASSERT_NE(writer3, nullptr);

    // 14. 创建Listener4
    auto listener4 = new DurabilityMatchDataWriterListener();
    // 15. DataWriter4
    DataWriter* writer4 = publisher->create_datawriter(
        topic4, qos_transient, listener4, STATUS_MASK_ALL);
    ASSERT_NE(writer4, nullptr);

    // 16. 等待被动端创建DataReader
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("DurabilityMatch", "reader_created_finish", dummy, 10000));

    // 17. VOLATILE Writer & VOLATILE Reader 匹配
    EXPECT_EQ(listener1->wait_offered_incompatible_qos(1000), 1); // 超时
    EXPECT_EQ(listener1->wait_publication_matched(5000), 0); // 成功
    PublicationMatchedStatus pub_status1;
    EXPECT_EQ(writer1->get_publication_matched_status(pub_status1), RETCODE_OK);
    EXPECT_EQ(pub_status1.total_count, 1);
    //listener 中读取后会被重置
    EXPECT_EQ(pub_status1.total_count_change, 0);
    //EXPECT_EQ(pub_status1.total_count_change, 1);
    OfferedIncompatibleQosStatus qos_status1;
    EXPECT_EQ(writer1->get_offered_incompatible_qos_status(qos_status1), RETCODE_OK);
    EXPECT_EQ(qos_status1.total_count, 0);
    EXPECT_EQ(qos_status1.total_count_change, 0);

    // 18. VOLATILE Writer & TRANSIENT_LOCAL Reader 不匹配
    EXPECT_EQ(listener2->wait_offered_incompatible_qos(5000), 0); // 成功
    EXPECT_EQ(listener2->wait_publication_matched(1000), 1); // 超时
    PublicationMatchedStatus pub_status2;
    EXPECT_EQ(writer2->get_publication_matched_status(pub_status2), RETCODE_OK);
    EXPECT_EQ(pub_status2.total_count, 0);
    EXPECT_EQ(pub_status2.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status2;
    EXPECT_EQ(writer2->get_offered_incompatible_qos_status(qos_status2), RETCODE_OK);
    EXPECT_EQ(qos_status2.total_count, 1);
    //listener 中读取后会被重置
    EXPECT_EQ(qos_status2.total_count_change, 0);
    //EXPECT_EQ(qos_status2.total_count_change, 1);
    EXPECT_EQ(qos_status2.last_policy_id, DURABILITY_QOS_POLICY_ID);

    // 19. TRANSIENT_LOCAL Writer & VOLATILE Reader 匹配
    EXPECT_EQ(listener3->wait_offered_incompatible_qos(5000), 1); // 成功
    EXPECT_EQ(listener3->wait_publication_matched(1000), 0); // 超时
    PublicationMatchedStatus pub_status3;
    EXPECT_EQ(writer3->get_publication_matched_status(pub_status3), RETCODE_OK);
    EXPECT_EQ(pub_status3.total_count, 1);
    EXPECT_EQ(pub_status3.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status3;
    EXPECT_EQ(writer3->get_offered_incompatible_qos_status(qos_status3), RETCODE_OK);
    EXPECT_EQ(qos_status3.total_count,0 );
    EXPECT_EQ(qos_status3.total_count_change, 0);
    //EXPECT_EQ(qos_status3.last_policy_id, DURABILITY_QOS_POLICY_ID);

    // 20. TRANSIENT_LOCAL Writer & TRANSIENT_LOCAL Reader 匹配
    EXPECT_EQ(listener4->wait_offered_incompatible_qos(1000), 1); // 超时
    EXPECT_EQ(listener4->wait_publication_matched(5000), 0); // 成功
    PublicationMatchedStatus pub_status4;
    EXPECT_EQ(writer4->get_publication_matched_status(pub_status4), RETCODE_OK);
    EXPECT_EQ(pub_status4.total_count, 1);
    EXPECT_EQ(pub_status4.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status4;
    EXPECT_EQ(writer4->get_offered_incompatible_qos_status(qos_status4), RETCODE_OK);
    EXPECT_EQ(qos_status4.total_count, 0);
    EXPECT_EQ(qos_status4.total_count_change, 0);

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

TEST_F(QoSCompatibilityTestPositive, ENABLED_ReliabilityMatchPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("QoSCompatibilityTestNegative.ENABLED_ReliabilityMatchNegative");
    CREATE_AND_SYNC_CONNECTION("ReliabilityMatch"); // 创建连接

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

    // 5. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 6. 创建DataWriterQos对象1，Reliability=BEST_EFFORT
    DataWriterQos qos_best_effort = DATAWRITER_QOS_DEFAULT;
    qos_best_effort.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;

    // 7. 创建Listener1
    auto listener1 = new ReliabilityMatchDataWriterListener();
    // 8. DataWriter1
    DataWriter* writer1 = publisher->create_datawriter(
        topic1, qos_best_effort, listener1, STATUS_MASK_ALL);
    ASSERT_NE(writer1, nullptr);

    // 9. 创建Listener2
    auto listener2 = new ReliabilityMatchDataWriterListener();
    // 10. DataWriter2
    DataWriter* writer2 = publisher->create_datawriter(
        topic2, qos_best_effort, listener2, STATUS_MASK_ALL);
    ASSERT_NE(writer2, nullptr);

    // 11. DataWriterQos对象2，Reliability=RELIABLE
    DataWriterQos qos_reliable = DATAWRITER_QOS_DEFAULT;
    qos_reliable.reliability.kind = RELIABLE_RELIABILITY_QOS;

    // 12. 创建Listener3
    auto listener3 = new ReliabilityMatchDataWriterListener();
    // 13. DataWriter3
    DataWriter* writer3 = publisher->create_datawriter(
        topic3, qos_reliable, listener3, STATUS_MASK_ALL);
    ASSERT_NE(writer3, nullptr);

    // 14. 创建Listener4
    auto listener4 = new ReliabilityMatchDataWriterListener();
    // 15. DataWriter4
    DataWriter* writer4 = publisher->create_datawriter(
        topic4, qos_reliable, listener4, STATUS_MASK_ALL);
    ASSERT_NE(writer4, nullptr);

    // 16. 等待被动端创建DataReader
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("ReliabilityMatch", "reader_created_finish", dummy, 10000));

    // 17. BEST_EFFORT Writer & BEST_EFFORT Reader 匹配
    EXPECT_EQ(listener1->wait_offered_incompatible_qos(1000), 1); // 超时
    EXPECT_EQ(listener1->wait_publication_matched(5000), 0); // 成功
    PublicationMatchedStatus pub_status1;
    EXPECT_EQ(writer1->get_publication_matched_status(pub_status1), RETCODE_OK);
    EXPECT_EQ(pub_status1.total_count, 1);
    EXPECT_EQ(pub_status1.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status1;
    EXPECT_EQ(writer1->get_offered_incompatible_qos_status(qos_status1), RETCODE_OK);
    EXPECT_EQ(qos_status1.total_count, 0);
    EXPECT_EQ(qos_status1.total_count_change, 0);

    // 18. BEST_EFFORT Writer & RELIABLE Reader 不匹配
    EXPECT_EQ(listener2->wait_offered_incompatible_qos(5000), 0); // 成功
    EXPECT_EQ(listener2->wait_publication_matched(1000), 1); // 超时
    PublicationMatchedStatus pub_status2;
    EXPECT_EQ(writer2->get_publication_matched_status(pub_status2), RETCODE_OK);
    EXPECT_EQ(pub_status2.total_count, 0);
    EXPECT_EQ(pub_status2.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status2;
    EXPECT_EQ(writer2->get_offered_incompatible_qos_status(qos_status2), RETCODE_OK);
    EXPECT_EQ(qos_status2.total_count, 1);
    EXPECT_EQ(qos_status2.total_count_change, 0);
    EXPECT_EQ(qos_status2.last_policy_id, RELIABILITY_QOS_POLICY_ID);

    // 19. RELIABLE Writer & BEST_EFFORT Reader 匹配
    EXPECT_EQ(listener3->wait_offered_incompatible_qos(5000), 1); // 成功
    EXPECT_EQ(listener3->wait_publication_matched(1000), 0); // 超时
    PublicationMatchedStatus pub_status3;
    EXPECT_EQ(writer3->get_publication_matched_status(pub_status3), RETCODE_OK);
    EXPECT_EQ(pub_status3.total_count, 1);
    EXPECT_EQ(pub_status3.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status3;
    EXPECT_EQ(writer3->get_offered_incompatible_qos_status(qos_status3), RETCODE_OK);
    EXPECT_EQ(qos_status3.total_count, 0);
    EXPECT_EQ(qos_status3.total_count_change, 0);
    //EXPECT_EQ(qos_status3.last_policy_id, RELIABILITY_QOS_POLICY_ID);

    // 20. RELIABLE Writer & RELIABLE Reader 匹配
    EXPECT_EQ(listener4->wait_offered_incompatible_qos(1000), 1); // 超时
    EXPECT_EQ(listener4->wait_publication_matched(5000), 0); // 成功
    PublicationMatchedStatus pub_status4;
    EXPECT_EQ(writer4->get_publication_matched_status(pub_status4), RETCODE_OK);
    EXPECT_EQ(pub_status4.total_count, 1);
    EXPECT_EQ(pub_status4.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status4;
    EXPECT_EQ(writer4->get_offered_incompatible_qos_status(qos_status4), RETCODE_OK);
    EXPECT_EQ(qos_status4.total_count, 0);
    EXPECT_EQ(qos_status4.total_count_change, 0);

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

TEST_F(QoSCompatibilityTestPositive, ENABLED_DeadlineMatchPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("QoSCompatibilityTestNegative.ENABLED_DeadlineMatchNegative");
    CREATE_AND_SYNC_CONNECTION("DeadlineMatch"); // 创建连接

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

    // 5. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 6. 创建DataWriterQos对象1，Deadline.period=100ms
    DataWriterQos qos1 = DATAWRITER_QOS_DEFAULT;
    qos1.deadline.period.sec = 0;
    qos1.deadline.period.nanosec = 100 * 1000 * 1000; // 100ms

    // 7. 创建Listener1
    auto listener1 = new DeadlineMatchDataWriterListener();
    // 8. DataWriter1
    DataWriter* writer1 = publisher->create_datawriter(
        topic1, qos1, listener1, STATUS_MASK_ALL);
    ASSERT_NE(writer1, nullptr);

    // 9. 创建DataWriterQos对象2，Deadline.period=100ms
    DataWriterQos qos2 = DATAWRITER_QOS_DEFAULT;
    qos2.deadline.period.sec = 0;
    qos2.deadline.period.nanosec = 100 * 1000 * 1000; // 100ms

    // 10. 创建Listener2
    auto listener2 = new DeadlineMatchDataWriterListener();
    // 11. DataWriter2
    DataWriter* writer2 = publisher->create_datawriter(
        topic2, qos2, listener2, STATUS_MASK_ALL);
    ASSERT_NE(writer2, nullptr);

    // 12. 等待被动端创建DataReader
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("DeadlineMatch", "reader_created_finish", dummy, 10000));

    // 13. DataWriter1和DataReader1匹配（period <= period）
    EXPECT_EQ(listener1->wait_offered_incompatible_qos(1000), 1); // 超时
    EXPECT_EQ(listener1->wait_publication_matched(5000), 0); // 成功
    PublicationMatchedStatus pub_status1;
    EXPECT_EQ(writer1->get_publication_matched_status(pub_status1), RETCODE_OK);
    EXPECT_EQ(pub_status1.total_count, 1);
    EXPECT_EQ(pub_status1.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status1;
    EXPECT_EQ(writer1->get_offered_incompatible_qos_status(qos_status1), RETCODE_OK);
    EXPECT_EQ(qos_status1.total_count, 0);
    EXPECT_EQ(qos_status1.total_count_change, 0);

    // 14. DataWriter2和DataReader1不匹配（period > period）
    EXPECT_EQ(listener2->wait_offered_incompatible_qos(5000), 0); // 成功
    EXPECT_EQ(listener2->wait_publication_matched(1000), 1); // 超时
    PublicationMatchedStatus pub_status2;
    EXPECT_EQ(writer2->get_publication_matched_status(pub_status2), RETCODE_OK);
    EXPECT_EQ(pub_status2.total_count, 0);
    EXPECT_EQ(pub_status2.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status2;
    EXPECT_EQ(writer2->get_offered_incompatible_qos_status(qos_status2), RETCODE_OK);
    EXPECT_EQ(qos_status2.total_count, 1);
    EXPECT_EQ(qos_status2.total_count_change, 0);

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

TEST_F(QoSCompatibilityTestPositive, ENABLED_OwnershipMatchPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("QoSCompatibilityTestNegative.ENABLED_OwnershipMatchNegative");
    CREATE_AND_SYNC_CONNECTION("OwnershipMatch"); // 创建连接

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

    // 5. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);


    // 6. 创建DataWriterQos对象1，Ownership=SHARED
    DataWriterQos qos_shared1 = DATAWRITER_QOS_DEFAULT;
    qos_shared1.ownership.kind = SHARED_OWNERSHIP_QOS;

    // 7. 创建Listener1
    auto listener1 = new OwnershipMatchDataWriterListener();
    // 8. DataWriter1
    DataWriter* writer1 = publisher->create_datawriter(
        topic1, qos_shared1, listener1, STATUS_MASK_ALL);
    ASSERT_NE(writer1, nullptr);

    // 9. 创建DataWriterQos对象2，Ownership=SHARED,此时对方为EXCLUSIVE
    DataWriterQos qos_shared2 = DATAWRITER_QOS_DEFAULT;
    qos_shared2.ownership.kind = SHARED_OWNERSHIP_QOS;

    // 10. 创建Listener2
    auto listener2 = new OwnershipMatchDataWriterListener();
    // 11. DataWriter2
    DataWriter* writer2 = publisher->create_datawriter(
        topic2, qos_shared2, listener2, STATUS_MASK_ALL);
    ASSERT_NE(writer2, nullptr);

    // 12. 等待被动端创建DataReader
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("OwnershipMatch", "reader_created_finish", dummy, 10000));

    // 13. DataWriter1和DataReader1匹配（kind == kind）
    EXPECT_EQ(listener1->wait_offered_incompatible_qos(1000), 1); // 超时
    EXPECT_EQ(listener1->wait_publication_matched(5000), 0); // 成功
    PublicationMatchedStatus pub_status1;
    EXPECT_EQ(writer1->get_publication_matched_status(pub_status1), RETCODE_OK);
    EXPECT_EQ(pub_status1.total_count, 1);
    EXPECT_EQ(pub_status1.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status1;
    EXPECT_EQ(writer1->get_offered_incompatible_qos_status(qos_status1), RETCODE_OK);
    EXPECT_EQ(qos_status1.total_count, 0);
    EXPECT_EQ(qos_status1.total_count_change, 0);

    // 14. DataWriter2和DataReader1不匹配（kind != kind）
    EXPECT_EQ(listener2->wait_offered_incompatible_qos(5000), 0); // 成功
    EXPECT_EQ(listener2->wait_publication_matched(1000), 1); // 超时
    PublicationMatchedStatus pub_status2;
    EXPECT_EQ(writer2->get_publication_matched_status(pub_status2), RETCODE_OK);
    EXPECT_EQ(pub_status2.total_count, 0);
    EXPECT_EQ(pub_status2.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status2;
    EXPECT_EQ(writer2->get_offered_incompatible_qos_status(qos_status2), RETCODE_OK);
    EXPECT_EQ(qos_status2.total_count, 1);
    EXPECT_EQ(qos_status2.total_count_change, 0);

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

TEST_F(QoSCompatibilityTestPositive, ENABLED_LivelinessMatchPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("QoSCompatibilityTestNegative.ENABLED_LivelinessMatchNegative");
    CREATE_AND_SYNC_CONNECTION("LivelinessMatch"); // 创建连接

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

    // 5. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 6. 创建DataWriterQos对象1，Liveliness.kind=MANUAL_BY_PARTICIPANT
    DataWriterQos qos_manual = DATAWRITER_QOS_DEFAULT;
    qos_manual.liveliness.kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;

    // 7. 创建Listener1
    auto listener1 = new LivelinessMatchDataWriterListener();
    // 8. DataWriter1
    DataWriter* writer1 = publisher->create_datawriter(
        topic1, qos_manual, listener1, STATUS_MASK_ALL);
    ASSERT_NE(writer1, nullptr);

    // 9. 创建DataWriterQos对象2，Liveliness.kind=AUTOMATIC
    DataWriterQos qos_auto = DATAWRITER_QOS_DEFAULT;
    qos_auto.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;

    // 10. 创建Listener2
    auto listener2 = new LivelinessMatchDataWriterListener();
    // 11. DataWriter2
    DataWriter* writer2 = publisher->create_datawriter(
        topic2, qos_auto, listener2, STATUS_MASK_ALL);
    ASSERT_NE(writer2, nullptr);

    // 12. 等待被动端创建DataReader
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("LivelinessMatch", "reader_created_finish", dummy, 10000));

    // 13. DataWriter1和DataReader1匹配（kind >= kind）
    EXPECT_EQ(listener1->wait_offered_incompatible_qos(1000), 1); // 超时
    EXPECT_EQ(listener1->wait_publication_matched(5000), 0); // 成功
    PublicationMatchedStatus pub_status1;
    EXPECT_EQ(writer1->get_publication_matched_status(pub_status1), RETCODE_OK);
    EXPECT_EQ(pub_status1.total_count, 1);
    EXPECT_EQ(pub_status1.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status1;
    EXPECT_EQ(writer1->get_offered_incompatible_qos_status(qos_status1), RETCODE_OK);
    EXPECT_EQ(qos_status1.total_count, 0);
    EXPECT_EQ(qos_status1.total_count_change, 0);

    // 14. DataWriter2和DataReader1不匹配（kind < kind）
    EXPECT_EQ(listener2->wait_offered_incompatible_qos(5000), 0); // 成功
    EXPECT_EQ(listener2->wait_publication_matched(1000), 1); // 超时
    PublicationMatchedStatus pub_status2;
    EXPECT_EQ(writer2->get_publication_matched_status(pub_status2), RETCODE_OK);
    EXPECT_EQ(pub_status2.total_count, 0);
    EXPECT_EQ(pub_status2.total_count_change, 0);
    OfferedIncompatibleQosStatus qos_status2;
    EXPECT_EQ(writer2->get_offered_incompatible_qos_status(qos_status2), RETCODE_OK);
    EXPECT_EQ(qos_status2.total_count, 1);
    EXPECT_EQ(qos_status2.total_count_change, 0);

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