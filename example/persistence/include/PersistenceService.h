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

/**
 * @brief 活跃DataWriter实体信息结构体
 * 
 * 用于存储和管理发现的DataWriter实体的详细信息，包括QoS策略、状态和时间戳等。
 */
struct ActiveDataWriterInfo {
    BuiltinTopicKey_t key;                                    ///< 实体唯一标识符
    std::string topic_name;                                   ///< Topic名称
    std::string type_name;                                    ///< 数据类型名称
    DurabilityQosPolicyKind durability_kind;                  ///< 持久化策略类型
    ReliabilityQosPolicyKind reliability_kind;                ///< 可靠性策略类型
    std::chrono::steady_clock::time_point discovered_at;      ///< 发现时间戳
    std::chrono::steady_clock::time_point last_seen;          ///< 最后活跃时间戳
    bool is_active;                                           ///< 实体是否活跃
    
    // 扩展的QoS字段用于兼容性检查
    PresentationQosPolicy presentation;                       ///< 展现策略
    DeadlineQosPolicy deadline;                              ///< 截止时间策略
    LatencyBudgetQosPolicy latency_budget;                   ///< 延迟预算策略
    OwnershipQosPolicy ownership;                            ///< 所有权策略
    OwnershipStrengthQosPolicy ownership_strength;           ///< 所有权强度策略
    LivelinessQosPolicy liveliness;                          ///< 活跃性策略
    DestinationOrderQosPolicy destination_order;             ///< 目标顺序策略
    LifespanQosPolicy lifespan;                             ///< 生命周期策略
    
    // 可选：存储完整的Publication数据
    PublicationBuiltinTopicData full_publication_data;       ///< 完整的发布数据（可选）
};

/**
 * @brief 活跃DataReader实体信息结构体
 * 
 * 用于存储和管理发现的DataReader实体的详细信息，包括QoS策略、状态和时间戳等。
 */
struct ActiveDataReaderInfo {
    BuiltinTopicKey_t key;                                    ///< 实体唯一标识符
    std::string topic_name;                                   ///< Topic名称
    std::string type_name;                                    ///< 数据类型名称
    DurabilityQosPolicyKind durability_kind;                  ///< 持久化策略类型
    ReliabilityQosPolicyKind reliability_kind;                ///< 可靠性策略类型
    std::chrono::steady_clock::time_point discovered_at;      ///< 发现时间戳
    std::chrono::steady_clock::time_point last_seen;          ///< 最后活跃时间戳
    bool is_active;                                           ///< 实体是否活跃
    
    // 扩展的QoS字段用于兼容性检查
    PresentationQosPolicy presentation;                       ///< 展现策略
    DeadlineQosPolicy deadline;                              ///< 截止时间策略
    LatencyBudgetQosPolicy latency_budget;                   ///< 延迟预算策略
    OwnershipQosPolicy ownership;                            ///< 所有权策略
    LivelinessQosPolicy liveliness;                          ///< 活跃性策略
    DestinationOrderQosPolicy destination_order;             ///< 目标顺序策略
    TimeBasedFilterQosPolicy time_based_filter;             ///< 基于时间的过滤策略
    
    // 可选：存储完整的Subscription数据
    SubscriptionBuiltinTopicData full_subscription_data;     ///< 完整的订阅数据（可选）
};

/**
 * @brief 持久化服务配置结构体
 * 
 * 包含持久化服务运行所需的所有配置参数，如数据保留时间、缓存大小、线程数量等。
 */
struct PersistenceConfig {
    static constexpr size_t DEFAULT_RETENTION_HOURS = 24;     ///< 默认数据保留时间（小时）
    static constexpr size_t DEFAULT_MAX_SAMPLES = 10000;      ///< 默认每个Topic最大样本数
    static constexpr size_t DEFAULT_CACHE_SIZE_MB = 100;      ///< 默认内存缓存大小（MB）
    static constexpr size_t DEFAULT_WORKER_THREADS = 4;       ///< 默认工作线程数
    
    std::chrono::hours data_retention{DEFAULT_RETENTION_HOURS}; ///< 数据保留时间
    size_t max_samples_per_topic = DEFAULT_MAX_SAMPLES;         ///< 每个Topic最大样本数
    size_t memory_cache_size_mb = DEFAULT_CACHE_SIZE_MB;        ///< 内存缓存大小（MB）
    size_t worker_thread_count = DEFAULT_WORKER_THREADS;        ///< 工作线程数量
    std::string database_path{"persistence_data.db"};           ///< 数据库文件路径
    bool enable_statistics = true;                              ///< 是否启用统计功能
    bool enable_ui_callbacks = false;                           ///< 是否启用UI回调
};

/**
 * @brief 主要的持久化服务类
 * 
 * 这是持久化服务的核心类，负责管理DDS实体的持久化数据存储、历史数据重放、
 * QoS兼容性检查以及与UI的交互。支持TRANSIENT和PERSISTENT持久化策略。
 */
class PersistenceService {
public:
    /**
     * @brief 构造函数
     * 
     * @param config 持久化服务配置，包含数据保留策略、缓存设置等
     */
    explicit PersistenceService(const PersistenceConfig& config = PersistenceConfig{});
    /**
     * @brief 析构函数
     * 
     * 自动调用shutdown()确保资源正确释放
     */
    ~PersistenceService();
    
    // 禁用拷贝
    PersistenceService(const PersistenceService&) = delete;
    PersistenceService& operator=(const PersistenceService&) = delete;
    
    // 生命周期管理
    /**
     * @brief 初始化持久化服务
     * 
     * 初始化数据库、内存缓存、监控组件和线程池等核心组件。
     * 
     * @return true 初始化成功，false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 关闭持久化服务
     * 
     * 优雅停止所有组件，确保数据完整性和资源正确释放。
     */
    void shutdown();
    
    /**
     * @brief 检查服务是否正在运行
     * 
     * @return true 服务运行中，false 服务已停止
     */
    bool is_running() const { return is_running_; }
    
    // DDS集成
    /**
     * @brief 设置DDS域参与者
     * 
     * 关联持久化服务与特定的DDS域参与者，用于发现和数据传输。
     * 
     * @param participant 共享的域参与者指针
     */
    void set_domain_participant(std::shared_ptr<DomainParticipant> participant);
    
    // 发现监听相关
    /**
     * @brief 设置发现监听器
     * 
     * 配置用于监听DataWriter和DataReader发现事件的内置监听器。
     */
    void setup_discovery_listeners();
    
    /**
     * @brief 处理DataWriter发现事件
     * 
     * 当发现新的DataWriter时调用，根据其QoS要求决定是否需要持久化其数据。
     * 
     * @param pub_data 发布者内置Topic数据，包含QoS和Topic信息
     */
    void on_publication_discovered(const PublicationBuiltinTopicData& pub_data);
    
    /**
     * @brief 处理DataReader发现事件
     * 
     * 当发现新的DataReader时调用，根据其QoS要求发送相应的历史数据。
     * 
     * @param sub_data 订阅者内置Topic数据，包含QoS和Topic信息
     */
    void on_subscription_discovered(const SubscriptionBuiltinTopicData& sub_data);
    
    // 数据操作
    /**
     * @brief 存储DDS数据样本
     * 
     * 将接收到的DDS数据样本持久化到内存缓存和数据库中。
     * 
     * @param data 包含序列化数据、时间戳和元信息的存储数据结构
     */
    void store_data(const StoredData& data);
    
    /**
     * @brief 获取Topic的历史数据
     * 
     * 检索指定Topic的所有历史数据样本，按时间戳排序。
     * 
     * @param topic_name Topic名称
     * @return 历史数据样本向量
     */
    auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData>;
    
    /**
     * @brief 获取指定时间范围的历史数据
     * 
     * 检索指定Topic在特定时间范围内的历史数据样本。
     * 
     * @param topic_name Topic名称
     * @param start_time 开始时间
     * @param end_time 结束时间
     * @return 时间范围内的历史数据样本向量
     */
    auto get_historical_data_range(const std::string& topic_name,
                                  const Time_t& start_time,
                                  const Time_t& end_time) -> std::vector<StoredData>;
    
    // 统计信息
    /**
     * @brief 获取持久化服务统计信息
     * 
     * 提供服务运行时的各种统计数据，如存储的样本数量、内存使用等。
     * 
     * @return 统计信息映射表，键为统计项名称，值为统计数值
     */
    auto get_statistics() -> std::map<std::string, int64_t>;
    
    // 【新增】获取数据库表名
    /**
     * @brief 获取数据库中所有表名
     * 
     * @return 数据库表名向量
     */
    auto get_db_table_names() -> std::vector<std::string>;

    // 【新增】获取数据库表内容
    /**
     * @brief 获取指定数据库表的内容
     * 
     * @param table_name 表名
     * @return 表内容，包含列名向量和数据行向量
     */
    auto get_db_table_content(const std::string& table_name) -> std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>>;

    // 【新增】获取内存缓存使用情况
    /**
     * @brief 获取内存缓存使用情况
     * 
     * @return 内存使用情况对，包含当前字节数和最大字节数
     */
    auto get_memory_cache_usage() const -> std::pair<size_t, size_t>; // current_bytes, max_bytes

    // 【新增】获取性能统计信息（为实时图表提供数据）
    
    
    
    /**
     * @brief 获取服务运行时间（秒）
     * 
     * @return 服务启动以来的运行时间
     */
    auto get_uptime_seconds() const -> int64_t;
    
    
    // 【新增】网络拓扑和域信息接口
    /**
     * @brief 获取域参与者信息
     * 
     * @return 包含域ID、参与者信息等的映射表
     */
    auto get_domain_participant_info() -> std::map<std::string, std::string>;
    
    /**
     * @brief 获取网络拓扑节点信息（用于可视化）
     * 
     * @return 网络节点信息向量，每个节点包含ID、类型、状态、连接关系等
     */
    auto get_network_topology_nodes() -> std::vector<std::map<std::string, std::string>>;
    
    /**
     * @brief 获取网络拓扑连接信息（用于可视化）
     * 
     * @return 网络连接信息向量，每个连接包含源节点、目标节点、连接类型等
     */
    auto get_network_topology_connections() -> std::vector<std::map<std::string, std::string>>;

    // 配置访问
    /**
     * @brief 获取服务配置
     * 
     * @return 当前持久化服务配置的常量引用
     */
    const PersistenceConfig& get_config() const { return config_; }
    
    // 访问内部组件（用于处理器）
    /**
     * @brief 获取内存缓存组件
     * 
     * @return 内存缓存管理器的引用
     */
    MemoryCache& get_memory_cache() { return *memory_cache_; }
    
    /**
     * @brief 获取数据库管理组件
     * 
     * @return 数据库管理器的引用
     */
    DatabaseManager& get_database_manager() { return *database_manager_; }
    
    // 为Handler提供的DDS实体创建辅助方法
    /**
     * @brief 为处理器创建DataReader
     * 
     * 根据指定的Topic和QoS要求创建用于接收历史数据的DataReader。
     * 
     * @param topic_name Topic名称
     * @param type_name 数据类型名称
     * @param durability 持久化策略
     * @return 创建的DataReader共享指针
     */
    auto create_datareader_for_handler(const std::string& topic_name, 
                                      const std::string& type_name, 
                                      DurabilityQosPolicyKind durability) -> std::shared_ptr<DataReader>;
    
    /**
     * @brief 为处理器创建DataWriter
     * 
     * 根据指定的Topic和QoS要求创建用于发送历史数据的DataWriter。
     * 
     * @param topic_name Topic名称
     * @param type_name 数据类型名称
     * @param durability 持久化策略
     * @return 创建的DataWriter共享指针
     */
    auto create_datawriter_for_handler(const std::string& topic_name, 
                                      const std::string& type_name, 
                                      DurabilityQosPolicyKind durability) -> std::shared_ptr<DataWriter>;
    
    // 为Handler提供的QoS兼容性检查方法
    /**
     * @brief 为处理器检查QoS兼容性
     * 
     * 检查指定Topic和Reader是否QoS兼容，用于决定是否发送历史数据。
     * 
     * @param topic_name Topic名称
     * @param reader_key DataReader实体键
     * @return true 兼容，false 不兼容
     */
    auto check_qos_compatibility_for_handler(const std::string& topic_name,
                                            const BuiltinTopicKey_t& reader_key) -> bool;
    
    // 获取活跃实体信息（新增，直接从内存返回）
    /**
     * @brief 获取活跃DataWriter信息列表
     * 
     * @return 所有活跃DataWriter的信息向量
     */
    std::vector<ActiveDataWriterInfo> get_active_datawriters_info() const;
    
    /**
     * @brief 获取活跃DataReader信息列表
     * 
     * @return 所有活跃DataReader的信息向量
     */
    std::vector<ActiveDataReaderInfo> get_active_datareaders_info() const;
    
    /**
     * @brief 获取指定Topic的DataWriter信息
     * 
     * @param topic_name Topic名称
     * @return 该Topic下所有DataWriter的信息向量
     */
    std::vector<ActiveDataWriterInfo> get_topic_datawriters_info(const std::string& topic_name) const;
    
    /**
     * @brief 获取指定Topic的DataReader信息
     * 
     * @param topic_name Topic名称
     * @return 该Topic下所有DataReader的信息向量
     */
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
    // 初始化方法
    void initialize_type_supports();
    
    // 混合TypeSupport查找策略
    std::shared_ptr<TypeSupport> get_type_support_for_serialization(const std::string& type_name);
    void* create_sample_for_deserialization(const std::string& type_name);
    void delete_sample_for_deserialization(const std::string& type_name, void* sample);
    
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


