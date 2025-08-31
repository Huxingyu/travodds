#include "MemoryCache.h"
#include <algorithm>
#include <iostream>

AGIDDS_NAMESPACE_BEGIN

namespace {
    constexpr size_t MB_TO_BYTES = 1024 * 1024;
    constexpr int64_t MICROSECONDS_PER_SECOND = 1000000LL;
    constexpr int64_t NANOSECONDS_PER_MICROSECOND = 1000;
}

MemoryCache::MemoryCache(size_t max_size_mb) 
    : max_size_bytes_(max_size_mb * MB_TO_BYTES), current_size_bytes_(0) {
    // 创建哑节点以简化LRU操作
    head_ = std::make_shared<CacheNode>("", MemoryCacheItem(StoredData{}));
    tail_ = std::make_shared<CacheNode>("", MemoryCacheItem(StoredData{}));
    head_->next = tail_;
    tail_->prev = head_;
}

void MemoryCache::put(const std::string& key, const StoredData& data) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto iterator = cache_map_.find(key);
    if (iterator != cache_map_.end()) {
        // 更新现有节点
        iterator->second->item = MemoryCacheItem(data);
        move_to_head(iterator->second);
        return;
    }
    
    // 创建新节点
    auto new_node = std::make_shared<CacheNode>(key, MemoryCacheItem(data));
    cache_map_[key] = new_node;
    add_to_head(new_node);
    current_size_bytes_ += data.data.size();
    
    // 检查容量限制
    while (current_size_bytes_ > max_size_bytes_ && !cache_map_.empty()) {
        auto tail_node = remove_tail();
        if (tail_node && tail_node != head_ && tail_node != tail_) {
            current_size_bytes_ -= tail_node->item.data.data.size();
            cache_map_.erase(tail_node->key);
        }
    }
}

auto MemoryCache::get(const std::string& topic_name) -> std::vector<StoredData> {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::vector<StoredData> result;
    
    for (const auto& [key, node] : cache_map_) {
        if (node->item.data.topic_name == topic_name) {
            result.push_back(node->item.data);
            move_to_head(node);
        }
    }
    
    // 按序列号排序
    std::sort(result.begin(), result.end(), 
        [](const StoredData& first, const StoredData& second) {
            return first.sequence_number < second.sequence_number;
        });
    
    return result;
}

auto MemoryCache::get_range(const std::string& topic_name, 
                           const Time_t& start_time, 
                           const Time_t& end_time) -> std::vector<StoredData> {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::vector<StoredData> result;
    
    int64_t start_us = start_time.sec * MICROSECONDS_PER_SECOND + start_time.nanosec / NANOSECONDS_PER_MICROSECOND;
    int64_t end_us = end_time.sec * MICROSECONDS_PER_SECOND + end_time.nanosec / NANOSECONDS_PER_MICROSECOND;
    
    for (const auto& [key, node] : cache_map_) {
        if (node->item.data.topic_name == topic_name) {
            int64_t data_time = node->item.data.sample_info.source_timestamp.sec * MICROSECONDS_PER_SECOND + 
                               node->item.data.sample_info.source_timestamp.nanosec / NANOSECONDS_PER_MICROSECOND;
            
            if (data_time >= start_us && data_time <= end_us) {
                result.push_back(node->item.data);
                move_to_head(node);
            }
        }
    }
    
    std::sort(result.begin(), result.end(), 
        [](const StoredData& first, const StoredData& second) {
            return first.sequence_number < second.sequence_number;
        });
    
    return result;
}

void MemoryCache::clear_topic(const std::string& topic_name) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto iterator = cache_map_.begin();
    while (iterator != cache_map_.end()) {
        if (iterator->second->item.data.topic_name == topic_name) {
            current_size_bytes_ -= iterator->second->item.data.data.size();
            remove_node(iterator->second);
            iterator = cache_map_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

void MemoryCache::clear_expired(std::chrono::hours retention_time) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    auto iterator = cache_map_.begin();
    while (iterator != cache_map_.end()) {
        auto age = now - iterator->second->item.timestamp;
        if (age > retention_time) {
            current_size_bytes_ -= iterator->second->item.data.data.size();
            remove_node(iterator->second);
            iterator = cache_map_.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

auto MemoryCache::size() const -> size_t {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_map_.size();
}

auto MemoryCache::memory_usage() const -> size_t {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return current_size_bytes_;
}

void MemoryCache::move_to_head(const std::shared_ptr<CacheNode>& node) {
    remove_node(node);
    add_to_head(node);
}

void MemoryCache::remove_node(const std::shared_ptr<CacheNode>& node) {
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
}

auto MemoryCache::remove_tail() -> std::shared_ptr<CacheNode> {
    auto last = tail_->prev;
    if (last && last != head_) {
        remove_node(last);
        return last;
    }
    return nullptr;
}

void MemoryCache::add_to_head(const std::shared_ptr<CacheNode>& node) {
    node->prev = head_;
    node->next = head_->next;
    if (head_->next) head_->next->prev = node;
    head_->next = node;
}

AGIDDS_NAMESPACE_END
