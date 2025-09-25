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

/**
 * @brief 持久化处理器基类
 * 
 * 定义了不同持久化策略（TRANSIENT、PERSISTENT）的通用接口。
 * 每个具体的处理器负责实现特定的存储策略和数据管理逻辑。
 */
class DurabilityHandler {
public:
    /**
     * @brief 构造函数
     * 
     * @param service 持久化服务实例指针
     */
    explicit DurabilityHandler(PersistenceService* service) : service_(service) {}
    
    /**
     * @brief 虚析构函数
     */
    virtual ~DurabilityHandler() = default;
    
    // 原有的数据存储接口
    /**
     * @brief 存储数据
     * 
     * 根据具体的持久化策略存储DDS数据样本。
     * 
     * @param data 要存储的数据
     */
    virtual void store_data(const StoredData& data) = 0;
    
    /**
     * @brief 获取Topic的历史数据
     * 
     * @param topic_name Topic名称
     * @return 历史数据样本向量
     */
    virtual auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData> = 0;
    
    /**
     * @brief 获取指定时间范围的历史数据
     * 
     * @param topic_name Topic名称
     * @param start_time 开始时间
     * @param end_time 结束时间
     * @return 时间范围内的历史数据样本向量
     */
    virtual auto get_historical_data_range(const std::string& topic_name, 
                                         const Time_t& start_time,
                                         const Time_t& end_time) -> std::vector<StoredData> = 0;
    
    /**
     * @brief 清理过期数据
     * 
     * 根据配置的数据保留策略清理过期的历史数据。
     */
    virtual void cleanup_expired_data() = 0;
    
    // 新增：生命周期管理接口
    /**
     * @brief 初始化处理器
     * 
     * @return true 初始化成功，false 初始化失败
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief 关闭处理器
     * 
     * 清理资源，关闭数据连接等。
     */
    virtual void shutdown() = 0;

protected:
    PersistenceService* service_;                                                 ///< 持久化服务实例指针
    
    // 新增：每个Handler管理自己的DDS实体
    std::map<std::string, std::shared_ptr<DataReader>> topic_readers_;           ///< Topic与DataReader的映射
    std::map<std::string, std::shared_ptr<DataWriter>> topic_writers_;           ///< Topic与DataWriter的映射
    mutable std::mutex entities_mutex_;                                          ///< 实体访问互斥锁
    
    // 新增：辅助方法（由具体子类实现）
    /**
     * @brief 向订阅者发送历史数据
     * 
     * 当发现新的DataReader时，根据QoS兼容性发送相应的历史数据。
     * 
     * @param topic_name Topic名称
     * @param reader_key DataReader实体键
     */
    virtual void send_historical_data_to_subscriber(const std::string& topic_name,
                                                   const BuiltinTopicKey_t& reader_key) = 0;
};


/**
 * @brief TRANSIENT策略处理器
 * 
 * 实现TRANSIENT持久化策略，数据存储在内存中，进程间不共享。
 * 适用于需要在当前进程生命周期内保留数据的场景。
 */
class TransientHandler : public DurabilityHandler {
public:
    /**
     * @brief 构造函数
     * 
     * @param service 持久化服务实例指针
     */
    explicit TransientHandler(PersistenceService* service);
    
    // 数据存储接口实现
    /**
     * @brief 存储数据到内存缓存
     * 
     * @param data 要存储的数据
     */
    void store_data(const StoredData& data) override;
    
    /**
     * @brief 从内存缓存获取历史数据
     * 
     * @param topic_name Topic名称
     * @return 历史数据样本向量
     */
    auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData> override;
    
    /**
     * @brief 从内存缓存获取指定时间范围的历史数据
     * 
     * @param topic_name Topic名称
     * @param start_time 开始时间
     * @param end_time 结束时间
     * @return 时间范围内的历史数据样本向量
     */
    auto get_historical_data_range(const std::string& topic_name, 
                                 const Time_t& start_time,
                                 const Time_t& end_time) -> std::vector<StoredData> override;
    
    /**
     * @brief 清理内存中的过期数据
     */
    void cleanup_expired_data() override;
    
    // 生命周期管理接口实现
    /**
     * @brief 初始化TRANSIENT处理器
     * 
     * @return true 初始化成功，false 初始化失败
     */
    auto initialize() -> bool override;
    
    /**
     * @brief 关闭TRANSIENT处理器
     */
    void shutdown() override;

protected:
    // 辅助方法实现
    /**
     * @brief 向订阅者发送内存中的历史数据
     * 
     * @param topic_name Topic名称
     * @param reader_key DataReader实体键
     */
    void send_historical_data_to_subscriber(const std::string& topic_name,
                                           const BuiltinTopicKey_t& reader_key) override;
};

/**
 * @brief PERSISTENT策略处理器
 * 
 * 实现PERSISTENT持久化策略，数据持久化到磁盘数据库。
 * 支持进程重启后的数据恢复，适用于需要长期保存数据的场景。
 */
class PersistentHandler : public DurabilityHandler {
public:
    /**
     * @brief 构造函数
     * 
     * @param service 持久化服务实例指针
     */
    explicit PersistentHandler(PersistenceService* service);
    
    // 数据存储接口实现
    /**
     * @brief 存储数据到数据库
     * 
     * @param data 要存储的数据
     */
    void store_data(const StoredData& data) override;
    
    /**
     * @brief 从数据库获取历史数据
     * 
     * @param topic_name Topic名称
     * @return 历史数据样本向量
     */
    auto get_historical_data(const std::string& topic_name) -> std::vector<StoredData> override;
    
    /**
     * @brief 从数据库获取指定时间范围的历史数据
     * 
     * @param topic_name Topic名称
     * @param start_time 开始时间
     * @param end_time 结束时间
     * @return 时间范围内的历史数据样本向量
     */
    auto get_historical_data_range(const std::string& topic_name, 
                                 const Time_t& start_time,
                                 const Time_t& end_time) -> std::vector<StoredData> override;
    
    /**
     * @brief 清理数据库中的过期数据
     */
    void cleanup_expired_data() override;
    
    // 生命周期管理接口实现
    /**
     * @brief 初始化PERSISTENT处理器
     * 
     * @return true 初始化成功，false 初始化失败
     */
    auto initialize() -> bool override;
    
    /**
     * @brief 关闭PERSISTENT处理器
     */
    void shutdown() override;

protected:
    // 辅助方法实现
    /**
     * @brief 向订阅者发送数据库中的历史数据
     * 
     * @param topic_name Topic名称
     * @param reader_key DataReader实体键
     */
    void send_historical_data_to_subscriber(const std::string& topic_name,
                                           const BuiltinTopicKey_t& reader_key) override;
};

TRAVODDS_NAMESPACE_END