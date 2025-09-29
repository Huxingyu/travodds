#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/topic/topic.h"
#include "../../cpp/hello_world/HelloWorld.h"
#include "../../cpp/hello_world/HelloWorldTypeSupport.h"

USING_TRAVODDS_NAMESPACE;

// 获取策略名称字符串
const char* get_strategy_name(DurabilityQosPolicyKind kind) {
	switch (kind) {
		case VOLATILE_DURABILITY_QOS: return "VOLATILE";
		case TRANSIENT_LOCAL_DURABILITY_QOS: return "TRANSIENT_LOCAL";
		case TRANSIENT_DURABILITY_QOS: return "TRANSIENT";
		case PERSISTENT_DURABILITY_QOS: return "PERSISTENT";
		default: return "UNKNOWN";
	}
}

// 从字符串获取策略
DurabilityQosPolicyKind get_strategy_from_string(const std::string& strategy_str) {
	if (strategy_str == "volatile" || strategy_str == "1") return VOLATILE_DURABILITY_QOS;
	if (strategy_str == "transient_local" || strategy_str == "2") return TRANSIENT_LOCAL_DURABILITY_QOS;
	if (strategy_str == "transient" || strategy_str == "3") return TRANSIENT_DURABILITY_QOS;
	if (strategy_str == "persistent" || strategy_str == "4") return PERSISTENT_DURABILITY_QOS;
	return VOLATILE_DURABILITY_QOS; // 默认值
}

// 获取与持久化服务匹配的Topic名称
std::string get_topic_name(DurabilityQosPolicyKind kind) {
	switch (kind) {
		case VOLATILE_DURABILITY_QOS: return "TopicVolatile";
		case TRANSIENT_LOCAL_DURABILITY_QOS: return "TopicTransientLocal";
		case TRANSIENT_DURABILITY_QOS: return "TopicTransient";
		case PERSISTENT_DURABILITY_QOS: return "TopicPersistent";
		default: return "TopicVolatile";
	}
}

// 显示帮助信息
void show_usage(const char* program_name) {
	std::cout << "用法: " << program_name << " <mode> <strategy>" << std::endl;
	std::cout << std::endl;
	std::cout << "模式 (mode):" << std::endl;
	std::cout << "  pub       - 发布者模式" << std::endl;
	std::cout << "  sub       - 订阅者模式" << std::endl;
	std::cout << std::endl;
	std::cout << "策略 (strategy):" << std::endl;
	std::cout << "  1 / volatile        - VOLATILE (易失性)" << std::endl;
	std::cout << "  2 / transient_local - TRANSIENT_LOCAL (瞬态本地)" << std::endl;
	std::cout << "  3 / transient       - TRANSIENT (瞬态)" << std::endl;
	std::cout << "  4 / persistent      - PERSISTENT (持久)" << std::endl;
	std::cout << std::endl;
	std::cout << "示例:" << std::endl;
	std::cout << "  " << program_name << " pub transient      # 使用TRANSIENT策略的发布者" << std::endl;
	std::cout << "  " << program_name << " sub 3              # 使用TRANSIENT策略的订阅者" << std::endl;
	std::cout << "  " << program_name << " pub persistent     # 使用PERSISTENT策略的发布者" << std::endl;
}

class myDataListener : public DataReaderListener {
public:
	virtual void on_data_available(DataReader* reader);
};

void myDataListener::on_data_available(DataReader* reader)
{
	SequenceData<HelloWorld> data_seq;
	SampleInfoSeq info_seq;
	ReturnCode_t retcode;
	int i;
	retcode = reader->take(
		data_seq, info_seq, LENGTH_UNLIMITED,
		ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);

	if (retcode == RETCODE_NO_DATA) {
		return;
	}
	else if (retcode != RETCODE_OK) {
		fprintf(stderr, "take error %d\n", retcode);
		return;
	}
	for (i = 0; i < data_seq.length(); ++i) {
		printf("Received data, index %d, message %s\n", data_seq[i].index, data_seq[i].message.data());
	}
}

static int publisher_shutdown(DomainParticipant* participant)
{
	ReturnCode_t retcode;
	int status = 0;

	if (participant != NULL) {
		retcode = participant->delete_contained_entities();
		if (retcode != RETCODE_OK) {
			fprintf(stderr, "delete_contained_entities error %d\n", retcode);
			status = -1;
		}

			DomainParticipantFactory::get_instance()->delete_participant(participant);
		if (retcode != RETCODE_OK) {
			fprintf(stderr, "delete_participant error %d\n", retcode);
			status = -1;
		}
	}

	return status;
}

extern "C" int publisher_main(DurabilityQosPolicyKind strategy)
{
	int sample_count = 10;
	int count = 0;
	DomainParticipant* participant = NULL;
	Publisher* publisher = NULL;
	Topic* topic = NULL;
	DataWriter* writer = NULL;
	ReturnCode_t retcode;
	const char* type_name = NULL;
	HelloWorld instance;
	InstanceHandle_t instance_handle = HANDLE_NIL;

	const char* strategy_name = get_strategy_name(strategy);
	printf("=== 启动发布者 - %s 策略 ===\n", strategy_name);

	participant = TheParticipantFactory->
		create_participant(0, PARTICIPANT_QOS_DEFAULT, NULL, STATUS_MASK_NONE);
	if (participant == NULL) {
		fprintf(stderr, "create_participant error\n");
		publisher_shutdown(participant);
		return -1;
	}

	publisher = participant->create_publisher(
		PUBLISHER_QOS_DEFAULT, NULL, STATUS_MASK_NONE);
	if (publisher == NULL) {
		fprintf(stderr, "create_publisher error\n");
		publisher_shutdown(participant);
		return -1;
	}

	type_name = HelloWorldTypeSupport::get_instance()->get_typename();
	participant->registe_type(type_name, HelloWorldTypeSupport::get_instance());
	
	// 根据策略创建对应的Topic名称
	std::string topic_name = get_topic_name(strategy);
	topic = participant->create_topic(
		topic_name.c_str(),
		type_name, TOPIC_QOS_DEFAULT, NULL,
		STATUS_MASK_NONE);
	if (topic == NULL) {
		fprintf(stderr, "create_topic error\n");
		publisher_shutdown(participant);
		return -1;
	}

	// 根据用户选择设置QoS策略
	DataWriterQos dw_qos;
	publisher->get_default_datawriter_qos(dw_qos);
	dw_qos.durability.kind = strategy;
	dw_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
	
	writer = publisher->create_datawriter(topic, dw_qos, nullptr, STATUS_MASK_ALL);
	if (writer == NULL) {
		fprintf(stderr, "create_datawriter error\n");
		publisher_shutdown(participant);
		return -1;
	}

	printf("成功创建DataWriter - 策略: %s, Topic: %s\n", strategy_name, topic_name.c_str());
	printf("等待发现完成...\n");
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	
	printf("开始发布 %s 数据...\n", strategy_name);
	for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {
		printf("发送数据 %d (策略: %s)\n", count, strategy_name);
		instance.index = count;
		instance.message = std::string(strategy_name) + " - Hello DDS World! Message #" + std::to_string(count);
		
		retcode = writer->write(&instance, instance_handle);
		if (retcode != RETCODE_OK) {
			fprintf(stderr, "write error %d\n", retcode);
		} else {
			printf("  发送成功: %s\n", instance.message.c_str());
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	printf("%s 发布者完成，保持运行5秒...\n", strategy_name);
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	return publisher_shutdown(participant);
}

extern "C" int subscriber_main(DurabilityQosPolicyKind strategy)
{
	int sample_count = 0;
	DomainParticipant* participant = NULL;
	Subscriber* subscriber = NULL;
	Topic* topic = NULL;
	myDataListener* reader_listener = NULL;
	DataReader* reader = NULL;
	const char* type_name = NULL;
	ReturnCode_t retcode;

	const char* strategy_name = get_strategy_name(strategy);
	printf("=== 启动订阅者 - %s 策略 ===\n", strategy_name);

	participant = TheParticipantFactory->
		create_participant(0, PARTICIPANT_QOS_DEFAULT, NULL, STATUS_MASK_NONE);
	if (participant == NULL) {
		fprintf(stderr, "create_participant error\n");
		return -1;
	}

	subscriber = participant->create_subscriber(
		SUBSCRIBER_QOS_DEFAULT, NULL, STATUS_MASK_NONE);
	if (subscriber == NULL) {
		fprintf(stderr, "create_subscriber error\n");
		return -1;
	}

	type_name = HelloWorldTypeSupport::get_instance()->get_typename();
	participant->registe_type(type_name, HelloWorldTypeSupport::get_instance());
	
	// 根据策略创建对应的Topic名称
	std::string topic_name = get_topic_name(strategy);
	topic = participant->create_topic(
		topic_name.c_str(),
		type_name, TOPIC_QOS_DEFAULT, NULL,
		STATUS_MASK_NONE);
	if (topic == NULL) {
		fprintf(stderr, "create_topic error\n");
		return -1;
	}

	reader_listener = new myDataListener();

	// 根据用户选择设置QoS策略
	DataReaderQos dr_qos;
	subscriber->get_default_datareader_qos(dr_qos);
	dr_qos.durability.kind = strategy;
	dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;

	reader = subscriber->create_datareader(
		topic, dr_qos, reader_listener, STATUS_MASK_ALL);
	if (reader == NULL) {
		fprintf(stderr, "create_datareader error\n");
		delete reader_listener;
		return -1;
	}

	printf("成功创建DataReader - 策略: %s, Topic: %s\n", strategy_name, topic_name.c_str());
	printf("等待 %s 数据 (60秒后自动停止)...\n", strategy_name);

	// 等待数据
	for (int i = 0; i < 60; ++i) {
		printf("等待中... (%d/60) - %s 策略\n", i+1, strategy_name);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	printf("%s 订阅者完成。\n", strategy_name);

	if (participant != NULL) {
		participant->delete_contained_entities();
	}
	TheParticipantFactory->delete_participant(participant);
	delete reader_listener;
	return 0;
}

int main(int argc, char* argv[])
{
	// 如果有命令行参数，使用命令行参数
	if (argc == 3) {
		std::string mode = argv[1];
		std::string strategy_str = argv[2];
		
		DurabilityQosPolicyKind strategy = get_strategy_from_string(strategy_str);
		const char* strategy_name = get_strategy_name(strategy);
		
		printf("=== TravoDDS 持久化策略测试程序 ===\n");
		printf("模式: %s, 策略: %s\n", mode.c_str(), strategy_name);
		
		if (mode == "pub" || mode == "publisher") {
			return publisher_main(strategy);
		}
		else if (mode == "sub" || mode == "subscriber") {
			return subscriber_main(strategy);
		}
		else {
			std::cout << "错误: 无效的模式 '" << mode << "'" << std::endl;
			show_usage(argv[0]);
			return -1;
		}
	}
	// 如果没有参数或参数不对，显示帮助并进入交互模式
	else if (argc == 1) {
		std::cout << "=== TravoDDS 持久化策略测试程序 ===\n" << std::endl;
		show_usage(argv[0]);
		std::cout << "\n进入交互模式..." << std::endl;
		
		// 交互模式
		std::string mode_input, strategy_input;
		
		std::cout << "\n请选择模式 (pub/sub): ";
		std::getline(std::cin, mode_input);
		
		std::cout << "请选择策略 (1-4 或 volatile/transient_local/transient/persistent): ";
		std::getline(std::cin, strategy_input);
		
		DurabilityQosPolicyKind strategy = get_strategy_from_string(strategy_input);
		const char* strategy_name = get_strategy_name(strategy);
		
		printf("\n选择的配置: 模式=%s, 策略=%s\n", mode_input.c_str(), strategy_name);
		
		if (mode_input == "pub" || mode_input == "publisher") {
			return publisher_main(strategy);
		}
		else if (mode_input == "sub" || mode_input == "subscriber") {
			return subscriber_main(strategy);
		}
		else {
			std::cout << "错误: 无效的模式输入" << std::endl;
			return -1;
		}
	}
	else {
		std::cout << "错误: 参数数量不正确" << std::endl;
		show_usage(argv[0]);
		return -1;
	}
}
