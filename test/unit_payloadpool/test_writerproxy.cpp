// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include <gtest/gtest.h>
#include "rtps/endpoint/writerproxy.h"
using namespace TRAVODDS;
namespace {
std::shared_ptr<CacheChange> change(unsigned sequence) {
    auto cache = std::make_shared<CacheChange>();
    cache->sequenceNumber = SequenceNumber_t(0, sequence);
    return cache;
}
WriterProxy proxy() { return WriterProxy(GUID_UNKNOWN, GUID_UNKNOWN, {}, {}); }
SequenceNumber_t seq(unsigned sequence) { return SequenceNumber_t(0, sequence); }
}

TEST(WriterProxyGap, LateVolatileGapAfterFirstDataUnblocksSubmission) {
    auto writer = proxy(); auto second = change(2);
    ASSERT_EQ(writer.AddReliableDataCache(second, VOLATILE_DURABILITY_QOS), RETCODE_OK);
    EXPECT_EQ(writer.GetSubmitCache(), nullptr);
    writer.NonAvailableChangeSet({seq(1)}, {});
    EXPECT_EQ(writer.GetSubmitCache(), second);
    EXPECT_EQ(writer.GetSubmitCache(), nullptr);
}
TEST(WriterProxyGap, LateVolatileGapBeforeFirstDataUnblocksSubmission) {
    auto writer = proxy(); auto second = change(2);
    writer.NonAvailableChangeSet({seq(1)}, {});
    EXPECT_EQ(writer.GetSubmitCache(), nullptr);
    ASSERT_EQ(writer.AddReliableDataCache(second, VOLATILE_DURABILITY_QOS), RETCODE_OK);
    EXPECT_EQ(writer.GetSubmitCache(), second);
}
TEST(WriterProxyGap, ExtendingGapPreservesAlreadyReceivedData) {
    auto writer = proxy(); auto second = change(2), fourth = change(4);
    ASSERT_EQ(writer.AddReliableDataCache(second, VOLATILE_DURABILITY_QOS), RETCODE_OK);
    writer.NonAvailableChangeSet({seq(1), seq(3)}, {});
    EXPECT_EQ(writer.GetSubmitCache(), second);
    EXPECT_EQ(writer.GetSubmitCache(), nullptr);
    ASSERT_EQ(writer.AddReliableDataCache(fourth, VOLATILE_DURABILITY_QOS), RETCODE_OK);
    EXPECT_EQ(writer.GetSubmitCache(), fourth);
}
TEST(WriterProxyGap, RepeatedOldGapDoesNotResurrectSubmittedChanges) {
    auto writer = proxy(); auto second = change(2), third = change(3);
    writer.NonAvailableChangeSet({seq(1)}, {});
    ASSERT_EQ(writer.AddReliableDataCache(second, VOLATILE_DURABILITY_QOS), RETCODE_OK);
    ASSERT_EQ(writer.GetSubmitCache(), second);
    writer.NonAvailableChangeSet({seq(1), seq(2)}, {});
    ASSERT_EQ(writer.AddReliableDataCache(third, VOLATILE_DURABILITY_QOS), RETCODE_OK);
    EXPECT_EQ(writer.GetSubmitCache(), third);
}
TEST(WriterProxyGap, TransientReaderWaitsForPrecedingDataOrGap) {
    auto writer = proxy(); auto first = change(1), second = change(2);
    ASSERT_EQ(writer.AddReliableDataCache(second, TRANSIENT_LOCAL_DURABILITY_QOS), RETCODE_OK);
    EXPECT_EQ(writer.GetSubmitCache(), nullptr);
    ASSERT_EQ(writer.AddReliableDataCache(first, TRANSIENT_LOCAL_DURABILITY_QOS), RETCODE_OK);
    EXPECT_EQ(writer.GetSubmitCache(), first);
    EXPECT_EQ(writer.GetSubmitCache(), second);
}

TEST(WriterProxyGap, TransientDataBeforeFirstHeartbeatSkipsUnavailableOlderHistory) {
    auto writer = proxy(); auto third = change(3);
    ASSERT_EQ(writer.AddReliableDataCache(third, TRANSIENT_LOCAL_DURABILITY_QOS), RETCODE_OK);
    EXPECT_EQ(writer.GetSubmitCache(), nullptr);
    HeartBeatSubMessage_t heartbeat{};
    heartbeat.firstSN = seq(3); heartbeat.lastSN = seq(3); heartbeat.count = 1;
    int32_t lost = -1; SequenceNumberSet_t missing;
    ASSERT_EQ(writer.RecvHeartbeat(heartbeat, lost, missing), RETCODE_OK);
    EXPECT_EQ(lost, 0);
    EXPECT_EQ(writer.GetSubmitCache(), third);
}

TEST(WriterProxyGap, TransientHeartbeatBeforeFirstDataPreservesTheRequestedSequence) {
    auto writer = proxy(); auto third = change(3);
    HeartBeatSubMessage_t heartbeat{};
    heartbeat.firstSN = seq(3); heartbeat.lastSN = seq(3); heartbeat.count = 1;
    int32_t lost = -1; SequenceNumberSet_t missing;
    ASSERT_EQ(writer.RecvHeartbeat(heartbeat, lost, missing), RETCODE_OK);
    EXPECT_EQ(lost, 0);
    EXPECT_EQ(missing.bitmapBase, seq(3));
    ASSERT_EQ(writer.AddReliableDataCache(third, TRANSIENT_LOCAL_DURABILITY_QOS), RETCODE_OK);
    EXPECT_EQ(writer.GetSubmitCache(), third);
}

TEST(WriterProxyGap, TransientTruncationKeepsEarlierReceivedDataAndSkipsMissingData) {
    auto writer = proxy(); auto first = change(1), third = change(3);
    ASSERT_EQ(writer.AddReliableDataCache(first, TRANSIENT_LOCAL_DURABILITY_QOS), RETCODE_OK);
    ASSERT_EQ(writer.AddReliableDataCache(third, TRANSIENT_LOCAL_DURABILITY_QOS), RETCODE_OK);
    HeartBeatSubMessage_t heartbeat{};
    heartbeat.firstSN = seq(3); heartbeat.lastSN = seq(3); heartbeat.count = 1;
    int32_t lost = -1; SequenceNumberSet_t missing;
    ASSERT_EQ(writer.RecvHeartbeat(heartbeat, lost, missing), RETCODE_OK);
    EXPECT_EQ(writer.GetSubmitCache(), first);
    EXPECT_EQ(writer.GetSubmitCache(), third);
}
