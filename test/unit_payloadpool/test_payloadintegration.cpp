#include "gtest/gtest.h"
#include "HelloWorldTypeSupport.h"
#include "common/memory/topicpayloadpool.h"
#include "common/memory/payloadbuffer.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/reader.h"
#include "rtps/endpoint/statelesswriter.h"
#include <limits>
#include <stdexcept>

USING_TRAVODDS_NAMESPACE

namespace {
// Retain a real generated serializer while controlling its failure contract.
class ControlledTypeSupport : public TypeSupport {
public:
    enum Mode { NORMAL, FAIL, THROW, OVERSIZE } mode = NORMAL;
    HelloWorldTypeSupport* delegate = HelloWorldTypeSupport::get_instance();
    void* create_data() override { return delegate->create_data(); }
    void delete_data(void* p) override { delegate->delete_data(p); }
    int32_t copy_data(void* dst, void* src) override { return delegate->copy_data(dst, src); }
    uint32_t get_serialized_data_size(void* p, uint32_t a) override {
        return mode == OVERSIZE ? std::numeric_limits<uint32_t>::max() : delegate->get_serialized_data_size(p, a);
    }
    uint32_t get_max_serialized_data_size(void* p, uint32_t a) override { return delegate->get_max_serialized_data_size(p, a); }
    int32_t serialize_data(void* p, CdrSerializer* cdr, int32_t e) override {
        if (mode == FAIL) return -1;
        if (mode == THROW) throw std::runtime_error("injected serializer failure");
        return delegate->serialize_data(p, cdr, e);
    }
    int32_t deserialize_data(void* p, CdrDeserializer* cdr, int32_t e) override { return delegate->deserialize_data(p, cdr, e); }
    TypeObject* get_typeobject() override { return nullptr; }
    ReturnCode_t get_instancehandle(void* p, CdrSerializer* cdr, InstanceHandle_t& h, bool m) override {
        return delegate->get_instancehandle(p, cdr, h, m);
    }
    bool has_key() override { return false; }
    const char* get_typename() override { return "PoolIntegrationHelloWorld"; }
    int32_t MakeKey(const void* p, InstanceHandle_t& h, bool m) override { return delegate->MakeKey(p, h, m); }
};

class PoolIntegration : public testing::Test {
protected:
    void SetUp() override {
        DomainParticipantQos qos;
        qos.transport.removeTransport(TRANSPORT_SHM_QOS);
        participant = TheParticipantFactory->create_participant(137, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(participant, nullptr);
        ASSERT_EQ(participant->registe_type(type.get_typename(), &type), RETCODE_OK);
        topic = participant->create_topic("pool-integration", type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(topic, nullptr);
        publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(publisher, nullptr);
        writer = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(writer, nullptr);
        pool = TopicPayloadPoolRegistry::instance().find({137, topic->get_name(), type.get_typename()});
        ASSERT_NE(pool, nullptr);
    }
    void TearDown() override {
        if (participant) {
            EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);
            EXPECT_EQ(TheParticipantFactory->delete_participant(participant), RETCODE_OK);
        }
    }
    DataWriterImpl* impl() { return dynamic_cast<DataWriterImpl*>(writer); }
    std::shared_ptr<Participant> rtps() {
        return dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    }
    ControlledTypeSupport type;
    DomainParticipant* participant = nullptr;
    Topic* topic = nullptr;
    Publisher* publisher = nullptr;
    DataWriter* writer = nullptr;
    std::shared_ptr<ITopicPayloadPool> pool;
};
}

TEST_F(PoolIntegration, RealWritersShareTopicPoolAndIsolateOtherTopics) {
    auto* second = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(second, nullptr);
    auto* otherTopic = participant->create_topic("other-pool-integration", type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(otherTopic, nullptr);
    auto* otherWriter = publisher->create_datawriter(otherTopic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(otherWriter, nullptr);
    auto otherPool = TopicPayloadPoolRegistry::instance().find({137, otherTopic->get_name(), type.get_typename()});
    ASSERT_NE(otherPool, nullptr);
    EXPECT_NE(pool, otherPool);
    SerializedBuffer* first = nullptr;
    SerializedBuffer* next = nullptr;
    ASSERT_EQ(impl()->CreateSerializedBuffer(first, 64), RETCODE_OK);
    PayloadBufferPtr firstOwner(first, PayloadBufferDeleter{pool});
    ASSERT_EQ(dynamic_cast<DataWriterImpl*>(second)->CreateSerializedBuffer(next, 64), RETCODE_OK);
    PayloadBufferPtr nextOwner(next, PayloadBufferDeleter{pool});
    EXPECT_TRUE(pool->owns(first->origin_buffer));
    EXPECT_TRUE(pool->owns(next->origin_buffer));
    EXPECT_FALSE(otherPool->owns(first->origin_buffer));
    EXPECT_EQ(pool->statistics().in_use, 2u);
}

TEST_F(PoolIntegration, FragmentBoundariesPreserveCdrDataAndReturnEveryAllocation) {
    for (uint32_t length : {0u, DATAMESSAGE_LIMIT - CDR_ENDIANNESS_LEN,
            DATAMESSAGE_LIMIT - CDR_ENDIANNESS_LEN + 1, DATAMESSAGE_LIMIT,
            DATAMESSAGE_LIMIT * 3 + 17}) {
        SCOPED_TRACE(length);
        SerializedBuffer* first = nullptr;
        ASSERT_EQ(impl()->CreateSerializedBuffer(first, length), RETCODE_OK);
        PayloadBufferPtr owner(first, PayloadBufferDeleter{pool});
        std::vector<char> input(length, char(0x53));
        CdrSerializer serializer(first);
        ASSERT_TRUE(serializer.serializeBaseTypeArray(input.data(), length));
        CdrDeserializer deserializer(first);
        std::vector<char> output(length);
        ASSERT_TRUE(deserializer.deserializeBaseTypeArray(output.data(), length));
        EXPECT_EQ(input, output);
        uint32_t fragments = 0;
        for (auto* part = first; part; part = part->next) {
            EXPECT_TRUE(pool->owns(part->origin_buffer));
            ++fragments;
        }
        EXPECT_EQ(fragments, (length + CDR_ENDIANNESS_LEN + DATAMESSAGE_LIMIT - 1) / DATAMESSAGE_LIMIT);
        owner.reset();
        EXPECT_EQ(pool->statistics().in_use, 0u);
    }
}

TEST_F(PoolIntegration, PublicPoliciesConfigureRealAllocationsAndPreserveActiveBuffers) {
    int number = 0;
    for (auto policy : {PayloadMemoryPolicy::PREALLOCATED, PayloadMemoryPolicy::PREALLOCATED_WITH_REALLOC,
            PayloadMemoryPolicy::DYNAMIC_RESERVE, PayloadMemoryPolicy::DYNAMIC_REUSABLE}) {
        SCOPED_TRACE(number);
        auto* configuredTopic = participant->create_topic(("public-pool-" + std::to_string(number++)).c_str(),
            type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(configuredTopic, nullptr);
        DataWriterQos qos;
        qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
        qos.payload_pool.policy = policy;
        qos.payload_pool.payload_size = 4096;
        qos.payload_pool.max_payloads = 2;
        qos.payload_pool.initial_payloads = policy == PayloadMemoryPolicy::DYNAMIC_RESERVE ? 0 : 1;
        qos.payload_pool.alignment = 64;
        auto* configured = publisher->create_datawriter(configuredTopic, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(configured, nullptr);
        auto configuredPool = TopicPayloadPoolRegistry::instance().find({137, configuredTopic->get_name(), type.get_typename()});
        ASSERT_NE(configuredPool, nullptr);
        EXPECT_EQ(configuredPool->configuration().policy, policy);
        EXPECT_EQ(configuredPool->configuration().max_payloads, 2u);
        DataWriterQos actual;
        ASSERT_EQ(configured->get_qos(actual), RETCODE_OK);
        EXPECT_EQ(actual.payload_pool, qos.payload_pool);
        auto* implementation = dynamic_cast<DataWriterImpl*>(configured);
        SerializedBuffer* a = nullptr; SerializedBuffer* b = nullptr; SerializedBuffer* c = nullptr;
        ASSERT_EQ(implementation->CreateSerializedBuffer(a, 64), RETCODE_OK);
        PayloadBufferPtr first(a, PayloadBufferDeleter{configuredPool});
        ASSERT_EQ(implementation->CreateSerializedBuffer(b, 64), RETCODE_OK);
        PayloadBufferPtr second(b, PayloadBufferDeleter{configuredPool});
        b->buffer[0] = 'p';
        EXPECT_EQ(reinterpret_cast<uintptr_t>(a->origin_buffer) % 64, 0u);
        EXPECT_EQ(implementation->CreateSerializedBuffer(c, 64), RETCODE_APPLY_MEM_ERR);
        EXPECT_EQ(c, nullptr);
        HelloWorld sample; sample.index = 79; sample.message = "configured";
        EXPECT_EQ(configured->write(&sample, HANDLE_NIL), RETCODE_APPLY_MEM_ERR);
        first.reset();
        EXPECT_EQ(b->buffer[0], 'p');
        EXPECT_EQ(configured->write(&sample, HANDLE_NIL), RETCODE_OK);
        actual.payload_pool.max_payloads = 3;
        EXPECT_EQ(configured->set_qos(actual), RETCODE_IMMUTABLE_POLICY);
        ASSERT_EQ(publisher->delete_datawriter(configured), RETCODE_OK);
        EXPECT_EQ(b->buffer[0], 'p');
        second.reset();
        EXPECT_EQ(configuredPool->statistics().in_use, 0u);
        EXPECT_EQ(configuredPool->statistics().allocated_payloads,
            policy == PayloadMemoryPolicy::DYNAMIC_RESERVE ? 0u : 2u);
    }
}

TEST_F(PoolIntegration, IncompatiblePublicPoolConfigurationReturnsNullAndKeepsExistingWriter) {
    DataWriterQos qos;
    ASSERT_EQ(writer->get_qos(qos), RETCODE_OK);
    auto* compatible = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(compatible, nullptr);
    qos.payload_pool.max_payloads = 2;
    DataWriter* incompatible = nullptr;
    EXPECT_NO_THROW(incompatible = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE));
    EXPECT_EQ(incompatible, nullptr);
    HelloWorld sample; sample.index = 3; sample.message = "still valid";
    EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
    ASSERT_EQ(publisher->delete_datawriter(compatible), RETCODE_OK);
}

TEST_F(PoolIntegration, SerializationFailureExceptionAndOverflowRollBackStorage) {
    HelloWorld value;
    value.index = 27;
    value.message.assign(DATAMESSAGE_LIMIT * 3, 'x');
    type.mode = ControlledTypeSupport::FAIL;
    EXPECT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_ERROR);
    EXPECT_EQ(pool->statistics().in_use, 0u);
    type.mode = ControlledTypeSupport::THROW;
    EXPECT_THROW(writer->write(&value, HANDLE_NIL), std::runtime_error);
    EXPECT_EQ(pool->statistics().in_use, 0u);
    type.mode = ControlledTypeSupport::OVERSIZE;
    EXPECT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_BAD_PARAMETER);
    EXPECT_EQ(pool->statistics().in_use, 0u);
    EXPECT_EQ(writer->write(nullptr, HANDLE_NIL), RETCODE_BAD_PARAMETER);
    type.mode = ControlledTypeSupport::NORMAL;
    EXPECT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_OK);
}

TEST_F(PoolIntegration, ActualWriteTransfersFragmentChainToHistoryWithSampleIdentity) {
    HelloWorld value;
    value.index = 197;
    value.message.assign(DATAMESSAGE_LIMIT * 2 + 17, 'h');
    SampleIdentity identity;
    ASSERT_EQ(writer->write_w_related_sample_identity(&value, HANDLE_NIL, identity, SampleIdentity{}), RETCODE_OK);
    auto endpoint = rtps()->GetWriter(Conversion::ToGuid(writer->get_instance_handle()).entityId);
    ASSERT_NE(endpoint, nullptr);
    auto change = endpoint->GetHistoryCache()->GetChange(identity.sequence_number);
    ASSERT_NE(change, nullptr);
    EXPECT_EQ(identity.writer_handle, writer->get_instance_handle());
    EXPECT_TRUE(pool->owns(change->serializedBuffer->origin_buffer));
    SerializedBuffer data = *change->serializedBuffer;
    data.buffer += CDR_ENDIANNESS_LEN;
    data.buffer_size -= CDR_ENDIANNESS_LEN;
    CdrDeserializer deserializer(&data);
    HelloWorld result;
    ASSERT_EQ(type.deserialize_data(&result, &deserializer, DEFAULT_ENDIAN), 0);
    EXPECT_EQ(result.index, value.index);
    EXPECT_EQ(result.message, value.message);
    EXPECT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    EXPECT_TRUE(pool->owns(change->serializedBuffer->origin_buffer));
    endpoint.reset();
    change.reset();
    EXPECT_EQ(pool->statistics().in_use, 0u);
}

TEST(PayloadPoolHistory, TakingRemovesAllHistoryReferencesButRetainsReturnedData) {
    PayloadPoolConfig config;
    config.max_payloads = 1;
    auto pool = std::make_shared<TopicPayloadPool>(config);
    HistoryCache history(HistoryQosPolicy{}, ResourceLimitsQosPolicy{}, TopicKind_t{NO_KEY});
    auto sample = std::make_shared<CacheChange>(pool);
    sample->serializedBuffer = new SerializedBuffer;
    ASSERT_TRUE(pool->getBuffer(64, sample->serializedBuffer->origin_buffer));
    sample->sequenceNumber = {0, 1};
    ASSERT_TRUE(history.AddChange(sample));
    auto sequence = sample->sequenceNumber;
    sample.reset();
    std::vector<std::shared_ptr<CacheChange>> taken;
    SampleInfoSeq infos;
    ASSERT_EQ(history.ReadOrTakeInstance(taken, infos, 1, HANDLE_NIL,
        ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, true), RETCODE_OK);
    ASSERT_EQ(taken.size(), 1u);
    EXPECT_EQ(history.GetChange(sequence), nullptr);
    EXPECT_EQ(pool->statistics().in_use, 1u);
    EXPECT_FALSE(history.IsFull());
    EXPECT_EQ(history.GetSampleCounters().sampleRead, 0u);
    taken.clear();
    EXPECT_EQ(pool->statistics().in_use, 0u);
}

namespace {
class TakePoolIntegration : public PoolIntegration, public testing::WithParamInterface<int> {};
}

TEST_P(TakePoolIntegration, EveryTakeVariantKeepsPayloadAliveUntilLastCacheReference) {
    auto* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(subscriber, nullptr);
    auto* reader = subscriber->create_datareader(topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);
    auto endpoint = rtps()->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
    ASSERT_NE(endpoint, nullptr);
    InstanceHandle_t handle = HANDLE_NIL;
    handle.keyHash.value[0] = 1; // Exercise instance iteration as well as HANDLE_NIL.
    endpoint->new_change(ALIVE, nullptr, handle, HANDLE_NIL, TIME_ZERO);
    auto change = std::make_shared<CacheChange>(pool);
    HelloWorld input;
    input.index = 49;
    input.message = "retained payload";
    SerializedBuffer* serialized = nullptr;
    ASSERT_EQ(impl()->CreateSerializedBuffer(serialized, type.get_serialized_data_size(&input, 0)), RETCODE_OK);
    change->serializedBuffer = serialized;
    CdrSerializer serializer(serialized);
    ASSERT_EQ(type.serialize_data(&input, &serializer, DEFAULT_ENDIAN), 0);
    serialized->buffer -= CDR_ENDIANNESS_LEN;
    serialized->buffer_size += CDR_ENDIANNESS_LEN;
    // A received RTPS descriptor counts encapsulation bytes in its length.
    serialized->writer_position += CDR_ENDIANNESS_LEN;
    change->instanceHandle = handle;
    change->sequenceNumber = {0, 1};
    ASSERT_EQ(endpoint->AddChanges(change), RETCODE_OK);
    char* address = serialized->origin_buffer;
    SequenceData<HelloWorld> values;
    SampleInfoSeq infos;
    HelloWorld single;
    SampleInfo info;
    auto* condition = reader->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    ASSERT_NE(condition, nullptr);
    ReturnCode_t result = RETCODE_ERROR;
    switch (GetParam()) {
        case 0: result = reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
        case 1: result = reader->take_w_condition(values, infos, 1, condition); break;
        case 2: result = reader->take_next_sample(&single, info); break;
        case 3: result = reader->take_instance(values, infos, 1, handle, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
        case 4: result = reader->take_next_instance(values, infos, 1, HANDLE_NIL, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
        case 5: result = reader->take_next_instance_w_condition(values, infos, 1, HANDLE_NIL, condition); break;
    }
    ASSERT_EQ(result, RETCODE_OK);
    if (GetParam() == 2) {
        EXPECT_EQ(single.index, input.index);
        EXPECT_EQ(single.message, input.message);
    } else {
        ASSERT_EQ(values.length(), 1u);
        EXPECT_EQ(values[0].index, input.index);
        EXPECT_EQ(values[0].message, input.message);
    }
    EXPECT_TRUE(pool->owns(address));
    EXPECT_EQ(pool->statistics().in_use, 1u);
    auto counts = endpoint->GetReaderCache()->GetSampleCounters();
    EXPECT_EQ(counts.sampleRead, 0u);
    EXPECT_EQ(counts.sampleNotRead, 0u);
    EXPECT_EQ(counts.instanceAlive, 0u);
    change.reset();
    EXPECT_EQ(pool->statistics().in_use, 0u);
    EXPECT_EQ(reader->delete_readcondition(condition), RETCODE_OK);
}

INSTANTIATE_TEST_SUITE_P(AllTakeMethods, TakePoolIntegration, testing::Range(0, 6));

TEST(PayloadPoolHistory, PartialFragmentAllocationFailureReturnsEveryEarlierFragment) {
    PayloadPoolConfig config;
    config.max_payloads = 2;
    auto pool = std::make_shared<TopicPayloadPool>(config);
    SerializedBuffer* output = nullptr;
    EXPECT_EQ(CreateSerializedPayload(pool, DATAMESSAGE_LIMIT * 3, output), RETCODE_APPLY_MEM_ERR);
    EXPECT_EQ(output, nullptr);
    EXPECT_EQ(pool->statistics().in_use, 0u);
    ASSERT_EQ(CreateSerializedPayload(pool, DATAMESSAGE_LIMIT, output), RETCODE_OK);
    PayloadBufferPtr owner(output, PayloadBufferDeleter{pool});
    EXPECT_EQ(pool->statistics().in_use, 2u);
    owner.reset();
    EXPECT_EQ(pool->statistics().in_use, 0u);
}

TEST(PayloadPoolHistory, FailedFirstAllocationAndInvalidPoolDoNotPublishAChain) {
    PayloadPoolConfig config;
    config.max_payloads = 1;
    auto pool = std::make_shared<TopicPayloadPool>(config);
    char* occupied = nullptr;
    ASSERT_TRUE(pool->getBuffer(64, occupied));
    SerializedBuffer* output = nullptr;
    EXPECT_EQ(CreateSerializedPayload(pool, 64, output), RETCODE_APPLY_MEM_ERR);
    EXPECT_EQ(output, nullptr);
    EXPECT_EQ(pool->statistics().in_use, 1u);
    EXPECT_TRUE(pool->releaseBuffer(occupied));
    EXPECT_EQ(CreateSerializedPayload(nullptr, 64, output), RETCODE_BAD_PARAMETER);
    EXPECT_EQ(output, nullptr);
}
