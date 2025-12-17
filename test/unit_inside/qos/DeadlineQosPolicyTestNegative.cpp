#include "QoSCompatibilityTestNegative.h"
#include "DeadlineQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"
//#include "types/TestModule::BasicDataTypeSupport.h"

void DeadlineQosPolicyTestNegative::SetUp() {
}

void DeadlineQosPolicyTestNegative::TearDown() {
}

class DeadlineQosPolicyDataReaderListener : public BaseDataReaderListener {
    // 直接继承BaseDataReaderListener，无需额外实现
};

TEST_F(DeadlineQosPolicyTestNegative, ENABLED_DeadlineQosPolicyNegative) {
    //等待连接
    WAIT_AND_SYNC_CONNECTION("DeadlineQosPolicy");

    // 2. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 3. 默认参数创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建deadline_topic_name主题
	TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "deadline_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 等待主动端创建DataWriter
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("DeadlineQosPolicy", "writer_created_finish", dummy, 10000));

    // 7. 创建自定义DataReaderQos对象，Deadline.period=2000ms
    DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
    dr_qos.deadline.period.sec = 2;
    dr_qos.deadline.period.nanosec = 0;

    // 8. 创建Listener
    auto listener = new DeadlineQosPolicyDataReaderListener();
    // 9. 创建DataReader
    DataReader* reader = subscriber->create_datareader(
        topic, dr_qos, listener, STATUS_MASK_ALL);
    ASSERT_NE(reader, nullptr);

    // 10. 在周期内能接收到数据
    RequestedDeadlineMissedStatus status;
    EXPECT_EQ(reader->get_requested_deadline_missed_status(status), RETCODE_OK);
    EXPECT_EQ(status.total_count, 0);
    EXPECT_EQ(status.total_count_change, 0);

    // 11. 等待主动端写入完成信号
    EXPECT_TRUE(TestFrame::getInstance().waitForData("DeadlineQosPolicy", "write_finish", dummy, 10000));

    // 12. 在周期内不能接收到数据
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    EXPECT_EQ(reader->get_requested_deadline_missed_status(status), RETCODE_OK);
    EXPECT_EQ(status.total_count, 1);
    //因为在listener中读取过status,total_count_change会被重置为0
    EXPECT_EQ(status.total_count_change, 0);
    EXPECT_EQ(listener->wait_requested_deadline_missed(1000), 0); // 成功

    // 13. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 14. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 15. finalize_instance
    factory->finalize();
    //内部会释放listener 
    //delete listener;
    DESTORY_CONNECTION("DeadlineQosPolicy");
}