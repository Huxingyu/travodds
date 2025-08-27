#pragma once

#include "PersistenceService.h"
#include <memory>
#include <functional>
#include <mutex>

AGIDDS_NAMESPACE_BEGIN

/**
 * @brief Qt可视化接口类
 * 
 * 这个类为将来的Qt图形界面开发提供接口
 * 可以轻松集成到Qt应用程序中
 */
class QtPersistenceInterface : public IPersistenceUICallback, 
                              public std::enable_shared_from_this<QtPersistenceInterface> {
public:
    // Qt信号回调类型定义
    using WriterDiscoveredCallback = std::function<void(const std::string&, const std::string&)>;
    using ReaderDiscoveredCallback = std::function<void(const std::string&, const std::string&)>;
    using DataStoredCallback = std::function<void(const std::string&, size_t)>;
    using HistoricalDataSentCallback = std::function<void(const std::string&, size_t)>;
    using StatisticsUpdatedCallback = std::function<void()>;
    
    // 数据结构用于Qt显示
    struct TopicInfo {
        std::string name;
        std::string type;
        uint64_t sample_count = 0;
        uint64_t total_bytes = 0;
        bool has_persistent_writers = false;
        bool has_persistent_readers = false;
        std::chrono::system_clock::time_point last_activity;
    };
    
    struct NetworkInfo {
        size_t total_participants = 0;
        size_t total_writers = 0;
        size_t total_readers = 0;
        size_t persistent_writers = 0;
        size_t persistent_readers = 0;
        std::vector<TopicInfo> topics;
    };

private:
    std::shared_ptr<PersistenceService> persistence_service_;
    
    // Qt回调函数
    WriterDiscoveredCallback writer_discovered_cb_;
    ReaderDiscoveredCallback reader_discovered_cb_;
    DataStoredCallback data_stored_cb_;
    HistoricalDataSentCallback historical_data_sent_cb_;
    StatisticsUpdatedCallback statistics_updated_cb_;
    
    // 缓存的网络信息
    mutable std::mutex network_info_mutex_;
    NetworkInfo cached_network_info_;

public:
    /**
     * @brief 构造函数
     * @param service 持久化服务实例
     */
    explicit QtPersistenceInterface(std::shared_ptr<PersistenceService> service);
    
    /**
     * @brief 析构函数
     */
    ~QtPersistenceInterface() override = default;
    
    // 禁用拷贝
    QtPersistenceInterface(const QtPersistenceInterface&) = delete;
    QtPersistenceInterface& operator=(const QtPersistenceInterface&) = delete;
    
    // 设置Qt回调函数
    void set_writer_discovered_callback(WriterDiscoveredCallback callback) {
        writer_discovered_cb_ = std::move(callback);
    }
    
    void set_reader_discovered_callback(ReaderDiscoveredCallback callback) {
        reader_discovered_cb_ = std::move(callback);
    }
    
    void set_data_stored_callback(DataStoredCallback callback) {
        data_stored_cb_ = std::move(callback);
    }
    
    void set_historical_data_sent_callback(HistoricalDataSentCallback callback) {
        historical_data_sent_cb_ = std::move(callback);
    }
    
    void set_statistics_updated_callback(StatisticsUpdatedCallback callback) {
        statistics_updated_cb_ = std::move(callback);
    }
    
    /**
     * @brief 获取网络信息（适用于Qt显示）
     */
    NetworkInfo get_network_info() const;
    
    /**
     * @brief 获取持久化统计信息
     */
    PersistenceStatistics get_statistics() const {
        return persistence_service_->get_statistics();
    }
    
    /**
     * @brief 获取活跃主题列表
     */
    std::vector<std::string> get_active_topics() const {
        return persistence_service_->get_active_topics();
    }
    
    /**
     * @brief 查询主题的历史数据
     */
    std::vector<StoredData> query_topic_data(const std::string& topic_name) const {
        return persistence_service_->query_historical_data(topic_name);
    }
    
    /**
     * @brief 导出主题数据为JSON（便于Qt处理）
     */
    std::string export_topic_json(const std::string& topic_name) const {
        return persistence_service_->export_data_json(topic_name);
    }
    
    /**
     * @brief 清理指定主题的数据
     */
    bool cleanup_topic(const std::string& topic_name) {
        return persistence_service_->cleanup_topic_data(topic_name);
    }
    
    /**
     * @brief 获取持久化服务配置
     */
    const PersistenceConfig& get_config() const {
        return persistence_service_->get_config();
    }

protected:
    // IPersistenceUICallback 接口实现
    void on_writer_discovered(const std::string& topic, const std::string& type) override;
    void on_reader_discovered(const std::string& topic, const std::string& type) override;
    void on_data_stored(const std::string& topic, size_t data_size) override;
    void on_historical_data_sent(const std::string& topic, size_t count) override;
    void on_statistics_updated() override;

private:
    /**
     * @brief 更新缓存的网络信息
     */
    void update_network_info();
};

AGIDDS_NAMESPACE_END
