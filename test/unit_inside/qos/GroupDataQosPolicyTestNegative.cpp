#include "GroupDataQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

//class GroupDataQosPolicyTestNegative : public ::testing::Test {
//protected:
//    void SetUp() override;
//    void TearDown() override;
//};
void GroupDataQosPolicyTestNegative::SetUp() {
}

void GroupDataQosPolicyTestNegative::TearDown() {
}

TEST_F(GroupDataQosPolicyTestNegative, ENABLED_GroupDataQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("GroupDataQosPolicy"); 

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

    // 5. PublisherQos配置group_data，通过发现协议传递
    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    pub_qos.group_data.value = {'p', 'u', 'b', 'g', 'r', 'o', 'u', 'p'};
    Publisher* publisher = participant->create_publisher(pub_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    DataWriter* writer = publisher->create_datawriter(
        topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 等待主动端完成第一次DataWriter group_data检查
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("GroupDataQosPolicy", "first_pub_group_data_check", dummy, 10000));

    // 6. SubscriberQos配置group_data，通过发现协议传递
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    sub_qos.group_data.value = {'s', 'u', 'b', 'g', 'r', 'o', 'u', 'p'};
    Subscriber* subscriber = participant->create_subscriber(sub_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    DataReader* reader = subscriber->create_datareader(
        topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 等待主动端完成第一次DataReader group_data检查
    EXPECT_TRUE(TestFrame::getInstance().waitForData("GroupDataQosPolicy", "first_sub_group_data_check", dummy, 10000));

    // 7. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 8. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 9. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("GroupDataQosPolicy");
}
