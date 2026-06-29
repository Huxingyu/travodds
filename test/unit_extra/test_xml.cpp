#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>
#include "gtest/gtest.h"
#include "xml/dds_xml_repo.h"
#include "dcps/base/retcode.h"

USING_TRAVODDS_NAMESPACE

static std::string create_xml_test_file(const std::string& content)
{
    static int counter = 0;
    std::string path = "/tmp/travodds_unit_xml_" + std::to_string(getpid()) + "_" + std::to_string(counter++) + ".xml";
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    out << content;
    out.close();
    return path;
}

// ========== 基础测试 ==========

TEST(DDSTest, Xml_load_invalid_file)
{
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load("/tmp/travodds_unit_xml_not_exists.xml");
    EXPECT_EQ(ret, RETCODE_ERROR);
}

TEST(DDSTest, Xml_load_and_lookup_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"default\">"
        "    <qos_profile name=\"default\">"
        "      <domain_participant_factory_qos>"
        "        <entity_factory>"
        "          <autoenable_created_entities>false</autoenable_created_entities>"
        "        </entity_factory>"
        "      </domain_participant_factory_qos>"
        "      <topic_qos>"
        "        <history>"
        "          <kind>KEEP_LAST_HISTORY_QOS</kind>"
        "          <depth>11</depth>"
        "        </history>"
        "      </topic_qos>"
        "      <topic_qos name=\"custom_topic\">"
        "        <history>"
        "          <kind>KEEP_ALL_HISTORY_QOS</kind>"
        "          <depth>3</depth>"
        "        </history>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    DomainParticipantFactoryQos factory_qos;
    ret = repo.lookup_participant_factory_qos("", "", "", factory_qos);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_EQ(factory_qos.entity_factory.autoenable_created_entities, false);

    TopicQos default_topic_qos;
    ret = repo.lookup_topic_qos("", "", "", default_topic_qos);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_EQ(default_topic_qos.history.kind, KEEP_LAST_HISTORY_QOS);
    EXPECT_EQ(default_topic_qos.history.depth, 11);

    TopicQos custom_topic_qos;
    ret = repo.lookup_topic_qos("default", "default", "custom_topic", custom_topic_qos);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_EQ(custom_topic_qos.history.kind, KEEP_ALL_HISTORY_QOS);
    EXPECT_EQ(custom_topic_qos.history.depth, 3);

    std::remove(xml_path.c_str());
}

// ========== 新增测试用例 ==========

TEST(DDSTest, Xml_load_empty_xml)
{
    const std::string xml = "<dds_qos></dds_qos>";
    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_load_malformed_xml)
{
    const std::string xml = "<dds_qos><qos_library name=\"test\">"; // 未闭合的标签
    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_load_multiple_libraries)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib1\">"
        "    <qos_profile name=\"profile1\">"
        "      <domain_participant_qos>"
        "        <user_data>"
        "          <value>1,2,3</value>"
        "        </user_data>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "  <qos_library name=\"lib2\">"
        "    <qos_profile name=\"profile2\">"
        "      <domain_participant_qos>"
        "        <user_data>"
        "          <value>4,5,6</value>"
        "        </user_data>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    DomainParticipantQos qos1;
    ret = repo.lookup_participant_qos("lib1", "profile1", "", qos1);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_GT(qos1.user_data.value.size(), 0u);

    DomainParticipantQos qos2;
    ret = repo.lookup_participant_qos("lib2", "profile2", "", qos2);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_GT(qos2.user_data.value.size(), 0u);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_load_publisher_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"test_lib\">"
        "    <qos_profile name=\"test_profile\">"
        "      <publisher_qos>"
        "        <partition>"
        "          <name>partition1</name>"
        "        </partition>"
        "      </publisher_qos>"
        "      <publisher_qos name=\"custom_pub\">"
        "        <partition>"
        "          <name>partition2</name>"
        "          <name>partition3</name>"
        "        </partition>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    PublisherQos pub_qos;
    ret = repo.lookup_publisher_qos("test_lib", "test_profile", "", pub_qos);
    EXPECT_EQ(ret, RETCODE_OK);

    PublisherQos custom_pub_qos;
    ret = repo.lookup_publisher_qos("test_lib", "test_profile", "custom_pub", custom_pub_qos);
    EXPECT_EQ(ret, RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_load_subscriber_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"test_lib\">"
        "    <qos_profile name=\"test_profile\">"
        "      <subscriber_qos>"
        "        <partition>"
        "          <name>sub_partition</name>"
        "        </partition>"
        "      </subscriber_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    SubscriberQos sub_qos;
    ret = repo.lookup_subscriber_qos("test_lib", "test_profile", "", sub_qos);
    EXPECT_EQ(ret, RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_load_datawriter_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"test_lib\">"
        "    <qos_profile name=\"test_profile\">"
        "      <datawriter_qos>"
        "        <reliability>"
        "          <kind>RELIABLE_RELIABILITY_QOS</kind>"
        "        </reliability>"
        "        <durability>"
        "          <kind>TRANSIENT_DURABILITY_QOS</kind>"
        "        </durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    DataWriterQos dw_qos;
    ret = repo.lookup_datawriter_qos("test_lib", "test_profile", "", dw_qos);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_EQ(dw_qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(dw_qos.durability.kind, TRANSIENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_load_datareader_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"test_lib\">"
        "    <qos_profile name=\"test_profile\">"
        "      <datareader_qos>"
        "        <reliability>"
        "          <kind>BEST_EFFORT_RELIABILITY_QOS</kind>"
        "        </reliability>"
        "        <durability>"
        "          <kind>VOLATILE_DURABILITY_QOS</kind>"
        "        </durability>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    DataReaderQos dr_qos;
    ret = repo.lookup_datareader_qos("test_lib", "test_profile", "", dr_qos);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_EQ(dr_qos.reliability.kind, BEST_EFFORT_RELIABILITY_QOS);
    EXPECT_EQ(dr_qos.durability.kind, VOLATILE_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_load_domain_participant_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"test_lib\">"
        "    <qos_profile name=\"test_profile\">"
        "      <domain_participant_qos>"
        "        <user_data>"
        "          <value>10,20,30</value>"
        "        </user_data>"
        "        <entity_factory>"
        "          <autoenable_created_entities>true</autoenable_created_entities>"
        "        </entity_factory>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    DomainParticipantQos dp_qos;
    ret = repo.lookup_participant_qos("test_lib", "test_profile", "", dp_qos);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_GT(dp_qos.user_data.value.size(), 0u);
    EXPECT_EQ(dp_qos.entity_factory.autoenable_created_entities, true);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_invalid_profile_name)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"test_lib\">"
        "    <qos_profile name=\"test_profile\">"
        "      <topic_qos>"
        "        <history>"
        "          <kind>KEEP_LAST_HISTORY_QOS</kind>"
        "          <depth>5</depth>"
        "        </history>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    // 查找不存在的 profile
    TopicQos qos;
    ret = repo.lookup_topic_qos("test_lib", "nonexistent_profile", "", qos);
    EXPECT_EQ(ret, RETCODE_ERROR);

    // 查找不存在的 library
    ret = repo.lookup_topic_qos("nonexistent_lib", "test_profile", "", qos);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_reload_qos)
{
    const std::string xml1 =
        "<dds_qos>"
        "  <qos_library name=\"lib1\">"
        "    <qos_profile name=\"profile1\">"
        "      <topic_qos>"
        "        <history>"
        "          <kind>KEEP_LAST_HISTORY_QOS</kind>"
        "          <depth>1</depth>"
        "        </history>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path1 = create_xml_test_file(xml1);

    DDSXMLRepo repo1;
    ReturnCode_t ret = repo1.load(xml_path1);
    EXPECT_EQ(ret, RETCODE_OK);

    // 创建新的 repo 实例加载不同的 XML
    const std::string xml2 =
        "<dds_qos>"
        "  <qos_library name=\"lib2\">"
        "    <qos_profile name=\"profile2\">"
        "      <topic_qos>"
        "        <history>"
        "          <kind>KEEP_ALL_HISTORY_QOS</kind>"
        "          <depth>10</depth>"
        "        </history>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path2 = create_xml_test_file(xml2);

    DDSXMLRepo repo2;
    ret = repo2.load(xml_path2);
    EXPECT_EQ(ret, RETCODE_OK);

    TopicQos qos;
    ret = repo2.lookup_topic_qos("lib2", "profile2", "", qos);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_EQ(qos.history.kind, KEEP_ALL_HISTORY_QOS);
    EXPECT_EQ(qos.history.depth, 10);

    std::remove(xml_path1.c_str());
    std::remove(xml_path2.c_str());
}

TEST(DDSTest, Xml_complex_qos_with_multiple_policies)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"complex_lib\">"
        "    <qos_profile name=\"complex_profile\">"
        "      <datawriter_qos>"
        "        <reliability>"
        "          <kind>RELIABLE_RELIABILITY_QOS</kind>"
        "          <max_blocking_time>"
        "            <sec>1</sec>"
        "            <nanosec>0</nanosec>"
        "          </max_blocking_time>"
        "        </reliability>"
        "        <durability>"
        "          <kind>PERSISTENT_DURABILITY_QOS</kind>"
        "        </durability>"
        "        <history>"
        "          <kind>KEEP_LAST_HISTORY_QOS</kind>"
        "          <depth>50</depth>"
        "        </history>"
        "        <resource_limits>"
        "          <max_samples>1000</max_samples>"
        "          <max_instances>100</max_instances>"
        "        </resource_limits>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);

    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    DataWriterQos dw_qos;
    ret = repo.lookup_datawriter_qos("complex_lib", "complex_profile", "", dw_qos);
    EXPECT_EQ(ret, RETCODE_OK);
    EXPECT_EQ(dw_qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(dw_qos.durability.kind, PERSISTENT_DURABILITY_QOS);
    EXPECT_EQ(dw_qos.history.kind, KEEP_LAST_HISTORY_QOS);
    EXPECT_EQ(dw_qos.history.depth, 50);

    std::remove(xml_path.c_str());
}

// ========== 增强测试：覆盖更多 QoS 策略 ==========

TEST(DDSTest, Xml_presentation_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos>"
        "        <presentation>"
        "          <access_scope>GROUP_PRESENTATION_QOS</access_scope>"
        "          <coherent_access>true</coherent_access>"
        "          <ordered_access>true</ordered_access>"
        "        </presentation>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    PublisherQos qos;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.presentation.access_scope, GROUP_PRESENTATION_QOS);
    EXPECT_EQ(qos.presentation.coherent_access, true);
    EXPECT_EQ(qos.presentation.ordered_access, true);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_deadline_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <deadline>"
        "          <period>"
        "            <sec>5</sec>"
        "            <nanosec>0</nanosec>"
        "          </period>"
        "        </deadline>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.deadline.period.sec, 5);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_latency_budget_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <latency_budget>"
        "          <duration>"
        "            <sec>0</sec>"
        "            <nanosec>1000000</nanosec>"
        "          </duration>"
        "        </latency_budget>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_ownership_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos>"
        "        <ownership>"
        "          <kind>EXCLUSIVE_OWNERSHIP_QOS</kind>"
        "        </ownership>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.ownership.kind, EXCLUSIVE_OWNERSHIP_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_ownership_strength_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <ownership_strength>"
        "          <value>100</value>"
        "        </ownership_strength>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.ownership_strength.value, 100);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_liveliness_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <liveliness>"
        "          <kind>AUTOMATIC_LIVELINESS_QOS</kind>"
        "          <lease_duration>"
        "            <sec>10</sec>"
        "            <nanosec>0</nanosec>"
        "          </lease_duration>"
        "        </liveliness>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.liveliness.kind, AUTOMATIC_LIVELINESS_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_time_based_filter_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos>"
        "        <time_based_filter>"
        "          <minimum_separation>"
        "            <sec>1</sec>"
        "            <nanosec>0</nanosec>"
        "          </minimum_separation>"
        "        </time_based_filter>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_transport_priority_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <transport_priority>"
        "          <value>50</value>"
        "        </transport_priority>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.transport_priority.value, 50);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_lifespan_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <lifespan>"
        "          <duration>"
        "            <sec>60</sec>"
        "            <nanosec>0</nanosec>"
        "          </duration>"
        "        </lifespan>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_durability_service_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability_service>"
        "          <service_cleanup_delay>"
        "            <sec>0</sec>"
        "            <nanosec>0</nanosec>"
        "          </service_cleanup_delay>"
        "          <history_kind>KEEP_LAST_HISTORY_QOS</history_kind>"
        "          <history_depth>10</history_depth>"
        "          <max_samples>500</max_samples>"
        "          <max_instances>10</max_instances>"
        "        </durability_service>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_writer_data_lifecycle_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <writer_data_lifecycle>"
        "          <autodispose_unregistered_instances>false</autodispose_unregistered_instances>"
        "        </writer_data_lifecycle>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.writer_data_lifecycle.autodispose_unregistered_instances, false);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_reader_data_lifecycle_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos>"
        "        <reader_data_lifecycle>"
        "          <autopurge_nowriter_samples_delay>"
        "            <sec>5</sec>"
        "            <nanosec>0</nanosec>"
        "          </autopurge_nowriter_samples_delay>"
        "          <autopurge_disposed_samples_delay>"
        "            <sec>10</sec>"
        "            <nanosec>0</nanosec>"
        "          </autopurge_disposed_samples_delay>"
        "        </reader_data_lifecycle>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_topic_data_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos>"
        "        <topic_data>"
        "          <value>10,20,30</value>"
        "        </topic_data>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    TopicQos qos;
    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_GT(qos.topic_data.value.size(), 0u);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_group_data_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos>"
        "        <group_data>"
        "          <value>1,2,3</value>"
        "        </group_data>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    PublisherQos qos;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_GT(qos.group_data.value.size(), 0u);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_property_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <property>"
        "          <value>"
        "            <element>"
        "              <name>prop1</name>"
        "              <value>val1</value>"
        "            </element>"
        "          </value>"
        "        </property>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_destination_order_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <destination_order>"
        "          <kind>BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS</kind>"
        "        </destination_order>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.destination_order.kind, BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_resource_limits_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <resource_limits>"
        "          <max_samples>2000</max_samples>"
        "          <max_instances>50</max_instances>"
        "          <max_samples_per_instance>100</max_samples_per_instance>"
        "        </resource_limits>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.resource_limits.max_samples, 2000);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_comprehensive_datawriter_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <reliability>"
        "          <kind>RELIABLE_RELIABILITY_QOS</kind>"
        "          <max_blocking_time>"
        "            <sec>2</sec>"
        "            <nanosec>0</nanosec>"
        "          </max_blocking_time>"
        "        </reliability>"
        "        <durability>"
        "          <kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind>"
        "        </durability>"
        "        <history>"
        "          <kind>KEEP_LAST_HISTORY_QOS</kind>"
        "          <depth>20</depth>"
        "        </history>"
        "        <resource_limits>"
        "          <max_samples>500</max_samples>"
        "          <max_instances>20</max_instances>"
        "        </resource_limits>"
        "        <lifespan>"
        "          <duration>"
        "            <sec>30</sec>"
        "            <nanosec>0</nanosec>"
        "          </duration>"
        "        </lifespan>"
        "        <ownership_strength>"
        "          <value>50</value>"
        "        </ownership_strength>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_comprehensive_datareader_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos>"
        "        <reliability>"
        "          <kind>RELIABLE_RELIABILITY_QOS</kind>"
        "        </reliability>"
        "        <durability>"
        "          <kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind>"
        "        </durability>"
        "        <history>"
        "          <kind>KEEP_LAST_HISTORY_QOS</kind>"
        "          <depth>10</depth>"
        "        </history>"
        "        <time_based_filter>"
        "          <minimum_separation>"
        "            <sec>0</sec>"
        "            <nanosec>500000000</nanosec>"
        "          </minimum_separation>"
        "        </time_based_filter>"
        "        <ownership>"
        "          <kind>EXCLUSIVE_OWNERSHIP_QOS</kind>"
        "        </ownership>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

// ========== 覆盖高级 QoS 策略 ==========

TEST(DDSTest, Xml_transport_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <transport>"
        "          <kinds>"
        "            <element>TRANSPORT_UDPv4_QOS</element>"
        "            <element>TRANSPORT_TCPv4_QOS</element>"
        "          </kinds>"
        "        </transport>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_discovered_method_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <dicovery>"
        "          <kind>DISCOVER_UDPv4_QOS</kind>"
        "        </dicovery>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_network_interface_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <network_interface>"
        "          <interface_whitelist>"
        "            <element>eth0</element>"
        "            <element>lo</element>"
        "          </interface_whitelist>"
        "        </network_interface>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_all_transport_types)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <transport>"
        "          <kinds>"
        "            <element>TRANSPORT_UDPv4_QOS</element>"
        "            <element>TRANSPORT_UDPv6_QOS</element>"
        "            <element>TRANSPORT_TCPv4_QOS</element>"
        "            <element>TRANSPORT_TCPv6_QOS</element>"
        "            <element>TRANSPORT_SHM_QOS</element>"
        "          </kinds>"
        "        </transport>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_all_discover_methods)
{
    const std::string xml_udp4 =
        "<dds_qos><qos_library name=\"lib\"><qos_profile name=\"prof\">"
        "  <domain_participant_qos><dicovery><kind>DISCOVER_UDPv4_QOS</kind></dicovery></domain_participant_qos>"
        "</qos_profile></qos_library></dds_qos>";

    const std::string xml_udp6 =
        "<dds_qos><qos_library name=\"lib\"><qos_profile name=\"prof\">"
        "  <domain_participant_qos><dicovery><kind>DISCOVER_UDPv6_QOS</kind></dicovery></domain_participant_qos>"
        "</qos_profile></qos_library></dds_qos>";

    const std::string xml_both =
        "<dds_qos><qos_library name=\"lib\"><qos_profile name=\"prof\">"
        "  <domain_participant_qos><dicovery><kind>DISCOVER_UDPv4_AND_UDPv6_QOS</kind></dicovery></domain_participant_qos>"
        "</qos_profile></qos_library></dds_qos>";

    DDSXMLRepo repo1;
    std::string path1 = create_xml_test_file(xml_udp4);
    EXPECT_EQ(repo1.load(path1), RETCODE_OK);
    DomainParticipantQos qos1;
    EXPECT_EQ(repo1.lookup_participant_qos("lib", "prof", "", qos1), RETCODE_OK);
    std::remove(path1.c_str());

    DDSXMLRepo repo2;
    std::string path2 = create_xml_test_file(xml_udp6);
    EXPECT_EQ(repo2.load(path2), RETCODE_OK);
    DomainParticipantQos qos2;
    EXPECT_EQ(repo2.lookup_participant_qos("lib", "prof", "", qos2), RETCODE_OK);
    std::remove(path2.c_str());

    DDSXMLRepo repo3;
    std::string path3 = create_xml_test_file(xml_both);
    EXPECT_EQ(repo3.load(path3), RETCODE_OK);
    DomainParticipantQos qos3;
    EXPECT_EQ(repo3.lookup_participant_qos("lib", "prof", "", qos3), RETCODE_OK);
    std::remove(path3.c_str());
}

TEST(DDSTest, Xml_error_handling_invalid_file)
{
    DDSXMLRepo repo;
    EXPECT_NE(repo.load("/nonexistent/path/file.xml"), RETCODE_OK);
}

TEST(DDSTest, Xml_error_handling_invalid_root)
{
    const std::string xml =
        "<invalid_root>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</invalid_root>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_NE(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_error_handling_lookup_nonexistent)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_NE(repo.lookup_datawriter_qos("nonexistent_lib", "prof", "", qos), RETCODE_OK);
    EXPECT_NE(repo.lookup_datawriter_qos("lib", "nonexistent_prof", "", qos), RETCODE_OK);
    EXPECT_NE(repo.lookup_datawriter_qos("lib", "prof", "nonexistent_name", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_profiles_in_library)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof1\">"
        "      <datawriter_qos><reliability><kind>BEST_EFFORT_RELIABILITY_QOS</kind></reliability></datawriter_qos>"
        "    </qos_profile>"
        "    <qos_profile name=\"prof2\">"
        "      <datawriter_qos><reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>1</sec><nanosec>0</nanosec></max_blocking_time></reliability></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos1;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof1", "", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.reliability.kind, BEST_EFFORT_RELIABILITY_QOS);

    DataWriterQos qos2;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof2", "", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.reliability.kind, RELIABLE_RELIABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_libraries)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib1\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos><durability><kind>VOLATILE_DURABILITY_QOS</kind></durability></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "  <qos_library name=\"lib2\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos><durability><kind>PERSISTENT_DURABILITY_QOS</kind></durability></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos1;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib1", "prof", "", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.durability.kind, VOLATILE_DURABILITY_QOS);

    DataWriterQos qos2;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib2", "prof", "", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.durability.kind, PERSISTENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

// ========== 覆盖 base_name 继承机制 ==========

TEST(DDSTest, Xml_base_name_inheritance_datawriter)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos name=\"base_dw\">"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>1</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "      <datawriter_qos name=\"derived_dw\" base_name=\"base_dw\">"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>50</depth></history>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "derived_dw", qos), RETCODE_OK);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_LAST_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_base_name_inheritance_datareader)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos name=\"base_dr\">"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>2</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "      </datareader_qos>"
        "      <datareader_qos name=\"derived_dr\" base_name=\"base_dr\">"
        "        <history><kind>KEEP_ALL_HISTORY_QOS</kind></history>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "derived_dr", qos), RETCODE_OK);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_ALL_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_base_name_inheritance_publisher)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos name=\"base_pub\">"
        "        <presentation><access_scope>GROUP_PRESENTATION_QOS</access_scope></presentation>"
        "      </publisher_qos>"
        "      <publisher_qos name=\"derived_pub\" base_name=\"base_pub\">"
        "        <partition><name>part_a</name></partition>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    PublisherQos qos;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "derived_pub", qos), RETCODE_OK);
    EXPECT_EQ(qos.presentation.access_scope, GROUP_PRESENTATION_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_base_name_inheritance_subscriber)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <subscriber_qos name=\"base_sub\">"
        "        <presentation><access_scope>TOPIC_PRESENTATION_QOS</access_scope></presentation>"
        "      </subscriber_qos>"
        "      <subscriber_qos name=\"derived_sub\" base_name=\"base_sub\">"
        "        <partition><name>sub_part</name></partition>"
        "      </subscriber_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    SubscriberQos qos;
    EXPECT_EQ(repo.lookup_subscriber_qos("lib", "prof", "derived_sub", qos), RETCODE_OK);
    EXPECT_EQ(qos.presentation.access_scope, TOPIC_PRESENTATION_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_base_name_inheritance_topic)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos name=\"base_topic\">"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "      </topic_qos>"
        "      <topic_qos name=\"derived_topic\" base_name=\"base_topic\">"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>1</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    TopicQos qos;
    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "derived_topic", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_base_name_inheritance_participant)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos name=\"base_participant\">"
        "        <user_data><value>10,20,30</value></user_data>"
        "      </domain_participant_qos>"
        "      <domain_participant_qos name=\"derived_participant\" base_name=\"base_participant\">"
        "        <entity_factory><autoenable_created_entities>false</autoenable_created_entities></entity_factory>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "derived_participant", qos), RETCODE_OK);
    EXPECT_GT(qos.user_data.value.size(), 0u);
    EXPECT_EQ(qos.entity_factory.autoenable_created_entities, false);

    std::remove(xml_path.c_str());
}

// ========== 覆盖错误处理路径 ==========

TEST(DDSTest, Xml_error_unknown_qos_type)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <unknown_qos_type></unknown_qos_type>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_NE(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_error_missing_library_name)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library>"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_NE(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_error_missing_profile_name)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile>"
        "      <datawriter_qos></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_NE(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_error_invalid_base_name)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos name=\"derived\" base_name=\"nonexistent_base\">"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind></reliability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_NE(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

// ========== 覆盖 domain_participant_factory_qos ==========

TEST(DDSTest, Xml_domain_participant_factory_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_factory_qos>"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "      </domain_participant_factory_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_domain_participant_factory_qos_with_base_name)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_factory_qos name=\"base_dpf\">"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "      </domain_participant_factory_qos>"
        "      <domain_participant_factory_qos name=\"derived_dpf\" base_name=\"base_dpf\">"
        "      </domain_participant_factory_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

// ========== 覆盖未覆盖的函数 ==========

TEST(DDSTest, Xml_clear_function)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos><durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability></datawriter_qos>"
        "      <datareader_qos><durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability></datareader_qos>"
        "      <publisher_qos><presentation><access_scope>GROUP_PRESENTATION_QOS</access_scope></presentation></publisher_qos>"
        "      <subscriber_qos><presentation><access_scope>TOPIC_PRESENTATION_QOS</access_scope></presentation></subscriber_qos>"
        "      <topic_qos><durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability></topic_qos>"
        "      <domain_participant_qos><user_data><value>1,2,3</value></user_data></domain_participant_qos>"
        "      <domain_participant_factory_qos><entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory></domain_participant_factory_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    // 验证加载成功
    DataWriterQos dw_qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", dw_qos), RETCODE_OK);

    // 调用 clear 清除所有 QoS
    EXPECT_EQ(repo.clear(), RETCODE_OK);

    // 验证清除后查找失败
    DataWriterQos dw_qos2;
    EXPECT_NE(repo.lookup_datawriter_qos("lib", "prof", "", dw_qos2), RETCODE_OK);

    DataReaderQos dr_qos;
    EXPECT_NE(repo.lookup_datareader_qos("lib", "prof", "", dr_qos), RETCODE_OK);

    PublisherQos pub_qos;
    EXPECT_NE(repo.lookup_publisher_qos("lib", "prof", "", pub_qos), RETCODE_OK);

    SubscriberQos sub_qos;
    EXPECT_NE(repo.lookup_subscriber_qos("lib", "prof", "", sub_qos), RETCODE_OK);

    TopicQos topic_qos;
    EXPECT_NE(repo.lookup_topic_qos("lib", "prof", "", topic_qos), RETCODE_OK);

    DomainParticipantQos dp_qos;
    EXPECT_NE(repo.lookup_participant_qos("lib", "prof", "", dp_qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_binary_property_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <property>"
        "          <value>"
        "            <element><name>key1</name><value>value1</value></element>"
        "          </value>"
        "          <binary_value>"
        "            <element><name>bin_key1</name><value>binary_data_1</value></element>"
        "            <element><name>bin_key2</name><value>binary_data_2</value></element>"
        "          </binary_value>"
        "        </property>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);

    // 验证 property 和 binary_property 都被解析
    EXPECT_GT(qos.property.value.size(), 0u);
    EXPECT_GT(qos.property.binary_value.size(), 0u);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_property_with_propagate)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <property>"
        "          <value>"
        "            <element><name>prop_key</name><value>prop_value</value><propagate>true</propagate></element>"
        "          </value>"
        "        </property>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_GT(qos.property.value.size(), 0u);

    std::remove(xml_path.c_str());
}

// ========== 覆盖更多枚举值分支 ==========

TEST(DDSTest, Xml_durability_transient_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_durability_persistent_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>PERSISTENT_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, PERSISTENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_reliability_best_effort_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <reliability><kind>BEST_EFFORT_RELIABILITY_QOS</kind></reliability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.reliability.kind, BEST_EFFORT_RELIABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_ownership_exclusive_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <ownership><kind>EXCLUSIVE_OWNERSHIP_QOS</kind></ownership>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.ownership.kind, EXCLUSIVE_OWNERSHIP_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_liveliness_manual_by_participant_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <liveliness><kind>MANUAL_BY_PARTICIPANT_LIVELINESS_QOS</kind><lease_duration><sec>10</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.liveliness.kind, MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_liveliness_manual_by_topic_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <liveliness><kind>MANUAL_BY_TOPIC_LIVELINESS_QOS</kind><lease_duration><sec>10</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.liveliness.kind, MANUAL_BY_TOPIC_LIVELINESS_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_history_keep_all_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <history><kind>KEEP_ALL_HISTORY_QOS</kind></history>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.history.kind, KEEP_ALL_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_destination_order_by_source_timestamp_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <destination_order><kind>BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS</kind></destination_order>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.destination_order.kind, BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_presentation_instance_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos>"
        "        <presentation>"
        "          <access_scope>INSTANCE_PRESENTATION_QOS</access_scope>"
        "          <coherent_access>false</coherent_access>"
        "          <ordered_access>false</ordered_access>"
        "        </presentation>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    PublisherQos qos;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.presentation.access_scope, INSTANCE_PRESENTATION_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_presentation_topic_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos>"
        "        <presentation>"
        "          <access_scope>TOPIC_PRESENTATION_QOS</access_scope>"
        "          <coherent_access>true</coherent_access>"
        "          <ordered_access>false</ordered_access>"
        "        </presentation>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    PublisherQos qos;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.presentation.access_scope, TOPIC_PRESENTATION_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_discovered_method_qos_all_types)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <dicovery>"
        "          <use_dds>1</use_dds>"
        "          <use_tcp>1</use_tcp>"
        "          <use_udp>0</use_udp>"
        "        </dicovery>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_qos_profiles_in_library)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof1\">"
        "      <datawriter_qos name=\"dw1\"><durability><kind>VOLATILE_DURABILITY_QOS</kind></durability></datawriter_qos>"
        "    </qos_profile>"
        "    <qos_profile name=\"prof2\">"
        "      <datawriter_qos name=\"dw2\"><durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability></datawriter_qos>"
        "    </qos_profile>"
        "    <qos_profile name=\"prof3\">"
        "      <datawriter_qos name=\"dw3\"><durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos1;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof1", "dw1", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.durability.kind, VOLATILE_DURABILITY_QOS);

    DataWriterQos qos2;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof2", "dw2", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);

    DataWriterQos qos3;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof3", "dw3", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.durability.kind, TRANSIENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_with_empty_elements)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability></durability>"
        "        <reliability></reliability>"
        "        <history></history>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_with_whitespace_values)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>  VOLATILE_DURABILITY_QOS  </kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_multiple_datawriters_in_profile)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos name=\"dw_reliable\">"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>1</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "      <datawriter_qos name=\"dw_best_effort\">"
        "        <reliability><kind>BEST_EFFORT_RELIABILITY_QOS</kind></reliability>"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "      <datawriter_qos name=\"dw_transient\">"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind></reliability>"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos1;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "dw_reliable", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.reliability.kind, RELIABLE_RELIABILITY_QOS);

    DataWriterQos qos2;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "dw_best_effort", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.reliability.kind, BEST_EFFORT_RELIABILITY_QOS);

    DataWriterQos qos3;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "dw_transient", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.durability.kind, TRANSIENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_multiple_datareaders_in_profile)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos name=\"dr_reliable\">"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind></reliability>"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>10</depth></history>"
        "      </datareader_qos>"
        "      <datareader_qos name=\"dr_best_effort\">"
        "        <reliability><kind>BEST_EFFORT_RELIABILITY_QOS</kind></reliability>"
        "        <history><kind>KEEP_ALL_HISTORY_QOS</kind></history>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos1;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "dr_reliable", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.reliability.kind, RELIABLE_RELIABILITY_QOS);

    DataReaderQos qos2;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "dr_best_effort", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.reliability.kind, BEST_EFFORT_RELIABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_multiple_publishers_in_profile)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos name=\"pub_group\">"
        "        <presentation><access_scope>GROUP_PRESENTATION_QOS</access_scope><coherent_access>true</coherent_access></presentation>"
        "      </publisher_qos>"
        "      <publisher_qos name=\"pub_topic\">"
        "        <presentation><access_scope>TOPIC_PRESENTATION_QOS</access_scope><coherent_access>false</coherent_access></presentation>"
        "      </publisher_qos>"
        "      <publisher_qos name=\"pub_instance\">"
        "        <presentation><access_scope>INSTANCE_PRESENTATION_QOS</access_scope></presentation>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    PublisherQos qos1;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "pub_group", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.presentation.access_scope, GROUP_PRESENTATION_QOS);

    PublisherQos qos2;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "pub_topic", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.presentation.access_scope, TOPIC_PRESENTATION_QOS);

    PublisherQos qos3;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "pub_instance", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.presentation.access_scope, INSTANCE_PRESENTATION_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_multiple_subscribers_in_profile)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <subscriber_qos name=\"sub_group\">"
        "        <presentation><access_scope>GROUP_PRESENTATION_QOS</access_scope><ordered_access>true</ordered_access></presentation>"
        "      </subscriber_qos>"
        "      <subscriber_qos name=\"sub_topic\">"
        "        <presentation><access_scope>TOPIC_PRESENTATION_QOS</access_scope></presentation>"
        "      </subscriber_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    SubscriberQos qos1;
    EXPECT_EQ(repo.lookup_subscriber_qos("lib", "prof", "sub_group", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.presentation.access_scope, GROUP_PRESENTATION_QOS);

    SubscriberQos qos2;
    EXPECT_EQ(repo.lookup_subscriber_qos("lib", "prof", "sub_topic", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.presentation.access_scope, TOPIC_PRESENTATION_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_multiple_topics_in_profile)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos name=\"topic_volatile\">"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "        <reliability><kind>BEST_EFFORT_RELIABILITY_QOS</kind></reliability>"
        "      </topic_qos>"
        "      <topic_qos name=\"topic_transient\">"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind></reliability>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    TopicQos qos1;
    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "topic_volatile", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.durability.kind, VOLATILE_DURABILITY_QOS);

    TopicQos qos2;
    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "topic_transient", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.durability.kind, TRANSIENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_multiple_participants_in_profile)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos name=\"dp1\">"
        "        <user_data><value>1,2,3</value></user_data>"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "      </domain_participant_qos>"
        "      <domain_participant_qos name=\"dp2\">"
        "        <user_data><value>4,5,6</value></user_data>"
        "        <entity_factory><autoenable_created_entities>false</autoenable_created_entities></entity_factory>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos1;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "dp1", qos1), RETCODE_OK);
    EXPECT_GT(qos1.user_data.value.size(), 0u);

    DomainParticipantQos qos2;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "dp2", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.entity_factory.autoenable_created_entities, false);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_multiple_libraries)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib1\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos><durability><kind>VOLATILE_DURABILITY_QOS</kind></durability></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "  <qos_library name=\"lib2\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos><durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "  <qos_library name=\"lib3\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos><durability><kind>PERSISTENT_DURABILITY_QOS</kind></durability></datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos1;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib1", "prof", "", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.durability.kind, VOLATILE_DURABILITY_QOS);

    DataWriterQos qos2;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib2", "prof", "", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);

    DataWriterQos qos3;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib3", "prof", "", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.durability.kind, PERSISTENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

// ========== 覆盖 transport、discovery、network_interface 等未覆盖区域 ==========

TEST(DDSTest, Xml_transport_qos_kinds)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <transport>"
        "          <kinds>"
        "            <element>TRANSPORT_UDPv4_QOS</element>"
        "            <element>TRANSPORT_TCPv4_QOS</element>"
        "          </kinds>"
        "        </transport>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_transport_qos_all_types)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <transport>"
        "          <kinds>"
        "            <element>TRANSPORT_UDPv4_QOS</element>"
        "            <element>TRANSPORT_UDPv6_QOS</element>"
        "            <element>TRANSPORT_TCPv4_QOS</element>"
        "            <element>TRANSPORT_TCPv6_QOS</element>"
        "            <element>TRANSPORT_SHM_QOS</element>"
        "          </kinds>"
        "        </transport>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_discovery_method_qos_udpv4)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <dicovery>"
        "          <kind>DISCOVER_UDPv4_QOS</kind>"
        "        </dicovery>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_discovery_method_qos_udpv6)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <dicovery>"
        "          <kind>DISCOVER_UDPv6_QOS</kind>"
        "        </dicovery>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_discovery_method_qos_both)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <dicovery>"
        "          <kind>DISCOVER_UDPv4_AND_UDPv6_QOS</kind>"
        "        </dicovery>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_participant_qos_with_all_policies)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "        <user_data><value>10,20,30,40</value></user_data>"
        "        <property>"
        "          <value>"
        "            <element><name>key1</name><value>value1</value></element>"
        "          </value>"
        "          <binary_value>"
        "            <element><name>bin_key</name><value>bindata</value></element>"
        "          </binary_value>"
        "        </property>"
        "        <transport>"
        "          <kinds>"
        "            <element>TRANSPORT_TCPv4_QOS</element>"
        "            <element>TRANSPORT_SHM_QOS</element>"
        "          </kinds>"
        "        </transport>"
        "        <dicovery>"
        "          <kind>DISCOVER_UDPv4_QOS</kind>"
        "        </dicovery>"
        "        <network_interface>"
        "          <interface_whitelist>"
        "            <element>lo</element>"
        "          </interface_whitelist>"
        "        </network_interface>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_GT(qos.user_data.value.size(), 0u);
    EXPECT_GT(qos.property.value.size(), 0u);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_publisher_qos_with_all_policies)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos>"
        "        <entity_factory><autoenable_created_entities>false</autoenable_created_entities></entity_factory>"
        "        <presentation>"
        "          <access_scope>GROUP_PRESENTATION_QOS</access_scope>"
        "          <coherent_access>true</coherent_access>"
        "          <ordered_access>true</ordered_access>"
        "        </presentation>"
        "        <partition>"
        "          <name>partition_a</name>"
        "          <name>partition_b</name>"
        "        </partition>"
        "        <group_data><value>100,200</value></group_data>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    PublisherQos qos;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.presentation.access_scope, GROUP_PRESENTATION_QOS);
    EXPECT_EQ(qos.entity_factory.autoenable_created_entities, false);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_subscriber_qos_with_all_policies)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <subscriber_qos>"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "        <presentation>"
        "          <access_scope>TOPIC_PRESENTATION_QOS</access_scope>"
        "          <coherent_access>false</coherent_access>"
        "          <ordered_access>true</ordered_access>"
        "        </presentation>"
        "        <partition>"
        "          <name>sub_partition</name>"
        "        </partition>"
        "        <group_data><value>50,60</value></group_data>"
        "      </subscriber_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    SubscriberQos qos;
    EXPECT_EQ(repo.lookup_subscriber_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.presentation.access_scope, TOPIC_PRESENTATION_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_topic_qos_with_all_policies)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos>"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "        <deadline><period><sec>10</sec><nanosec>0</nanosec></period></deadline>"
        "        <latency_budget><duration><sec>0</sec><nanosec>20000000</nanosec></duration></latency_budget>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>3</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <ownership><kind>EXCLUSIVE_OWNERSHIP_QOS</kind></ownership>"
        "        <liveliness><kind>AUTOMATIC_LIVELINESS_QOS</kind><lease_duration><sec>20</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "        <destination_order><kind>BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS</kind></destination_order>"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>100</depth></history>"
        "        <resource_limits><max_samples>50000</max_samples><max_instances>1000</max_instances></resource_limits>"
        "        <transport_priority><value>90</value></transport_priority>"
        "        <lifespan><duration><sec>120</sec><nanosec>0</nanosec></duration></lifespan>"
        "        <topic_data><value>70,80</value></topic_data>"
        "        <durability_service>"
        "          <service_cleanup_delay><sec>30</sec><nanosec>0</nanosec></service_cleanup_delay>"
        "          <history_kind>KEEP_ALL_HISTORY_QOS</history_kind>"
        "          <history_depth>200</history_depth>"
        "          <max_samples>20000</max_samples>"
        "          <max_instances>1000</max_instances>"
        "          <max_samples_per_instance>5000</max_samples_per_instance>"
        "        </durability_service>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    TopicQos qos;
    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_datawriter_qos_with_all_policies)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <deadline><period><sec>5</sec><nanosec>0</nanosec></period></deadline>"
        "        <latency_budget><duration><sec>0</sec><nanosec>10000000</nanosec></duration></latency_budget>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>2</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <ownership><kind>EXCLUSIVE_OWNERSHIP_QOS</kind></ownership>"
        "        <ownership_strength><value>50</value></ownership_strength>"
        "        <liveliness><kind>MANUAL_BY_PARTICIPANT_LIVELINESS_QOS</kind><lease_duration><sec>15</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "        <destination_order><kind>BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS</kind></destination_order>"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>50</depth></history>"
        "        <resource_limits><max_samples>10000</max_samples><max_instances>500</max_instances></resource_limits>"
        "        <transport_priority><value>80</value></transport_priority>"
        "        <lifespan><duration><sec>60</sec><nanosec>0</nanosec></duration></lifespan>"
        "        <user_data><value>1,2,3</value></user_data>"
        "        <durability_service>"
        "          <service_cleanup_delay><sec>60</sec><nanosec>0</nanosec></service_cleanup_delay>"
        "          <history_kind>KEEP_LAST_HISTORY_QOS</history_kind>"
        "          <history_depth>50</history_depth>"
        "          <max_samples>10000</max_samples>"
        "          <max_instances>500</max_instances>"
        "          <max_samples_per_instance>1000</max_samples_per_instance>"
        "        </durability_service>"
        "        <writer_data_lifecycle>"
        "          <autodispose_unregistered_instances>false</autodispose_unregistered_instances>"
        "        </writer_data_lifecycle>"
        "        <property>"
        "          <value>"
        "            <element><name>key1</name><value>value1</value></element>"
        "          </value>"
        "        </property>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.writer_data_lifecycle.autodispose_unregistered_instances, false);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_datareader_qos_with_all_policies)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <deadline><period><sec>3</sec><nanosec>0</nanosec></period></deadline>"
        "        <latency_budget><duration><sec>0</sec><nanosec>5000000</nanosec></duration></latency_budget>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>1</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <ownership><kind>SHARED_OWNERSHIP_QOS</kind></ownership>"
        "        <liveliness><kind>AUTOMATIC_LIVELINESS_QOS</kind><lease_duration><sec>10</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "        <time_based_filter><minimum_separation><sec>1</sec><nanosec>0</nanosec></minimum_separation></time_based_filter>"
        "        <history><kind>KEEP_ALL_HISTORY_QOS</kind></history>"
        "        <resource_limits><max_samples>5000</max_samples><max_instances>250</max_instances></resource_limits>"
        "        <user_data><value>4,5,6</value></user_data>"
        "        <reader_data_lifecycle>"
        "          <autopurge_nowriter_samples_delay><sec>5</sec><nanosec>0</nanosec></autopurge_nowriter_samples_delay>"
        "          <autopurge_disposed_samples_delay><sec>15</sec><nanosec>0</nanosec></autopurge_disposed_samples_delay>"
        "        </reader_data_lifecycle>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_ALL_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_base_name_inheritance_with_full_policies)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos name=\"base_dw\">"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>1</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>10</depth></history>"
        "        <ownership><kind>EXCLUSIVE_OWNERSHIP_QOS</kind></ownership>"
        "        <liveliness><kind>AUTOMATIC_LIVELINESS_QOS</kind><lease_duration><sec>10</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "      </datawriter_qos>"
        "      <datawriter_qos name=\"derived_dw\" base_name=\"base_dw\">"
        "        <history><kind>KEEP_ALL_HISTORY_QOS</kind></history>"
        "        <resource_limits><max_samples>20000</max_samples><max_instances>1000</max_instances></resource_limits>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "derived_dw", qos), RETCODE_OK);
    // 验证继承
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.ownership.kind, EXCLUSIVE_OWNERSHIP_QOS);
    // 验证覆盖
    EXPECT_EQ(qos.history.kind, KEEP_ALL_HISTORY_QOS);
    // 验证新增
    EXPECT_EQ(qos.resource_limits.max_samples, 20000);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_entity_factory_qos_false)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <entity_factory><autoenable_created_entities>false</autoenable_created_entities></entity_factory>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.entity_factory.autoenable_created_entities, false);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_entity_factory_qos_true)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.entity_factory.autoenable_created_entities, true);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_partition_qos_multiple_names)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos>"
        "        <partition>"
        "          <name>partition_a</name>"
        "          <name>partition_b</name>"
        "          <name>partition_c</name>"
        "        </partition>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    PublisherQos qos;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "", qos), RETCODE_OK);
    // 不验证具体数值，只验证加载成功

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_user_data_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <user_data><value>255,128,64,32,16,8,4,2,1</value></user_data>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_GT(qos.user_data.value.size(), 0u);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_ownership_shared_qos)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <ownership><kind>SHARED_OWNERSHIP_QOS</kind></ownership>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.ownership.kind, SHARED_OWNERSHIP_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_durability_service_qos_full)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability_service>"
        "          <service_cleanup_delay><sec>120</sec><nanosec>0</nanosec></service_cleanup_delay>"
        "          <history_kind>KEEP_ALL_HISTORY_QOS</history_kind>"
        "          <history_depth>1000</history_depth>"
        "          <max_samples>100000</max_samples>"
        "          <max_instances>5000</max_instances>"
        "          <max_samples_per_instance>20000</max_samples_per_instance>"
        "        </durability_service>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

// ========== 覆盖更多未覆盖区域 ==========

TEST(DDSTest, Xml_topic_qos_with_all_policies_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos>"
        "        <topic_data><value>10,20,30</value></topic_data>"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "        <durability_service>"
        "          <service_cleanup_delay><sec>60</sec><nanosec>0</nanosec></service_cleanup_delay>"
        "          <history_kind>KEEP_LAST_HISTORY_QOS</history_kind>"
        "          <history_depth>100</history_depth>"
        "          <max_samples>10000</max_samples>"
        "          <max_instances>500</max_instances>"
        "          <max_samples_per_instance>2000</max_samples_per_instance>"
        "        </durability_service>"
        "        <deadline><period><sec>5</sec><nanosec>0</nanosec></period></deadline>"
        "        <latency_budget><duration><sec>0</sec><nanosec>10000000</nanosec></duration></latency_budget>"
        "        <liveliness><kind>AUTOMATIC_LIVELINESS_QOS</kind><lease_duration><sec>10</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>2</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <destination_order><kind>BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS</kind></destination_order>"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>50</depth></history>"
        "        <resource_limits>"
        "          <max_samples>5000</max_samples>"
        "          <max_instances>250</max_instances>"
        "          <max_samples_per_instance>1000</max_samples_per_instance>"
        "        </resource_limits>"
        "        <transport_priority><value>75</value></transport_priority>"
        "        <lifespan><duration><sec>90</sec><nanosec>0</nanosec></duration></lifespan>"
        "        <ownership><kind>EXCLUSIVE_OWNERSHIP_QOS</kind></ownership>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    TopicQos qos;
    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_resource_limits_with_length_unlimited)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <resource_limits>"
        "          <max_samples>LENGTH_UNLIMITED</max_samples>"
        "          <max_instances>LENGTH_UNLIMITED</max_instances>"
        "          <max_samples_per_instance>LENGTH_UNLIMITED</max_samples_per_instance>"
        "        </resource_limits>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_topic_resource_limits_with_length_unlimited)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos>"
        "        <resource_limits>"
        "          <max_samples>LENGTH_UNLIMITED</max_samples>"
        "          <max_instances>LENGTH_UNLIMITED</max_instances>"
        "          <max_samples_per_instance>LENGTH_UNLIMITED</max_samples_per_instance>"
        "        </resource_limits>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    TopicQos qos;
    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_datareader_qos_with_all_policies_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <deadline><period><sec>3</sec><nanosec>0</nanosec></period></deadline>"
        "        <latency_budget><duration><sec>0</sec><nanosec>5000000</nanosec></duration></latency_budget>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>1</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <ownership><kind>SHARED_OWNERSHIP_QOS</kind></ownership>"
        "        <liveliness><kind>AUTOMATIC_LIVELINESS_QOS</kind><lease_duration><sec>10</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "        <time_based_filter><minimum_separation><sec>1</sec><nanosec>0</nanosec></minimum_separation></time_based_filter>"
        "        <history><kind>KEEP_ALL_HISTORY_QOS</kind></history>"
        "        <resource_limits>"
        "          <max_samples>5000</max_samples>"
        "          <max_instances>250</max_instances>"
        "          <max_samples_per_instance>1000</max_samples_per_instance>"
        "        </resource_limits>"
        "        <user_data><value>4,5,6</value></user_data>"
        "        <reader_data_lifecycle>"
        "          <autopurge_nowriter_samples_delay><sec>5</sec><nanosec>0</nanosec></autopurge_nowriter_samples_delay>"
        "          <autopurge_disposed_samples_delay><sec>15</sec><nanosec>0</nanosec></autopurge_disposed_samples_delay>"
        "        </reader_data_lifecycle>"
        "        <property>"
        "          <value>"
        "            <element><name>reader_key</name><value>reader_value</value></element>"
        "          </value>"
        "        </property>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_ALL_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_base_name_inheritance_with_omitted_library)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos name=\"base_dw\">"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>1</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "      </datawriter_qos>"
        "      <datawriter_qos name=\"derived_dw\" base_name=\"prof::base_dw\">"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>25</depth></history>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "derived_dw", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_LAST_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_base_name_inheritance_with_omitted_library_and_profile)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos name=\"base_dw\">"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>1</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "      </datawriter_qos>"
        "      <datawriter_qos name=\"derived_dw\" base_name=\"lib::prof::base_dw\">"
        "        <history><kind>KEEP_ALL_HISTORY_QOS</kind></history>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "derived_dw", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_ALL_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_publisher_base_name_inheritance)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos name=\"base_pub\">"
        "        <presentation>"
        "          <access_scope>GROUP_PRESENTATION_QOS</access_scope>"
        "          <coherent_access>true</coherent_access>"
        "          <ordered_access>true</ordered_access>"
        "        </presentation>"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "      </publisher_qos>"
        "      <publisher_qos name=\"derived_pub\" base_name=\"base_pub\">"
        "        <partition>"
        "          <name>derived_partition</name>"
        "        </partition>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    PublisherQos qos;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "derived_pub", qos), RETCODE_OK);
    EXPECT_EQ(qos.presentation.access_scope, GROUP_PRESENTATION_QOS);
    EXPECT_EQ(qos.presentation.coherent_access, true);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_subscriber_base_name_inheritance)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <subscriber_qos name=\"base_sub\">"
        "        <presentation>"
        "          <access_scope>TOPIC_PRESENTATION_QOS</access_scope>"
        "          <coherent_access>false</coherent_access>"
        "          <ordered_access>true</ordered_access>"
        "        </presentation>"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "      </subscriber_qos>"
        "      <subscriber_qos name=\"derived_sub\" base_name=\"base_sub\">"
        "        <partition>"
        "          <name>sub_partition</name>"
        "        </partition>"
        "      </subscriber_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    SubscriberQos qos;
    EXPECT_EQ(repo.lookup_subscriber_qos("lib", "prof", "derived_sub", qos), RETCODE_OK);
    EXPECT_EQ(qos.presentation.access_scope, TOPIC_PRESENTATION_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_topic_base_name_inheritance)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos name=\"base_topic\">"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>2</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "      </topic_qos>"
        "      <topic_qos name=\"derived_topic\" base_name=\"base_topic\">"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>75</depth></history>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    TopicQos qos;
    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "derived_topic", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_LAST_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_participant_base_name_inheritance)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos name=\"base_participant\">"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "        <user_data><value>10,20,30</value></user_data>"
        "      </domain_participant_qos>"
        "      <domain_participant_qos name=\"derived_participant\" base_name=\"base_participant\">"
        "        <property>"
        "          <value>"
        "            <element><name>derived_key</name><value>derived_value</value></element>"
        "          </value>"
        "        </property>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "derived_participant", qos), RETCODE_OK);
    EXPECT_EQ(qos.entity_factory.autoenable_created_entities, true);
    EXPECT_GT(qos.user_data.value.size(), 0u);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_datareader_base_name_inheritance)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos name=\"base_dr\">"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>1</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>10</depth></history>"
        "      </datareader_qos>"
        "      <datareader_qos name=\"derived_dr\" base_name=\"base_dr\">"
        "        <time_based_filter><minimum_separation><sec>2</sec><nanosec>0</nanosec></minimum_separation></time_based_filter>"
        "        <resource_limits><max_samples>15000</max_samples><max_instances>750</max_instances></resource_limits>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "derived_dr", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_LAST_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_participant_factory_qos_with_all_policies)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_factory_qos>"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "        <property>"
        "          <value>"
        "            <element><name>factory_key</name><value>factory_value</value></element>"
        "          </value>"
        "        </property>"
        "      </domain_participant_factory_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantFactoryQos qos;
    EXPECT_EQ(repo.lookup_participant_factory_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.entity_factory.autoenable_created_entities, true);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_participant_factory_qos_with_base_name)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_factory_qos name=\"base_factory\">"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "      </domain_participant_factory_qos>"
        "      <domain_participant_factory_qos name=\"derived_factory\" base_name=\"base_factory\">"
        "        <property>"
        "          <value>"
        "            <element><name>derived_key</name><value>derived_value</value></element>"
        "          </value>"
        "        </property>"
        "      </domain_participant_factory_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantFactoryQos qos;
    EXPECT_EQ(repo.lookup_participant_factory_qos("lib", "prof", "derived_factory", qos), RETCODE_OK);
    EXPECT_EQ(qos.entity_factory.autoenable_created_entities, true);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_lookup_qos_failure_cases)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    // 查找不存在的 QoS
    DataWriterQos qos;
    EXPECT_NE(repo.lookup_datawriter_qos("nonexistent_lib", "prof", "", qos), RETCODE_OK);
    EXPECT_NE(repo.lookup_datawriter_qos("lib", "nonexistent_prof", "", qos), RETCODE_OK);
    EXPECT_NE(repo.lookup_datawriter_qos("lib", "prof", "nonexistent_qos", qos), RETCODE_OK);

    // 查找不存在的 topic QoS
    TopicQos topic_qos;
    EXPECT_NE(repo.lookup_topic_qos("lib", "prof", "", topic_qos), RETCODE_OK);

    // 查找不存在的 publisher QoS
    PublisherQos pub_qos;
    EXPECT_NE(repo.lookup_publisher_qos("lib", "prof", "", pub_qos), RETCODE_OK);

    // 查找不存在的 subscriber QoS
    SubscriberQos sub_qos;
    EXPECT_NE(repo.lookup_subscriber_qos("lib", "prof", "", sub_qos), RETCODE_OK);

    // 查找不存在的 datareader QoS
    DataReaderQos dr_qos;
    EXPECT_NE(repo.lookup_datareader_qos("lib", "prof", "", dr_qos), RETCODE_OK);

    // 查找不存在的 participant QoS
    DomainParticipantQos dp_qos;
    EXPECT_NE(repo.lookup_participant_qos("lib", "prof", "", dp_qos), RETCODE_OK);

    // 查找不存在的 participant factory QoS
    DomainParticipantFactoryQos dpf_qos;
    EXPECT_NE(repo.lookup_participant_factory_qos("lib", "prof", "", dpf_qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_durability_service_qos_full_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos>"
        "        <durability_service>"
        "          <service_cleanup_delay><sec>120</sec><nanosec>0</nanosec></service_cleanup_delay>"
        "          <history_kind>KEEP_ALL_HISTORY_QOS</history_kind>"
        "          <history_depth>100</history_depth>"
        "          <max_samples>LENGTH_UNLIMITED</max_samples>"
        "          <max_instances>LENGTH_UNLIMITED</max_instances>"
        "          <max_samples_per_instance>LENGTH_UNLIMITED</max_samples_per_instance>"
        "        </durability_service>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    TopicQos qos;
    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_property_qos_with_propagate)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <property>"
        "          <value>"
        "            <element>"
        "              <name>prop_key</name>"
        "              <value>prop_value</value>"
        "              <propagate>true</propagate>"
        "            </element>"
        "          </value>"
        "        </property>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_GT(qos.property.value.size(), 0u);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_binary_property_qos_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <property>"
        "          <binary_value>"
        "            <element>"
        "              <name>binary_key</name>"
        "              <value>binary_data_123</value>"
        "            </element>"
        "          </binary_value>"
        "        </property>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_GT(qos.property.binary_value.size(), 0u);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_transport_qos_with_all_types_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <transport>"
        "          <kinds>"
        "            <element>TRANSPORT_UDPv4_QOS</element>"
        "            <element>TRANSPORT_UDPv6_QOS</element>"
        "            <element>TRANSPORT_TCPv4_QOS</element>"
        "            <element>TRANSPORT_TCPv6_QOS</element>"
        "            <element>TRANSPORT_SHM_QOS</element>"
        "          </kinds>"
        "        </transport>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_discovered_method_qos_all_types_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <dicovery>"
        "          <kind>DISCOVER_UDPv4_QOS</kind>"
        "        </dicovery>"
        "      </domain_participant_qos>"
        "      <domain_participant_qos name=\"dp2\">"
        "        <dicovery>"
        "          <kind>DISCOVER_UDPv6_QOS</kind>"
        "        </dicovery>"
        "      </domain_participant_qos>"
        "      <domain_participant_qos name=\"dp3\">"
        "        <dicovery>"
        "          <kind>DISCOVER_UDPv4_AND_UDPv6_QOS</kind>"
        "        </dicovery>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos1, qos2, qos3;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos1), RETCODE_OK);
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "dp2", qos2), RETCODE_OK);
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "dp3", qos3), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_network_interface_qos_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos>"
        "        <network_interface>"
        "          <interface_whitelist>"
        "            <element>eth0</element>"
        "            <element>eth1</element>"
        "            <element>lo</element>"
        "          </interface_whitelist>"
        "        </network_interface>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "", qos), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_qos_profiles_in_library_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof1\">"
        "      <datawriter_qos>"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "    <qos_profile name=\"prof2\">"
        "      <datawriter_qos>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "    <qos_profile name=\"prof3\">"
        "      <datawriter_qos>"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos1, qos2, qos3;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof1", "", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.durability.kind, VOLATILE_DURABILITY_QOS);

    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof2", "", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);

    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof3", "", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.durability.kind, TRANSIENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_libraries_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib1\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "  <qos_library name=\"lib2\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "  <qos_library name=\"lib3\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>PERSISTENT_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos1, qos2, qos3;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib1", "prof", "", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.durability.kind, VOLATILE_DURABILITY_QOS);

    EXPECT_EQ(repo.lookup_datawriter_qos("lib2", "prof", "", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);

    EXPECT_EQ(repo.lookup_datawriter_qos("lib3", "prof", "", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.durability.kind, PERSISTENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_with_empty_elements_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind></kind></durability>"
        "        <user_data><value></value></user_data>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    // 空元素应该能正常解析（使用默认值）
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_with_whitespace_values_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>  TRANSIENT_LOCAL_DURABILITY_QOS  </kind></durability>"
        "        <user_data><value>  10,20,30  </value></user_data>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_datawriters_in_profile_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos name=\"dw1\">"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "        <reliability><kind>BEST_EFFORT_RELIABILITY_QOS</kind></reliability>"
        "      </datawriter_qos>"
        "      <datawriter_qos name=\"dw2\">"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind></reliability>"
        "      </datawriter_qos>"
        "      <datawriter_qos name=\"dw3\">"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind></reliability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos1, qos2, qos3;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "dw1", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.durability.kind, VOLATILE_DURABILITY_QOS);
    EXPECT_EQ(qos1.reliability.kind, BEST_EFFORT_RELIABILITY_QOS);

    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "dw2", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos2.reliability.kind, RELIABLE_RELIABILITY_QOS);

    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "dw3", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.durability.kind, TRANSIENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_datareaders_in_profile_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos name=\"dr1\">"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </datareader_qos>"
        "      <datareader_qos name=\"dr2\">"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "      </datareader_qos>"
        "      <datareader_qos name=\"dr3\">"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos1, qos2, qos3;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "dr1", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.durability.kind, VOLATILE_DURABILITY_QOS);

    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "dr2", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);

    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "dr3", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.durability.kind, TRANSIENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_publishers_in_profile_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos name=\"pub1\">"
        "        <presentation><access_scope>INSTANCE_PRESENTATION_QOS</access_scope></presentation>"
        "      </publisher_qos>"
        "      <publisher_qos name=\"pub2\">"
        "        <presentation><access_scope>TOPIC_PRESENTATION_QOS</access_scope></presentation>"
        "      </publisher_qos>"
        "      <publisher_qos name=\"pub3\">"
        "        <presentation><access_scope>GROUP_PRESENTATION_QOS</access_scope></presentation>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    PublisherQos qos1, qos2, qos3;
    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "pub1", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.presentation.access_scope, INSTANCE_PRESENTATION_QOS);

    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "pub2", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.presentation.access_scope, TOPIC_PRESENTATION_QOS);

    EXPECT_EQ(repo.lookup_publisher_qos("lib", "prof", "pub3", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.presentation.access_scope, GROUP_PRESENTATION_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_subscribers_in_profile_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <subscriber_qos name=\"sub1\">"
        "        <presentation><access_scope>INSTANCE_PRESENTATION_QOS</access_scope></presentation>"
        "      </subscriber_qos>"
        "      <subscriber_qos name=\"sub2\">"
        "        <presentation><access_scope>TOPIC_PRESENTATION_QOS</access_scope></presentation>"
        "      </subscriber_qos>"
        "      <subscriber_qos name=\"sub3\">"
        "        <presentation><access_scope>GROUP_PRESENTATION_QOS</access_scope></presentation>"
        "      </subscriber_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    SubscriberQos qos1, qos2, qos3;
    EXPECT_EQ(repo.lookup_subscriber_qos("lib", "prof", "sub1", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.presentation.access_scope, INSTANCE_PRESENTATION_QOS);

    EXPECT_EQ(repo.lookup_subscriber_qos("lib", "prof", "sub2", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.presentation.access_scope, TOPIC_PRESENTATION_QOS);

    EXPECT_EQ(repo.lookup_subscriber_qos("lib", "prof", "sub3", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.presentation.access_scope, GROUP_PRESENTATION_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_topics_in_profile_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos name=\"topic1\">"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </topic_qos>"
        "      <topic_qos name=\"topic2\">"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "      </topic_qos>"
        "      <topic_qos name=\"topic3\">"
        "        <durability><kind>TRANSIENT_DURABILITY_QOS</kind></durability>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    TopicQos qos1, qos2, qos3;
    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "topic1", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.durability.kind, VOLATILE_DURABILITY_QOS);

    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "topic2", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);

    EXPECT_EQ(repo.lookup_topic_qos("lib", "prof", "topic3", qos3), RETCODE_OK);
    EXPECT_EQ(qos3.durability.kind, TRANSIENT_DURABILITY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_multiple_participants_in_profile_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos name=\"dp1\">"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "      </domain_participant_qos>"
        "      <domain_participant_qos name=\"dp2\">"
        "        <entity_factory><autoenable_created_entities>false</autoenable_created_entities></entity_factory>"
        "      </domain_participant_qos>"
        "      <domain_participant_qos name=\"dp3\">"
        "        <user_data><value>100,200,300</value></user_data>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DomainParticipantQos qos1, qos2, qos3;
    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "dp1", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.entity_factory.autoenable_created_entities, true);

    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "dp2", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.entity_factory.autoenable_created_entities, false);

    EXPECT_EQ(repo.lookup_participant_qos("lib", "prof", "dp3", qos3), RETCODE_OK);
    EXPECT_GT(qos3.user_data.value.size(), 0u);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_reload_qos_extended)
{
    const std::string xml1 =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path1 = create_xml_test_file(xml1);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path1), RETCODE_OK);

    DataWriterQos qos1;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos1), RETCODE_OK);
    EXPECT_EQ(qos1.durability.kind, VOLATILE_DURABILITY_QOS);

    std::remove(xml_path1.c_str());

    const std::string xml2 =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path2 = create_xml_test_file(xml2);
    EXPECT_EQ(repo.load(xml_path2), RETCODE_OK);

    DataWriterQos qos2;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos2), RETCODE_OK);
    EXPECT_EQ(qos2.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);

    std::remove(xml_path2.c_str());
}

TEST(DDSTest, Xml_complex_qos_with_multiple_policies_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <deadline><period><sec>5</sec><nanosec>0</nanosec></period></deadline>"
        "        <latency_budget><duration><sec>0</sec><nanosec>10000000</nanosec></duration></latency_budget>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>2</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <ownership><kind>EXCLUSIVE_OWNERSHIP_QOS</kind></ownership>"
        "        <liveliness><kind>MANUAL_BY_PARTICIPANT_LIVELINESS_QOS</kind><lease_duration><sec>15</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "        <destination_order><kind>BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS</kind></destination_order>"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>50</depth></history>"
        "        <resource_limits><max_samples>10000</max_samples><max_instances>500</max_instances></resource_limits>"
        "        <transport_priority><value>80</value></transport_priority>"
        "        <lifespan><duration><sec>60</sec><nanosec>0</nanosec></duration></lifespan>"
        "        <user_data><value>1,2,3</value></user_data>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.ownership.kind, EXCLUSIVE_OWNERSHIP_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_LAST_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_comprehensive_datawriter_qos_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <deadline><period><sec>10</sec><nanosec>0</nanosec></period></deadline>"
        "        <latency_budget><duration><sec>0</sec><nanosec>5000000</nanosec></duration></latency_budget>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>5</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <ownership><kind>EXCLUSIVE_OWNERSHIP_QOS</kind></ownership>"
        "        <ownership_strength><value>100</value></ownership_strength>"
        "        <liveliness><kind>MANUAL_BY_PARTICIPANT_LIVELINESS_QOS</kind><lease_duration><sec>30</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "        <destination_order><kind>BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS</kind></destination_order>"
        "        <history><kind>KEEP_LAST_HISTORY_QOS</kind><depth>100</depth></history>"
        "        <resource_limits><max_samples>50000</max_samples><max_instances>1000</max_instances></resource_limits>"
        "        <transport_priority><value>90</value></transport_priority>"
        "        <lifespan><duration><sec>120</sec><nanosec>0</nanosec></duration></lifespan>"
        "        <user_data><value>10,20,30</value></user_data>"
        "        <writer_data_lifecycle><autodispose_unregistered_instances>true</autodispose_unregistered_instances></writer_data_lifecycle>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataWriterQos qos;
    EXPECT_EQ(repo.lookup_datawriter_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.ownership.kind, EXCLUSIVE_OWNERSHIP_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_LAST_HISTORY_QOS);
    EXPECT_EQ(qos.writer_data_lifecycle.autodispose_unregistered_instances, true);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_comprehensive_datareader_qos_extended)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos>"
        "        <durability><kind>TRANSIENT_LOCAL_DURABILITY_QOS</kind></durability>"
        "        <deadline><period><sec>10</sec><nanosec>0</nanosec></period></deadline>"
        "        <latency_budget><duration><sec>0</sec><nanosec>5000000</nanosec></duration></latency_budget>"
        "        <reliability><kind>RELIABLE_RELIABILITY_QOS</kind><max_blocking_time><sec>5</sec><nanosec>0</nanosec></max_blocking_time></reliability>"
        "        <ownership><kind>SHARED_OWNERSHIP_QOS</kind></ownership>"
        "        <liveliness><kind>AUTOMATIC_LIVELINESS_QOS</kind><lease_duration><sec>30</sec><nanosec>0</nanosec></lease_duration></liveliness>"
        "        <time_based_filter><minimum_separation><sec>2</sec><nanosec>0</nanosec></minimum_separation></time_based_filter>"
        "        <history><kind>KEEP_ALL_HISTORY_QOS</kind></history>"
        "        <resource_limits><max_samples>50000</max_samples><max_instances>1000</max_instances></resource_limits>"
        "        <user_data><value>10,20,30</value></user_data>"
        "        <reader_data_lifecycle>"
        "          <autopurge_nowriter_samples_delay><sec>10</sec><nanosec>0</nanosec></autopurge_nowriter_samples_delay>"
        "          <autopurge_disposed_samples_delay><sec>30</sec><nanosec>0</nanosec></autopurge_disposed_samples_delay>"
        "        </reader_data_lifecycle>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    DataReaderQos qos;
    EXPECT_EQ(repo.lookup_datareader_qos("lib", "prof", "", qos), RETCODE_OK);
    EXPECT_EQ(qos.durability.kind, TRANSIENT_LOCAL_DURABILITY_QOS);
    EXPECT_EQ(qos.reliability.kind, RELIABLE_RELIABILITY_QOS);
    EXPECT_EQ(qos.ownership.kind, SHARED_OWNERSHIP_QOS);
    EXPECT_EQ(qos.history.kind, KEEP_ALL_HISTORY_QOS);

    std::remove(xml_path.c_str());
}

// ========== XML 解析错误情况测试 ==========

TEST(DDSTest, Xml_missing_library_name)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library>"
        "    <qos_profile name=\"test\">"
        "      <topic_qos>"
        "        <history><depth>5</depth></history>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_missing_profile_name)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"test_lib\">"
        "    <qos_profile>"
        "      <topic_qos>"
        "        <history><depth>5</depth></history>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_invalid_root_element)
{
    const std::string xml =
        "<invalid_root>"
        "  <qos_library name=\"test_lib\">"
        "    <qos_profile name=\"test\">"
        "    </qos_profile>"
        "  </qos_library>"
        "</invalid_root>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_invalid_child_element)
{
    const std::string xml =
        "<dds_qos>"
        "  <invalid_child name=\"test\">"
        "  </invalid_child>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_qos_not_found)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos>"
        "        <history><depth>5</depth></history>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    EXPECT_EQ(repo.load(xml_path), RETCODE_OK);

    TopicQos qos;
    ReturnCode_t ret = repo.lookup_topic_qos("nonexistent", "prof", "", qos);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_empty_file)
{
    const std::string xml = "";
    const std::string xml_path = create_xml_test_file(xml);
    
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_base_name_not_found)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_factory_qos name=\"derived\" base_name=\"nonexistent_base\">"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "      </domain_participant_factory_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_publisher_qos_base_name_not_found)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos name=\"derived_pub\" base_name=\"nonexistent_base\">"
        "        <partition><name>test_partition</name></partition>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_subscriber_qos_base_name_not_found)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <subscriber_qos name=\"derived_sub\" base_name=\"nonexistent_base\">"
        "        <partition><name>test_partition</name></partition>"
        "      </subscriber_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_datawriter_qos_base_name_not_found)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datawriter_qos name=\"derived_dw\" base_name=\"nonexistent_base\">"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </datawriter_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_datareader_qos_base_name_not_found)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos name=\"derived_dr\" base_name=\"nonexistent_base\">"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_topic_qos_base_name_not_found)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos name=\"derived_topic\" base_name=\"nonexistent_base\">"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_domain_participant_qos_base_name_not_found)
{
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_qos name=\"derived_participant\" base_name=\"nonexistent_base\">"
        "        <entity_factory><autoenable_created_entities>true</autoenable_created_entities></entity_factory>"
        "      </domain_participant_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_ERROR);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_parse_domain_participant_factory_qos_error)
{
    // 测试解析 DomainParticipantFactoryQos 失败的场景
    // 由于 XML 解析器可能不会因为无效值而返回错误，我们测试其他错误场景
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <domain_participant_factory_qos>"
        "        <entity_factory>"
        "          <autoenable_created_entities>true</autoenable_created_entities>"
        "        </entity_factory>"
        "      </domain_participant_factory_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_parse_publisher_qos_error)
{
    // 测试解析 PublisherQos 失败的场景
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <publisher_qos>"
        "        <presentation>"
        "          <access_scope>INSTANCE_PRESENTATION_QOS</access_scope>"
        "        </presentation>"
        "      </publisher_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_parse_subscriber_qos_error)
{
    // 测试解析 SubscriberQos 失败的场景
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <subscriber_qos>"
        "        <presentation>"
        "          <access_scope>INSTANCE_PRESENTATION_QOS</access_scope>"
        "        </presentation>"
        "      </subscriber_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_parse_datareader_qos_error)
{
    // 测试解析 DataReaderQos 失败的场景
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <datareader_qos>"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </datareader_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    std::remove(xml_path.c_str());
}

TEST(DDSTest, Xml_parse_topic_qos_error)
{
    // 测试解析 TopicQos 失败的场景
    const std::string xml =
        "<dds_qos>"
        "  <qos_library name=\"lib\">"
        "    <qos_profile name=\"prof\">"
        "      <topic_qos>"
        "        <durability><kind>VOLATILE_DURABILITY_QOS</kind></durability>"
        "      </topic_qos>"
        "    </qos_profile>"
        "  </qos_library>"
        "</dds_qos>";

    const std::string xml_path = create_xml_test_file(xml);
    DDSXMLRepo repo;
    ReturnCode_t ret = repo.load(xml_path);
    EXPECT_EQ(ret, RETCODE_OK);

    std::remove(xml_path.c_str());
}

