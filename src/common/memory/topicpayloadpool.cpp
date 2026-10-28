// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include "common/memory/topicpayloadpool.h"
#include <algorithm>
#include <new>
#include <stdexcept>

USING_TRAVODDS_NAMESPACE

namespace {
PayloadPoolConfig withPolicy(PayloadPoolConfig config, PayloadMemoryPolicy policy) {
    config.policy = policy;
    return config;
}

bool preallocated(PayloadMemoryPolicy policy) {
    return policy == PayloadMemoryPolicy::PREALLOCATED ||
           policy == PayloadMemoryPolicy::PREALLOCATED_WITH_REALLOC;
}

void validate(const PayloadPoolConfig& config) {
    switch (config.policy) {
        case PayloadMemoryPolicy::PREALLOCATED:
        case PayloadMemoryPolicy::PREALLOCATED_WITH_REALLOC:
        case PayloadMemoryPolicy::DYNAMIC_RESERVE:
        case PayloadMemoryPolicy::DYNAMIC_REUSABLE:
            break;
        default:
            throw std::invalid_argument("Unknown payload memory policy");
    }
    if (!config.max_payloads || config.initial_payloads > config.max_payloads ||
        config.alignment < alignof(void*) ||
        (config.alignment & (config.alignment - 1)) != 0 ||
        (preallocated(config.policy) && !config.payload_size) ||
        (config.policy == PayloadMemoryPolicy::DYNAMIC_RESERVE && config.initial_payloads)) {
        throw std::invalid_argument("Invalid payload pool capacity/alignment");
    }
}
}

bool PayloadPoolConfig::operator==(const PayloadPoolConfig& other) const {
    return policy == other.policy && payload_size == other.payload_size &&
           max_payloads == other.max_payloads && initial_payloads == other.initial_payloads &&
           alignment == other.alignment;
}

TopicPayloadPool::TopicPayloadPool(const PayloadPoolConfig& config) : config_(config) {
    validate(config_);
    const uint32_t initial = preallocated(config_.policy) ? config_.max_payloads : config_.initial_payloads;
    blocks_.reserve(initial);
    available_.reserve(initial);
    addresses_.reserve(initial);
    try {
        for (uint32_t i = 0; i < initial; ++i) {
            blocks_.push_back(Block{});
            if (!replaceFreeBlock(i, std::max(1u, config_.payload_size))) {
                throw std::bad_alloc();
            }
            available_.push_back(i);
        }
    } catch (...) {
        for (const auto& block : blocks_) {
            deallocate(block.data);
        }
        throw;
    }
}

TopicPayloadPool::~TopicPayloadPool() {
    for (const auto& block : blocks_) {
        deallocate(block.data);
    }
}

char* TopicPayloadPool::allocate(uint32_t capacity) const {
    return static_cast<char*>(::operator new(capacity, std::align_val_t(config_.alignment), std::nothrow));
}

void TopicPayloadPool::deallocate(char* buffer) const {
    ::operator delete(buffer, std::align_val_t(config_.alignment));
}

bool TopicPayloadPool::replaceFreeBlock(size_t index, uint32_t capacity) {
    auto& block = blocks_[index];
    char* replacement = allocate(capacity);
    if (!replacement) {
        return false;
    }
    try {
        addresses_.emplace(replacement, index);
    } catch (const std::bad_alloc&) {
        deallocate(replacement);
        return false;
    }
    addresses_.erase(block.data);
    deallocate(block.data);
    block.data = replacement;
    block.capacity = capacity;
    return true;
}

bool TopicPayloadPool::getBuffer(uint32_t size, char*& buffer) {
    buffer = nullptr;
    const uint32_t requested = std::max(1u, size);
    std::lock_guard<std::mutex> lock(mutex_);
    if (config_.policy == PayloadMemoryPolicy::PREALLOCATED && requested > config_.payload_size) {
        return false;
    }
    size_t position = available_.size();
    for (size_t i = 0; i < available_.size(); ++i) {
        const auto capacity = blocks_[available_[i]].capacity;
        if (capacity >= requested &&
            (position == available_.size() || capacity < blocks_[available_[position]].capacity)) {
            position = i;
        }
    }
    if (position == available_.size() && !available_.empty()) {
        // Every free block is undersized. Grow one, leaving active samples alone.
        position = available_.size() - 1;
    }
    size_t index;
    if (position != available_.size()) {
        index = available_[position];
        if (blocks_[index].capacity < requested &&
            !replaceFreeBlock(index, std::max(requested, config_.payload_size))) {
            return false;
        }
        available_[position] = available_.back();
        available_.pop_back();
    } else {
        if (blocks_.size() == config_.max_payloads) {
            return false;
        }
        index = blocks_.size();
        try {
            // Reserve return-stack space before lending another block. Even a
            // large configured upper bound must not allocate all metadata up
            // front, and releasing the last live samples must never allocate.
            if (available_.capacity() <= index) {
                const size_t capacity = std::min<size_t>(config_.max_payloads,
                    std::max<size_t>(8, available_.capacity() * 2));
                available_.reserve(capacity);
            }
            blocks_.push_back(Block{});
        } catch (const std::bad_alloc&) {
            return false;
        } catch (const std::length_error&) {
            return false;
        }
        if (!replaceFreeBlock(index, std::max(requested, config_.payload_size))) {
            blocks_.pop_back();
            return false;
        }
    }
    blocks_[index].in_use = true;
    buffer = blocks_[index].data;
    return true;
}

bool TopicPayloadPool::releaseBuffer(char*& buffer) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = addresses_.find(buffer);
    if (found == addresses_.end() || !blocks_[found->second].in_use) {
        return false;
    }
    const auto index = found->second;
    auto& block = blocks_[index];
    block.in_use = false;
    if (config_.policy == PayloadMemoryPolicy::DYNAMIC_RESERVE) {
        addresses_.erase(found);
        deallocate(block.data);
        block.data = nullptr;
        block.capacity = 0;
    }
    available_.push_back(index);
    buffer = nullptr;
    return true;
}

bool TopicPayloadPool::owns(const char* buffer) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = addresses_.find(buffer);
    return found != addresses_.end() && blocks_[found->second].in_use;
}

const PayloadPoolConfig& TopicPayloadPool::configuration() const {
    return config_;
}

PayloadPoolStats TopicPayloadPool::statistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    PayloadPoolStats result;
    for (const auto& block : blocks_) {
        if (block.data) {
            ++result.allocated_payloads;
            result.allocated_bytes += block.capacity;
        }
        result.in_use += block.in_use;
    }
    return result;
}

PreallocatedPayloadPool::PreallocatedPayloadPool(PayloadPoolConfig config)
    : TopicPayloadPool(withPolicy(config, PayloadMemoryPolicy::PREALLOCATED)) {}
PreallocatedWithReallocPayloadPool::PreallocatedWithReallocPayloadPool(PayloadPoolConfig config)
    : TopicPayloadPool(withPolicy(config, PayloadMemoryPolicy::PREALLOCATED_WITH_REALLOC)) {}
DynamicReservePayloadPool::DynamicReservePayloadPool(PayloadPoolConfig config)
    : TopicPayloadPool(withPolicy(config, PayloadMemoryPolicy::DYNAMIC_RESERVE)) {}
DynamicReusablePayloadPool::DynamicReusablePayloadPool(PayloadPoolConfig config)
    : TopicPayloadPool(withPolicy(config, PayloadMemoryPolicy::DYNAMIC_REUSABLE)) {}

TopicPayloadPoolRegistry& TopicPayloadPoolRegistry::instance() {
    static TopicPayloadPoolRegistry registry;
    return registry;
}

std::shared_ptr<ITopicPayloadPool> TopicPayloadPoolRegistry::getOrCreate(
        const TopicPayloadPoolKey& key, const PayloadPoolConfig& config) {
    validate(config);
    if (key.topic_name.empty() || key.type_name.empty()) {
        throw std::invalid_argument("Topic pool requires topic and type names");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto found = pools_.find(key);
    if (found != pools_.end()) {
        if (auto pool = found->second.lock()) {
            if (!(pool->configuration() == config)) {
                throw std::invalid_argument("Active topic pool has incompatible configuration");
            }
            return pool;
        }
    }
    auto pool = std::make_shared<TopicPayloadPool>(config);
    pools_[key] = pool;
    return pool;
}

std::shared_ptr<ITopicPayloadPool> TopicPayloadPoolRegistry::find(const TopicPayloadPoolKey& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = pools_.find(key);
    return found == pools_.end() ? nullptr : found->second.lock();
}

size_t TopicPayloadPoolRegistry::prune() {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t removed = 0;
    for (auto it = pools_.begin(); it != pools_.end();) {
        if (it->second.expired()) {
            it = pools_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}
