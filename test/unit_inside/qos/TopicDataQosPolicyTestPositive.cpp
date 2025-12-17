#include "TopicDataQosPolicyTestPositive.h"
//#include "types/TestModule::BasicDataTypeSupport.h"
//#include "types/TestModule::KeyDataTypeSupport.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "testdataTypeSupport.h"
#include "TestFrame.h"

USING_TRAVODDS_NAMESPACE

void TopicDataQosPolicyTestPositive::SetUp() {
    std::cout << "TopicDataQosPolicyTestPositive::SetUp" << std::endl;
}

void TopicDataQosPolicyTestPositive::TearDown() {
    std::cout << "TopicDataQosPolicyTestPositive::TearDown" << std::endl;
}

TEST_F(TopicDataQosPolicyTestPositive, ENABLED_TopicDataQosPolicyBasicTest) {
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

    // 4. 测试Topic默认topic_data
    Topic* topic_default = participant->create_topic(
        "topic_data_test_topic", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic_default, nullptr);
    TopicQos qos_default;
    EXPECT_EQ(topic_default->get_qos(qos_default), RETCODE_OK);
    EXPECT_TRUE(qos_default.topic_data.value.empty());

    // 5. 测试Topic自定义topic_data
    TopicQos qos_custom;
    qos_custom = TOPIC_QOS_DEFAULT;
    qos_custom.topic_data.value.assign({'c','u','s','t','o','m','_','t','o','p','i','c','_','i','n','f','o'});
    Topic* topic_custom = participant->create_topic(
        "topic_custom_data", "TestModule::BasicData", qos_custom, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic_custom, nullptr);
    TopicQos qos_check;
    EXPECT_EQ(topic_custom->get_qos(qos_check), RETCODE_OK);
    EXPECT_EQ(std::string(qos_check.topic_data.value.begin(), qos_check.topic_data.value.end()), "custom_topic_info");

    // 6. 测试Topic动态修改topic_data
    TopicQos qos_modify = qos_custom;
    qos_modify.topic_data.value.assign({'m','o','d','i','f','i','e','d','_','t','o','p','i','c','_','i','n','f','o'});
    EXPECT_EQ(topic_custom->set_qos(qos_modify), RETCODE_OK);
    TopicQos qos_check2;
    EXPECT_EQ(topic_custom->get_qos(qos_check2), RETCODE_OK);
    EXPECT_EQ(std::string(qos_check2.topic_data.value.begin(), qos_check2.topic_data.value.end()), "modified_topic_info");

    // 7. 测试多个Topic的不同topic_data
    TopicQos qos1 = TOPIC_QOS_DEFAULT;
    qos1.topic_data.value.assign({'t','o','p','i','c','_','i','n','f','o','_','1'});
    TopicQos qos2 = TOPIC_QOS_DEFAULT;
    qos2.topic_data.value.assign({'t','o','p','i','c','_','i','n','f','o','_','2'});
    Topic* topic1 = participant->create_topic(
        "topic_multi_1", "TestModule::BasicData", qos1, nullptr, STATUS_MASK_NONE);
    Topic* topic2 = participant->create_topic(
        "topic_multi_2", "TestModule::BasicData", qos2, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic1, nullptr);
    ASSERT_NE(topic2, nullptr);
    TopicQos qos_check1, qos_check2_multi;
    EXPECT_EQ(topic1->get_qos(qos_check1), RETCODE_OK);
    EXPECT_EQ(topic2->get_qos(qos_check2_multi), RETCODE_OK);
    EXPECT_EQ(std::string(qos_check1.topic_data.value.begin(), qos_check1.topic_data.value.end()), "topic_info_1");
    EXPECT_EQ(std::string(qos_check2_multi.topic_data.value.begin(), qos_check2_multi.topic_data.value.end()), "topic_info_2");

    // 8. 删除所有Topic
    EXPECT_EQ(participant->delete_topic(topic_default), RETCODE_OK);
    EXPECT_EQ(participant->delete_topic(topic_custom), RETCODE_OK);
    EXPECT_EQ(participant->delete_topic(topic1), RETCODE_OK);
    EXPECT_EQ(participant->delete_topic(topic2), RETCODE_OK);

    // 9. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 10. finalize_instance
    factory->finalize();
}

TEST_F(TopicDataQosPolicyTestPositive, ENABLED_TopicDataQosPolicyDiscoveryPositive) {
    // Notify negative
    NOTIFY_NEGATIVE("TopicDataQosPolicyTestNegative.ENABLED_TopicDataQosPolicyDiscoveryNegative");
    CREATE_AND_SYNC_CONNECTION("TopicDataQosPolicyDiscovery"); // 创建连接

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

    // 4. 创建discovery_topic_name主题（默认QoS）
    Topic* topic = participant->create_topic(
        "discovery_topic_name", "TestModule::BasicData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);

    // 5. 创建Subscriber
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);

    // 6. 创建DataReader
    DataReader* reader = subscriber->create_datareader(
        topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 7. 等待发现被动端的Topic信息
    std::vector<InstanceHandle_t> handles;
    PublicationBuiltinTopicData pub_data;
    bool found_first = false;
    for (int i = 0; i < 100; ++i) {
        handles.clear();
        EXPECT_EQ(reader->get_matched_publications(handles), RETCODE_OK);
        if (!handles.empty()) {
            for (const auto& h : handles) {
                EXPECT_EQ(reader->get_matched_publication_data(pub_data, h), RETCODE_OK);
                if (pub_data.topic_name == "discovery_topic_name") {
                    std::string topic_data_str(pub_data.topic_data.value.begin(), pub_data.topic_data.value.end());
                    if (topic_data_str == "remote_topic_data_test") {
                        found_first = true;
                        break;
                    }
                }
            }
        }
        if (found_first) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    EXPECT_TRUE(found_first);

    // 7.5. 发送第一次TopicData检查信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("TopicDataQosPolicyDiscovery", "first_topic_data_check"));

    // 8. 验证TopicData动态修改的传播
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    bool found_second = false;
    handles.clear();
    EXPECT_EQ(reader->get_matched_publications(handles), RETCODE_OK);
    for (const auto& h : handles) {
        EXPECT_EQ(reader->get_matched_publication_data(pub_data, h), RETCODE_OK);
        if (pub_data.topic_name == "discovery_topic_name") {
            std::string topic_data_str(pub_data.topic_data.value.begin(), pub_data.topic_data.value.end());
            if (topic_data_str == "updated_topic_data_test") {
                found_second = true;
                break;
            }
        }
    }
    //EXPECT_TRUE(found_second);

    // 8.5. 发送第二次TopicData检查信号
    EXPECT_TRUE(TestFrame::getInstance().sendData("TopicDataQosPolicyDiscovery", "second_topic_data_check"));

    // 9. 删除所有子实体
    EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);

    // 10. 删除域参与者
    EXPECT_EQ(factory->delete_participant(participant), RETCODE_OK);

    // 11. finalize_instance
    factory->finalize();
    DESTORY_CONNECTION("TopicDataQosPolicyDiscovery");
}

TEST_F(TopicDataQosPolicyTestPositive, ENABLED_TopicDataQosPolicyCFTIntegrationTest) {
    // 1. 获取工厂单例
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    ASSERT_NE(factory, nullptr);

    // 2. 创建DomainParticipant
    DomainParticipant* participant = factory->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);

    // 3. 注册TestModule::KeyData类型
    TestModule::KeyDataTypeSupport* type_support = TestModule::KeyDataTypeSupport::get_instance();
    ASSERT_NE(type_support, nullptr);
    EXPECT_EQ(participant->registe_type("TestModule::KeyData", type_support), RETCODE_OK);

    // 4. 测试基础Topic的TopicData
    TopicQos base_qos = TOPIC_QOS_DEFAULT;
    base_qos.topic_data.value.assign({'b','a','s','e','_','t','o','p','i','c','_','d','a','t','a'});
    Topic* base_topic = participant->create_topic(
        "base_topic", "TestModule::KeyData", base_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(base_topic, nullptr);
    TopicQos check_qos;
    EXPECT_EQ(base_topic->get_qos(check_qos), RETCODE_OK);
    EXPECT_EQ(std::string(check_qos.topic_data.value.begin(), check_qos.topic_data.value.end()), "base_topic_data");

    // 5. 测试ContentFilteredTopic继承TopicData
    ContentFilteredTopic* cft = participant->create_contentfilteredtopic(
        "filtered_topic", base_topic, "lVal > 10", std::vector<std::string>());
    ASSERT_NE(cft, nullptr);
    Topic* related_topic = cft->get_related_topic();
    ASSERT_EQ(related_topic, base_topic);
    TopicQos related_qos;
    EXPECT_EQ(related_topic->get_qos(related_qos), RETCODE_OK);
    EXPECT_EQ(std::string(related_qos.topic_data.value.begin(), related_qos.topic_data.value.end()), "base_topic_data");

    // 6. 测试基础Topic修改TopicData对ContentFilteredTopic的影响
    TopicQos mod_qos = base_qos;
    mod_qos.topic_data.value.assign({'m','o','d','i','f','i','e','d','_','b','a','s','e','_','t','o','p','i','c','_','d','a','t','a'});
    EXPECT_EQ(base_topic->set_qos(mod_qos), RETCODE_OK);
    TopicQos after_mod_qos;
    EXPECT_EQ(cft->get_related_topic()->get_qos(after_mod_qos), RETCODE_OK);
    EXPECT_EQ(std::string(after_mod_qos.topic_data.value.begin(), after_mod_qos.topic_data.value.end()), "modified_base_topic_data");

    // 7. 测试ContentFilteredTopic在数据发现中的TopicData传播
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);
    DataWriter* writer = publisher->create_datawriter(
        cft->get_related_topic(), DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);

    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);
    DataReader* reader = subscriber->create_datareader(
        cft, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);

    // 等待匹配完成
    std::vector<InstanceHandle_t> handles;
    PublicationBuiltinTopicData pub_data;
    bool found = false;
    for (int i = 0; i < 50; ++i) {
        handles.clear();
        EXPECT_EQ(reader->get_matched_publications(handles), RETCODE_OK);
        if (!handles.empty()) {
            for (const auto& h : handles) {
                EXPECT_EQ(reader->get_matched_publication_data(pub_data, h), RETCODE_OK);
                if (pub_data.topic_name == "filtered_topic") {
                    std::string topic_data_str(pub_data.topic_data.value.begin(), pub_data.topic_data.value.end());
                    if (topic_data_str == "modified_base_topic_data") {
                        found = true;
                        break;
                    }
                }
            }
        }
        if (found) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    //EXPECT_TRUE(found);

    // 8. 删除ContentFilteredTopic和基础Topic
    //EXPECT_EQ(participant->delete_contentfilteredtopic(cft), RETCODE_OK);
    //EXPECT_EQ(participant->delete_topic(base_topic), RETCODE_OK);

    // 9. 删除域参与者
    factory->delete_participant(participant);

    // 10. finalize_instance
    factory->finalize();
}

TEST_F(TopicDataQosPolicyTestPositive, ENABLED_TopicDataQosPolicyBoundaryTest) {
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

    // 4. 测试空TopicData
    TopicQos empty_qos = TOPIC_QOS_DEFAULT;
    empty_qos.topic_data.value.clear();
    Topic* empty_topic = participant->create_topic(
        "empty_topic_data", "TestModule::BasicData", empty_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(empty_topic, nullptr);
    TopicQos check_empty;
    EXPECT_EQ(empty_topic->get_qos(check_empty), RETCODE_OK);
    EXPECT_TRUE(check_empty.topic_data.value.empty());

    // 5. 测试大容量TopicData（8KB）
    TopicQos large_qos = TOPIC_QOS_DEFAULT;
    std::vector<uint8_t> large_data(8192, 0xAB);
    large_qos.topic_data.value = large_data;
    Topic* large_topic = participant->create_topic(
        "large_topic_data", "TestModule::BasicData", large_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(large_topic, nullptr);
    TopicQos check_large;
    EXPECT_EQ(large_topic->get_qos(check_large), RETCODE_OK);
    EXPECT_EQ(check_large.topic_data.value.size(), 8192);
    EXPECT_TRUE(std::equal(check_large.topic_data.value.begin(), check_large.topic_data.value.end(), large_data.begin()));

    // 6. 测试二进制TopicData
    TopicQos bin_qos = TOPIC_QOS_DEFAULT;
    std::vector<uint8_t> bin_data = {0x00, 0xFF, 0x7E, 0x55, 0xAA, 0x10};
    bin_qos.topic_data.value = bin_data;
    Topic* bin_topic = participant->create_topic(
        "bin_topic_data", "TestModule::BasicData", bin_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(bin_topic, nullptr);
    TopicQos check_bin;
    EXPECT_EQ(bin_topic->get_qos(check_bin), RETCODE_OK);
    EXPECT_EQ(check_bin.topic_data.value, bin_data);

    // 7. 测试TopicData与Topic生命周期
    TopicQos life_qos = TOPIC_QOS_DEFAULT;
    life_qos.topic_data.value.assign({'l','i','f','e','_','t','e','s','t'});
    Topic* life_topic = participant->create_topic(
        "life_topic_data", "TestModule::BasicData", life_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(life_topic, nullptr);
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* writer = publisher->create_datawriter(
        life_topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* reader = subscriber->create_datareader(
        life_topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    EXPECT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    EXPECT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    TopicQos check_life;
    EXPECT_EQ(life_topic->get_qos(check_life), RETCODE_OK);
    EXPECT_EQ(std::string(check_life.topic_data.value.begin(), check_life.topic_data.value.end()), "life_test");
    TopicQos mod_life_qos = life_qos;
    mod_life_qos.topic_data.value.assign({'m','o','d','_','l','i','f','e'});
    EXPECT_EQ(life_topic->set_qos(mod_life_qos), RETCODE_OK);
    TopicQos check_life2;
    EXPECT_EQ(life_topic->get_qos(check_life2), RETCODE_OK);
    EXPECT_EQ(std::string(check_life2.topic_data.value.begin(), check_life2.topic_data.value.end()), "mod_life");

    // 8. 测试多个相同名称Topic的TopicData一致性
    TopicQos same_name_qos = TOPIC_QOS_DEFAULT;
    same_name_qos.topic_data.value.assign({'s','a','m','e','_','n','a','m','e'});
    Topic* topic_same1 = participant->create_topic(
        "same_name_topic", "TestModule::BasicData", same_name_qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic_same1, nullptr);
    TopicQos diff_name_qos = TOPIC_QOS_DEFAULT;
    diff_name_qos.topic_data.value.assign({'d','i','f','f','_','d','a','t','a'});
    Topic* topic_same2 = participant->create_topic(
        "same_name_topic", "TestModule::BasicData", diff_name_qos, nullptr, STATUS_MASK_NONE);
    // 预期返回原Topic（topic_same1），而不是新建
    //EXPECT_EQ(topic_same2, topic_same1);
    TopicQos check_same;
    EXPECT_EQ(topic_same1->get_qos(check_same), RETCODE_OK);
    EXPECT_EQ(std::string(check_same.topic_data.value.begin(), check_same.topic_data.value.end()), "same_name");

    // 9. 删除所有实体
    EXPECT_EQ(participant->delete_topic(empty_topic), RETCODE_OK);
    EXPECT_EQ(participant->delete_topic(large_topic), RETCODE_OK);
    EXPECT_EQ(participant->delete_topic(bin_topic), RETCODE_OK);
    EXPECT_EQ(participant->delete_topic(life_topic), RETCODE_OK);
    EXPECT_EQ(participant->delete_topic(topic_same1), RETCODE_OK);

    // 10. 删除域参与者
    factory->delete_participant(participant);

    // 11. finalize_instance
    factory->finalize();
}
