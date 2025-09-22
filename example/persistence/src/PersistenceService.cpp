#include "PersistenceService.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/subscribe/subscriber.h"
#include "dcps/subscribe/datareader.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "dcps/publish/publisher.h"
#include "dcps/publish/datawriter.h"
#include "rtps/discovery/discovery.h"
#include "type/builtintopicdata.h"
#include "../../cpp/hello_world/HelloWorldTypeSupport.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <set>
#include <chrono>
#include <cstring>

TRAVODDS_NAMESPACE_BEGIN

PersistenceService::PersistenceService(const PersistenceConfig& config)
    : config_(config) {
    
    // 初始化核心组件
    memory_cache_ = std::make_unique<MemoryCache>(config_.memory_cache_size_mb);
    
    DatabaseConfig db_config;
    db_config.database_path = config_.database_path;
    db_config.data_retention = config_.data_retention;
    db_config.max_samples_per_topic = config_.max_samples_per_topic;
    database_manager_ = std::make_unique<DatabaseManager>(db_config);
}

PersistenceService::~PersistenceService() {
    shutdown();
}

bool PersistenceService::initialize() {
    if (is_running_) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(service_mutex_);
    
    // 初始化数据库
    if (!database_manager_->initialize()) {
        std::cerr << "[PersistenceService] Failed to initialize database" << std::endl;
        return false;
    }
    
    // 初始化处理器
    initialize_handlers();
    
    // 启动工作线程
    should_stop_ = false;
    for (size_t i = 0; i < config_.worker_thread_count; ++i) {
        worker_threads_.emplace_back(&PersistenceService::worker_thread_main, this);
    }
    
    // 启动实体监控线程
    monitoring_running_ = true;
    entity_monitoring_thread_ = std::thread(&PersistenceService::entity_monitoring_thread_main, this);
    
    is_running_ = true;
    
    // 如果已经有participant，设置发现监听器
    if (participant_) {
        setup_discovery_listeners();
    }
    
    std::cout << "[PersistenceService] Service initialized successfully" << std::endl;
    return true;
}

void PersistenceService::shutdown() {
    if (!is_running_) {
        return;
    }
    
    
    // 停止发现线程
    discovery_running_ = false;
    if (discovery_thread_.joinable()) {
        discovery_thread_.join();
    }
    
    // 停止实体监控线程
    monitoring_running_ = false;
    if (entity_monitoring_thread_.joinable()) {
        entity_monitoring_thread_.join();
    }
    
    // 停止工作线程
    should_stop_ = true;
    cv_.notify_all();
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    
    // 清理资源
    cleanup_resources();
    
    is_running_ = false;
    std::cout << "[PersistenceService] Service shutdown completed" << std::endl;
}

void PersistenceService::initialize_handlers() {
    handlers_[TRANSIENT_DURABILITY_QOS] = std::make_unique<TransientHandler>(this);
    handlers_[PERSISTENT_DURABILITY_QOS] = std::make_unique<PersistentHandler>(this);
    
    // 初始化所有Handler
    for (auto& [durability, handler] : handlers_) {
        if (!(handler && handler->initialize())) {
            std::cerr << "[PersistenceService] Failed to initialize " 
                      << durability_kind_to_string(durability) << " handler" << std::endl;
        }
    }
    
    std::cout << "[PersistenceService] Durability handlers initialized" << std::endl;
}

// ============ 策略检查工具方法 ============

auto PersistenceService::should_handle_durability(DurabilityQosPolicyKind durability) const -> bool {
    return durability != VOLATILE_DURABILITY_QOS && 
           durability != TRANSIENT_LOCAL_DURABILITY_QOS;
}

auto PersistenceService::requires_persistence_service(DurabilityQosPolicyKind durability) const -> bool {
    return durability == TRANSIENT_DURABILITY_QOS || 
           durability == PERSISTENT_DURABILITY_QOS;
}

auto PersistenceService::durability_kind_to_string(DurabilityQosPolicyKind durability) const -> const char* {
    switch (durability) {
        case VOLATILE_DURABILITY_QOS: return "VOLATILE";
        case TRANSIENT_LOCAL_DURABILITY_QOS: return "TRANSIENT_LOCAL";
        case TRANSIENT_DURABILITY_QOS: return "TRANSIENT";
        case PERSISTENT_DURABILITY_QOS: return "PERSISTENT";
        default: return "UNKNOWN";
    }
}

// ============ QoS兼容性检查方法 ============
auto PersistenceService::are_qos_compatible(const StoredData& stored_data, 
                                           const ActiveDataReaderInfo& reader_info) const -> bool {
    // 检查DURABILITY兼容性
    if (!check_durability_compatibility(stored_data.durability_kind, reader_info.durability_kind)) {
        return false;
    }
    
    // 检查PRESENTATION兼容性
    if (!check_presentation_compatibility(stored_data.presentation, reader_info.presentation)) {
        return false;
    }
    
    // 检查DEADLINE兼容性
    if (!check_deadline_compatibility(stored_data.deadline, reader_info.deadline)) {
        return false;
    }
    
    // 检查LATENCY_BUDGET兼容性
    if (!check_latency_budget_compatibility(stored_data.latency_budget, reader_info.latency_budget)) {
        return false;
    }
    
    // 检查OWNERSHIP兼容性
    if (!check_ownership_compatibility(stored_data.ownership, reader_info.ownership)) {
        return false;
    }
    
    // 检查LIVELINESS兼容性
    if (!check_liveliness_compatibility(stored_data.liveliness, reader_info.liveliness)) {
        return false;
    }
    
    // 检查RELIABILITY兼容性
    if (!check_reliability_compatibility(stored_data.reliability_kind, reader_info.reliability_kind)) {
        return false;
    }
    
    // 检查DESTINATION_ORDER兼容性
    if (!check_destination_order_compatibility(stored_data.destination_order, reader_info.destination_order)) {
        return false;
    }
    
    return true;
}

auto PersistenceService::check_durability_compatibility(DurabilityQosPolicyKind writer_kind, 
                                                       DurabilityQosPolicyKind reader_kind) const -> bool {
    // DURABILITY兼容性规则：DataWriter的kind >= DataReader的kind
    // VOLATILE(0) < TRANSIENT_LOCAL(1) < TRANSIENT(2) < PERSISTENT(3)
    return static_cast<int>(writer_kind) >= static_cast<int>(reader_kind);
}

auto PersistenceService::check_presentation_compatibility(const PresentationQosPolicy& writer_pres, 
                                                         const PresentationQosPolicy& reader_pres) const -> bool {
    // PRESENTATION兼容性规则：
    // 1. DataWriter的access_scope >= DataReader的access_scope
    // INSTANCE(0) < TOPIC(1) < GROUP(2)
    if (static_cast<int>(writer_pres.access_scope) < static_cast<int>(reader_pres.access_scope)) {
        return false;
    }
    
    // 2. DataReader的coherent_access为FALSE，或者DataWriter和DataReader的coherent_access都为TRUE
    if (reader_pres.coherent_access && !writer_pres.coherent_access) {
        return false;
    }
    
    // 3. DataReader的ordered_access为FALSE，或者DataWriter和DataReader的ordered_access都为TRUE
    if (reader_pres.ordered_access && !writer_pres.ordered_access) {
        return false;
    }
    
    return true;
}

auto PersistenceService::check_deadline_compatibility(const DeadlineQosPolicy& writer_deadline, 
                                                     const DeadlineQosPolicy& reader_deadline) const -> bool {
    // DEADLINE兼容性规则：DataWriter的deadline period <= DataReader的deadline period
    
    // 如果reader的deadline是INFINITE，则总是兼容
    if (reader_deadline.period.sec == DURATION_INFINITE_SEC && 
        reader_deadline.period.nanosec == DURATION_INFINITE_NSEC) {
        return true;
    }
    
    // 如果writer的deadline是INFINITE而reader不是，则不兼容
    if (writer_deadline.period.sec == DURATION_INFINITE_SEC && 
        writer_deadline.period.nanosec == DURATION_INFINITE_NSEC) {
        return false;
    }
    
    // 比较时间周期：writer_period <= reader_period
    if (writer_deadline.period.sec < reader_deadline.period.sec) {
        return true;
    } else if (writer_deadline.period.sec == reader_deadline.period.sec) {
        return writer_deadline.period.nanosec <= reader_deadline.period.nanosec;
    } else {
        return false;
    }
}

auto PersistenceService::check_latency_budget_compatibility(const LatencyBudgetQosPolicy& writer_budget, 
                                                           const LatencyBudgetQosPolicy& reader_budget) const -> bool {
    // LATENCY_BUDGET兼容性规则：DataWriter的duration <= DataReader的duration
    
    // 如果reader的latency budget是INFINITE，则总是兼容
    if (reader_budget.duration.sec == DURATION_INFINITE_SEC && 
        reader_budget.duration.nanosec == DURATION_INFINITE_NSEC) {
        return true;
    }
    
    // 如果writer的latency budget是INFINITE而reader不是，则不兼容
    if (writer_budget.duration.sec == DURATION_INFINITE_SEC && 
        writer_budget.duration.nanosec == DURATION_INFINITE_NSEC) {
        return false;
    }
    
    // 比较时间周期：writer_duration <= reader_duration
    if (writer_budget.duration.sec < reader_budget.duration.sec) {
        return true;
    } else if (writer_budget.duration.sec == reader_budget.duration.sec) {
        return writer_budget.duration.nanosec <= reader_budget.duration.nanosec;
    } else {
        return false;
    }
}

auto PersistenceService::check_ownership_compatibility(const OwnershipQosPolicy& writer_ownership, 
                                                      const OwnershipQosPolicy& reader_ownership) const -> bool {
    // OWNERSHIP兼容性规则：DataWriter和DataReader的kind必须完全匹配
    return writer_ownership.kind == reader_ownership.kind;
}

auto PersistenceService::check_liveliness_compatibility(const LivelinessQosPolicy& writer_liveliness, 
                                                       const LivelinessQosPolicy& reader_liveliness) const -> bool {
    // LIVELINESS兼容性规则：
    // 1. DataWriter的kind >= DataReader的kind
    // AUTOMATIC(0) < MANUAL_BY_PARTICIPANT(1) < MANUAL_BY_TOPIC(2)
    if (static_cast<int>(writer_liveliness.kind) < static_cast<int>(reader_liveliness.kind)) {
        return false;
    }
    
    // 2. DataWriter的lease_duration <= DataReader的lease_duration
    
    // 如果reader的lease_duration是INFINITE，则总是兼容
    if (reader_liveliness.lease_duration.sec == DURATION_INFINITE_SEC && 
        reader_liveliness.lease_duration.nanosec == DURATION_INFINITE_NSEC) {
        return true;
    }
    
    // 如果writer的lease_duration是INFINITE而reader不是，则不兼容
    if (writer_liveliness.lease_duration.sec == DURATION_INFINITE_SEC && 
        writer_liveliness.lease_duration.nanosec == DURATION_INFINITE_NSEC) {
        return false;
    }
    
    // 比较时间周期：writer_lease_duration <= reader_lease_duration
    if (writer_liveliness.lease_duration.sec < reader_liveliness.lease_duration.sec) {
        return true;
    } else if (writer_liveliness.lease_duration.sec == reader_liveliness.lease_duration.sec) {
        return writer_liveliness.lease_duration.nanosec <= reader_liveliness.lease_duration.nanosec;
    } else {
        return false;
    }
}

auto PersistenceService::check_reliability_compatibility(ReliabilityQosPolicyKind writer_kind, 
                                                        ReliabilityQosPolicyKind reader_kind) const -> bool {
    // RELIABILITY兼容性规则：DataWriter的kind >= DataReader的kind
    // BEST_EFFORT(0) < RELIABLE(1)
    return static_cast<int>(writer_kind) >= static_cast<int>(reader_kind);
}

auto PersistenceService::check_destination_order_compatibility(const DestinationOrderQosPolicy& writer_order, 
                                                              const DestinationOrderQosPolicy& reader_order) const -> bool {
    // DESTINATION_ORDER兼容性规则：DataWriter的kind >= DataReader的kind
    // BY_RECEPTION_TIMESTAMP(0) < BY_SOURCE_TIMESTAMP(1)
    return static_cast<int>(writer_order.kind) >= static_cast<int>(reader_order.kind);
}

void PersistenceService::cleanup_resources() {
    // 清理自身实体句柄记录
    {
        std::lock_guard<std::mutex> lock(own_entities_mutex_);
        own_datawriter_handles_.clear();
        own_datareader_handles_.clear();
        std::cout << "[PersistenceService] Cleared own entity handles" << std::endl;
    }
    
    // 清理writers
    {
        std::lock_guard<std::mutex> lock(writers_mutex_);
        topic_writers_.clear();
    }
    
    // 清理readers
    {
        std::lock_guard<std::mutex> lock(readers_mutex_);
        topic_readers_.clear();
        topic_durability_.clear();
    }
    
    // 安全清理handlers - 先调用shutdown，再重置每个handler，最后清理容器
    for (auto& [durability, handler] : handlers_) {
        if (handler) {
            handler->shutdown();
            std::cout << "[PersistenceService] Shutdown " 
                      << durability_kind_to_string(durability) << " handler" << std::endl;
            handler.reset();
        }
    }
    handlers_.clear();
    
    // 清理publisher
    if (publisher_ && participant_) {
        participant_->delete_publisher(publisher_);
        publisher_ = nullptr;
    }
    
    // 最后清理数据库管理器
    if (database_manager_) {
        database_manager_->shutdown();
        database_manager_.reset();
    }
}

void PersistenceService::worker_thread_main() {
    std::cout << "[PersistenceService] Worker thread started" << std::endl;
    
    while (!should_stop_) {
        std::unique_lock<std::mutex> lock(service_mutex_);
        cv_.wait_for(lock, std::chrono::seconds(1), [this] { return should_stop_.load(); });
        
        if (should_stop_) break;
        
        // 轮询所有DataReader的数据
        try {
            poll_data_readers();
        } catch (const std::exception& e) {
            std::cerr << "[PersistenceService] Data polling error: " << e.what() << std::endl;
        }
        
        // 执行定期清理任务（降低频率）
        static int cleanup_counter = 0;
        if (++cleanup_counter >= 10) { // 每10秒执行一次清理
            cleanup_counter = 0;
            try {
                for (auto& [kind, handler] : handlers_) {
                    if (handler) {
                        handler->cleanup_expired_data();
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "[PersistenceService] Worker thread error: " << e.what() << std::endl;
            }
        }
    }
    
    std::cout << "[PersistenceService] Worker thread stopped" << std::endl;
}

auto PersistenceService::get_handler_for_durability(DurabilityQosPolicyKind kind) -> DurabilityHandler* {
    auto handler_it = handlers_.find(kind);
    return (handler_it != handlers_.end()) ? handler_it->second.get() : nullptr;
}

void PersistenceService::store_data(const StoredData& data) {
    if (!is_running_) {
        std::cerr << "[PersistenceService] Service not running, cannot store data" << std::endl;
        return;
    }
    
    auto* handler = get_handler_for_durability(data.durability_kind);
    if (handler) {
        handler->store_data(data);
    } else {
        std::cerr << "[PersistenceService] No handler for durability kind: " 
                  << data.durability_kind << std::endl;
    }
}

auto PersistenceService::get_historical_data(const std::string& topic_name) -> std::vector<StoredData> {
    if (!is_running_) {
        return {};
    }
    
    // 尝试从所有处理器获取数据
    std::vector<StoredData> all_data;
    for (auto& [kind, handler] : handlers_) {
        if (handler) {
            auto handler_data = handler->get_historical_data(topic_name);
            all_data.insert(all_data.end(), handler_data.begin(), handler_data.end());
        }
    }
    
    // 按序列号排序
    std::sort(all_data.begin(), all_data.end(),
        [](const StoredData& first, const StoredData& second) {
            return first.sequence_number < second.sequence_number;
        });
    
    return all_data;
}

auto PersistenceService::get_historical_data_range(const std::string& topic_name,
                                                  const Time_t& start_time,
                                                  const Time_t& end_time) -> std::vector<StoredData> {
    if (!is_running_) {
        return {};
    }
    
    // 尝试从所有处理器获取数据
    std::vector<StoredData> all_data;
    for (auto& [kind, handler] : handlers_) {
        if (handler) {
            auto handler_data = handler->get_historical_data_range(topic_name, start_time, end_time);
            all_data.insert(all_data.end(), handler_data.begin(), handler_data.end());
        }
    }
    
    // 按序列号排序
    std::sort(all_data.begin(), all_data.end(),
        [](const StoredData& first, const StoredData& second) {
            return first.sequence_number < second.sequence_number;
        });
    
    return all_data;
}

auto PersistenceService::get_statistics() -> std::map<std::string, int64_t> {
    std::map<std::string, int64_t> stats;
    
    if (memory_cache_) {
        stats["memory_cache_size"] = memory_cache_->size();
        stats["memory_cache_bytes"] = memory_cache_->memory_usage();
    }
    
    if (database_manager_) {
        auto db_stats = database_manager_->get_database_stats();
        stats.insert(db_stats.begin(), db_stats.end());
    }
    
    return stats;
}

void PersistenceService::cleanup_old_data() {
    if (database_manager_) {
        database_manager_->cleanup_old_data(config_.data_retention);
    }
}

void PersistenceService::set_domain_participant(std::shared_ptr<DomainParticipant> participant) {
    participant_ = participant;
    
    // 创建Publisher用于历史数据发送
    if (participant_) {
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        if (!publisher_) {
            std::cerr << "[PersistenceService] Failed to create Publisher for historical data sending" << std::endl;
        }
    }
    
    // 设置发现监听器
    if (participant_ && is_running_) {
        setup_discovery_listeners();
    }
}

void PersistenceService::setup_discovery_listeners() {
    if (!participant_) {
        std::cerr << "[PersistenceService] Cannot setup discovery: no participant" << std::endl;
        return;
    }
      
    // 启动发现轮询线程
    discovery_running_ = true;
    discovery_thread_ = std::thread(&PersistenceService::discovery_polling_thread, this);
    
    std::cout << "[PersistenceService] Discovery polling thread started" << std::endl;
}

void PersistenceService::on_publication_discovered(const PublicationBuiltinTopicData& pub_data) {
    // 使用统一的策略检查方法
    if (!should_handle_durability(pub_data.durability.kind)) {
        std::cout << "[PersistenceService] " << durability_kind_to_string(pub_data.durability.kind) 
                  << " Publication detected: " << pub_data.topic_name 
                  << " - 持久化服务不处理" << durability_kind_to_string(pub_data.durability.kind) << "策略" << std::endl;
        return; 
    }
        
    std::cout << "[PersistenceService] Persistent Publication discovered: " 
              << pub_data.topic_name << " (" << durability_kind_to_string(pub_data.durability.kind) 
              << ") Type: " << pub_data.type_name << std::endl;
    
    // 保存Topic信息 
    {
        std::lock_guard<std::mutex> lock(topic_info_mutex_);
        TopicInfo& info = topic_info_map_[pub_data.topic_name];
        info.topic_name = pub_data.topic_name;
        info.type_name = pub_data.type_name;
        info.durability_kind = pub_data.durability.kind;
        info.reliability_kind = pub_data.reliability.kind;
        info.discovered_at = std::chrono::steady_clock::now();
    }
    
    // 注册topic信息到数据库
    if (database_manager_) {
        database_manager_->insert_or_update_topic(pub_data.topic_name, pub_data.type_name);
    }
    
    // 更新DataWriter注册表
    update_datawriter_registry(pub_data);
    
    // 创建匹配的DataReader来接收外部Writer的数据
    if (create_data_reader_for_topic(pub_data)) {
        std::cout << "[PersistenceService] Successfully created matching DataReader for external Writer" << std::endl;
    } else {
        std::cout << "[PersistenceService] Failed to create matching DataReader for external Writer" << std::endl;
    }
    
}

void PersistenceService::on_subscription_discovered(const SubscriptionBuiltinTopicData& sub_data) {
    // 使用统一的策略检查方法
    if (!should_handle_durability(sub_data.durability.kind)) {
        std::cout << "[PersistenceService] " << durability_kind_to_string(sub_data.durability.kind) 
                  << " Subscription detected: " << sub_data.topic_name 
                  << " - 持久化服务不处理" << durability_kind_to_string(sub_data.durability.kind) << "策略" << std::endl;
        return;
    }
    
    std::cout << "[PersistenceService] Persistent Subscription discovered: " 
              << sub_data.topic_name << " (" << durability_kind_to_string(sub_data.durability.kind) 
              << ") Type: " << sub_data.type_name << std::endl;
    
    // 保存Topic信息
    {
        std::lock_guard<std::mutex> lock(topic_info_mutex_);
        if (topic_info_map_.find(sub_data.topic_name) == topic_info_map_.end()) {
            TopicInfo& info = topic_info_map_[sub_data.topic_name];
            info.topic_name = sub_data.topic_name;
            info.type_name = sub_data.type_name;
            info.durability_kind = sub_data.durability.kind;
            info.reliability_kind = sub_data.reliability.kind;
            info.discovered_at = std::chrono::steady_clock::now();
        }
    }
    
    // 确保topic在数据库中注册
    if (database_manager_) {
        database_manager_->insert_or_update_topic(sub_data.topic_name, sub_data.type_name);
    }
    
    // 更新DataReader注册表
    update_datareader_registry(sub_data);
    
}

// 根据外部Writer的QoS创建匹配的DataReader
auto PersistenceService::create_data_reader_for_topic(const PublicationBuiltinTopicData& pub_data) -> bool {
    // 使用统一的策略检查方法
    if (!should_handle_durability(pub_data.durability.kind)) {
        std::cout << "[PersistenceService] 跳过为" << durability_kind_to_string(pub_data.durability.kind) 
                  << " Writer创建匹配DataReader: " << pub_data.topic_name << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(readers_mutex_);
    
    // 检查是否已经为此topic创建了reader
    if (topic_readers_.find(pub_data.topic_name) != topic_readers_.end()) {
        std::cout << "[PersistenceService] DataReader already exists for topic: " << pub_data.topic_name << std::endl;
        return true;
    }
    
    if (!participant_) {
        std::cout << "[PersistenceService] DomainParticipant not available" << std::endl;
        return false;
    }
    
    try {
        // 创建Subscriber - 配置与Writer兼容的QoS
        SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;
        subscriber_qos.presentation = pub_data.presentation;
        
        auto* subscriber = participant_->create_subscriber(subscriber_qos, nullptr, STATUS_MASK_NONE);
        if (subscriber == nullptr) {
            std::cout << "[PersistenceService] Failed to create Subscriber" << std::endl;
            return false;
        }
        
        // 创建Topic (先查找是否已存在)
        Duration_t timeout = {0, 0}; // 0秒超时，立即返回
        auto* topic = participant_->find_topic(pub_data.topic_name, timeout);
        if (topic == nullptr) {
            // Topic不存在，创建新的
            topic = participant_->create_topic(pub_data.topic_name, pub_data.type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
            if (topic == nullptr) {
                std::cout << "[PersistenceService] Failed to create Topic: " << pub_data.topic_name << std::endl;
                return false;
            }
            std::cout << "[PersistenceService] Created new Topic: " << pub_data.topic_name << std::endl;
        } else {
            std::cout << "[PersistenceService] Using existing Topic: " << pub_data.topic_name << std::endl;
        }
        
        // 设置DataReader QoS - 完全匹配Writer的QoS以确保兼容性
        DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
        reader_qos.durability.kind = pub_data.durability.kind;
        reader_qos.reliability.kind = pub_data.reliability.kind;
        reader_qos.deadline = pub_data.deadline;
        reader_qos.latency_budget = pub_data.latency_budget;
        reader_qos.ownership = pub_data.ownership;
        reader_qos.liveliness = pub_data.liveliness;
        reader_qos.destination_order = pub_data.destination_order;
        reader_qos.history.kind = KEEP_ALL_HISTORY_QOS;  // 持久化服务使用KEEP_ALL
        reader_qos.time_based_filter.minimum_separation.sec = 0;
        reader_qos.time_based_filter.minimum_separation.nanosec = 0;
        
        std::cout << "[PersistenceService] Creating DataReader with matching QoS for Writer:" << std::endl;
        std::cout << "  Topic: " << pub_data.topic_name << ", Type: " << pub_data.type_name << std::endl;
        std::cout << "  Durability: " << durability_kind_to_string(pub_data.durability.kind) << std::endl;
        std::cout << "  Reliability: " << (pub_data.reliability.kind == RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT") << std::endl;
        
        // 创建DataReader
        auto* reader = subscriber->create_datareader(topic, reader_qos, nullptr, STATUS_MASK_NONE);
        if (reader == nullptr) {
            std::cout << "[PersistenceService] Failed to create DataReader for topic: " << pub_data.topic_name << std::endl;
            return false;
        }
        
        // 保存reader信息
        topic_readers_[pub_data.topic_name] = std::shared_ptr<DataReader>(reader);
        topic_durability_[pub_data.topic_name] = pub_data.durability.kind;
        
        // 记录自己创建的DataReader句柄
        {
            std::lock_guard<std::mutex> entities_lock(own_entities_mutex_);
            own_datareader_handles_.insert(reader->get_instance_handle());
        }
        
        std::cout << "[PersistenceService] Successfully created matching DataReader for external Writer on topic: " 
                  << pub_data.topic_name << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "[PersistenceService] Exception creating matching DataReader: " << e.what() << std::endl;
        return false;
    }
}

void PersistenceService::poll_data_readers() {
    std::lock_guard<std::mutex> lock(readers_mutex_);
    
    for (const auto& [topic_name, reader] : topic_readers_) {
        if (!reader || !is_running_) {
            continue;
        }
        
        try {
            // 改用循环读取所有可用的样本
            HelloWorld sample;
            SampleInfo sample_info;
            
            // 循环读取直到没有更多数据
            while (true) {
                ReturnCode_t result = reader->read_next_sample(&sample, sample_info);
                
                if (result == RETCODE_NO_DATA) {
                    // 没有更多数据，退出循环
                    break;
                } else if (result != RETCODE_OK) {
                    // 其他错误，记录并退出循环
                    std::cout << "[PersistenceService] Error reading from topic " << topic_name 
                              << ", return code: " << static_cast<int>(result) << std::endl;
                    break;
                }
                
                // 只处理有效数据
                if (!sample_info.valid_data) {
                    continue;
                }
                
                // 获取topic的durability类型和type_name
                auto durability_it = topic_durability_.find(topic_name);
                if (durability_it == topic_durability_.end()) {
                    std::cout << "[PersistenceService] Warning: No durability info for topic: " << topic_name << std::endl;
                    continue;
                }
                
                DurabilityQosPolicyKind durability = durability_it->second;
                
                // 获取type_name 
                std::string real_type_name;
                {
                    std::lock_guard<std::mutex> lock(topic_info_mutex_);
                    auto topic_info_it = topic_info_map_.find(topic_name);
                    if (topic_info_it != topic_info_map_.end()) {
                        real_type_name = topic_info_it->second.type_name;
                    } else {
                        std::cout << "[PersistenceService] Warning: No type info for topic: " << topic_name 
                                  << ", using fallback" << std::endl;
                        real_type_name = "UnknownType"; // 降级处理
                    }
                }
                
                // 创建StoredData对象
                StoredData stored_data;
                stored_data.topic_name = topic_name;
                stored_data.type_name = real_type_name; 
                stored_data.instance_id = "instance_" + std::to_string(reinterpret_cast<uintptr_t>(&sample_info.instance_handle));
                stored_data.is_disposed = (sample_info.instance_state == NOT_ALIVE_DISPOSED_INSTANCE_STATE);
                
                // 从sample_info获取Writer的GUID
                // 注意：sample_info.publication_handle包含了Writer的GUID信息
                stored_data.writer_guid = Conversion::ToGuid(sample_info.publication_handle);
                
                // 查找对应Writer的完整QoS信息
                {
                    std::lock_guard<std::mutex> lock(writers_mutex_);
                    auto writer_it = active_writers_info_.find(Conversion::ToBuiltinTopicKey(stored_data.writer_guid));
                    if (writer_it != active_writers_info_.end()) {
                        const auto& writer_info = writer_it->second;
                        // 复制完整的QoS信息
                        stored_data.durability_kind = writer_info.durability_kind;
                        stored_data.reliability_kind = writer_info.reliability_kind;
                        stored_data.presentation = writer_info.presentation;
                        stored_data.deadline = writer_info.deadline;
                        stored_data.latency_budget = writer_info.latency_budget;
                        stored_data.ownership = writer_info.ownership;
                        stored_data.ownership_strength = writer_info.ownership_strength;
                        stored_data.liveliness = writer_info.liveliness;
                        stored_data.destination_order = writer_info.destination_order;
                        stored_data.lifespan = writer_info.lifespan;
                    } else {
                        // 如果找不到Writer信息，使用默认值（向后兼容）
                        std::cout << "[PersistenceService] Warning: Writer QoS info not found for GUID, using defaults" << std::endl;
                        stored_data.durability_kind = durability;
                        stored_data.reliability_kind = BEST_EFFORT_RELIABILITY_QOS;
                        // 其他QoS字段保持默认初始化
                    }
                }
                
                // 设置时间戳
                stored_data.storage_time.sec = sample_info.source_timestamp.sec;
                stored_data.storage_time.nanosec = sample_info.source_timestamp.nanosec;
                
                // 设置序列号
                stored_data.sequence_number = sample_info.write_sequence_number;
                
                // 使用DDS原生TypeSupport序列化完整对象
                try {
                    HelloWorldTypeSupport* type_support = HelloWorldTypeSupport::get_instance();
                    if (type_support != nullptr) {
                        // 获取序列化所需的缓冲区大小
                        uint32_t data_size = type_support->get_serialized_data_size(&sample, 0);
                        
                        // 创建序列化缓冲区
                        SerializedBuffer buffer;
                        
                        // 创建CDR序列化器
                        CdrSerializer serializer(&buffer, true); // 使用小端字节序
                        
                        // 执行序列化
                        int result = type_support->serialize_data(&sample, &serializer, 1);
                        if (result == 0) {
                            // 序列化成功，保存完整的二进制数据
                            uint32_t total_length = buffer.bufferTotalLength();
                            if (total_length > 0) {
                                stored_data.data.resize(total_length);
                                
                                // 复制缓冲区数据
                                uint32_t copied = 0;
                                for (SerializedBuffer* current = &buffer; current != nullptr && copied < total_length; current = current->next) {
                                    uint32_t current_length = current->bufferLength();
                                    if (current_length > 0 && current->buffer != nullptr) {
                                        std::memcpy(stored_data.data.data() + copied, 
                                                   current->buffer + current->reader_position, 
                                                   current_length);
                                        copied += current_length;
                                    }
                                }
                                
                                std::cout << "[PersistenceService] Successfully serialized HelloWorld object "
                                          << "(index: " << sample.index << ", message: '" << sample.message 
                                          << "') to " << total_length << " bytes" << std::endl;
                            } else {
                                // 序列化结果为空，回退到手动提取
                                std::cerr << "[PersistenceService] Serialization produced empty buffer, falling back to manual extraction" << std::endl;
                                std::string sample_data = sample.message;
                                stored_data.data.assign(sample_data.begin(), sample_data.end());
                            }
                        } else {
                            // 序列化失败，回退到手动提取
                            std::cerr << "[PersistenceService] TypeSupport serialization failed, falling back to manual extraction" << std::endl;
                            std::string sample_data = sample.message;
                            stored_data.data.assign(sample_data.begin(), sample_data.end());
                        }
                    } else {
                        // TypeSupport不可用，回退到手动提取
                        std::cerr << "[PersistenceService] TypeSupport instance not available, using manual extraction" << std::endl;
                        std::string sample_data = sample.message;
                        stored_data.data.assign(sample_data.begin(), sample_data.end());
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[PersistenceService] Exception during serialization: " << e.what() 
                              << ", falling back to manual extraction" << std::endl;
                    std::string sample_data = sample.message;
                    stored_data.data.assign(sample_data.begin(), sample_data.end());
                }
                
                // 设置完整的sample info - 生产级持久化服务必须保存完整状态
                stored_data.sample_info = sample_info;
                
                std::cout << "[PersistenceService] Received REAL data for topic: " << topic_name 
                          << " (type: " << real_type_name << "), message: '" << sample.message 
                          << "', durability: " << static_cast<int>(durability) 
                          << ", sequence: " << stored_data.sequence_number << std::endl;
                
                // 存储数据
                store_data(stored_data);
            }
            
        } catch (const std::exception& e) {
            std::cout << "[PersistenceService] Exception polling reader for topic " 
                      << topic_name << ": " << e.what() << std::endl;
        }
    }
}

// 根据外部Reader的QoS创建匹配的DataWriter
auto PersistenceService::create_data_writer_for_topic(const SubscriptionBuiltinTopicData& sub_data) -> bool {
    // 使用统一的策略检查方法
    if (!should_handle_durability(sub_data.durability.kind)) {
        std::cout << "[PersistenceService] 跳过为" << durability_kind_to_string(sub_data.durability.kind) 
                  << " Reader创建匹配DataWriter: " << sub_data.topic_name << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(writers_mutex_);
    
    // 检查是否已经为此topic创建了writer
    if (topic_writers_.find(sub_data.topic_name) != topic_writers_.end()) {
        std::cout << "[PersistenceService] DataWriter already exists for topic: " << sub_data.topic_name << std::endl;
        return true;
    }
    
    if (!participant_ || !publisher_) {
        std::cout << "[PersistenceService] DomainParticipant or Publisher not available" << std::endl;
        return false;
    }
    
    try {
        // 创建Topic (先查找是否已存在)
        Duration_t timeout = {0, 0}; // 0秒超时，立即返回
        auto* topic = participant_->find_topic(sub_data.topic_name, timeout);
        if (topic == nullptr) {
            // Topic不存在，创建新的
            topic = participant_->create_topic(sub_data.topic_name, sub_data.type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
            if (topic == nullptr) {
                std::cout << "[PersistenceService] Failed to create Topic: " << sub_data.topic_name << std::endl;
                return false;
            }
            std::cout << "[PersistenceService] Created new Topic: " << sub_data.topic_name << std::endl;
        } else {
            std::cout << "[PersistenceService] Using existing Topic: " << sub_data.topic_name << std::endl;
        }
        
        // 设置DataWriter QoS - 完全匹配Reader的QoS以确保兼容性
        DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
        
        // 基础QoS - 需要满足兼容性规则
        writer_qos.durability.kind = sub_data.durability.kind;  // 匹配Reader的durability
        writer_qos.reliability.kind = sub_data.reliability.kind;  // 匹配Reader的reliability
        writer_qos.deadline = sub_data.deadline;
        writer_qos.latency_budget = sub_data.latency_budget;
        writer_qos.ownership = sub_data.ownership;
        writer_qos.liveliness = sub_data.liveliness;
        writer_qos.destination_order = sub_data.destination_order;
        writer_qos.history.kind = KEEP_ALL_HISTORY_QOS;  // 持久化服务使用KEEP_ALL
        
        std::cout << "[PersistenceService] Creating DataWriter with matching QoS for Reader:" << std::endl;
        std::cout << "  Topic: " << sub_data.topic_name << ", Type: " << sub_data.type_name << std::endl;
        std::cout << "  Durability: " << durability_kind_to_string(sub_data.durability.kind) << std::endl;
        std::cout << "  Reliability: " << (sub_data.reliability.kind == RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT") << std::endl;
        
        // 创建DataWriter
        auto* writer = publisher_->create_datawriter(topic, writer_qos, nullptr, STATUS_MASK_NONE);
        if (writer == nullptr) {
            std::cout << "[PersistenceService] Failed to create DataWriter for topic: " << sub_data.topic_name << std::endl;
            return false;
        }
        
        // 保存writer信息
        topic_writers_[sub_data.topic_name] = std::shared_ptr<DataWriter>(writer);
        
        // 记录自己创建的DataWriter句柄
        {
            std::lock_guard<std::mutex> entities_lock(own_entities_mutex_);
            own_datawriter_handles_.insert(writer->get_instance_handle());
        }
        
        std::cout << "[PersistenceService] Successfully created matching DataWriter for external Reader on topic: " 
                  << sub_data.topic_name << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "[PersistenceService] Exception creating matching DataWriter: " << e.what() << std::endl;
        return false;
    }
}

void PersistenceService::send_historical_data_to_subscriber(const std::string& topic_name, 
                                                           const std::string& type_name, 
                                                           DurabilityQosPolicyKind durability,
                                                           const BuiltinTopicKey_t& reader_key) {
    // 智能发送检查：如果有活跃的DataWriter，则不发送历史数据
    if (!should_send_historical_data(topic_name)) {
        std::cout << "[PersistenceService] Skipping historical data send for topic '" 
                  << topic_name << "' due to active DataWriter(s)" << std::endl;
        return;
    }
    
    // 获取目标DataReader的QoS信息
    ActiveDataReaderInfo reader_info;
    bool reader_found = false;
    {
        std::lock_guard<std::mutex> lock(readers_mutex_);
        auto reader_it = active_readers_info_.find(reader_key);
        if (reader_it != active_readers_info_.end()) {
            reader_info = reader_it->second;
            reader_found = true;
        }
    }
    
    if (!reader_found) {
        std::cout << "[PersistenceService] DataReader not found for key" << std::endl;
        return;
    }
    
    // 获取历史数据
    auto historical_data = get_historical_data(topic_name);
    if (historical_data.empty()) {
        std::cout << "[PersistenceService] No historical data found for topic: " << topic_name << std::endl;
        return;
    }
    
    // 过滤QoS兼容的历史数据
    std::vector<StoredData> compatible_data;
    for (const auto& stored_data : historical_data) {
        // 检查存储数据的QoS与目标DataReader的QoS是否兼容
        if (are_qos_compatible(stored_data, reader_info)) {
            compatible_data.push_back(stored_data);
        } else {
            std::cout << "[PersistenceService] Skipping incompatible data: seq=" 
                      << stored_data.sequence_number << " (QoS mismatch with DataReader)" << std::endl;
        }
    }
    
    if (compatible_data.empty()) {
        std::cout << "[PersistenceService] No QoS-compatible historical data found for DataReader" << std::endl;
        return;
    }
    
    std::cout << "[PersistenceService] Found " << compatible_data.size() 
              << " QoS-compatible samples out of " << historical_data.size() 
              << " total for topic: " << topic_name << std::endl;
    
    // 查找或创建用于发送历史数据的DataWriter
    std::shared_ptr<DataWriter> writer;
    {
        std::lock_guard<std::mutex> lock(writers_mutex_);
        auto writer_it = topic_writers_.find(topic_name);
        if (writer_it != topic_writers_.end()) {
            writer = writer_it->second;
        }
    }

    // 如果Writer不存在，则根据Reader的QoS信息创建它
    if (!writer) {
        std::cout << "[PersistenceService] DataWriter for topic '" << topic_name 
                  << "' not found, creating one based on Reader's QoS..." << std::endl;
        if (create_data_writer_for_topic(reader_info.full_subscription_data)) {
            std::lock_guard<std::mutex> lock(writers_mutex_);
            auto writer_it = topic_writers_.find(topic_name);
            if (writer_it != topic_writers_.end()) {
                writer = writer_it->second;
            }
        }
    }
    
    if (!writer) {
        std::cout << "[PersistenceService] Failed to find or create DataWriter for topic: " << topic_name << std::endl;
        return;
    }
    
    // 发送历史数据
    size_t sent_count = 0;
    for (const auto& stored_data : compatible_data) {
        try {
            // 使用TypeSupport反序列化完整对象
            HelloWorld sample;
            bool deserialization_success = false;
            
            if (!stored_data.data.empty()) {
                try {
                    HelloWorldTypeSupport* type_support = HelloWorldTypeSupport::get_instance();
                    if (type_support != nullptr) {
                        // 创建序列化缓冲区并填充数据
                        SerializedBuffer buffer;
                        
                        // 将存储的数据复制到缓冲区（简化处理，假设数据在单个缓冲区中）
                        if (!stored_data.data.empty()) {
                            // 创建临时缓冲区来存储数据
                            char* temp_buffer = new char[stored_data.data.size()];
                            std::memcpy(temp_buffer, stored_data.data.data(), stored_data.data.size());
                            
                            // 设置缓冲区属性
                            buffer.buffer = temp_buffer;
                            buffer.origin_buffer = temp_buffer;
                            buffer.buffer_size = stored_data.data.size();
                            buffer.writer_position = stored_data.data.size();
                            buffer.reader_position = 0;
                            buffer.next = nullptr;
                            
                            // 创建CDR反序列化器
                            CdrDeserializer deserializer(&buffer, true); // 使用小端字节序
                            
                            // 执行反序列化
                            int result = type_support->deserialize_data(&sample, &deserializer, 1);
                            if (result == 0) {
                                deserialization_success = true;
                                std::cout << "[PersistenceService] Successfully deserialized HelloWorld object "
                                          << "(index: " << sample.index << ", message: '" << sample.message 
                                          << "') from " << stored_data.data.size() << " bytes" << std::endl;
                            } else {
                                std::cerr << "[PersistenceService] TypeSupport deserialization failed, result: " << result << std::endl;
                            }
                            
                            // 清理临时缓冲区
                            delete[] temp_buffer;
                        }
                    } else {
                        std::cerr << "[PersistenceService] TypeSupport instance not available for deserialization" << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[PersistenceService] Exception during deserialization: " << e.what() << std::endl;
                }
                
                // 如果TypeSupport反序列化失败，回退到手动提取
                if (!deserialization_success) {
                    std::cout << "[PersistenceService] Falling back to manual data extraction" << std::endl;
                    std::string message_str(stored_data.data.begin(), stored_data.data.end());
                    sample.message = message_str;
                    sample.index = 0; // 默认值，因为手动提取时没有index信息
                }
            }
            
            // 发送数据
            ReturnCode_t result = writer->write(&sample, HANDLE_NIL);
            if (result == RETCODE_OK) {
                sent_count++;
                std::cout << "[PersistenceService] Sent historical data: seq=" 
                          << stored_data.sequence_number << ", message='" << sample.message << "'" << std::endl;
            } else {
                std::cout << "[PersistenceService] Failed to send historical data: seq=" 
                          << stored_data.sequence_number << ", error=" << static_cast<int>(result) << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "[PersistenceService] Exception sending historical data: " << e.what() << std::endl;
        }
    }
    
    std::cout << "[PersistenceService] Historical data sending completed for topic: " 
              << topic_name << ". Sent " << sent_count << " samples out of " 
              << compatible_data.size() << " QoS-compatible samples (total stored: " 
              << historical_data.size() << ")." << std::endl;
}

void PersistenceService::send_historical_data_for_handler(const std::string& topic_name, 
                                                         const std::string& type_name, 
                                                         DurabilityQosPolicyKind durability,
                                                         const BuiltinTopicKey_t& reader_key) {
    // 委托给私有的send_historical_data_to_subscriber方法
    send_historical_data_to_subscriber(topic_name, type_name, durability, reader_key);
}

// 发现轮询线程
void PersistenceService::discovery_polling_thread() {
    std::cout << "[PersistenceService] Discovery polling thread started" << std::endl;
    
    // 用于追踪已处理的publication和subscription
    std::set<std::string> processed_publications;
    std::set<std::string> processed_subscriptions;
    
    while (discovery_running_ && !should_stop_) {
        try {
            // 检查新的publications
            check_new_publications(processed_publications);
            
            // 检查新的subscriptions  
            check_new_subscriptions(processed_subscriptions);
            
            // 等待2秒再检查
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
        } catch (const std::exception& e) {
            std::cerr << "[PersistenceService] Discovery polling error: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 错误时等待更短时间
        }
    }
    
    std::cout << "[PersistenceService] Discovery polling thread stopped" << std::endl;
}

void PersistenceService::check_new_publications(std::set<std::string>& processed_publications) {
    if (!participant_) {
        return;
    }
    
    try {
        InstanceHandleSeq publication_handles;
        
        // 使用DDS标准API获取所有publications
        ReturnCode_t ret = participant_->get_publications(publication_handles, true);
        if (ret != RETCODE_OK) {
            std::cerr << "[PersistenceService] Failed to get publications: " << ret << std::endl;
            return;
        }
        
        // 检查每个publication
        for (const auto& handle : publication_handles) {
            std::string handle_str = handle_to_string(handle);

            // 检查是否已经处理过这个publication
            if (processed_publications.find(handle_str) != processed_publications.end()) {
                continue; // 已经处理过，跳过
            }
            
            // 获取publication详细数据
            PublicationBuiltinTopicData pub_data;
            ret = participant_->get_publication_data(pub_data, handle);
            if (ret != RETCODE_OK) {
                std::cerr << "[PersistenceService] Failed to get publication data for handle " 
                          << handle_str << ": " << ret << std::endl;
                continue;
            }
            
            // 标记为已处理
            processed_publications.insert(handle_str);
            
            // 检查是否是自己创建的DataWriter - 避免存储自己的实体
            {
                std::lock_guard<std::mutex> lock(own_entities_mutex_);
                if (own_datawriter_handles_.find(handle) != own_datawriter_handles_.end()) {
                    std::cout << "[PersistenceService] Skipping own DataWriter: topic='" 
                              << pub_data.topic_name << "', type='" << pub_data.type_name << "'" << std::endl;
                    continue; // 跳过自己创建的DataWriter
                }
            }
            
            // 处理发现的publication
            std::cout << "[PersistenceService] Discovered publication: topic='" 
                      << pub_data.topic_name << "', type='" << pub_data.type_name 
                      << "', durability=" << pub_data.durability.kind << std::endl;
            
            // 更新DataWriter注册表
            update_datawriter_registry(pub_data);
            
            on_publication_discovered(pub_data);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceService] Exception in check_new_publications: " << e.what() << std::endl;
    }
}

void PersistenceService::check_new_subscriptions(std::set<std::string>& processed_subscriptions) {
    if (!participant_) {
        return;
    }
    
    try {
        InstanceHandleSeq subscription_handles;
        
        // 使用DDS标准API获取所有subscriptions
        ReturnCode_t ret = participant_->get_subscriptions(subscription_handles, true);
        if (ret != RETCODE_OK) {
            std::cerr << "[PersistenceService] Failed to get subscriptions: " << ret << std::endl;
            return;
        }
        
        // 检查每个subscription
        for (const auto& handle : subscription_handles) {
            std::string handle_str = handle_to_string(handle);
            
            // 检查是否已经处理过这个subscription
            if (processed_subscriptions.find(handle_str) != processed_subscriptions.end()) {
                continue; // 已经处理过，跳过
            }
            
            // 获取subscription详细数据
            SubscriptionBuiltinTopicData sub_data;
            ret = participant_->get_subscription_data(sub_data, handle);
            if (ret != RETCODE_OK) {
                std::cerr << "[PersistenceService] Failed to get subscription data for handle " 
                          << handle_str << ": " << ret << std::endl;
                continue;
            }
            
            // 标记为已处理
            processed_subscriptions.insert(handle_str);
            
            //检查是否是自己创建的DataReader - 避免存储自己的实体
            {
                std::lock_guard<std::mutex> lock(own_entities_mutex_);
                if (own_datareader_handles_.find(handle) != own_datareader_handles_.end()) {
                    std::cout << "[PersistenceService] Skipping own DataReader: topic='" 
                              << sub_data.topic_name << "', type='" << sub_data.type_name << "'" << std::endl;
                    continue; // 跳过自己创建的DataReader
                }
            }
            
            // 处理发现的subscription
            std::cout << "[PersistenceService] Discovered subscription: topic='" 
                      << sub_data.topic_name << "', type='" << sub_data.type_name 
                      << "', durability=" << sub_data.durability.kind << std::endl;
            
            // 更新DataReader注册表
            update_datareader_registry(sub_data);
            
            on_subscription_discovered(sub_data);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceService] Exception in check_new_subscriptions: " << e.what() << std::endl;
    }
}

auto PersistenceService::handle_to_string(const InstanceHandle_t& handle) -> std::string {
    std::ostringstream oss;
    for (const auto& byte : handle.keyHash.value) {
        oss << std::hex << std::setfill('0') << std::setw(2) 
            << static_cast<unsigned int>(byte);
    }
    return oss.str();
}

// ===== DataWriter/DataReader 监控相关实现 =====

void PersistenceService::entity_monitoring_thread_main() {
    std::cout << "[PersistenceService] Entity monitoring thread started" << std::endl;
    
    while (monitoring_running_.load()) {
        try {
            // 检查实体活跃状态
            check_entities_active_status();
            
            // 清理不活跃的实体
            cleanup_inactive_entities();
            
            // 等待下一次监控间隔
            std::this_thread::sleep_for(smart_config_.monitoring_interval);
            
        } catch (const std::exception& e) {
            std::cerr << "[PersistenceService] Exception in entity monitoring thread: " 
                      << e.what() << std::endl;
        }
    }
    
    std::cout << "[PersistenceService] Entity monitoring thread stopped" << std::endl;
}

void PersistenceService::update_datawriter_registry(const PublicationBuiltinTopicData& pub_data) {
    try {
        // 存储到内存而不是数据库
        std::lock_guard<std::mutex> lock(entity_info_mutex_);
        
        auto now = std::chrono::steady_clock::now();
        
        // 检查是否已存在
        auto writer_it = active_writers_info_.find(pub_data.key);
        if (writer_it != active_writers_info_.end()) {
            // 实体已存在，只更新last_seen时间
            writer_it->second.last_seen = now;
            writer_it->second.is_active = true;  // 重新发现时标记为活跃
        } else {
            // 新实体，创建完整信息
            ActiveDataWriterInfo writer_info;
            writer_info.key = pub_data.key;
            writer_info.topic_name = pub_data.topic_name;
            writer_info.type_name = pub_data.type_name;
            writer_info.durability_kind = pub_data.durability.kind;
            writer_info.reliability_kind = pub_data.reliability.kind;
            writer_info.discovered_at = now;
            writer_info.last_seen = now;
            writer_info.is_active = true;
            
            // 收集完整的QoS信息用于兼容性检查
            writer_info.presentation = pub_data.presentation;
            writer_info.deadline = pub_data.deadline;
            writer_info.latency_budget = pub_data.latency_budget;
            writer_info.ownership = pub_data.ownership;
            writer_info.ownership_strength = pub_data.ownership_strength;
            writer_info.liveliness = pub_data.liveliness;
            writer_info.destination_order = pub_data.destination_order;
            writer_info.lifespan = pub_data.lifespan;
            
            // 存储完整的Publication数据以备将来使用
            writer_info.full_publication_data = pub_data;
            
            active_writers_info_[pub_data.key] = writer_info;
            
            std::cout << "[PersistenceService] Registered new DataWriter for topic '" 
                      << pub_data.topic_name << "' (in memory)" << std::endl;
        }
        
        // 同时更新活跃实体列表
        {
            std::lock_guard<std::mutex> entity_lock(entity_monitoring_mutex_);
            active_datawriters_[pub_data.topic_name].insert(pub_data.key);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceService] Exception updating DataWriter registry: " 
                  << e.what() << std::endl;
    }
}

void PersistenceService::update_datareader_registry(const SubscriptionBuiltinTopicData& sub_data) {
    try {
        bool is_new_reader = false;
        std::string topic_name_copy = sub_data.topic_name;  // 拷贝一份topic_name
        
        // 存储到内存而不是数据库
        {
            std::lock_guard<std::mutex> lock(entity_info_mutex_);
            
            auto now = std::chrono::steady_clock::now();
            
            // 检查是否已存在
            auto existing_reader = active_readers_info_.find(sub_data.key);
            if (existing_reader != active_readers_info_.end()) {
                // 实体已存在，只更新last_seen时间
                existing_reader->second.last_seen = now;
                existing_reader->second.is_active = true;  // 重新发现时标记为活跃
            } else {
                // 新实体，创建完整信息
                ActiveDataReaderInfo reader_info;
                reader_info.key = sub_data.key;
                reader_info.topic_name = sub_data.topic_name;
                reader_info.type_name = sub_data.type_name;
                reader_info.durability_kind = sub_data.durability.kind;
                reader_info.reliability_kind = sub_data.reliability.kind;
                reader_info.discovered_at = now;
                reader_info.last_seen = now;
                reader_info.is_active = true;
                
                // 收集完整的QoS信息用于兼容性检查
                reader_info.presentation = sub_data.presentation;
                reader_info.deadline = sub_data.deadline;
                reader_info.latency_budget = sub_data.latency_budget;
                reader_info.ownership = sub_data.ownership;
                reader_info.liveliness = sub_data.liveliness;
                reader_info.destination_order = sub_data.destination_order;
                reader_info.time_based_filter = sub_data.time_based_filter;
                
                // 存储完整的Subscription数据以备将来使用
                reader_info.full_subscription_data = sub_data;
                
                active_readers_info_[sub_data.key] = reader_info;
                is_new_reader = true;
                
                std::cout << "[PersistenceService] Registered new DataReader for topic '" 
                          << sub_data.topic_name << "' (in memory)" << std::endl;
            }
        }
        
        // 同时更新活跃实体列表
        {
            std::lock_guard<std::mutex> entity_lock(entity_monitoring_mutex_);
            active_datareaders_[sub_data.topic_name].insert(sub_data.key);
        }
        
        // 检查是否需要发送历史数据（在锁外执行，避免死锁）
        if (is_new_reader && should_send_historical_data(topic_name_copy)) {
            std::cout << "[PersistenceService] Triggering historical data send for topic '" 
                      << topic_name_copy << "'" << std::endl;
            // 实际发送历史数据
            send_historical_data_to_subscriber(sub_data.topic_name, sub_data.type_name, sub_data.durability.kind, sub_data.key);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceService] Exception updating DataReader registry: " 
                  << e.what() << std::endl;
    }
}

void PersistenceService::check_entities_active_status() {
    std::lock_guard<std::mutex> lock(entity_info_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    // 检查DataWriter活跃状态
    for (auto& [key, writer_info] : active_writers_info_) {
        auto time_since_last_seen = now - writer_info.last_seen;
        bool was_active = writer_info.is_active;
        
        // 如果超过超时时间，标记为不活跃
        if (time_since_last_seen > smart_config_.entity_timeout) {
            writer_info.is_active = false;
            if (was_active) {
                std::cout << "[PersistenceService] DataWriter for topic '" 
                          << writer_info.topic_name << "' marked as inactive (timeout)" << std::endl;
            }
        } else {
            writer_info.is_active = true;
            if (!was_active) {
                std::cout << "[PersistenceService] DataWriter for topic '" 
                          << writer_info.topic_name << "' marked as active" << std::endl;
            }
        }
    }
    
    // 检查DataReader活跃状态
    for (auto& [key, reader_info] : active_readers_info_) {
        auto time_since_last_seen = now - reader_info.last_seen;
        bool was_active = reader_info.is_active;
        
        // 如果超过超时时间，标记为不活跃
        if (time_since_last_seen > smart_config_.entity_timeout) {
            reader_info.is_active = false;
            if (was_active) {
                std::cout << "[PersistenceService] DataReader for topic '" 
                          << reader_info.topic_name << "' marked as inactive (timeout)" << std::endl;
            }
        } else {
            reader_info.is_active = true;
            if (!was_active) {
                std::cout << "[PersistenceService] DataReader for topic '" 
                          << reader_info.topic_name << "' marked as active" << std::endl;
            }
        }
    }
}

void PersistenceService::cleanup_inactive_entities() {
    try {
        auto timeout_duration = smart_config_.entity_timeout;
        auto now = std::chrono::steady_clock::now();
        
        // 获取需要清理的实体列表（在锁外面创建列表）
        std::vector<BuiltinTopicKey_t> writers_to_remove;
        std::vector<BuiltinTopicKey_t> readers_to_remove;
        
        {
            std::lock_guard<std::mutex> lock(entity_info_mutex_);
            
            // 检查inactive的DataWriters
            for (auto it = active_writers_info_.begin(); it != active_writers_info_.end(); ) {
                const auto& [key, writer_info] = *it;
                auto time_since_last_seen = now - writer_info.last_seen;
                
                if (time_since_last_seen > timeout_duration || !writer_info.is_active) {
                    writers_to_remove.push_back(key);
                    std::cout << "[PersistenceService] Marking inactive writer for removal - Topic: " 
                              << writer_info.topic_name 
                              << ", Key: [" << key.value[0] << "," << key.value[1] << "," << key.value[2] << "," << key.value[3] << "]"
                              << ", Inactive for: " << std::chrono::duration_cast<std::chrono::seconds>(time_since_last_seen).count() 
                              << " seconds" << std::endl;
                }
                ++it;
            }
            
            // 检查inactive的DataReaders
            for (auto it = active_readers_info_.begin(); it != active_readers_info_.end(); ) {
                const auto& [key, reader_info] = *it;
                auto time_since_last_seen = now - reader_info.last_seen;
                
                if (time_since_last_seen > timeout_duration || !reader_info.is_active) {
                    readers_to_remove.push_back(key);
                    std::cout << "[PersistenceService] Marking inactive reader for removal - Topic: " 
                              << reader_info.topic_name 
                              << ", Key: [" << key.value[0] << "," << key.value[1] << "," << key.value[2] << "," << key.value[3] << "]"
                              << ", Inactive for: " << std::chrono::duration_cast<std::chrono::seconds>(time_since_last_seen).count() 
                              << " seconds" << std::endl;
                }
                ++it;
            }
        }
        
        // 实际删除操作（在锁外执行，避免长时间持锁）
        if (!writers_to_remove.empty() || !readers_to_remove.empty()) {
            std::lock_guard<std::mutex> lock(entity_info_mutex_);
            
            // 移除inactive的DataWriters
            for (const auto& key : writers_to_remove) {
                auto removed_count = active_writers_info_.erase(key);
                if (removed_count > 0) {
                    std::cout << "[PersistenceService] Removed inactive writer with key: [" 
                              << key.value[0] << "," << key.value[1] << "," << key.value[2] << "," << key.value[3] << "]" << std::endl;
                }
            }
            
            // 移除inactive的DataReaders  
            for (const auto& key : readers_to_remove) {
                auto removed_count = active_readers_info_.erase(key);
                if (removed_count > 0) {
                    std::cout << "[PersistenceService] Removed inactive reader with key: [" 
                              << key.value[0] << "," << key.value[1] << "," << key.value[2] << "," << key.value[3] << "]" << std::endl;
                }
            }
        }
        
        // 委托给各个Handler进行专门的清理（如数据库清理等）
        for (auto& [kind, handler] : handlers_) {
            if (handler) {
                // 对于PERSISTENT handler，可以清理数据库中的过期数据
                // 对于TRANSIENT handler，可以清理内存缓存中的过期数据
                // 这里handlers可以各自实现自己的清理逻辑
            }
        }
        
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceService] Exception in cleanup_inactive_entities: " 
                  << e.what() << std::endl;
    }
}

bool PersistenceService::should_send_historical_data(const std::string& topic_name) const {
    if (!smart_config_.enable_smart_sending) {
        return true; // 如果未启用智能发送，总是发送历史数据
    }
    
    if (!database_manager_ || !database_manager_->is_initialized()) {
        return true; // 数据库未就绪时，采用安全策略，发送历史数据
    }
    
    try {
        // 检查当前Topic上是否有活跃的、非持久化服务创建的DataWriter
        std::lock_guard<std::mutex> lock(entity_info_mutex_);
        
       
        constexpr auto ACTIVE_WRITER_TIMEOUT_SECONDS = 10;
        
        for (const auto& [guid, info] : active_writers_info_) {
            // 检查是否是同一个topic
            if (info.topic_name == topic_name) {
                // 检查是否是其他应用程序创建的DataWriter（非持久化服务）
                auto current_time = std::chrono::steady_clock::now();
                auto time_since_last_seen = current_time - info.last_seen;
                
                // 如果Writer在过去10秒内是活跃的，认为它可能仍在发布数据
                if (time_since_last_seen <= std::chrono::seconds(ACTIVE_WRITER_TIMEOUT_SECONDS)) {
                    std::cout << "[PersistenceService] Found active external writer (GUID: " 
                             << std::hex << guid.value[3] << std::dec 
                             << ") for topic '" << topic_name 
                             << "', skipping historical data" << std::endl;
                    return false;
                }
            }
        }
        
        // 如果没有找到活跃的外部DataWriter，则发送历史数据
        std::cout << "[PersistenceService] No active external writers found for topic '" 
                 << topic_name << "', sending historical data" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceService] Exception in should_send_historical_data: " 
                  << e.what() << std::endl;
        return true; // 出错时采用安全策略，发送历史数据
    }
}

// ===== 内存实体信息查询接口实现 =====

std::vector<ActiveDataWriterInfo> PersistenceService::get_active_datawriters_info() const {
    std::lock_guard<std::mutex> lock(entity_info_mutex_);
    std::vector<ActiveDataWriterInfo> result;
    
    for (const auto& pair : active_writers_info_) {
        if (pair.second.is_active) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<ActiveDataReaderInfo> PersistenceService::get_active_datareaders_info() const {
    std::lock_guard<std::mutex> lock(entity_info_mutex_);
    std::vector<ActiveDataReaderInfo> result;
    
    for (const auto& pair : active_readers_info_) {
        if (pair.second.is_active) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<ActiveDataWriterInfo> PersistenceService::get_topic_datawriters_info(const std::string& topic_name) const {
    std::lock_guard<std::mutex> lock(entity_info_mutex_);
    std::vector<ActiveDataWriterInfo> result;
    
    for (const auto& pair : active_writers_info_) {
        if (pair.second.is_active && pair.second.topic_name == topic_name) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<ActiveDataReaderInfo> PersistenceService::get_topic_datareaders_info(const std::string& topic_name) const {
    std::lock_guard<std::mutex> lock(entity_info_mutex_);
    std::vector<ActiveDataReaderInfo> result;
    
    for (const auto& pair : active_readers_info_) {
        if (pair.second.is_active && pair.second.topic_name == topic_name) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

auto PersistenceService::get_db_table_names() -> std::vector<std::string> {
    if (database_manager_) {
        return database_manager_->get_table_names();
    }
    return {};
}

auto PersistenceService::get_db_table_content(const std::string& table_name) -> std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>> {
    if (database_manager_) {
        return database_manager_->get_table_content(table_name);
    }
    return {};
}

auto PersistenceService::get_memory_cache_usage() const -> std::pair<size_t, size_t> {
    if (memory_cache_) {
        return {memory_cache_->memory_usage(), config_.memory_cache_size_mb * 1024 * 1024};
    }
    return {0, 0};
}

TRAVODDS_NAMESPACE_END
