#include "QoSCompatibilityTestNegative.h"
#include "PartitionQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "testdataTypeSupport.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
//#include "types/TestModule::BasicDataTypeSupport.h"

void PartitionQosPolicyTestNegative::SetUp() {
}

void PartitionQosPolicyTestNegative::TearDown() {
}

class PartitionQosPolicyDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};

TEST_F(PartitionQosPolicyTestNegative, ENABLED_PartitionQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("PartitionQosPolicy"); 

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
        "partition_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    Topic* topic2 = participant->create_topic(
        "partition_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic2, nullptr);

    // 5. 创建SubscriberQos对象1，Partition=partition_topic_name_1
    SubscriberQos sub_qos1 = SUBSCRIBER_QOS_DEFAULT;
    sub_qos1.partition.name = {"partition_topic_name_1"};

    // 6. 创建SubscriberQos对象2，Partition=not_in_one_partition
    SubscriberQos sub_qos2 = SUBSCRIBER_QOS_DEFAULT;
    sub_qos2.partition.name = {"not_in_one_partition"};

    // 7. 以对象1创建Subscriber1
    Subscriber* subscriber1 = participant->create_subscriber(sub_qos1, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber1, nullptr);

    // 8. 以对象2创建Subscriber2
    Subscriber* subscriber2 = participant->create_subscriber(sub_qos2, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber2, nullptr);

    // 9. 使用Subscriber1创建DataReader1
    DataReader* reader1 = subscriber1->create_datareader(
        topic1, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader1, nullptr);

    // 10. 使用Subscriber2创建DataReader2
    DataReader* reader2 = subscriber2->create_datareader(
        topic2, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader2, nullptr);
    EXPECT_TRUE(TestFrame::getInstance().waitForData("PartitionQosPolicy", "create_finish", 10000));

    // 11. 相同Partition匹配
    SubscriptionMatchedStatus status1;
    EXPECT_EQ(reader1->get_subscription_matched_status(status1), RETCODE_OK);
    EXPECT_EQ(status1.total_count, 1);
    EXPECT_EQ(status1.total_count_change, 1);

    // 12. 不同Partition不匹配
    SubscriptionMatchedStatus status2;
    EXPECT_EQ(reader2->get_subscription_matched_status(status2), RETCODE_OK);
    EXPECT_EQ(status2.total_count, 0);
    EXPECT_EQ(status2.total_count_change, 0);

    // 13. 发送检查完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("PartitionQosPolicy","check_finish"));

    // 14. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 15. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 16. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("PartitionQosPolicy");
}
