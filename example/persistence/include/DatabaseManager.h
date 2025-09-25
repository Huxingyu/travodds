#pragma once

#include "PersistenceTypes.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include <sqlite3.h>

TRAVODDS_NAMESPACE_BEGIN

/**
 * @brief 数据库配置结构体
 * 
 * 包含SQLite数据库的所有配置参数，如文件路径、保留策略、性能优化选项等。
 */
struct DatabaseConfig {
    std::string database_path{"persistence_data.db"};     ///< 数据库文件路径
    std::chrono::hours data_retention{24};                ///< 数据保留时间（小时）
    size_t max_samples_per_topic = 10000;                 ///< 每个Topic最大样本数
    bool enable_wal_mode = true;                          ///< 启用Write-Ahead Logging模式
    bool enable_foreign_keys = true;                      ///< 启用外键约束
    int connection_timeout_ms = 30000;                    ///< 连接超时时间（毫秒）
};

/**
 * @brief 数据库管理器
 * 
 * 负责SQLite数据库的初始化、数据存储、查询、清理等操作。
 * 支持WAL模式、事务管理、错误恢复等高级功能。
 */
class DatabaseManager {
public:
    /**
     * @brief 构造函数
     * 
     * @param config 数据库配置参数
     */
    explicit DatabaseManager(const DatabaseConfig& config);
    
    /**
     * @brief 析构函数
     * 
     * 自动关闭数据库连接并清理资源。
     */
    ~DatabaseManager();
    
    // 禁用拷贝
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    // 初始化和清理
    /**
     * @brief 初始化数据库
     * 
     * 创建数据库文件、表结构，设置性能优化参数。
     * 
     * @return true 初始化成功，false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 关闭数据库
     * 
     * 安全关闭数据库连接，确保所有事务完成。
     */
    void shutdown();
    
private:
    // 数据库恢复相关方法
    /**
     * @brief 带恢复功能的初始化
     * 
     * 尝试正常初始化，如果失败则尝试恢复数据库。
     * 
     * @return true 初始化成功，false 初始化失败
     */
    bool initialize_with_recovery();
    
    /**
     * @brief 恢复损坏的数据库
     * 
     * 当数据库文件损坏时，尝试恢复或重建数据库。
     * 
     * @return true 恢复成功，false 恢复失败
     */
    bool recover_corrupted_database();
    
public:
    
    // 数据操作
    /**
     * @brief 存储数据到数据库
     * 
     * 将序列化的DDS数据样本持久化到SQLite数据库中。
     * 
     * @param data 要存储的数据，包含序列化数据和元信息
     * @return true 存储成功，false 存储失败
     */
    bool store_data(const StoredData& data);
    
    /**
     * @brief 加载指定Topic的所有数据
     * 
     * @param topic_name Topic名称
     * @return 该Topic的所有存储数据向量
     */
    auto load_data(const std::string& topic_name) -> std::vector<StoredData>;
    
    /**
     * @brief 加载指定时间范围内的数据
     * 
     * @param topic_name Topic名称
     * @param start_time 开始时间
     * @param end_time 结束时间
     * @return 时间范围内的存储数据向量
     */
    auto load_data_range(const std::string& topic_name, 
                        const Time_t& start_time,
                        const Time_t& end_time) -> std::vector<StoredData>;
    
    // Topic管理
    /**
     * @brief 插入或更新Topic信息
     * 
     * 在数据库中维护Topic的元信息，如类型名称、创建时间等。
     * 
     * @param topic_name Topic名称
     * @param type_name 数据类型名称
     * @return true 操作成功，false 操作失败
     */
    bool insert_or_update_topic(const std::string& topic_name, 
                               const std::string& type_name);
    
    /**
     * @brief 更新Topic统计信息
     * 
     * 更新Topic的数据大小、样本数量等统计信息。
     * 
     * @param topic_name Topic名称
     * @param data_size 数据大小（字节）
     * @return true 更新成功，false 更新失败
     */
    bool update_topic_stats(const std::string& topic_name, size_t data_size);
    
    // 清理操作
    /**
     * @brief 清理过期的旧数据
     * 
     * 根据数据保留策略删除过期的数据样本。
     * 
     * @param retention_time 数据保留时间
     * @return true 清理成功，false 清理失败
     */
    bool cleanup_old_data(std::chrono::hours retention_time);
    
    /**
     * @brief 清理指定Topic的所有数据
     * 
     * @param topic_name 要清理的Topic名称
     * @return true 清理成功，false 清理失败
     */
    bool cleanup_topic_data(const std::string& topic_name);
    
    // 统计信息
    /**
     * @brief 获取数据库统计信息
     * 
     * 提供数据库大小、表数量、记录数等统计信息。
     * 
     * @return 统计信息映射表
     */
    auto get_database_stats() -> std::map<std::string, int64_t>;
    
    /**
     * @brief 检查数据库是否已初始化
     * 
     * @return true 已初始化，false 未初始化
     */
    bool is_initialized() const { return db_ != nullptr; }

    // 【新增】获取所有表名
    /**
     * @brief 获取数据库中所有表名
     * 
     * @return 表名称向量
     */
    auto get_table_names() -> std::vector<std::string>;

    // 【新增】获取指定表的通用内容（返回二维字符串数组）
    /**
     * @brief 获取指定表的内容
     * 
     * @param table_name 表名
     * @return 表内容，包含列名向量和数据行向量
     */
    auto get_table_content(const std::string& table_name) -> std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>>;
    
private:
    DatabaseConfig config_;                               ///< 数据库配置
    sqlite3* db_ = nullptr;                              ///< SQLite数据库连接句柄
    
    // 内部方法
    /**
     * @brief 创建数据库表结构
     * 
     * @return true 创建成功，false 创建失败
     */
    bool create_tables();
    
    /**
     * @brief 执行SQL语句
     * 
     * @param sql SQL语句
     * @return true 执行成功，false 执行失败
     */
    bool execute_sql(const std::string& sql);
    
    /**
     * @brief 准备SQL语句
     * 
     * @param sql SQL语句
     * @return 准备好的语句句柄，使用智能指针自动管理
     */
    auto prepare_statement(const std::string& sql) -> std::unique_ptr<struct sqlite3_stmt, void(*)(sqlite3_stmt*)>;
    
    // 辅助方法
    /**
     * @brief 将DDS时间转换为微秒数
     * 
     * @param time DDS时间戳
     * @return 微秒数
     */
    static auto time_to_microseconds(const Time_t& time) -> int64_t;
    
    /**
     * @brief 将微秒数转换为DDS时间
     * 
     * @param microseconds 微秒数
     * @return DDS时间戳
     */
    static auto microseconds_to_time(int64_t microseconds) -> Time_t;
};

TRAVODDS_NAMESPACE_END
