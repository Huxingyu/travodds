#include "PartitionQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

void PartitionQosPolicyTestPositive::SetUp() {
    std::cout << "PartitionQosPolicyTestPositive::SetUp" << std::endl;
}

void PartitionQosPolicyTestPositive::TearDown() {
    std::cout << "PartitionQosPolicyTestPositive::TearDown" << std::endl;
}

TEST_F(PartitionQosPolicyTestPositive, ENABLED_PartitionQosPolicyPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("PartitionQosPolicyTestNegative.ENABLED_PartitionQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("PartitionQosPolicy"); // 创建连接

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建2个主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic1 = participant->create_topic(
        "partition_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    Topic* topic2 = participant->create_topic(
        "partition_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic2, nullptr);

    // 4. 创建PublisherQos对象1，Partition=partition_topic_name_1
    PublisherQos pub_qos1 = PUBLISHER_QOS_DEFAULT;
    pub_qos1.partition.name = {"partition_topic_name_1"};

    // 5. 创建PublisherQos对象2，Partition=partition_topic_name_2
    PublisherQos pub_qos2 = PUBLISHER_QOS_DEFAULT;
    pub_qos2.partition.name = {"partition_topic_name_2"};

    // 6. 以对象1创建Publisher1
    Publisher* publisher1 = participant->create_publisher(pub_qos1, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher1, nullptr);

    // 7. 以对象2创建Publisher2
    Publisher* publisher2 = participant->create_publisher(pub_qos2, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher2, nullptr);

    // 8. 使用Publisher1创建DataWriter1
    DataWriter* writer1 = publisher1->create_datawriter(
        topic1, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer1, nullptr);

    // 9. 使用Publisher2创建DataWriter2
    DataWriter* writer2 = publisher2->create_datawriter(
        topic2, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer2, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_TRUE(TestFrame::getInstance().sendData("PartitionQosPolicy", "create_finish"));

    // 10. 等待被动端检查完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("PartitionQosPolicy", "check_finish", dummy, 10000));

    // 11. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 12. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 13. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("PartitionQosPolicy");
}