// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include <gtest/gtest.h>
#include "rtps/endpoint/readerproxy.h"
#include "rtps/endpoint/writerproxy.h"
#include <future>
#include <thread>
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

TEST(ReaderProxyRequests, SnapshotsOwnTheirSequencesAcrossReplacementAndEmptyRequests) {
    ReaderProxy reader(GUID_UNKNOWN, GUID_UNKNOWN, {}, {}, true);
    EXPECT_TRUE(reader.RequestedChanges().empty());
    EXPECT_EQ(reader.NextRequestedChange(), SEQUENCENUMBER_UNKNOWN);

    const SeqNumberVec first{seq(11), seq(12)}, second{seq(21), seq(22)};
    reader.RequestedChangesSet(first);
    auto snapshot = reader.RequestedChanges();
    EXPECT_EQ(snapshot, first);
    EXPECT_EQ(reader.NextRequestedChange(), first.front());
    snapshot.front() = seq(99);
    EXPECT_EQ(reader.RequestedChanges(), first);

    reader.RequestedChangesSet(second);
    EXPECT_EQ(snapshot.front(), seq(99));
    EXPECT_EQ(snapshot.back(), first.back());
    EXPECT_EQ(reader.RequestedChanges(), second);
    EXPECT_EQ(reader.NextRequestedChange(), second.front());

    reader.RequestedChangesSet({});
    EXPECT_TRUE(reader.RequestedChanges().empty());
    EXPECT_EQ(reader.NextRequestedChange(), SEQUENCENUMBER_UNKNOWN);
    EXPECT_EQ(snapshot.back(), first.back());
}

TEST(ReaderProxyRequests, ConcurrentReplacementAndSnapshotsKeepCompleteRequestSets) {
    ReaderProxy reader(GUID_UNKNOWN, GUID_UNKNOWN, {}, {}, true);
    SeqNumberVec first, second;
    for (unsigned index = 0; index < 32; ++index) {
        first.push_back(seq(101 + index));
        second.push_back(seq(201 + index));
    }
    reader.RequestedChangesSet(first);
    std::promise<void> release;
    auto start = release.get_future().share();
    bool complete = true;
    // This reproduces ACKNACK replacement versus the flow worker's copy.
    // A common start gives both branches the initialized request set but no
    // happens-before edge between their accesses. TSan therefore checks the
    // old unguarded copy even if the scheduler runs the branches in order.
    // Equal sizes preserve capacity, keeping the probe focused on the real
    // element/metadata race rather than relying on a reallocating vector.
    std::thread replacing([&, start] {
        start.wait();
        for (unsigned iteration = 0; iteration < 256; ++iteration)
            reader.RequestedChangesSet(iteration & 1 ? first : second);
    });
    std::thread copying([&, start] {
        start.wait();
        for (unsigned iteration = 0; iteration < 256; ++iteration) {
            const auto snapshot = reader.RequestedChanges();
            if (snapshot != first && snapshot != second) complete = false;
        }
    });
    release.set_value();
    replacing.join();
    copying.join();
    EXPECT_TRUE(complete);
    EXPECT_EQ(reader.RequestedChanges(), first);
}

TEST(ReaderProxyRequests, ConcurrentReplacementAndNextRequestShareSynchronization) {
    ReaderProxy reader(GUID_UNKNOWN, GUID_UNKNOWN, {}, {}, true);
    const SeqNumberVec first{seq(31)}, second{seq(41)};
    reader.RequestedChangesSet(first);
    std::promise<void> release;
    auto start = release.get_future().share();
    bool valid = true;
    // Exercise NextRequestedChange independently: a locked snapshot call in
    // this branch could otherwise supply synchronization that hides an
    // unguarded next-element read from the race detector.
    std::thread replacing([&, start] {
        start.wait();
        for (unsigned iteration = 0; iteration < 256; ++iteration)
            reader.RequestedChangesSet(iteration & 1 ? first : second);
    });
    std::thread inspecting([&, start] {
        start.wait();
        for (unsigned iteration = 0; iteration < 256; ++iteration) {
            const auto next = reader.NextRequestedChange();
            if (next != first.front() && next != second.front()) valid = false;
        }
    });
    release.set_value();
    replacing.join();
    inspecting.join();
    EXPECT_TRUE(valid);
    EXPECT_EQ(reader.NextRequestedChange(), first.front());
}
