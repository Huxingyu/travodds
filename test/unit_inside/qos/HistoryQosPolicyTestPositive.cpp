#include "HistoryQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

void HistoryQosPolicyTestPositive::SetUp() {
    std::cout << "HistoryQosPolicyTestPositive::SetUp" << std::endl;
}

void HistoryQosPolicyTestPositive::TearDown() {
    std::cout << "HistoryQosPolicyTestPositive::TearDown" << std::endl;
}

TEST_F(HistoryQosPolicyTestPositive, ENABLED_HistoryQosPolicyPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("HistoryQosPolicyTestNegative.ENABLED_HistoryQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("HistoryQosPolicy"); // 创建连接

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建history_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "history_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 4. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 5. 创建自定义DataWriterQos对象1，History=KEEP_LAST, depth=10
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.history.kind = KEEP_LAST_HISTORY_QOS;
    dw_qos.history.depth = 10;

    // 6. 创建DataWriter1
    DataWriter* writer = publisher->create_datawriter(
        topic, dw_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 7. 等待被动端DataReader匹配完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("HistoryQosPolicy", "reader_match_finish", dummy, 10000));

    // 8. 间隔10ms发送10包样本数据
    for (int i = 0; i < 10; ++i) {
        TestModule::BasicData sample;
        sample.id = i;
        EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 9. 发送写入完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("HistoryQosPolicy","write_finish"));

    // 10. 等待被动端DataReader接收完成
    EXPECT_TRUE(TestFrame::getInstance().waitForData("HistoryQosPolicy", "reader_take_finish", dummy, 10000));

    // 11. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 12. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 13. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("HistoryQosPolicy");
}