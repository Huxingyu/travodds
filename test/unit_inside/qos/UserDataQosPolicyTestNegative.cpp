#include "QoSCompatibilityTestNegative.h"
#include "UserDataQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
//#include "types/TestModule::BasicDataTypeSupport.h"

#include "testdataTypeSupport.h"


void UserDataQosPolicyTestNegative::SetUp() {
}

void UserDataQosPolicyTestNegative::TearDown() {
}

TEST_F(UserDataQosPolicyTestNegative, ENABLED_UserDataQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("UserDataQosPolicy");

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 自定义DomainParticipantQos对象，设置user_data
    DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
    dp_qos.user_data.value = {'d', 'p', 'u', 's', 'e', 'r'};

    // 3. 以自定义Qos创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, dp_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 4. 注册TestModule::BasicData类型，并创建example_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "example_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 5. 默认创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 6. 默认创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 7. DataWriterQos配置user_data，通过发现协议传递
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.user_data.value = {'w', 'r', 'i', 't', 'e', 'r', 'u', 's', 'e', 'r'};
    DataWriter* writer = publisher->create_datawriter(
        topic, dw_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 8. 等待主动端完成第一次DataWriter user_data检查
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("UserDataQosPolicy", "first_writer_user_data_check", dummy, 10000));

    // 9. DataReaderQos配置user_data，通过发现协议传递
    DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
    dr_qos.user_data.value = {'r', 'e', 'a', 'd', 'e', 'r', 'u', 's', 'e', 'r'};
    DataReader* reader = subscriber->create_datareader(
        topic, dr_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 10. 等待主动端完成第一次DataReader user_data检查
    EXPECT_TRUE(TestFrame::getInstance().waitForData("UserDataQosPolicy", "first_reader_user_data_check", dummy, 10000));

    // 11. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 12. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 13. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("UserDataQosPolicy");
}
