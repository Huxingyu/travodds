#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <string>
#include <memory>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include "HelloWorldPubSubTypes.h"
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/log/StdoutErrConsumer.hpp>
#include <fastrtps/transport/UDPv4TransportDescriptor.h>

using namespace eprosima::fastdds::dds;

std::atomic<bool> stop_flag(false);

void signal_handler(int signal)
{
    stop_flag = true;
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
}

bool test_data_serialization()
{
    HelloWorld test_data;
    test_data.index(123);
    test_data.message("Test message");

    // 尝试获取序列化大小
    try {
        size_t serialized_size = test_data.getMaxCdrSerializedSize();
        std::cout << "Max serialized size: " << serialized_size << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Serialization test failed: " << e.what() << std::endl;
        return false;
    }
}

bool configure_security(DomainParticipantQos& participant_qos)
{
    participant_qos.transport().use_builtin_transports = false;
    //添加UDPv4传输
    auto udp_transport = std::make_shared<eprosima::fastrtps::rtps::UDPv4TransportDescriptor>();

    // 配置UDP参数
    udp_transport->sendBufferSize = 65536;
    udp_transport->receiveBufferSize = 65536;

    // 添加到用户传输列表
    participant_qos.transport().user_transports.push_back(udp_transport);
    // 启用详细的安全日志
    participant_qos.properties().properties().emplace_back(
        "dds.sec.log.file", "security_debug.log");
    participant_qos.properties().properties().emplace_back(
        "dds.sec.log.level", "ALL");  // ALL是最详细的级别

    // 设置安全插件名称（对应TravODDS的dds.sec.plugin_name）
    // 明确设置所有三个安全插件
    participant_qos.properties().properties().emplace_back(
        "dds.sec.auth.plugin", "builtin.PKI-DH");
    participant_qos.properties().properties().emplace_back(
        "dds.sec.access.plugin", "builtin.Access-Permissions");
    participant_qos.properties().properties().emplace_back(
        "dds.sec.crypto.plugin", "builtin.AES-GCM-GMAC");  // 明确指定加密插件

    // TODO 需要根据实际路径修改
    // 身份验证配置
    participant_qos.properties().properties().emplace_back(
        "dds.sec.auth.builtin.PKI-DH.identity_ca", "file://../config/identity_ca.cert.pem");
    participant_qos.properties().properties().emplace_back(
        "dds.sec.auth.builtin.PKI-DH.identity_certificate", "file://../config/cert.pem");
    participant_qos.properties().properties().emplace_back(
        "dds.sec.auth.builtin.PKI-DH.private_key", "file://../config/key.pem");

    // 访问控制配置
    participant_qos.properties().properties().emplace_back(
        "dds.sec.access.builtin.Access-Permissions.permissions_ca", "file://../config/permissions_ca.cert.pem");
    participant_qos.properties().properties().emplace_back(
        "dds.sec.access.builtin.Access-Permissions.permissions", "file://../config/permissions.p7s");
    participant_qos.properties().properties().emplace_back(
        "dds.sec.access.builtin.Access-Permissions.governance", "file://../config/governance.p7s");
  
    return true;
}

class FastDDSClient
{
private:
    DomainParticipant* participant_;
    Publisher* publisher_;
    Subscriber* subscriber_;
    Topic* topic_;
    DataWriter* writer_;
    DataReader* reader_;
    TypeSupport type_;
    
    bool is_publisher_;
    int domain_id_;
    std::string topic_name_;

    // Writer listener for publication matched events
    class WriterListener : public DataWriterListener
    {
    public:
        void on_publication_matched(
            DataWriter* writer,
            const PublicationMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                std::cout << "[Publisher] Matched with subscriber " << info.last_subscription_handle << std::endl;
            }
            else if (info.current_count_change == -1)
            {
                std::cout << "[Publisher] Unmatched with subscriber " << info.last_subscription_handle << std::endl;
            }
        }
    } writer_listener_;

    // Reader listener for data available events
    class ReaderListener : public DataReaderListener
    {
    public:
        void on_data_available(DataReader* reader) override
        {
            SampleInfo info;
            HelloWorld data;
            
            while (reader->take_next_sample(&data, &info) == ReturnCode_t::RETCODE_OK)
            {
                if (info.valid_data)
                {
                    std::cout << "[Subscriber] Received - Index: " << data.index() 
                              << ", Message: " << data.message() << std::endl;
                }
            }
        }

        void on_subscription_matched(
            DataReader* reader,
            const SubscriptionMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                std::cout << "[Subscriber] Matched with publisher " << info.last_publication_handle << std::endl;
            }
            else if (info.current_count_change == -1)
            {
                std::cout << "[Subscriber] Unmatched with publisher " << info.last_publication_handle << std::endl;
            }
        }
    } reader_listener_;

public:
    FastDDSClient(bool is_publisher, int domain_id = 0, const std::string& topic_name = "rr/aReply")
        : participant_(nullptr)
        , publisher_(nullptr)
        , subscriber_(nullptr)
        , topic_(nullptr)
        , writer_(nullptr)
        , reader_(nullptr)
        , type_(new HelloWorldPubSubType())
        , is_publisher_(is_publisher)
        , domain_id_(domain_id)
        , topic_name_(topic_name)
    {
    }

    ~FastDDSClient()
    {
        cleanup();
    }

    bool initialize()
    {
        eprosima::fastdds::dds::Log::SetVerbosity(eprosima::fastdds::dds::Log::Kind::Info);

        // Create Domain Participant
        //DomainParticipantQos participant_qos = PARTICIPANT_QOS_DEFAULT;
        //participant_ = DomainParticipantFactory::get_instance()->create_participant(domain_id_, participant_qos);

        DomainParticipantQos participant_qos = PARTICIPANT_QOS_DEFAULT;

        // 配置安全
        if (!configure_security(participant_qos)) {
            return false;
        }
        participant_ = DomainParticipantFactory::get_instance()->create_participant(domain_id_, participant_qos);

        if (participant_ == nullptr)
        {
            std::cerr << "Error: Failed to create domain participant" << std::endl;
            return false;
        }

        // Register the Type
        type_.register_type(participant_);

        // Create Topic
        topic_ = participant_->create_topic(topic_name_, "HelloWorld", TOPIC_QOS_DEFAULT);
        if (topic_ == nullptr)
        {
            std::cerr << "Error: Failed to create topic" << std::endl;
            cleanup();
            return false;
        }

        if (is_publisher_)
        {
            return initialize_publisher();
        }
        else
        {
            return initialize_subscriber();
        }
    }

    void run()
    {
        if (is_publisher_)
        {
            run_publisher();
        }
        else
        {
            run_subscriber();
        }
    }

private:
    bool initialize_publisher()
    {
        // 测试数据序列化
        if (!test_data_serialization()) {
            std::cerr << "Data serialization test failed!" << std::endl;
            return false;
        }

        // Create Publisher
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        if (publisher_ == nullptr)
        {
            std::cerr << "Error: Failed to create publisher" << std::endl;
            cleanup();
            return false;
        }

        // Create DataWriter
        DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
        writer_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        //writer_qos.reliability().kind = RELIABLE_RELIABILITY_QOS/*BEST_EFFORT_RELIABILITY_QOS*/;

        writer_ = publisher_->create_datawriter(topic_, writer_qos, &writer_listener_);
        if (writer_ == nullptr)
        {
            std::cerr << "Error: Failed to create data writer" << std::endl;
            cleanup();
            return false;
        }

        std::cout << "FastDDS Publisher initialized successfully!" << std::endl;
        std::cout << "Domain ID: " << domain_id_ << std::endl;
        std::cout << "Topic: " << topic_name_ << std::endl;
        std::cout << "Waiting for subscribers..." << std::endl;

        return true;
    }

    bool initialize_subscriber()
    {
        // Create Subscriber
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        if (subscriber_ == nullptr)
        {
            std::cerr << "Error: Failed to create subscriber" << std::endl;
            cleanup();
            return false;
        }

        // Create DataReader
        DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
        reader_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        //reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

        reader_ = subscriber_->create_datareader(topic_, reader_qos, &reader_listener_);
        if (reader_ == nullptr)
        {
            std::cerr << "Error: Failed to create data reader" << std::endl;
            cleanup();
            return false;
        }

        std::cout << "FastDDS Subscriber initialized successfully!" << std::endl;
        std::cout << "Domain ID: " << domain_id_ << std::endl;
        std::cout << "Topic: " << topic_name_ << std::endl;
        std::cout << "Waiting for publishers..." << std::endl;

        return true;
    }

    void run_publisher()
    {
        HelloWorld data;
        int count = 0;

        std::cout << "Publisher started. Press Ctrl+C to stop." << std::endl;

        while (getchar()/*!stop_flag*/)
        {
            data.index(count);
            data.message("Hello from FastDDS - Count: " + std::to_string(count));
            bool ret = writer_->write(&data);
            if (!ret)
            {
                if (1/*count % 10 == 0*/)  // Print every 10th message
                {
                    std::cout << "[Publisher] Sent - Index: " << data.index()
                        << ", Message: " << data.message() << std::endl;
                }
            }
            else
            {
                if (1/*count % 10 == 0*/)  // Print every 10th message
                {
                    std::cout << "[Publisher] Sent - Index: " << data.index() 
                              << ", Message: " << data.message() << std::endl;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            count++;
        }
        
        std::cout << "Publisher stopped after sending " << count << " messages." << std::endl;
    }

    void run_subscriber()
    {
        std::cout << "Subscriber started. Press Ctrl+C to stop." << std::endl;

        while (!stop_flag)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "Subscriber stopped." << std::endl;
    }

    void cleanup()
    {
        if (writer_ != nullptr)
        {
            publisher_->delete_datawriter(writer_);
            writer_ = nullptr;
        }
        if (reader_ != nullptr)
        {
            subscriber_->delete_datareader(reader_);
            reader_ = nullptr;
        }
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
            publisher_ = nullptr;
        }
        if (subscriber_ != nullptr)
        {
            participant_->delete_subscriber(subscriber_);
            subscriber_ = nullptr;
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
            topic_ = nullptr;
        }
        if (participant_ != nullptr)
        {
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
            participant_ = nullptr;
        }
    }
};

void print_usage(const char* program_name)
{
    std::cout << "FastDDS Client - Communication with TravODDS" << std::endl;
    std::cout << "Usage: " << program_name << " [pub|sub] [domain_id] [topic_name]" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  pub|sub      : Run as Publisher or Subscriber" << std::endl;
    std::cout << "  domain_id    : Domain ID (default: 0)" << std::endl;
    std::cout << "  topic_name   : Topic name (default: rr/aReply)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " pub                    # Run as Publisher" << std::endl;
    std::cout << "  " << program_name << " sub                    # Run as Subscriber" << std::endl;
    std::cout << "  " << program_name << " pub 150 my_topic       # Custom domain and topic" << std::endl;
    std::cout << "  " << program_name << "                        # Interactive mode" << std::endl;
}

int main(int argc, char* argv[])
{
    // Setup signal handling for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 启用所有INFO级别日志（包括SECURITY）
    eprosima::fastdds::dds::Log::SetVerbosity(eprosima::fastdds::dds::Log::Kind::Info);

    static eprosima::fastdds::dds::StdoutErrConsumer consumer;
    eprosima::fastdds::dds::Log::ClearConsumers();
    eprosima::fastdds::dds::Log::RegisterConsumer(std::unique_ptr<eprosima::fastdds::dds::StdoutErrConsumer>(&consumer));

    // 设置详细级别
    eprosima::fastdds::dds::Log::SetVerbosity(eprosima::fastdds::dds::Log::Kind::Info);

    // 启用 SECURITY 类别
    eprosima::fastdds::dds::Log::SetCategoryFilter(std::regex("SECURITY"));

    // 启用文件名和行号显示
    eprosima::fastdds::dds::Log::ReportFilenames(true);
    eprosima::fastdds::dds::Log::ReportFunctions(true);

    // Default parameters
    bool is_publisher = false;
    int domain_id = 150; 
    //"HelloWorldTopic", "HelloWorld"
    std::string topic_name = "rr/aReply";//"HelloWorldTopic";

    // Parse command line arguments
    if (argc >= 2)
    {
        std::string mode = argv[1];
        if (mode == "pub")
        {
            is_publisher = true;
        }
        else if (mode == "sub")
        {
            is_publisher = false;
        }
        else
        {
            std::cerr << "Error: Invalid mode '" << mode << "'. Use 'pub' or 'sub'." << std::endl;
            print_usage(argv[0]);
            return 1;
        }

        if (argc >= 3)
        {
            try
            {
                domain_id = std::stoi(argv[2]);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error: Invalid domain ID '" << argv[2] << "'" << std::endl;
                return 1;
            }
        }

        if (argc >= 4)
        {
            topic_name = argv[3];
        }
    }
    else
    {
        // Interactive mode
        std::string mode;
        std::cout << "Select mode (pub/sub): ";
        std::cin >> mode;
        if (mode == "pub")
        {
            is_publisher = true;
        }
        else if (mode == "sub")
        {
            is_publisher = false;
        }
        else
        {
            std::cerr << "Error: Invalid mode '" << mode << "'" << std::endl;
            return 1;
        }

        std::cout << "domain ID (default 0): ";
        domain_id = 0;

    }

    std::cout << "Starting FastDDS Client..." << std::endl;
    std::cout << "Mode: " << (is_publisher ? "Publisher" : "Subscriber") << std::endl;
    std::cout << "Domain ID: " << domain_id << std::endl;
    std::cout << "Topic: " << topic_name << std::endl;

    try
    {
        FastDDSClient client(is_publisher, domain_id, topic_name);
        
        if (client.initialize())
        {
            client.run();
            std::cout << "FastDDS Client terminated successfully." << std::endl;
            return 0;
        }
        else
        {
            std::cerr << "Failed to initialize FastDDS Client" << std::endl;
            return 1;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
