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
    
    // 清理handlers
    handlers_.clear();
    
    // 清理publisher
    if (publisher_ && participant_) {
        participant_->delete_publisher(publisher_);
        publisher_ = nullptr;
    }
    
    if (database_manager_) {
        database_manager_->shutdown();
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
    auto it = handlers_.find(kind);
    return (it != handlers_.end()) ? it->second.get() : nullptr;
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

void PersistenceService::set_ui_callback(std::shared_ptr<IPersistenceUICallback> callback) {
    ui_callback_ = callback;
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

void PersistenceService::on_publication_discovered(const std::string& topic_name, 
                                                  const std::string& type_name, 
                                                  DurabilityQosPolicyKind durability) {
    // 使用统一的策略检查方法
    if (!should_handle_durability(durability)) {
        std::cout << " - 持久化服务不处理" << durability_kind_to_string(durability) 
                  << "策略，跳过订阅" << std::endl;
        return; 
    }
        
    // TRANSIENT和PERSISTENT策略：持久化服务需要处理
    std::cout << "[PersistenceService] Persistent Publication: " 
              << topic_name << " (" << durability_kind_to_string(durability) << ") Type: " << type_name << std::endl;
    
    // 保存Topic信息 
    {
        std::lock_guard<std::mutex> lock(topic_info_mutex_);
        TopicInfo& info = topic_info_map_[topic_name];
        info.topic_name = topic_name;
        info.type_name = type_name;
        info.durability_kind = durability;
        info.reliability_kind = RELIABLE_RELIABILITY_QOS; // 默认值，后续可从发现数据中获取
        info.discovered_at = std::chrono::steady_clock::now();
        // TODO: 从发现的PublicationBuiltinTopicData中获取完整信息
    }
    
    // 注册topic信息到数据库
    if (database_manager_) {
        database_manager_->insert_or_update_topic(topic_name, type_name, durability);
    }
    
    // 通知UI回调
    if (ui_callback_) {
        ui_callback_->on_writer_discovered(topic_name, type_name);
    }
    
    // 激活相应的持久化处理器
    auto handler_it = handlers_.find(durability);
    if (handler_it != handlers_.end()) {
        std::cout << "[PersistenceService] Activated " 
                  << durability_kind_to_string(durability)
                  << " handler for " << topic_name << std::endl;
    }
    
    // 只为TRANSIENT和PERSISTENT策略创建DataReader来订阅数据
    if (create_data_reader_for_topic(topic_name, type_name, durability)) {
        std::cout << "[PersistenceService] Created DataReader for persistent topic: " << topic_name << std::endl;
    }
    
    // 同时创建DataWriter用于向新订阅者发送历史数据
    if (create_data_writer_for_topic(topic_name, type_name, durability)) {
        std::cout << "[PersistenceService] Created DataWriter for historical data sending: " << topic_name << std::endl;
    }
}

void PersistenceService::on_subscription_discovered(const std::string& topic_name, 
                                                   const std::string& type_name, 
                                                   DurabilityQosPolicyKind durability) {
    // 使用统一的策略检查方法
    if (!should_handle_durability(durability)) {
        std::cout << "[PersistenceService] " << durability_kind_to_string(durability) 
                  << " Subscription detected: " << topic_name 
                  << " - 持久化服务不处理" << durability_kind_to_string(durability) << "策略" << std::endl;
        
        // 对于TRANSIENT_LOCAL，仍然通知UI回调
        if (durability == TRANSIENT_LOCAL_DURABILITY_QOS && ui_callback_) {
            ui_callback_->on_reader_discovered(topic_name, type_name);
        }
        return;
    }
    
    // TRANSIENT和PERSISTENT策略：持久化服务需要处理
    std::cout << "[PersistenceService] Persistent Subscription: " 
              << topic_name << " (" << durability_kind_to_string(durability) << ")" << std::endl;
    
    // 保存Topic信息
    {
        std::lock_guard<std::mutex> lock(topic_info_mutex_);
        if (topic_info_map_.find(topic_name) == topic_info_map_.end()) {
            TopicInfo& info = topic_info_map_[topic_name];
            info.topic_name = topic_name;
            info.type_name = type_name;
            info.durability_kind = durability;
            info.reliability_kind = RELIABLE_RELIABILITY_QOS;
            info.discovered_at = std::chrono::steady_clock::now();
        }
    }
    
    // 确保topic在数据库中注册
    if (database_manager_) {
        database_manager_->insert_or_update_topic(topic_name, type_name, durability);
    }
    
    // 通知UI回调
    if (ui_callback_) {
        ui_callback_->on_reader_discovered(topic_name, type_name);
    }
    
    // 当发现新的Subscription时，需要确保有DataWriter可以发送历史数据
    // 如果还没有为此topic创建DataWriter，现在创建一个
    {
        std::lock_guard<std::mutex> lock(writers_mutex_);
        if (topic_writers_.find(topic_name) == topic_writers_.end()) {
            std::cout << "[PersistenceService] Creating DataWriter for historical data sending: " << topic_name << std::endl;
        }
    }
    
    // 在锁外创建DataWriter以避免死锁
    if (topic_writers_.find(topic_name) == topic_writers_.end()) {
        if (!create_data_writer_for_topic(topic_name, type_name, durability)) {
            std::cerr << "[PersistenceService] Failed to create DataWriter for topic: " << topic_name << std::endl;
            return; 
        }
    }
    
    // 向新订阅者发送历史数据
    send_historical_data_to_subscriber(topic_name, type_name, durability);
}

// 数据订阅相关方法实现
auto PersistenceService::create_data_reader_for_topic(const std::string& topic_name, 
                                                     const std::string& type_name, 
                                                     DurabilityQosPolicyKind durability) -> bool {
    // 使用统一的策略检查方法
    if (!should_handle_durability(durability)) {
        std::cout << "[PersistenceService] 跳过为" << durability_kind_to_string(durability) 
                  << " Topic创建DataReader: " << topic_name << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(readers_mutex_);
    
    // 检查是否已经为此topic创建了reader
    if (topic_readers_.find(topic_name) != topic_readers_.end()) {
        std::cout << "[PersistenceService] DataReader already exists for topic: " << topic_name << std::endl;
        return true;
    }
    
    if (!participant_) {
        std::cout << "[PersistenceService] DomainParticipant not available" << std::endl;
        return false;
    }
    
    try {
        // 创建Subscriber
        auto* subscriber = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        if (subscriber == nullptr) {
            std::cout << "[PersistenceService] Failed to create Subscriber" << std::endl;
            return false;
        }
        
        // 创建Topic
        auto* topic = participant_->create_topic(topic_name, type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        if (topic == nullptr) {
            std::cout << "[PersistenceService] Failed to create Topic: " << topic_name << std::endl;
            return false;
        }
        
        // 设置DataReader QoS
        DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
        reader_qos.durability.kind = durability;
        reader_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        reader_qos.history.kind = KEEP_ALL_HISTORY_QOS;
        
        // 创建DataReader
        auto* reader = subscriber->create_datareader(topic, reader_qos, nullptr, STATUS_MASK_NONE);
        if (reader == nullptr) {
            std::cout << "[PersistenceService] Failed to create DataReader for topic: " << topic_name << std::endl;
            return false;
        }
        
        // 保存reader信息（转换为shared_ptr）
        topic_readers_[topic_name] = std::shared_ptr<DataReader>(reader);
        topic_durability_[topic_name] = durability;
        
        // 记录自己创建的DataReader句柄，避免在发现时将其存入数据库,后续可以替换为持久化服务自己独有的guid或Qos
        {
            std::lock_guard<std::mutex> entities_lock(own_entities_mutex_);
            own_datareader_handles_.insert(reader->get_instance_handle());
            std::cout << "[PersistenceService] Registered own DataReader handle for topic: " << topic_name << std::endl;
        }
        
        std::cout << "[PersistenceService] Successfully created DataReader for topic: " 
                  << topic_name << " with durability: " << static_cast<int>(durability) << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "[PersistenceService] Exception creating DataReader: " << e.what() << std::endl;
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
                stored_data.durability_kind = durability;
                stored_data.is_disposed = (sample_info.instance_state == NOT_ALIVE_DISPOSED_INSTANCE_STATE);
                
                // 设置时间戳
                stored_data.storage_time.sec = sample_info.source_timestamp.sec;
                stored_data.storage_time.nanosec = sample_info.source_timestamp.nanosec;
                
                // 设置序列号
                stored_data.sequence_number = sample_info.write_sequence_number;
                
                // TODO: 使用DDS原生序列化而非手动提取 - 待实现TypeSupport序列化
                // 当前暂时保持兼容性，存储真实数据内容
                std::string sample_data = sample.message;
                stored_data.data.assign(sample_data.begin(), sample_data.end());
                
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

// 历史数据传送相关方法实现
auto PersistenceService::create_data_writer_for_topic(const std::string& topic_name, 
                                                     const std::string& type_name, 
                                                     DurabilityQosPolicyKind durability) -> bool {
    // 使用统一的策略检查方法
    if (!should_handle_durability(durability)) {
        std::cout << "[PersistenceService] 跳过为" << durability_kind_to_string(durability) 
                  << " Topic创建DataWriter: " << topic_name << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(writers_mutex_);
    
    // 检查是否已经为此topic创建了writer
    if (topic_writers_.find(topic_name) != topic_writers_.end()) {
        std::cout << "[PersistenceService] DataWriter already exists for topic: " << topic_name << std::endl;
        return true;
    }
    
    if (!participant_ || !publisher_) {
        std::cout << "[PersistenceService] DomainParticipant or Publisher not available" << std::endl;
        return false;
    }
    
    try {
        // 查找或创建Topic  
        Duration_t timeout = {1, 0}; // 1秒超时
        auto* topic = participant_->find_topic(topic_name, timeout);
        if (topic == nullptr) {
            // Topic不存在，创建新的
            topic = participant_->create_topic(topic_name, type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
            if (topic == nullptr) {
                std::cout << "[PersistenceService] Failed to create Topic for writer: " << topic_name << std::endl;
                return false;
            }
        }
        
        // 设置DataWriter QoS
        DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
        writer_qos.durability.kind = durability;
        writer_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        writer_qos.history.kind = KEEP_ALL_HISTORY_QOS;
        
        // 创建DataWriter
        auto* writer = publisher_->create_datawriter(topic, writer_qos, nullptr, STATUS_MASK_NONE);
        if (writer == nullptr) {
            std::cout << "[PersistenceService] Failed to create DataWriter for topic: " << topic_name << std::endl;
            return false;
        }
        
        // 保存writer信息（转换为shared_ptr）
        topic_writers_[topic_name] = std::shared_ptr<DataWriter>(writer);
        
        //记录自己创建的DataWriter句柄，避免在发现时将其存入数据库
        {
            std::lock_guard<std::mutex> entities_lock(own_entities_mutex_);
            own_datawriter_handles_.insert(writer->get_instance_handle());
            std::cout << "[PersistenceService] Registered own DataWriter handle for topic: " << topic_name << std::endl;
        }
        
        std::cout << "[PersistenceService] Successfully created DataWriter for topic: " 
                  << topic_name << " with durability: " << durability_kind_to_string(durability) << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "[PersistenceService] Exception creating DataWriter: " << e.what() << std::endl;
        return false;
    }
}

void PersistenceService::send_historical_data_to_subscriber(const std::string& topic_name, 
                                                           const std::string& type_name, 
                                                           DurabilityQosPolicyKind durability) {
    // 智能发送检查：如果有活跃的DataWriter，则不发送历史数据
    if (!should_send_historical_data(topic_name)) {
        std::cout << "[PersistenceService] Skipping historical data send for topic '" 
                  << topic_name << "' due to active DataWriter(s)" << std::endl;
        return;
    }
    
    std::cout << "[PersistenceService] Sending historical data to new subscriber for topic: " 
              << topic_name << " (" << durability_kind_to_string(durability) << ")" << std::endl;
    
    // 获取历史数据
    auto historical_data = get_historical_data(topic_name);
    if (historical_data.empty()) {
        std::cout << "[PersistenceService] No historical data found for topic: " << topic_name << std::endl;
        return;
    }
    
    // 查找对应的DataWriter
    std::lock_guard<std::mutex> lock(writers_mutex_);
    auto writer_it = topic_writers_.find(topic_name);
    if (writer_it == topic_writers_.end()) {
        std::cout << "[PersistenceService] No DataWriter found for topic: " << topic_name << std::endl;
        return;
    }
    
    auto writer = writer_it->second;
    if (!writer) {
        std::cout << "[PersistenceService] DataWriter is null for topic: " << topic_name << std::endl;
        return;
    }
    
    // 发送历史数据
    size_t sent_count = 0;
    for (const auto& stored_data : historical_data) {
        try {
            // 将存储的数据反序列化为HelloWorld对象
            HelloWorld sample;
            if (!stored_data.data.empty()) {
                std::string message_str(stored_data.data.begin(), stored_data.data.end());
                sample.message = message_str;
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
              << historical_data.size() << " total." << std::endl;
    
    // 通知UI回调
    if (ui_callback_) {
        ui_callback_->on_historical_data_sent(topic_name, sent_count);
    }
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
            
            on_publication_discovered(pub_data.topic_name, pub_data.type_name, pub_data.durability.kind);
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
            
            on_subscription_discovered(sub_data.topic_name, sub_data.type_name, sub_data.durability.kind);
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
    if (!database_manager_ || !database_manager_->is_initialized()) {
        return;
    }
    
    try {
        // 注册或更新DataWriter
        bool success = database_manager_->register_datawriter(
            pub_data.key,
            pub_data.topic_name,
            pub_data.type_name,
            pub_data.durability.kind,
            pub_data.reliability.kind
        );
        
        if (success) {
            // 更新内存中的活跃列表
            std::lock_guard<std::mutex> lock(entity_monitoring_mutex_);
            active_datawriters_[pub_data.topic_name].insert(pub_data.key);
            
            std::cout << "[PersistenceService] Registered DataWriter for topic '" 
                      << pub_data.topic_name << "'" << std::endl;
        } else {
            std::cerr << "[PersistenceService] Failed to register DataWriter for topic '" 
                      << pub_data.topic_name << "'" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceService] Exception updating DataWriter registry: " 
                  << e.what() << std::endl;
    }
}

void PersistenceService::update_datareader_registry(const SubscriptionBuiltinTopicData& sub_data) {
    if (!database_manager_ || !database_manager_->is_initialized()) {
        return;
    }
    
    try {
        // 注册或更新DataReader
        bool success = database_manager_->register_datareader(
            sub_data.key,
            sub_data.topic_name,
            sub_data.type_name,
            sub_data.durability.kind,
            sub_data.reliability.kind
        );
        
        if (success) {
            // 更新内存中的活跃列表
            std::lock_guard<std::mutex> lock(entity_monitoring_mutex_);
            active_datareaders_[sub_data.topic_name].insert(sub_data.key);
            
            std::cout << "[PersistenceService] Registered DataReader for topic '" 
                      << sub_data.topic_name << "'" << std::endl;
            
            // 检查是否需要发送历史数据
            if (should_send_historical_data(sub_data.topic_name)) {
                std::cout << "[PersistenceService] Triggering historical data send for topic '" 
                          << sub_data.topic_name << "'" << std::endl;
                // 这里可以异步触发历史数据发送
                // 注意：需要确保原有的历史数据发送逻辑会调用should_send_historical_data进行检查
            }
        } else {
            std::cerr << "[PersistenceService] Failed to register DataReader for topic '" 
                      << sub_data.topic_name << "'" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceService] Exception updating DataReader registry: " 
                  << e.what() << std::endl;
    }
}

void PersistenceService::cleanup_inactive_entities() {
    if (!database_manager_ || !database_manager_->is_initialized()) {
        return;
    }
    
    try {
        // 使用配置的超时时间清理数据库中的不活跃实体
        auto timeout_minutes = std::chrono::duration_cast<std::chrono::minutes>(smart_config_.entity_timeout);
        bool success = database_manager_->cleanup_inactive_entities(timeout_minutes);
        
        if (success) {
            // 同步更新内存中的活跃列表
            std::lock_guard<std::mutex> lock(entity_monitoring_mutex_);
            
            for (auto& topic_writers : active_datawriters_) {
                const std::string& topic_name = topic_writers.first;
                auto active_writers = database_manager_->get_active_datawriters(topic_name);
                
                // 重建活跃Writer集合
                std::set<BuiltinTopicKey_t> new_active_set;
                for (const auto& key : active_writers) {
                    new_active_set.insert(key);
                }
                topic_writers.second = std::move(new_active_set);
            }
            
            for (auto& topic_readers : active_datareaders_) {
                const std::string& topic_name = topic_readers.first;
                auto active_readers = database_manager_->get_active_datareaders(topic_name);
                
                // 重建活跃Reader集合
                std::set<BuiltinTopicKey_t> new_active_set;
                for (const auto& key : active_readers) {
                    new_active_set.insert(key);
                }
                topic_readers.second = std::move(new_active_set);
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
        // 直接查询数据库中的活跃DataWriter，避免使用非const成员
        auto active_writers = database_manager_->get_active_datawriters(topic_name);
        
        if (active_writers.empty()) {
            // 没有活跃的DataWriter，可以发送历史数据
            return true;
        }
        
        // 有活跃的DataWriter，不发送历史数据，避免重复
        std::cout << "[PersistenceService] Topic '" << topic_name 
                  << "' has " << active_writers.size() 
                  << " active DataWriter(s), skipping historical data send" << std::endl;
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceService] Exception in should_send_historical_data: " 
                  << e.what() << std::endl;
        return true; // 出错时采用安全策略，发送历史数据
    }
}

TRAVODDS_NAMESPACE_END
