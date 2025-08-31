#include "PersistenceService.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/subscribe/subscriber.h"
#include "dcps/subscribe/datareader.h"
#include "type/builtintopicdata.h"
#include "../../cpp/hello_world/HelloWorldTypeSupport.h"
#include <iostream>

AGIDDS_NAMESPACE_BEGIN

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
    
    std::cout << "[PersistenceService] Shutting down..." << std::endl;
    
    // 停止工作线程
    should_stop_ = true;
    cv_.notify_all();
    
    // 停止发现线程
    discovery_running_ = false;
    if (discovery_thread_.joinable()) {
        discovery_thread_.join();
    }
    
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

void PersistenceService::cleanup_resources() {
    handlers_.clear();
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

// 数据库操作的便利方法（供处理器使用）
bool PersistenceService::store_data_to_database(const StoredData& data) {
    return database_manager_ ? database_manager_->store_data(data) : false;
}

auto PersistenceService::load_data_from_database(const std::string& topic_name) -> std::vector<StoredData> {
    return database_manager_ ? database_manager_->load_data(topic_name) : std::vector<StoredData>{};
}

auto PersistenceService::load_data_from_database_range(const std::string& topic_name,
                                                     const Time_t& start_time,
                                                     const Time_t& end_time) -> std::vector<StoredData> {
    return database_manager_ ? 
           database_manager_->load_data_range(topic_name, start_time, end_time) : 
           std::vector<StoredData>{};
}

bool PersistenceService::insert_or_update_topic_info(const std::string& topic_name,
                                                    const std::string& type_name,
                                                    DurabilityQosPolicyKind durability_kind) {
    return database_manager_ ? 
           database_manager_->insert_or_update_topic(topic_name, type_name, durability_kind) : 
           false;
}

void PersistenceService::cleanup_old_data() {
    if (database_manager_) {
        database_manager_->cleanup_old_data(config_.data_retention);
    }
}

void PersistenceService::set_domain_participant(std::shared_ptr<DomainParticipant> participant) {
    participant_ = participant;
    
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
    
    try {
        // 创建用于发现的订阅者
        discovery_subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        if (!discovery_subscriber_) {
            std::cerr << "[PersistenceService] Failed to create discovery subscriber" << std::endl;
            return;
        }
        
        // 1. 订阅DCPSPublication内置主题来发现DataWriter
        auto* pub_topic = participant_->create_topic(
            "DCPSPublication", 
            "DDS::PublicationBuiltinTopicData", 
            TOPIC_QOS_DEFAULT, 
            nullptr, 
            STATUS_MASK_NONE
        );
        
        if (pub_topic) {
            DataReaderQos pub_reader_qos = DATAREADER_QOS_DEFAULT;
            pub_reader_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
            pub_reader_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
            
            auto* pub_reader = discovery_subscriber_->create_datareader(
                pub_topic, pub_reader_qos, nullptr, STATUS_MASK_NONE
            );
            
            if (pub_reader) {
                publication_reader_ = std::shared_ptr<DataReader>(pub_reader);
                std::cout << "[PersistenceService] Successfully created DCPSPublication reader" << std::endl;
            } else {
                std::cerr << "[PersistenceService] Failed to create DCPSPublication reader" << std::endl;
            }
        }
        
        // 2. 订阅DCPSSubscription内置主题来发现DataReader
        auto* sub_topic = participant_->create_topic(
            "DCPSSubscription", 
            "DDS::SubscriptionBuiltinTopicData", 
            TOPIC_QOS_DEFAULT, 
            nullptr, 
            STATUS_MASK_NONE
        );
        
        if (sub_topic) {
            DataReaderQos sub_reader_qos = DATAREADER_QOS_DEFAULT;
            sub_reader_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
            sub_reader_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
            
            auto* sub_reader = discovery_subscriber_->create_datareader(
                sub_topic, sub_reader_qos, nullptr, STATUS_MASK_NONE
            );
            
            if (sub_reader) {
                subscription_reader_ = std::shared_ptr<DataReader>(sub_reader);
                std::cout << "[PersistenceService] Successfully created DCPSSubscription reader" << std::endl;
            } else {
                std::cerr << "[PersistenceService] Failed to create DCPSSubscription reader" << std::endl;
            }
        }
        
        std::cout << "[PersistenceService] Discovery setup completed, starting discovery thread..." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceService] Exception in setup_discovery_listeners: " << e.what() << std::endl;
    }
    
    // 启动发现监听线程
    discovery_running_ = true;
    discovery_thread_ = std::thread(&PersistenceService::discovery_thread_main, this);
    
    std::cout << "[PersistenceService] Discovery monitoring started (using automatic discovery)" << std::endl;
}

void PersistenceService::discovery_thread_main() {
    std::cout << "[PersistenceService] Discovery thread started" << std::endl;
    
    while (discovery_running_ && !should_stop_) {
        try {
            // 检查新的发布者和订阅者
            check_publication_data();
            check_subscription_data();
            
            // 等待一段时间再检查
            std::this_thread::sleep_for(std::chrono::seconds(2));
        } catch (const std::exception& e) {
            std::cerr << "[PersistenceService] Discovery thread error: " << e.what() << std::endl;
        }
    }
    
    std::cout << "[PersistenceService] Discovery thread stopped" << std::endl;
}

void PersistenceService::check_publication_data() {
    if (!participant_) return;
    
    // 简化的发现检查 - 目前通过外部通知实现
    // 未来可以通过内置主题DataReader实现真正的自动发现
    // 这里只是保持发现线程运行状态
}

void PersistenceService::check_subscription_data() {
    if (!participant_) return;
    
    // 简化的发现检查 - 目前通过外部通知实现
    // 未来可以通过内置主题DataReader实现真正的自动发现
    // 这里只是保持发现线程运行状态
}

void PersistenceService::on_publication_discovered(const std::string& topic_name, 
                                                  const std::string& type_name, 
                                                  DurabilityQosPolicyKind durability) {
    // VOLATILE策略：持久化服务完全不处理
    // TRANSIENT_LOCAL策略：持久化服务不订阅，由DataWriter本地管理
    if (durability == VOLATILE_DURABILITY_QOS||durability == TRANSIENT_LOCAL_DURABILITY_QOS) {
        std::cout << " - 持久化服务不处理VOLATILE和TRANSIENT_LOCAL策略，跳过订阅" << std::endl;
        return; 
    }
        
    
    // TRANSIENT和PERSISTENT策略：持久化服务需要处理
    std::cout << "[PersistenceService] Persistent Publication: " 
              << topic_name << " (" << static_cast<int>(durability) << ")" << std::endl;
    
    // 注册topic信息到数据库
    insert_or_update_topic_info(topic_name, type_name, durability);
    
    // 通知UI回调
    if (ui_callback_) {
        ui_callback_->on_writer_discovered(topic_name, type_name);
    }
    
    // 激活相应的持久化处理器
    auto handler_it = handlers_.find(durability);
    if (handler_it != handlers_.end()) {
        std::cout << "[PersistenceService] Activated " 
                  << (durability == TRANSIENT_DURABILITY_QOS ? "TRANSIENT" : "PERSISTENT")
                  << " handler for " << topic_name << std::endl;
    }
    
    // 只为TRANSIENT和PERSISTENT策略创建DataReader来订阅数据
    if (create_data_reader_for_topic(topic_name, type_name, durability)) {
        std::cout << "[PersistenceService] Created DataReader for persistent topic: " << topic_name << std::endl;
    }
}

void PersistenceService::on_subscription_discovered(const std::string& topic_name, 
                                                   const std::string& type_name, 
                                                   DurabilityQosPolicyKind durability) {
    // VOLATILE策略：持久化服务完全不处理
    if (durability == VOLATILE_DURABILITY_QOS) {
        std::cout << "[PersistenceService] VOLATILE Subscription detected: " << topic_name 
                  << " - 持久化服务不处理VOLATILE策略" << std::endl;
        return; // 不处理VOLATILE订阅
    }
    
    // TRANSIENT_LOCAL策略：持久化服务不处理，由DataWriter本地管理
    if (durability == TRANSIENT_LOCAL_DURABILITY_QOS) {
        std::cout << "[PersistenceService] TRANSIENT_LOCAL Subscription detected: " << topic_name 
                  << " - 历史数据由DataWriter直接提供" << std::endl;
        
        // 通知UI回调
        if (ui_callback_) {
            ui_callback_->on_reader_discovered(topic_name, type_name);
        }
        return; // 不处理TRANSIENT_LOCAL订阅
    }
    
    // TRANSIENT和PERSISTENT策略：持久化服务需要处理
    std::cout << "[PersistenceService] Persistent Subscription: " 
              << topic_name << " (" << static_cast<int>(durability) << ")" << std::endl;
    
    // 通知UI回调
    if (ui_callback_) {
        ui_callback_->on_reader_discovered(topic_name, type_name);
    }
}

void PersistenceService::notify_writer_created(const std::string& topic_name, 
                                              const std::string& type_name, 
                                              DurabilityQosPolicyKind durability) {
    // 记录所有策略的发现，但区分处理方式
    std::cout << "[PersistenceService] Writer detected: Topic=" << topic_name 
              << ", Durability=" << static_cast<int>(durability);
    
    if (durability == VOLATILE_DURABILITY_QOS) {
        std::cout << " (VOLATILE - 持久化服务不处理)" << std::endl;
    } else if (durability == TRANSIENT_LOCAL_DURABILITY_QOS) {
        std::cout << " (TRANSIENT_LOCAL - DataWriter本地管理)" << std::endl;
    } else {
        std::cout << " (持久化服务处理)" << std::endl;
    }
              
    on_publication_discovered(topic_name, type_name, durability);
}

void PersistenceService::notify_reader_created(const std::string& topic_name, 
                                              const std::string& type_name, 
                                              DurabilityQosPolicyKind durability) {
    // 记录所有策略的发现，但区分处理方式
    std::cout << "[PersistenceService] Reader detected: Topic=" << topic_name 
              << ", Durability=" << static_cast<int>(durability);
    
    if (durability == VOLATILE_DURABILITY_QOS) {
        std::cout << " (VOLATILE - 持久化服务不处理)" << std::endl;
    } else if (durability == TRANSIENT_LOCAL_DURABILITY_QOS) {
        std::cout << " (TRANSIENT_LOCAL - DataWriter本地管理)" << std::endl;
    } else {
        std::cout << " (持久化服务处理)" << std::endl;
    }
              
    on_subscription_discovered(topic_name, type_name, durability);
}

// 数据订阅相关方法实现
bool PersistenceService::create_data_reader_for_topic(const std::string& topic_name, 
                                                     const std::string& type_name, 
                                                     DurabilityQosPolicyKind durability) {
    // 只为TRANSIENT和PERSISTENT策略创建DataReader
    if (durability == VOLATILE_DURABILITY_QOS) {
        std::cout << "[PersistenceService] 跳过为VOLATILE Topic创建DataReader: " << topic_name << std::endl;
        return false; // 不创建DataReader
    }
    
    if (durability == TRANSIENT_LOCAL_DURABILITY_QOS) {
        std::cout << "[PersistenceService] 跳过为TRANSIENT_LOCAL Topic创建DataReader: " << topic_name << std::endl;
        return false; // 不创建DataReader，由DataWriter本地管理
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
        
        std::cout << "[PersistenceService] Successfully created DataReader for topic: " 
                  << topic_name << " with durability: " << static_cast<int>(durability) << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "[PersistenceService] Exception creating DataReader: " << e.what() << std::endl;
        return false;
    }
}

void PersistenceService::on_data_available(const std::string& topic_name, 
                                          const std::shared_ptr<DataReader>& reader) {
    if (!reader || !is_running_) {
        return;
    }
    
    try {
        // 这里需要根据实际的DDS API来实现数据读取
        // 由于HelloWorld类型的具体实现可能不同，这里提供一个通用框架
        
        // 获取topic的durability类型
        auto durability_it = topic_durability_.find(topic_name);
        if (durability_it == topic_durability_.end()) {
            std::cout << "[PersistenceService] Unknown durability for topic: " << topic_name << std::endl;
            return;
        }
        
        DurabilityQosPolicyKind durability = durability_it->second;
        
        // 创建StoredData对象
        StoredData stored_data;
        stored_data.topic_name = topic_name;
        stored_data.type_name = "HelloWorld";  // 这里硬编码，实际应该从topic获取
        stored_data.instance_id = "default_instance";
        stored_data.durability_kind = durability;
        stored_data.is_disposed = false;
        
        // 设置时间戳
        Time_t::now(stored_data.storage_time);
        
        // 设置序列号（简单递增）
        static std::atomic<uint64_t> sequence_counter{0};
        stored_data.sequence_number = ++sequence_counter;
        
        // 模拟数据内容（实际应该从reader读取）
        std::string sample_data = "HelloWorld sample data";
        stored_data.data.assign(sample_data.begin(), sample_data.end());
        
        // 设置sample info
        stored_data.sample_info.sample_state = READ_SAMPLE_STATE;
        stored_data.sample_info.view_state = NEW_VIEW_STATE;
        stored_data.sample_info.instance_state = ALIVE_INSTANCE_STATE;
        
        std::cout << "[PersistenceService] Received data for topic: " << topic_name 
                  << ", size: " << stored_data.data.size() << " bytes" << std::endl;
        
        // 存储数据
        store_data(stored_data);
        
    } catch (const std::exception& e) {
        std::cout << "[PersistenceService] Exception processing data: " << e.what() << std::endl;
    }
}

void PersistenceService::poll_data_readers() {
    std::lock_guard<std::mutex> lock(readers_mutex_);
    
    for (const auto& [topic_name, reader] : topic_readers_) {
        if (!reader || !is_running_) {
            continue;
        }
        
        try {
            // 实现真正的数据读取
            HelloWorld sample;
            SampleInfo sample_info;
            
            // 使用 read_next_sample 读取数据
            ReturnCode_t result = reader->read_next_sample(&sample, sample_info);
            
            if (result == RETCODE_OK && sample_info.valid_data) {
                // 获取topic的durability类型
                auto durability_it = topic_durability_.find(topic_name);
                if (durability_it == topic_durability_.end()) {
                    std::cout << "[PersistenceService] Warning: No durability info for topic: " << topic_name << std::endl;
                    continue;
                }
                
                DurabilityQosPolicyKind durability = durability_it->second;
                
                // 创建StoredData对象
                StoredData stored_data;
                stored_data.topic_name = topic_name;
                stored_data.type_name = "HelloWorld";
                stored_data.instance_id = "instance_" + std::to_string(reinterpret_cast<uintptr_t>(&sample_info.instance_handle));
                stored_data.durability_kind = durability;
                stored_data.is_disposed = (sample_info.instance_state == NOT_ALIVE_DISPOSED_INSTANCE_STATE);
                
                // 设置时间戳
                stored_data.storage_time.sec = sample_info.source_timestamp.sec;
                stored_data.storage_time.nanosec = sample_info.source_timestamp.nanosec;
                
                // 设置序列号（使用sample_rank作为序列号）
                stored_data.sequence_number = static_cast<uint64_t>(sample_info.sample_rank);
                
                // 存储真实数据内容
                std::string sample_data = sample.message;
                stored_data.data.assign(sample_data.begin(), sample_data.end());
                
                // 设置sample info
                stored_data.sample_info.sample_state = sample_info.sample_state;
                stored_data.sample_info.view_state = sample_info.view_state;
                stored_data.sample_info.instance_state = sample_info.instance_state;
                
                std::cout << "[PersistenceService] Received REAL data for topic: " << topic_name 
                          << ", message: '" << sample.message << "', durability: " 
                          << static_cast<int>(durability) << ", sequence: " << stored_data.sequence_number << std::endl;
                
                // 存储数据
                store_data(stored_data);
                
            } else if (result != RETCODE_NO_DATA) {
                // 如果返回码不是 NO_DATA，打印错误信息
                std::cout << "[PersistenceService] Error reading from topic " << topic_name 
                          << ", return code: " << static_cast<int>(result) << std::endl;
            }
            // 如果是 RETCODE_NO_DATA，这是正常的，表示没有新数据
            
        } catch (const std::exception& e) {
            std::cout << "[PersistenceService] Exception polling reader for topic " 
                      << topic_name << ": " << e.what() << std::endl;
        }
    }
}

AGIDDS_NAMESPACE_END
