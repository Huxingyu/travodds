#include "dcps/domain/domainparticipant.h"
#include "dcps/topic/topic.h"
#include "HelloWorld.h"
#include "HelloWorldTypeSupport.h"
#include <iostream>
#include "dcps/domain/domainparticipantfactory.h"
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

int main(int argc, char* argv[])
{

    std::string type = "";
    if (argc < 2) {
        std::cerr << "input pub、 sub、pubnosec or subnosec" << std::endl;
        std::cin >> type;
    }
    else {
        type = argv[1];
    }
    if (type == "pub")
    {
        DomainParticipantFactory* factory = TheParticipantFactory;
        int retCode;
        retCode = factory->load_security_plugin(
            "SecurityPlugin",
            "libSecurityPlugin.so",
            "SecPluginGetInstance",
            "SecPluginFinalize");
        if (retCode != 0)
        {
            printf("load_security_plugin failed(%d).", retCode);
            return -1;
        }

        int sample_count = 100;
        DomainParticipant* participant = NULL;
        Publisher* publisher = NULL;
        Topic* topic = NULL;
        DataWriter* writer = NULL;
        HelloWorld instance = { 0,"a" };

        Subscriber* subscriber = NULL;
        myDataListener* reader_listener = NULL;
        DataReader* reader = NULL;

        ReturnCode_t retcode;
        InstanceHandle_t instance_handle = HANDLE_NIL;
        const char* type_name = NULL;
        int count = 0;

        {
            factory->load_qos_from_xml("../dds_security_qos.xml");
            participant = factory->create_participant_with_profile(
                0, "default_lib",
                "default_profile", "example_pub", NULL, PUBLICATION_MATCHED_STATUS);
            if (participant == NULL) {
                fprintf(stderr, "create_participant error\n");
                return -1;
            }
            //participant = factory->create_participant(0 , PARTICIPANT_QOS_DEFAULT, NULL, PUBLICATION_MATCHED_STATUS);
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
            "rr/aReply",
            type_name, TOPIC_QOS_DEFAULT, NULL,
            STATUS_MASK_NONE);
        if (topic == NULL) {
            fprintf(stderr, "create_topic error\n");
            publisher_shutdown(participant);
            return -1;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //考虑时序性

        DataWriterQos wrQos;
        publisher->get_default_datawriter_qos(wrQos);
        //wrQos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
        wrQos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        writer = publisher->create_datawriter(topic, wrQos, nullptr, STATUS_MASK_ALL);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {
            if (count % 20 == 0)
                printf("Writing myData, count %d\n", count);
            instance.index = count;
            retcode = writer->write(&instance, instance_handle);
            if (retcode != RETCODE_OK) {
                fprintf(stderr, "write error %d\n", retcode);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        publisher_shutdown(participant);
    }
    else if (type == "sub") {
        DomainParticipantFactory* factory = TheParticipantFactory;
        int retCode;
        retCode = factory->load_security_plugin(
            "SecurityPlugin",
            "libSecurityPlugin.so",
            "SecPluginGetInstance",
            "SecPluginFinalize");
        if (retCode != 0)
        {
            printf("load_security_plugin failed(%d).", retCode);
            return -1;
        }

        int sample_count = 0;
        DomainParticipant* participant = NULL;
        Publisher* publisher = NULL;
        Topic* topic = NULL;
        DataWriter* writer = NULL;
        HelloWorld instance = { 0,"a" };

        Subscriber* subscriber = NULL;
        myDataListener* reader_listener = NULL;
        DataReader* reader = NULL;

        ReturnCode_t retcode;
        InstanceHandle_t instance_handle = HANDLE_NIL;
        const char* type_name = NULL;
        int count = 0;
        
        {
            factory->load_qos_from_xml("../dds_security_qos.xml");
            participant = factory->create_participant_with_profile(
                0, "default_lib",
                "default_profile", "example_pub", NULL, PUBLICATION_MATCHED_STATUS);
            if (participant == NULL) {
                fprintf(stderr, "create_participant error\n");
                return -1;
            }
            //participant = factory->create_participant(0, PARTICIPANT_QOS_DEFAULT, NULL, PUBLICATION_MATCHED_STATUS);
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
            "rr/aReply",
            type_name, TOPIC_QOS_DEFAULT, NULL,
            STATUS_MASK_NONE);
        if (topic == NULL) {
            fprintf(stderr, "create_topic error\n");
            subscriber_shutdown(participant);
            return -1;
        }

        reader_listener = new myDataListener();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //考虑时序性

        DataReaderQos drQos;
        subscriber->get_default_datareader_qos(drQos);
        //drQos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
        drQos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        reader = subscriber->create_datareader(
            topic, drQos, reader_listener,
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

        return subscriber_shutdown(participant);
    }
    else if (type == "pubnosec")
    {
        DomainParticipantFactory* factory = TheParticipantFactory;
        int retCode;
        int sample_count = 0;
        DomainParticipant* participant = NULL;
        Publisher* publisher = NULL;
        Topic* topic = NULL;
        DataWriter* writer = NULL;
        HelloWorld instance = { 0,"a" };
        Subscriber* subscriber = NULL;
        myDataListener* reader_listener = NULL;
        DataReader* reader = NULL;
        ReturnCode_t retcode;
        InstanceHandle_t instance_handle = HANDLE_NIL;
        const char* type_name = NULL;
        int count = 0;
        participant = factory->create_participant(
            0, PARTICIPANT_QOS_DEFAULT, NULL, PUBLICATION_MATCHED_STATUS);
        if (participant == NULL) {
            fprintf(stderr, "create_participant error\n");
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
            "rr/aReply",
            type_name, TOPIC_QOS_DEFAULT, NULL,
            STATUS_MASK_NONE);
        if (topic == NULL) {
            fprintf(stderr, "create_topic error\n");
            publisher_shutdown(participant);
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //考虑时序性
        DataWriterQos wrQos;
        publisher->get_default_datawriter_qos(wrQos);
        //wrQos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
        wrQos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        writer = publisher->create_datawriter(topic, wrQos, nullptr, STATUS_MASK_ALL);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {
            if (count % 20 == 0)
                printf("Writing myData, count %d\n", count);
            instance.index = count;
            retcode = writer->write(&instance, instance_handle);
            if (retcode != RETCODE_OK) {
                fprintf(stderr, "write error %d\n", retcode);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        publisher_shutdown(participant);
    }
    else if (type == "subnosec") {
        DomainParticipantFactory* factory = TheParticipantFactory;
        int retCode;
        int sample_count = 0;
        DomainParticipant* participant = NULL;
        Publisher* publisher = NULL;
        Topic* topic = NULL;
        DataWriter* writer = NULL;
        HelloWorld instance = { 0,"a" };

        Subscriber* subscriber = NULL;
        myDataListener* reader_listener = NULL;
        DataReader* reader = NULL;

        ReturnCode_t retcode;
        InstanceHandle_t instance_handle = HANDLE_NIL;
        const char* type_name = NULL;
        int count = 0;
        participant = factory->create_participant(
            0, PARTICIPANT_QOS_DEFAULT, NULL, PUBLICATION_MATCHED_STATUS);
        if (participant == NULL) {
            fprintf(stderr, "create_participant error\n");
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
            "rr/aReply",
            type_name, TOPIC_QOS_DEFAULT, NULL,
            STATUS_MASK_NONE);
        if (topic == NULL) {
            fprintf(stderr, "create_topic error\n");
            subscriber_shutdown(participant);
            return -1;
        }

        reader_listener = new myDataListener();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //考虑时序性

        DataReaderQos drQos;
        subscriber->get_default_datareader_qos(drQos);
        //drQos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
        drQos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        reader = subscriber->create_datareader(
            topic, drQos, reader_listener,
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

        return subscriber_shutdown(participant);
    }
    else {
        return 0;
    }
}

