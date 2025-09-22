#include "DurabilityHandlers.h"
#include "PersistenceService.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/subscribe/subscriber.h"
#include "dcps/subscribe/datareader.h"
#include "dcps/publish/publisher.h"
#include "dcps/publish/datawriter.h"
#include "type/builtintopicdata.h"
#include "../../cpp/hello_world/HelloWorldTypeSupport.h"
#include <iostream>

TRAVODDS_NAMESPACE_BEGIN


// ===== TransientHandler 实现 =====
TransientHandler::TransientHandler(PersistenceService* service)
    : DurabilityHandler(service) {}

void TransientHandler::store_data(const StoredData& data) {
    std::cout << "[TransientHandler] 存储数据到服务内存缓存 for topic: " << data.topic_name << std::endl;
    
    // TRANSIENT策略：仅存储在内存缓存中，不写入数据库
    auto& memory_cache = service_->get_memory_cache();
    std::string cache_key = data.topic_name + "_" + data.instance_id + "_" + std::to_string(data.sequence_number);
    memory_cache.put(cache_key, data);
}

auto TransientHandler::get_historical_data(const std::string& topic_name) -> std::vector<StoredData> {
    std::cout << "[TransientHandler] 从服务内存缓存获取历史数据 for topic: " << topic_name << std::endl;
    
    // TRANSIENT策略：从内存缓存获取数据
    auto& memory_cache = service_->get_memory_cache();
    return memory_cache.get(topic_name);
}

auto TransientHandler::get_historical_data_range(const std::string& topic_name, 
                                               const Time_t& start_time,
                                               const Time_t& end_time) -> std::vector<StoredData> {
    std::cout << "[TransientHandler] 从服务内存缓存获取历史数据范围 for topic: " << topic_name << std::endl;
    
    // TRANSIENT策略：从内存缓存获取指定时间范围的数据
    auto& memory_cache = service_->get_memory_cache();
    return memory_cache.get_range(topic_name, start_time, end_time);
}

void TransientHandler::cleanup_expired_data() {
    // TRANSIENT策略：清理过期的内存缓存数据
    auto& memory_cache = service_->get_memory_cache();
    constexpr auto CACHE_EXPIRE_HOURS = 24;
    auto expire_duration = std::chrono::hours(CACHE_EXPIRE_HOURS);
    memory_cache.clear_expired(expire_duration);
}

// ===== TransientHandler 新增方法实现 =====
auto TransientHandler::initialize() -> bool {
    std::cout << "[TransientHandler] Initializing TRANSIENT handler..." << std::endl;
    return true;
}

void TransientHandler::shutdown() {
    std::cout << "[TransientHandler] Shutting down TRANSIENT handler..." << std::endl;
    
    std::lock_guard<std::mutex> lock(entities_mutex_);
    topic_readers_.clear();
    topic_writers_.clear();
}



void TransientHandler::send_historical_data_to_subscriber(const std::string& topic_name,
                                                         const BuiltinTopicKey_t& reader_key) {
    std::cout << "[TransientHandler] Sending historical data for topic: " << topic_name << std::endl;
    
    // 委托给PersistenceService来发送历史数据，因为它管理着实际的DataWriter
    service_->send_historical_data_for_handler(topic_name, "HelloWorld", TRANSIENT_DURABILITY_QOS, reader_key);
}

// ===== PersistentHandler 实现 =====
PersistentHandler::PersistentHandler(PersistenceService* service)
    : DurabilityHandler(service) {}

void PersistentHandler::store_data(const StoredData& data) {
    std::cout << "[PersistentHandler] 持久化存储数据 for topic: " << data.topic_name << std::endl;
    
    // PERSISTENT策略：存储到数据库，同时更新内存缓存
    auto& db_manager = service_->get_database_manager();
    if (!db_manager.store_data(data)) {
        std::cerr << "[PersistentHandler] 数据库存储失败 for topic: " << data.topic_name << std::endl;
        return;
    }
    
    // 同时更新内存缓存以提高访问性能 (Write-Through缓存策略)
    auto& memory_cache = service_->get_memory_cache();
    std::string cache_key = data.topic_name + "_" + data.instance_id + "_" + std::to_string(data.sequence_number);
    memory_cache.put(cache_key, data);
}

auto PersistentHandler::get_historical_data(const std::string& topic_name) -> std::vector<StoredData> {
    std::cout << "[PersistentHandler] 从持久化数据库获取历史数据 for topic: " << topic_name << std::endl;
    
    // PERSISTENT策略：主要从数据库获取，同时更新缓存
    auto& db_manager = service_->get_database_manager();
    auto persistent_data = db_manager.load_data(topic_name);
    
    // 更新缓存 (Write-Through缓存策略)
    if (!persistent_data.empty()) {
        auto& memory_cache = service_->get_memory_cache();
        for (const auto& data : persistent_data) {
            std::string cache_key = data.topic_name + "_" + data.instance_id + "_" + std::to_string(data.sequence_number);
            memory_cache.put(cache_key, data);
        }
    }
    
    return persistent_data;
}

auto PersistentHandler::get_historical_data_range(const std::string& topic_name, 
                                                const Time_t& start_time,
                                                const Time_t& end_time) -> std::vector<StoredData> {
    std::cout << "[PersistentHandler] 从持久化数据库获取历史数据范围 for topic: " << topic_name << std::endl;
    
    // PERSISTENT策略：从数据库获取指定时间范围的数据
    auto& db_manager = service_->get_database_manager();
    return db_manager.load_data_range(topic_name, start_time, end_time);
}

void PersistentHandler::cleanup_expired_data() {
    // PERSISTENT策略：可以选择不清理数据，或根据配置清理很老的数据
    //std::cout << "[PersistentHandler] 持久化数据保留，不执行过期清理" << std::endl;
}

// ===== PersistentHandler 新增方法实现 =====
auto PersistentHandler::initialize() -> bool {
    std::cout << "[PersistentHandler] Initializing PERSISTENT handler..." << std::endl;
    return true;
}

void PersistentHandler::shutdown() {
    std::cout << "[PersistentHandler] Shutting down PERSISTENT handler..." << std::endl;
    
    std::lock_guard<std::mutex> lock(entities_mutex_);
    topic_readers_.clear();
    topic_writers_.clear();
}



void PersistentHandler::send_historical_data_to_subscriber(const std::string& topic_name,
                                                          const BuiltinTopicKey_t& reader_key) {
    std::cout << "[PersistentHandler] Sending historical data for topic: " << topic_name << std::endl;
    
    // 委托给PersistenceService来发送历史数据，因为它管理着实际的DataWriter
    std::cout << "[PersistentHandler] Delegating to PersistenceService for historical data send..." << std::endl;
    service_->send_historical_data_for_handler(topic_name, "HelloWorld", PERSISTENT_DURABILITY_QOS, reader_key);
}

TRAVODDS_NAMESPACE_END