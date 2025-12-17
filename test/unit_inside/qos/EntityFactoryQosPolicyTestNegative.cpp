#include "QoSCompatibilityTestNegative.h"
#include "EntityFactoryQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "testdataTypeSupport.h"



void EntityFactoryQosPolicyTestNegative::SetUp() {
}

void EntityFactoryQosPolicyTestNegative::TearDown() {
}



TEST_F(EntityFactoryQosPolicyTestNegative, ENABLED_EntityFactoryQosPolicyNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("EntityFactoryQosPolicy");
    
    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建entity_factory_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "entity_factory_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 4. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 5. 默认参数创建DataWriter
    DataWriter* writer = publisher->create_datawriter(
        topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 6. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 7. 默认参数创建DataReader
    DataReader* reader = subscriber->create_datareader(
        topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 8. 等待主动端创建完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("EntityFactoryQosPolicy", "create_finish", dummy, 10000));

    //本地也有匹配 所以统计数量要+1
    // 9. 主动端未手动使能时，无法匹配到对象
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    PublicationMatchedStatus pub_status;
    EXPECT_EQ(writer->get_publication_matched_status(pub_status), RETCODE_OK);
    EXPECT_EQ(pub_status.total_count, 1);
    EXPECT_EQ(pub_status.total_count_change, 1);
    SubscriptionMatchedStatus sub_status;
    EXPECT_EQ(reader->get_subscription_matched_status(sub_status), RETCODE_OK);
    EXPECT_EQ(sub_status.total_count, 1);
    EXPECT_EQ(sub_status.total_count_change, 1);

    // 10. 发送检查完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("EntityFactoryQosPolicy","match_check_1"));

    // 11. 等待主动端使能完成
    EXPECT_TRUE(TestFrame::getInstance().waitForData("EntityFactoryQosPolicy", "enable_finish", dummy, 10000));

    // 12. 主动端手动使能时，能够匹配到对象
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(writer->get_publication_matched_status(pub_status), RETCODE_OK);
    EXPECT_EQ(pub_status.total_count, 2);
    EXPECT_EQ(pub_status.total_count_change, 1);
    EXPECT_EQ(reader->get_subscription_matched_status(sub_status), RETCODE_OK);
    EXPECT_EQ(sub_status.total_count, 2);
    EXPECT_EQ(sub_status.total_count_change, 1);

    // 13. 发送检查完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("EntityFactoryQosPolicy","check_finish"));

    // 14. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 15. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 16. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("EntityFactoryQosPolicy");
}