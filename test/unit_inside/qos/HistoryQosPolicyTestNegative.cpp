#include "QoSCompatibilityTestNegative.h"
#include "HistoryQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "testdataTypeSupport.h"

void HistoryQosPolicyTestNegative::SetUp() {
}

void HistoryQosPolicyTestNegative::TearDown() {
}

class HistoryQosPolicyDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};

TEST_F(HistoryQosPolicyTestNegative, ENABLED_HistoryQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("HistoryQosPolicy");

    // 2. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 3. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建history_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "history_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 创建自定义DataReaderQos对象1，History=KEEP_LAST, depth=5
    DataReaderQos dr_qos1 = DATAREADER_QOS_DEFAULT;
    dr_qos1.history.kind = KEEP_LAST_HISTORY_QOS;
    dr_qos1.history.depth = 5;

    // 7. 创建Listener1
    auto listener1 = new HistoryQosPolicyDataReaderListener();
    // 8. 创建自定义DataReaderQos对象2，History=KEEP_ALL
    DataReaderQos dr_qos2 = DATAREADER_QOS_DEFAULT;
    dr_qos2.history.kind = KEEP_ALL_HISTORY_QOS;

    // 9. DataReader1
    DataReader* reader1 = subscriber->create_datareader(
        topic, dr_qos1, listener1, STATUS_MASK_ALL);
    ASSERT_NE(reader1, nullptr);

    // 10. DataReader2
    DataReader* reader2 = subscriber->create_datareader(
        topic, dr_qos2, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader2, nullptr);

    // 11. 等待DataReader1匹配完成
    EXPECT_TRUE(TestHelper::waitmatch(reader1, 1, 5));
    // 12. 等待DataReader2匹配完成
    EXPECT_TRUE(TestHelper::waitmatch(reader2, 1, 5));

    //这边应该是少了发送通知的步骤
    EXPECT_TRUE(TestFrame::getInstance().sendData("HistoryQosPolicy", "reader_match_finish"));

    // 13. 等待主动端写入完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("HistoryQosPolicy", "write_finish", dummy, 10000));

    //该情况不会触发on_sample_lost接口
    // 14. KEEP_LAST类型DataReader仅接收部分数据并触发on_sample_lost
    SequenceData<TestModule::BasicData> data1;
    SampleInfoSeq infos1;
    EXPECT_EQ(reader1->take(data1, infos1, LENGTH_UNLIMITED,
        ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    EXPECT_EQ(data1.length(), 5u);
    EXPECT_EQ(listener1->wait_sample_lost(1000), 1); 
    SampleLostStatus lost_status;
    EXPECT_EQ(reader1->get_sample_lost_status(lost_status), RETCODE_OK);
    EXPECT_EQ(lost_status.total_count, 0);
    EXPECT_EQ(lost_status.total_count_change, 0);

    // 15. KEEP_ALL类型DataReader接收所有数据
    SequenceData<TestModule::BasicData> data2;
    SampleInfoSeq infos2;
    EXPECT_EQ(reader2->take(data2, infos2, LENGTH_UNLIMITED,
        ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    EXPECT_EQ(data2.length(), 10u);

    // 16. 发送DataReader接收完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("HistoryQosPolicy","reader_take_finish"));

    // 17. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 18. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 19. finalize_instance
    factory->finalize();

    delete listener1;
    DESTORY_CONNECTION("HistoryQosPolicy");
}
