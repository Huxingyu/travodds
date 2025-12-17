#include "TopicDataQosPolicyTestNegative.h"
#include "TestFrame.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
#include "testdataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"

USING_TRAVODDS_NAMESPACE

void TopicDataQosPolicyTestNegative::SetUp() {}
void TopicDataQosPolicyTestNegative::TearDown() {}

TEST_F(TopicDataQosPolicyTestNegative, ENABLED_TopicDataQosPolicyDiscoveryNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("TopicDataQosPolicyDiscovery");

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    // 4. 自定义TopicQos，设置topic_data为"remote_topic_data_test"
    TopicQos qos;
    qos = TOPIC_QOS_DEFAULT;
    qos.topic_data.value.assign({'r','e','m','o','t','e','_','t','o','p','i','c','_','d','a','t','a','_','t','e','s','t'});
    Topic* topic = participant->create_topic(
        "discovery_topic_name", "TestModule::BasicData", qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 5. 创建Publisher
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 6. 创建DataWriter
    DataWriter* writer = publisher->create_datawriter(
        topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 7. 等待主动端完成第一次TopicData检查
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TopicDataQosPolicyDiscovery", "first_topic_data_check", dummy, 10000));

    // 8. 动态修改topic_data为"updated_topic_data_test"
    TopicQos qos2 = qos;
    qos2.topic_data.value.assign({'u','p','d','a','t','e','d','_','t','o','p','i','c','_','d','a','t','a','_','t','e','s','t'});
    EXPECT_EQ(topic->set_qos(qos2), RETCODE_OK);

    // 9. 等待主动端完成第二次TopicData检查
    EXPECT_TRUE(TestFrame::getInstance().waitForData("TopicDataQosPolicyDiscovery", "second_topic_data_check", dummy, 10000));

    // 10. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 11. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 12. finalize_instance
    factory->finalize();
}
