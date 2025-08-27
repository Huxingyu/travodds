#include "PersistenceService.h"
#include "qos/qos.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <ctime>

USING_AGIDDS_NAMESPACE;

// 构造函数更新为使用配置结构
PersistenceService::PersistenceService(DomainParticipant* participant, 
                                     const PersistenceConfig& config)
    : config_(config)
    , stats_()
    , participant_(participant)
    , db_(nullptr)
    , db_path_(config.db_path)
    , running_(false)
{
    if (!initialize_database()) {
        throw std::runtime_error("Failed to initialize persistence database");
    }
    
    std::cout << "PersistenceService created with enhanced configuration:" << std::endl;
    std::cout << "- Database: " << config_.db_path << std::endl;
    std::cout << "- Max samples per topic: " << config_.max_samples_per_topic << std::endl;
    std::cout << "- Worker threads: " << config_.worker_threads << std::endl;
    std::cout << "- Compression: " << (config_.enable_compression ? "enabled" : "disabled") << std::endl;
}

PersistenceService::~PersistenceService() {
    stop();
    if (db_) {
        sqlite3_close(db_);
    }
}

ReturnCode_t PersistenceService::start() {
    if (running_.exchange(true)) {
        return RETCODE_OK; // 已经在运行
    }
    
    std::cout << "Starting Enhanced DDS Persistence Service..." << std::endl;
    
    // 启动工作线程池
    start_worker_threads();
    
    // 启动发现线程
    discovery_thread_ = std::thread(&PersistenceService::discovery_loop, this);
    
    // 启动清理线程
    cleanup_thread_ = std::thread(&PersistenceService::cleanup_loop, this);
    
    stats_.start_time = std::chrono::steady_clock::now();
    
    // 通知UI回调
    notify_ui_callback([](IPersistenceUICallback* callback) {
        callback->on_statistics_updated();
    });
    
    return RETCODE_OK;
}

void PersistenceService::stop() {
    if (!running_.exchange(false)) {
        return; // 已经停止
    }
    
    std::cout << "Stopping Enhanced DDS Persistence Service..." << std::endl;
    
    // 通知所有等待的工作线程
    task_condition_.notify_all();
    
    // 等待工作线程结束
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    
    // 等待发现线程结束
    if (discovery_thread_.joinable()) {
        discovery_thread_.join();
    }
    
    // 等待清理线程结束
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
    
    // 清理DDS实体
    for (auto& [topic, reader] : collectors_) {
        // 这里需要通过Subscriber删除DataReader
    }
    
    for (auto& [topic, writer] : publishers_) {
        // 这里需要通过Publisher删除DataWriter
    }
    
    collectors_.clear();
    publishers_.clear();
    listeners_.clear();
}

// 启动工作线程池
void PersistenceService::start_worker_threads() {
    for (int i = 0; i < config_.worker_threads; ++i) {
        worker_threads_.emplace_back(&PersistenceService::worker_thread_function, this);
    }
    std::cout << "Started " << config_.worker_threads << " worker threads" << std::endl;
}

// 工作线程函数
void PersistenceService::worker_thread_function() {
    while (running_) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(task_queue_mutex_);
            task_condition_.wait(lock, [this] {
                return !task_queue_.empty() || !running_;
            });
            
            if (!running_) break;
            
            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
            }
        }
        
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Worker thread error: " << e.what() << std::endl;
            }
        }
    }
}

// 添加任务到队列
void PersistenceService::enqueue_task(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(task_queue_mutex_);
        task_queue_.push(std::move(task));
    }
    task_condition_.notify_one();
}

bool PersistenceService::initialize_database() {
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    // 启用外键约束
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    
    // 创建基础表结构
    if (!create_enhanced_tables()) {
        return false;
    }
    
    std::cout << "Enhanced database initialized successfully" << std::endl;
    return true;
}

bool PersistenceService::create_enhanced_tables() {
    const char* create_tables_sql = R"(
        -- 主题表
        CREATE TABLE IF NOT EXISTS topics (
            topic_id INTEGER PRIMARY KEY AUTOINCREMENT,
            topic_name TEXT UNIQUE NOT NULL,
            type_name TEXT NOT NULL,
            created_at INTEGER DEFAULT (strftime('%s','now')),
            last_updated INTEGER DEFAULT (strftime('%s','now'))
        );
        
        -- 增强的持久化数据表
        CREATE TABLE IF NOT EXISTS persistent_data (
            data_id INTEGER PRIMARY KEY AUTOINCREMENT,
            topic_id INTEGER REFERENCES topics(topic_id) ON DELETE CASCADE,
            instance_id TEXT NOT NULL,
            sequence_number INTEGER NOT NULL,
            source_timestamp INTEGER NOT NULL,
            reception_timestamp INTEGER NOT NULL,
            data_content BLOB NOT NULL,
            data_size INTEGER NOT NULL,
            is_disposed BOOLEAN DEFAULT 0,
            is_compressed BOOLEAN DEFAULT 0,
            qos_info TEXT,
            storage_timestamp INTEGER DEFAULT (strftime('%s','now')),
            UNIQUE(topic_id, instance_id, sequence_number)
        );
        
        -- 发布者表
        CREATE TABLE IF NOT EXISTS publishers (
            publisher_id INTEGER PRIMARY KEY AUTOINCREMENT,
            participant_guid TEXT NOT NULL,
            writer_guid TEXT UNIQUE NOT NULL,
            topic_id INTEGER REFERENCES topics(topic_id) ON DELETE CASCADE,
            qos_profile TEXT,
            first_seen INTEGER DEFAULT (strftime('%s','now')),
            last_seen INTEGER DEFAULT (strftime('%s','now')),
            is_active BOOLEAN DEFAULT 1
        );
        
        -- 订阅者表
        CREATE TABLE IF NOT EXISTS subscribers (
            subscriber_id INTEGER PRIMARY KEY AUTOINCREMENT,
            participant_guid TEXT NOT NULL,
            reader_guid TEXT UNIQUE NOT NULL,
            topic_id INTEGER REFERENCES topics(topic_id) ON DELETE CASCADE,
            qos_profile TEXT,
            first_seen INTEGER DEFAULT (strftime('%s','now')),
            last_seen INTEGER DEFAULT (strftime('%s','now')),
            is_active BOOLEAN DEFAULT 1
        );
        
        -- 统计信息表
        CREATE TABLE IF NOT EXISTS statistics (
            stat_id INTEGER PRIMARY KEY AUTOINCREMENT,
            topic_id INTEGER REFERENCES topics(topic_id) ON DELETE CASCADE,
            samples_stored INTEGER DEFAULT 0,
            samples_served INTEGER DEFAULT 0,
            bytes_stored INTEGER DEFAULT 0,
            bytes_served INTEGER DEFAULT 0,
            last_updated INTEGER DEFAULT (strftime('%s','now'))
        );
        
        -- 索引优化
        CREATE INDEX IF NOT EXISTS idx_persistent_data_topic_time 
        ON persistent_data(topic_id, source_timestamp);
        
        CREATE INDEX IF NOT EXISTS idx_persistent_data_instance 
        ON persistent_data(topic_id, instance_id, sequence_number);
        
        CREATE INDEX IF NOT EXISTS idx_persistent_data_storage_time 
        ON persistent_data(storage_timestamp);
        
        CREATE INDEX IF NOT EXISTS idx_publishers_active 
        ON publishers(topic_id, is_active);
        
        CREATE INDEX IF NOT EXISTS idx_subscribers_active 
        ON subscribers(topic_id, is_active);
    )";
    
    char* error_msg = nullptr;
    int rc = sqlite3_exec(db_, create_tables_sql, nullptr, nullptr, &error_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error creating tables: " << error_msg << std::endl;
        sqlite3_free(error_msg);
        return false;
    }
    
    return true;
}

void PersistenceService::discovery_loop() {
    std::cout << "Discovery loop started" << std::endl;
    
    while (running_) {
        try {
            // 获取当前网络中的DataWriter列表
            InstanceHandleSeq publication_handles;
            ReturnCode_t ret = participant_->get_publications(publication_handles, false);
            
            if (ret == RETCODE_OK) {
                for (const auto& handle : publication_handles) {
                    PublicationBuiltinTopicData writer_data;
                    ret = participant_->get_publication_data(writer_data, handle);
                    
                    if (ret == RETCODE_OK) {
                        handle_discovered_writer(writer_data);
                    }
                }
            }
            
            // 检查新的DataReader
            InstanceHandleSeq subscription_handles;
            ret = participant_->get_subscriptions(subscription_handles, false);
            
            if (ret == RETCODE_OK) {
                for (const auto& handle : subscription_handles) {
                    SubscriptionBuiltinTopicData reader_data;
                    ret = participant_->get_subscription_data(reader_data, handle);
                    
                    if (ret == RETCODE_OK) {
                        handle_new_reader(reader_data);
                    }
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Discovery loop error: " << e.what() << std::endl;
        }
        
        // 等待5秒后再次检查
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    std::cout << "Discovery loop stopped" << std::endl;
}

void PersistenceService::handle_discovered_writer(const PublicationBuiltinTopicData& writer_data) {
    // 检查是否是持久化QoS
    if (!is_persistent_qos(writer_data)) {
        return;
    }
    
    std::string topic_name = writer_data.topic_name.data();
    std::string type_name = writer_data.type_name.data();
    
    // 检查是否已经为该Topic创建了DataReader
    if (collectors_.find(topic_name) != collectors_.end()) {
        return; // 已经存在
    }
    
    std::cout << "Discovered persistent writer for topic: " << topic_name 
              << " type: " << type_name << std::endl;
    
    try {
        // 创建Topic
        TopicQos topic_qos = TOPIC_QOS_DEFAULT;
        Topic* topic = participant_->create_topic(
            topic_name, type_name, topic_qos, nullptr, STATUS_MASK_NONE);
        
        if (!topic) {
            std::cerr << "Failed to create topic: " << topic_name << std::endl;
            return;
        }
        
        // 创建Subscriber
        SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
        Subscriber* subscriber = participant_->create_subscriber(
            sub_qos, nullptr, STATUS_MASK_NONE);
        
        if (!subscriber) {
            std::cerr << "Failed to create subscriber" << std::endl;
            return;
        }
        
        // 创建DataReader用于接收持久化数据
        DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
        
        // 设置持久化QoS
        reader_qos.durability.kind = TRANSIENT_DURABILITY_QOS;
        reader_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        reader_qos.history.kind = KEEP_ALL_HISTORY_QOS;
        
        // 创建监听器并存储到类成员中
        auto listener = std::make_shared<PersistenceDataListener>(this, topic_name);
        listeners_[topic_name] = listener;  // 存储监听器，防止被销毁
        
        DataReader* reader = subscriber->create_datareader(
            topic, reader_qos, listener.get(), STATUS_MASK_ALL);
        
        if (reader) {
            collectors_[topic_name] = reader;
            std::cout << "Created DataReader for persistent topic: " << topic_name << std::endl;
        } else {
            std::cerr << "Failed to create DataReader for topic: " << topic_name << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating DataReader: " << e.what() << std::endl;
    }
}

bool PersistenceService::is_persistent_qos(const PublicationBuiltinTopicData& data) const {
    // 检查Durability QoS策略
    return (data.durability.kind == TRANSIENT_DURABILITY_QOS || 
            data.durability.kind == PERSISTENT_DURABILITY_QOS);
}

void PersistenceService::handle_new_reader(const SubscriptionBuiltinTopicData& reader_data) {
    std::string topic_name = reader_data.topic_name.data();
    
    // 检查是否是持久化QoS的DataReader
    if (reader_data.durability.kind == TRANSIENT_DURABILITY_QOS || 
        reader_data.durability.kind == PERSISTENT_DURABILITY_QOS) {
        
        std::cout << "New persistent reader detected for topic: " << topic_name << std::endl;
        
        // 发送历史数据
        send_historical_data(topic_name, reader_data);
    }
}

void PersistenceService::send_historical_data(const std::string& topic_name,
                                            const SubscriptionBuiltinTopicData& reader_data) {
    // 查询历史数据
    auto historical_data = query_historical_data(topic_name);
    
    if (historical_data.empty()) {
        std::cout << "No historical data found for topic: " << topic_name << std::endl;
        return;
    }
    
    std::cout << "Sending " << historical_data.size() 
              << " historical samples for topic: " << topic_name << std::endl;
    
    // 这里需要根据具体的数据类型来创建DataWriter并发送数据
    // 实际实现中需要使用TypeSupport来处理不同的数据类型
}

// 增强版存储数据函数
bool PersistenceService::store_data(const std::string& topic_name,
                                   const std::string& type_name,
                                   const std::string& instance_id,
                                   const void* data,
                                   size_t data_size,
                                   const SampleInfo& info) {
    
    // 首先检查数据库大小限制
    if (!check_database_size_limit()) {
        std::cerr << "Database size limit exceeded" << std::endl;
        return false;
    }
    
    // 压缩数据（如果启用）
    std::vector<uint8_t> final_data;
    bool is_compressed = false;
    
    if (config_.enable_compression && data_size > 1024) { // 大于1KB的数据进行压缩
        std::vector<uint8_t> raw_data(static_cast<const uint8_t*>(data), 
                                     static_cast<const uint8_t*>(data) + data_size);
        final_data = compress_data(raw_data);
        is_compressed = true;
    } else {
        final_data.assign(static_cast<const uint8_t*>(data), 
                         static_cast<const uint8_t*>(data) + data_size);
    }
    
    const char* insert_topic_sql = "INSERT OR IGNORE INTO topics (topic_name, type_name) VALUES (?, ?)";
    const char* insert_data_sql = R"(
        INSERT OR REPLACE INTO persistent_data 
        (topic_id, instance_id, sequence_number, source_timestamp, reception_timestamp, 
         data_content, data_size, is_compressed, qos_info)
        VALUES (
            (SELECT topic_id FROM topics WHERE topic_name = ?),
            ?, ?, ?, ?, ?, ?, ?, ?
        )
    )";
    
    // 首先插入topic信息
    sqlite3_stmt* topic_stmt;
    int result_code = sqlite3_prepare_v2(db_, insert_topic_sql, -1, &topic_stmt, nullptr);
    
    if (result_code != SQLITE_OK) {
        std::cerr << "Failed to prepare topic statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(topic_stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(topic_stmt, 2, type_name.c_str(), -1, SQLITE_STATIC);
    
    result_code = sqlite3_step(topic_stmt);
    sqlite3_finalize(topic_stmt);
    
    if (result_code != SQLITE_DONE) {
        std::cerr << "Failed to insert topic: " << sqlite3_errmsg(db_) << std::endl;
        // 继续执行，可能topic已经存在
    }
    
    // 然后插入数据
    sqlite3_stmt* data_stmt;
    result_code = sqlite3_prepare_v2(db_, insert_data_sql, -1, &data_stmt, nullptr);
    
    if (result_code != SQLITE_OK) {
        std::cerr << "Failed to prepare data statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    // 绑定数据参数
    sqlite3_bind_text(data_stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(data_stmt, 2, instance_id.c_str(), -1, SQLITE_STATIC);
    
    // 使用源时间戳作为序列号
    int64_t sequence_num = info.source_timestamp.sec * 1000000LL + info.source_timestamp.nanosec / 1000;
    sqlite3_bind_int64(data_stmt, 3, sequence_num);
    sqlite3_bind_int64(data_stmt, 4, sequence_num);
    
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    sqlite3_bind_int64(data_stmt, 5, now);
    
    sqlite3_bind_blob(data_stmt, 6, final_data.data(), final_data.size(), SQLITE_STATIC);
    sqlite3_bind_int64(data_stmt, 7, final_data.size());
    sqlite3_bind_int(data_stmt, 8, is_compressed ? 1 : 0);
    sqlite3_bind_text(data_stmt, 9, "default_qos", -1, SQLITE_STATIC);
    
    result_code = sqlite3_step(data_stmt);
    sqlite3_finalize(data_stmt);
    
    if (result_code != SQLITE_DONE) {
        std::cerr << "Failed to insert data: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    // 更新统计信息
    update_statistics(topic_name, data_size, true);
    
    // 通知UI回调
    notify_ui_callback([topic_name, data_size](IPersistenceUICallback* callback) {
        callback->on_data_stored(topic_name, data_size);
    });
    
    return true;
}

// 查询历史数据 - 增强版本
std::vector<StoredData> PersistenceService::query_historical_data(
    const std::string& topic_name,
    const Time_t& start_time,
    const Time_t& end_time) {
    
    std::vector<StoredData> result;
    
    const char* query_sql = R"(
        SELECT t.topic_name, t.type_name, p.instance_id, p.data_content, 
               p.source_timestamp, p.reception_timestamp, p.sequence_number,
               p.is_disposed, p.is_compressed, p.data_size
        FROM persistent_data p
        JOIN topics t ON p.topic_id = t.topic_id
        WHERE t.topic_name = ? 
        AND p.source_timestamp >= ? 
        AND p.source_timestamp <= ?
        ORDER BY p.sequence_number
    )";
    
    sqlite3_stmt* stmt;
    int result_code = sqlite3_prepare_v2(db_, query_sql, -1, &stmt, nullptr);
    
    if (result_code != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db_) << std::endl;
        return result;
    }
    
    const int64_t MICROSECONDS_PER_SECOND = 1000000LL;
    const int64_t NANOSECONDS_PER_MICROSECOND = 1000;
    
    sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, start_time.sec * MICROSECONDS_PER_SECOND + 
                      start_time.nanosec / NANOSECONDS_PER_MICROSECOND);
    sqlite3_bind_int64(stmt, 3, end_time.sec * MICROSECONDS_PER_SECOND + 
                      end_time.nanosec / NANOSECONDS_PER_MICROSECOND);
    
    while ((result_code = sqlite3_step(stmt)) == SQLITE_ROW) {
        StoredData data;
        data.topic_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        data.type_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        data.instance_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        const void* blob_data = sqlite3_column_blob(stmt, 3);
        int blob_size = sqlite3_column_bytes(stmt, 3);
        
        // 处理压缩数据
        bool is_compressed = sqlite3_column_int(stmt, 8) != 0;
        if (is_compressed) {
            std::vector<uint8_t> compressed_data(static_cast<const uint8_t*>(blob_data),
                                               static_cast<const uint8_t*>(blob_data) + blob_size);
            data.data = decompress_data(compressed_data);
        } else {
            data.data.resize(blob_size);
            memcpy(data.data.data(), blob_data, blob_size);
        }
        
        int64_t source_ts = sqlite3_column_int64(stmt, 4);
        data.sample_info.source_timestamp.sec = source_ts / MICROSECONDS_PER_SECOND;
        data.sample_info.source_timestamp.nanosec = (source_ts % MICROSECONDS_PER_SECOND) * NANOSECONDS_PER_MICROSECOND;
        
        data.sequence_number = sqlite3_column_int64(stmt, 6);
        data.is_disposed = sqlite3_column_int(stmt, 7) != 0;
        
        // 设置存储时间戳
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds);
        
        data.storage_time.sec = static_cast<uint32_t>(seconds.count());
        data.storage_time.nanosec = static_cast<uint32_t>(nanoseconds.count());
        
        result.push_back(data);
    }
    
    sqlite3_finalize(stmt);
    
    // 更新统计信息
    update_statistics(topic_name, result.size(), false);
    
    return result;
}

// 网络拓扑查询
NetworkTopology PersistenceService::get_network_topology() const {
    NetworkTopology topology;
    
    // 获取参与者信息
    InstanceHandleSeq participant_handles;
    if (participant_->get_discovered_participants(participant_handles) == RETCODE_OK) {
        for (const auto& handle : participant_handles) {
            ParticipantBuiltinTopicData data;
            if (participant_->get_discovered_participant_data(data, handle) == RETCODE_OK) {
                topology.participants.push_back(data);
            }
        }
    }
    
    // 获取DataWriter信息
    InstanceHandleSeq publication_handles;
    if (participant_->get_publications(publication_handles, false) == RETCODE_OK) {
        for (const auto& handle : publication_handles) {
            PublicationBuiltinTopicData data;
            if (participant_->get_publication_data(data, handle) == RETCODE_OK) {
                topology.writers.push_back(data);
            }
        }
    }
    
    // 获取DataReader信息
    InstanceHandleSeq subscription_handles;
    if (participant_->get_subscriptions(subscription_handles, false) == RETCODE_OK) {
        for (const auto& handle : subscription_handles) {
            SubscriptionBuiltinTopicData data;
            if (participant_->get_subscription_data(data, handle) == RETCODE_OK) {
                topology.readers.push_back(data);
            }
        }
    }
    
    return topology;
}

// 清理过期数据 - 增强版本
void PersistenceService::cleanup_expired_data() {
    std::cout << "Starting cleanup of expired data..." << std::endl;
    
    // 使用配置的数据保留期限
    int retention_hours = static_cast<int>(config_.data_retention.count());
    
    std::string delete_sql = "DELETE FROM persistent_data WHERE storage_timestamp < strftime('%s','now','-" + 
                           std::to_string(retention_hours) + " hours')";
    
    char* error_msg = nullptr;
    int result_code = sqlite3_exec(db_, delete_sql.c_str(), nullptr, nullptr, &error_msg);
    
    if (result_code != SQLITE_OK) {
        std::cerr << "Failed to cleanup expired data: " << error_msg << std::endl;
        sqlite3_free(error_msg);
    } else {
        int deleted = sqlite3_changes(db_);
        if (deleted > 0) {
            std::cout << "Cleaned up " << deleted << " expired data records" << std::endl;
        }
    }
    
    // 清理超出样本数量限制的数据
    const char* cleanup_excess_sql = R"(
        DELETE FROM persistent_data 
        WHERE data_id IN (
            SELECT data_id FROM persistent_data p1
            WHERE (
                SELECT COUNT(*) FROM persistent_data p2 
                WHERE p2.topic_id = p1.topic_id 
                AND p2.sequence_number >= p1.sequence_number
            ) > ?
        )
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, cleanup_excess_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, config_.max_samples_per_topic);
        
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            int deleted_excess = sqlite3_changes(db_);
            if (deleted_excess > 0) {
                std::cout << "Cleaned up " << deleted_excess << " excess samples" << std::endl;
            }
        }
        sqlite3_finalize(stmt);
    }
}

void PersistenceService::cleanup_loop() {
    while (running_) {
        cleanup_expired_data();
        
        // 使用配置的清理间隔
        std::this_thread::sleep_for(config_.cleanup_interval);
    }
}

// 实用函数实现
void PersistenceService::update_statistics(const std::string& topic_name, size_t data_size, bool is_store) {
    if (!config_.enable_statistics) {
        return;
    }
    
    if (is_store) {
        stats_.total_samples_stored++;
        stats_.total_bytes_stored += data_size;
    } else {
        stats_.total_samples_served++;
        stats_.total_bytes_served += data_size;
    }
    
    // 更新Topic级别统计
    {
        std::lock_guard<std::mutex> lock(stats_.topic_stats_mutex);
        if (is_store) {
            stats_.samples_per_topic[topic_name]++;
            stats_.bytes_per_topic[topic_name] += data_size;
        }
    }
}

void PersistenceService::notify_ui_callback(std::function<void(IPersistenceUICallback*)> callback_func) {
    if (ui_callback_) {
        enqueue_task([callback_func, cb = ui_callback_]() {
            callback_func(cb.get());
        });
    }
}

bool PersistenceService::check_database_size_limit() {
    const char* size_query = "SELECT page_count * page_size as size FROM pragma_page_count(), pragma_page_size();";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, size_query, -1, &stmt, nullptr) != SQLITE_OK) {
        return true; // 如果查询失败，假设没有超出限制
    }
    
    bool within_limit = true;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t db_size_bytes = sqlite3_column_int64(stmt, 0);
        int64_t limit_bytes = config_.max_db_size_mb * 1024 * 1024;
        within_limit = db_size_bytes < limit_bytes;
        
        if (!within_limit) {
            std::cerr << "Database size (" << db_size_bytes / (1024*1024) 
                     << "MB) exceeds limit (" << config_.max_db_size_mb << "MB)" << std::endl;
        }
    }
    
    sqlite3_finalize(stmt);
    return within_limit;
}

// 压缩相关函数（简单实现，可以后续集成zlib等）
std::vector<uint8_t> PersistenceService::compress_data(const std::vector<uint8_t>& data) const {
    // 简单实现：这里可以集成zlib、lz4等压缩库
    // 目前返回原数据，标记为"已压缩"但实际未压缩
    return data;
}

std::vector<uint8_t> PersistenceService::decompress_data(const std::vector<uint8_t>& compressed_data) const {
    // 简单实现：对应compress_data的解压缩
    return compressed_data;
}

// 新增的公有方法实现
std::vector<StoredData> PersistenceService::query_instance_data(
    const std::string& topic_name,
    const std::string& instance_id) {
    
    std::vector<StoredData> result;
    
    const char* query_sql = R"(
        SELECT t.topic_name, t.type_name, p.instance_id, p.data_content, 
               p.source_timestamp, p.sequence_number, p.is_disposed, p.is_compressed
        FROM persistent_data p
        JOIN topics t ON p.topic_id = t.topic_id
        WHERE t.topic_name = ? AND p.instance_id = ?
        ORDER BY p.sequence_number
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, query_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare instance query: " << sqlite3_errmsg(db_) << std::endl;
        return result;
    }
    
    sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, instance_id.c_str(), -1, SQLITE_STATIC);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        StoredData data;
        data.topic_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        data.type_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        data.instance_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        const void* blob_data = sqlite3_column_blob(stmt, 3);
        int blob_size = sqlite3_column_bytes(stmt, 3);
        
        bool is_compressed = sqlite3_column_int(stmt, 7) != 0;
        if (is_compressed) {
            std::vector<uint8_t> compressed_data(static_cast<const uint8_t*>(blob_data),
                                               static_cast<const uint8_t*>(blob_data) + blob_size);
            data.data = decompress_data(compressed_data);
        } else {
            data.data.resize(blob_size);
            memcpy(data.data.data(), blob_data, blob_size);
        }
        
        int64_t source_ts = sqlite3_column_int64(stmt, 4);
        const int64_t MICROSECONDS_PER_SECOND = 1000000LL;
        const int64_t NANOSECONDS_PER_MICROSECOND = 1000;
        
        data.sample_info.source_timestamp.sec = source_ts / MICROSECONDS_PER_SECOND;
        data.sample_info.source_timestamp.nanosec = (source_ts % MICROSECONDS_PER_SECOND) * NANOSECONDS_PER_MICROSECOND;
        
        data.sequence_number = sqlite3_column_int64(stmt, 5);
        data.is_disposed = sqlite3_column_int(stmt, 6) != 0;
        
        result.push_back(data);
    }
    
    sqlite3_finalize(stmt);
    return result;
}

std::vector<std::string> PersistenceService::get_active_topics() const {
    std::vector<std::string> topics;
    
    const char* query_sql = "SELECT DISTINCT topic_name FROM topics ORDER BY topic_name";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, query_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare topics query: " << sqlite3_errmsg(db_) << std::endl;
        return topics;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        topics.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    
    sqlite3_finalize(stmt);
    return topics;
}

bool PersistenceService::cleanup_topic_data(const std::string& topic_name) {
    const char* delete_sql = R"(
        DELETE FROM persistent_data 
        WHERE topic_id = (SELECT topic_id FROM topics WHERE topic_name = ?)
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, delete_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare topic cleanup: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    int deleted = sqlite3_changes(db_);
    
    sqlite3_finalize(stmt);
    
    if (success && deleted > 0) {
        std::cout << "Cleaned up " << deleted << " samples for topic: " << topic_name << std::endl;
    }
    
    return success;
}

std::string PersistenceService::export_data_json(const std::string& topic_name) const {
    // 简单的JSON导出实现，便于Qt显示
    std::ostringstream json;
    json << "{\n";
    json << "  \"topic\": \"" << topic_name << "\",\n";
    json << "  \"timestamp\": " << std::time(nullptr) << ",\n";
    json << "  \"statistics\": {\n";
    json << "    \"total_samples\": " << stats_.total_samples_stored.load() << ",\n";
    json << "    \"total_bytes\": " << stats_.total_bytes_stored.load() << ",\n";
    json << "    \"uptime_seconds\": " << std::fixed << std::setprecision(2) << stats_.get_uptime_seconds() << "\n";
    json << "  }\n";
    json << "}";
    
    return json.str();
}

// DataListener实现 - 通用数据处理版本
void PersistenceService::PersistenceDataListener::on_data_available(DataReader* reader) {
    std::cout << "Data listener: Processing data for topic: " << topic_name_ << std::endl;
    
    // 定义默认数据大小常量
    const size_t DEFAULT_DATA_SIZE = 256;
    const size_t HELLO_WORLD_DATA_SIZE = 64;
    const size_t STRING_DATA_SIZE = 512;
    
    try {
        // 使用read_next_sample方法逐个读取样本
        // 这种方法更通用，不需要具体的数据类型信息
        std::vector<uint8_t> sample_buffer(DEFAULT_DATA_SIZE); // 创建缓冲区
        SampleInfo sample_info;
        
        int samples_processed = 0;
        ReturnCode_t ret;
        
        // 循环读取所有可用样本
        do {
            // 读取下一个样本
            ret = reader->take_next_sample(sample_buffer.data(), sample_info);
            
            if (ret == RETCODE_OK && sample_info.valid_data) {
                samples_processed++;
                
                // 生成实例ID（使用instance_handle转换为字符串）
                std::ostringstream instance_id_stream;
                instance_id_stream << "instance_" << std::hex << sample_info.instance_handle.isValid;
                // 还可以添加keyHash的一些字节作为更好的唯一性
                if (sample_info.instance_handle.keyHash.length > 0) {
                    for (int j = 0; j < std::min(4, static_cast<int>(sample_info.instance_handle.keyHash.length)); ++j) {
                        instance_id_stream << "_" << std::hex << static_cast<unsigned>(sample_info.instance_handle.keyHash.value[j]);
                    }
                }
                std::string instance_id = instance_id_stream.str();
                
                // 获取数据类型名称（从Topic获取）
                std::string type_name = "UnknownType"; // 默认类型名
                
                // 根据主题名称估算数据大小
                size_t estimated_data_size = DEFAULT_DATA_SIZE;
                if (topic_name_.find("HelloWorld") != std::string::npos) {
                    estimated_data_size = HELLO_WORLD_DATA_SIZE;
                } else if (topic_name_.find("String") != std::string::npos) {
                    estimated_data_size = STRING_DATA_SIZE;
                }
                
                std::cout << "Processing sample " << samples_processed 
                          << " - Instance: " << instance_id 
                          << ", Size: " << estimated_data_size << " bytes" << std::endl;
                
                // 复制数据到持久缓冲区（用于异步处理）
                std::vector<uint8_t> persistent_data(sample_buffer.begin(), 
                                                   sample_buffer.begin() + estimated_data_size);
                
                // 异步存储数据到数据库
                service_->enqueue_task([service = service_, topic_name = topic_name_, 
                                      type_name, instance_id, 
                                      data = std::move(persistent_data), sample_info]() {
                    try {
                        bool success = service->store_data(
                            topic_name, type_name, instance_id, 
                            data.data(), data.size(), sample_info
                        );
                        
                        if (success) {
                            std::cout << "Successfully stored data for topic: " << topic_name 
                                      << ", instance: " << instance_id << std::endl;
                        } else {
                            std::cerr << "Failed to store data for topic: " << topic_name 
                                      << ", instance: " << instance_id << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error storing data: " << e.what() << std::endl;
                    }
                });
                
                // 更新统计信息
                service_->stats_.total_samples_stored++;
                
                // 通知UI回调
                service_->notify_ui_callback([topic_name = topic_name_, 
                                            estimated_data_size](IPersistenceUICallback* callback) {
                    callback->on_data_stored(topic_name, estimated_data_size);
                });
                
            } else if (ret == RETCODE_NO_DATA) {
                // 没有更多数据，正常退出循环
                break;
            } else {
                // 其他错误
                std::cerr << "Error reading sample from topic " << topic_name_ 
                          << ", return code: " << ret << std::endl;
                break;
            }
            
        } while (ret == RETCODE_OK);
        
        if (samples_processed > 0) {
            std::cout << "Processed " << samples_processed << " samples for topic: " << topic_name_ << std::endl;
        } else {
            std::cout << "No valid data samples available for topic: " << topic_name_ << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in data listener for topic " << topic_name_ << ": " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in data listener for topic: " << topic_name_ << std::endl;
    }
}
