#pragma once

#include "PersistenceTypes.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>

TRAVODDS_NAMESPACE_BEGIN

// 前向声明
class PersistenceService;
class DataReader;
class DataWriter;
struct PublicationBuiltinTopicData;
struct SubscriptionBuiltinTopicData;

// 持久化处理器基类
class DurabilityHandler {
public:
    explicit DurabilityHandler(PersistenceService* service) : service_(service) {}
    virtual ~DurabilityHandler() = default;
    
    // 原有的数据存储接口
    virtual void store_data(const StoredData& data) = 0;
    virtual auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData> = 0;
    virtual auto get_historical_data_range(const std::string& topic_name, 
                                         const Time_t& start_time,
                                         const Time_t& end_time) -> std::vector<StoredData> = 0;
    virtual void cleanup_expired_data() = 0;
    
  
    // 新增：生命周期管理接口
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

protected:
    PersistenceService* service_;
    
    // 新增：每个Handler管理自己的DDS实体
    std::map<std::string, std::shared_ptr<DataReader>> topic_readers_;
    std::map<std::string, std::shared_ptr<DataWriter>> topic_writers_;
    mutable std::mutex entities_mutex_;
    
    // 新增：辅助方法（由具体子类实现）
    virtual void send_historical_data_to_subscriber(const std::string& topic_name,
                                                   const BuiltinTopicKey_t& reader_key) = 0;
};


// TRANSIENT策略处理器 - 内存存储，实例间共享
class TransientHandler : public DurabilityHandler {
public:
    explicit TransientHandler(PersistenceService* service);
    
    // 数据存储接口实现
    void store_data(const StoredData& data) override;
    auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData> override;
    auto get_historical_data_range(const std::string& topic_name, 
                                 const Time_t& start_time,
                                 const Time_t& end_time) -> std::vector<StoredData> override;
    void cleanup_expired_data() override;
    
  
    // 生命周期管理接口实现
    auto initialize() -> bool override;
    void shutdown() override;

protected:
    // 辅助方法实现
    void send_historical_data_to_subscriber(const std::string& topic_name,
                                           const BuiltinTopicKey_t& reader_key) override;
};

// PERSISTENT策略处理器 - 数据库存储，持久化到磁盘
class PersistentHandler : public DurabilityHandler {
public:
    explicit PersistentHandler(PersistenceService* service);
    
    // 数据存储接口实现
    void store_data(const StoredData& data) override;
    auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData> override;
    auto get_historical_data_range(const std::string& topic_name, 
                                 const Time_t& start_time,
                                 const Time_t& end_time) -> std::vector<StoredData> override;
    void cleanup_expired_data() override;
    
     
    // 生命周期管理接口实现
    auto initialize() -> bool override;
    void shutdown() override;

protected:
    // 辅助方法实现
    void send_historical_data_to_subscriber(const std::string& topic_name,
                                           const BuiltinTopicKey_t& reader_key) override;
};

TRAVODDS_NAMESPACE_END