#include "DatabaseManager.h"
#include <sqlite3.h>
#include <iostream>
#include <chrono>
#include <map>
#include <cstdio>

AGIDDS_NAMESPACE_BEGIN

namespace {
    constexpr int64_t MICROSECONDS_PER_SECOND = 1000000;
    constexpr int NANOSECONDS_PER_MICROSECOND = 1000;
    
    // SQLite语句清理器
    struct StatementDeleter {
        void operator()(sqlite3_stmt* stmt) const {
            if (stmt) {
                sqlite3_finalize(stmt);
            }
        }
    };
    
    using UniqueStatement = std::unique_ptr<sqlite3_stmt, StatementDeleter>;
}

DatabaseManager::DatabaseManager(const DatabaseConfig& config) 
    : config_(config) {}

DatabaseManager::~DatabaseManager() {
    shutdown();
}

bool DatabaseManager::initialize() {
    if (db_) {
        return true;  // 已经初始化
    }
    
    return initialize_with_recovery();
}

bool DatabaseManager::initialize_with_recovery() {
    // 尝试打开数据库
    int result = sqlite3_open_v2(
        config_.database_path.c_str(),
        &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr
    );
    
    if (result != SQLITE_OK) {
        std::cerr << "[DatabaseManager] Failed to open database: " 
                  << sqlite3_errmsg(db_) << std::endl;
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        
        // 如果是I/O错误，尝试删除损坏的数据库文件并重新创建
        if (result == SQLITE_IOERR || result == SQLITE_CORRUPT) {
            std::cerr << "[DatabaseManager] Database appears corrupted, attempting recovery..." << std::endl;
            return recover_corrupted_database();
        }
        
        return false;
    }
    
    // 配置数据库 - 尝试执行基本操作来验证数据库完整性
    bool db_corrupted = false;
    
    if (config_.enable_foreign_keys) {
        if (!execute_sql("PRAGMA foreign_keys = ON;")) {
            db_corrupted = true;
        }
    }
    
    if (!db_corrupted && config_.enable_wal_mode) {
        if (!execute_sql("PRAGMA journal_mode = WAL;")) {
            db_corrupted = true;
        }
    }
    
    if (!db_corrupted) {
        if (!execute_sql("PRAGMA synchronous = NORMAL;") ||
            !execute_sql("PRAGMA temp_store = MEMORY;") ||
            !execute_sql("PRAGMA cache_size = -64000;")) {  // 64MB cache
            db_corrupted = true;
        }
    }
    
    // 如果检测到数据库损坏，尝试恢复
    if (db_corrupted) {
        shutdown();
        std::cerr << "[DatabaseManager] Database corruption detected during configuration" << std::endl;
        return recover_corrupted_database();
    }
    
    // 创建表结构
    if (!create_tables()) {
        shutdown();
        std::cerr << "[DatabaseManager] Failed to create tables, attempting recovery..." << std::endl;
        return recover_corrupted_database();
    }
    
    std::cout << "[DatabaseManager] Database initialized: " << config_.database_path << std::endl;
    return true;
}

void DatabaseManager::shutdown() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        std::cout << "[DatabaseManager] Database connection closed" << std::endl;
    }
}

bool DatabaseManager::recover_corrupted_database() {
    std::cout << "[DatabaseManager] Attempting to recover corrupted database..." << std::endl;
    
    // 备份文件名
    std::string backup_path = config_.database_path + ".corrupted_backup";
    
    // 尝试重命名损坏的文件作为备份
    if (std::rename(config_.database_path.c_str(), backup_path.c_str()) == 0) {
        std::cout << "[DatabaseManager] Corrupted database backed up to: " << backup_path << std::endl;
    } else {
        // 如果重命名失败，直接删除
        std::remove(config_.database_path.c_str());
        std::cout << "[DatabaseManager] Removed corrupted database file" << std::endl;
    }
    
    // 删除相关的WAL和SHM文件
    std::remove((config_.database_path + "-wal").c_str());
    std::remove((config_.database_path + "-shm").c_str());
    
    // 直接重新创建数据库，避免递归调用initialize_with_recovery
    int result = sqlite3_open_v2(
        config_.database_path.c_str(),
        &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr
    );
    
    if (result != SQLITE_OK) {
        std::cerr << "[DatabaseManager] Failed to create new database after recovery: " 
                  << sqlite3_errmsg(db_) << std::endl;
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        return false;
    }
    
    // 基本配置
    execute_sql("PRAGMA synchronous = NORMAL;");
    execute_sql("PRAGMA temp_store = MEMORY;");
    
    // 创建表结构
    if (!create_tables()) {
        std::cerr << "[DatabaseManager] Failed to create tables after recovery" << std::endl;
        shutdown();
        return false;
    }
    
    std::cout << "[DatabaseManager] Database recovery completed successfully" << std::endl;
    return true;
}

bool DatabaseManager::create_tables() {
    // 核心数据表：dds_samples - 只存储PERSISTENT策略的数据
    const std::string dds_samples_table = R"(
        CREATE TABLE IF NOT EXISTS dds_samples (
            topic_name TEXT NOT NULL,
            writer_guid_prefix BLOB NOT NULL,
            writer_guid_entity BLOB NOT NULL,  
            sequence_number_high INTEGER NOT NULL,
            sequence_number_low INTEGER NOT NULL,
            instance_handle_hash BLOB NOT NULL,
            sample_status INTEGER NOT NULL DEFAULT 0,
            source_timestamp INTEGER NOT NULL,
            reception_timestamp INTEGER NOT NULL,
            serialized_data BLOB NOT NULL,
            data_size INTEGER NOT NULL,
            PRIMARY KEY (topic_name, writer_guid_prefix, writer_guid_entity, sequence_number_high, sequence_number_low)
        );
    )";
    

    // Topic注册表：记录持久化服务处理的所有Topic信息
    const std::string topics_table = R"(
        CREATE TABLE IF NOT EXISTS topics (
            topic_name TEXT PRIMARY KEY,
            type_name TEXT NOT NULL,
            durability_kind INTEGER NOT NULL,
            first_seen INTEGER NOT NULL,
            last_updated INTEGER NOT NULL,
            total_samples INTEGER NOT NULL DEFAULT 0,
            total_bytes INTEGER NOT NULL DEFAULT 0
        );
    )";
    
    // DataWriter注册表：记录发现的DataWriter信息
    const std::string datawriters_table = R"(
        CREATE TABLE IF NOT EXISTS datawriters (
            writer_guid_prefix BLOB NOT NULL,
            writer_guid_entity BLOB NOT NULL,
            topic_name TEXT NOT NULL,
            type_name TEXT NOT NULL,
            durability_kind INTEGER NOT NULL,
            reliability_kind INTEGER NOT NULL,
            discovered_at INTEGER NOT NULL,
            last_seen INTEGER NOT NULL,
            is_active INTEGER NOT NULL DEFAULT 1,
            PRIMARY KEY (writer_guid_prefix, writer_guid_entity),
            FOREIGN KEY (topic_name) REFERENCES topics(topic_name) ON DELETE CASCADE
        );
    )";
    
    // DataReader注册表：记录发现的DataReader信息  
    const std::string datareaders_table = R"(
        CREATE TABLE IF NOT EXISTS datareaders (
            reader_guid_prefix BLOB NOT NULL,
            reader_guid_entity BLOB NOT NULL,
            topic_name TEXT NOT NULL,
            type_name TEXT NOT NULL,
            durability_kind INTEGER NOT NULL,
            reliability_kind INTEGER NOT NULL,
            discovered_at INTEGER NOT NULL,
            last_seen INTEGER NOT NULL,
            is_active INTEGER NOT NULL DEFAULT 1,
            PRIMARY KEY (reader_guid_prefix, reader_guid_entity),
            FOREIGN KEY (topic_name) REFERENCES topics(topic_name) ON DELETE CASCADE
        );
    )";
    
    // 系统统计表：记录持久化服务的运行统计
    const std::string service_stats_table = R"(
        CREATE TABLE IF NOT EXISTS service_stats (
            stat_name TEXT PRIMARY KEY,
            stat_value INTEGER NOT NULL,
            last_updated INTEGER NOT NULL
        );
    )";
    
    // 创建高效索引
    const std::vector<std::string> indexes = {
        // dds_samples表的索引
        "CREATE INDEX IF NOT EXISTS idx_samples_topic ON dds_samples(topic_name);",
        "CREATE INDEX IF NOT EXISTS idx_samples_timestamp ON dds_samples(source_timestamp);",
        "CREATE INDEX IF NOT EXISTS idx_samples_instance ON dds_samples(instance_handle_hash);",
        "CREATE INDEX IF NOT EXISTS idx_samples_writer ON dds_samples(writer_guid_prefix, writer_guid_entity);",
        
        // topics表的索引
        "CREATE INDEX IF NOT EXISTS idx_topics_durability ON topics(durability_kind);",
        
        // datawriters表的索引
        "CREATE INDEX IF NOT EXISTS idx_writers_topic ON datawriters(topic_name);",
        "CREATE INDEX IF NOT EXISTS idx_writers_active ON datawriters(is_active);",
        
        // datareaders表的索引
        "CREATE INDEX IF NOT EXISTS idx_readers_topic ON datareaders(topic_name);",
        "CREATE INDEX IF NOT EXISTS idx_readers_active ON datareaders(is_active);"
    };
    
    // 执行表创建
    if (!execute_sql(dds_samples_table) || 
        !execute_sql(topics_table) ||
        !execute_sql(datawriters_table) ||
        !execute_sql(datareaders_table) ||
        !execute_sql(service_stats_table)) {
        std::cerr << "[DatabaseManager] Failed to create tables" << std::endl;
        return false;
    }
    
    // 创建索引
    for (const auto& index : indexes) {
        execute_sql(index);  // 索引创建失败不是致命错误
    }
    
    return true;
}

bool DatabaseManager::execute_sql(const std::string& sql) {
    if (!db_) return false;
    
    char* error_msg = nullptr;
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_msg);
    
    if (result != SQLITE_OK) {
        std::cerr << "[DatabaseManager] SQL execution failed: " << error_msg << std::endl;
        std::cerr << "[DatabaseManager] SQL: " << sql << std::endl;
        
        // 检查是否是数据库损坏错误
        if (result == SQLITE_NOTADB || result == SQLITE_CORRUPT) {
            std::cerr << "[DatabaseManager] Database corruption detected during SQL execution" << std::endl;
            sqlite3_free(error_msg);
            return false; // 让上层处理恢复
        }
        
        sqlite3_free(error_msg);
        return false;
    }
    
    return true;
}

auto DatabaseManager::prepare_statement(const std::string& sql) -> std::unique_ptr<sqlite3_stmt, void(*)(sqlite3_stmt*)> {
    if (!db_) {
        return {nullptr, [](sqlite3_stmt*){}};
    }
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        std::cerr << "[DatabaseManager] Failed to prepare statement: " 
                  << sqlite3_errmsg(db_) << std::endl;
        std::cerr << "[DatabaseManager] SQL: " << sql << std::endl;
        return {nullptr, [](sqlite3_stmt*){}};
    }
    
    return {stmt, [](sqlite3_stmt* s){ if(s) sqlite3_finalize(s); }};
}

auto DatabaseManager::time_to_microseconds(const Time_t& time) -> int64_t {
    return time.sec * MICROSECONDS_PER_SECOND + time.nanosec / NANOSECONDS_PER_MICROSECOND;
}

auto DatabaseManager::microseconds_to_time(int64_t microseconds) -> Time_t {
    Time_t time;
    time.sec = microseconds / MICROSECONDS_PER_SECOND;
    time.nanosec = (microseconds % MICROSECONDS_PER_SECOND) * NANOSECONDS_PER_MICROSECOND;
    return time;
}

bool DatabaseManager::insert_or_update_topic(const std::string& topic_name, 
                                            const std::string& type_name,
                                            DurabilityQosPolicyKind durability_kind) {
    std::cout << "[DatabaseManager] Insert/update topic: " << topic_name << std::endl;
    
    if (db_ == nullptr) {
        std::cout << "[DatabaseManager] Database not initialized" << std::endl;
        return false;
    }
    
    try {
        // 使用INSERT OR REPLACE来插入或更新topic
        const char* insert_sql = R"(
            INSERT OR REPLACE INTO topics (topic_name, type_name, durability_kind, first_seen, last_updated, total_samples, total_bytes)
            VALUES (?, ?, ?, ?, ?, 0, 0)
        )";
        
        sqlite3_stmt* stmt = nullptr;
        int result = sqlite3_prepare_v2(db_, insert_sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            std::cout << "[DatabaseManager] Failed to prepare topic insert statement: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        // 获取当前时间戳
        int64_t current_time = std::time(nullptr);
        
        // 绑定参数
        sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, type_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, static_cast<int>(durability_kind));
        sqlite3_bind_int64(stmt, 4, current_time);
        sqlite3_bind_int64(stmt, 5, current_time);
        
        result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (result == SQLITE_DONE) {
            std::cout << "[DatabaseManager] Topic inserted/updated successfully: " << topic_name << std::endl;
            return true;
        }
        std::cout << "[DatabaseManager] Failed to insert/update topic: " << sqlite3_errmsg(db_) << std::endl;
        return false;
        
    } catch (const std::exception& e) {
        std::cout << "[DatabaseManager] Exception during topic insert/update: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::store_data(const StoredData& data) {
    std::cout << "[DatabaseManager] Storing PERSISTENT data for topic: " << data.topic_name << std::endl;
    
    if (!db_) {
        std::cout << "[DatabaseManager] Database not initialized" << std::endl;
        return false;
    }
    
    try {
        // 只有PERSISTENT策略的数据才写入数据库
        if (data.durability_kind != PERSISTENT_DURABILITY_QOS) {
            std::cout << "[DatabaseManager] Skipping non-PERSISTENT data (durability=" 
                      << static_cast<int>(data.durability_kind) << ")" << std::endl;
            return true; // 不是错误，只是不需要存储
        }
        
        // 确保topic已注册
        insert_or_update_topic(data.topic_name, data.type_name, data.durability_kind);
        
        // 插入样本数据到dds_samples表
        const char* insert_sql = R"(
            INSERT OR REPLACE INTO dds_samples (
                topic_name, writer_guid_prefix, writer_guid_entity,
                sequence_number_high, sequence_number_low, instance_handle_hash,
                sample_status, source_timestamp, reception_timestamp,
                serialized_data, data_size
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";
        
        sqlite3_stmt* stmt = nullptr;
        int result = sqlite3_prepare_v2(db_, insert_sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            std::cout << "[DatabaseManager] Failed to prepare insert statement: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        // 绑定参数
        sqlite3_bind_text(stmt, 1, data.topic_name.c_str(), -1, SQLITE_STATIC);
        
        // TODO: 实际实现中需要从DDS GUID中提取这些信息
        // 这里使用placeholder数据
        std::string guid_prefix = "guid_prefix_placeholder";
        std::string guid_entity = "guid_entity_placeholder";
        sqlite3_bind_blob(stmt, 2, guid_prefix.data(), guid_prefix.size(), SQLITE_STATIC);
        sqlite3_bind_blob(stmt, 3, guid_entity.data(), guid_entity.size(), SQLITE_STATIC);
        
        // 序列号分解为高低位
        sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(data.sequence_number >> 32));
        sqlite3_bind_int64(stmt, 5, static_cast<int64_t>(data.sequence_number & 0xFFFFFFFF));
        
        // 实例句柄哈希
        sqlite3_bind_blob(stmt, 6, data.instance_id.data(), data.instance_id.size(), SQLITE_STATIC);
        
        // 样本状态
        sqlite3_bind_int(stmt, 7, data.is_disposed ? 1 : 0);
        
        // 时间戳转换为微秒
        int64_t source_timestamp_us = data.storage_time.ToNs() / 1000;
        int64_t reception_timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
            
        sqlite3_bind_int64(stmt, 8, source_timestamp_us);
        sqlite3_bind_int64(stmt, 9, reception_timestamp_us);
        
        // 序列化数据
        sqlite3_bind_blob(stmt, 10, data.data.data(), data.data.size(), SQLITE_STATIC);
        sqlite3_bind_int(stmt, 11, static_cast<int>(data.data.size()));
        
        result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (result == SQLITE_DONE) {
            // 更新topic统计
            update_topic_stats(data.topic_name, data.data.size());
            
            std::cout << "[DatabaseManager] PERSISTENT data stored successfully for topic: " << data.topic_name 
                      << ", sequence: " << data.sequence_number << std::endl;
            return true;
        }
        std::cout << "[DatabaseManager] Failed to store data: " << sqlite3_errmsg(db_) << std::endl;
        return false;
        
    } catch (const std::exception& e) {
        std::cout << "[DatabaseManager] Exception during data storage: " << e.what() << std::endl;
        return false;
    }
}

auto DatabaseManager::load_data(const std::string& topic_name) -> std::vector<StoredData> {
    std::vector<StoredData> result;
    
    if (db_ == nullptr) {
        std::cout << "[DatabaseManager] Database not initialized" << std::endl;
        return result;
    }
    
    try {
        // 从新的dds_samples表读取PERSISTENT数据
        const char* select_sql = R"(
            SELECT s.topic_name, t.type_name, s.instance_handle_hash, s.serialized_data,
                   s.source_timestamp, s.sequence_number_high, s.sequence_number_low,
                   s.sample_status, t.durability_kind
            FROM dds_samples s
            JOIN topics t ON s.topic_name = t.topic_name
            WHERE s.topic_name = ? AND t.durability_kind = ?
            ORDER BY s.sequence_number_high, s.sequence_number_low
        )";
        
        sqlite3_stmt* stmt = nullptr;
        int query_result = sqlite3_prepare_v2(db_, select_sql, -1, &stmt, nullptr);
        if (query_result != SQLITE_OK) {
            std::cout << "[DatabaseManager] Failed to prepare select statement: " << sqlite3_errmsg(db_) << std::endl;
            return result;
        }
        
        sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, static_cast<int>(PERSISTENT_DURABILITY_QOS));
        
        while ((query_result = sqlite3_step(stmt)) == SQLITE_ROW) {
            StoredData data;
            data.topic_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            data.type_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            
            // 实例句柄哈希作为实例ID
            const void* hash_data = sqlite3_column_blob(stmt, 2);
            int hash_size = sqlite3_column_bytes(stmt, 2);
            if (hash_data != nullptr && hash_size > 0) {
                const auto* byte_data = static_cast<const char*>(hash_data);
                data.instance_id.assign(byte_data, hash_size);
            }
            
            // 读取序列化数据
            const void* blob_data = sqlite3_column_blob(stmt, 3);
            int blob_size = sqlite3_column_bytes(stmt, 3);
            if (blob_data != nullptr && blob_size > 0) {
                const auto* byte_data = static_cast<const uint8_t*>(blob_data);
                data.data.assign(byte_data, byte_data + blob_size);
            }
            
            // 时间戳转换
            int64_t timestamp_us = sqlite3_column_int64(stmt, 4);
            data.storage_time.sec = timestamp_us / 1000000;
            data.storage_time.nanosec = (timestamp_us % 1000000) * 1000;
            
            // 重组序列号
            uint64_t seq_high = static_cast<uint64_t>(sqlite3_column_int64(stmt, 5));
            uint64_t seq_low = static_cast<uint64_t>(sqlite3_column_int64(stmt, 6));
            data.sequence_number = (seq_high << 32) | seq_low;
            
            data.is_disposed = sqlite3_column_int(stmt, 7) != 0;
            data.durability_kind = static_cast<DurabilityQosPolicyKind>(sqlite3_column_int(stmt, 8));
            
            // 设置默认的样本状态
            data.sample_info.sample_state = READ_SAMPLE_STATE;
            data.sample_info.view_state = NEW_VIEW_STATE;
            data.sample_info.instance_state = ALIVE_INSTANCE_STATE;
            
            result.push_back(data);
        }
        
        sqlite3_finalize(stmt);
        
        std::cout << "[DatabaseManager] Loaded " << result.size() << " PERSISTENT records for topic: " << topic_name << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "[DatabaseManager] Exception loading data: " << e.what() << std::endl;
    }
    
    return result;
}

auto DatabaseManager::load_data_range(const std::string& topic_name,
                                    const Time_t& start_time,
                                    const Time_t& end_time) -> std::vector<StoredData> {
    std::vector<StoredData> result;
    
    if (db_ == nullptr) {
        std::cout << "[DatabaseManager] Database not initialized" << std::endl;
        return result;
    }
    
    try {
        const char* select_sql = R"(
            SELECT topic_name, type_name, instance_id, data,
                   storage_time_sec, storage_time_nsec, sequence_number,
                   is_disposed, durability_kind, sample_state,
                   view_state, instance_state
            FROM persistent_data 
            WHERE topic_name = ? 
            AND (storage_time_sec > ? OR (storage_time_sec = ? AND storage_time_nsec >= ?))
            AND (storage_time_sec < ? OR (storage_time_sec = ? AND storage_time_nsec <= ?))
            ORDER BY sequence_number
        )";
        
        sqlite3_stmt* stmt = nullptr;
        int query_result = sqlite3_prepare_v2(db_, select_sql, -1, &stmt, nullptr);
        if (query_result != SQLITE_OK) {
            std::cout << "[DatabaseManager] Failed to prepare range select statement: " << sqlite3_errmsg(db_) << std::endl;
            return result;
        }
        
        sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 2, start_time.sec);
        sqlite3_bind_int64(stmt, 3, start_time.sec);
        sqlite3_bind_int(stmt, 4, start_time.nanosec);
        sqlite3_bind_int64(stmt, 5, end_time.sec);
        sqlite3_bind_int64(stmt, 6, end_time.sec);
        sqlite3_bind_int(stmt, 7, end_time.nanosec);
        
        while ((query_result = sqlite3_step(stmt)) == SQLITE_ROW) {
            StoredData data;
            data.topic_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            data.type_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            data.instance_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            
            // 读取blob数据
            const void* blob_data = sqlite3_column_blob(stmt, 3);
            int blob_size = sqlite3_column_bytes(stmt, 3);
            if (blob_data != nullptr && blob_size > 0) {
                const auto* byte_data = static_cast<const uint8_t*>(blob_data);
                data.data.assign(byte_data, byte_data + blob_size);
            }
            
            data.storage_time.sec = sqlite3_column_int64(stmt, 4);
            data.storage_time.nanosec = sqlite3_column_int(stmt, 5);
            data.sequence_number = sqlite3_column_int64(stmt, 6);
            data.is_disposed = sqlite3_column_int(stmt, 7) != 0;
            data.durability_kind = static_cast<DurabilityQosPolicyKind>(sqlite3_column_int(stmt, 8));
            data.sample_info.sample_state = static_cast<SampleStateKind>(sqlite3_column_int(stmt, 9));
            data.sample_info.view_state = static_cast<ViewStateKind>(sqlite3_column_int(stmt, 10));
            data.sample_info.instance_state = static_cast<InstanceStateKind>(sqlite3_column_int(stmt, 11));
            
            result.push_back(data);
        }
        
        sqlite3_finalize(stmt);
        
        std::cout << "[DatabaseManager] Loaded " << result.size() 
                  << " records for topic: " << topic_name << " in time range" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "[DatabaseManager] Exception loading data range: " << e.what() << std::endl;
    }
    
    return result;
}

bool DatabaseManager::cleanup_old_data(std::chrono::hours retention_time) {
    if (db_ == nullptr) {
        std::cout << "[DatabaseManager] Database not initialized" << std::endl;
        return false;
    }
    
    try {
        // 计算过期时间
        auto retention_seconds = std::chrono::duration_cast<std::chrono::seconds>(retention_time).count();
        auto cutoff_time = std::time(nullptr) - retention_seconds;
        
        const char* delete_sql = "DELETE FROM persistent_data WHERE storage_time_sec < ?";
        
        sqlite3_stmt* stmt = nullptr;
        int result = sqlite3_prepare_v2(db_, delete_sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            std::cout << "[DatabaseManager] Failed to prepare cleanup statement: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        sqlite3_bind_int64(stmt, 1, cutoff_time);
        
        result = sqlite3_step(stmt);
        int deleted_rows = sqlite3_changes(db_);
        sqlite3_finalize(stmt);
        
        if (result == SQLITE_DONE) {
            std::cout << "[DatabaseManager] Cleaned up " << deleted_rows << " old records" << std::endl;
            return true;
        }
        std::cout << "[DatabaseManager] Failed to cleanup old data: " << sqlite3_errmsg(db_) << std::endl;
        return false;
        
    } catch (const std::exception& e) {
        std::cout << "[DatabaseManager] Exception during cleanup: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::cleanup_topic_data(const std::string& topic_name) {
    if (db_ == nullptr) {
        std::cout << "[DatabaseManager] Database not initialized" << std::endl;
        return false;
    }
    
    try {
        const char* delete_sql = "DELETE FROM persistent_data WHERE topic_name = ?";
        
        sqlite3_stmt* stmt = nullptr;
        int result = sqlite3_prepare_v2(db_, delete_sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            std::cout << "[DatabaseManager] Failed to prepare topic cleanup statement: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
        
        result = sqlite3_step(stmt);
        int deleted_rows = sqlite3_changes(db_);
        sqlite3_finalize(stmt);
        
        if (result == SQLITE_DONE) {
            std::cout << "[DatabaseManager] Cleaned up " << deleted_rows 
                      << " records for topic: " << topic_name << std::endl;
            return true;
        }
        std::cout << "[DatabaseManager] Failed to cleanup topic data: " << sqlite3_errmsg(db_) << std::endl;
        return false;
        
    } catch (const std::exception& e) {
        std::cout << "[DatabaseManager] Exception during topic cleanup: " << e.what() << std::endl;
        return false;
    }
}

auto DatabaseManager::get_database_stats() -> std::map<std::string, int64_t> {
    std::map<std::string, int64_t> stats;
    
    if (db_ == nullptr) {
        std::cout << "[DatabaseManager] Database not initialized" << std::endl;
        return stats;
    }
    
    try {
        // 获取各表的记录数
        const char* queries[] = {
            "SELECT COUNT(*) FROM persistent_data",
            "SELECT COUNT(*) FROM topics", 
            "SELECT COUNT(*) FROM publishers",
            "SELECT COUNT(*) FROM subscribers",
            "SELECT COUNT(*) FROM statistics"
        };
        
        const char* stat_names[] = {
            "persistent_data_count",
            "topics_count",
            "publishers_count", 
            "subscribers_count",
            "statistics_count"
        };
        
        for (size_t i = 0; i < 5; ++i) {
            sqlite3_stmt* stmt = nullptr;
            int result = sqlite3_prepare_v2(db_, queries[i], -1, &stmt, nullptr);
            if (result == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    stats[stat_names[i]] = sqlite3_column_int64(stmt, 0);
                }
                sqlite3_finalize(stmt);
            }
        }
        
        // 获取数据库文件大小
        const char* pragma_sql = "PRAGMA page_count; PRAGMA page_size;";
        sqlite3_stmt* stmt = nullptr;
        int result = sqlite3_prepare_v2(db_, "PRAGMA page_count", -1, &stmt, nullptr);
        if (result == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t page_count = sqlite3_column_int64(stmt, 0);
            sqlite3_finalize(stmt);
            
            result = sqlite3_prepare_v2(db_, "PRAGMA page_size", -1, &stmt, nullptr);
            if (result == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
                int64_t page_size = sqlite3_column_int64(stmt, 0);
                stats["database_size_bytes"] = page_count * page_size;
                sqlite3_finalize(stmt);
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "[DatabaseManager] Exception getting database stats: " << e.what() << std::endl;
    }
    
    return stats;
}

bool DatabaseManager::update_topic_stats(const std::string& topic_name, size_t data_size) {
    if (!db_) {
        return false;
    }
    
    const char* update_sql = R"(
        UPDATE topics 
        SET total_samples = total_samples + 1, 
            last_updated = ?, 
            total_bytes = total_bytes + ?
        WHERE topic_name = ?
    )";
    
    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db_, update_sql, -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        return false;
    }
    
    int64_t current_time = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    sqlite3_bind_int64(stmt, 1, current_time);
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(data_size));
    sqlite3_bind_text(stmt, 3, topic_name.c_str(), -1, SQLITE_STATIC);
    
    result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return result == SQLITE_DONE;
}

AGIDDS_NAMESPACE_END
