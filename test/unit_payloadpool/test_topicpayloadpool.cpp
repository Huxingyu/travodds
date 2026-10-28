#include "gtest/gtest.h"
#include "common/memory/topicpayloadpool.h"
#include "rtps/historycache/historycache.h"
#include <atomic>
#include <cstring>
#include <thread>
#include <limits>

USING_TRAVODDS_NAMESPACE

namespace {
PayloadPoolConfig config(PayloadMemoryPolicy policy, uint32_t slots = 2) {
    PayloadPoolConfig result;
    result.policy = policy;
    result.payload_size = 64;
    result.max_payloads = slots;
    result.alignment = 64;
    return result;
}

class TopicPoolPolicyTest : public testing::TestWithParam<PayloadMemoryPolicy> {};
}

TEST_P(TopicPoolPolicyTest, CapacityFailureDoesNotInvalidateOutstandingSamples) {
    TopicPayloadPool pool(config(GetParam()));
    char* first = nullptr;
    char* second = nullptr;
    char* extra = nullptr;
    ASSERT_TRUE(pool.getBuffer(64, first));
    ASSERT_TRUE(pool.getBuffer(64, second));
    std::memset(first, 0x35, 64);
    std::memset(second, 0x47, 64);
    EXPECT_FALSE(pool.getBuffer(64, extra));
    EXPECT_EQ(extra, nullptr);
    EXPECT_EQ(pool.statistics().in_use, 2u);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(first[i], 0x35);
        EXPECT_EQ(second[i], 0x47);
    }
    EXPECT_TRUE(pool.releaseBuffer(first));
    EXPECT_TRUE(pool.getBuffer(64, extra));
    EXPECT_TRUE(pool.releaseBuffer(second));
    EXPECT_TRUE(pool.releaseBuffer(extra));
    EXPECT_EQ(pool.statistics().in_use, 0u);
}

TEST_P(TopicPoolPolicyTest, ProvidesRequestedAlignment) {
    TopicPayloadPool pool(config(GetParam()));
    char* buffer = nullptr;
    ASSERT_TRUE(pool.getBuffer(64, buffer));
    EXPECT_EQ(reinterpret_cast<uintptr_t>(buffer) % 64, 0u);
    EXPECT_TRUE(pool.releaseBuffer(buffer));
}

TEST_P(TopicPoolPolicyTest, RejectsForeignInteriorAndDoubleReturns) {
    TopicPayloadPool pool(config(GetParam()));
    TopicPayloadPool other(config(GetParam()));
    char* buffer = nullptr;
    ASSERT_TRUE(pool.getBuffer(64, buffer));
    char* foreign = buffer;
    EXPECT_FALSE(other.releaseBuffer(foreign));
    EXPECT_EQ(foreign, buffer);
    char* interior = buffer + 1;
    EXPECT_FALSE(pool.releaseBuffer(interior));
    EXPECT_FALSE(pool.owns(interior));
    EXPECT_TRUE(pool.owns(buffer));
    EXPECT_TRUE(pool.releaseBuffer(buffer));
    EXPECT_EQ(buffer, nullptr);
    EXPECT_FALSE(pool.owns(foreign));
    EXPECT_FALSE(pool.releaseBuffer(foreign));
    EXPECT_FALSE(pool.releaseBuffer(buffer));
}

TEST_P(TopicPoolPolicyTest, CacheChangeReturnsToTopicPoolAndKeepsItAlive) {
    auto pool = std::make_shared<TopicPayloadPool>(config(GetParam()));
    std::weak_ptr<ITopicPayloadPool> weak = pool;
    auto change = std::make_unique<CacheChange>(pool);
    change->serializedBuffer = new SerializedBuffer;
    ASSERT_TRUE(pool->getBuffer(64, change->serializedBuffer->origin_buffer));
    pool.reset();
    ASSERT_FALSE(weak.expired());
    change.reset();
    EXPECT_TRUE(weak.expired());
}

TEST_P(TopicPoolPolicyTest, ConcurrentUsersNeverReceiveTheSameLiveAllocation) {
    TopicPayloadPool pool(config(GetParam(), 8));
    std::mutex liveMutex;
    std::set<char*> live;
    std::atomic<int> errors{0};
    std::vector<std::thread> workers;
    for (int worker = 0; worker < 8; ++worker) {
        workers.emplace_back([&, worker] {
            for (int iteration = 0; iteration < 500; ++iteration) {
                char* buffer = nullptr;
                while (!pool.getBuffer(64, buffer)) {
                    std::this_thread::yield();
                }
                {
                    std::lock_guard<std::mutex> lock(liveMutex);
                    if (!live.insert(buffer).second) {
                        ++errors;
                    }
                }
                std::memset(buffer, worker + 1, 64);
                std::this_thread::yield();
                for (int i = 0; i < 64; ++i) {
                    if (buffer[i] != worker + 1) {
                        ++errors;
                    }
                }
                {
                    std::lock_guard<std::mutex> lock(liveMutex);
                    if (live.erase(buffer) != 1 || !pool.releaseBuffer(buffer)) {
                        ++errors;
                    }
                }
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }
    EXPECT_EQ(errors.load(), 0);
    EXPECT_TRUE(live.empty());
    EXPECT_EQ(pool.statistics().in_use, 0u);
}

INSTANTIATE_TEST_SUITE_P(AllPolicies, TopicPoolPolicyTest, testing::Values(
    PayloadMemoryPolicy::PREALLOCATED,
    PayloadMemoryPolicy::PREALLOCATED_WITH_REALLOC,
    PayloadMemoryPolicy::DYNAMIC_RESERVE,
    PayloadMemoryPolicy::DYNAMIC_REUSABLE));

TEST(TopicPool, FixedPreallocationRejectsOversizedPayloadsWithoutGrowing) {
    PreallocatedPayloadPool pool(config(PayloadMemoryPolicy::PREALLOCATED));
    EXPECT_EQ(pool.statistics().allocated_payloads, 2u);
    EXPECT_EQ(pool.statistics().allocated_bytes, 128u);
    char* buffer = nullptr;
    EXPECT_FALSE(pool.getBuffer(65, buffer));
    EXPECT_EQ(buffer, nullptr);
    EXPECT_EQ(pool.statistics().allocated_bytes, 128u);
}

TEST(TopicPool, ReallocationGrowsOnlyReturnedStorage) {
    PreallocatedWithReallocPayloadPool pool(config(PayloadMemoryPolicy::PREALLOCATED_WITH_REALLOC));
    char* pinned = nullptr;
    char* growing = nullptr;
    ASSERT_TRUE(pool.getBuffer(64, pinned));
    std::memset(pinned, 0x29, 64);
    ASSERT_TRUE(pool.getBuffer(4096, growing));
    EXPECT_EQ(pool.statistics().allocated_bytes, 4160u);
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(pinned[i], 0x29);
    }
    EXPECT_TRUE(pool.releaseBuffer(growing));
    EXPECT_TRUE(pool.releaseBuffer(pinned));
}

TEST(TopicPool, DynamicReserveReleasesStorageAndCanReuseSlotMetadata) {
    DynamicReservePayloadPool pool(config(PayloadMemoryPolicy::DYNAMIC_RESERVE, 1));
    EXPECT_EQ(pool.statistics().allocated_payloads, 0u);
    for (int i = 0; i < 100; ++i) {
        char* buffer = nullptr;
        ASSERT_TRUE(pool.getBuffer(4096, buffer));
        EXPECT_EQ(pool.statistics().allocated_payloads, 1u);
        ASSERT_TRUE(pool.releaseBuffer(buffer));
        EXPECT_EQ(pool.statistics().allocated_payloads, 0u);
        EXPECT_EQ(pool.statistics().allocated_bytes, 0u);
    }
}

TEST(TopicPool, DynamicReusableRetainsReturnedStorage) {
    auto cfg = config(PayloadMemoryPolicy::DYNAMIC_REUSABLE, 1);
    cfg.initial_payloads = 1;
    DynamicReusablePayloadPool pool(cfg);
    EXPECT_EQ(pool.statistics().allocated_payloads, 1u);
    char* buffer = nullptr;
    ASSERT_TRUE(pool.getBuffer(32, buffer));
    char* saved = buffer;
    ASSERT_TRUE(pool.releaseBuffer(buffer));
    EXPECT_EQ(pool.statistics().allocated_bytes, 64u);
    ASSERT_TRUE(pool.getBuffer(64, buffer));
    EXPECT_EQ(buffer, saved);
    EXPECT_TRUE(pool.releaseBuffer(buffer));
}

TEST(TopicPool, EmptyPayloadStillGetsAnOwnedReturnableAllocation) {
    auto cfg = config(PayloadMemoryPolicy::DYNAMIC_REUSABLE, 1);
    cfg.payload_size = 0;
    TopicPayloadPool pool(cfg);
    char* buffer = nullptr;
    ASSERT_TRUE(pool.getBuffer(0, buffer));
    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(pool.statistics().allocated_bytes, 1u);
    EXPECT_TRUE(pool.releaseBuffer(buffer));
}

TEST(TopicPool, RejectsInvalidConfiguration) {
    auto cfg = config(PayloadMemoryPolicy::DYNAMIC_REUSABLE);
    cfg.max_payloads = 0;
    EXPECT_THROW(TopicPayloadPool{cfg}, std::invalid_argument);
    cfg.max_payloads = 2;
    cfg.initial_payloads = 3;
    EXPECT_THROW(TopicPayloadPool{cfg}, std::invalid_argument);
    cfg.initial_payloads = 0;
    cfg.alignment = 3;
    EXPECT_THROW(TopicPayloadPool{cfg}, std::invalid_argument);
    cfg.alignment = 0;
    EXPECT_THROW(TopicPayloadPool{cfg}, std::invalid_argument);
    cfg.alignment = 64;
    cfg.policy = static_cast<PayloadMemoryPolicy>(99);
    EXPECT_THROW(TopicPayloadPool{cfg}, std::invalid_argument);
    cfg.policy = PayloadMemoryPolicy::PREALLOCATED;
    cfg.payload_size = 0;
    EXPECT_THROW(TopicPayloadPool{cfg}, std::invalid_argument);
    cfg.policy = PayloadMemoryPolicy::DYNAMIC_RESERVE;
    cfg.initial_payloads = 1;
    EXPECT_THROW(TopicPayloadPool{cfg}, std::invalid_argument);
}

TEST(TopicPoolRegistry, SharesOnlyMatchingTopicDomainTypeAndConfiguration) {
    TopicPayloadPoolRegistry registry;
    auto cfg = config(PayloadMemoryPolicy::DYNAMIC_REUSABLE);
    TopicPayloadPoolKey key{1, "camera", "Image"};
    auto pool = registry.getOrCreate(key, cfg);
    EXPECT_EQ(registry.getOrCreate(key, cfg), pool);
    EXPECT_NE(registry.getOrCreate({2, "camera", "Image"}, cfg), pool);
    EXPECT_NE(registry.getOrCreate({1, "depth", "Image"}, cfg), pool);
    EXPECT_NE(registry.getOrCreate({1, "camera", "PointCloud"}, cfg), pool);
    cfg.max_payloads++;
    EXPECT_THROW(registry.getOrCreate(key, cfg), std::invalid_argument);
    EXPECT_EQ(registry.find(key), pool);
}

TEST(TopicPoolRegistry, ExpiredRegistrationDoesNotPinPoolOrConfiguration) {
    TopicPayloadPoolRegistry registry;
    auto cfg = config(PayloadMemoryPolicy::DYNAMIC_REUSABLE);
    TopicPayloadPoolKey key{2, "camera", "Image"};
    std::weak_ptr<ITopicPayloadPool> weak;
    {
        auto pool = registry.getOrCreate(key, cfg);
        weak = pool;
    }
    EXPECT_TRUE(weak.expired());
    EXPECT_EQ(registry.find(key), nullptr);
    cfg.max_payloads++;
    auto next = registry.getOrCreate(key, cfg);
    EXPECT_EQ(next->configuration().max_payloads, cfg.max_payloads);
    next.reset();
    EXPECT_EQ(registry.prune(), 1u);
    EXPECT_EQ(registry.prune(), 0u);
}

TEST(TopicPoolRegistry, RejectsUnnamedTopicsAndTypes) {
    TopicPayloadPoolRegistry registry;
    auto cfg = config(PayloadMemoryPolicy::DYNAMIC_REUSABLE);
    EXPECT_THROW(registry.getOrCreate({0, "", "Image"}, cfg), std::invalid_argument);
    EXPECT_THROW(registry.getOrCreate({0, "camera", ""}, cfg), std::invalid_argument);
}

TEST(TopicPoolRegistry, ConcurrentRegistrationCreatesOneSharedPool) {
    TopicPayloadPoolRegistry registry;
    const auto cfg = config(PayloadMemoryPolicy::DYNAMIC_REUSABLE);
    const TopicPayloadPoolKey key{3, "camera", "Image"};
    std::vector<std::shared_ptr<ITopicPayloadPool>> pools(16);
    std::vector<std::thread> workers;
    for (size_t i = 0; i < pools.size(); ++i) {
        workers.emplace_back([&, i] { pools[i] = registry.getOrCreate(key, cfg); });
    }
    for (auto& worker : workers) {
        worker.join();
    }
    for (const auto& pool : pools) {
        EXPECT_EQ(pool, pools.front());
    }
}

TEST(TopicPool, LargeDynamicLimitAllocatesOnlyDemandedMetadataAndStorage) {
    auto cfg = config(PayloadMemoryPolicy::DYNAMIC_REUSABLE);
    cfg.max_payloads = std::numeric_limits<uint32_t>::max();
    TopicPayloadPool pool(cfg);
    EXPECT_EQ(pool.statistics().allocated_payloads, 0u);
    std::vector<char*> buffers(257, nullptr);
    for (auto& buffer : buffers) {
        ASSERT_TRUE(pool.getBuffer(64, buffer));
    }
    EXPECT_EQ(pool.statistics().in_use, buffers.size());
    for (auto& buffer : buffers) {
        EXPECT_TRUE(pool.releaseBuffer(buffer));
    }
    EXPECT_EQ(pool.statistics().in_use, 0u);
}
