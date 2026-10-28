// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#ifndef TRAVODDS_COMMON_MEMORY_TOPICPAYLOADPOOL_H
#define TRAVODDS_COMMON_MEMORY_TOPICPAYLOADPOOL_H

#include "common/memory/payloadpool.h"
#include "qos/payloadpoolqospolicy.h"
#include <cstddef>
#include <tuple>
#include <unordered_map>

TRAVODDS_NAMESPACE_BEGIN

struct PayloadPoolConfig {
    PayloadMemoryPolicy policy = PayloadMemoryPolicy::DYNAMIC_REUSABLE;
    uint32_t payload_size = 0;
    uint32_t max_payloads = 64;
    uint32_t initial_payloads = 0;
    size_t alignment = alignof(std::max_align_t);

    bool operator==(const PayloadPoolConfig& other) const;
};

struct PayloadPoolStats {
    size_t allocated_payloads = 0;
    size_t in_use = 0;
    size_t allocated_bytes = 0;
};

class ITopicPayloadPool : public IPayloadPool {
public:
    virtual const PayloadPoolConfig& configuration() const = 0;
    virtual PayloadPoolStats statistics() const = 0;
    // Exact, currently allocated addresses only. Reused addresses cannot detect
    // stale aliases; the Loan layer must additionally validate generations.
    virtual bool owns(const char* buffer) const = 0;
};

/**
 * Thread-safe, aligned payload pool. Preallocated policies allocate all slots
 * at construction. Reallocation only affects free slots. DYNAMIC_RESERVE frees
 * payload storage on return; DYNAMIC_REUSABLE retains it for subsequent writes.
 * Dynamic metadata grows with demand up to max_payloads. Returning storage
 * allocates no memory. Callers retain the pool while holding buffers.
 */
class TopicPayloadPool : public ITopicPayloadPool {
public:
    explicit TopicPayloadPool(const PayloadPoolConfig& config);
    ~TopicPayloadPool() override;
    TopicPayloadPool(const TopicPayloadPool&) = delete;
    TopicPayloadPool& operator=(const TopicPayloadPool&) = delete;

    bool getBuffer(uint32_t size, char*& buffer) override;
    bool releaseBuffer(char*& buffer) override;
    bool owns(const char* buffer) const override;
    const PayloadPoolConfig& configuration() const override;
    PayloadPoolStats statistics() const override;

private:
    struct Block {
        char* data = nullptr;
        uint32_t capacity = 0;
        bool in_use = false;
    };

    bool replaceFreeBlock(size_t index, uint32_t capacity);
    char* allocate(uint32_t capacity) const;
    void deallocate(char* buffer) const;

    const PayloadPoolConfig config_;
    mutable std::mutex mutex_;
    std::vector<Block> blocks_;
    std::vector<size_t> available_;
    std::unordered_map<const char*, size_t> addresses_;
};

class PreallocatedPayloadPool final : public TopicPayloadPool {
public:
    explicit PreallocatedPayloadPool(PayloadPoolConfig config);
};
class PreallocatedWithReallocPayloadPool final : public TopicPayloadPool {
public:
    explicit PreallocatedWithReallocPayloadPool(PayloadPoolConfig config);
};
class DynamicReservePayloadPool final : public TopicPayloadPool {
public:
    explicit DynamicReservePayloadPool(PayloadPoolConfig config);
};
class DynamicReusablePayloadPool final : public TopicPayloadPool {
public:
    explicit DynamicReusablePayloadPool(PayloadPoolConfig config);
};

struct TopicPayloadPoolKey {
    uint32_t domain_id = 0;
    std::string topic_name;
    std::string type_name;

    bool operator<(const TopicPayloadPoolKey& other) const {
        return std::tie(domain_id, topic_name, type_name) <
               std::tie(other.domain_id, other.topic_name, other.type_name);
    }
};

/** Weak registration never extends the lifetime of an unused Topic pool. */
class TopicPayloadPoolRegistry {
public:
    static TopicPayloadPoolRegistry& instance();
    std::shared_ptr<ITopicPayloadPool> getOrCreate(
        const TopicPayloadPoolKey& key, const PayloadPoolConfig& config);
    std::shared_ptr<ITopicPayloadPool> find(const TopicPayloadPoolKey& key);
    size_t prune();

private:
    std::mutex mutex_;
    std::map<TopicPayloadPoolKey, std::weak_ptr<ITopicPayloadPool>> pools_;
};

TRAVODDS_NAMESPACE_END
#endif
