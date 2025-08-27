#include "PersistenceService.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "../../cpp/hello_world/HelloWorldTypeSupport.h"
#include <iostream>
#include <iomanip>
#include <signal.h>
#include <thread>
#include <chrono>

USING_AGIDDS_NAMESPACE;

std::unique_ptr<PersistenceService> g_persistence_service;

// 简单的Qt回调示例实现
class ConsoleUICallback : public IPersistenceUICallback {
public:
    void on_writer_discovered(const std::string& topic, const std::string& type) override {
        std::cout << "[UI] Writer discovered - Topic: " << topic << ", Type: " << type << std::endl;
    }
    
    void on_reader_discovered(const std::string& topic, const std::string& type) override {
        std::cout << "[UI] Reader discovered - Topic: " << topic << ", Type: " << type << std::endl;
    }
    
    void on_data_stored(const std::string& topic, size_t data_size) override {
        std::cout << "[UI] Data stored - Topic: " << topic << ", Size: " << data_size << " bytes" << std::endl;
    }
    
    void on_historical_data_sent(const std::string& topic, size_t count) override {
        std::cout << "[UI] Historical data sent - Topic: " << topic << ", Count: " << count << std::endl;
    }
    
    void on_statistics_updated() override {
        // 定期统计更新，避免太频繁的输出
        static auto last_print = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() > 10) {
            std::cout << "[UI] Statistics updated" << std::endl;
            last_print = now;
        }
    }
};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_persistence_service) {
        g_persistence_service->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "Enhanced TravoDDS Persistence Service" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    try {
        // 创建DomainParticipant
        DomainId_t domain_id = 0;
        if (argc > 1) {
            domain_id = std::atoi(argv[1]);
        }
        
        std::cout << "Creating DomainParticipant for domain " << domain_id << "..." << std::endl;
        
        DomainParticipant* participant = TheParticipantFactory->create_participant(
            domain_id, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        
        if (!participant) {
            std::cerr << "Failed to create DomainParticipant" << std::endl;
            return -1;
        }
        
        // 注册HelloWorld类型到持久化服务的DomainParticipant
        std::cout << "Registering HelloWorld type..." << std::endl;
        const char* type_name = HelloWorldTypeSupport::get_instance()->get_typename();
        ReturnCode_t register_ret = participant->registe_type(type_name, HelloWorldTypeSupport::get_instance());
        if (register_ret != RETCODE_OK) {
            std::cerr << "Failed to register HelloWorld type" << std::endl;
            return -1;
        }
        std::cout << "HelloWorld type registered successfully" << std::endl;
        
        // 创建增强配置
        PersistenceConfig config;
        config.db_path = "enhanced_persistence.db";
        config.cleanup_interval = std::chrono::seconds(180);  // 3分钟清理一次
        config.data_retention = std::chrono::hours(48);       // 48小时数据保留
        config.max_samples_per_topic = 5000;                 // 每个Topic最大5000样本
        config.max_db_size_mb = 512;                          // 最大512MB数据库
        config.enable_compression = true;                     // 启用压缩
        config.enable_statistics = true;                     // 启用统计
        config.worker_threads = 3;                           // 3个工作线程
        
        if (argc > 2) {
            config.db_path = argv[2];
        }
        
        std::cout << "Initializing enhanced persistence service with configuration:" << std::endl;
        std::cout << "- Database: " << config.db_path << std::endl;
        std::cout << "- Cleanup interval: " << config.cleanup_interval.count() << " seconds" << std::endl;
        std::cout << "- Data retention: " << config.data_retention.count() << " hours" << std::endl;
        std::cout << "- Compression: " << (config.enable_compression ? "enabled" : "disabled") << std::endl;
        std::cout << "- Worker threads: " << config.worker_threads << std::endl;
        
        g_persistence_service = std::make_unique<PersistenceService>(participant, config);
        
        // 设置UI回调
        auto ui_callback = std::make_shared<ConsoleUICallback>();
        g_persistence_service->set_ui_callback(ui_callback);
        
        // 启动服务
        ReturnCode_t ret = g_persistence_service->start();
        if (ret != RETCODE_OK) {
            std::cerr << "Failed to start persistence service" << std::endl;
            return -1;
        }
        
        std::cout << "Enhanced persistence service started successfully!" << std::endl;
        std::cout << "Features: Statistics tracking, UI callbacks, compression, configurable cleanup" << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;
        
        // 主循环 - 定期显示增强的统计信息
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            
            // 获取网络拓扑
            auto topology = g_persistence_service->get_network_topology();
            auto stats = g_persistence_service->get_statistics();
            auto active_topics = g_persistence_service->get_active_topics();
            
            std::cout << "\n=== Enhanced Network Status ===" << std::endl;
            std::cout << "Participants: " << topology.participants.size() << std::endl;
            std::cout << "DataWriters: " << topology.writers.size() << std::endl;
            std::cout << "DataReaders: " << topology.readers.size() << std::endl;
            
            // 显示持久化DataWriter
            int persistent_writers = 0;
            for (const auto& writer : topology.writers) {
                if (writer.durability.kind == TRANSIENT_DURABILITY_QOS || 
                    writer.durability.kind == PERSISTENT_DURABILITY_QOS) {
                    persistent_writers++;
                }
            }
            std::cout << "Persistent Writers: " << persistent_writers << std::endl;
            
            // 显示增强的统计信息
            std::cout << "\n=== Persistence Statistics ===" << std::endl;
            std::cout << "Samples Stored: " << stats.total_samples_stored.load() << std::endl;
            std::cout << "Samples Served: " << stats.total_samples_served.load() << std::endl;
            std::cout << "Bytes Stored: " << stats.total_bytes_stored.load() << std::endl;
            std::cout << "Bytes Served: " << stats.total_bytes_served.load() << std::endl;
            std::cout << "Active Topics: " << active_topics.size() << std::endl;
            std::cout << "Uptime: " << std::fixed << std::setprecision(1) 
                     << stats.get_uptime_seconds() << " seconds" << std::endl;
            
            // 显示活跃的主题列表
            if (!active_topics.empty()) {
                std::cout << "\n=== Active Topics ===" << std::endl;
                for (const auto& topic : active_topics) {
                    std::cout << "- " << topic << std::endl;
                }
            }
            
            std::cout << "===============================" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
