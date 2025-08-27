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
#include <future>
#include <condition_variable>
#include <queue>
#include <algorithm>
#include <sqlite3.h>

AGIDDS_NAMESPACE_BEGIN

// Qt 可视化回调接口（便于后续Qt集成）
class IPersistenceUICallback {
public:
    virtual ~IPersistenceUICallback() = default;
    virtual void on_writer_discovered(const std::string& topic, const std::string& type) = 0;
    virtual void on_reader_discovered(const std::string& topic, const std::string& type) = 0;
    virtual void on_data_stored(const std::string& topic, size_t data_size) = 0;
    virtual void on_historical_data_sent(const std::string& topic, size_t count) = 0;
    virtual void on_statistics_updated() = 0;
};

// 持久化配置
struct PersistenceConfig {
    std::string db_path = "persistence.db";
    std::chrono::seconds cleanup_interval{300};  // 5分钟清理一次
    std::chrono::hours data_retention{24};       // 24小时数据保留
    size_t max_samples_per_topic = 10000;        // 每个Topic最大样本数
    size_t max_db_size_mb = 1024;                // 最大数据库大小1GB
    bool enable_compression = false;              // 是否启用数据压缩
    bool enable_statistics = true;               // 是否启用统计信息
    int worker_threads = 2;                      // 工作线程数量
};

// 持久化统计信息
struct PersistenceStatistics {
    std::atomic<uint64_t> total_samples_stored{0};
    std::atomic<uint64_t> total_samples_served{0};
    std::atomic<uint64_t> total_bytes_stored{0};
    std::atomic<uint64_t> total_bytes_served{0};
    std::atomic<uint32_t> active_writers{0};
    std::atomic<uint32_t> active_readers{0};
    std::atomic<uint32_t> active_topics{0};
    std::chrono::steady_clock::time_point start_time;
    
    // Topic级别统计
    std::map<std::string, uint64_t> samples_per_topic;
    std::map<std::string, uint64_t> bytes_per_topic;
    mutable std::mutex topic_stats_mutex;
    
    PersistenceStatistics() : start_time(std::chrono::steady_clock::now()) {}
    
    // 拷贝构造函数
    PersistenceStatistics(const PersistenceStatistics& other) 
        : total_samples_stored{other.total_samples_stored.load()}
        , total_samples_served{other.total_samples_served.load()}
        , total_bytes_stored{other.total_bytes_stored.load()}
        , total_bytes_served{other.total_bytes_served.load()}
        , active_writers{other.active_writers.load()}
        , active_readers{other.active_readers.load()}
        , active_topics{other.active_topics.load()}
        , start_time{other.start_time}
    {
        std::lock_guard<std::mutex> lock(other.topic_stats_mutex);
        samples_per_topic = other.samples_per_topic;
        bytes_per_topic = other.bytes_per_topic;
    }
    
    // 赋值操作符
    PersistenceStatistics& operator=(const PersistenceStatistics& other) {
        if (this != &other) {
            total_samples_stored = other.total_samples_stored.load();
            total_samples_served = other.total_samples_served.load();
            total_bytes_stored = other.total_bytes_stored.load();
            total_bytes_served = other.total_bytes_served.load();
            active_writers = other.active_writers.load();
            active_readers = other.active_readers.load();
            active_topics = other.active_topics.load();
            start_time = other.start_time;
            
            std::lock_guard<std::mutex> lock1(topic_stats_mutex);
            std::lock_guard<std::mutex> lock2(other.topic_stats_mutex);
            samples_per_topic = other.samples_per_topic;
            bytes_per_topic = other.bytes_per_topic;
        }
        return *this;
    }
    
    double get_uptime_seconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(now - start_time).count();
    }
};

// 前向声明
class PersistenceService;

// 存储的数据项结构
struct StoredData {
    std::string topic_name;
    std::string type_name;
    std::string instance_id;      // 实例标识
    std::vector<uint8_t> data;
    SampleInfo sample_info;
    Time_t storage_time;
    uint64_t sequence_number;     // 序列号，便于排序
    bool is_disposed = false;     // 是否被dispose
    
    // 为Qt显示提供的便利方法
    std::string to_string() const {
        return "Topic: " + topic_name + ", Type: " + type_name + 
               ", Size: " + std::to_string(data.size()) + " bytes";
    }
};

// 网络拓扑信息
struct NetworkTopology {
    std::vector<ParticipantBuiltinTopicData> participants;
    std::vector<PublicationBuiltinTopicData> writers;
    std::vector<SubscriptionBuiltinTopicData> readers;
    
    // 统计信息
    size_t total_participants() const { return participants.size(); }
    size_t total_writers() const { return writers.size(); }
    size_t total_readers() const { return readers.size(); }
    
    // 获取所有活跃的主题
    std::vector<std::string> get_active_topics() const {
        std::vector<std::string> topics;
        for (const auto& writer : writers) {
            topics.push_back(writer.topic_name);
        }
        std::sort(topics.begin(), topics.end());
        topics.erase(std::unique(topics.begin(), topics.end()), topics.end());
        return topics;
    }
};

/**
 * @brief DDS持久化服务主类 - 基于FastDDS设计增强
 * 
 * 功能：
 * 1. 监控网络中的持久化DataWriter
 * 2. 自动接收和存储持久化数据
 * 3. 为新的DataReader提供历史数据
 * 4. 提供丰富的统计信息和监控接口
 * 5. 支持Qt可视化回调
 */
class PersistenceService {
public:
    // 数据监听器类
    class PersistenceDataListener : public DataReaderListener {
    private:
        PersistenceService* service_;
        std::string topic_name_;
    
    public:
        PersistenceDataListener(PersistenceService* service, std::string topic_name)
            : service_(service), topic_name_(std::move(topic_name)) {}
        
        void on_data_available(DataReader* reader) override;
        
        const std::string& get_topic_name() const { return topic_name_; }
    };

private:
    // 配置和统计
    PersistenceConfig config_;
    PersistenceStatistics stats_;
    
    // Qt UI回调接口
    std::shared_ptr<IPersistenceUICallback> ui_callback_;
    
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
    std::vector<std::thread> worker_threads_;
    std::thread discovery_thread_;
    std::thread cleanup_thread_;
    
    // 任务队列
    std::queue<std::function<void()>> task_queue_;
    std::mutex task_queue_mutex_;
    std::condition_variable task_condition_;

public:
    /**
     * @brief 构造函数 
     * @param participant DDS域参与者
     * @param config 持久化配置
     */
    explicit PersistenceService(DomainParticipant* participant, 
                              const PersistenceConfig& config = PersistenceConfig{});
    
    /**
     * @brief 析构函数
     */
    ~PersistenceService();
    
    // 禁用拷贝构造和赋值
    PersistenceService(const PersistenceService&) = delete;
    PersistenceService& operator=(const PersistenceService&) = delete;
    
    /**
     * @brief 设置Qt UI回调接口
     */
    void set_ui_callback(std::shared_ptr<IPersistenceUICallback> callback) {
        std::lock_guard<std::mutex> lock(task_queue_mutex_);
        ui_callback_ = std::move(callback);
    }
    
    /**
     * @brief 启动持久化服务
     */
    ReturnCode_t start();
    
    /**
     * @brief 停止持久化服务
     */
    void stop();
    
    /**
     * @brief 获取当前配置
     */
    const PersistenceConfig& get_config() const { return config_; }
    
    /**
     * @brief 获取统计信息
     */
    PersistenceStatistics get_statistics() const { return stats_; }
    
    /**
     * @brief 获取网络拓扑信息
     */
    NetworkTopology get_network_topology() const;
    
    /**
     * @brief 查询历史数据 
     */
    std::vector<StoredData> query_historical_data(
        const std::string& topic_name,
        const Time_t& start_time = Time_t{0, 0},
        const Time_t& end_time = Time_t{0x7FFFFFFF, 0xFFFFFFFF});
    
    /**
     * @brief 查询特定实例的历史数据
     */
    std::vector<StoredData> query_instance_data(
        const std::string& topic_name,
        const std::string& instance_id);
    
    /**
     * @brief 获取所有活跃的Topic列表
     */
    std::vector<std::string> get_active_topics() const;
    
    /**
     * @brief 清理指定Topic的数据
     */
    bool cleanup_topic_data(const std::string& topic_name);
    
    /**
     * @brief 导出数据为JSON格式（便于Qt显示）
     */
    std::string export_data_json(const std::string& topic_name) const;

private:
    /**
     * @brief 初始化数据库
     */
    bool initialize_database();
    
    /**
     * @brief 创建数据库表结构
     */
    bool create_enhanced_tables();
    
    /**
     * @brief 启动工作线程
     */
    void start_worker_threads();
    
    /**
     * @brief 工作线程函数
     */
    void worker_thread_function();
    
    /**
     * @brief 添加任务到队列
     */
    void enqueue_task(std::function<void()> task);
    
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
     * @brief 存储数据到数据库 - 增强版本
     */
    bool store_data(const std::string& topic_name,
                   const std::string& type_name,
                   const std::string& instance_id,
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
    
    /**
     * @brief 更新统计信息
     */
    void update_statistics(const std::string& topic_name, size_t data_size, bool is_store);
    
    /**
     * @brief 通知UI回调
     */
    void notify_ui_callback(std::function<void(IPersistenceUICallback*)> callback);
    
    /**
     * @brief 检查数据库大小限制
     */
    bool check_database_size_limit();
    
    /**
     * @brief 压缩数据（如果启用）
     */
    std::vector<uint8_t> compress_data(const std::vector<uint8_t>& data) const;
    
    /**
     * @brief 解压缩数据
     */
    std::vector<uint8_t> decompress_data(const std::vector<uint8_t>& compressed_data) const;
};

AGIDDS_NAMESPACE_END
