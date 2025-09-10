#pragma once

#include "PersistenceTypes.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include <sqlite3.h>

AGIDDS_NAMESPACE_BEGIN

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
                               const std::string& type_name,
                               DurabilityQosPolicyKind durability_kind);
    bool update_topic_stats(const std::string& topic_name, size_t data_size);
    
    // 清理操作
    bool cleanup_old_data(std::chrono::hours retention_time);
    bool cleanup_topic_data(const std::string& topic_name);
    
    // DataWriter/DataReader 管理
    bool register_datawriter(const BuiltinTopicKey_t& key,
                            const std::string& topic_name,
                            const std::string& type_name,
                            DurabilityQosPolicyKind durability_kind,
                            ReliabilityQosPolicyKind reliability_kind);
    
    bool register_datareader(const BuiltinTopicKey_t& key,
                           const std::string& topic_name,
                           const std::string& type_name,
                           DurabilityQosPolicyKind durability_kind,
                           ReliabilityQosPolicyKind reliability_kind);
    
    bool update_entity_activity(const BuiltinTopicKey_t& key, bool is_writer);
    bool mark_entity_inactive(const BuiltinTopicKey_t& key, bool is_writer);
    
    auto get_active_datawriters(const std::string& topic_name) -> std::vector<BuiltinTopicKey_t>;
    auto get_active_datareaders(const std::string& topic_name) -> std::vector<BuiltinTopicKey_t>;
    
    bool is_datawriter_active(const BuiltinTopicKey_t& key);
    bool cleanup_inactive_entities(std::chrono::minutes inactive_threshold);
    
    // 统计信息
    auto get_database_stats() -> std::map<std::string, int64_t>;
    bool is_initialized() const { return db_ != nullptr; }
    
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
    
    // GUID和BuiltinTopicKey转换辅助方法
    static auto builtin_key_to_guid_prefix(const BuiltinTopicKey_t& key) -> std::vector<uint8_t>;
    static auto builtin_key_to_guid_entity(const BuiltinTopicKey_t& key) -> std::vector<uint8_t>;
    static auto guid_to_builtin_key(const std::vector<uint8_t>& prefix, const std::vector<uint8_t>& entity) -> BuiltinTopicKey_t;
};

AGIDDS_NAMESPACE_END
