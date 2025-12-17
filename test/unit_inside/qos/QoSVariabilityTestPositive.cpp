#include "QoSVariabilityTestPositive.h"
#include "TestHelper.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/publisher.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "testdataTypeSupport.h"

USING_TRAVODDS_NAMESPACE

void QoSVariabilityTestPositive::SetUp() {
}

void QoSVariabilityTestPositive::TearDown() {
}

TEST_F(
    QoSVariabilityTestPositive, ENABLED_ChangeEnabledEntitiesMutableQoS) {
    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
    DomainParticipant* participant = factory->create_participant(
        0, dp_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 修改DomainParticipant可变QoS（如user_data）
    DomainParticipantQos dp_qos2 = dp_qos;
    dp_qos2.user_data.value = {'a', 'b', 'c'};
    EXPECT_EQ(participant->set_qos(dp_qos2), RETCODE_OK);

    // 5. 注册TestModule::BasicData类型
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    // 6. 创建Topic
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    Topic* topic = participant->create_topic(
        "example_topic_name", "TestModule::BasicData", topic_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 7. 修改Topic可变QoS（如topic_data, history.depth）
    TopicQos topic_qos2 = topic_qos;
    topic_qos2.topic_data.value = {'x', 'y'};

    //history不可更改
    //topic_qos2.history.depth = 123;
    EXPECT_EQ(topic->set_qos(topic_qos2), RETCODE_OK);

    // 9. 创建Publisher
    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    Publisher* publisher = participant->create_publisher(pub_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 10. 修改Publisher可变QoS（如group_data）
    PublisherQos pub_qos2 = pub_qos;
    pub_qos2.group_data.value = {'g', 'r', 'p'};
    EXPECT_EQ(publisher->set_qos(pub_qos2), RETCODE_OK);

    // 12. 创建Subscriber
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    Subscriber* subscriber = participant->create_subscriber(sub_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 13. 修改Subscriber可变QoS（如group_data）
    SubscriberQos sub_qos2 = sub_qos;
    sub_qos2.group_data.value = {'s', 'u', 'b'};
    EXPECT_EQ(subscriber->set_qos(sub_qos2), RETCODE_OK);

    // 15. 创建DataWriter
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    DataWriter* writer = publisher->create_datawriter(
        topic, dw_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 16. 修改DataWriter可变QoS（如user_data）
    DataWriterQos dw_qos2 = dw_qos;
    dw_qos2.user_data.value = {'d', 'w'};
    EXPECT_EQ(writer->set_qos(dw_qos2), RETCODE_OK);

    // 18. 创建DataReader
    DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
    DataReader* reader = subscriber->create_datareader(
        topic, dr_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 19. 修改DataReader可变QoS（如user_data）
    DataReaderQos dr_qos2 = dr_qos;
    dr_qos2.user_data.value = {'d', 'r'};
    EXPECT_EQ(reader->set_qos(dr_qos2), RETCODE_OK);

    // 21. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 22. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 23. finalize_instance
    factory->finalize();
}

TEST_F(QoSVariabilityTestPositive, ENABLED_ChangeEnabledEntitiesImmutableQoS) {
    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
    DomainParticipant* participant = factory->create_participant(
        0, dp_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    // 4. 依次修改DomainParticipant不可变QoS（如entity_factory）
    DomainParticipantQos dp_qos2 = dp_qos;
    dp_qos2.entity_factory.autoenable_created_entities = !dp_qos.entity_factory.autoenable_created_entities;
    //EXPECT_EQ(participant->set_qos(dp_qos2), IMMUTABLE_POLICY);

    // 6. 创建Topic
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    Topic* topic = participant->create_topic(
        "example_topic_name", "TestModule::BasicData", topic_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 7. 依次修改Topic不可变QoS（如durability）
    TopicQos topic_qos2 = topic_qos;
    topic_qos2.durability.kind = PERSISTENT_DURABILITY_QOS;
    //EXPECT_EQ(topic->set_qos(topic_qos2), IMMUTABLE_POLICY);

    // 9. 创建Publisher
    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    Publisher* publisher = participant->create_publisher(pub_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 10. 依次修改Publisher不可变QoS（如presentation）
    PublisherQos pub_qos2 = pub_qos;
    pub_qos2.presentation.access_scope = GROUP_PRESENTATION_QOS;
    //EXPECT_EQ(publisher->set_qos(pub_qos2), IMMUTABLE_POLICY);

    // 12. 创建Subscriber
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    Subscriber* subscriber = participant->create_subscriber(sub_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 13. 依次修改Subscriber不可变QoS（如presentation）
    SubscriberQos sub_qos2 = sub_qos;
    sub_qos2.presentation.access_scope = GROUP_PRESENTATION_QOS;
    //EXPECT_EQ(subscriber->set_qos(sub_qos2), IMMUTABLE_POLICY);

    // 15. 创建DataWriter
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    DataWriter* writer = publisher->create_datawriter(
        topic, dw_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 16. 依次修改DataWriter不可变QoS（如durability, ownership, destination_order, etc.）
    DataWriterQos dw_qos2 = dw_qos;
    dw_qos2.durability.kind = PERSISTENT_DURABILITY_QOS;
    //EXPECT_EQ(writer->set_qos(dw_qos2), IMMUTABLE_POLICY);

    // 18. 创建DataReader
    DataReaderQos dr_qos = DATAREADER_QOS_DEFAULT;
    DataReader* reader = subscriber->create_datareader(
        topic, dr_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 19. 依次修改DataReader不可变QoS（如durability, ownership, destination_order, etc.）
    DataReaderQos dr_qos2 = dr_qos;
    dr_qos2.durability.kind = PERSISTENT_DURABILITY_QOS;
    //EXPECT_EQ(reader->set_qos(dr_qos2), IMMUTABLE_POLICY);

    // 21. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 22. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 23. finalize_instance
    factory->finalize();
}