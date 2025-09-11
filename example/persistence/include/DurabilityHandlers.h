#pragma once

#include "PersistenceTypes.h"
#include <memory>
#include <string>
#include <vector>

TRAVODDS_NAMESPACE_BEGIN

// 前向声明
class PersistenceService;

// 持久化处理器基类
class DurabilityHandler {
public:
    explicit DurabilityHandler(PersistenceService* service) : service_(service) {}
    virtual ~DurabilityHandler() = default;
    
    virtual void store_data(const StoredData& data) = 0;
    virtual auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData> = 0;
    virtual auto get_historical_data_range(const std::string& topic_name, 
                                         const Time_t& start_time,
                                         const Time_t& end_time) -> std::vector<StoredData> = 0;
    virtual void cleanup_expired_data() = 0;

protected:
    PersistenceService* service_;
};


// TRANSIENT策略处理器 - 数据库存储，实例间共享
class TransientHandler : public DurabilityHandler {
public:
    explicit TransientHandler(PersistenceService* service);
    
    void store_data(const StoredData& data) override;
    auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData> override;
    auto get_historical_data_range(const std::string& topic_name, 
                                 const Time_t& start_time,
                                 const Time_t& end_time) -> std::vector<StoredData> override;
    void cleanup_expired_data() override;
};

// PERSISTENT策略处理器 - 数据库存储，持久化到磁盘
class PersistentHandler : public DurabilityHandler {
public:
    explicit PersistentHandler(PersistenceService* service);
    
    void store_data(const StoredData& data) override;
    auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData> override;
    auto get_historical_data_range(const std::string& topic_name, 
                                 const Time_t& start_time,
                                 const Time_t& end_time) -> std::vector<StoredData> override;
    void cleanup_expired_data() override;
};

TRAVODDS_NAMESPACE_END