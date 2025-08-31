#pragma once

#include "PersistenceTypes.h"
#include "MemoryCache.h"
#include "DatabaseManager.h"
#include "DurabilityHandlers.h"
#include <memory>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <algorithm>

AGIDDS_NAMESPACE_BEGIN

// 前向声明
class DomainParticipant;
class DataReader;
class DataWriter;
class Subscriber;

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

// 持久化配置
struct PersistenceConfig {
    static constexpr size_t DEFAULT_RETENTION_HOURS = 24;
    static constexpr size_t DEFAULT_MAX_SAMPLES = 10000;
    static constexpr size_t DEFAULT_CACHE_SIZE_MB = 100;
    static constexpr size_t DEFAULT_WORKER_THREADS = 4;
    
    std::chrono::hours data_retention{DEFAULT_RETENTION_HOURS};
    size_t max_samples_per_topic = DEFAULT_MAX_SAMPLES;
    size_t memory_cache_size_mb = DEFAULT_CACHE_SIZE_MB;
    size_t worker_thread_count = DEFAULT_WORKER_THREADS;
    std::string database_path{"persistence_data.db"};
    bool enable_statistics = true;
    bool enable_ui_callbacks = false;
};

// 主要的持久化服务类
class PersistenceService {
public:
    explicit PersistenceService(const PersistenceConfig& config = PersistenceConfig{});
    ~PersistenceService();
    
    // 禁用拷贝
    PersistenceService(const PersistenceService&) = delete;
    PersistenceService& operator=(const PersistenceService&) = delete;
    
    // 生命周期管理
    bool initialize();
    void shutdown();
    bool is_running() const { return is_running_; }
    
    // DDS集成
    void set_domain_participant(std::shared_ptr<DomainParticipant> participant);
    
    // 发现监听相关
    void setup_discovery_listeners();
    void on_publication_discovered(const std::string& topic_name, const std::string& type_name, DurabilityQosPolicyKind durability);
    void on_subscription_discovered(const std::string& topic_name, const std::string& type_name, DurabilityQosPolicyKind durability);
    
    // 手动发现通知（由外部应用调用）
    void notify_writer_created(const std::string& topic_name, const std::string& type_name, DurabilityQosPolicyKind durability = VOLATILE_DURABILITY_QOS);
    void notify_reader_created(const std::string& topic_name, const std::string& type_name, DurabilityQosPolicyKind durability = VOLATILE_DURABILITY_QOS);
    
    // 数据操作
    void store_data(const StoredData& data);
    auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData>;
    auto get_historical_data_range(const std::string& topic_name,
                                  const Time_t& start_time,
                                  const Time_t& end_time) -> std::vector<StoredData>;
    
    // UI回调接口
    void set_ui_callback(std::shared_ptr<IPersistenceUICallback> callback);
    
    // 统计信息
    auto get_statistics() -> std::map<std::string, int64_t>;
    
    // 配置访问
    const PersistenceConfig& get_config() const { return config_; }
    
    // 访问内部组件（用于处理器）
    MemoryCache& get_memory_cache() { return *memory_cache_; }
    DatabaseManager& get_database_manager() { return *database_manager_; }
    IPersistenceUICallback* get_ui_callback() const { return ui_callback_.get(); }
    
    // 数据库操作的便利方法（供处理器使用）
    bool store_data_to_database(const StoredData& data);
    auto load_data_from_database(const std::string& topic_name) -> std::vector<StoredData>;
    auto load_data_from_database_range(const std::string& topic_name,
                                     const Time_t& start_time,
                                     const Time_t& end_time) -> std::vector<StoredData>;
    bool insert_or_update_topic_info(const std::string& topic_name,
                                    const std::string& type_name,
                                    DurabilityQosPolicyKind durability_kind);
    void cleanup_old_data();

private:
    PersistenceConfig config_;
    std::atomic<bool> is_running_{false};
    std::atomic<bool> should_stop_{false};
    
    // 核心组件
    std::unique_ptr<MemoryCache> memory_cache_;
    std::unique_ptr<DatabaseManager> database_manager_;
    
    // 持久化处理器
    std::map<DurabilityQosPolicyKind, std::unique_ptr<DurabilityHandler>> handlers_;
    
    // DDS集成
    std::shared_ptr<DomainParticipant> participant_;
    std::shared_ptr<IPersistenceUICallback> ui_callback_;
    
    // 发现监听相关
    std::shared_ptr<DataReader> publication_reader_;
    std::shared_ptr<DataReader> subscription_reader_;
    Subscriber* discovery_subscriber_ = nullptr;  // 用于发现的订阅者
    std::thread discovery_thread_;
    std::atomic<bool> discovery_running_{false};
    
    // 数据订阅相关
    std::map<std::string, std::shared_ptr<DataReader>> topic_readers_;  // topic_name -> DataReader
    std::map<std::string, DurabilityQosPolicyKind> topic_durability_;   // topic_name -> durability
    std::mutex readers_mutex_;
    
    // 线程管理
    std::vector<std::thread> worker_threads_;
    std::mutex service_mutex_;
    std::condition_variable cv_;
    
    // 内部方法
    void initialize_handlers();
    void cleanup_resources();
    void worker_thread_main();
    void discovery_thread_main();
    void check_publication_data();
    void check_subscription_data();
    auto get_handler_for_durability(DurabilityQosPolicyKind kind) -> DurabilityHandler*;
    
    // 数据订阅相关
    bool create_data_reader_for_topic(const std::string& topic_name, const std::string& type_name, DurabilityQosPolicyKind durability);
    void on_data_available(const std::string& topic_name, const std::shared_ptr<DataReader>& reader);
    void poll_data_readers();
};

AGIDDS_NAMESPACE_END
