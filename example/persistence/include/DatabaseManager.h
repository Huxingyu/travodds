#pragma once

#include "PersistenceTypes.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include <sqlite3.h>

TRAVODDS_NAMESPACE_BEGIN

// 数据库配置结构
struct DatabaseConfig {
    std::string database_path{"persistence_data.db"};
    std::chrono::hours data_retention{24};
    size_t max_samples_per_topic = 10000;
    bool enable_wal_mode = true;  // Write-Ahead Logging
    bool enable_foreign_keys = true;
    int connection_timeout_ms = 30000;
};

// 数据库管理器
class DatabaseManager {
public:
    explicit DatabaseManager(const DatabaseConfig& config);
    ~DatabaseManager();
    
    // 禁用拷贝
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    // 初始化和清理
    bool initialize();
    void shutdown();
    
private:
    // 数据库恢复相关方法
    bool initialize_with_recovery();
    bool recover_corrupted_database();
    
public:
    
    // 数据操作
    bool store_data(const StoredData& data);
    auto load_data(const std::string& topic_name) -> std::vector<StoredData>;
    auto load_data_range(const std::string& topic_name, 
                        const Time_t& start_time,
                        const Time_t& end_time) -> std::vector<StoredData>;
    
    // Topic管理
    bool insert_or_update_topic(const std::string& topic_name, 
                               const std::string& type_name);
    bool update_topic_stats(const std::string& topic_name, size_t data_size);
    
    // 清理操作
    bool cleanup_old_data(std::chrono::hours retention_time);
    bool cleanup_topic_data(const std::string& topic_name);
    
    // 统计信息
    auto get_database_stats() -> std::map<std::string, int64_t>;
    bool is_initialized() const { return db_ != nullptr; }

    // 【新增】获取所有表名
    auto get_table_names() -> std::vector<std::string>;

    // 【新增】获取指定表的通用内容（返回二维字符串数组）
    auto get_table_content(const std::string& table_name) -> std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>>;
    
private:
    DatabaseConfig config_;
    sqlite3* db_ = nullptr;
    
    // 内部方法
    bool create_tables();
    bool execute_sql(const std::string& sql);
    auto prepare_statement(const std::string& sql) -> std::unique_ptr<struct sqlite3_stmt, void(*)(sqlite3_stmt*)>;
    
    // 辅助方法
    static auto time_to_microseconds(const Time_t& time) -> int64_t;
    static auto microseconds_to_time(int64_t microseconds) -> Time_t;
};

TRAVODDS_NAMESPACE_END
