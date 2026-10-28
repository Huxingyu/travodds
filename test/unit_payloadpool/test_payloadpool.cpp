#include "gtest/gtest.h"
#include "common/memory/memorypool.h"
#include "rtps/historycache/historycache.h"

USING_TRAVODDS_NAMESPACE

namespace {
class CountingPool : public IPayloadPool {
public:
    bool getBuffer(uint32_t size, char*& buffer) override {
        buffer = new char[size];
        return true;
    }
    bool releaseBuffer(char*& buffer) override {
        if (!buffer) {
            return false;
        }
        delete[] buffer;
        buffer = nullptr;
        ++releases;
        return true;
    }
    int releases = 0;
};

SerializedBuffer* allocateBuffer(IPayloadPool& pool) {
    auto buffer = std::make_unique<SerializedBuffer>();
    if (!pool.getBuffer(64, buffer->origin_buffer)) {
        return nullptr;
    }
    buffer->buffer = buffer->origin_buffer;
    buffer->buffer_size = 64;
    return buffer.release();
}
}

TEST(PayloadPool, LegacyPoolReusesReturnedStorageThroughInterface) {
    std::shared_ptr<IPayloadPool> pool = std::make_shared<MemoryPool>(1);
    char* buffer = nullptr;
    ASSERT_TRUE(pool->getBuffer(64, buffer));
    char* first = buffer;
    ASSERT_TRUE(pool->releaseBuffer(buffer));
    EXPECT_EQ(buffer, nullptr);
    ASSERT_TRUE(pool->getBuffer(64, buffer));
    EXPECT_EQ(buffer, first);
    EXPECT_TRUE(pool->releaseBuffer(buffer));
}

TEST(PayloadPool, ExhaustedLegacyPoolReportsFailureAndRecovers) {
    std::shared_ptr<IPayloadPool> pool = std::make_shared<MemoryPool>(1);
    char* first = nullptr;
    char* second = nullptr;
    ASSERT_TRUE(pool->getBuffer(64, first));
    EXPECT_FALSE(pool->getBuffer(64, second));
    EXPECT_EQ(second, nullptr);
    ASSERT_TRUE(pool->releaseBuffer(first));
    ASSERT_TRUE(pool->getBuffer(128, second));
    EXPECT_TRUE(pool->releaseBuffer(second));
}

TEST(PayloadPool, CacheChangeReturnsEachChainedBufferToOriginPool) {
    // new[] storage has no legacy MemoryPool metadata: using the global pool
    // to reclaim it would be invalid, so this also checks pool routing.
    auto pool = std::make_shared<CountingPool>();
    {
        CacheChange change(pool);
        change.serializedBuffer = allocateBuffer(*pool);
        ASSERT_NE(change.serializedBuffer, nullptr);
        change.serializedBuffer->next = allocateBuffer(*pool);
        ASSERT_NE(change.serializedBuffer->next, nullptr);
        EXPECT_EQ(pool->releases, 0);
    }
    EXPECT_EQ(pool->releases, 2);
}

TEST(PayloadPool, CacheChangeKeepsPoolAliveUntilPayloadIsReturned) {
    auto pool = std::make_shared<CountingPool>();
    std::weak_ptr<IPayloadPool> weak = pool;
    auto change = std::make_unique<CacheChange>(pool);
    change->serializedBuffer = allocateBuffer(*pool);
    ASSERT_NE(change->serializedBuffer, nullptr);
    pool.reset();
    EXPECT_FALSE(weak.expired());
    change.reset();
    EXPECT_TRUE(weak.expired());
}

TEST(PayloadPool, CacheChangeWithoutPayloadDoesNotReturnStorage) {
    auto pool = std::make_shared<CountingPool>();
    { CacheChange change(pool); }
    EXPECT_EQ(pool->releases, 0);
}

TEST(PayloadPool, CacheChangePreservesLegacyDefaultPool) {
    auto pool = MemoryPool::GetInstance();
    char* storage = nullptr;
    {
        CacheChange change;
        change.serializedBuffer = allocateBuffer(*pool);
        ASSERT_NE(change.serializedBuffer, nullptr);
        storage = change.serializedBuffer->origin_buffer;
        EXPECT_TRUE(BufferNode::info(reinterpret_cast<octet*>(storage)).onUsing);
    }
    // The legacy pool retains its allocation after return, so metadata remains
    // readable here. Existing endpoint code needs no ownership override.
    EXPECT_FALSE(BufferNode::info(reinterpret_cast<octet*>(storage)).onUsing);
}

TEST(PayloadPool, CacheChangeRejectsMissingPool) {
    EXPECT_THROW(CacheChange{std::shared_ptr<IPayloadPool>{}}, std::invalid_argument);
}
