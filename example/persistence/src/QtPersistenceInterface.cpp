#include "QtPersistenceInterface.h"
#include <algorithm>
#include <chrono>

USING_AGIDDS_NAMESPACE;

QtPersistenceInterface::QtPersistenceInterface(std::shared_ptr<PersistenceService> service)
    : persistence_service_(std::move(service))
{
    // 将自己注册为持久化服务的UI回调
    if (persistence_service_) {
        persistence_service_->set_ui_callback(shared_from_this());
    }
}

void QtPersistenceInterface::on_writer_discovered(const std::string& topic, const std::string& type) {
    // 更新网络信息缓存
    update_network_info();
    
    // 调用Qt回调
    if (writer_discovered_cb_) {
        writer_discovered_cb_(topic, type);
    }
}

void QtPersistenceInterface::on_reader_discovered(const std::string& topic, const std::string& type) {
    // 更新网络信息缓存
    update_network_info();
    
    // 调用Qt回调
    if (reader_discovered_cb_) {
        reader_discovered_cb_(topic, type);
    }
}

void QtPersistenceInterface::on_data_stored(const std::string& topic, size_t data_size) {
    // 更新网络信息缓存中的数据统计
    {
        std::lock_guard<std::mutex> lock(network_info_mutex_);
        
        auto it = std::find_if(cached_network_info_.topics.begin(), 
                              cached_network_info_.topics.end(),
                              [&topic](const TopicInfo& info) {
                                  return info.name == topic;
                              });
        
        if (it != cached_network_info_.topics.end()) {
            it->sample_count++;
            it->total_bytes += data_size;
            it->last_activity = std::chrono::system_clock::now();
        }
    }
    
    // 调用Qt回调
    if (data_stored_cb_) {
        data_stored_cb_(topic, data_size);
    }
}

void QtPersistenceInterface::on_historical_data_sent(const std::string& topic, size_t count) {
    // 调用Qt回调
    if (historical_data_sent_cb_) {
        historical_data_sent_cb_(topic, count);
    }
}

void QtPersistenceInterface::on_statistics_updated() {
    // 更新完整的网络信息
    update_network_info();
    
    // 调用Qt回调
    if (statistics_updated_cb_) {
        statistics_updated_cb_();
    }
}

QtPersistenceInterface::NetworkInfo QtPersistenceInterface::get_network_info() const {
    std::lock_guard<std::mutex> lock(network_info_mutex_);
    return cached_network_info_;
}

void QtPersistenceInterface::update_network_info() {
    if (!persistence_service_) {
        return;
    }
    
    // 获取网络拓扑
    auto topology = persistence_service_->get_network_topology();
    auto active_topics = persistence_service_->get_active_topics();
    
    std::lock_guard<std::mutex> lock(network_info_mutex_);
    
    // 更新基础统计信息
    cached_network_info_.total_participants = topology.total_participants();
    cached_network_info_.total_writers = topology.total_writers();
    cached_network_info_.total_readers = topology.total_readers();
    
    // 计算持久化实体数量
    cached_network_info_.persistent_writers = 0;
    cached_network_info_.persistent_readers = 0;
    
    for (const auto& writer : topology.writers) {
        if (writer.durability.kind == TRANSIENT_DURABILITY_QOS || 
            writer.durability.kind == PERSISTENT_DURABILITY_QOS) {
            cached_network_info_.persistent_writers++;
        }
    }
    
    for (const auto& reader : topology.readers) {
        if (reader.durability.kind == TRANSIENT_DURABILITY_QOS || 
            reader.durability.kind == PERSISTENT_DURABILITY_QOS) {
            cached_network_info_.persistent_readers++;
        }
    }
    
    // 更新主题信息
    cached_network_info_.topics.clear();
    
    for (const auto& topic_name : active_topics) {
        TopicInfo topic_info;
        topic_info.name = topic_name;
        
        // 从网络拓扑中找到对应的类型信息
        for (const auto& writer : topology.writers) {
            if (writer.topic_name == topic_name) {
                topic_info.type = writer.type_name;
                if (writer.durability.kind == TRANSIENT_DURABILITY_QOS || 
                    writer.durability.kind == PERSISTENT_DURABILITY_QOS) {
                    topic_info.has_persistent_writers = true;
                }
                break;
            }
        }
        
        for (const auto& reader : topology.readers) {
            if (reader.topic_name == topic_name) {
                if (topic_info.type.empty()) {
                    topic_info.type = reader.type_name;
                }
                if (reader.durability.kind == TRANSIENT_DURABILITY_QOS || 
                    reader.durability.kind == PERSISTENT_DURABILITY_QOS) {
                    topic_info.has_persistent_readers = true;
                }
            }
        }
        
        // 设置当前时间作为最后活动时间
        topic_info.last_activity = std::chrono::system_clock::now();
        
        cached_network_info_.topics.push_back(topic_info);
    }
}
