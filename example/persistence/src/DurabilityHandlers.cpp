#include "DurabilityHandlers.h"
#include "PersistenceService.h"
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
    auto expire_duration = std::chrono::hours(24);
    memory_cache.clear_expired(expire_duration);
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
    
    // 同时更新内存缓存以提高访问性能
    // auto& memory_cache = service_->get_memory_cache();
    // std::string cache_key = data.topic_name + "_" + data.instance_id + "_" + std::to_string(data.sequence_number);
    // memory_cache.put(cache_key, data);
}

auto PersistentHandler::get_historical_data(const std::string& topic_name) -> std::vector<StoredData> {
    std::cout << "[PersistentHandler] 从持久化数据库获取历史数据 for topic: " << topic_name << std::endl;
    
    // PERSISTENT策略：主要从数据库获取，同时更新缓存
    auto& db_manager = service_->get_database_manager();
    auto persistent_data = db_manager.load_data(topic_name);
    
    // 更新缓存
    // if (!persistent_data.empty()) {
    //     auto& memory_cache = service_->get_memory_cache();
    //     for (const auto& data : persistent_data) {
    //         std::string cache_key = data.topic_name + "_" + data.instance_id + "_" + std::to_string(data.sequence_number);
    //         memory_cache.put(cache_key, data);
    //     }
    // }
    
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

TRAVODDS_NAMESPACE_END