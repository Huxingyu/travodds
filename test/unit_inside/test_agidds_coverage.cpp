#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/condition/statuscondition.h"
#include "testdataTypeSupport.h"
#include "dcps/condition/waitset.h"
#ifdef COVERAGE_TEST
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include "dcps/domain/domainentityimpl.h"
#include "dcps/condition/statusconditionimpl.h"
#include "dcps/condition/guardcondition.h"
#include "dcps/listener/listener.h"
#include "dcps/sql/ddssqlpredicate.h"
#include "dcps/topic/multitopicimpl.h"
#include "dcps/topic/topicdescriptionimpl.h"
#include "dcps/topic/typesupport.h"
#include "rtps/messages/messageprocessor.h"
#include "rtps/endpoint/statelessreader.h"
#include "rtps/endpoint/statelesswriter.h"
#include "rtps/transport/sharedmementity.h"
#define private public
#include "rtps/transport/sharedmemuuid.h"
#undef private
#endif // COVERAGE_TEST
USING_TRAVODDS_NAMESPACE

TEST(Coverage_Test, Coverage_Test_1)
{
    DomainParticipant* par = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    const char* type_name = TestModule::BasicDataTypeSupport::get_instance()->get_typename();
    par->registe_type(type_name, TestModule::BasicDataTypeSupport::get_instance());
    Topic* top = par->create_topic("topicname", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    Publisher* publisher = par->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriterQos dw_qos;
    publisher->get_default_datawriter_qos(dw_qos);
    dw_qos.deadline.period.sec = 1;
    dw_qos.deadline.period.nanosec = 0;
    dw_qos.lifespan.duration.sec = 1;
    dw_qos.lifespan.duration.nanosec = 0;
    DataWriter* dw = publisher->create_datawriter(top, dw_qos, nullptr, STATUS_MASK_NONE);
    //DataWriterQos qos;
    dw_qos.deadline.period.sec = 2;
    dw_qos.lifespan.duration.sec = 2;
    ReturnCode_t result = dw->set_qos(dw_qos);
    EXPECT_EQ(result, RETCODE_OK);
    DataWriterQos qosGet;
    ReturnCode_t resultGet = dw->get_qos(qosGet);
    EXPECT_EQ(resultGet, RETCODE_OK);

    par->delete_contained_entities();
    TheParticipantFactory->delete_participant(par);
}



TEST(Coverage_Test, Coverage_DATA_FRAG_INLINE_QOS_BEST_EFFORT)
{
    const char* type_name = TestModule::KeyDataTypeSupport::get_instance()->get_typename();

    DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_w->registe_type(type_name, TestModule::KeyDataTypeSupport::get_instance());
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
    Topic* top_w = par_w->create_topic("KeyData", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datawriter(top_w, dw_qos, nullptr, STATUS_MASK_NONE);

    DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_r->registe_type(type_name, TestModule::KeyDataTypeSupport::get_instance());
    Topic* top_r = par_r->create_topic("KeyData", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datareader(top_r, DATAREADER_QOS_DEFAULT, nullptr, DATA_AVAILABLE_STATUS);



    std::thread th_2(
        [&] {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        TestModule::KeyData data;
        int len = 6553500;
        char* pData = new char[len + 1];
        memset(pData, 'A', len);
        pData[len] = 0;
        data.data = pData;
        for (int i = 0; i < 5; i++ ) {
            EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
    th_2.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    par_r->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_r);
    par_w->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_w);


}


TEST(Coverage_Test, Coverage_DATA_FRAG_INLINE_QOS_RELAIBLE)
{
    const char* type_name = TestModule::KeyDataTypeSupport::get_instance()->get_typename();

    DomainParticipant* par_w = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_w->registe_type(type_name, TestModule::KeyDataTypeSupport::get_instance());

    Topic* top_w = par_w->create_topic("KeyData", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datawriter(top_w, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    DomainParticipant* par_r = TheParticipantFactory->create_participant(0, PARTICIPANT_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    par_r->registe_type(type_name, TestModule::KeyDataTypeSupport::get_instance());
    Topic* top_r = par_r->create_topic("KeyData", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    DataReaderQos dr_qos;
    dr_qos = DATAREADER_QOS_DEFAULT;
    dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
    DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datareader(top_r, dr_qos, nullptr, DATA_AVAILABLE_STATUS);



    std::thread th_2(
        [&] {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        TestModule::KeyData data;
        int len = 6553500;
        char* pData = new char[len + 1];
        memset(pData, 'A', len);
        pData[len] = 0;
        data.data = pData;
        for (int i = 0; i < 5; i++) {
            EXPECT_EQ(dw->write(&data, HANDLE_NIL), RETCODE_OK);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
    th_2.join();

    par_r->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_r);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    par_w->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_w);

}





TEST(Coverage_Test, Coverage_WRITE_GAP)
{
    const char* type_name = TestModule::KeyDataTypeSupport::get_instance()->get_typename();
    DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
    dp_qos.transport.removeTransport(TRANSPORT_SHM_QOS);
    dp_qos.dynamic_adjust.adjustment_interval = {1,0};
    dp_qos.dynamic_adjust.score_threshold_factor = 1;
    DomainParticipant* par_w = TheParticipantFactory->create_participant(0, dp_qos, nullptr, STATUS_MASK_NONE);
    par_w->registe_type(type_name, TestModule::KeyDataTypeSupport::get_instance());

    Topic* top_w = par_w->create_topic("KeyData", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    dw_qos.resource_limits.max_instances = 3;
    dw_qos.resource_limits.max_samples = 9;
    dw_qos.resource_limits.max_samples_per_instance = 3;
    dw_qos.history.depth = 3;
    DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datawriter(top_w, dw_qos, nullptr, STATUS_MASK_NONE);





    TestModule::KeyData data;
    int len = 10;
    char* pData = new char[len + 1];
    memset(pData, 'A', len);
    pData[len] = 0;
    data.data = pData;

    //while(1)
    //	dw->write(&data, HANDLE_NIL);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    data.userId = 1;
    dw->write(&data, HANDLE_NIL);

    data.userId = 2;
    dw->unregister_instance(&data, HANDLE_NIL);
    data.userId = 4;
    dw->write(&data, HANDLE_NIL);


    std::this_thread::sleep_for(std::chrono::seconds(2));
    //PARTICIPANT_QOS_DEFAULT.discovery.kind = DISCOVER_UDPv4_QOS


    DomainParticipant* par_r = TheParticipantFactory->create_participant(0, dp_qos, nullptr, STATUS_MASK_NONE);
    par_r->registe_type(type_name, TestModule::KeyDataTypeSupport::get_instance());
    Topic* top_r = par_r->create_topic("KeyData", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    DataReaderQos dr_qos;
    dr_qos = DATAREADER_QOS_DEFAULT;
    dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
    dr_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datareader(top_r, dr_qos, nullptr, DATA_AVAILABLE_STATUS);

    std::this_thread::sleep_for(std::chrono::seconds(2));


    //th_2.join();
    par_w->delete_contained_entities();
    par_r->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_w);
    TheParticipantFactory->delete_participant(par_r);


}


//TEST(Coverage_Test, Coverage_SEQLENCE)
//{
//	try {
//		SequenceData<TestModule::BasicData> data_seq;
//
//		data_seq.length(5);
//		data_seq.length(3);
//		data_seq.length();
//		data_seq[10].id;
//	}
//	catch (std::exception& e) {
//
//	}
//
//
//
//}

TEST(Coverage_Test, Coverage_FlowControl)
{
    const char* type_name = TestModule::KeyDataTypeSupport::get_instance()->get_typename();
    DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
    dp_qos.transport.removeTransport(TRANSPORT_SHM_QOS);
    dp_qos.dynamic_adjust.adjustment_interval = { 1,0 };
    dp_qos.dynamic_adjust.score_threshold_factor = 0;
    dp_qos.flow_parameters.maxBytesPerPeriod = 1;
    dp_qos.flow_parameters.maxBytesPerPeriod = 2 * 1024*1024;
    dp_qos.transport.removeTransport(TRANSPORT_SHM_QOS);
    DomainParticipant* par_w = TheParticipantFactory->create_participant(0, dp_qos, nullptr, STATUS_MASK_NONE);
    par_w->registe_type(type_name, TestModule::KeyDataTypeSupport::get_instance());

    Topic* top_w = par_w->create_topic("KeyData", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataWriterQos dw_qos = DATAWRITER_QOS_DEFAULT;
    dw_qos.flow_controller_config.kind = FlowControllerQosKind::SYNC;
    //dw_qos.flow_controller_config.kind = FlowControllerQosKind::SYNC;
    DataWriter* dw = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datawriter(top_w, dw_qos, nullptr, STATUS_MASK_NONE);

    Topic* top_w_2 = par_w->create_topic("KeyData2", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    dw_qos.flow_controller_config.kind = FlowControllerQosKind::ASYNC;
    DataWriter* dw_2 = par_w->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datawriter(top_w, dw_qos, nullptr, STATUS_MASK_NONE);

    DomainParticipant* par_r = TheParticipantFactory->create_participant(0, dp_qos, nullptr, STATUS_MASK_NONE);
    par_r->registe_type(type_name, TestModule::KeyDataTypeSupport::get_instance());
    Topic* top_r = par_r->create_topic("KeyData", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);

    DataReaderQos dr_qos;
    dr_qos = DATAREADER_QOS_DEFAULT;
    DataReader* dr = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datareader(top_r, dr_qos, nullptr, DATA_AVAILABLE_STATUS);

    Topic* top_r_2 = par_r->create_topic("KeyData2", type_name, TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    DataReader* dr_2 = par_r->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE)->
        create_datareader(top_r_2, dr_qos, nullptr, DATA_AVAILABLE_STATUS);

    std::this_thread::sleep_for(std::chrono::seconds(2));


    TestModule::KeyData data;
    int len =  1 * 1024 * 1024;
    char* pData = new char[len + 1];
    memset(pData, 'A', len);
    pData[len] = 0;
    data.data = pData;

    dw_2->write(&data, HANDLE_NIL);
    for (int i = 0; i < 200; i++)
    {
        data.userId = 1;

        dw->write(&data, HANDLE_NIL);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::this_thread::sleep_for(std::chrono::seconds(2));


    //th_2.join();
    par_w->delete_contained_entities();
    par_r->delete_contained_entities();
    TheParticipantFactory->delete_participant(par_w);
    TheParticipantFactory->delete_participant(par_r);



}

#ifdef COVERAGE_TEST

namespace {

void clear_shm_uuid()
{
    using namespace TRAVODDS::InterprocessType;

    named_mutex::remove("TravoddsGlobalUUIDMutex");
    shared_memory_object::remove("TravoddsGlobalUUIDSharedMemory");

    std::string shared_dir;
    ipcdetail::get_shared_dir(shared_dir);
    std::remove((shared_dir + "/TravoddsGlobalUUIDMutex").c_str());
    std::remove((shared_dir + "/TravoddsGlobalUUIDSharedMemory").c_str());
}

bool has_non_zero_byte(const std::array<char, 16>& uuid)
{
    return std::any_of(uuid.begin(), uuid.end(), [](char value) {
        return value != 0;
    });
}

class DefaultDynamicTypeSupport : public TypeSupport {
public:
    void* create_data() override { return nullptr; }
    void delete_data(void* data) override { static_cast<void>(data); }
    int32_t copy_data(void* dst, void* src) override
    {
        static_cast<void>(dst);
        static_cast<void>(src);
        return 0;
    }
    uint32_t get_serialized_data_size(void* data, uint32_t currentAlignment) override
    {
        static_cast<void>(data);
        static_cast<void>(currentAlignment);
        return 0;
    }
    uint32_t get_max_serialized_data_size(void* data, uint32_t currentAlignment) override
    {
        static_cast<void>(data);
        static_cast<void>(currentAlignment);
        return 0;
    }
    int32_t serialize_data(void* data, CdrSerializer* cdr, int32_t endian) override
    {
        static_cast<void>(data);
        static_cast<void>(cdr);
        static_cast<void>(endian);
        return 0;
    }
    int32_t deserialize_data(void* data, CdrDeserializer* cdr, int32_t endian) override
    {
        static_cast<void>(data);
        static_cast<void>(cdr);
        static_cast<void>(endian);
        return 0;
    }
    TypeObject* get_typeobject() override { return nullptr; }
    ReturnCode_t get_instancehandle(
        void* data,
        CdrSerializer* cdr,
        InstanceHandle_t& iHandle,
        bool forceMd5 = false) override
    {
        static_cast<void>(data);
        static_cast<void>(cdr);
        static_cast<void>(iHandle);
        static_cast<void>(forceMd5);
        return RETCODE_OK;
    }
    bool has_key() override { return false; }
    const char* get_typename() override { return "DefaultDynamicTypeSupport"; }
    int32_t MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5 = false) override
    {
        static_cast<void>(data);
        static_cast<void>(iHandle);
        static_cast<void>(forceMd5);
        return 0;
    }
};

}

TEST(Coverage_Test, Coverage_typesupport_default_dynamictype)
{
    DefaultDynamicTypeSupport support;
    TypeSupport* base = &support;
    EXPECT_EQ(base->get_dynamictype(), nullptr);
}

TEST(SharedMemUUID, LocalInstancesCreateReuseAndCleanupSharedUuid)
{
    clear_shm_uuid();

    std::array<char, 16> uuid;
    {
        SharedMemUUID first;
        uuid = first.GetUUID();
        EXPECT_TRUE(has_non_zero_byte(uuid));

        {
            SharedMemUUID second;
            EXPECT_EQ(uuid, second.GetUUID());
        }
    }

    clear_shm_uuid();
}

TEST(SharedMemUUID, NormalCreateExistShm)
{
    // 第一次创建shm，生成对象
    const auto& inst1 = SharedMemUUID::GetInstance();
    auto uuid1 = inst1.GetUUID();
    // 第二次获取实例，shm已存在，走if分支
    const auto& inst2 = SharedMemUUID::GetInstance();
    auto uuid2 = inst2.GetUUID();
    EXPECT_EQ(uuid1, uuid2);
    EXPECT_TRUE(has_non_zero_byte(uuid1));
}

TEST(Coverage_Test, Coverage_NackFragMessage)
{
    SerializedBuffer messageBuf;
    messageBuf.buffer_size = 1024;
    messageBuf.buffer = new char[messageBuf.buffer_size];
    uint32_t shift = 0;
    NackFragSubMessage_t nackFragMessage;
    nackFragMessage.fragmentNumberState.bitmapBase = 1;
    nackFragMessage.fragmentNumberState.bitmap.bits[0] = 1;
    nackFragMessage.fragmentNumberState.bitmap.size = 1;
    ReturnCode_t  ret = MessageProcessor::EncapsulateNackFragMessage(messageBuf, shift, nackFragMessage);
    EXPECT_EQ(ret, RETCODE_OK);
}



TEST(Coverage_Test, Coverage_entity_release)
{
    {
        Entity* entity = new DomainEntityImpl(STATUS_MASK_NONE);
        EXPECT_EQ(entity->enable(), RETCODE_OK);
        delete entity;
    }

    {
        DomainEntity* entity = new DomainEntityImpl(STATUS_MASK_NONE);
        EXPECT_EQ(entity->get_status_changes(), StatusMask(0));
        delete entity;
    }

    {
        DomainEntityImpl owner(STATUS_MASK_NONE);
        Condition* condition = new StatusConditionImpl(&owner);
        EXPECT_FALSE(condition->get_trigger_value());
        delete condition;
    }

    {
        Listener* listener = new Listener();
        delete listener;
    }

    {
        TopicDescription* topicDescription =
            new TopicDescriptionImpl("topic", "BasicData", nullptr);
        EXPECT_STREQ(topicDescription->get_name(), "topic");
        EXPECT_STREQ(topicDescription->get_type_name(), "BasicData");
        EXPECT_EQ(topicDescription->get_participant(), nullptr);
        delete topicDescription;
    }

    {
        Condition* condition = new GuardCondition();
        EXPECT_FALSE(condition->get_trigger_value());
        delete condition;
    }

    {
        auto left = std::make_shared<DDSSQL::TypicalValue>();
        auto right = std::make_shared<DDSSQL::TypicalValue>();
        left->kind = DDSSQL::SIGNED_INTEGER;
        left->signedIntegerValue = 1;
        right->kind = DDSSQL::SIGNED_INTEGER;
        right->signedIntegerValue = 1;

        DDSSQL::DDSSQLPredicate* predicate = new DDSSQL::DDSSQLPredicate(
            DDSSQL::RelOperationKind::EQUAL, left, right);
        predicate->Check();
        EXPECT_TRUE(predicate->GetResult());
        delete predicate;
    }

    {
        StringSeq seq;
        seq.push_back("expression_parameters");

        MultiTopic* mltop = new MultiTopicImpl(
            "mltop",
            "BasicData",
            "subscription_expression",
            seq,
            nullptr);

        EXPECT_EQ(mltop->get_subscription_expression(), "subscription_expression");
        StringSeq seqGet;
        EXPECT_EQ(mltop->get_expression_parameters(seqGet), RETCODE_OK);
        EXPECT_EQ(seqGet, seq);
        delete mltop;
    }

}

TEST(Coverage_Test, Coverage_stateless_reader)
{
    TopicKind_t kind{ NO_KEY };
    auto statelessReader = std::make_unique<StatelessReader>(
        DATAREADER_QOS_DEFAULT, GUID_UNKNOWN, kind);

    Receive_t headerInfo;
    HeartBeatSubMessage_t heartMsg;
    ReturnCode_t ret = statelessReader->ReceiveHeartbeatMessage(headerInfo, heartMsg);
    EXPECT_EQ(ret, RETCODE_UNSUPPORTED);

    HeartBeatFragSubMessage_t heartFragMsg;
    ret = statelessReader->ReceiveHeartFragMessage(headerInfo, heartFragMsg);
    EXPECT_EQ(ret, RETCODE_UNSUPPORTED);

    GapSubMessage_t gapInfo;
    ret = statelessReader->ReceiveGapMessage(headerInfo, gapInfo);
    EXPECT_EQ(ret, RETCODE_UNSUPPORTED);

    EXPECT_FALSE(statelessReader->MatchedWriterIsMatched(GUID_UNKNOWN));
    EXPECT_EQ(statelessReader->MatchedWriterLookup(GUID_UNKNOWN), nullptr);
}

TEST(Coverage_Test, Coverage_stateless_writer)
{
    TopicKind_t kind{ NO_KEY };
    auto statelessWriter = std::make_unique<StatelessWriter>(
        DATAWRITER_QOS_DEFAULT, GUID_UNKNOWN, kind);

    Receive_t headerInfo;
    AcknackSubMessage_t ackMsg;
    ReturnCode_t ret = statelessWriter->ReceiveAckNackMessage(headerInfo, ackMsg);
    EXPECT_EQ(ret, RETCODE_OK);

    NackFragSubMessage_t nackMsg;
    ret = statelessWriter->ReceiveNackFragMessage(headerInfo, nackMsg);
    EXPECT_EQ(ret, RETCODE_OK);
}
#endif // COVERAGE_TEST
