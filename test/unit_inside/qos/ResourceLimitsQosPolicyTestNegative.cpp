#include "QoSCompatibilityTestNegative.h"
#include "ResourceLimitsQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "testdataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
//#include "types/TestModule::BasicDataTypeSupport.h"

void ResourceLimitsQosPolicyTestNegative::SetUp() {
}

void ResourceLimitsQosPolicyTestNegative::TearDown() {
}

class ResourceLimitsQosPolicyDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};

TEST_F(ResourceLimitsQosPolicyTestNegative, ENABLED_ResourceLimitsQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("ResourceLimitsQosPolicy");

    // 2. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 3. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建reliability_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "reliability_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 创建自定义DataReaderQos对象1
    DataReaderQos dr_qos1 = DATAREADER_QOS_DEFAULT;
    dr_qos1.reliability.kind = RELIABLE_RELIABILITY_QOS;
    dr_qos1.history.kind = KEEP_ALL_HISTORY_QOS;
    dr_qos1.resource_limits.max_samples = 10;
    dr_qos1.resource_limits.max_samples_per_instance = 10;
    dr_qos1.resource_limits.max_instances = 1;

    // 7. 创建Listener1
    auto listener1 = new ResourceLimitsQosPolicyDataReaderListener();
    // 8. DataReader1
    DataReader* reader1 = subscriber->create_datareader(
        topic, dr_qos1, listener1, STATUS_MASK_ALL);
    ASSERT_NE(reader1, nullptr);

    // 9. 等待DataReader1匹配完成
    EXPECT_TRUE(TestHelper::waitmatch(reader1, 1, 5));

    EXPECT_TRUE(TestFrame::getInstance().sendData("ResourceLimitsQosPolicy", "reader_match_finish"));

    // 10. 等待主动端DataWriter写入完成
    EXPECT_TRUE(TestFrame::getInstance().waitForData("ResourceLimitsQosPolicy", "timeout_finish", 10000));

    // 11. DataReader1 take，预期10个样本
    SequenceData<TestModule::BasicData> data;
    SampleInfoSeq infos;
    EXPECT_EQ(reader1->take(data, infos, LENGTH_UNLIMITED,
        ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    EXPECT_EQ(data.length(), 10u);

    // 12. 等待DataWriter2写入完成
    EXPECT_TRUE(TestFrame::getInstance().waitForData("ResourceLimitsQosPolicy", "write_finish2", 10000));

    // 13. 等待sample_reject回调
    EXPECT_EQ(listener1->wait_sample_rejected(1000), 0); // 成功

    // 14. 检查SampleRejectStatus
    SampleRejectedStatus reject_status;
    EXPECT_EQ(reader1->get_sample_rejected_status(reject_status), RETCODE_OK);
    EXPECT_GT(reject_status.total_count, 0);
    EXPECT_EQ(reject_status.total_count_change, 0);    //on_sample_rejected回调已先一步读取并重置了该值

    // 15. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 16. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 17. finalize_instance
    factory->finalize();

    delete listener1;
    DESTORY_CONNECTION("ResourceLimitsQosPolicy");
}
