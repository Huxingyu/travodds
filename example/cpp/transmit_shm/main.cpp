#include <iostream>
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/topic/topic.h"
#include "HelloWorld.h"
#include "HelloWorldTypeSupport.h"

USING_TRAVODDS_NAMESPACE

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

	//retcode = myData_reader->return_loan(data_seq, info_seq);
	//if (retcode != DDS_RETCODE_OK) {
	//	fprintf(stderr, "return loan error %d\n", retcode);
	//}
}

static int publisher_shutdown(
	DomainParticipant* participant)
{
	ReturnCode_t retcode;
	int status = 0;

	if (participant != NULL) {
		retcode = participant->delete_contained_entities();
		if (retcode != RETCODE_OK) {
			fprintf(stderr, "delete_contained_entities error %d\n", retcode);
			status = -1;
		}

		retcode = TheParticipantFactory->delete_participant(participant);
		if (retcode != RETCODE_OK) {
			fprintf(stderr, "delete_participant error %d\n", retcode);
			status = -1;
		}
	}
	return status;
}
static int subscriber_shutdown(
	DomainParticipant* participant)
{
	ReturnCode_t retcode;
	int status = 0;

	if (participant != NULL) {
		retcode = participant->delete_contained_entities();
		if (retcode != RETCODE_OK) {
			fprintf(stderr, "delete_contained_entities error %d\n", retcode);
			status = -1;
		}

		retcode = TheParticipantFactory->delete_participant(participant);
		if (retcode != RETCODE_OK) {
			fprintf(stderr, "delete_participant error %d\n", retcode);
			status = -1;
		}
	}
	return status;
}
extern "C" int publisher_main()
{
	int sample_count = 0;
	DomainParticipant* participant = NULL;
	Publisher* publisher = NULL;
	Topic* topic = NULL;
	DataWriter* writer = NULL;
	HelloWorld instance = { 0,"hello world"};

	ReturnCode_t retcode;
	InstanceHandle_t instance_handle = HANDLE_NIL;
	const char* type_name = NULL;
	int count = 0;

	// 定制 DomainParticipant 专属 Qos
	DomainParticipantQos qos;
	// 移除默认的 UDPv4 传输
	qos.transport.removeTransport(TRANSPORT_UDPv4_QOS);
	// 添加共享内存传输
	qos.transport.addTransport(TRANSPORT_SHM_QOS);

	participant = TheParticipantFactory->create_participant(
		114, qos,
		NULL, STATUS_MASK_NONE);
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
		"topic",
		type_name, TOPIC_QOS_DEFAULT, NULL,
		STATUS_MASK_NONE);
	if (topic == NULL) {
		fprintf(stderr, "create_topic error\n");
		publisher_shutdown(participant);
		return -1;
	}

	writer = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_ALL);
	
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	
	for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {

		printf("Writing myData, count %d\n", count);
		instance.index = count;
		retcode = writer->write(&instance, instance_handle);
		if (retcode != RETCODE_OK) {
			fprintf(stderr, "write error %d\n", retcode);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

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

	int count = 0;

	// 定制 DomainParticipant 专属 Qos
	DomainParticipantQos qos;
	// 移除默认的 UDPv4 传输
	qos.transport.removeTransport(TRANSPORT_UDPv4_QOS);
	// 添加共享内存传输
	qos.transport.addTransport(TRANSPORT_SHM_QOS);

	participant = TheParticipantFactory->create_participant(
		114, qos,
		NULL, STATUS_MASK_NONE);
	if (participant == NULL) {
		fprintf(stderr, "create_participant error\n");
		subscriber_shutdown(participant);
		return -1;
	}

	subscriber = participant->create_subscriber(
		SUBSCRIBER_QOS_DEFAULT, NULL, STATUS_MASK_NONE);
	if (subscriber == NULL) {
		fprintf(stderr, "create_subscriber error\n");
		subscriber_shutdown(participant);
		return -1;
	}

	type_name = HelloWorldTypeSupport::get_instance()->get_typename();
	participant->registe_type(type_name, HelloWorldTypeSupport::get_instance());

	topic = participant->create_topic(
		"topic",
		type_name, TOPIC_QOS_DEFAULT, NULL,
		STATUS_MASK_NONE);
	if (topic == NULL) {
		fprintf(stderr, "create_topic error\n");
		subscriber_shutdown(participant);
		return -1;
	}

	reader_listener = new myDataListener();

	reader = subscriber->create_datareader(
		topic, DATAREADER_QOS_DEFAULT, reader_listener,
		STATUS_MASK_ALL);
	if (reader == NULL) {
		fprintf(stderr, "create_datareader error\n");
		subscriber_shutdown(participant);
		delete reader_listener;
		return -1;
	}

	/* Main loop */
	for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//break;
	}

	return subscriber_shutdown(participant);;
}

int main(int argc, char* argv[]) {
	std::string type = "";
	if (argc < 2) {
		std::cerr << "input pub or sub" << std::endl;
		std::cin >> type;
	} else {
		type = argv[1];
	}
	if (type == "pub") {
		publisher_main();
	} else if (type == "sub") {
		subscriber_main();
	} else {
		std::cerr << "input pub or sub" << std::endl;
		return -1;
	}

	return 0;
}
