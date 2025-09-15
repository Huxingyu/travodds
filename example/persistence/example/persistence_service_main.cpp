#include "PersistenceService.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "../../cpp/hello_world/HelloWorldTypeSupport.h"
#include <iostream>
#include <iomanip>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>
#include <unistd.h>
#include <string>
#include <cstring>
#include <clocale>

// Qt头文件（条件性包含）
#ifdef BUILD_WITH_QT
#include "PersistenceMonitor.h"
#include <QtWidgets/QApplication>
#endif

USING_TRAVODDS_NAMESPACE;

std::shared_ptr<PersistenceService> g_persistence_service;
std::atomic<bool> g_running{true};
bool g_gui_mode = false;

// 使用说明
void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [domain_id] [database_path] [--gui|gui]" << std::endl;
    std::cout << "  domain_id      : DDS domain ID (default: 0)" << std::endl;
    std::cout << "  database_path  : SQLite database file path (default: enhanced_persistence.db)" << std::endl;
    std::cout << "  --gui or gui   : Start with Qt GUI interface" << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << "              # Console mode, domain 0" << std::endl;
    std::cout << "  " << program_name << " 1            # Console mode, domain 1" << std::endl;
    std::cout << "  " << program_name << " 0 my.db gui  # GUI mode, custom database" << std::endl;
    std::cout << "  " << program_name << " --gui        # GUI mode, default settings" << std::endl;
}

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
    
    if (g_gui_mode) {
#ifdef BUILD_WITH_QT
        // GUI模式下，先退出Qt事件循环
        if (QApplication::instance()) {
            QApplication::instance()->quit();
        }
#endif
    }
    
    // 清理持久化服务
    if (g_persistence_service) {
        g_persistence_service->shutdown();
        g_persistence_service.reset();
    }
    
    if (!g_gui_mode) {
        // 控制台模式直接退出
        std::cout << "Graceful shutdown completed." << std::endl;
        _exit(0);
    }
}

int main(int argc, char* argv[]) {
    // 设置locale环境，确保支持UTF-8
    std::setlocale(LC_ALL, "");
    
    // 解析命令行参数
    DomainId_t domain_id = 0;
    std::string database_path = "enhanced_persistence.db";
    
    // 检查是否有GUI参数
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--gui") == 0 || strcmp(argv[i], "gui") == 0) {
            g_gui_mode = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (i == 1 && argv[i][0] != '-') {
            // 第一个非选项参数是domain_id
            domain_id = std::atoi(argv[i]);
        } else if (i == 2 && argv[i][0] != '-') {
            // 第二个非选项参数是database_path
            database_path = argv[i];
        }
    }

#ifdef BUILD_WITH_QT
    // 如果启用GUI模式，创建QApplication
    std::unique_ptr<QApplication> app;
    if (g_gui_mode) {
        app = std::make_unique<QApplication>(argc, argv);
       
        // 设置应用程序信息
        app->setApplicationName("TravoDDS Persistence Service");
        app->setApplicationVersion("1.0");
        app->setOrganizationName("TravoDDS");
    }
#else
    if (g_gui_mode) {
        std::cerr << "GUI mode requested but Qt support not compiled in." << std::endl;
        std::cerr << "Please rebuild with Qt5 installed." << std::endl;
        return -1;
    }
#endif

    // 设置信号处理
    signal(SIGINT, signal_handler);// 处理 Ctrl+C 中断信号
    signal(SIGTERM, signal_handler);// 处理终止信号
    
    std::cout << "Enhanced TravoDDS Persistence Service" << std::endl;
    std::cout << "=====================================" << std::endl;
    if (g_gui_mode) {
        std::cout << "Starting in GUI mode..." << std::endl;
    } else {
        std::cout << "Starting in console mode..." << std::endl;
    }
    
    try {
        // 创建DomainParticipant
        std::cout << "Using domain ID: " << domain_id << std::endl;
        std::cout << "Using database: " << database_path << std::endl;
        
        
        DomainParticipant* participant = TheParticipantFactory->create_participant(
            domain_id, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        
        if (!participant) {
            std::cerr << "Failed to create DomainParticipant" << std::endl;
            return -1;
        }
        
        // 明确启用DomainParticipant以确保自动发现功能可用
        ReturnCode_t enable_ret = participant->enable();
        if (enable_ret != RETCODE_OK) {
            std::cerr << "Failed to enable DomainParticipant, error code: " << enable_ret << std::endl;
            return -1;
        }
        
        // 注册topic类型
        const char* type_name = HelloWorldTypeSupport::get_instance()->get_typename();
        ReturnCode_t register_ret = participant->registe_type(type_name, HelloWorldTypeSupport::get_instance());
        if (register_ret != RETCODE_OK) {
            std::cerr << "Failed to register HelloWorld type, error code: " << register_ret << std::endl;
            return -1;
        }
        
        // 创建持久化服务配置信息
        PersistenceConfig config;
        config.database_path = database_path;
        config.data_retention = std::chrono::hours(48);       // 48小时数据保留
        config.max_samples_per_topic = 5000;                 // 每个Topic最大5000样本
        config.memory_cache_size_mb = 100;                    // 100MB内存缓存
        config.enable_statistics = true;                     // 启用统计
        config.worker_thread_count = 3;                       // 3个工作线程
        
        std::cout << "Initializing persistence service with configuration:" << std::endl;
        std::cout << "- Database: " << config.database_path << std::endl;
        std::cout << "- Data retention: " << config.data_retention.count() << " hours" << std::endl;
        std::cout << "- Max samples per topic: " << config.max_samples_per_topic << std::endl;
        std::cout << "- Memory cache size: " << config.memory_cache_size_mb << " MB" << std::endl;
        std::cout << "- Worker threads: " << config.worker_thread_count << std::endl;
        
        g_persistence_service = std::make_shared<PersistenceService>(config);
        
        // 使用自定义删除器创建shared_ptr，确保正确的生命周期管理
        auto participant_deleter = [](DomainParticipant* participant_ptr) {
            if (participant_ptr != nullptr) {
                TheParticipantFactory->delete_participant(participant_ptr);
                std::cout << "[Cleanup] DomainParticipant correctly deleted by custom deleter" << std::endl;
            }
        };
        std::shared_ptr<DomainParticipant> participant_ptr(participant, participant_deleter);
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
        
 
        std::cout << "\npersistence service started successfully!" << std::endl;
        
        if (g_gui_mode) {
#ifdef BUILD_WITH_QT
            std::cout << "Starting Qt GUI monitoring interface..." << std::endl;
            
            // 创建监控窗口
            PersistenceMonitor monitor;
            monitor.setPersistenceService(g_persistence_service);
            monitor.show();
            
            std::cout << "Qt GUI started successfully!" << std::endl;
            
            // 进入Qt事件循环
            return app->exec();
#else
            std::cerr << "GUI mode not available - Qt not compiled in" << std::endl;
            return -1;
#endif
        } else {
            // 控制台模式
            std::cout << "Press Ctrl+C to stop..." << std::endl;
            
            // 主循环 - 定期显示统计信息
            while (g_running) {
                std::this_thread::sleep_for(std::chrono::seconds(30));
                
                if (!g_running) break;  
                
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
        }
        
        // 关闭持久化服务
        std::cout << "Shutting down gracefully..." << std::endl;
        if (g_persistence_service) {
            g_persistence_service->shutdown();
            g_persistence_service.reset();
        }
        
        // DomainParticipant 将通过 shared_ptr 的自定义删除器自动清理
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        
        // 发生异常时也要确保清理
        if (g_persistence_service) {
            g_persistence_service->shutdown();
            g_persistence_service.reset();
        }
        
        return -1;
    }
    
    return 0;
}
