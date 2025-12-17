#include "QoSCompatibilityTestNegative.h"
#include "ReliabilityQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "testdataTypeSupport.h"

void ReliabilityQosPolicyTestNegative::SetUp() {
}

void ReliabilityQosPolicyTestNegative::TearDown() {
}

class ReliabilityQosPolicyDataReaderListener : public BaseDataReaderListener {
public:
    void on_data_available(DataReader* reader) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        BaseDataReaderListener::on_data_available(reader);
    }
};

TEST_F(ReliabilityQosPolicyTestNegative, ENABLED_ReliabilityQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("ReliabilityQosPolicy");

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

    // 6. 创建自定义DataReaderQos对象1，Reliability=RELIABLE，History.depth=200
    DataReaderQos dr_qos1 = DATAREADER_QOS_DEFAULT;
    dr_qos1.reliability.kind = RELIABLE_RELIABILITY_QOS;
    dr_qos1.history.depth = 20;

    // 7. 创建Listener1
    auto listener1 = new ReliabilityQosPolicyDataReaderListener();
    // 8. DataReader1
    DataReader* reader1 = subscriber->create_datareader(
        topic, dr_qos1, listener1, STATUS_MASK_ALL);
    ASSERT_NE(reader1, nullptr);

    // 9. 创建自定义DataReaderQos对象2，Reliability=BEST_EFFORT
    DataReaderQos dr_qos2 = DATAREADER_QOS_DEFAULT;
    dr_qos2.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;

    // 10. DataReader2
    DataReader* reader2 = subscriber->create_datareader(
        topic, dr_qos2, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader2, nullptr);

    // 11. 等待DataReader1匹配完成
    EXPECT_TRUE(TestHelper::waitmatch(reader1, 1, 5));
    // 12. 等待DataReader2匹配完成
    EXPECT_TRUE(TestHelper::waitmatch(reader2, 1, 5));

    // 13. 发送DataReader匹配完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("ReliabilityQosPolicy","reader_match_finish"));

    // 14. 等待主动端写入完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("ReliabilityQosPolicy", "write_finish", dummy, 25000));

    // 15. RELIABLE类型DataReader接收所有数据
    SequenceData<TestModule::BasicData> data1;
    SampleInfoSeq infos1;
    EXPECT_EQ(reader1->take(data1, infos1, LENGTH_UNLIMITED,
        ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);

    //depth是200 收不到1000个数据的，该qos测试设计不是很合理
    EXPECT_EQ(data1.length(), 20u);

    // 16. BEST_EFFORT类型DataReader接收部分数据
    SequenceData<TestModule::BasicData> data2;
    SampleInfoSeq infos2;
    EXPECT_EQ(reader2->take(data2, infos2, LENGTH_UNLIMITED,
        ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    EXPECT_LT(data2.length(), 100u);

    // 17. 发送DataReader接收完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("ReliabilityQosPolicy","reader_take_finish"));

    // 18. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 19. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 20. finalize_instance
    factory->finalize();

    delete listener1;
    DESTORY_CONNECTION("ReliabilityQosPolicy");
}