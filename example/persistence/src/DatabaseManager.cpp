#include "DatabaseManager.h"
#include <sqlite3.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <cstdio>
#include <sstream>
#include <iomanip>

TRAVODDS_NAMESPACE_BEGIN

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
    // topics表：规范化Topic信息
    const std::string create_topics_table_sql = R"(
        CREATE TABLE IF NOT EXISTS topics (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            topic_name TEXT NOT NULL UNIQUE,
            type_name TEXT NOT NULL,
            first_seen INTEGER NOT NULL DEFAULT (strftime('%s', 'now') * 1000000),
            last_updated INTEGER NOT NULL DEFAULT (strftime('%s', 'now') * 1000000),
            total_samples INTEGER NOT NULL DEFAULT 0,
            total_bytes INTEGER NOT NULL DEFAULT 0
        );
    )";

    // samples表：存储每一个数据样本及其完整的QoS上下文
    const std::string create_samples_table_sql = R"(
        CREATE TABLE IF NOT EXISTS samples (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            topic_id INTEGER NOT NULL,
            writer_guid BLOB NOT NULL,
            sequence_number INTEGER NOT NULL,
            source_timestamp INTEGER NOT NULL,
            reception_timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now') * 1000000),
            data BLOB,
            data_size INTEGER NOT NULL DEFAULT 0,
            
            -- QoS字段
            durability_kind INTEGER,
            reliability_kind INTEGER,
            presentation_access_scope INTEGER,
            presentation_coherent_access BOOLEAN,
            presentation_ordered_access BOOLEAN,
            deadline_period_sec INTEGER,
            deadline_period_nanosec INTEGER,
            latency_budget_duration_sec INTEGER,
            latency_budget_duration_nanosec INTEGER,
            ownership_kind INTEGER,
            liveliness_kind INTEGER,
            liveliness_lease_duration_sec INTEGER,
            liveliness_lease_duration_nanosec INTEGER,
            destination_order_kind INTEGER,
            
            FOREIGN KEY (topic_id) REFERENCES topics(id) ON DELETE CASCADE,
            UNIQUE(topic_id, writer_guid, sequence_number)
        );
    )";

    // 创建索引以提高查询性能
    const std::vector<std::string> indexes = {
        "CREATE INDEX IF NOT EXISTS idx_topic_name ON topics(topic_name);",
        "CREATE INDEX IF NOT EXISTS idx_sample_topic_id ON samples(topic_id);",
        "CREATE INDEX IF NOT EXISTS idx_sample_timestamp ON samples(source_timestamp);",
        "CREATE INDEX IF NOT EXISTS idx_sample_writer ON samples(writer_guid);",
        "CREATE INDEX IF NOT EXISTS idx_sample_qos_durability ON samples(durability_kind);",
        "CREATE INDEX IF NOT EXISTS idx_sample_qos_reliability ON samples(reliability_kind);"
    };

    // 执行创建表的SQL语句
    if (!execute_sql(create_topics_table_sql)) {
        std::cerr << "[DatabaseManager] Failed to create topics table" << std::endl;
        return false;
    }
    
    if (!execute_sql(create_samples_table_sql)) {
        std::cerr << "[DatabaseManager] Failed to create samples table" << std::endl;
        return false;
    }
    
    // 创建所有索引
    for (const auto& index_sql : indexes) {
        if (!execute_sql(index_sql)) {
            std::cerr << "[DatabaseManager] Failed to create index: " << index_sql << std::endl;
            return false;
        }
    }

    std::cout << "[DatabaseManager] Database tables created successfully" << std::endl;
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
                                            const std::string& type_name) {
    if (db_ == nullptr) {
        return false;
    }
    
    try {
        // 首先尝试插入topic，如果已存在则忽略（保持原ID不变）
        const char* insert_sql = R"(
            INSERT OR IGNORE INTO topics (topic_name, type_name, first_seen, last_updated, total_samples, total_bytes)
            VALUES (?, ?, ?, ?, 0, 0)
        )";
        
        sqlite3_stmt* stmt = nullptr;
        int result = sqlite3_prepare_v2(db_, insert_sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            std::cerr << "[DatabaseManager] Failed to prepare topic insert statement: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        // 获取当前时间戳
        int64_t current_time = std::time(nullptr);
        
        // 绑定参数
        sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, type_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, current_time);
        sqlite3_bind_int64(stmt, 4, current_time);
        
        result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        // 无论是新插入还是已存在，我们都更新last_updated时间
        const char* update_sql = R"(
            UPDATE topics SET last_updated = ? WHERE topic_name = ?
        )";
        
        sqlite3_stmt* update_stmt = nullptr;
        int update_result = sqlite3_prepare_v2(db_, update_sql, -1, &update_stmt, nullptr);
        if (update_result == SQLITE_OK) {
            sqlite3_bind_int64(update_stmt, 1, current_time);
            sqlite3_bind_text(update_stmt, 2, topic_name.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(update_stmt);
            sqlite3_finalize(update_stmt);
        }
        
        return (result == SQLITE_DONE || result == SQLITE_CONSTRAINT);
        
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseManager] Exception during topic insert/update: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::store_data(const StoredData& data) {
    if (!db_) {
        return false;
    }
    
    try {
        // 确保topic已注册
        insert_or_update_topic(data.topic_name, data.type_name);
        
        // 插入样本数据到samples表，包含完整的QoS信息
        const char* insert_sql = R"(
            INSERT INTO samples (
                topic_id, writer_guid, sequence_number, source_timestamp, reception_timestamp,
                data, data_size,
                durability_kind, reliability_kind,
                presentation_access_scope, presentation_coherent_access, presentation_ordered_access,
                deadline_period_sec, deadline_period_nanosec,
                latency_budget_duration_sec, latency_budget_duration_nanosec,
                ownership_kind, liveliness_kind,
                liveliness_lease_duration_sec, liveliness_lease_duration_nanosec,
                destination_order_kind
            ) VALUES (
                (SELECT id FROM topics WHERE topic_name = ?),
                ?, ?, ?, ?, ?, ?,
                ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
            )
        )";
        
        sqlite3_stmt* stmt = nullptr;
        int result = sqlite3_prepare_v2(db_, insert_sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            std::cerr << "[DatabaseManager] Failed to prepare insert statement: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        // 生成writer GUID作为二进制数据 - 完整的16字节
        std::vector<uint8_t> writer_guid_blob;
        writer_guid_blob.reserve(16); // 12 bytes prefix + 3 bytes entityKey + 1 byte entityKind
        
        // 添加prefix (12 bytes)
        for (int i = 0; i < 12; ++i) {
            writer_guid_blob.push_back(data.writer_guid.prefix.value[i]);
        }
        
        // 添加entityId.entityKey (3 bytes)
        for (int i = 0; i < 3; ++i) {
            writer_guid_blob.push_back(data.writer_guid.entityId.entityKey[i]);
        }
        
        // 添加entityId.entityKind (1 byte)
        writer_guid_blob.push_back(data.writer_guid.entityId.entityKind);
        
        // 绑定基本参数
        sqlite3_bind_text(stmt, 1, data.topic_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_blob(stmt, 2, writer_guid_blob.data(), writer_guid_blob.size(), SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(data.sequence_number));
        
        // 时间戳转换
        int64_t timestamp_us = data.storage_time.ToNs() / 1000;
        int64_t reception_timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        sqlite3_bind_int64(stmt, 4, timestamp_us);
        sqlite3_bind_int64(stmt, 5, reception_timestamp_us);
        
        // 数据
        sqlite3_bind_blob(stmt, 6, data.data.data(), data.data.size(), SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 7, static_cast<int64_t>(data.data.size()));
        
        // QoS字段（从StoredData的QoS策略对象获取）
        sqlite3_bind_int(stmt, 8, static_cast<int>(data.durability_kind));
        sqlite3_bind_int(stmt, 9, static_cast<int>(data.reliability_kind));
        sqlite3_bind_int(stmt, 10, static_cast<int>(data.presentation.access_scope));
        sqlite3_bind_int(stmt, 11, data.presentation.coherent_access ? 1 : 0);
        sqlite3_bind_int(stmt, 12, data.presentation.ordered_access ? 1 : 0);
        sqlite3_bind_int64(stmt, 13, data.deadline.period.sec);
        sqlite3_bind_int64(stmt, 14, data.deadline.period.nanosec);
        sqlite3_bind_int64(stmt, 15, data.latency_budget.duration.sec);
        sqlite3_bind_int64(stmt, 16, data.latency_budget.duration.nanosec);
        sqlite3_bind_int(stmt, 17, static_cast<int>(data.ownership.kind));
        sqlite3_bind_int(stmt, 18, static_cast<int>(data.liveliness.kind));
        sqlite3_bind_int64(stmt, 19, data.liveliness.lease_duration.sec);
        sqlite3_bind_int64(stmt, 20, data.liveliness.lease_duration.nanosec);
        sqlite3_bind_int(stmt, 21, static_cast<int>(data.destination_order.kind));
        
        result = sqlite3_step(stmt);
        
        if (result == SQLITE_DONE) {
            sqlite3_finalize(stmt);
            
            // 强制同步到磁盘确保数据持久化
            sqlite3_exec(db_, "PRAGMA wal_checkpoint(FULL);", nullptr, nullptr, nullptr);
            
            // 更新topic统计
            update_topic_stats(data.topic_name, data.data.size());
            return true;
        }
        
        std::cerr << "[DatabaseManager] Failed to insert sample: " << sqlite3_errmsg(db_) 
                  << " (result code: " << result << ")" << std::endl;
        std::cerr << "[DatabaseManager] Topic: " << data.topic_name << ", sequence: " << data.sequence_number << std::endl;
        sqlite3_finalize(stmt);
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseManager] Exception during data storage: " << e.what() << std::endl;
        return false;
    }
}

auto DatabaseManager::load_data(const std::string& topic_name) -> std::vector<StoredData> {
    std::vector<StoredData> result;
    
    if (db_ == nullptr) {
        return result;
    }
    
    try {
        // 从新的samples表读取数据，通过JOIN获取topic信息
        const char* select_sql = R"(
            SELECT s.writer_guid, s.sequence_number, s.source_timestamp, s.reception_timestamp,
                   s.data, s.data_size,
                   s.durability_kind, s.reliability_kind,
                   s.presentation_access_scope, s.presentation_coherent_access, s.presentation_ordered_access,
                   s.deadline_period_sec, s.deadline_period_nanosec,
                   s.latency_budget_duration_sec, s.latency_budget_duration_nanosec,
                   s.ownership_kind, s.liveliness_kind,
                   s.liveliness_lease_duration_sec, s.liveliness_lease_duration_nanosec,
                   s.destination_order_kind,
                   t.topic_name, t.type_name
            FROM samples s
            JOIN topics t ON s.topic_id = t.id
            WHERE t.topic_name = ?
            ORDER BY s.sequence_number
        )";
        
        sqlite3_stmt* stmt = nullptr;
        int query_result = sqlite3_prepare_v2(db_, select_sql, -1, &stmt, nullptr);
        if (query_result != SQLITE_OK) {
            std::cerr << "[DatabaseManager] Failed to prepare load query: " << sqlite3_errmsg(db_) << std::endl;
            return result;
        }
        
        sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
        
        while ((query_result = sqlite3_step(stmt)) == SQLITE_ROW) {
            StoredData data;
            
            // 基本信息
            data.topic_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20));
            data.type_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 21));
            
            // 解析writer GUID从BLOB (16字节)
            const void* guid_blob = sqlite3_column_blob(stmt, 0);
            int guid_size = sqlite3_column_bytes(stmt, 0);
            if (guid_blob && guid_size == 16) {
                const uint8_t* guid_bytes = static_cast<const uint8_t*>(guid_blob);
                
                // 解析prefix（前12字节）
                for (int i = 0; i < 12; ++i) {
                    data.writer_guid.prefix.value[i] = guid_bytes[i];
                }
                
                // 解析entityId.entityKey（接下来3字节）
                for (int i = 0; i < 3; ++i) {
                    data.writer_guid.entityId.entityKey[i] = guid_bytes[12 + i];
                }
                
                // 解析entityId.entityKind（最后1字节）
                data.writer_guid.entityId.entityKind = guid_bytes[15];
            }
            
            data.sequence_number = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
            
            // 时间戳转换
            int64_t timestamp_us = sqlite3_column_int64(stmt, 2);
            data.storage_time.sec = timestamp_us / 1000000;
            data.storage_time.nanosec = (timestamp_us % 1000000) * 1000;
            
            // 读取序列化数据
            const void* blob_data = sqlite3_column_blob(stmt, 4);
            int blob_size = sqlite3_column_bytes(stmt, 4);
            if (blob_data != nullptr && blob_size > 0) {
                const auto* byte_data = static_cast<const uint8_t*>(blob_data);
                data.data.assign(byte_data, byte_data + blob_size);
            }
            
            // 恢复完整的QoS信息
            data.durability_kind = static_cast<DurabilityQosPolicyKind>(sqlite3_column_int(stmt, 6));
            data.reliability_kind = static_cast<ReliabilityQosPolicyKind>(sqlite3_column_int(stmt, 7));
            data.presentation.access_scope = static_cast<PresentationQosPolicyAccessScopeKind>(sqlite3_column_int(stmt, 8));
            data.presentation.coherent_access = sqlite3_column_int(stmt, 9) != 0;
            data.presentation.ordered_access = sqlite3_column_int(stmt, 10) != 0;
            data.deadline.period.sec = sqlite3_column_int64(stmt, 11);
            data.deadline.period.nanosec = sqlite3_column_int64(stmt, 12);
            data.latency_budget.duration.sec = sqlite3_column_int64(stmt, 13);
            data.latency_budget.duration.nanosec = sqlite3_column_int64(stmt, 14);
            data.ownership.kind = static_cast<OwnershipQosPolicyKind>(sqlite3_column_int(stmt, 15));
            data.liveliness.kind = static_cast<LivelinessQosPolicyKind>(sqlite3_column_int(stmt, 16));
            data.liveliness.lease_duration.sec = sqlite3_column_int64(stmt, 17);
            data.liveliness.lease_duration.nanosec = sqlite3_column_int64(stmt, 18);
            data.destination_order.kind = static_cast<DestinationOrderQosPolicyKind>(sqlite3_column_int(stmt, 19));
            
            result.push_back(std::move(data));
        }
        
        sqlite3_finalize(stmt);
        
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseManager] Exception during data load: " << e.what() << std::endl;
    }
    
    return result;
}

auto DatabaseManager::load_data_range(const std::string& topic_name,
                                    const Time_t& start_time,
                                    const Time_t& end_time) -> std::vector<StoredData> {
    std::vector<StoredData> result;
    
    if (db_ == nullptr) {
        std::cerr << "[DatabaseManager] Database not initialized" << std::endl;
        return result;
    }
    
    try {
        // 将Time_t转换为微秒时间戳进行比较
        int64_t start_timestamp_us = start_time.sec * MICROSECONDS_PER_SECOND + start_time.nanosec / NANOSECONDS_PER_MICROSECOND;
        int64_t end_timestamp_us = end_time.sec * MICROSECONDS_PER_SECOND + end_time.nanosec / NANOSECONDS_PER_MICROSECOND;
        
        // 从新的samples表读取指定时间范围的数据
        const char* select_sql = R"(
            SELECT s.writer_guid, s.sequence_number, s.source_timestamp, s.reception_timestamp,
                   s.data, s.data_size,
                   s.durability_kind, s.reliability_kind,
                   s.presentation_access_scope, s.presentation_coherent_access, s.presentation_ordered_access,
                   s.deadline_period_sec, s.deadline_period_nanosec,
                   s.latency_budget_duration_sec, s.latency_budget_duration_nanosec,
                   s.ownership_kind, s.liveliness_kind,
                   s.liveliness_lease_duration_sec, s.liveliness_lease_duration_nanosec,
                   s.destination_order_kind,
                   t.topic_name, t.type_name
            FROM samples s
            JOIN topics t ON s.topic_id = t.id
            WHERE t.topic_name = ? 
            AND s.source_timestamp >= ? 
            AND s.source_timestamp <= ?
            ORDER BY s.source_timestamp, s.sequence_number
        )";
        
        sqlite3_stmt* stmt = nullptr;
        int query_result = sqlite3_prepare_v2(db_, select_sql, -1, &stmt, nullptr);
        if (query_result != SQLITE_OK) {
            std::cerr << "[DatabaseManager] Failed to prepare range select statement: " << sqlite3_errmsg(db_) << std::endl;
            return result;
        }
        
        sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 2, start_timestamp_us);
        sqlite3_bind_int64(stmt, 3, end_timestamp_us);
        
        while ((query_result = sqlite3_step(stmt)) == SQLITE_ROW) {
            StoredData data;
            
            // 基本信息
            data.topic_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20));
            data.type_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 21));
            
            // 解析writer GUID从BLOB (16字节)
            const void* guid_blob = sqlite3_column_blob(stmt, 0);
            int guid_size = sqlite3_column_bytes(stmt, 0);
            if (guid_blob && guid_size == 16) {
                const auto* guid_bytes = static_cast<const uint8_t*>(guid_blob);
                
                // 解析prefix（前12字节）
                for (int i = 0; i < 12; ++i) {
                    data.writer_guid.prefix.value[i] = guid_bytes[i];
                }
                
                // 解析entityId.entityKey（接下来3字节）
                for (int i = 0; i < 3; ++i) {
                    data.writer_guid.entityId.entityKey[i] = guid_bytes[12 + i];
                }
                
                // 解析entityId.entityKind（最后1字节）
                data.writer_guid.entityId.entityKind = guid_bytes[15];
            }
            
            data.sequence_number = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
            
            // 时间戳转换
            int64_t timestamp_us = sqlite3_column_int64(stmt, 2);
            data.storage_time.sec = timestamp_us / MICROSECONDS_PER_SECOND;
            data.storage_time.nanosec = (timestamp_us % MICROSECONDS_PER_SECOND) * NANOSECONDS_PER_MICROSECOND;
            
            // 读取序列化数据
            const void* blob_data = sqlite3_column_blob(stmt, 4);
            int blob_size = sqlite3_column_bytes(stmt, 4);
            if (blob_data != nullptr && blob_size > 0) {
                const auto* byte_data = static_cast<const uint8_t*>(blob_data);
                data.data.assign(byte_data, byte_data + blob_size);
            }
            
            // 恢复完整的QoS信息
            data.durability_kind = static_cast<DurabilityQosPolicyKind>(sqlite3_column_int(stmt, 6));
            data.reliability_kind = static_cast<ReliabilityQosPolicyKind>(sqlite3_column_int(stmt, 7));
            data.presentation.access_scope = static_cast<PresentationQosPolicyAccessScopeKind>(sqlite3_column_int(stmt, 8));
            data.presentation.coherent_access = sqlite3_column_int(stmt, 9) != 0;
            data.presentation.ordered_access = sqlite3_column_int(stmt, 10) != 0;
            data.deadline.period.sec = sqlite3_column_int64(stmt, 11);
            data.deadline.period.nanosec = sqlite3_column_int64(stmt, 12);
            data.latency_budget.duration.sec = sqlite3_column_int64(stmt, 13);
            data.latency_budget.duration.nanosec = sqlite3_column_int64(stmt, 14);
            data.ownership.kind = static_cast<OwnershipQosPolicyKind>(sqlite3_column_int(stmt, 15));
            data.liveliness.kind = static_cast<LivelinessQosPolicyKind>(sqlite3_column_int(stmt, 16));
            data.liveliness.lease_duration.sec = sqlite3_column_int64(stmt, 17);
            data.liveliness.lease_duration.nanosec = sqlite3_column_int64(stmt, 18);
            data.destination_order.kind = static_cast<DestinationOrderQosPolicyKind>(sqlite3_column_int(stmt, 19));
            
            result.push_back(std::move(data));
        }
        
        sqlite3_finalize(stmt);
        
        std::cout << "[DatabaseManager] Loaded " << result.size() 
                  << " samples for topic '" << topic_name << "' in time range" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseManager] Exception loading data range: " << e.what() << std::endl;
    }
    
    return result;
}

bool DatabaseManager::cleanup_old_data(std::chrono::hours retention_time) {
    if (db_ == nullptr) {
        std::cerr << "[DatabaseManager] Database not initialized" << std::endl;
        return false;
    }
    
    try {
        // 计算过期时间（微秒为单位）
        auto retention_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(retention_time).count();
        auto now_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto cutoff_time_microseconds = now_microseconds - retention_microseconds;
        
        const char* delete_sql = "DELETE FROM samples WHERE source_timestamp < ?";
        
        sqlite3_stmt* stmt = nullptr;
        int result = sqlite3_prepare_v2(db_, delete_sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            std::cerr << "[DatabaseManager] Failed to prepare cleanup statement: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        sqlite3_bind_int64(stmt, 1, cutoff_time_microseconds);
        
        result = sqlite3_step(stmt);
        int deleted_count = sqlite3_changes(db_);
        sqlite3_finalize(stmt);
        
        if (result == SQLITE_DONE) {
            if (deleted_count > 0) {
                std::cout << "[DatabaseManager] Cleaned up " << deleted_count 
                         << " old samples older than " << retention_time.count() << " hours" << std::endl;
            }
            return true;
        }
        std::cerr << "[DatabaseManager] Failed to cleanup old data: " << sqlite3_errmsg(db_) << std::endl;
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseManager] Exception during cleanup: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::cleanup_topic_data(const std::string& topic_name) {
    if (db_ == nullptr) {
        std::cerr << "[DatabaseManager] Database not initialized" << std::endl;
        return false;
    }
    
    try {
        const char* delete_sql = "DELETE FROM persistent_data WHERE topic_name = ?";
        
        sqlite3_stmt* stmt = nullptr;
        int result = sqlite3_prepare_v2(db_, delete_sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            std::cerr << "[DatabaseManager] Failed to prepare topic cleanup statement: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
        
        result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (result == SQLITE_DONE) {
            return true;
        }
        std::cerr << "[DatabaseManager] Failed to cleanup topic data: " << sqlite3_errmsg(db_) << std::endl;
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseManager] Exception during topic cleanup: " << e.what() << std::endl;
        return false;
    }
}

auto DatabaseManager::get_database_stats() -> std::map<std::string, int64_t> {
    std::map<std::string, int64_t> stats;
    
    if (db_ == nullptr) {
        std::cerr << "[DatabaseManager] Database not initialized" << std::endl;
        return stats;
    }
    
    try {
        // 获取各表的记录数
        std::array<const char*, 2> queries = {
            "SELECT COUNT(*) FROM samples",
            "SELECT COUNT(*) FROM topics"
        };
        
        std::array<const char*, 2> stat_names = {
            "samples_count",
            "topics_count"
        };
        
        constexpr size_t query_count = 2;
        for (size_t i = 0; i < query_count; ++i) {
            sqlite3_stmt* stmt = nullptr;
            int result = sqlite3_prepare_v2(db_, queries[i], -1, &stmt, nullptr);
            if (result == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    stats[stat_names[i]] = sqlite3_column_int64(stmt, 0);
                }
                sqlite3_finalize(stmt);
            } else {
                std::cerr << "[DatabaseManager] Failed to prepare query " << queries[i] 
                         << ": " << sqlite3_errmsg(db_) << std::endl;
            }
        }
        
        // 获取样本数据的时间范围和大小统计
        const char* time_range_sql = R"(
            SELECT 
                MIN(source_timestamp) as earliest_timestamp,
                MAX(source_timestamp) as latest_timestamp,
                COUNT(*) as total_samples,
                SUM(LENGTH(serialized_data)) as total_data_bytes
            FROM samples
        )";
        
        sqlite3_stmt* stmt = nullptr;
        int result = sqlite3_prepare_v2(db_, time_range_sql, -1, &stmt, nullptr);
        if (result == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats["earliest_timestamp_us"] = sqlite3_column_int64(stmt, 0);
                stats["latest_timestamp_us"] = sqlite3_column_int64(stmt, 1);
                stats["total_samples"] = sqlite3_column_int64(stmt, 2);
                stats["total_data_bytes"] = sqlite3_column_int64(stmt, 3);
            }
            sqlite3_finalize(stmt);
        }
        
        // 获取每个topic的样本数量
        const char* per_topic_sql = R"(
            SELECT t.topic_name, COUNT(s.id) as sample_count
            FROM topics t
            LEFT JOIN samples s ON t.id = s.topic_id
            GROUP BY t.id, t.topic_name
        )";
        
        result = sqlite3_prepare_v2(db_, per_topic_sql, -1, &stmt, nullptr);
        if (result == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* topic_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                int64_t sample_count = sqlite3_column_int64(stmt, 1);
                std::string key = std::string("topic_") + topic_name + "_samples";
                stats[key] = sample_count;
            }
            sqlite3_finalize(stmt);
        }
        
        // 获取数据库文件大小
        result = sqlite3_prepare_v2(db_, "PRAGMA page_count", -1, &stmt, nullptr);
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
        std::cerr << "[DatabaseManager] Exception getting database stats: " << e.what() << std::endl;
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

auto DatabaseManager::get_table_names() -> std::vector<std::string> {
    std::vector<std::string> tables;
    if (!db_) return tables;

    std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';";
    auto stmt = prepare_statement(sql);
    if (!stmt) return tables;

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
        if (text) {
            tables.push_back(text);
        }
    }
    return tables;
}

auto DatabaseManager::get_table_content(const std::string& table_name) -> std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>> {
    std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>> result;
    if (!db_ || table_name.empty() || table_name.find(';') != std::string::npos) {
        // 基本的SQL注入防护
        return result;
    }

    std::string sql = "SELECT * FROM " + table_name + " LIMIT 100;"; // 限制最多返回100行
    auto stmt = prepare_statement(sql);
    if (!stmt) return result;

    // 获取表头
    int col_count = sqlite3_column_count(stmt.get());
    for (int i = 0; i < col_count; ++i) {
        const char* column_name = sqlite3_column_name(stmt.get(), i);
        result.first.push_back(column_name ? column_name : "");
    }

    // 获取数据行
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        std::vector<std::string> row;
        for (int i = 0; i < col_count; ++i) {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), i));
            row.push_back(text ? text : "NULL");
        }
        result.second.push_back(row);
    }

    return result;
}

TRAVODDS_NAMESPACE_END