#include "gtest/gtest.h"
#include "common/memory/topicpayloadpool.h"
#include "common/memory/payloadbuffer.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/reader.h"
#include "qos/qospolicycheck.h"
#include "xml/dds_xml_repo.h"
#include <chrono>
#include <cstring>
#include <fstream>
#include <thread>
#include <unistd.h>

USING_TRAVODDS_NAMESPACE

namespace {
struct ReaderPoolSample {
    uint32_t number = 0;
    std::string text;
};

// Independent of generated example code; deliberately uses ordinary CDR.
class ReaderPoolSupport : public TypeSupport {
public:
    bool keyed = false;
    void* create_data() override { return new ReaderPoolSample; }
    void delete_data(void* value) override { delete static_cast<ReaderPoolSample*>(value); }
    int32_t copy_data(void* destination, void* source) override {
        *static_cast<ReaderPoolSample*>(destination) = *static_cast<ReaderPoolSample*>(source);
        return 0;
    }
    uint32_t get_serialized_data_size(void* value, uint32_t alignment) override {
        const auto initial = alignment;
        alignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), alignment);
        alignment += CdrSerializer::getStringSize(static_cast<ReaderPoolSample*>(value)->text.size() + 1, alignment);
        return alignment - initial;
    }
    uint32_t get_max_serialized_data_size(void*, uint32_t) override { return LENGTH_UNLIMITED; }
    int32_t serialize_data(void* value, CdrSerializer* cdr, int32_t) override {
        const auto& sample = *static_cast<ReaderPoolSample*>(value);
        return cdr->serializeBaseType(sample.number) && cdr->serializeString(sample.text) ? 0 : -1;
    }
    int32_t deserialize_data(void* value, CdrDeserializer* cdr, int32_t) override {
        auto& sample = *static_cast<ReaderPoolSample*>(value);
        return cdr->deserializeBaseType(sample.number) && cdr->deserializeString(sample.text) ? 0 : -1;
    }
    TypeObject* get_typeobject() override { return nullptr; }
    ReturnCode_t get_instancehandle(void* value, CdrSerializer*, InstanceHandle_t& handle, bool force) override {
        return MakeKey(value, handle, force) == 0 ? RETCODE_OK : RETCODE_ERROR;
    }
    int32_t MakeKey(const void* value, InstanceHandle_t& handle, bool) override {
        handle = HANDLE_NIL;
        if (keyed) std::memcpy(handle.keyHash.value, &static_cast<const ReaderPoolSample*>(value)->number, sizeof(uint32_t));
        return 0;
    }
    bool has_key() override { return keyed; }
    const char* get_typename() override { return "ReaderPoolSample"; }
};

template<class Predicate> bool waitForReaderPool(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    do {
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

class ReaderPayloadPool : public testing::Test {
protected:
    void SetUp() override {
        DomainParticipantQos qos;
        qos.transport.removeTransport(TRANSPORT_SHM_QOS);
        participant = TheParticipantFactory->create_participant(145, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(participant, nullptr);
        ASSERT_EQ(participant->registe_type(type.get_typename(), &type), RETCODE_OK);
        topic = participant->create_topic("reader-payload-pool", type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(topic, nullptr);
        publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_TRUE(publisher && subscriber);
    }
    void TearDown() override {
        if (remote) {
            EXPECT_EQ(remote->delete_contained_entities(), RETCODE_OK);
            EXPECT_EQ(TheParticipantFactory->delete_participant(remote), RETCODE_OK);
        }
        if (participant) {
            EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);
            EXPECT_EQ(TheParticipantFactory->delete_participant(participant), RETCODE_OK);
        }
    }
    DataReaderQos readerQos(uint32_t capacity = UINT32_MAX) {
        DataReaderQos qos;
        qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
        qos.payload_pool.max_payloads = capacity;
        return qos;
    }
    DataReader* createReader(const DataReaderQos& qos) {
        return subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
    }
    std::shared_ptr<Reader> endpoint(DataReader* reader) {
        return dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
            Conversion::ToGuid(reader->get_instance_handle()).entityId);
    }
    std::shared_ptr<ITopicPayloadPool> topicPool() {
        return TopicPayloadPoolRegistry::instance().find({145, topic->get_name(), type.get_typename()});
    }
    std::shared_ptr<CacheChange> received(const std::shared_ptr<IPayloadPool>& sourcePool,
            ReaderPoolSample sample, uint32_t sequence = 1) {
        SerializedBuffer* data = nullptr;
        if (CreateSerializedPayload(sourcePool, type.get_serialized_data_size(&sample, 0), data) != RETCODE_OK)
            return nullptr;
        PayloadBufferPtr owner(data, PayloadBufferDeleter{sourcePool});
        CdrSerializer cdr(data);
        if (type.serialize_data(&sample, &cdr, DEFAULT_ENDIAN) != 0) return nullptr;
        data->buffer -= CDR_ENDIANNESS_LEN;
        data->buffer_size += CDR_ENDIANNESS_LEN;
        data->writer_position += CDR_ENDIANNESS_LEN;
        auto change = std::make_shared<CacheChange>(sourcePool);
        change->serializedBuffer = owner.release();
        change->sequenceNumber = {0, sequence};
        type.MakeKey(&sample, change->instanceHandle, true);
        change->sampleInfo.instance_handle = change->instanceHandle;
        change->sampleInfo.sample_state = NOT_READ_SAMPLE_STATE;
        Time_t::now(change->sourceTimespace);
        return change;
    }
    void expectSample(DataReader* reader, const ReaderPoolSample& expected, const InstanceHandle_t& handle = HANDLE_NIL) {
        ReaderPoolSample actual;
        SampleInfo info;
        ASSERT_EQ(reader->take_next_sample(&actual, info), RETCODE_OK);
        EXPECT_EQ(actual.number, expected.number);
        EXPECT_EQ(actual.text, expected.text);
        EXPECT_EQ(info.instance_handle, handle);
    }
    ReaderPoolSupport type;
    DomainParticipant* participant = nullptr;
    DomainParticipant* remote = nullptr;
    Topic* topic = nullptr;
    Publisher* publisher = nullptr;
    Subscriber* subscriber = nullptr;
};
}

TEST_F(ReaderPayloadPool, ReaderFirstSharesCompatiblePoolWithWriterAndOtherReaders) {
    auto* reader = createReader(readerQos());
    ASSERT_NE(reader, nullptr);
    auto pool = topicPool();
    ASSERT_NE(pool, nullptr);
    auto* another = createReader(readerQos());
    ASSERT_NE(another, nullptr);
    DataWriterQos qos; qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
    auto* writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    SerializedBuffer* writerData = nullptr;
    ASSERT_EQ(dynamic_cast<DataWriterImpl*>(writer)->CreateSerializedBuffer(writerData, 16), RETCODE_OK);
    PayloadBufferPtr owner(writerData, PayloadBufferDeleter{pool});
    EXPECT_TRUE(pool->owns(writerData->origin_buffer));
    auto source = std::make_shared<TopicPayloadPool>(PayloadPoolConfig{});
    auto first = received(source, {7, "first"});
    auto second = received(source, {8, "second"});
    ASSERT_EQ(endpoint(reader)->AddChanges(first), RETCODE_OK);
    ASSERT_EQ(endpoint(another)->AddChanges(second), RETCODE_OK);
    EXPECT_TRUE(pool->owns(first->serializedBuffer->origin_buffer));
    EXPECT_TRUE(pool->owns(second->serializedBuffer->origin_buffer));
    EXPECT_EQ(pool->statistics().in_use, 3u);
    EXPECT_EQ(source->statistics().in_use, 0u);
}

TEST_F(ReaderPayloadPool, ConflictingReaderConfigurationFailsWithoutChangingExistingPool) {
    auto* reader = createReader(readerQos(2));
    ASSERT_NE(reader, nullptr);
    auto pool = topicPool();
    DataReader* conflict = nullptr;
    EXPECT_NO_THROW(conflict = createReader(readerQos(3)));
    EXPECT_EQ(conflict, nullptr);
    EXPECT_EQ(topicPool(), pool);
    auto change = received(MemoryPool::GetInstance(), {9, "still usable"});
    ASSERT_EQ(endpoint(reader)->AddChanges(change), RETCODE_OK);
    expectSample(reader, {9, "still usable"});
    DataReaderQos actual;
    ASSERT_EQ(reader->get_qos(actual), RETCODE_OK);
    actual.payload_pool.max_payloads = 3;
    EXPECT_EQ(reader->set_qos(actual), RETCODE_IMMUTABLE_POLICY);
}

TEST_F(ReaderPayloadPool, MultiTopicReadersUseResultIdentityAndPreserveReferencedLifetime) {
    StringSeq parameters;
    auto* multi = participant->create_multitopic("reader-pool-multitopic", type.get_typename(),
        "SELECT number, text FROM reader-payload-pool", parameters);
    ASSERT_NE(multi, nullptr);
    auto* first = subscriber->create_datareader(multi, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(first, nullptr);
    auto* second = subscriber->create_datareader(multi, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(second, nullptr);
    auto pool = TopicPayloadPoolRegistry::instance().find({145, multi->get_name(), type.get_typename()});
    ASSERT_NE(pool, nullptr);
    EXPECT_EQ(pool->configuration().max_payloads, UINT32_MAX);
    DataReader* conflict = nullptr;
    EXPECT_NO_THROW(conflict = subscriber->create_datareader(multi, readerQos(2), nullptr, STATUS_MASK_NONE));
    EXPECT_EQ(conflict, nullptr);
    // A failed conflicting enable must release only its own Topic reference.
    EXPECT_EQ(participant->delete_multitopic(multi), RETCODE_PRECONDITION_NOT_MET);
    auto source = std::make_shared<TopicPayloadPool>(PayloadPoolConfig{});
    auto firstChange = received(source, {10, "first MultiTopic result"});
    auto secondChange = received(source, {11, "second MultiTopic result"});
    ASSERT_EQ(endpoint(first)->AddChanges(firstChange), RETCODE_OK);
    ASSERT_EQ(endpoint(second)->AddChanges(secondChange), RETCODE_OK);
    EXPECT_TRUE(pool->owns(firstChange->serializedBuffer->origin_buffer));
    EXPECT_TRUE(pool->owns(secondChange->serializedBuffer->origin_buffer));
    EXPECT_EQ(source->statistics().in_use, 0u);
    EXPECT_EQ(pool->statistics().in_use, 2u);
    expectSample(first, {10, "first MultiTopic result"});
    expectSample(second, {11, "second MultiTopic result"});
    ASSERT_EQ(subscriber->delete_datareader(first), RETCODE_OK);
    EXPECT_EQ(participant->delete_multitopic(multi), RETCODE_PRECONDITION_NOT_MET);
    ASSERT_EQ(subscriber->delete_datareader(second), RETCODE_OK);
    EXPECT_EQ(participant->delete_multitopic(multi), RETCODE_OK);
    EXPECT_EQ(multi, nullptr);
    // Result caches retain the original pool after both Readers and MultiTopic
    // are deleted; returning them does not depend on a live TopicDescription.
    EXPECT_EQ(pool->statistics().in_use, 2u);
    firstChange.reset();
    secondChange.reset();
    EXPECT_EQ(pool->statistics().in_use, 0u);
}

TEST_F(ReaderPayloadPool, AllFourReaderPoliciesControlActualPayloadAllocations) {
    unsigned index = 0;
    for (const auto policy : {PayloadMemoryPolicy::PREALLOCATED, PayloadMemoryPolicy::PREALLOCATED_WITH_REALLOC,
            PayloadMemoryPolicy::DYNAMIC_RESERVE, PayloadMemoryPolicy::DYNAMIC_REUSABLE}) {
        SCOPED_TRACE(index);
        auto* configuredTopic = participant->create_topic("reader-policy-" + std::to_string(index++),
            type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(configuredTopic, nullptr);
        auto qos = readerQos(2);
        qos.payload_pool.policy = policy;
        qos.payload_pool.payload_size = 4096;
        qos.payload_pool.alignment = 64;
        auto* reader = subscriber->create_datareader(configuredTopic, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(reader, nullptr);
        auto pool = TopicPayloadPoolRegistry::instance().find({145, configuredTopic->get_name(), type.get_typename()});
        ASSERT_NE(pool, nullptr);
        auto change = received(MemoryPool::GetInstance(), {17, "configured reader"});
        ASSERT_EQ(endpoint(reader)->AddChanges(change), RETCODE_OK);
        EXPECT_TRUE(pool->owns(change->serializedBuffer->origin_buffer));
        EXPECT_EQ(reinterpret_cast<uintptr_t>(change->serializedBuffer->origin_buffer) % 64, 0u);
        EXPECT_EQ(pool->statistics().in_use, 1u);
        DataReaderQos actual;
        ASSERT_EQ(reader->get_qos(actual), RETCODE_OK);
        EXPECT_EQ(actual.payload_pool, qos.payload_pool);
        expectSample(reader, {17, "configured reader"});
        change.reset();
        EXPECT_EQ(pool->statistics().in_use, 0u);
        if (policy == PayloadMemoryPolicy::DYNAMIC_RESERVE)
            EXPECT_EQ(pool->statistics().allocated_payloads, 0u);
    }
}

TEST_F(ReaderPayloadPool, FragmentExhaustionRollsBackBeforeRegisteringTheInstance) {
    type.keyed = true;
    auto* reader = createReader(readerQos(2));
    ASSERT_NE(reader, nullptr);
    auto rtpsReader = endpoint(reader);
    auto pool = topicPool();
    auto source = std::make_shared<TopicPayloadPool>(PayloadPoolConfig{});
    auto large = received(source, {1, std::string(DATAMESSAGE_LIMIT * 2 + 17, 'f')});
    ASSERT_NE(large, nullptr);
    auto* original = large->serializedBuffer;
    EXPECT_EQ(source->statistics().in_use, 3u);
    EXPECT_EQ(rtpsReader->AddChanges(large), RETCODE_APPLY_MEM_ERR);
    EXPECT_EQ(large->serializedBuffer, original);
    EXPECT_EQ(pool->statistics().in_use, 0u);
    EXPECT_EQ(source->statistics().in_use, 3u);
    EXPECT_FALSE(rtpsReader->GetReaderCache()->IsInstancePresent(large->instanceHandle));
    InstanceHandle_t next;
    EXPECT_EQ(rtpsReader->FindNextInstance(HANDLE_NIL, next), RETCODE_ERROR);
    auto small = received(source, {2, "fits"}, 2);
    ASSERT_EQ(rtpsReader->AddChanges(small), RETCODE_OK);
    expectSample(reader, {2, "fits"}, small->instanceHandle);
}

TEST_F(ReaderPayloadPool, FragmentedSamplesCanBeReadRepeatedlyBeforeTake) {
    auto* reader = createReader(readerQos());
    ASSERT_NE(reader, nullptr);
    ReaderPoolSample input{79, std::string(DATAMESSAGE_LIMIT * 2 + 17, 'c')};
    auto change = received(MemoryPool::GetInstance(), input);
    ASSERT_EQ(endpoint(reader)->AddChanges(change), RETCODE_OK);
    for (unsigned i = 0; i < 2; ++i) {
        SequenceData<ReaderPoolSample> values;
        SampleInfoSeq infos;
        ASSERT_EQ(reader->read(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
        ASSERT_EQ(values.length(), 1u);
        EXPECT_EQ(values[0].number, input.number);
        EXPECT_EQ(values[0].text, input.text);
    }
    // take_next_sample selects NOT_READ; these samples have already been read.
    SequenceData<ReaderPoolSample> taken;
    SampleInfoSeq infos;
    ASSERT_EQ(reader->take(taken, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    ASSERT_EQ(taken.length(), 1u);
    ASSERT_EQ(infos.length(), 1u);
    EXPECT_EQ(infos[0].sample_state, READ_SAMPLE_STATE);
    EXPECT_EQ(taken[0].number, input.number);
    EXPECT_EQ(taken[0].text, input.text);
    for (auto* part = change->serializedBuffer; part; part = part->next)
        EXPECT_EQ(part->reader_position, 0u);
}

TEST_F(ReaderPayloadPool, ExhaustionPreservesThePreviousKeepLastSample) {
    auto* reader = createReader(readerQos(2));
    ASSERT_NE(reader, nullptr);
    auto pool = topicPool();
    char* busy = nullptr;
    ASSERT_TRUE(pool->getBuffer(16, busy));
    auto first = received(MemoryPool::GetInstance(), {3, "retained"});
    auto second = received(MemoryPool::GetInstance(), {4, "cannot evict on failure"}, 2);
    auto rtpsReader = endpoint(reader);
    ASSERT_EQ(rtpsReader->AddChanges(first), RETCODE_OK);
    EXPECT_EQ(rtpsReader->AddChanges(second), RETCODE_APPLY_MEM_ERR);
    expectSample(reader, {3, "retained"});
    EXPECT_EQ(pool->statistics().in_use, 2u);
    first.reset();
    EXPECT_EQ(pool->statistics().in_use, 1u);
    ASSERT_EQ(rtpsReader->AddChanges(second), RETCODE_OK);
    expectSample(reader, {4, "cannot evict on failure"});
    EXPECT_TRUE(pool->releaseBuffer(busy));
}

TEST_F(ReaderPayloadPool, RejectsKeepLastBudgetsWhichCannotStageOneFragmentReplacement) {
    EXPECT_EQ(createReader(readerQos(1)), nullptr);
    auto qos = readerQos(2);
    qos.history.depth = 2;
    EXPECT_EQ(createReader(qos), nullptr);
    EXPECT_EQ(topicPool(), nullptr);
    // KEEP_ALL expresses explicit consumption/backpressure and needs no
    // automatic history replacement, so a single fragment budget is valid.
    qos = readerQos(1);
    qos.history.kind = KEEP_ALL_HISTORY_QOS;
    EXPECT_NE(createReader(qos), nullptr);
}

TEST_F(ReaderPayloadPool, KeepLastReplacesRepeatedlyWithOneStagingSlotAndNoTake) {
    auto* reader = createReader(readerQos(2));
    ASSERT_NE(reader, nullptr);
    auto rtpsReader = endpoint(reader);
    auto pool = topicPool();
    for (uint32_t sequence = 1; sequence <= 20; ++sequence) {
        auto change = received(MemoryPool::GetInstance(), {sequence, "latest"}, sequence);
        ASSERT_EQ(rtpsReader->AddChanges(change), RETCODE_OK);
        EXPECT_EQ(pool->statistics().in_use, 1u);
    }
    expectSample(reader, {20, "latest"});
    EXPECT_EQ(pool->statistics().in_use, 0u);
}

TEST_F(ReaderPayloadPool, SuccessfulAdmissionReleasesTheOriginalTransportMessage) {
    auto* reader = createReader(readerQos());
    ASSERT_NE(reader, nullptr);
    auto prepared = received(MemoryPool::GetInstance(), {5, "extracted CDR"});
    ASSERT_NE(prepared, nullptr);
    const auto length = prepared->serializedBuffer->buffer_size;
    auto packet = std::shared_ptr<Message_t>(new Message_t(new char[length + 64], length + 64), [](Message_t* message) {
        delete[] message->originBuffer;
        delete message;
    });
    std::memcpy(packet->buffer + 32, prepared->serializedBuffer->buffer, length);
    std::weak_ptr<Message_t> transport = packet;
    auto* view = new CacheSerializedBuffer;
    view->buffer = packet->buffer + 32;
    view->buffer_size = length;
    view->writer_position = length;
    view->originMessage = packet;
    auto change = endpoint(reader)->new_change(ALIVE, view, HANDLE_NIL, HANDLE_NIL, TIME_INVALID);
    packet.reset();
    ASSERT_FALSE(transport.expired());
    ASSERT_EQ(endpoint(reader)->AddChanges(change), RETCODE_OK);
    EXPECT_TRUE(transport.expired());
    EXPECT_EQ(change->serializedBuffer->buffer_size, length);
    EXPECT_TRUE(topicPool()->owns(change->serializedBuffer->origin_buffer));
    expectSample(reader, {5, "extracted CDR"});
}

TEST_F(ReaderPayloadPool, TakenCachePinsPoolBeyondReaderDeletionAndReturnsToThatPool) {
    auto* reader = createReader(readerQos());
    ASSERT_NE(reader, nullptr);
    auto rtpsReader = endpoint(reader);
    auto pool = topicPool();
    std::weak_ptr<ITopicPayloadPool> lifetime = pool;
    auto change = received(MemoryPool::GetInstance(), {6, "outlives entity"});
    ASSERT_EQ(rtpsReader->AddChanges(change), RETCODE_OK);
    std::vector<std::shared_ptr<CacheChange>> taken;
    SampleInfoSeq infos;
    ASSERT_EQ(rtpsReader->ReadOrTake(taken, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, true), RETCODE_OK);
    ASSERT_EQ(taken.size(), 1u);
    ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    rtpsReader.reset();
    change.reset();
    EXPECT_EQ(pool->statistics().in_use, 1u);
    pool.reset();
    EXPECT_FALSE(lifetime.expired());
    auto saved = lifetime.lock();
    taken.clear();
    EXPECT_EQ(saved->statistics().in_use, 0u);
    saved.reset();
    EXPECT_TRUE(lifetime.expired());
}

TEST_F(ReaderPayloadPool, NativePayloadBypassesAnExhaustedOrdinaryPool) {
    auto* reader = createReader(readerQos(2));
    ASSERT_NE(reader, nullptr);
    auto pool = topicPool();
    char* busy = nullptr;
    char* another = nullptr;
    ASSERT_TRUE(pool->getBuffer(16, busy));
    ASSERT_TRUE(pool->getBuffer(16, another));
    auto change = endpoint(reader)->new_change(ALIVE, nullptr, HANDLE_NIL, HANDLE_NIL, TIME_INVALID);
    auto native = std::make_shared<ReaderPoolSample>(ReaderPoolSample{7, "native owner"});
    change->nativePayload = native;
    ASSERT_EQ(endpoint(reader)->AddChanges(change), RETCODE_OK);
    EXPECT_EQ(change->nativePayload.get(), native.get());
    EXPECT_EQ(change->serializedBuffer, nullptr);
    EXPECT_EQ(pool->statistics().in_use, 2u);
    EXPECT_TRUE(pool->releaseBuffer(busy));
    EXPECT_TRUE(pool->releaseBuffer(another));
}

TEST_F(ReaderPayloadPool, SamePoolCacheRetryDoesNotConsumeAnotherAllocation) {
    auto* reader = createReader(readerQos(2));
    ASSERT_NE(reader, nullptr);
    auto pool = topicPool();
    auto change = received(pool, {8, "already owned"});
    ASSERT_NE(change, nullptr);
    char* busy = nullptr;
    ASSERT_TRUE(pool->getBuffer(16, busy));
    auto* original = change->serializedBuffer;
    auto* implementation = dynamic_cast<DataReaderImpl*>(reader);
    EXPECT_EQ(implementation->PrepareSerializedPayload(change), RETCODE_OK);
    EXPECT_EQ(implementation->PrepareSerializedPayload(change), RETCODE_OK);
    EXPECT_EQ(change->serializedBuffer, original);
    EXPECT_EQ(pool->statistics().in_use, 2u);
    EXPECT_EQ(endpoint(reader)->AddChanges(change), RETCODE_OK);
    expectSample(reader, {8, "already owned"});
    EXPECT_TRUE(pool->releaseBuffer(busy));
}

TEST_F(ReaderPayloadPool, EmptyRtpsLifecycleDescriptorsNeedNoOrdinaryPayloadAllocation) {
    type.keyed = true;
    auto* reader = createReader(readerQos(2));
    ASSERT_NE(reader, nullptr);
    auto rtpsReader = endpoint(reader);
    auto pool = topicPool();
    char* busy = nullptr;
    ASSERT_TRUE(pool->getBuffer(16, busy));
    auto alive = received(MemoryPool::GetInstance(), {9, "lifecycle"});
    ASSERT_EQ(rtpsReader->AddChanges(alive), RETCODE_OK);
    const auto handle = alive->instanceHandle;
    expectSample(reader, {9, "lifecycle"}, handle);
    // Keep the old cache pinned so the ordinary pool remains full. Control
    // changes still carry state, without allocating a sample payload.
    ASSERT_EQ(pool->statistics().in_use, 2u);
    for (const auto kind : {NOT_ALIVE_UNREGISTERED, NOT_ALIVE_DISPOSED}) {
        auto* empty = new CacheSerializedBuffer;
        char unused = 0;
        if (kind == NOT_ALIVE_UNREGISTERED) empty->buffer = &unused;
        auto control = rtpsReader->new_change(kind, empty, handle, HANDLE_NIL, TIME_INVALID);
        ASSERT_EQ(rtpsReader->AddChanges(control), RETCODE_OK);
        ReaderPoolSample output;
        SampleInfo info;
        ASSERT_EQ(reader->take_next_sample(&output, info), RETCODE_OK);
        EXPECT_FALSE(info.valid_data);
        EXPECT_EQ(info.instance_handle, handle);
        EXPECT_EQ(info.instance_state, kind == NOT_ALIVE_DISPOSED ?
            NOT_ALIVE_DISPOSED_INSTANCE_STATE : NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
        EXPECT_EQ(pool->statistics().in_use, 2u);
    }
    auto malformed = rtpsReader->new_change(ALIVE, new CacheSerializedBuffer, handle, HANDLE_NIL, TIME_INVALID);
    EXPECT_EQ(rtpsReader->AddChanges(malformed), RETCODE_BAD_PARAMETER);
    EXPECT_TRUE(pool->releaseBuffer(busy));
}

TEST_F(ReaderPayloadPool, RealRtpsReassemblesFragmentsIntoReaderTopicPool) {
    auto* reader = createReader(readerQos());
    ASSERT_NE(reader, nullptr);
    DomainParticipantQos participantQos;
    participantQos.transport.removeTransport(TRANSPORT_SHM_QOS);
    remote = TheParticipantFactory->create_participant(145, participantQos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(remote, nullptr);
    ASSERT_EQ(remote->registe_type(type.get_typename(), &type), RETCODE_OK);
    auto* remoteTopic = remote->create_topic(topic->get_name(), type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(remoteTopic, nullptr);
    auto* remotePublisher = remote->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(remotePublisher, nullptr);
    DataWriterQos writerQos;
    writerQos.data_sharing.kind = OFF_DATA_SHARING_QOS;
    auto* writer = remotePublisher->create_datawriter(remoteTopic, writerQos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    ASSERT_TRUE(waitForReaderPool([&] {
        PublicationMatchedStatus offered;
        SubscriptionMatchedStatus requested;
        return writer->get_publication_matched_status(offered) == RETCODE_OK &&
            reader->get_subscription_matched_status(requested) == RETCODE_OK &&
            offered.current_count == 1 && requested.current_count == 1;
    }));
    ReaderPoolSample input{197, std::string(DATAMESSAGE_LIMIT * 2 + 17, 'r')};
    ASSERT_EQ(writer->write(&input, HANDLE_NIL), RETCODE_OK);
    ASSERT_TRUE(waitForReaderPool([&] { return reader->get_unread_count() == 1; }));
    auto pool = topicPool();
    auto rtpsReader = endpoint(reader);
    std::vector<std::shared_ptr<CacheChange>> taken;
    SampleInfoSeq infos;
    ASSERT_EQ(rtpsReader->ReadOrTake(taken, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE, true), RETCODE_OK);
    ASSERT_EQ(taken.size(), 1u);
    EXPECT_FALSE(taken[0]->nativePayload);
    unsigned fragments = 0;
    for (auto* part = taken[0]->serializedBuffer; part; part = part->next) {
        EXPECT_TRUE(pool->owns(part->origin_buffer));
        EXPECT_EQ(dynamic_cast<CacheSerializedBuffer*>(part), nullptr);
        ++fragments;
    }
    EXPECT_GE(fragments, 3u);
    SerializedBuffer view = *taken[0]->serializedBuffer;
    const auto littleEndian = dynamic_cast<DataReaderImpl*>(reader)->GetEndianess(&view);
    CdrDeserializer cdr(&view, littleEndian);
    ReaderPoolSample output;
    ASSERT_EQ(type.deserialize_data(&output, &cdr, littleEndian), 0);
    EXPECT_EQ(output.number, input.number);
    EXPECT_EQ(output.text, input.text);
    ASSERT_EQ(remotePublisher->delete_datawriter(writer), RETCODE_OK);
    ASSERT_TRUE(waitForReaderPool([&] { return pool->statistics().in_use == fragments; }));
    taken.clear();
    EXPECT_EQ(pool->statistics().in_use, 0u);
}

TEST_F(ReaderPayloadPool, RealRtpsDisposeAndUnregisterUpdateKeyedInstanceWithoutPayload) {
    type.keyed = true;
    auto* reader = createReader(readerQos());
    ASSERT_NE(reader, nullptr);
    DomainParticipantQos participantQos;
    participantQos.transport.removeTransport(TRANSPORT_SHM_QOS);
    remote = TheParticipantFactory->create_participant(145, participantQos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(remote, nullptr);
    ASSERT_EQ(remote->registe_type(type.get_typename(), &type), RETCODE_OK);
    auto* remoteTopic = remote->create_topic(topic->get_name(), type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(remoteTopic, nullptr);
    auto* remotePublisher = remote->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(remotePublisher, nullptr);
    DataWriterQos qos;
    qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
    qos.writer_data_lifecycle.autodispose_unregistered_instances = false;
    auto* writer = remotePublisher->create_datawriter(remoteTopic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    ASSERT_TRUE(waitForReaderPool([&] {
        PublicationMatchedStatus offered;
        SubscriptionMatchedStatus requested;
        return writer->get_publication_matched_status(offered) == RETCODE_OK &&
            reader->get_subscription_matched_status(requested) == RETCODE_OK &&
            offered.current_count == 1 && requested.current_count == 1;
    }));
    ReaderPoolSample input{91, "lifecycle over RTPS"};
    const auto handle = writer->register_instance(&input);
    ASSERT_NE(handle, HANDLE_NIL);
    for (const bool dispose : {true, false}) {
        ASSERT_EQ(writer->write(&input, handle), RETCODE_OK);
        ASSERT_TRUE(waitForReaderPool([&] { return reader->get_unread_count() == 1; }));
        expectSample(reader, input, handle);
        ASSERT_EQ(dispose ? writer->dispose(nullptr, handle) : writer->unregister_instance(nullptr, handle), RETCODE_OK);
        ASSERT_TRUE(waitForReaderPool([&] { return reader->get_unread_count() == 1; }));
        ReaderPoolSample output;
        SampleInfo info;
        ASSERT_EQ(reader->take_next_sample(&output, info), RETCODE_OK);
        EXPECT_FALSE(info.valid_data);
        EXPECT_EQ(info.instance_handle, handle);
        EXPECT_EQ(info.instance_state, dispose ? NOT_ALIVE_DISPOSED_INSTANCE_STATE : NOT_ALIVE_NO_WRITERS_INSTANCE_STATE);
    }
}

TEST(ReaderPayloadPoolQos, RejectsInvalidPoliciesAndKeepsEveryAllocationFieldImmutable) {
    DataReaderQos original, changed;
    EXPECT_EQ(QosPolicyCheck::CheckQos(original), RETCODE_OK);
    for (auto edit : {+[](PayloadPoolQosPolicy& q) { q.policy = PayloadMemoryPolicy::DYNAMIC_RESERVE; },
            +[](PayloadPoolQosPolicy& q) { q.payload_size = 128; },
            +[](PayloadPoolQosPolicy& q) { q.max_payloads = 2; },
            +[](PayloadPoolQosPolicy& q) { q.initial_payloads = 1; },
            +[](PayloadPoolQosPolicy& q) { q.alignment = 64; }}) {
        changed = original;
        edit(changed.payload_pool);
        EXPECT_FALSE(QosPolicyCheck::CanQosBeUpdated(original, changed));
    }
    for (auto edit : {+[](PayloadPoolQosPolicy& q) { q.max_payloads = 0; },
            +[](PayloadPoolQosPolicy& q) { q.initial_payloads = 2; q.max_payloads = 1; },
            +[](PayloadPoolQosPolicy& q) { q.policy = PayloadMemoryPolicy::PREALLOCATED; },
            +[](PayloadPoolQosPolicy& q) { q.policy = PayloadMemoryPolicy::DYNAMIC_RESERVE; q.initial_payloads = 1; },
            +[](PayloadPoolQosPolicy& q) { q.alignment = 3; }}) {
        changed = original;
        edit(changed.payload_pool);
        EXPECT_EQ(QosPolicyCheck::CheckQos(changed), RETCODE_INCONSISTENT_POLICY);
    }
}

#ifdef _XML_INTERFACE_CPP
namespace {
class ReaderPayloadPoolXml : public testing::Test {
protected:
    void SetUp() override {
        char pattern[] = "/tmp/travodds-reader-pool-XXXXXX";
        const int fd = mkstemp(pattern);
        ASSERT_GE(fd, 0);
        close(fd);
        path = pattern;
    }
    void TearDown() override { unlink(path.c_str()); }
    ReturnCode_t load(const std::string& body) {
        std::ofstream file(path);
        file << "<dds_qos><qos_library name=\"lib\"><qos_profile name=\"p\">" << body
             << "</qos_profile></qos_library></dds_qos>";
        file.close();
        return repo.load(path);
    }
    std::string path;
    DDSXMLRepo repo;
};
}

TEST_F(ReaderPayloadPoolXml, ParsesAllReaderPoliciesAndInheritedConfiguration) {
    for (const std::string policy : {"PREALLOCATED", "PREALLOCATED_WITH_REALLOC", "DYNAMIC_RESERVE", "DYNAMIC_REUSABLE"}) {
        ASSERT_EQ(repo.clear(), RETCODE_OK);
        ASSERT_EQ(load("<datareader_qos name=\"base\"><payload_pool><policy>" + policy +
            "</policy><payload_size>4096</payload_size><max_payloads>2</max_payloads><alignment>64</alignment>"
            "</payload_pool></datareader_qos><datareader_qos name=\"child\" base_name=\"base\">"
            "<payload_pool><max_payloads>3</max_payloads></payload_pool></datareader_qos>"), RETCODE_OK);
        DataReaderQos a, b;
        ASSERT_EQ(repo.lookup_datareader_qos("lib", "p", "base", a), RETCODE_OK);
        ASSERT_EQ(repo.lookup_datareader_qos("lib", "p", "child", b), RETCODE_OK);
        EXPECT_TRUE(a.payload_pool.valid());
        EXPECT_EQ(a.payload_pool.payload_size, 4096u);
        EXPECT_EQ(a.payload_pool.max_payloads, 2u);
        EXPECT_EQ(a.payload_pool.alignment, 64u);
        EXPECT_EQ(b.payload_pool.policy, a.payload_pool.policy);
        EXPECT_EQ(b.payload_pool.max_payloads, 3u);
        EXPECT_EQ(b.payload_pool.payload_size, 4096u);
    }
}

TEST_F(ReaderPayloadPoolXml, RejectsInvalidFieldsAndDuplicateReaderPoolSections) {
    for (const std::string body : {"<policy>UNKNOWN</policy>", "<policy>PREALLOCATED</policy>",
            "<max_payloads>0</max_payloads>", "<alignment>3</alignment>",
            "<policy>DYNAMIC_RESERVE</policy><initial_payloads>1</initial_payloads>",
            "<max_payloads>2</max_payloads><max_payloads>3</max_payloads>",
            "<max_payloads extra=\"x\">2</max_payloads>", "<unknown>2</unknown>"}) {
        ASSERT_EQ(repo.clear(), RETCODE_OK);
        EXPECT_NE(load("<datareader_qos><payload_pool>" + body + "</payload_pool></datareader_qos>"), RETCODE_OK);
    }
    ASSERT_EQ(repo.clear(), RETCODE_OK);
    EXPECT_NE(load("<datareader_qos><payload_pool/><payload_pool/></datareader_qos>"), RETCODE_OK);
}
#endif
