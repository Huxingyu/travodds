#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/topic/topic.h"
#include "HelloWorld.h"
#include "HelloWorldTypeSupport.h"

USING_AGIDDS_NAMESPACE

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

extern "C" int publisher_main()
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
	topic = participant->create_topic(
		"HelloWorldTopic",
		type_name, TOPIC_QOS_DEFAULT, NULL,
		STATUS_MASK_NONE);
	if (topic == NULL) {
		fprintf(stderr, "create_topic error\n");
		publisher_shutdown(participant);
		return -1;
	}

	// 创建TRANSIENT QoS for persistence
	DataWriterQos dw_qos;
	publisher->get_default_datawriter_qos(dw_qos);
	dw_qos.durability.kind = TRANSIENT_DURABILITY_QOS;
	dw_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
	
	writer = publisher->create_datawriter(topic, dw_qos, nullptr, STATUS_MASK_ALL);
	if (writer == NULL) {
		fprintf(stderr, "create_datawriter error\n");
		publisher_shutdown(participant);
		return -1;
	}

	printf("Publishing HelloWorld data with TRANSIENT QoS...\n");
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	
	for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {
		printf("Writing myData, count %d\n", count);
		instance.index = count;
		instance.message = "Hello DDS World! Message #" + std::to_string(count);
		
		retcode = writer->write(&instance, instance_handle);
		if (retcode != RETCODE_OK) {
			fprintf(stderr, "write error %d\n", retcode);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	printf("Publisher finished. Keeping alive for a while...\n");
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));

	return publisher_shutdown(participant);
}

extern "C" int subscriber_main()
{
	int sample_count = 0;
	DomainParticipant* participant = NULL;
	Subscriber* subscriber = NULL;
	Topic* topic = NULL;
	myDataListener* reader_listener = NULL;
	DataReader* reader = NULL;
	const char* type_name = NULL;
	ReturnCode_t retcode;

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
	topic = participant->create_topic(
		"HelloWorldTopic",
		type_name, TOPIC_QOS_DEFAULT, NULL,
		STATUS_MASK_NONE);
	if (topic == NULL) {
		fprintf(stderr, "create_topic error\n");
		return -1;
	}

	reader_listener = new myDataListener();

	// 创建TRANSIENT QoS for persistence
	DataReaderQos dr_qos;
	subscriber->get_default_datareader_qos(dr_qos);
	dr_qos.durability.kind = TRANSIENT_DURABILITY_QOS;
	dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;

	reader = subscriber->create_datareader(
		topic, dr_qos, reader_listener, STATUS_MASK_ALL);
	if (reader == NULL) {
		fprintf(stderr, "create_datareader error\n");
		delete reader_listener;
		return -1;
	}

	printf("Subscribing to HelloWorld data with TRANSIENT QoS...\n");
	printf("Waiting for data (press Ctrl+C to stop)...\n");

	// 等待数据
	for (int i = 0; i < 60; ++i) {
		printf("Waiting... (%d/60)\n", i+1);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	if (participant != NULL) {
		participant->delete_contained_entities();
	}
	TheParticipantFactory->delete_participant(participant);
	delete reader_listener;
	return 0;
}

int main()
{
	std::string input;
	std::cout << "Enter 'pub' for publisher or 'sub' for subscriber: ";
	std::getline(std::cin, input);

	if (input == "pub" || input == "publisher") {
		return publisher_main();
	}
	else if (input == "sub" || input == "subscriber") {
		return subscriber_main();
	}
	else {
		std::cout << "Invalid input. Use 'pub' or 'sub'." << std::endl;
		return -1;
	}
}
