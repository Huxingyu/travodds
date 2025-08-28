#pragma once

#include "dcps/domain/domainparticipant.h"
#include "dcps/subscribe/datareader.h"
#include "dcps/publish/datawriter.h"
#include "type/basictype.h"
#include "type/builtintopicdata.h"
#include <memory>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <future>
#include <condition_variable>
#include <queue>
#include <algorithm>
#include <sqlite3.h>

AGIDDS_NAMESPACE_BEGIN

// Qt 可视化回调接口（便于后续Qt集成）
class IPersistenceUICallback {
public:
    virtual ~IPersistenceUICallback() = default;
    virtual void on_writer_discovered(const std::string& topic, const std::string& type) = 0;
    virtual void on_reader_discovered(const std::string& topic, const std::string& type) = 0;
    virtual void on_data_stored(const std::string& topic, size_t data_size) = 0;
    virtual void on_historical_data_sent(const std::string& topic, size_t count) = 0;
    virtual void on_statistics_updated() = 0;
};

// 不同持久化策略的配置
struct DurabilityConfig {
    std::chrono::hours data_retention{24};       // 数据保留时间
    size_t max_samples_per_topic = 10000;        // 每个Topic最大样本数
    size_t memory_cache_size_mb = 100;           // 内存缓存大小(MB)
    bool enable_database_storage = true;         // 是否启用数据库存储
    bool enable_compression = false;              // 是否启用压缩
};

// 持久化配置
struct PersistenceConfig {
    std::string db_path = "persistence.db";
    std::chrono::seconds cleanup_interval{300};  // 5分钟清理一次
    size_t max_db_size_mb = 1024;                // 最大数据库大小1GB
    bool enable_statistics = true;               // 是否启用统计信息
    int worker_threads = 2;                      // 工作线程数量
    
    // 通用配置
    std::chrono::hours data_retention{24};       // 默认数据保留时间
    size_t max_samples_per_topic = 1000;         // 默认每个Topic最大样本数
    bool enable_compression = false;             // 默认是否启用压缩
    
    // 各种持久化策略的配置
    DurabilityConfig volatile_config{
        std::chrono::hours{0},    // 不保留数据
        0,                        // 不限制样本数
        0,                        // 不使用内存缓存
        false,                    // 不使用数据库
        false                     // 不压缩
    };
    
    DurabilityConfig transient_local_config{
        std::chrono::hours{1},    // 1小时内存保留
        1000,                     // 每Topic 1000样本
        50,                       // 50MB内存缓存
        false,                    // 不使用数据库存储
        false                     // 不压缩
    };
    
    DurabilityConfig transient_config{
        std::chrono::hours{6},    // 6小时数据保留
        5000,                     // 每Topic 5000样本
        20,                       // 20MB内存缓存
        true,                     // 使用数据库存储
        true                      // 启用压缩
    };
    
    DurabilityConfig persistent_config{
        std::chrono::hours{168},  // 7天数据保留
        50000,                    // 每Topic 50000样本
        10,                       // 10MB内存缓存
        true,                     // 使用数据库存储
        true                      // 启用压缩
    };
};

// 持久化统计信息
struct PersistenceStatistics {
    std::atomic<uint64_t> total_samples_stored{0};
    std::atomic<uint64_t> total_samples_served{0};
    std::atomic<uint64_t> total_bytes_stored{0};
    std::atomic<uint64_t> total_bytes_served{0};
    std::atomic<uint32_t> active_writers{0};
    std::atomic<uint32_t> active_readers{0};
    std::atomic<uint32_t> active_topics{0};
    std::chrono::steady_clock::time_point start_time;
    
    // Topic级别统计
    std::map<std::string, uint64_t> samples_per_topic;
    std::map<std::string, uint64_t> bytes_per_topic;
    mutable std::mutex topic_stats_mutex;
    
    PersistenceStatistics() : start_time(std::chrono::steady_clock::now()) {}
    
    // 拷贝构造函数
    PersistenceStatistics(const PersistenceStatistics& other) 
        : total_samples_stored{other.total_samples_stored.load()}
        , total_samples_served{other.total_samples_served.load()}
        , total_bytes_stored{other.total_bytes_stored.load()}
        , total_bytes_served{other.total_bytes_served.load()}
        , active_writers{other.active_writers.load()}
        , active_readers{other.active_readers.load()}
        , active_topics{other.active_topics.load()}
        , start_time{other.start_time}
    {
        std::lock_guard<std::mutex> lock(other.topic_stats_mutex);
        samples_per_topic = other.samples_per_topic;
        bytes_per_topic = other.bytes_per_topic;
    }
    
    // 赋值操作符
    PersistenceStatistics& operator=(const PersistenceStatistics& other) {
        if (this != &other) {
            total_samples_stored = other.total_samples_stored.load();
            total_samples_served = other.total_samples_served.load();
            total_bytes_stored = other.total_bytes_stored.load();
            total_bytes_served = other.total_bytes_served.load();
            active_writers = other.active_writers.load();
            active_readers = other.active_readers.load();
            active_topics = other.active_topics.load();
            start_time = other.start_time;
            
            std::lock_guard<std::mutex> lock1(topic_stats_mutex);
            std::lock_guard<std::mutex> lock2(other.topic_stats_mutex);
            samples_per_topic = other.samples_per_topic;
            bytes_per_topic = other.bytes_per_topic;
        }
        return *this;
    }
    
    double get_uptime_seconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - start_time).count();
    }
};

// 存储的数据项结构
struct StoredData {
    std::string topic_name;
    std::string type_name;
    std::string instance_id;      // 实例标识
    std::vector<uint8_t> data;
    SampleInfo sample_info;
    Time_t storage_time;
    uint64_t sequence_number;     // 序列号，便于排序
    bool is_disposed = false;     // 是否被dispose
    DurabilityQosPolicyKind durability_kind; // 持久化策略类型
    
    // 为Qt显示提供的便利方法
    [[nodiscard]] auto to_string() const -> std::string {
        return "Topic: " + topic_name + ", Type: " + type_name + 
               ", Size: " + std::to_string(data.size()) + " bytes";
    }
};

// 内存缓存项
struct MemoryCacheItem {
    StoredData data;
    std::chrono::steady_clock::time_point timestamp;
    
    MemoryCacheItem(StoredData stored_data) 
        : data(std::move(stored_data)), timestamp(std::chrono::steady_clock::now()) {}
};

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
    
    void move_to_head(std::shared_ptr<CacheNode> node);
    void remove_node(std::shared_ptr<CacheNode> node);
    auto remove_tail() -> std::shared_ptr<CacheNode>;
    void add_to_head(std::shared_ptr<CacheNode> node);

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

// 前向声明
class PersistenceService;
class DurabilityHandler;



// 网络拓扑信息
struct NetworkTopology {
    std::vector<ParticipantBuiltinTopicData> participants;
    std::vector<PublicationBuiltinTopicData> writers;
    std::vector<SubscriptionBuiltinTopicData> readers;
    
    // 统计信息
    [[nodiscard]] auto total_participants() const -> size_t { return participants.size(); }
    [[nodiscard]] auto total_writers() const -> size_t { return writers.size(); }
    [[nodiscard]] auto total_readers() const -> size_t { return readers.size(); }
    
    // 获取所有活跃的主题
    [[nodiscard]] auto get_active_topics() const -> std::vector<std::string> {
        std::vector<std::string> topics;
        for (const auto& writer : writers) {
            topics.push_back(writer.topic_name);
        }
        std::sort(topics.begin(), topics.end());
        topics.erase(std::unique(topics.begin(), topics.end()), topics.end());
        return topics;
    }
};

/**
 * @brief DDS持久化服务主类 - 增强的多策略支持
 * 
 * 功能：
 * 1. 监控网络中的持久化DataWriter
 * 2. 根据不同的Durability QoS策略选择相应的处理器
 * 3. 自动接收和存储持久化数据
 * 4. 为新的DataReader提供历史数据
 * 5. 提供丰富的统计信息和监控接口
 * 6. 支持Qt可视化回调
 */
class PersistenceService {
public:
    // 数据监听器类
    class PersistenceDataListener : public DataReaderListener {
    private:
        PersistenceService* service_;
        std::string topic_name_;
        DurabilityQosPolicyKind durability_kind_;
    
    public:
        PersistenceDataListener(PersistenceService* service, std::string topic_name, DurabilityQosPolicyKind kind)
            : service_(service), topic_name_(std::move(topic_name)), durability_kind_(kind) {}
        
        void on_data_available(DataReader* reader) override;
        
        [[nodiscard]] auto get_topic_name() const -> const std::string& { return topic_name_; }
        [[nodiscard]] auto get_durability_kind() const -> DurabilityQosPolicyKind { return durability_kind_; }
    };

private:
    // 配置和统计
    PersistenceConfig config_;
    PersistenceStatistics stats_;
    
    // Qt UI回调接口
    std::shared_ptr<IPersistenceUICallback> ui_callback_;
    
    // DDS实体
    DomainParticipant* participant_;
    std::map<std::string, DataReader*> collectors_;
    std::map<std::string, DataWriter*> publishers_;
    
    // 监听器存储 - 防止监听器被过早销毁
    std::map<std::string, std::shared_ptr<PersistenceDataListener>> listeners_;
    
    // 持久化处理器 - 根据durability策略选择
    std::unique_ptr<DurabilityHandler> volatile_handler_;
    std::unique_ptr<DurabilityHandler> transient_local_handler_;
    std::unique_ptr<DurabilityHandler> transient_handler_;
    std::unique_ptr<DurabilityHandler> persistent_handler_;
    
    // 存储相关
    sqlite3* db_;
    std::string db_path_;
    
    // 线程控制
    std::atomic<bool> running_;
    std::vector<std::thread> worker_threads_;
    std::thread discovery_thread_;
    std::thread cleanup_thread_;
    
    // 任务队列
    std::queue<std::function<void()>> task_queue_;
    std::mutex task_queue_mutex_;
    std::condition_variable task_condition_;

public:
    /**
     * @brief 构造函数 
     * @param participant DDS域参与者
     * @param config 持久化配置
     */
    explicit PersistenceService(DomainParticipant* participant, 
                              const PersistenceConfig& config = PersistenceConfig{});
    
    /**
     * @brief 析构函数
     */
    ~PersistenceService();
    
    // 禁用拷贝构造和赋值
    PersistenceService(const PersistenceService&) = delete;
    PersistenceService& operator=(const PersistenceService&) = delete;
    
    /**
     * @brief 设置Qt UI回调接口
     */
    void set_ui_callback(std::shared_ptr<IPersistenceUICallback> callback) {
        std::lock_guard<std::mutex> lock(task_queue_mutex_);
        ui_callback_ = std::move(callback);
    }
    
    /**
     * @brief 启动持久化服务
     */
    ReturnCode_t start();
    
    /**
     * @brief 停止持久化服务
     */
    void stop();
    
    /**
     * @brief 获取当前配置
     */
    [[nodiscard]] auto get_config() const -> const PersistenceConfig& { return config_; }
    
    /**
     * @brief 获取统计信息
     */
    PersistenceStatistics get_statistics() const { return stats_; }
    
    /**
     * @brief 获取网络拓扑信息
     */
    NetworkTopology get_network_topology() const;
    
    /**
     * @brief 查询历史数据 - 支持不同durability策略
     */
    std::vector<StoredData> query_historical_data(
        const std::string& topic_name,
        DurabilityQosPolicyKind durability_kind = PERSISTENT_DURABILITY_QOS,
        const Time_t& start_time = Time_t{0, 0},
        const Time_t& end_time = Time_t{INT32_MAX, UINT32_MAX});
    
    /**
     * @brief 查询特定实例的历史数据
     */
    std::vector<StoredData> query_instance_data(
        const std::string& topic_name,
        const std::string& instance_id);
    
    /**
     * @brief 获取所有活跃的Topic列表
     */
    std::vector<std::string> get_active_topics() const;
    
    /**
     * @brief 清理指定Topic的数据
     */
    bool cleanup_topic_data(const std::string& topic_name);
    
    /**
     * @brief 导出数据为JSON格式（便于Qt显示）
     */
    std::string export_data_json(const std::string& topic_name) const;
    
    /**
     * @brief 获取指定durability类型的处理器（内部使用）
     */
    DurabilityHandler* get_handler(DurabilityQosPolicyKind kind);

public:
    // 提供给处理器使用的数据库访问方法
    sqlite3* get_database() { return db_; }
     void update_statistics_internal(const std::string& topic_name, size_t data_size, bool is_store);
    void notify_ui_callback_internal(std::function<void(IPersistenceUICallback*)> callback_func);
    
    // 共享的数据库插入方法，减少重复代码
    bool insert_data_to_database(const std::string& topic_name,
                                 const std::string& type_name,
                                 const std::string& instance_id,
                                 const StoredData& stored_data,
                                 const std::string& qos_info);

private:
    /**
     * @brief 初始化持久化处理器
     */
    void initialize_handlers();
    
    /**
     * @brief 初始化数据库
     */
    bool initialize_database();
    
    /**
     * @brief 创建数据库表结构
     */
    bool create_enhanced_tables();
    
    /**
     * @brief 启动工作线程
     */
    void start_worker_threads();
    
    /**
     * @brief 工作线程函数
     */
    void worker_thread_function();
    
    /**
     * @brief 添加任务到队列
     */
    void enqueue_task(std::function<void()> task);
    
    /**
     * @brief 发现循环线程
     */
    void discovery_loop();
    
    /**
     * @brief 处理发现的DataWriter
     */
    void handle_discovered_writer(const PublicationBuiltinTopicData& writer_data);
    
    /**
     * @brief 处理新的DataReader
     */
    void handle_new_reader(const SubscriptionBuiltinTopicData& reader_data);
    
    /**
     * @brief 检查QoS是否需要持久化
     */
    static bool is_persistent_qos(const PublicationBuiltinTopicData& data);
    
    /**
     * @brief 发送历史数据给新的DataReader
     */
    void send_historical_data(const std::string& topic_name,
                            const SubscriptionBuiltinTopicData& reader_data);
    
    /**
     * @brief 存储数据到数据库（旧接口兼容）
     */
    bool store_data(const std::string& topic_name,
                   const std::string& type_name,
                   const std::string& instance_id,
                   const void* data,
                   size_t data_size,
                   const SampleInfo& info);
    
    /**
     * @brief 更新统计信息（兼容旧接口）
     */
    void update_statistics(const std::string& topic_name, size_t data_size, bool is_store);
    
    /**
     * @brief 通知UI回调
     */
    void notify_ui_callback(std::function<void(IPersistenceUICallback*)> callback_func);
    
    /**
     * @brief 清理过期数据
     */
    void cleanup_expired_data();
    
    /**
     * @brief 数据清理线程
     */
    void cleanup_loop();
    
    /**
     * @brief 检查数据库大小限制
     */
    bool check_database_size_limit();
    
    /**
     * @brief 压缩数据（如果启用）
     */
    std::vector<uint8_t> compress_data(const std::vector<uint8_t>& data) const;
    
    /**
     * @brief 解压缩数据
     */
    std::vector<uint8_t> decompress_data(const std::vector<uint8_t>& compressed_data) const;
};

// 持久化处理器抽象基类
class DurabilityHandler {
public:
    explicit DurabilityHandler(PersistenceService* service, DurabilityConfig config)
        : service_(service), config_(config) {}
    
    virtual ~DurabilityHandler() = default;
    
    // 存储数据
    virtual auto store_data(const std::string& topic_name,
                           const std::string& type_name,
                           const std::string& instance_id,
                           const void* data,
                           size_t data_size,
                           const SampleInfo& info) -> bool = 0;
    
    // 查询历史数据
    virtual auto query_historical_data(
        const std::string& topic_name,
        const Time_t& start_time = Time_t{0, 0},
        const Time_t& end_time = Time_t{0x7FFFFFFF, 0xFFFFFFFF}) -> std::vector<StoredData> = 0;
    
    // 清理过期数据
    virtual void cleanup_expired_data() = 0;
    
    // 获取配置
    const DurabilityConfig& get_config() const { return config_; }
    
protected:
    PersistenceService* service_;
    DurabilityConfig config_;
};

// VOLATILE处理器 - 不存储数据
class VolatileHandler : public DurabilityHandler {
public:
    explicit VolatileHandler(PersistenceService* service, DurabilityConfig config)
        : DurabilityHandler(service, config) {}
    
    auto store_data(const std::string& topic_name,
                   const std::string& type_name,
                   const std::string& instance_id,
                   const void* data,
                   size_t data_size,
                   const SampleInfo& info) -> bool override;
    
    auto query_historical_data(
        const std::string& topic_name,
        const Time_t& start_time,
        const Time_t& end_time) -> std::vector<StoredData> override;
    
    void cleanup_expired_data() override;
};

// TRANSIENT_LOCAL处理器 - 仅内存缓存
class TransientLocalHandler : public DurabilityHandler {
private:
    std::unique_ptr<MemoryCache> memory_cache_;
    
public:
    explicit TransientLocalHandler(PersistenceService* service, DurabilityConfig config);
    
    auto store_data(const std::string& topic_name,
                   const std::string& type_name,
                   const std::string& instance_id,
                   const void* data,
                   size_t data_size,
                   const SampleInfo& info) -> bool override;
    
    auto query_historical_data(
        const std::string& topic_name,
        const Time_t& start_time,
        const Time_t& end_time) -> std::vector<StoredData> override;
    
    void cleanup_expired_data() override;
};

// TRANSIENT处理器 - 数据库 + 内存缓存
class TransientHandler : public DurabilityHandler {
private:
    std::unique_ptr<MemoryCache> memory_cache_;
    
public:
    explicit TransientHandler(PersistenceService* service, DurabilityConfig config);
    
    auto store_data(const std::string& topic_name,
                   const std::string& type_name,
                   const std::string& instance_id,
                   const void* data,
                   size_t data_size,
                   const SampleInfo& info) -> bool override;
    
    auto query_historical_data(
        const std::string& topic_name,
        const Time_t& start_time,
        const Time_t& end_time) -> std::vector<StoredData> override;
    
    void cleanup_expired_data() override;
};

// PERSISTENT处理器 - 长期数据库存储
class PersistentHandler : public DurabilityHandler {
private:
    std::unique_ptr<MemoryCache> memory_cache_;
    
public:
    explicit PersistentHandler(PersistenceService* service, DurabilityConfig config);
    
    auto store_data(const std::string& topic_name,
                   const std::string& type_name,
                   const std::string& instance_id,
                   const void* data,
                   size_t data_size,
                   const SampleInfo& info) -> bool override;
    
    auto query_historical_data(
        const std::string& topic_name,
        const Time_t& start_time,
        const Time_t& end_time) -> std::vector<StoredData> override;
    
    void cleanup_expired_data() override;
};

AGIDDS_NAMESPACE_END
