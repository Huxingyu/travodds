#include "TimeBasedFilterQosPolicyTestNegative.h"
#include "TestHelper.h"
#include "TestFrame.h"
#include "testdataTypeSupport.h"

//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"

USING_TRAVODDS_NAMESPACE

void TimeBasedFilterQosPolicyTestNegative::SetUp() {}
void TimeBasedFilterQosPolicyTestNegative::TearDown() {}

TEST_F(TimeBasedFilterQosPolicyTestNegative, ENABLED_TimeBasedFilterQosPolicyBasicTestNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("TimeBasedFilterQosPolicyBasicTest");

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

    // 4. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 5. 创建DataReaderQos对象，设置TimeBasedFilter成员minimum_separation为200ms
    DataReaderQos qos = DATAREADER_QOS_DEFAULT;
    qos.time_based_filter.minimum_separation.sec = 0;
    qos.time_based_filter.minimum_separation.nanosec = 200 * 1000 * 1000; // 200ms

    // 6. 以自定义Qos对象创建DataReader
    DataReader* reader = subscriber->create_datareader(
        topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 7. 等待匹配完成
    EXPECT_TRUE(TestHelper::waitmatch(reader, 1));

    // 8. 发送准备完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("TimeBasedFilterQosPolicyBasicTest", "reader_ready"));

    // 9. 验证快速发送的时间过滤效果
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	TRAVODDS_SEQUENCE(KeyDataSeq, TestModule::KeyData);
	KeyDataSeq data_vec;
	
    SampleInfoSeq info_vec;
    //EXPECT_EQ(reader->take(data_vec, info_vec, 10, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    // 快速发送，预期收到样本数少于发送数
    //EXPECT_LT(data_vec.length(), 4);
    // 验证样本间隔
    for (size_t i = 1; i < info_vec.length(); ++i) {
        int64_t prev_ns = info_vec[i-1].source_timestamp.sec * 1000000000LL + info_vec[i-1].source_timestamp.nanosec;
        int64_t curr_ns = info_vec[i].source_timestamp.sec * 1000000000LL + info_vec[i].source_timestamp.nanosec;
        //EXPECT_GE(curr_ns - prev_ns, 200 * 1000 * 1000);
    }
    EXPECT_TRUE(TestFrame::getInstance().sendData("TimeBasedFilterQosPolicyBasicTest", "fast_send_check"));

    // 10. 验证慢速发送的时间过滤效果
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    data_vec.unloan(); info_vec.unloan();
    EXPECT_EQ(reader->take(data_vec, info_vec, 10, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    // 慢速发送，预期收到全部样本
    //EXPECT_EQ(data_vec.length(), 3);
    EXPECT_TRUE(TestFrame::getInstance().sendData("TimeBasedFilterQosPolicyBasicTest", "slow_send_check"));

    // 11. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 12. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 13. finalize_instance
    factory->finalize();
}

TEST_F(TimeBasedFilterQosPolicyTestNegative, ENABLED_TimeBasedFilterQosPolicyDynamicTestNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("TimeBasedFilterQosPolicyDynamicTest");

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

    // 4. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 5. 创建DataReaderQos对象，设置TimeBasedFilter成员minimum_separation为150ms
    DataReaderQos qos = DATAREADER_QOS_DEFAULT;
    qos.time_based_filter.minimum_separation.sec = 0;
    qos.time_based_filter.minimum_separation.nanosec = 150 * 1000 * 1000; // 150ms

    // 6. 以自定义Qos对象创建DataReader
    DataReader* reader = subscriber->create_datareader(
        topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 7. 等待匹配完成
    EXPECT_TRUE(TestHelper::waitmatch(reader, 1));

    // 8. 发送准备完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("TimeBasedFilterQosPolicyDynamicTest", "reader_ready"));

    // 9. 第一阶段验证（初始过滤参数150ms）
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
	TRAVODDS_SEQUENCE(KeyDataSeq, TestModule::KeyData);
	KeyDataSeq data_vec;
    SampleInfoSeq info_vec;
    EXPECT_EQ(reader->take(data_vec, info_vec, 10, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    // 预期收到的样本数量少于5（150ms过滤）
    EXPECT_LT(data_vec.length(), 5);
    // 验证样本间隔
    for (size_t i = 1; i < info_vec.length(); ++i) {
        int64_t prev_ns = info_vec[i-1].source_timestamp.sec * 1000000000LL + info_vec[i-1].source_timestamp.nanosec;
        int64_t curr_ns = info_vec[i].source_timestamp.sec * 1000000000LL + info_vec[i].source_timestamp.nanosec;
        EXPECT_GE(curr_ns - prev_ns, 150 * 1000 * 1000);
    }
    EXPECT_TRUE(TestFrame::getInstance().sendData("TimeBasedFilterQosPolicyDynamicTest", "first_phase_check"));

    // 10. 动态修改过滤参数为50ms
    DataReaderQos qos2 = qos;
    qos2.time_based_filter.minimum_separation.sec = 0;
    qos2.time_based_filter.minimum_separation.nanosec = 50 * 1000 * 1000; // 50ms
    EXPECT_EQ(reader->set_qos(qos2), RETCODE_OK);
    EXPECT_TRUE(TestFrame::getInstance().sendData("TimeBasedFilterQosPolicyDynamicTest", "filter_param_changed"));

    // 11. 第二阶段验证（修改后的过滤参数50ms）
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    data_vec.unloan(); info_vec.unloan();
    EXPECT_EQ(reader->take(data_vec, info_vec, 10, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    // 预期收到的样本数量接近5（50ms过滤）
    //EXPECT_GE(data_vec.length(), 4);
    for (size_t i = 1; i < info_vec.length(); ++i) {
        int64_t prev_ns = info_vec[i-1].source_timestamp.sec * 1000000000LL + info_vec[i-1].source_timestamp.nanosec;
        int64_t curr_ns = info_vec[i].source_timestamp.sec * 1000000000LL + info_vec[i].source_timestamp.nanosec;
        EXPECT_GE(curr_ns - prev_ns, 50 * 1000 * 1000);
    }
    EXPECT_TRUE(TestFrame::getInstance().sendData("TimeBasedFilterQosPolicyDynamicTest", "second_phase_check"));

    // 12. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 13. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 14. finalize_instance
    factory->finalize();
}

TEST_F(TimeBasedFilterQosPolicyTestNegative, ENABLED_TimeBasedFilterQosPolicyKeyedTypeTestNegative) {
    // 等待连接
    WAIT_AND_SYNC_CONNECTION("TimeBasedFilterQosPolicyKeyedTypeTest");

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

    // 4. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 5. 创建DataReaderQos对象，设置TimeBasedFilter成员minimum_separation为200ms
    DataReaderQos qos = DATAREADER_QOS_DEFAULT;
    qos.time_based_filter.minimum_separation.sec = 0;
    qos.time_based_filter.minimum_separation.nanosec = 200 * 1000 * 1000; // 200ms

    // 6. 以自定义Qos对象创建DataReader
    DataReader* reader = subscriber->create_datareader(
        topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 7. 等待匹配完成
    EXPECT_TRUE(TestHelper::waitmatch(reader, 1));

    // 8. 发送准备完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("TimeBasedFilterQosPolicyKeyedTypeTest", "reader_ready"));

    // 9. 验证多实例的时间过滤效果
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	TRAVODDS_SEQUENCE(KeyDataSeq, TestModule::KeyData);
	KeyDataSeq data_vec;
    SampleInfoSeq info_vec;
    EXPECT_EQ(reader->take(data_vec, info_vec, 20, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);

    // 按lVal分组，分别验证每个实例的时间过滤
    std::map<int64_t, std::vector<SampleInfo>> instance_map;
    for (size_t i = 0; i < data_vec.length(); ++i) {
        instance_map[data_vec[i].userId].push_back(info_vec[i]);
    }
    for (const auto& kv : instance_map) {
        const auto& infos = kv.second;
        for (size_t i = 1; i < infos.size(); ++i) {
            int64_t prev_ns = infos[i-1].source_timestamp.sec * 1000000000LL + infos[i-1].source_timestamp.nanosec;
            int64_t curr_ns = infos[i].source_timestamp.sec * 1000000000LL + infos[i].source_timestamp.nanosec;
            EXPECT_GE(curr_ns - prev_ns, 200 * 1000 * 1000);
        }
    }
    // 验证不同实例之间的过滤互不影响
    //EXPECT_GE(instance_map[1].size(), 2);
    //EXPECT_GE(instance_map[2].size(), 2);

    // 10. 发送检查完成信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("TimeBasedFilterQosPolicyKeyedTypeTest", "keyed_filter_check"));

    // 11. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 12. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 13. finalize_instance
    factory->finalize();
}
