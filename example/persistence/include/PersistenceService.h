#pragma once

#include "dcps/domain/domainparticipant.h"
#include "dcps/subscribe/datareader.h"
#include "dcps/publish/datawriter.h"
#include "type/basictype.h"
#include <memory>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <sqlite3.h>

AGIDDS_NAMESPACE_BEGIN

// 前向声明
class PersistenceService;

/**
 * @brief DDS持久化服务主类
 * 
 * 功能：
 * 1. 监控网络中的持久化DataWriter
 * 2. 自动接收和存储持久化数据
 * 3. 为新的DataReader提供历史数据
 */
class PersistenceService {
public:
    struct StoredData {
        std::string topic_name;
        std::string type_name;
        std::vector<uint8_t> data;
        SampleInfo sample_info;
        Time_t storage_time;
    };

    // 数据监听器类
    class PersistenceDataListener : public DataReaderListener {
    private:
        PersistenceService* service_;
        std::string topic_name_;
    
    public:
        PersistenceDataListener(PersistenceService* service, const std::string& topic_name)
            : service_(service), topic_name_(topic_name) {}
        
        void on_data_available(DataReader* reader) override;
    };

private:
    // DDS实体
    DomainParticipant* participant_;
    std::map<std::string, DataReader*> collectors_;
    std::map<std::string, DataWriter*> publishers_;
    
    // 监听器存储 - 防止监听器被过早销毁
    std::map<std::string, std::shared_ptr<PersistenceDataListener>> listeners_;
    
    // 存储相关
    sqlite3* db_;
    std::string db_path_;
    
    // 线程控制
    std::atomic<bool> running_;
    std::thread discovery_thread_;
    std::thread cleanup_thread_;

public:
    /**
     * @brief 构造函数
     * @param participant DDS域参与者
     * @param db_path 数据库文件路径
     */
    PersistenceService(DomainParticipant* participant, const std::string& db_path);
    
    /**
     * @brief 析构函数
     */
    ~PersistenceService();
    
    /**
     * @brief 启动持久化服务
     */
    ReturnCode_t start();
    
    /**
     * @brief 停止持久化服务
     */
    void stop();
    
    /**
     * @brief 获取网络拓扑信息
     */
    struct NetworkTopology {
        std::vector<ParticipantBuiltinTopicData> participants;
        std::vector<PublicationBuiltinTopicData> writers;
        std::vector<SubscriptionBuiltinTopicData> readers;
    };
    
    NetworkTopology get_network_topology() const;
    
    /**
     * @brief 查询历史数据
     */
    std::vector<StoredData> query_historical_data(
        const std::string& topic_name,
        const Time_t& start_time = Time_t{0, 0},
        const Time_t& end_time = Time_t{0x7FFFFFFF, 0xFFFFFFFF});

private:
    /**
     * @brief 初始化数据库
     */
    bool initialize_database();
    
    /**
     * @brief 发现循环线程
     */
    void discovery_loop();
    
    /**
     * @brief 处理发现的DataWriter
     */
    void handle_discovered_writer(const PublicationBuiltinTopicData& writer_data);
    
    /**
     * @brief 处理新的DataReader
     */
    void handle_new_reader(const SubscriptionBuiltinTopicData& reader_data);
    
    /**
     * @brief 检查QoS是否需要持久化
     */
    bool is_persistent_qos(const PublicationBuiltinTopicData& data) const;
    
    /**
     * @brief 存储数据到数据库
     */
    bool store_data(const std::string& topic_name,
                   const std::string& type_name,
                   const void* data,
                   size_t data_size,
                   const SampleInfo& info);
    
    /**
     * @brief 发送历史数据给新的DataReader
     */
    void send_historical_data(const std::string& topic_name,
                             const SubscriptionBuiltinTopicData& reader_data);
    
    /**
     * @brief 清理过期数据
     */
    void cleanup_expired_data();
    
    /**
     * @brief 数据清理线程
     */
    void cleanup_loop();
};

AGIDDS_NAMESPACE_END
