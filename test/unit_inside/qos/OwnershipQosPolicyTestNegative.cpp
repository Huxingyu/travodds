#include "QoSCompatibilityTestNegative.h"
#include "OwnershipQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "testdataTypeSupport.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
//#include "types/TestModule::BasicDataTypeSupport.h"

void OwnershipQosPolicyTestNegative::SetUp() {
}

void OwnershipQosPolicyTestNegative::TearDown() {
}

class OwnershipQosPolicyDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};

TEST_F(OwnershipQosPolicyTestNegative, ENABLED_OwnershipQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("OwnershipQosPolicy");

    // 2. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 3. 默认参数创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建2个主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic1 = participant->create_topic(
        "ownership_topic_name_1", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    Topic* topic2 = participant->create_topic(
        "ownership_topic_name_2", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic2, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 创建自定义DataReaderQos对象，Ownership=SHARED
    DataReaderQos qos_shared = DATAREADER_QOS_DEFAULT;
    qos_shared.ownership.kind = SHARED_OWNERSHIP_QOS;
    qos_shared.history.kind = KEEP_ALL_HISTORY_QOS;

    // 7. 创建Listener1
    auto listener1 = new OwnershipQosPolicyDataReaderListener();
    // 8. DataReader1
    DataReader* reader1 = subscriber->create_datareader(
        topic1, qos_shared, listener1, STATUS_MASK_ALL);
    ASSERT_NE(reader1, nullptr);

    // 9. 创建自定义DataReaderQos对象，Ownership=EXCLUSIVE
    DataReaderQos qos_exclusive = DATAREADER_QOS_DEFAULT;
    qos_exclusive.ownership.kind = EXCLUSIVE_OWNERSHIP_QOS;
    qos_exclusive.history.kind = KEEP_ALL_HISTORY_QOS;

    // 10. 创建Listener2
    auto listener2 = new OwnershipQosPolicyDataReaderListener();
    // 11. DataReader2
    DataReader* reader2 = subscriber->create_datareader(
        topic2, qos_exclusive, listener2, STATUS_MASK_ALL);
    ASSERT_NE(reader2, nullptr);

    // 12. 等待DataReader1匹配2个对端
    EXPECT_TRUE(TestHelper::waitmatch(reader1, 2, 5));
    // 13. 等待DataReader2匹配2个对端
    EXPECT_TRUE(TestHelper::waitmatch(reader2, 2, 5));

    // 14. 发送匹配完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("OwnershipQosPolicy","reader_match_finish"));

    // 15. SHARED类型所有匹配的对端的数据均能接收到
    EXPECT_TRUE(TestFrame::getInstance().waitForData("OwnershipQosPolicy", "write_finish"));
    TestModule::BasicData sample;
    SampleInfo info;
    EXPECT_EQ(reader1->take_next_sample(&sample, info), RETCODE_OK);
    EXPECT_EQ(sample.id, 1);
    EXPECT_EQ(reader1->take_next_sample(&sample, info), RETCODE_OK);
    EXPECT_EQ(sample.id, 2);

    // 16. EXCLUSIVE类型只能接收到ownership_strength最大的对端的数据
    EXPECT_EQ(reader2->take_next_sample(&sample, info), RETCODE_OK);
    EXPECT_EQ(sample.id, 3);
    EXPECT_EQ(reader2->take_next_sample(&sample, info), RETCODE_NO_DATA);

    // 17. 发送检查完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("OwnershipQosPolicy","check_finish"));

    // 18. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 19. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 20. finalize_instance
    factory->finalize();

    delete listener1;
    delete listener2;
    DESTORY_CONNECTION("OwnershipQosPolicy");
}