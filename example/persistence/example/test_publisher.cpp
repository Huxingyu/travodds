#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <map>
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/topic/topic.h"
#include "dcps/publish/publisher.h"
#include "dcps/publish/datawriter.h"
#include "dcps/subscribe/subscriber.h"
#include "dcps/subscribe/datareader.h"
#include "../../cpp/hello_world/HelloWorld.h"
#include "../../cpp/hello_world/HelloWorldTypeSupport.h"
#include "common/sequence/sequence.h"

USING_AGIDDS_NAMESPACE

// 简单的DataReader监听器，用于接收数据
class TestDataReaderListener : public DataReaderListener {
private:
    std::string strategy_name_;
    
public:
    explicit TestDataReaderListener(std::string strategy_name) 
        : strategy_name_(std::move(strategy_name)) {}
    
    void on_data_available(DataReader* reader) override {
        SequenceData<HelloWorld> data_seq;
        SampleInfoSeq info_seq;
        
        ReturnCode_t ret = reader->take(data_seq, info_seq, LENGTH_UNLIMITED,
                                       ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
        
        if (ret == RETCODE_OK) {
            for (int i = 0; i < data_seq.length(); ++i) {
                if (info_seq[i].valid_data) {
                    std::cout << "[" << strategy_name_ << " Reader] 接收到数据: " 
                              << "index=" << data_seq[i].index 
                              << ", message=" << data_seq[i].message 
                              << std::endl;
                }
            }
            reader->return_loan(data_seq, info_seq);
        }
    }
};

auto main() -> int {
    std::cout << "=== 测试三种持久化策略的DataWriter和DataReader ===" << std::endl;
    
    // 创建DomainParticipant
    DomainParticipantFactory* factory = DomainParticipantFactory::get_instance();
    DomainParticipant* participant = factory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    
    if (participant == nullptr) {
        std::cerr << "Failed to create DomainParticipant" << std::endl;
        return -1;
    }
    
    // 注册HelloWorld类型
    auto* type_support = HelloWorldTypeSupport::get_instance();
    const char* type_name = type_support->get_typename();
    ReturnCode_t register_ret = participant->registe_type(type_name, type_support);
    if (register_ret != RETCODE_OK) {
        std::cerr << "Failed to register HelloWorld type" << std::endl;
        return -1;
    }
    
    // 创建主题
    Topic* topic = participant->create_topic("TestDurabilityStrategies", "HelloWorld", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    
    if (topic == nullptr) {
        std::cerr << "Failed to create Topic" << std::endl;
        return -1;
    }
    
    // 创建Publisher和Subscriber
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    
    if (publisher == nullptr || subscriber == nullptr) {
        std::cerr << "Failed to create Publisher or Subscriber" << std::endl;
        return -1;
    }
    
    // 定义三种持久化策略
    struct DurabilityStrategy {
        std::string name;
        DurabilityQosPolicyKind kind;
    };
    
    std::vector<DurabilityStrategy> strategies = {
        {"TRANSIENT_LOCAL", TRANSIENT_LOCAL_DURABILITY_QOS},
        {"TRANSIENT", TRANSIENT_DURABILITY_QOS},
        {"PERSISTENT", PERSISTENT_DURABILITY_QOS}
    };
    
    // 存储创建的DataWriter和DataReader
    std::vector<DataWriter*> writers;
    std::vector<DataReader*> readers;
    std::vector<std::unique_ptr<TestDataReaderListener>> listeners;
    
    std::cout << "\n=== 创建三种策略的DataWriter ===" << std::endl;
    
    // 创建三种策略的DataWriter
    for (const auto& strategy : strategies) {
        DataWriterQos writer_qos;
        publisher->get_default_datawriter_qos(writer_qos);
        writer_qos.durability.kind = strategy.kind;
        writer_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        
        DataWriter* writer = publisher->create_datawriter(topic, writer_qos, nullptr, STATUS_MASK_NONE);
        if (writer == nullptr) {
            std::cerr << "Failed to create " << strategy.name << " DataWriter" << std::endl;
            continue;
        }
        
        writers.push_back(writer);
        std::cout << "✓ 成功创建 " << strategy.name << " DataWriter" << std::endl;
    }
    
    std::cout << "\n=== 创建三种策略的DataReader ===" << std::endl;
    
    // 创建三种策略的DataReader
    for (const auto& strategy : strategies) {
        DataReaderQos reader_qos;
        subscriber->get_default_datareader_qos(reader_qos);
        reader_qos.durability.kind = strategy.kind;
        reader_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        
        // 创建监听器
        auto listener = std::make_unique<TestDataReaderListener>(strategy.name);
        
        DataReader* reader = subscriber->create_datareader(topic, reader_qos, listener.get(), STATUS_MASK_ALL);
        if (reader == nullptr) {
            std::cerr << "Failed to create " << strategy.name << " DataReader" << std::endl;
            continue;
        }
        
        readers.push_back(reader);
        listeners.push_back(std::move(listener));
        std::cout << "✓ 成功创建 " << strategy.name << " DataReader" << std::endl;
    }
    
    // 等待一段时间让DDS发现完成
    std::cout << "\n=== 等待DDS发现完成 ===" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    std::cout << "\n=== 开始发布数据 ===" << std::endl;
    
    // 为每种策略发布不同的数据
    const int messages_per_strategy = 3;
    auto* sample = new HelloWorld();
    
    for (size_t strategy_idx = 0; strategy_idx < strategies.size() && strategy_idx < writers.size(); ++strategy_idx) {
        const auto& strategy = strategies[strategy_idx];
        DataWriter* writer = writers[strategy_idx];
        
        std::cout << "\n--- " << strategy.name << " Writer 发布数据 ---" << std::endl;
        
        for (int i = 0; i < messages_per_strategy; ++i) {
            sample->index = strategy_idx * 100 + i;  // 不同策略使用不同的index范围
            sample->message = strategy.name + "_message_" + std::to_string(i);
            
            ReturnCode_t ret = writer->write(sample, HANDLE_NIL);
            if (ret == RETCODE_OK) {
                std::cout << "  发布: index=" << sample->index 
                          << ", message=" << sample->message << std::endl;
            } else {
                std::cerr << "  发布失败: " << ret << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    std::cout << "\n=== 等待数据传输和处理 ===" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    std::cout << "\n=== 测试历史数据功能 ===" << std::endl;
    std::cout << "现在删除所有DataWriter，然后创建新的DataReader来测试历史数据..." << std::endl;
    
    // 删除所有DataWriter
    for (DataWriter* writer : writers) {
        if (writer != nullptr) {
            publisher->delete_datawriter(writer);
        }
    }
    writers.clear();
    
    std::cout << "DataWriter已删除，等待2秒..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 创建新的DataReader来测试历史数据
    std::cout << "创建新的DataReader来接收历史数据..." << std::endl;
    std::vector<std::unique_ptr<TestDataReaderListener>> history_listeners;
    std::vector<DataReader*> history_readers;
    
    for (const auto& strategy : strategies) {
        DataReaderQos reader_qos;
        subscriber->get_default_datareader_qos(reader_qos);
        reader_qos.durability.kind = strategy.kind;
        reader_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        
        auto listener = std::make_unique<TestDataReaderListener>(strategy.name + "_HISTORY");
        
        DataReader* reader = subscriber->create_datareader(topic, reader_qos, listener.get(), STATUS_MASK_ALL);
        if (reader != nullptr) {
            history_readers.push_back(reader);
            history_listeners.push_back(std::move(listener));
            std::cout << "✓ 创建历史数据测试的 " << strategy.name << " DataReader" << std::endl;
        }
    }
    
    std::cout << "\n等待历史数据传输..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    std::cout << "\n=== 测试完成，清理资源 ===" << std::endl;
    
    // 清理
    delete sample;
    participant->delete_contained_entities();
    factory->delete_participant(participant);
    
    std::cout << "\n=== 三种持久化策略测试完成 ===" << std::endl;
    return 0;
}
