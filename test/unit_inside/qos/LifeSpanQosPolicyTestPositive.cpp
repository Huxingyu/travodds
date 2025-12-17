#include "LifeSpanQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

void LifeSpanQosPolicyTestPositive::SetUp() {
    std::cout << "LifeSpanQosPolicyTestPositive::SetUp" << std::endl;
}

void LifeSpanQosPolicyTestPositive::TearDown() {
    std::cout << "LifeSpanQosPolicyTestPositive::TearDown" << std::endl;
}

//该qos还未实现
TEST_F(LifeSpanQosPolicyTestPositive, ENABLED_LifeSpanQosPolicyPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("LifeSpanQosPolicyTestNegative.ENABLED_LifeSpanQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("LifeSpanQosPolicy"); // 创建连接

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建lifespan_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "lifespan_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 4. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 5. 创建自定义DataWriterQos对象，Lifespan=2000ms
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.lifespan.duration.sec = 2;
    dw_qos.lifespan.duration.nanosec = 0;

    // 6. 创建DataWriter
    DataWriter* writer = publisher->create_datawriter(
        topic, dw_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 7. 等待被动端DataReader匹配完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("LifeSpanQosPolicy", "reader_match_finish", dummy, 10000));

    // 8. 周期为1ms发送100包样本数据
    for (int i = 0; i < 100; ++i) {
        TestModule::BasicData sample;
        sample.id = i;
        EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 9. 发送DataWriter写入完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("LifeSpanQosPolicy","write_finish"));

    // 10. 等待被动端DataReader接收完成
    EXPECT_TRUE(TestFrame::getInstance().waitForData("LifeSpanQosPolicy", "reader_take_finish", dummy, 10000));

    // 11. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 12. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 13. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("LifeSpanQosPolicy");
}