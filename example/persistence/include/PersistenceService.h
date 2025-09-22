#pragma once

#include "../../../include/travodds_dll.h"
#include "PersistenceTypes.h"
#include "MemoryCache.h"
#include "DatabaseManager.h"
#include "DurabilityHandlers.h"
#include "dcps/listener/datareaderlistener.h"
#include <memory>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <algorithm>

TRAVODDS_NAMESPACE_BEGIN

// 前向声明
class DomainParticipant;
class DataReader;
class DataReaderImpl;
class DataWriter;
class Subscriber;
class Publisher;

// 实体信息结构体（供外部使用，内存管理）
struct ActiveDataWriterInfo {
    BuiltinTopicKey_t key;
    std::string topic_name;
    std::string type_name;
    DurabilityQosPolicyKind durability_kind;
    ReliabilityQosPolicyKind reliability_kind;
    std::chrono::steady_clock::time_point discovered_at;
    std::chrono::steady_clock::time_point last_seen;
    bool is_active;
    
    // 扩展的QoS字段用于兼容性检查
    PresentationQosPolicy presentation;
    DeadlineQosPolicy deadline;
    LatencyBudgetQosPolicy latency_budget;
    OwnershipQosPolicy ownership;
    OwnershipStrengthQosPolicy ownership_strength;
    LivelinessQosPolicy liveliness;
    DestinationOrderQosPolicy destination_order;
    LifespanQosPolicy lifespan;
    
    // 可选：存储完整的Publication数据
    PublicationBuiltinTopicData full_publication_data;
};

struct ActiveDataReaderInfo {
    BuiltinTopicKey_t key;
    std::string topic_name;
    std::string type_name;
    DurabilityQosPolicyKind durability_kind;
    ReliabilityQosPolicyKind reliability_kind;
    std::chrono::steady_clock::time_point discovered_at;
    std::chrono::steady_clock::time_point last_seen;
    bool is_active;
    
    // 扩展的QoS字段用于兼容性检查
    PresentationQosPolicy presentation;
    DeadlineQosPolicy deadline;
    LatencyBudgetQosPolicy latency_budget;
    OwnershipQosPolicy ownership;
    LivelinessQosPolicy liveliness;
    DestinationOrderQosPolicy destination_order;
    TimeBasedFilterQosPolicy time_based_filter;
    
    // 可选：存储完整的Subscription数据
    SubscriptionBuiltinTopicData full_subscription_data;
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
    void on_publication_discovered(const PublicationBuiltinTopicData& pub_data);
    void on_subscription_discovered(const SubscriptionBuiltinTopicData& sub_data);
    
    // 数据操作
    void store_data(const StoredData& data);
    auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData>;
    auto get_historical_data_range(const std::string& topic_name,
                                  const Time_t& start_time,
                                  const Time_t& end_time) -> std::vector<StoredData>;
    
    // 统计信息
    auto get_statistics() -> std::map<std::string, int64_t>;
    
    // 【新增】获取数据库表名
    auto get_db_table_names() -> std::vector<std::string>;

    // 【新增】获取数据库表内容
    auto get_db_table_content(const std::string& table_name) -> std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>>;

    // 【新增】获取内存缓存使用情况
    auto get_memory_cache_usage() const -> std::pair<size_t, size_t>; // current_bytes, max_bytes

    // 配置访问
    const PersistenceConfig& get_config() const { return config_; }
    
    // 访问内部组件（用于处理器）
    MemoryCache& get_memory_cache() { return *memory_cache_; }
    DatabaseManager& get_database_manager() { return *database_manager_; }
    
    // 为Handler提供的DDS实体创建辅助方法
    auto create_datareader_for_handler(const std::string& topic_name, 
                                      const std::string& type_name, 
                                      DurabilityQosPolicyKind durability) -> std::shared_ptr<DataReader>;
    auto create_datawriter_for_handler(const std::string& topic_name, 
                                      const std::string& type_name, 
                                      DurabilityQosPolicyKind durability) -> std::shared_ptr<DataWriter>;
    
    // 为Handler提供的QoS兼容性检查方法
    auto check_qos_compatibility_for_handler(const std::string& topic_name,
                                            const BuiltinTopicKey_t& reader_key) -> bool;
    
    // 获取活跃实体信息（新增，直接从内存返回）
    std::vector<ActiveDataWriterInfo> get_active_datawriters_info() const;
    std::vector<ActiveDataReaderInfo> get_active_datareaders_info() const;
    std::vector<ActiveDataWriterInfo> get_topic_datawriters_info(const std::string& topic_name) const;
    std::vector<ActiveDataReaderInfo> get_topic_datareaders_info(const std::string& topic_name) const;
    
    // QoS兼容性检查方法（public for testing）
    // StoredData与ActiveDataReaderInfo的QoS兼容性检查重载
    [[nodiscard]] auto are_qos_compatible(const StoredData& stored_data, 
                                          const ActiveDataReaderInfo& reader_info) const -> bool;
    [[nodiscard]] auto check_durability_compatibility(DurabilityQosPolicyKind writer_kind, 
                                                      DurabilityQosPolicyKind reader_kind) const -> bool;
    [[nodiscard]] auto check_presentation_compatibility(const PresentationQosPolicy& writer_pres, 
                                                        const PresentationQosPolicy& reader_pres) const -> bool;
    [[nodiscard]] auto check_deadline_compatibility(const DeadlineQosPolicy& writer_deadline, 
                                                    const DeadlineQosPolicy& reader_deadline) const -> bool;
    [[nodiscard]] auto check_latency_budget_compatibility(const LatencyBudgetQosPolicy& writer_budget, 
                                                          const LatencyBudgetQosPolicy& reader_budget) const -> bool;
    [[nodiscard]] auto check_ownership_compatibility(const OwnershipQosPolicy& writer_ownership, 
                                                     const OwnershipQosPolicy& reader_ownership) const -> bool;
    [[nodiscard]] auto check_liveliness_compatibility(const LivelinessQosPolicy& writer_liveliness, 
                                                      const LivelinessQosPolicy& reader_liveliness) const -> bool;
    [[nodiscard]] auto check_reliability_compatibility(ReliabilityQosPolicyKind writer_kind, 
                                                       ReliabilityQosPolicyKind reader_kind) const -> bool;
    [[nodiscard]] auto check_destination_order_compatibility(const DestinationOrderQosPolicy& writer_order, 
                                                             const DestinationOrderQosPolicy& reader_order) const -> bool;
    
    // 数据清理
    void cleanup_old_data();
    
    // 数据订阅相关（移到public让Handler使用）
    auto create_data_reader_for_topic(const PublicationBuiltinTopicData& pub_data) -> bool;  // 根据Writer QoS创建匹配的Reader
    auto create_data_writer_for_topic(const SubscriptionBuiltinTopicData& sub_data) -> bool;  // 根据Reader QoS创建匹配的Writer
    
    // 为Handler提供的历史数据发送方法
    void send_historical_data_for_handler(const std::string& topic_name, 
                                         const std::string& type_name, 
                                         DurabilityQosPolicyKind durability,
                                         const BuiltinTopicKey_t& reader_key);

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
    
    // 发现监听相关
    std::shared_ptr<DataReader> publication_reader_;
    std::shared_ptr<DataReader> subscription_reader_;
    const Subscriber* discovery_subscriber_ = nullptr;  // 用于发现的订阅者
    
    // 数据订阅相关
    std::map<std::string, std::shared_ptr<DataReader>> topic_readers_;  // topic_name -> DataReader
    std::map<std::string, DurabilityQosPolicyKind> topic_durability_;   // topic_name -> durability
    std::mutex readers_mutex_;
    
    // Topic信息管理 - 生产级持久化服务核心
    struct TopicInfo {
        std::string topic_name;
        std::string type_name;
        DurabilityQosPolicyKind durability_kind;
        ReliabilityQosPolicyKind reliability_kind;
        PublicationBuiltinTopicData publication_data;
        std::chrono::steady_clock::time_point discovered_at;
    };
    std::map<std::string, TopicInfo> topic_info_map_;  // topic_name -> TopicInfo
    std::mutex topic_info_mutex_;
    
    // DataWriter/DataReader 监控和智能发送相关
    std::map<std::string, std::set<BuiltinTopicKey_t>> active_datawriters_;  // topic_name -> set of writer keys
    std::map<std::string, std::set<BuiltinTopicKey_t>> active_datareaders_;  // topic_name -> set of reader keys
    std::mutex entity_monitoring_mutex_;
    
    // 活动监控线程
    std::atomic<bool> monitoring_running_{false};
    std::thread entity_monitoring_thread_;
    
    // 智能发送配置
    struct SmartSendingConfig {
        bool enable_smart_sending = true;            // 是否启用智能发送
        std::chrono::seconds entity_timeout{30};    // 实体活动超时时间
        std::chrono::seconds monitoring_interval{5}; // 监控间隔
    };
    SmartSendingConfig smart_config_;
    
    // 内存中的实体信息管理（替代数据库存储）
    std::map<BuiltinTopicKey_t, ActiveDataWriterInfo> active_writers_info_;
    std::map<BuiltinTopicKey_t, ActiveDataReaderInfo> active_readers_info_;
    mutable std::mutex entity_info_mutex_;
    
    // 历史数据发送相关
    std::map<std::string, std::shared_ptr<DataWriter>> topic_writers_;  // topic_name -> DataWriter
    Publisher* publisher_ = nullptr;  // 用于创建DataWriter
    std::mutex writers_mutex_;
    
    // 自身实体过滤 - 避免将持久化服务自己创建的实体存入数据库
    std::set<InstanceHandle_t> own_datawriter_handles_;  // 自己创建的DataWriter句柄
    std::set<InstanceHandle_t> own_datareader_handles_;  // 自己创建的DataReader句柄
    std::mutex own_entities_mutex_;
    
    // 线程管理
    std::vector<std::thread> worker_threads_;
    std::mutex service_mutex_;
    std::condition_variable cv_;
    
    //发现轮询线程
    std::atomic<bool> discovery_running_{false};
    std::thread discovery_thread_;
    
    // 内部方法
    void initialize_handlers();
    void cleanup_resources();
    auto get_handler_for_durability(DurabilityQosPolicyKind kind) -> DurabilityHandler*;
    
    // 策略检查工具方法
    [[nodiscard]] auto should_handle_durability(DurabilityQosPolicyKind durability) const -> bool;
    [[nodiscard]] auto requires_persistence_service(DurabilityQosPolicyKind durability) const -> bool;
    [[nodiscard]] auto durability_kind_to_string(DurabilityQosPolicyKind durability) const -> const char*;
    

    
    // 工作线程方法
    void worker_thread_main();
    void discovery_polling_thread();
    
    // 新的简化发现方法（使用DDS标准API）
    void check_new_publications(std::set<std::string>& processed_publications);
    void check_new_subscriptions(std::set<std::string>& processed_subscriptions);
    static auto handle_to_string(const InstanceHandle_t& handle) -> std::string;
    
    // DataWriter/DataReader 监控相关方法
    void entity_monitoring_thread_main();
    void update_datawriter_registry(const PublicationBuiltinTopicData& pub_data);
    void update_datareader_registry(const SubscriptionBuiltinTopicData& sub_data);
    void cleanup_inactive_entities();
    void check_entities_active_status();  // 新增：检查实体活跃状态
    bool should_send_historical_data(const std::string& topic_name) const;
    
    // 数据处理方法
    void poll_data_readers();
    
    // 历史数据传送方法
    void send_historical_data_to_subscriber(const std::string& topic_name, const std::string& type_name, DurabilityQosPolicyKind durability, const BuiltinTopicKey_t& reader_key);
};

TRAVODDS_NAMESPACE_END


