#include "TimeBasedFilterQosPolicyTestPositive.h"
#include "TestFrame.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "testdataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"

USING_TRAVODDS_NAMESPACE

void TimeBasedFilterQosPolicyTestPositive::SetUp() {
    std::cout << "TimeBasedFilterQosPolicyTestPositive::SetUp" << std::endl;
}

void TimeBasedFilterQosPolicyTestPositive::TearDown() {
    std::cout << "TimeBasedFilterQosPolicyTestPositive::TearDown" << std::endl;
}

TEST_F(TimeBasedFilterQosPolicyTestPositive, ENABLED_TimeBasedFilterQosPolicyBasicTestPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("TimeBasedFilterQosPolicyTestNegative.ENABLED_TimeBasedFilterQosPolicyBasicTestNegative");
    CREATE_AND_SYNC_CONNECTION("TimeBasedFilterQosPolicyBasicTest"); // 创建连接

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建time_filter_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "time_filter_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 4. 创建Publisher
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 5. 创建DataWriter
    DataWriter* writer = publisher->create_datawriter(
        topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 6. 等待被动端准备完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TimeBasedFilterQosPolicyBasicTest", "reader_ready", dummy, 10000));

    // 7. 快速发送测试数据
    TestModule::BasicData inst1; inst1.id = 100;
    EXPECT_EQ(writer->write(&inst1, HANDLE_NIL), RETCODE_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    TestModule::BasicData inst2; inst2.id = 200;
    EXPECT_EQ(writer->write(&inst2, HANDLE_NIL), RETCODE_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    TestModule::BasicData inst3; inst3.id = 300;
    EXPECT_EQ(writer->write(&inst3, HANDLE_NIL), RETCODE_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    TestModule::BasicData inst4; inst4.id = 400;
    EXPECT_EQ(writer->write(&inst4, HANDLE_NIL), RETCODE_OK);
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TimeBasedFilterQosPolicyBasicTest", "fast_send_check", dummy, 10000));

    // 8. 慢速发送测试数据
    TestModule::BasicData inst5; inst5.id = 500;
    EXPECT_EQ(writer->write(&inst5, HANDLE_NIL), RETCODE_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    TestModule::BasicData inst6; inst6.id = 600;
    EXPECT_EQ(writer->write(&inst6, HANDLE_NIL), RETCODE_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    TestModule::BasicData inst7; inst7.id = 700;
    EXPECT_EQ(writer->write(&inst7, HANDLE_NIL), RETCODE_OK);
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TimeBasedFilterQosPolicyBasicTest", "slow_send_check", dummy, 10000));

    // 9. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 10. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 11. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("TimeBasedFilterQosPolicyBasicTest");
}

TEST_F(TimeBasedFilterQosPolicyTestPositive, ENABLED_TimeBasedFilterQosPolicyDynamicTestPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("TimeBasedFilterQosPolicyTestNegative.ENABLED_TimeBasedFilterQosPolicyDynamicTestNegative");
    CREATE_AND_SYNC_CONNECTION("TimeBasedFilterQosPolicyDynamicTest"); // 创建连接

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建dynamic_time_filter_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "dynamic_time_filter_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 4. 创建Publisher
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 5. 创建DataWriter
    DataWriter* writer = publisher->create_datawriter(
        topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 6. 等待被动端准备完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TimeBasedFilterQosPolicyDynamicTest", "reader_ready", dummy, 10000));

    // 7. 第一阶段测试（初始过滤参数）
    for (int i = 1; i <= 5; ++i) {
        TestModule::BasicData inst; inst.id = i * 10;
        EXPECT_EQ(writer->write(&inst, HANDLE_NIL), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TimeBasedFilterQosPolicyDynamicTest", "first_phase_check", dummy, 10000));

    // 8. 等待被动端修改过滤参数
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TimeBasedFilterQosPolicyDynamicTest", "filter_param_changed", dummy, 10000));

    // 9. 第二阶段测试（修改后的过滤参数）
    for (int i = 6; i <= 10; ++i) {
        TestModule::BasicData inst; inst.id = i * 10;
        EXPECT_EQ(writer->write(&inst, HANDLE_NIL), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TimeBasedFilterQosPolicyDynamicTest", "second_phase_check", dummy, 10000));

    // 10. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 11. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 12. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("TimeBasedFilterQosPolicyDynamicTest");
}

TEST_F(TimeBasedFilterQosPolicyTestPositive, ENABLED_TimeBasedFilterQosPolicyKeyedTypeTestPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("TimeBasedFilterQosPolicyTestNegative.ENABLED_TimeBasedFilterQosPolicyKeyedTypeTestNegative");
    CREATE_AND_SYNC_CONNECTION("TimeBasedFilterQosPolicyKeyedTypeTest"); // 创建连接

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::KeyData类型，并创建keyed_time_filter_topic_name主题
    TestModule::KeyDataTypeSupport* type_support = TestModule::KeyDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::KeyData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "keyed_time_filter_topic_name", "TestModule::KeyData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 4. 创建Publisher
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 5. 创建DataWriter
    DataWriter* writer = publisher->create_datawriter(
        topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 6. 等待被动端准备完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TimeBasedFilterQosPolicyKeyedTypeTest", "reader_ready", dummy, 10000));

    // 7. 发送多实例数据测试时间过滤
    TestModule::KeyData instA; instA.userId = 1;
    TestModule::KeyData instB; instB.userId = 2;
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(writer->write(&instA, HANDLE_NIL), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(writer->write(&instB, HANDLE_NIL), RETCODE_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_EQ(writer->write(&instA, HANDLE_NIL), RETCODE_OK);
    EXPECT_EQ(writer->write(&instB, HANDLE_NIL), RETCODE_OK);

    // 8. 等待被动端检查完成
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TimeBasedFilterQosPolicyKeyedTypeTest", "keyed_filter_check", dummy, 10000));

    // 9. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 10. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 11. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("TimeBasedFilterQosPolicyKeyedTypeTest");
}
