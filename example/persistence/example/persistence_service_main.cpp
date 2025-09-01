#include "PersistenceService.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "../../cpp/hello_world/HelloWorldTypeSupport.h"
#include <iostream>
#include <iomanip>
#include <signal.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <unistd.h>

USING_AGIDDS_NAMESPACE

std::unique_ptr<PersistenceService> g_persistence_service;
std::atomic<bool> g_running{true};

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
    g_running = false;
    if (g_persistence_service) {
        g_persistence_service->shutdown();
        g_persistence_service.reset();
    }
    
    // 直接退出，避免在主线程中再次清理
    std::cout << "Graceful shutdown completed." << std::endl;
    _exit(0);
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, signal_handler);// 处理 Ctrl+C 中断信号
    signal(SIGTERM, signal_handler);// 处理终止信号
    
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
        
        // 注册topic类型
        std::cout << "Registering HelloWorld type ..." << std::endl;
        const char* type_name = HelloWorldTypeSupport::get_instance()->get_typename();
        ReturnCode_t register_ret = participant->registe_type(type_name, HelloWorldTypeSupport::get_instance());
        if (register_ret != RETCODE_OK) {
            std::cerr << "Failed to register HelloWorld type, error code: " << register_ret << std::endl;
            return -1;
        }
        std::cout << "HelloWorld type registered successfully" << std::endl;
        
        // 创建持久化服务配置信息
        PersistenceConfig config;
        config.database_path = "enhanced_persistence.db";
        config.data_retention = std::chrono::hours(48);       // 48小时数据保留
        config.max_samples_per_topic = 5000;                 // 每个Topic最大5000样本
        config.memory_cache_size_mb = 100;                    // 100MB内存缓存
        config.enable_statistics = true;                     // 启用统计
        config.worker_thread_count = 3;                       // 3个工作线程
        
        if (argc > 2) {
            config.database_path = argv[2];
        }
        
        std::cout << "Initializing persistence service with configuration:" << std::endl;
        std::cout << "- Database: " << config.database_path << std::endl;
        std::cout << "- Data retention: " << config.data_retention.count() << " hours" << std::endl;
        std::cout << "- Max samples per topic: " << config.max_samples_per_topic << std::endl;
        std::cout << "- Memory cache size: " << config.memory_cache_size_mb << " MB" << std::endl;
        std::cout << "- Worker threads: " << config.worker_thread_count << std::endl;
        
        g_persistence_service = std::make_unique<PersistenceService>(config);
        // 使用shared_ptr包装原始指针
        std::shared_ptr<DomainParticipant> participant_ptr(participant);
        g_persistence_service->set_domain_participant(participant_ptr);
        
        // 设置UI回调
        auto ui_callback = std::make_shared<ConsoleUICallback>();
        g_persistence_service->set_ui_callback(ui_callback);
        
        // 启动服务
        bool success = g_persistence_service->initialize();
        if (!success) {
            std::cerr << "Failed to initialize persistence service" << std::endl;
            return -1;
        }
        
        // ===============================================================
        // 临时手动注册机制 (当AgirosDDS内置发现完善后可移除)
        // ===============================================================
        std::cout << "\n[Manual Registration] 开始手动注册Topic..." << std::endl;
        std::cout << "[Manual Registration] 注意: 这是临时方案，未来将使用自动发现机制" << std::endl;
        
        // 手动注册需要持久化服务处理的Topic类型
        // 注意：只注册TRANSIENT和PERSISTENT策略，VOLATILE和TRANSIENT_LOCAL由服务自动忽略
        g_persistence_service->notify_writer_created("TopicTransient", type_name, TRANSIENT_DURABILITY_QOS);
        g_persistence_service->notify_reader_created("TopicTransient", type_name, TRANSIENT_DURABILITY_QOS);
        
        g_persistence_service->notify_writer_created("TopicPersistent", type_name, PERSISTENT_DURABILITY_QOS);
        g_persistence_service->notify_reader_created("TopicPersistent", type_name, PERSISTENT_DURABILITY_QOS);
        
        std::cout << "[Manual Registration] 手动注册完成，持久化服务现在可以处理:" << std::endl;
        std::cout << "[Manual Registration]   - TopicTransient (TRANSIENT策略)" << std::endl;
        std::cout << "[Manual Registration]   - TopicPersistent (PERSISTENT策略)" << std::endl;
        std::cout << "[Manual Registration] 自动发现线程同时在后台运行，准备就绪后将自动接管" << std::endl;
        // ===============================================================
        
        std::cout << "\nEnhanced persistence service started successfully!" << std::endl;
        std::cout << "Features: Statistics tracking, UI callbacks, compression, configurable cleanup" << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;
        
        // 主循环 - 定期显示统计信息
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            
            if (!g_running) break;  // 再次检查以便快速退出
            
            // 获取统计信息
            auto stats = g_persistence_service->get_statistics();
            
            // 显示统计信息
            std::cout << "\n=== Persistence Statistics ===" << std::endl;
            for (const auto& [key, value] : stats) {
                std::cout << key << ": " << value << std::endl;
            }
            
            std::cout << "Service Status: " << (g_persistence_service->is_running() ? "Running" : "Stopped") << std::endl;
            
            std::cout << "===============================" << std::endl;
        }
        
        // 关闭持久化服务
        std::cout << "Shutting down gracefully..." << std::endl;
        if (g_persistence_service) {
            g_persistence_service->shutdown();
            g_persistence_service.reset();
        }
        
        // 清理DDS参与者
        if (participant) {
            TheParticipantFactory->delete_participant(participant);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
