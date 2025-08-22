#include "PersistenceService.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "../../cpp/hello_world/HelloWorldTypeSupport.h"
#include <iostream>
#include <signal.h>

USING_AGIDDS_NAMESPACE;

std::unique_ptr<PersistenceService> g_persistence_service;

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
    
    std::cout << "TravoDDS Persistence Service" << std::endl;
    std::cout << "============================" << std::endl;
    
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
        
        // 创建持久化服务
        std::string db_path = "persistence_service.db";
        if (argc > 2) {
            db_path = argv[2];
        }
        
        std::cout << "Initializing persistence service with database: " << db_path << std::endl;
        
        g_persistence_service = std::make_unique<PersistenceService>(participant, db_path);
        
        // 启动服务
        ReturnCode_t ret = g_persistence_service->start();
        if (ret != RETCODE_OK) {
            std::cerr << "Failed to start persistence service" << std::endl;
            return -1;
        }
        
        std::cout << "Persistence service started successfully!" << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;
        
        // 主循环 - 定期显示统计信息
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            
            // 获取网络拓扑
            auto topology = g_persistence_service->get_network_topology();
            
            std::cout << "\n=== Network Status ===" << std::endl;
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
            std::cout << "======================" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
