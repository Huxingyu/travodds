#include "UserDataQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "testdataTypeSupport.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

void UserDataQosPolicyTestPositive::SetUp() {
    std::cout << "UserDataQosPolicyTestPositive::SetUp" << std::endl;
}

void UserDataQosPolicyTestPositive::TearDown() {
    std::cout << "UserDataQosPolicyTestPositive::TearDown" << std::endl;
}

TEST_F(UserDataQosPolicyTestPositive, ENABLED_UserDataQosPolicyPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("UserDataQosPolicyTestNegative.ENABLED_UserDataQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("UserDataQosPolicy"); // 创建连接

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建example_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "example_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 4. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 创建DataReader
    DataReader* reader = subscriber->create_datareader(
        topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 7. 轮询get_matched_publications直到有匹配
    std::vector<InstanceHandle_t> pub_handles;
    for (int i = 0; i < 50; ++i) {
        pub_handles.clear();
        EXPECT_EQ(reader->get_matched_publications(pub_handles), RETCODE_OK);
        if (!pub_handles.empty()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_FALSE(pub_handles.empty());

    // 8. 检查PublicationBuiltinTopicData.user_data
    bool found_writer_user_data = false;
    for (const auto& h : pub_handles) {
        PublicationBuiltinTopicData data;
        EXPECT_EQ(reader->get_matched_publication_data(data, h), RETCODE_OK);
        std::string user_data_str(data.user_data.value.begin(), data.user_data.value.end());
        if (user_data_str == "writeruser") {
            found_writer_user_data = true;
        }
    }
    EXPECT_TRUE(found_writer_user_data);

    // 9. 发送DataWriter user_data检查信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("UserDataQosPolicy", "first_writer_user_data_check"));

    // 10. 创建DataWriter
    DataWriter* writer = publisher->create_datawriter(
        topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
    ASSERT_NE(writer, nullptr);

    // 11. 轮询get_matched_subscriptions直到有匹配
    std::vector<InstanceHandle_t> sub_handles;
    for (int i = 0; i < 50; ++i) {
        sub_handles.clear();
        EXPECT_EQ(writer->get_matched_subscriptions(sub_handles), RETCODE_OK);
        if (!sub_handles.empty()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_FALSE(sub_handles.empty());

    // 12. 检查SubscriptionBuiltinTopicData.user_data
    bool found_reader_user_data = false;
    for (const auto& h : sub_handles) {
        SubscriptionBuiltinTopicData data;
        EXPECT_EQ(writer->get_matched_subscription_data(data, h), RETCODE_OK);
        std::string user_data_str(data.user_data.value.begin(), data.user_data.value.end());
        if (user_data_str == "readeruser") {
            found_reader_user_data = true;
        }
    }
    EXPECT_TRUE(found_reader_user_data);

    // 13. 发送DataReader user_data检查信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("UserDataQosPolicy", "first_reader_user_data_check"));

    // 14. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 15. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 16. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("UserDataQosPolicy");
}