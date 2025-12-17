#include "OwnershipQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "testdataTypeSupport.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

void OwnershipQosPolicyTestPositive::SetUp() {
    std::cout << "OwnershipQosPolicyTestPositive::SetUp" << std::endl;
}

void OwnershipQosPolicyTestPositive::TearDown() {
    std::cout << "OwnershipQosPolicyTestPositive::TearDown" << std::endl;
}

TEST_F(OwnershipQosPolicyTestPositive, ENABLED_OwnershipQosPolicyPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("OwnershipQosPolicyTestNegative.ENABLED_OwnershipQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("OwnershipQosPolicy"); // 创建连接

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 默认参数创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建2个主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic1 = participant->create_topic(
        "ownership_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    Topic* topic2 = participant->create_topic(
        "ownership_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic2, nullptr);

    // 4. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 5. 创建自定义DataWriterQos对象1，Ownership=SHARED
    DataWriterQos qos_shared = DATAWRITER_QOS_DEFAULT;
    qos_shared.ownership.kind = SHARED_OWNERSHIP_QOS;

    // 6. DataWriter1
    DataWriter* writer1 = publisher->create_datawriter(
        topic1, qos_shared, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer1, nullptr);

    // 7. DataWriter2
    DataWriter* writer2 = publisher->create_datawriter(
        topic1, qos_shared, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer2, nullptr);

    // 8. 创建自定义DataWriterQos对象2，Ownership=EXCLUSIVE，ownership_strength=100
    DataWriterQos qos_exclusive_100 = DATAWRITER_QOS_DEFAULT;
    qos_exclusive_100.ownership.kind = EXCLUSIVE_OWNERSHIP_QOS;
    qos_exclusive_100.ownership_strength.value = 100;

    // 9. DataWriter3
    DataWriter* writer3 = publisher->create_datawriter(
        topic2, qos_exclusive_100, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer3, nullptr);

    // 10. 创建自定义DataWriterQos对象3，Ownership=EXCLUSIVE，ownership_strength=50
    DataWriterQos qos_exclusive_50 = DATAWRITER_QOS_DEFAULT;
    qos_exclusive_50.ownership.kind = EXCLUSIVE_OWNERSHIP_QOS;
    qos_exclusive_50.ownership_strength.value = 50;

    // 11. DataWriter4
    DataWriter* writer4 = publisher->create_datawriter(
        topic2, qos_exclusive_50, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer4, nullptr);

    // 12. 等待被动端匹配完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("OwnershipQosPolicy", "reader_match_finish", dummy, 10000));

    // 13. DataWriter1发送lVal=1
    TestModule::BasicData sample;
    sample.id = 1;
    EXPECT_EQ(writer1->write(&sample, HANDLE_NIL), RETCODE_OK);

    // 14. DataWriter2发送lVal=2
    sample.id = 2;
    EXPECT_EQ(writer2->write(&sample, HANDLE_NIL), RETCODE_OK);

    // 15. DataWriter3发送lVal=3
    sample.id = 3;
    EXPECT_EQ(writer3->write(&sample, HANDLE_NIL), RETCODE_OK);

    // 16. DataWriter4发送lVal=4
    sample.id = 4;
    EXPECT_EQ(writer4->write(&sample, HANDLE_NIL), RETCODE_OK);

    // 17. 等待被动端检查完成
    EXPECT_TRUE(TestFrame::getInstance().sendData("OwnershipQosPolicy", "write_finish"));
    EXPECT_TRUE(TestFrame::getInstance().waitForData("OwnershipQosPolicy", "check_finish", dummy, 10000));

    // 18. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 19. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 20. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("OwnershipQosPolicy");
}