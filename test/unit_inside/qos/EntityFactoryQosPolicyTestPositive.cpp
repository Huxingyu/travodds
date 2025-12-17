#include "EntityFactoryQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

void EntityFactoryQosPolicyTestPositive::SetUp() {
    std::cout << "EntityFactoryQosPolicyTestPositive::SetUp" << std::endl;
}

void EntityFactoryQosPolicyTestPositive::TearDown() {
    std::cout << "EntityFactoryQosPolicyTestPositive::TearDown" << std::endl;
}

TEST_F(EntityFactoryQosPolicyTestPositive, ENABLED_EntityFactoryQosPolicyPositive) {
    
    // Notify negative
    NOTIFY_NEGATIVE("EntityFactoryQosPolicyTestNegative.ENABLED_EntityFactoryQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("EntityFactoryQosPolicy");

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipantFactoryQos对象，设置autoenable_created_entities=false
    //DomainParticipantFactoryQos factory_qos = PARTICIPANT_FACTORY_QOS_DEFAULT;
    DomainParticipantFactoryQos factory_qos;
    factory_qos.entity_factory.autoenable_created_entities = false;

    // 3. 设置工厂Qos
    EXPECT_EQ(factory->set_qos(factory_qos), RETCODE_OK);

    // 4. 创建DomainParticipantQos对象，设置autoenable_created_entities=false
    DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
    dp_qos.entity_factory.autoenable_created_entities = false;

    // 5. 以自定义Qos对象创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, dp_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 6. 注册TestModule::BasicData类型，并创建entity_factory_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "entity_factory_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 7. 创建PublisherQos对象，设置autoenable_created_entities=false
    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    pub_qos.entity_factory.autoenable_created_entities = false;


    // 8. 以自定义Qos对象创建Publisher
    Publisher* publisher = participant->create_publisher(pub_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 9. 默认参数创建DataWriter
    DataWriter* writer = publisher->create_datawriter(
        topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 10. 创建SubscriberQos对象，设置autoenable_created_entities=false
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    sub_qos.entity_factory.autoenable_created_entities = false;

    // 11. 以自定义Qos对象创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(sub_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 12. 默认参数创建DataReader
    DataReader* reader = subscriber->create_datareader(
        topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 13. 发送创建完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("EntityFactoryQosPolicy","create_finish"));

    // 14. 等待被动端检查完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("EntityFactoryQosPolicy", "match_check_1", dummy, 10000));

    // 15. 手动使能实体
    EXPECT_EQ(participant->enable(), RETCODE_OK);
    EXPECT_EQ(publisher->enable(), RETCODE_OK);
    EXPECT_EQ(subscriber->enable(), RETCODE_OK);
    EXPECT_EQ(writer->enable(), RETCODE_OK);
    EXPECT_EQ(reader->enable(), RETCODE_OK);

    // 16. 发送使能完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("EntityFactoryQosPolicy","enable_finish"));

    // 17. 等待被动端检查完成
    EXPECT_TRUE(TestFrame::getInstance().waitForData("EntityFactoryQosPolicy", "check_finish", dummy, 10000));

    // 18. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 19. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 20. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("EntityFactoryQosPolicy");
}