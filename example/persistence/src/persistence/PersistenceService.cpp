#include "PersistenceService.h"
#include "qos/qos.h"
#include <chrono>
#include <iostream>

USING_AGIDDS_NAMESPACE;

PersistenceService::PersistenceService(DomainParticipant* participant, const std::string& db_path)
    : participant_(participant)
    , db_(nullptr)
    , db_path_(db_path)
    , running_(false)
{
    if (!initialize_database()) {
        throw std::runtime_error("Failed to initialize persistence database");
    }
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
    
    std::cout << "Starting DDS Persistence Service..." << std::endl;
    
    // 启动发现线程
    discovery_thread_ = std::thread(&PersistenceService::discovery_loop, this);
    
    // 启动清理线程
    cleanup_thread_ = std::thread(&PersistenceService::cleanup_loop, this);
    
    return RETCODE_OK;
}

void PersistenceService::stop() {
    if (!running_.exchange(false)) {
        return; // 已经停止
    }
    
    std::cout << "Stopping DDS Persistence Service..." << std::endl;
    
    // 等待线程结束
    if (discovery_thread_.joinable()) {
        discovery_thread_.join();
    }
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
    
    // 清理DataReader和DataWriter
    for (auto& [topic, reader] : collectors_) {
        // 这里需要通过Subscriber删除DataReader
        // reader的删除需要通过其parent subscriber
    }
    
    for (auto& [topic, writer] : publishers_) {
        // 这里需要通过Publisher删除DataWriter
        // writer的删除需要通过其parent publisher
    }
    
    collectors_.clear();
    publishers_.clear();
    listeners_.clear();  // 清理监听器
}

bool PersistenceService::initialize_database() {
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    // 创建表结构
    const char* create_tables_sql = R"(
        CREATE TABLE IF NOT EXISTS topics (
            topic_id INTEGER PRIMARY KEY AUTOINCREMENT,
            topic_name TEXT UNIQUE NOT NULL,
            type_name TEXT NOT NULL,
            created_at INTEGER DEFAULT (strftime('%s','now'))
        );
        
        CREATE TABLE IF NOT EXISTS persistent_data (
            data_id INTEGER PRIMARY KEY AUTOINCREMENT,
            topic_id INTEGER REFERENCES topics(topic_id),
            instance_handle TEXT,
            source_timestamp INTEGER,
            reception_timestamp INTEGER,
            data_content BLOB,
            qos_info TEXT,
            storage_timestamp INTEGER DEFAULT (strftime('%s','now'))
        );
        
        CREATE INDEX IF NOT EXISTS idx_topic_timestamp 
        ON persistent_data(topic_id, source_timestamp);
        
        CREATE TABLE IF NOT EXISTS publishers (
            publisher_id INTEGER PRIMARY KEY AUTOINCREMENT,
            participant_guid TEXT,
            writer_guid TEXT,
            topic_id INTEGER REFERENCES topics(topic_id),
            qos_profile TEXT,
            last_seen INTEGER DEFAULT (strftime('%s','now'))
        );
    )";
    
    char* error_msg = nullptr;
    rc = sqlite3_exec(db_, create_tables_sql, nullptr, nullptr, &error_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << error_msg << std::endl;
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

bool PersistenceService::store_data(const std::string& topic_name,
                                   const std::string& type_name,
                                   const void* data,
                                   size_t data_size,
                                   const SampleInfo& info) {
    
    const char* insert_sql = R"(
        INSERT OR IGNORE INTO topics (topic_name, type_name) VALUES (?, ?);
        INSERT INTO persistent_data 
        (topic_id, instance_handle, source_timestamp, reception_timestamp, data_content)
        VALUES (
            (SELECT topic_id FROM topics WHERE topic_name = ?),
            ?, ?, ?, ?
        );
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, insert_sql, -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    // 绑定参数
    sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, type_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, topic_name.c_str(), -1, SQLITE_STATIC);
    
    // 将InstanceHandle转换为字符串
    // 将InstanceHandle_t转换为字符串
    char handle_str[64];
    std::string handle_hex;
    for (int i = 0; i < info.instance_handle.keyHash.length && i < 8; ++i) {
        char byte_str[3];
        snprintf(byte_str, sizeof(byte_str), "%02X", info.instance_handle.keyHash.value[i]);
        handle_hex += byte_str;
    }
    sqlite3_bind_text(stmt, 4, handle_hex.c_str(), -1, SQLITE_TRANSIENT);
    
    sqlite3_bind_int64(stmt, 5, info.source_timestamp.sec * 1000000LL + info.source_timestamp.nanosec / 1000);
    sqlite3_bind_int64(stmt, 6, std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
    sqlite3_bind_blob(stmt, 7, data, data_size, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to insert data: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    return true;
}

std::vector<PersistenceService::StoredData> PersistenceService::query_historical_data(
    const std::string& topic_name,
    const Time_t& start_time,
    const Time_t& end_time) {
    
    std::vector<StoredData> result;
    
    const char* query_sql = R"(
        SELECT t.topic_name, t.type_name, p.data_content, 
               p.source_timestamp, p.reception_timestamp
        FROM persistent_data p
        JOIN topics t ON p.topic_id = t.topic_id
        WHERE t.topic_name = ? 
        AND p.source_timestamp >= ? 
        AND p.source_timestamp <= ?
        ORDER BY p.source_timestamp
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, query_sql, -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db_) << std::endl;
        return result;
    }
    
    sqlite3_bind_text(stmt, 1, topic_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, start_time.sec * 1000000LL + start_time.nanosec / 1000);
    sqlite3_bind_int64(stmt, 3, end_time.sec * 1000000LL + end_time.nanosec / 1000);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        StoredData data;
        data.topic_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        data.type_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        const void* blob_data = sqlite3_column_blob(stmt, 2);
        int blob_size = sqlite3_column_bytes(stmt, 2);
        data.data.resize(blob_size);
        memcpy(data.data.data(), blob_data, blob_size);
        
        int64_t source_ts = sqlite3_column_int64(stmt, 3);
        data.sample_info.source_timestamp.sec = source_ts / 1000000;
        data.sample_info.source_timestamp.nanosec = (source_ts % 1000000) * 1000;
        
        int64_t reception_ts = sqlite3_column_int64(stmt, 4);
        // 注意：SampleInfo中没有reception_timestamp字段，这里省略
        // data.sample_info.reception_timestamp.sec = reception_ts / 1000000;
        // data.sample_info.reception_timestamp.nanosec = (reception_ts % 1000000) * 1000;
        
        result.push_back(data);
    }
    
    sqlite3_finalize(stmt);
    return result;
}

PersistenceService::NetworkTopology PersistenceService::get_network_topology() const {
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

void PersistenceService::cleanup_expired_data() {
    // 删除30天前的数据
    const char* delete_sql = R"(
        DELETE FROM persistent_data 
        WHERE storage_timestamp < strftime('%s','now','-30 days')
    )";
    
    char* error_msg = nullptr;
    int rc = sqlite3_exec(db_, delete_sql, nullptr, nullptr, &error_msg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to cleanup expired data: " << error_msg << std::endl;
        sqlite3_free(error_msg);
    } else {
        int deleted = sqlite3_changes(db_);
        if (deleted > 0) {
            std::cout << "Cleaned up " << deleted << " expired data records" << std::endl;
        }
    }
}

void PersistenceService::cleanup_loop() {
    while (running_) {
        cleanup_expired_data();
        
        // 每小时清理一次
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
}

// DataListener实现
void PersistenceService::PersistenceDataListener::on_data_available(DataReader* reader) {
    std::cout << "Data available for topic: " << topic_name_ << std::endl;
    
    try {
        // 对于HelloWorld类型的特殊处理
        if (topic_name_ == "HelloWorldTopic") {
            // 暂时只记录数据到达，不进行实际读取
            // 避免因为类型不匹配导致的段错误
            std::cout << "HelloWorld data detected, but not processed (avoiding type mismatch)" << std::endl;
            return;
        }
        
        // 对于其他类型，也暂时只记录
        std::cout << "Data detected for topic: " << topic_name_ << " (not processed)" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error in data listener: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error in data listener" << std::endl;
    }
}
