#pragma once

#include "PersistenceTypes.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>

TRAVODDS_NAMESPACE_BEGIN

/**
 * @brief 内存缓存管理器，使用LRU（最近最少使用）策略
 * 
 * 这个类提供高效的内存缓存管理，用于存储最近访问的DDS数据样本。
 * 使用LRU算法确保在内存限制下保留最有价值的数据。
 */
class MemoryCache {
private:
    /**
     * @brief 缓存节点结构体
     * 
     * 双向链表节点，用于实现LRU算法的高效访问和更新。
     */
    struct CacheNode {
        std::string key;                               ///< 缓存键
        MemoryCacheItem item;                         ///< 缓存的数据项
        std::shared_ptr<CacheNode> prev, next;       ///< 双向链表指针
        
        /**
         * @brief 缓存节点构造函数
         * 
         * @param cache_key 缓存键
         * @param cache_item 缓存数据项
         */
        CacheNode(std::string cache_key, MemoryCacheItem cache_item) 
            : key(std::move(cache_key)), item(std::move(cache_item)) {}
    };
    
    std::unordered_map<std::string, std::shared_ptr<CacheNode>> cache_map_;  ///< 快速查找的哈希表
    std::shared_ptr<CacheNode> head_, tail_;                                ///< LRU链表的头尾指针
    size_t max_size_bytes_;                                                 ///< 最大缓存大小（字节）
    size_t current_size_bytes_;                                             ///< 当前缓存大小（字节）
    mutable std::mutex cache_mutex_;                                        ///< 线程安全互斥锁
    
    /**
     * @brief 将节点移动到链表头部
     * 
     * 在LRU算法中，最近访问的节点移动到头部。
     * 
     * @param node 要移动的节点
     */
    void move_to_head(const std::shared_ptr<CacheNode>& node);
    
    /**
     * @brief 从链表中移除指定节点
     * 
     * @param node 要移除的节点
     */
    static void remove_node(const std::shared_ptr<CacheNode>& node);
    
    /**
     * @brief 移除并返回链表尾部节点
     * 
     * 在缓存满时，移除最久未使用的节点（尾部节点）。
     * 
     * @return 被移除的尾部节点
     */
    auto remove_tail() -> std::shared_ptr<CacheNode>;
    
    /**
     * @brief 在链表头部添加节点
     * 
     * @param node 要添加的节点
     */
    void add_to_head(const std::shared_ptr<CacheNode>& node);

public:
    /**
     * @brief 构造函数
     * 
     * @param max_size_mb 最大缓存大小（兆字节）
     */
    explicit MemoryCache(size_t max_size_mb);
    
    /**
     * @brief 析构函数
     */
    ~MemoryCache() = default;
    
    /**
     * @brief 存储数据到缓存
     * 
     * 将数据样本存储到内存缓存中，如果缓存满了会根据LRU策略清理旧数据。
     * 
     * @param key 缓存键，通常为topic_name
     * @param data 要缓存的存储数据
     */
    void put(const std::string& key, const StoredData& data);
    
    /**
     * @brief 获取指定Topic的所有缓存数据
     * 
     * @param topic_name Topic名称
     * @return 该Topic的所有存储数据向量
     */
    auto get(const std::string& topic_name) -> std::vector<StoredData>;
    
    /**
     * @brief 获取指定时间范围内的缓存数据
     * 
     * @param topic_name Topic名称
     * @param start_time 开始时间
     * @param end_time 结束时间
     * @return 时间范围内的存储数据向量
     */
    auto get_range(const std::string& topic_name, 
                  const Time_t& start_time, 
                  const Time_t& end_time) -> std::vector<StoredData>;
    
    /**
     * @brief 清理指定Topic的所有缓存数据
     * 
     * @param topic_name 要清理的Topic名称
     */
    void clear_topic(const std::string& topic_name);
    
    /**
     * @brief 清理过期的缓存数据
     * 
     * 根据数据保留时间策略清理过期的缓存项。
     * 
     * @param retention_time 数据保留时间
     */
    void clear_expired(std::chrono::hours retention_time);
    
    /**
     * @brief 获取缓存中的数据项数量
     * 
     * @return 缓存项数量
     */
    auto size() const -> size_t;
    
    /**
     * @brief 获取当前内存使用量
     * 
     * @return 当前内存使用字节数
     */
    auto memory_usage() const -> size_t;
};

TRAVODDS_NAMESPACE_END
