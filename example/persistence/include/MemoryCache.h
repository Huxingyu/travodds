#pragma once

#include "PersistenceTypes.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>

TRAVODDS_NAMESPACE_BEGIN

// 内存缓存管理器 - LRU策略
class MemoryCache {
private:
    struct CacheNode {
        std::string key;
        MemoryCacheItem item;
        std::shared_ptr<CacheNode> prev, next;
        
        CacheNode(std::string cache_key, MemoryCacheItem cache_item) 
            : key(std::move(cache_key)), item(std::move(cache_item)) {}
    };
    
    std::unordered_map<std::string, std::shared_ptr<CacheNode>> cache_map_;
    std::shared_ptr<CacheNode> head_, tail_;
    size_t max_size_bytes_;
    size_t current_size_bytes_;
    mutable std::mutex cache_mutex_;
    
    void move_to_head(const std::shared_ptr<CacheNode>& node);
    static void remove_node(const std::shared_ptr<CacheNode>& node);
    auto remove_tail() -> std::shared_ptr<CacheNode>;
    void add_to_head(const std::shared_ptr<CacheNode>& node);

public:
    explicit MemoryCache(size_t max_size_mb);
    ~MemoryCache() = default;
    
    void put(const std::string& key, const StoredData& data);
    auto get(const std::string& topic_name) -> std::vector<StoredData>;
    auto get_range(const std::string& topic_name, 
                  const Time_t& start_time, 
                  const Time_t& end_time) -> std::vector<StoredData>;
    void clear_topic(const std::string& topic_name);
    void clear_expired(std::chrono::hours retention_time);
    auto size() const -> size_t;
    auto memory_usage() const -> size_t;
};

TRAVODDS_NAMESPACE_END
