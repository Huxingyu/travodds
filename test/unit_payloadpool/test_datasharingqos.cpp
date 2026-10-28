#include "gtest/gtest.h"
#include "qos/qospolicycheck.h"
#include "xml/dds_xml_repo.h"
#include <fstream>
#include <unistd.h>

USING_TRAVODDS_NAMESPACE

TEST(PayloadPoolQos, ValidatesCapacityPolicyAndAlignmentBeforeAllocation) {
    PayloadPoolQosPolicy q;
    EXPECT_TRUE(q.valid());
    EXPECT_EQ(q.max_payloads, UINT32_MAX);
    EXPECT_EQ(q.policy, PayloadMemoryPolicy::DYNAMIC_REUSABLE);
    for (auto policy : {PayloadMemoryPolicy::PREALLOCATED, PayloadMemoryPolicy::PREALLOCATED_WITH_REALLOC,
            PayloadMemoryPolicy::DYNAMIC_RESERVE, PayloadMemoryPolicy::DYNAMIC_REUSABLE}) {
        q = {}; q.policy = policy; q.payload_size = 128; q.max_payloads = 2;
        EXPECT_TRUE(q.valid());
    }
    auto invalid = [](PayloadPoolQosPolicy value) {
        EXPECT_FALSE(value.valid());
        DataWriterQos writer; writer.payload_pool = value;
        EXPECT_EQ(QosPolicyCheck::CheckQos(writer), RETCODE_INCONSISTENT_POLICY);
    };
    q = {}; q.policy = static_cast<PayloadMemoryPolicy>(99); invalid(q);
    q = {}; q.max_payloads = 0; invalid(q);
    q = {}; q.max_payloads = 2; q.initial_payloads = 3; invalid(q);
    q = {}; q.policy = PayloadMemoryPolicy::PREALLOCATED; invalid(q);
    q.policy = PayloadMemoryPolicy::PREALLOCATED_WITH_REALLOC; invalid(q);
    q = {}; q.policy = PayloadMemoryPolicy::DYNAMIC_RESERVE; q.initial_payloads = 1; invalid(q);
    q = {}; q.alignment = 3; invalid(q);
    q.alignment = 0; invalid(q);
}

TEST(PayloadPoolQos, EveryAllocationParameterIsImmutable) {
    auto check = [](auto edit) {
        DataWriterQos a, b;
        edit(b.payload_pool);
        EXPECT_FALSE(QosPolicyCheck::CanQosBeUpdated(a, b));
    };
    check([](auto& q) { q.policy = PayloadMemoryPolicy::DYNAMIC_RESERVE; });
    check([](auto& q) { q.payload_size = 128; });
    check([](auto& q) { q.max_payloads = 2; });
    check([](auto& q) { q.initial_payloads = 1; });
    check([](auto& q) { q.alignment = 64; });
}

TEST(DataSharingQos, DefaultsGroupsAndValidation) {
    DataSharingQosPolicy a, b;
    EXPECT_TRUE(a.valid());
    EXPECT_EQ(a.kind, AUTO_DATA_SHARING_QOS);
    EXPECT_EQ(a.max_samples, 64u);
    EXPECT_EQ(a.max_readers, 16u);
    EXPECT_EQ(a, b);
    EXPECT_TRUE(a.shares_domain_with(b));
    b.domain_ids = {7, 9};
    EXPECT_FALSE(a.shares_domain_with(b));
    a.domain_ids = {3, 9};
    EXPECT_TRUE(a.shares_domain_with(b));
    b.kind = ON_DATA_SHARING_QOS;
    EXPECT_TRUE(a.shares_domain_with(b));
    b.kind = OFF_DATA_SHARING_QOS;
    EXPECT_FALSE(a.shares_domain_with(b));
    auto bad = [&](auto change) {
        DataWriterQos w; DataReaderQos r;
        change(w.data_sharing); r.data_sharing = w.data_sharing;
        EXPECT_FALSE(w.data_sharing.valid());
        EXPECT_EQ(QosPolicyCheck::CheckQos(w), RETCODE_INCONSISTENT_POLICY);
        EXPECT_EQ(QosPolicyCheck::CheckQos(r), RETCODE_INCONSISTENT_POLICY);
        EXPECT_FALSE(w.data_sharing.shares_domain_with(a));
    };
    // 3 is outside the named policies but inside this enum's representable
    // range. Casting 99 would itself have undefined behavior before validation.
    bad([](auto& q) { q.kind = static_cast<DataSharingQosPolicyKind>(3); });
    bad([](auto& q) { q.shared_memory_directory.clear(); });
    bad([](auto& q) { q.shared_memory_directory = "relative"; });
    bad([](auto& q) { q.shared_memory_directory = std::string("/tmp\0suffix", 11); });
    bad([](auto& q) { q.shared_memory_directory.assign(4097, '/'); });
    bad([](auto& q) { q.max_samples = 0; });
    bad([](auto& q) { q.max_samples = 1048577; });
    bad([](auto& q) { q.max_readers = 0; });
    bad([](auto& q) { q.max_readers = 65; });
    bad([](auto& q) { q.max_segment_bytes = 0; });
    bad([](auto& q) { q.domain_ids.clear(); });
    bad([](auto& q) { q.domain_ids = {1, 1}; });
    bad([](auto& q) { q.domain_ids.resize(33); });
}

TEST(DataSharingQos, EveryAllocationAndNegotiationFieldIsImmutable) {
    auto immutable = [](auto change) {
        DataWriterQos w, w2; DataReaderQos r, r2;
        change(w2.data_sharing); r2.data_sharing = w2.data_sharing;
        EXPECT_FALSE(QosPolicyCheck::CanQosBeUpdated(w, w2));
        EXPECT_FALSE(QosPolicyCheck::CanQosBeUpdated(r, r2));
    };
    immutable([](auto& q) { q.kind = ON_DATA_SHARING_QOS; });
    immutable([](auto& q) { q.shared_memory_directory = "/tmp"; });
    immutable([](auto& q) { q.domain_ids = {42}; });
    immutable([](auto& q) { q.max_samples = 12; });
    immutable([](auto& q) { q.max_readers = 8; });
    immutable([](auto& q) { q.max_segment_bytes = 65536; });
    EXPECT_TRUE(QosPolicyCheck::CanQosBeUpdated(DataWriterQos{}, DataWriterQos{}));
    EXPECT_TRUE(QosPolicyCheck::CanQosBeUpdated(DataReaderQos{}, DataReaderQos{}));
}

#ifdef _XML_INTERFACE_CPP
namespace {
class DataSharingXml : public testing::Test {
protected:
    void SetUp() override {
        char pattern[] = "/tmp/travodds-ds-qos-XXXXXX";
        int fd = mkstemp(pattern);
        ASSERT_GE(fd, 0);
        close(fd); path = pattern;
    }
    void TearDown() override { unlink(path.c_str()); }
    ReturnCode_t load(const std::string& body) {
        std::ofstream file(path);
        file << "<dds_qos><qos_library name=\"lib\"><qos_profile name=\"p\">" << body
             << "</qos_profile></qos_library></dds_qos>";
        file.close(); return repo.load(path);
    }
    std::string path;
    DDSXMLRepo repo;
};
}

TEST_F(DataSharingXml, PublicPoolPoliciesAndInheritedOverrides) {
    for (const auto& policy : {"PREALLOCATED", "PREALLOCATED_WITH_REALLOC", "DYNAMIC_RESERVE", "DYNAMIC_REUSABLE"}) {
        ASSERT_EQ(repo.clear(), RETCODE_OK);
        ASSERT_EQ(load(std::string("<datawriter_qos name=\"base\"><payload_pool><policy>") + policy +
            "</policy><payload_size>4096</payload_size><max_payloads>2</max_payloads><alignment>64</alignment>"
            "</payload_pool></datawriter_qos><datawriter_qos name=\"child\" base_name=\"base\">"
            "<payload_pool><max_payloads>3</max_payloads></payload_pool></datawriter_qos>"), RETCODE_OK);
        DataWriterQos a, b;
        ASSERT_EQ(repo.lookup_datawriter_qos("lib", "p", "base", a), RETCODE_OK);
        ASSERT_EQ(repo.lookup_datawriter_qos("lib", "p", "child", b), RETCODE_OK);
        EXPECT_TRUE(a.payload_pool.valid());
        EXPECT_EQ(a.payload_pool.payload_size, 4096u);
        EXPECT_EQ(a.payload_pool.max_payloads, 2u);
        EXPECT_EQ(a.payload_pool.alignment, 64u);
        EXPECT_EQ(b.payload_pool.policy, a.payload_pool.policy);
        EXPECT_EQ(b.payload_pool.max_payloads, 3u);
        EXPECT_EQ(b.payload_pool.payload_size, 4096u);
    }
}

TEST_F(DataSharingXml, RejectsMalformedPoolPoliciesAndDuplicateSections) {
    for (const std::string body : {
            "<policy>UNKNOWN</policy>", "<policy/>", "<policy>PREALLOCATED</policy>",
            "<policy>DYNAMIC_RESERVE</policy><initial_payloads>1</initial_payloads>",
            "<max_payloads>0</max_payloads>", "<max_payloads>-1</max_payloads>",
            "<payload_size>4294967296</payload_size>", "<initial_payloads>2</initial_payloads><max_payloads>1</max_payloads>",
            "<max_payloads>2x</max_payloads>", "<max_payloads/>", "<max_payloads><x/></max_payloads>",
            "<alignment>3</alignment>", "<alignment>0</alignment>", "<alignment>18446744073709551616</alignment>",
            "<policy>DYNAMIC_REUSABLE<x/></policy>", "<max_payloads extra=\"x\">2</max_payloads>",
            "<max_payloads>2</max_payloads><max_payloads>3</max_payloads>", "<unknown>2</unknown>", "junk"}) {
        SCOPED_TRACE(body);
        ASSERT_EQ(repo.clear(), RETCODE_OK);
        EXPECT_NE(load("<datawriter_qos><payload_pool>" + body + "</payload_pool></datawriter_qos>"), RETCODE_OK);
    }
    EXPECT_NE(load("<datawriter_qos><payload_pool/><payload_pool/></datawriter_qos>"), RETCODE_OK);
}

TEST_F(DataSharingXml, WriterReaderProfilesAndInheritedOverrides) {
    ASSERT_EQ(load(R"(
      <datawriter_qos name="base"><data_sharing>
        <kind>ON</kind><shared_memory_directory>/tmp</shared_memory_directory>
        <max_samples>12</max_samples><max_readers>3</max_readers>
        <max_segment_bytes>65536</max_segment_bytes><domain_ids><id>5</id><id>4294967295</id></domain_ids>
      </data_sharing></datawriter_qos>
      <datawriter_qos name="child" base_name="base"><data_sharing><kind>OFF</kind></data_sharing></datawriter_qos>
      <datareader_qos name="base"><data_sharing><kind>ON</kind><domain_ids><id>5</id></domain_ids></data_sharing></datareader_qos>
      <datareader_qos name="child" base_name="base"><data_sharing><max_samples>7</max_samples></data_sharing></datareader_qos>
    )"), RETCODE_OK);
    DataWriterQos w; DataReaderQos r;
    ASSERT_EQ(repo.lookup_datawriter_qos("lib", "p", "base", w), RETCODE_OK);
    EXPECT_EQ(w.data_sharing.kind, ON_DATA_SHARING_QOS);
    EXPECT_EQ(w.data_sharing.shared_memory_directory, "/tmp");
    EXPECT_EQ(w.data_sharing.max_samples, 12u);
    EXPECT_EQ(w.data_sharing.max_readers, 3u);
    EXPECT_EQ(w.data_sharing.max_segment_bytes, 65536u);
    EXPECT_EQ(w.data_sharing.domain_ids, (std::vector<uint32_t>{5, UINT32_MAX}));
    ASSERT_EQ(repo.lookup_datawriter_qos("lib", "p", "child", w), RETCODE_OK);
    EXPECT_EQ(w.data_sharing.kind, OFF_DATA_SHARING_QOS);
    EXPECT_EQ(w.data_sharing.max_samples, 12u);
    ASSERT_EQ(repo.lookup_datareader_qos("lib", "p", "child", r), RETCODE_OK);
    EXPECT_EQ(r.data_sharing.kind, ON_DATA_SHARING_QOS);
    EXPECT_EQ(r.data_sharing.max_samples, 7u);
    EXPECT_EQ(r.data_sharing.domain_ids, std::vector<uint32_t>{5});
}

TEST_F(DataSharingXml, RejectsMalformedAndOutOfRangeValuesForBothEndpoints) {
    const std::vector<std::string> invalid{
        "<kind>YES</kind>", "<kind/>", "<kind>ON<x/></kind>", "junk",
        "<max_samples>-1</max_samples>", "<max_samples>12x</max_samples>",
        "<max_samples>0</max_samples>", "<max_samples/>", "<max_samples>4294967296</max_samples>",
        "<max_readers>65</max_readers>", "<max_segment_bytes>18446744073709551616</max_segment_bytes>",
        "<max_segment_bytes>0</max_segment_bytes>", "<shared_memory_directory>relative</shared_memory_directory>",
        "<shared_memory_directory/>", "<domain_ids/>", "<domain_ids><id>2</id><id>2</id></domain_ids>",
        "<domain_ids><id>4294967296</id></domain_ids>", "<domain_ids><id>-1</id></domain_ids>",
        "<domain_ids><unknown>2</unknown></domain_ids>", "<unknown/>",
        "<kind>ON</kind><kind>OFF</kind>", "<max_samples extra=\"x\">12</max_samples>"
    };
    for (const auto& endpoint : {"datawriter_qos", "datareader_qos"}) {
        for (const auto& value : invalid) {
            SCOPED_TRACE(std::string(endpoint) + value);
            EXPECT_EQ(repo.clear(), RETCODE_OK);
            EXPECT_NE(load("<" + std::string(endpoint) + "><data_sharing>" + value +
                "</data_sharing></" + endpoint + ">"), RETCODE_OK);
        }
        EXPECT_NE(load("<" + std::string(endpoint) + " base_name=\"missing\"/>"), RETCODE_OK);
        EXPECT_NE(load("<" + std::string(endpoint) + "><data_sharing/><data_sharing/></" + endpoint + ">"), RETCODE_OK);
    }
}
TEST_F(DataSharingXml, FiniteLifecycleMatchesCppAndInheritedValues) {
    ASSERT_EQ(load(R"(
      <datareader_qos name="base"><reader_data_lifecycle>
        <autopurge_nowriter_samples_delay><sec>0</sec><nanosec>30000000</nanosec></autopurge_nowriter_samples_delay>
        <autopurge_disposed_samples_delay><sec>2</sec><nanosec>0</nanosec></autopurge_disposed_samples_delay>
      </reader_data_lifecycle></datareader_qos>
      <datareader_qos name="child" base_name="base"><reader_data_lifecycle>
        <autopurge_disposed_samples_delay><sec>0</sec><nanosec>0</nanosec></autopurge_disposed_samples_delay>
      </reader_data_lifecycle></datareader_qos>)"), RETCODE_OK);
    DataReaderQos q;
    ASSERT_EQ(repo.lookup_datareader_qos("lib", "p", "child", q), RETCODE_OK);
    EXPECT_EQ(q.reader_data_lifecycle.autopurge_nowriter_samples_delay, (Duration_t{0,30000000}));
    EXPECT_EQ(q.reader_data_lifecycle.autopurge_disposed_samples_delay, Duration_t(TIME_ZERO));
    EXPECT_EQ(QosPolicyCheck::CheckQos(q), RETCODE_OK);
    for (const std::string duration : {"<sec>-1</sec><nanosec>0</nanosec>",
            "<sec>0</sec><nanosec>1000000000</nanosec>", "<sec/>"}) {
        ASSERT_EQ(repo.clear(), RETCODE_OK);
        EXPECT_NE(load("<datareader_qos><reader_data_lifecycle><autopurge_nowriter_samples_delay>" + duration +
            "</autopurge_nowriter_samples_delay></reader_data_lifecycle></datareader_qos>"), RETCODE_OK);
    }
    q.reader_data_lifecycle.autopurge_nowriter_samples_delay = {-1,0};
    EXPECT_EQ(QosPolicyCheck::CheckQos(q), RETCODE_INCONSISTENT_POLICY);
}
#endif
