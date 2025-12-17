#include "QoSCompatibilityTestPositive.h"
#include "DeadlineQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "testdataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

void DeadlineQosPolicyTestPositive::SetUp() {
    std::cout << "DeadlineQosPolicyTestPositive::SetUp" << std::endl;
}

void DeadlineQosPolicyTestPositive::TearDown() {
    std::cout << "DeadlineQosPolicyTestPositive::TearDown" << std::endl;
}

class DeadlineQosPolicyDataWriterListener : public BaseDataWriterListener {
    // 直接继承BaseDataWriterListener，无需额外实现
};

TEST_F(DeadlineQosPolicyTestPositive, ENABLED_DeadlineQosPolicyPositive) {
    // 1.通知辅测端
    NOTIFY_NEGATIVE("DeadlineQosPolicyTestNegative.ENABLED_DeadlineQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("DeadlineQosPolicy");

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

    // 5. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 6. 创建自定义DataWriterQos对象，Deadline.period=500ms
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.deadline.period.sec = 0;
    dw_qos.deadline.period.nanosec = 500 * 1000 * 1000; // 500ms

    // 7. 创建Listener
    auto listener = new DeadlineQosPolicyDataWriterListener();
    // 8. 创建DataWriter
    DataWriter* writer = publisher->create_datawriter(
        topic, dw_qos, listener, STATUS_MASK_ALL);
    ASSERT_NE(writer, nullptr);

    // 9. 发送DataWriter创建完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("DeadlineQosPolicy", "writer_created_finish"));

    // 10. 满足Deadline不产生状态和触发回调
    for (int i = 0; i < 10; ++i) {
        TestModule::BasicData sample;
        sample.id = i;
        EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
        OfferedDeadlineMissedStatus status;
        EXPECT_EQ(writer->get_offered_deadline_missed_status(status), RETCODE_OK);
        EXPECT_EQ(status.total_count, 0);
        EXPECT_EQ(status.total_count_change, 0);
        EXPECT_EQ(listener->wait_offered_deadline_missed(100), 1); // 超时
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // 11. 不满足Deadline产生状态和触发回调
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    TestModule::BasicData sample;
    sample.id = 100;
    EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
    OfferedDeadlineMissedStatus status;
    EXPECT_EQ(writer->get_offered_deadline_missed_status(status), RETCODE_OK);
    EXPECT_EQ(status.total_count, 1);
    //因为在listener中读取过status,total_count_change会被重置为0
    EXPECT_EQ(status.total_count_change, 0);
    EXPECT_EQ(listener->wait_offered_deadline_missed(1000), 0); // 成功

    // 12. 发送写入完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("DeadlineQosPolicy", "write_finish"));

    // 13. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 14. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 15. finalize_instance
    factory->finalize();

    DESTORY_CONNECTION("DeadlineQosPolicy");
}