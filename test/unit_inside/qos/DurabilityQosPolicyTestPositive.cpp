#include "DurabilityQosPolicyTestPositive.h"
#include "TestHelper.h"
#include "TestFrame.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "testdataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include <thread>

USING_TRAVODDS_NAMESPACE

void DurabilityQosPolicyTestPositive::SetUp() {
    std::cout << "DurabilityQosPolicyTestPositive::SetUp" << std::endl;
}

void DurabilityQosPolicyTestPositive::TearDown() {
    std::cout << "DurabilityQosPolicyTestPositive::TearDown" << std::endl;
}

TEST_F(DurabilityQosPolicyTestPositive, ENABLED_DurabilityQosPolicyPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("DurabilityQosPolicyTestNegative.ENABLED_DurabilityQosPolicyNegative");
    CREATE_AND_SYNC_CONNECTION("DurabilityQosPolicy"); // 创建连接

    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::BasicData类型，并创建durability_topic_name主题
    TestModule::BasicDataTypeSupport* type_support = TestModule::BasicDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::BasicData", type_support), RETCODE_OK);

    Topic* topic = participant->create_topic(
        "durability_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 4. 创建Publisher
    Publisher* publisher = participant->create_publisher(
        PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);

    // 5. 创建自定义DataWriterQos对象1，Durability=TRANSIENT_LOCAL
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    // 6. 创建DataWriter1
    DataWriter* writer = publisher->create_datawriter(
        topic, dw_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    // 7. 发送一包样本数据，lVal=1
    TestModule::BasicData sample;
    sample.id = 1;
    EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);

    // 8. 发送写入完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("DurabilityQosPolicy","write_finish"));

    // 9. 等待被动端检查完成
    std::string dummy;
    EXPECT_TRUE(TestFrame::getInstance().waitForData("DurabilityQosPolicy", "reader_check_finish", dummy, 10000));

    // 10. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 11. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 12. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("DurabilityQosPolicy");
}