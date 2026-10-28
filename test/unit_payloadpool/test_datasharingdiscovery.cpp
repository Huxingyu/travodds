#include "gtest/gtest.h"
#include "rtps/discovery/datasharingendpoint.h"
#include "rtps/messages/parameter.h"

USING_TRAVODDS_NAMESPACE

namespace {
DataSharingEndpointInfo readerInfo() {
    DataSharingEndpointInfo info;
    info.version = 1; info.kind = AUTO_DATA_SHARING_QOS;
    info.type = {true, true, 64, 64, "ospp.Native.u64x8.v1"};
    info.domain_ids = {0, 7};
    info.host_id = "11111111-2222-3333-4444-555555555555";
    info.pid_namespace_device = 4; info.pid_namespace_inode = 42;
    return info;
}
DataSharingEndpointInfo writerInfo() {
    auto info = readerInfo();
    info.kind = ON_DATA_SHARING_QOS;
    info.session_high = 17; info.session_low = 37;
    info.segment_bytes = 16384; info.slots = 64; info.max_readers = 16;
    return info;
}
std::vector<octet> wire(ParameterList& list) {
    std::vector<octet> bytes(list.GetMessageSize());
    auto length = static_cast<uint16_t>(bytes.size());
    EXPECT_TRUE(list.WriterToMessage(bytes.data(), length));
    return bytes;
}
}

TEST(DataSharingDiscovery, MatchingRequiresCommonHostNamespaceTypeAndDomain) {
    const auto writer = writerInfo();
    const auto reader = readerInfo();
    ASSERT_TRUE(writer.valid(true));
    ASSERT_TRUE(reader.valid(false));
    EXPECT_TRUE(writer.compatible_with(reader));
    auto incompatible = [&](auto change) {
        auto other = reader; change(other); EXPECT_FALSE(writer.compatible_with(other));
    };
    incompatible([](auto& i) { i.version = 0; });
    incompatible([](auto& i) { i.version = 2; });
    incompatible([](auto& i) { i.kind = OFF_DATA_SHARING_QOS; });
    incompatible([](auto& i) { i.type.plain = false; });
    incompatible([](auto& i) { i.type.bounded = false; });
    incompatible([](auto& i) { i.type.size = 128; });
    incompatible([](auto& i) { i.type.alignment = 128; });
    incompatible([](auto& i) { i.type.layout_id += ".incompatible"; });
    incompatible([](auto& i) { i.host_id[0] = '9'; });
    incompatible([](auto& i) { ++i.pid_namespace_device; });
    incompatible([](auto& i) { ++i.pid_namespace_inode; });
    incompatible([](auto& i) { i.domain_ids = {8}; });
    incompatible([](auto& i) { i.domain_ids = {0, 0}; });
    incompatible([](auto& i) { i.session_high = 1; });
    EXPECT_FALSE(reader.compatible_with(writer));
}

TEST(DataSharingDiscovery, CodecRoundTripsAndRejectsTruncationOversizedOrInvalidFields) {
    for (auto info : {readerInfo(), writerInfo()}) {
        auto bytes = EncodeDataSharing(info);
        ASSERT_FALSE(bytes.empty());
        DataSharingEndpointInfo result;
        ASSERT_TRUE(DecodeDataSharing(bytes.data(), bytes.size(), result));
        EXPECT_EQ(EncodeDataSharing(result), bytes);
        // Mutating every field boundary either fails closed or produces a
        // fully validated canonical record; never partially retain old state.
        for (size_t offset = 0; offset < bytes.size(); ++offset) {
            auto mutation = bytes; mutation[offset] ^= 255;
            if (DecodeDataSharing(mutation.data(), mutation.size(), result))
                EXPECT_EQ(EncodeDataSharing(result), mutation);
            else EXPECT_EQ(result.version, 0u);
        }
        for (size_t size = 0; size < bytes.size(); ++size) {
            SCOPED_TRACE(size);
            EXPECT_FALSE(DecodeDataSharing(bytes.data(), size, result));
            EXPECT_EQ(result.version, 0u);
        }
        auto bad = bytes; bad[0] ^= 1;
        EXPECT_FALSE(DecodeDataSharing(bad.data(), bad.size(), result));
        bad = bytes; bad[7] = 2;
        EXPECT_FALSE(DecodeDataSharing(bad.data(), bad.size(), result));
        bad = bytes; bad[11] = 2;
        EXPECT_FALSE(DecodeDataSharing(bad.data(), bad.size(), result));
        bad = bytes; bad[20] = 255; bad[21] = 255;
        EXPECT_FALSE(DecodeDataSharing(bad.data(), bad.size(), result));
        bad = bytes; bad.push_back(0);
        EXPECT_FALSE(DecodeDataSharing(bad.data(), bad.size(), result));
        bad = bytes; bad.insert(bad.end(), 4, 0);
        EXPECT_FALSE(DecodeDataSharing(bad.data(), bad.size(), result));
    }
    DataSharingEndpointInfo result;
    EXPECT_FALSE(DecodeDataSharing(nullptr, 16, result));
    std::vector<uint8_t> oversized(1028);
    EXPECT_FALSE(DecodeDataSharing(oversized.data(), oversized.size(), result));
    auto invalid = [](auto change) {
        auto info = writerInfo(); change(info); EXPECT_TRUE(EncodeDataSharing(info).empty());
    };
    invalid([](auto& i) { i.host_id = "wrong"; });
    invalid([](auto& i) { i.host_id[8] = '0'; });
    invalid([](auto& i) { i.host_id[0] = 'X'; });
    invalid([](auto& i) { i.type.layout_id.assign(257, 'a'); });
    invalid([](auto& i) { i.type.layout_id.push_back('\0'); });
    invalid([](auto& i) { i.type.alignment = 8192; });
    invalid([](auto& i) { i.pid_namespace_inode = 0; });
    invalid([](auto& i) { i.domain_ids.clear(); });
    invalid([](auto& i) { i.domain_ids.resize(33); });
    invalid([](auto& i) { i.max_readers = 65; });
    invalid([](auto& i) { i.slots = 1048577; });
    invalid([](auto& i) { i.session_high = i.session_low = 0; });
}

TEST(DataSharingDiscovery, WriterReaderParameterListsPreserveExtensionInBothByteOrders) {
    for (auto endian : {EndiannessKind::PL_CDR_BE, EndiannessKind::PL_CDR_LE}) {
        DiscoveredWriterData writer;
        writer.ddsPublicationData.topic_name = "native-topic";
        writer.ddsPublicationData.data_sharing = writerInfo();
        ParameterList wp(writer, endian); auto bytes = wire(wp);
        ParameterList parsed(bytes.data(), static_cast<uint32_t>(bytes.size()));
        DiscoveredWriterData output; parsed.GetDiscoveredData(output);
        EXPECT_EQ(output.ddsPublicationData.topic_name, "native-topic");
        EXPECT_EQ(EncodeDataSharing(output.ddsPublicationData.data_sharing), EncodeDataSharing(writerInfo()));
        DiscoveredReaderData reader;
        reader.ddsSubscriptionData.data_sharing = readerInfo();
        ParameterList rp(reader, endian); bytes = wire(rp);
        ParameterList rparsed(bytes.data(), static_cast<uint32_t>(bytes.size()));
        DiscoveredReaderData routput; rparsed.GetDiscoveredData(routput);
        EXPECT_TRUE(output.ddsPublicationData.data_sharing.compatible_with(routput.ddsSubscriptionData.data_sharing));
        // An old endpoint omits the extension. Reused outputs must lose stale capability.
        ParameterList legacy(DiscoveredWriterData{}, endian);
        legacy.GetDiscoveredData(output);
        EXPECT_EQ(output.ddsPublicationData.data_sharing.version, 0u);
    }
}

TEST(DataSharingDiscovery, MalformedDuplicateOrWrongRoleExtensionFallsBack) {
    ParameterList list;
    list.AddParameter(PID_TRAVO_DATA_SHARING, writerInfo());
    auto bytes = wire(list);
    const auto body = std::vector<octet>(bytes.begin() + 4, bytes.end() - 4);
    // A duplicate cannot select one of two conflicting capability records.
    bytes.insert(bytes.end() - 4, body.begin(), body.end());
    ParameterList duplicate(bytes.data(), static_cast<uint32_t>(bytes.size()));
    DiscoveredWriterData writer;
    duplicate.GetDiscoveredData(writer);
    EXPECT_EQ(writer.ddsPublicationData.data_sharing.version, 0u);
    bytes = wire(list);
    for (size_t length = 5; length < 8 + EncodeDataSharing(writerInfo()).size(); ++length) {
        ParameterList truncated(bytes.data(), static_cast<uint32_t>(length));
        truncated.GetDiscoveredData(writer);
        EXPECT_EQ(writer.ddsPublicationData.data_sharing.version, 0u);
    }
    DiscoveredReaderData reader;
    list.GetDiscoveredData(reader);
    EXPECT_EQ(reader.ddsSubscriptionData.data_sharing.version, 0u);
    bytes = wire(list); bytes[8] ^= 1;
    ParameterList bad(bytes.data(), static_cast<uint32_t>(bytes.size()));
    bad.GetDiscoveredData(writer);
    EXPECT_EQ(writer.ddsPublicationData.data_sharing.version, 0u);
}

TEST(DataSharingDiscovery, LocalIdentityIsStableAndOffOrLegacyTypeNeverAdvertises) {
    DataSharingQosPolicy qos;
    const auto type = readerInfo().type;
    const auto first = LocalDataSharingInfo(qos, type);
#if defined(__linux__)
    ASSERT_TRUE(first.valid(false));
    EXPECT_EQ(EncodeDataSharing(first), EncodeDataSharing(LocalDataSharingInfo(qos, type)));
#else
    EXPECT_EQ(first.version, 0u);
#endif
    qos.kind = OFF_DATA_SHARING_QOS;
    EXPECT_EQ(LocalDataSharingInfo(qos, type).version, 0u);
    qos.kind = AUTO_DATA_SHARING_QOS;
    EXPECT_EQ(LocalDataSharingInfo(qos, NativeTypeInfo{}).version, 0u);
    qos.max_readers = 0;
    EXPECT_EQ(LocalDataSharingInfo(qos, type).version, 0u);
}
