#include "gtest/gtest.h"
#include "common/memory/sharedpayloadpool.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "dcps/topic/typesupport.h"
#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/writer.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>

USING_TRAVODDS_NAMESPACE
using namespace std::chrono_literals;

namespace {
struct alignas(64) UninitializedSample {
    uint64_t key;
    uint64_t payload[7];
};
struct alignas(64) MemberInitializedSample {
    uint64_t key = 19;
    uint64_t payload[7]{};
};
struct alignas(64) ConstructorInitializedSample {
    static std::atomic<unsigned> constructions;
    uint64_t key;
    uint64_t payload[7];
    ConstructorInitializedSample() noexcept : key(23), payload{} { ++constructions; }
};
std::atomic<unsigned> ConstructorInitializedSample::constructions{0};
static_assert(sizeof(UninitializedSample) == 64, "Keep the native layout explicit");
static_assert(std::is_trivially_default_constructible<UninitializedSample>::value, "No initialization");
static_assert(!std::is_trivially_default_constructible<MemberInitializedSample>::value, "Writes defaults");
static_assert(!std::is_trivially_default_constructible<ConstructorInitializedSample>::value, "Runs user code");

template<class T> class InitializationSupport : public NativeTypeSupport<T> {
public:
    explicit InitializationSupport(const char* name = "NativeInitializationSample")
        : NativeTypeSupport<T>("ospp.NoInitialization.v1.u64x8.align64"), name_(name) {}
    std::atomic<bool> failConstruction{false}, throwConstruction{false}, forbidPayloadCdr{false};
    mutable std::atomic<unsigned> constructionAttempts{0};
    std::atomic<unsigned> serializations{0}, deserializations{0}, copies{0};

    bool construct_native_sample(void* storage) const override {
        ++constructionAttempts;
        if (throwConstruction) throw std::runtime_error("Injected native construction failure");
        if (failConstruction) return false;
        return NativeTypeSupport<T>::construct_native_sample(storage);
    }
    void* create_data() override { return new T{}; }
    void delete_data(void* sample) override { delete static_cast<T*>(sample); }
    int32_t copy_data(void* destination, void* source) override {
        ++copies;
        *static_cast<T*>(destination) = *static_cast<T*>(source);
        return 0;
    }
    uint32_t get_serialized_data_size(void*, uint32_t) override { return sizeof(T); }
    uint32_t get_max_serialized_data_size(void*, uint32_t) override { return sizeof(T); }
    int32_t serialize_data(void* sample, CdrSerializer* cdr, int32_t) override {
        ++serializations;
        if (forbidPayloadCdr) return -1;
        const auto& value = *static_cast<T*>(sample);
        if (!cdr->serializeBaseType(value.key)) return -1;
        for (auto field : value.payload) if (!cdr->serializeBaseType(field)) return -1;
        return 0;
    }
    int32_t deserialize_data(void* sample, CdrDeserializer* cdr, int32_t) override {
        ++deserializations;
        if (forbidPayloadCdr) return -1;
        auto& value = *static_cast<T*>(sample);
        if (!cdr->deserializeBaseType(value.key)) return -1;
        for (auto& field : value.payload) if (!cdr->deserializeBaseType(field)) return -1;
        return 0;
    }
    TypeObject* get_typeobject() override { return nullptr; }
    ReturnCode_t get_instancehandle(void*, CdrSerializer*, InstanceHandle_t& handle, bool) override {
        handle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int32_t MakeKey(const void*, InstanceHandle_t& handle, bool) override {
        handle = HANDLE_NIL;
        return 0;
    }
    bool has_key() override { return false; }
    const char* get_typename() override { return name_; }
private:
    const char* name_;
};

// An old custom TypeSupport can describe a native layout and initialize a
// sample, without promising that its construction leaves payload untouched.
// The new conservative capability must keep such supports on CDR/RTPS.
class LegacyInitializingSupport : public TypeSupport {
public:
    InitializationSupport<UninitializedSample> cdr{"LegacyNativeInitializationSample"};
    mutable std::atomic<unsigned> nativeConstructions{0};
    NativeTypeInfo get_native_type_info() const override { return cdr.get_native_type_info(); }
    bool construct_native_sample(void* storage) const override {
        ++nativeConstructions;
        new (storage) UninitializedSample{};
        return true;
    }
    void* create_data() override { return cdr.create_data(); }
    void delete_data(void* sample) override { cdr.delete_data(sample); }
    int32_t copy_data(void* destination, void* source) override { return cdr.copy_data(destination, source); }
    uint32_t get_serialized_data_size(void* sample, uint32_t alignment) override {
        return cdr.get_serialized_data_size(sample, alignment);
    }
    uint32_t get_max_serialized_data_size(void* sample, uint32_t alignment) override {
        return cdr.get_max_serialized_data_size(sample, alignment);
    }
    int32_t serialize_data(void* sample, CdrSerializer* stream, int32_t endian) override {
        return cdr.serialize_data(sample, stream, endian);
    }
    int32_t deserialize_data(void* sample, CdrDeserializer* stream, int32_t endian) override {
        return cdr.deserialize_data(sample, stream, endian);
    }
    TypeObject* get_typeobject() override { return nullptr; }
    ReturnCode_t get_instancehandle(void*, CdrSerializer*, InstanceHandle_t& handle, bool) override {
        handle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int32_t MakeKey(const void*, InstanceHandle_t& handle, bool) override {
        handle = HANDLE_NIL;
        return 0;
    }
    bool has_key() override { return false; }
    const char* get_typename() override { return cdr.get_typename(); }
};

TEST(NativeInitialization, TrivialConstructionPreservesEveryStorageByteOnReuse) {
    InitializationSupport<UninitializedSample> type;
    EXPECT_TRUE(type.supports_native_uninitialized_sample());
    EXPECT_TRUE(type.supports_native_data_sharing(type.get_native_type_info()));
    alignas(UninitializedSample) std::array<unsigned char, sizeof(UninitializedSample)> storage;
    for (auto pattern : {0xabu, 0x37u}) {
        storage.fill(static_cast<unsigned char>(pattern));
        const auto expected = storage;
        ASSERT_TRUE(type.construct_native_sample(storage.data()));
        // Observe representation through unsigned bytes, never uninitialized
        // scalar fields. Every byte must survive a new object lifetime.
        EXPECT_EQ(std::memcmp(storage.data(), expected.data(), storage.size()), 0);
    }
}

TEST(NativeInitialization, InvalidStorageIsRejectedWithoutWritingBytes) {
    InitializationSupport<UninitializedSample> type;
    alignas(UninitializedSample) std::array<unsigned char, sizeof(UninitializedSample) + 64> storage;
    storage.fill(0x9d);
    const auto expected = storage;
    EXPECT_FALSE(type.construct_native_sample(nullptr));
    EXPECT_FALSE(type.construct_native_sample(storage.data() + 1));
    EXPECT_EQ(std::memcmp(storage.data(), expected.data(), storage.size()), 0);
}

template<class T> void refusesInitializingConstructor() {
    InitializationSupport<T> type;
    ASSERT_TRUE(type.get_native_type_info().loanable());
    EXPECT_FALSE(type.supports_native_uninitialized_sample());
    EXPECT_FALSE(type.supports_native_data_sharing(type.get_native_type_info()));
    alignas(T) std::array<unsigned char, sizeof(T)> storage;
    storage.fill(0xab);
    const auto expected = storage;
    EXPECT_FALSE(type.construct_native_sample(storage.data()));
    EXPECT_EQ(std::memcmp(storage.data(), expected.data(), storage.size()), 0);
}

TEST(NativeInitialization, DefaultMemberInitializersAreNotANativeConstructionCapability) {
    refusesInitializingConstructor<MemberInitializedSample>();
}

TEST(NativeInitialization, UserDefaultConstructorIsNotInvokedByNativeConstruction) {
    const auto before = ConstructorInitializedSample::constructions.load();
    refusesInitializingConstructor<ConstructorInitializedSample>();
    EXPECT_EQ(ConstructorInitializedSample::constructions.load(), before);
}

TEST(NativeInitialization, DirectTypeSupportLayoutAloneDoesNotOptIntoUninitializedConstruction) {
    LegacyInitializingSupport type;
    ASSERT_TRUE(type.get_native_type_info().loanable());
    EXPECT_FALSE(type.supports_native_uninitialized_sample());
    EXPECT_FALSE(type.supports_native_data_sharing(type.get_native_type_info()));
    EXPECT_EQ(type.nativeConstructions.load(), 0u);
}

#if defined(__linux__)
template<class Predicate> bool initializationEventually(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    do {
        if (predicate()) return true;
        std::this_thread::sleep_for(2ms);
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

class NativeInitializationDds : public testing::Test {
protected:
    void start(std::unique_ptr<TypeSupport> support) {
        type = std::move(support);
        DomainParticipantQos qos;
        qos.transport.removeTransport(TRANSPORT_SHM_QOS);
        participant = TheParticipantFactory->create_participant(144, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(participant, nullptr);
        ASSERT_EQ(participant->registe_type(type->get_typename(), type.get()), RETCODE_OK);
        topic = participant->create_topic("native-initialization", type->get_typename(),
            TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(topic, nullptr);
        publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_TRUE(publisher && subscriber);
    }
    void TearDown() override {
        if (!participant) return;
        EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);
        EXPECT_EQ(TheParticipantFactory->delete_participant(participant), RETCODE_OK);
    }
    DataWriter* createWriter(DataSharingQosPolicyKind kind) {
        DataWriterQos qos;
        qos.data_sharing.kind = kind;
        qos.data_sharing.max_samples = 1;
        qos.data_sharing.max_readers = 1;
        qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
        qos.history.depth = 1;
        writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
        return writer;
    }
    DataReader* createReader(DataSharingQosPolicyKind kind) {
        DataReaderQos qos;
        qos.data_sharing.kind = kind;
        qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
        reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
        return reader;
    }
    bool matched() {
        return initializationEventually([&] {
            PublicationMatchedStatus offered;
            SubscriptionMatchedStatus requested;
            return writer->get_publication_matched_status(offered) == RETCODE_OK &&
                reader->get_subscription_matched_status(requested) == RETCODE_OK &&
                offered.current_count == 1 && requested.current_count == 1;
        });
    }
    std::shared_ptr<SharedPayloadPool> openPool() {
        const auto info = dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData().data_sharing;
        DataWriterQos qos;
        EXPECT_EQ(writer->get_qos(qos), RETCODE_OK);
        SharedPoolDescriptor descriptor;
        descriptor.writer = Conversion::ToGuid(writer->get_instance_handle());
        descriptor.session = {info.session_high, info.session_low};
        descriptor.path = SharedPayloadPool::pathFor(descriptor.writer, descriptor.session,
            qos.data_sharing.shared_memory_directory);
        descriptor.config.payload_size = info.type.size;
        descriptor.config.alignment = std::max(64u, info.type.alignment);
        descriptor.config.slots = info.slots;
        descriptor.config.max_readers = info.max_readers;
        descriptor.config.max_segment_bytes = qos.data_sharing.max_segment_bytes;
        descriptor.segment_bytes = info.segment_bytes;
        return SharedPayloadPool::open(descriptor);
    }
    template<class T> void cdrFallback() {
        ASSERT_EQ(createWriter(ON_DATA_SHARING_QOS), nullptr);
        ASSERT_EQ(createReader(ON_DATA_SHARING_QOS), nullptr);
        ASSERT_NE(createWriter(AUTO_DATA_SHARING_QOS), nullptr);
        ASSERT_NE(createReader(AUTO_DATA_SHARING_QOS), nullptr);
        EXPECT_EQ(dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData().data_sharing.version, 0u);
        EXPECT_EQ(dynamic_cast<DataReaderImpl*>(reader)->GetBuiltinTopicData().data_sharing.version, 0u);
        SampleLoan loan;
        EXPECT_EQ(writer->loan_sample(loan), RETCODE_UNSUPPORTED);
        EXPECT_FALSE(loan);
        ASSERT_TRUE(matched());
        T sample;
        sample.key = 61;
        for (uint64_t i = 0; i < 7; ++i) sample.payload[i] = 800 + i;
        ASSERT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
        SequenceData<T> values;
        SampleInfoSeq infos;
        ReturnCode_t result = RETCODE_NO_DATA;
        ASSERT_TRUE(initializationEventually([&] {
            result = reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
            return result != RETCODE_NO_DATA;
        }));
        ASSERT_EQ(result, RETCODE_OK);
        ASSERT_EQ(values.length(), 1u);
        ASSERT_EQ(infos.length(), 1u);
        EXPECT_TRUE(infos[0].valid_data);
        EXPECT_FALSE(values.has_loan());
        EXPECT_EQ(values[0].key, 61u);
        for (uint64_t i = 0; i < 7; ++i) EXPECT_EQ(values[0].payload[i], 800 + i);
        EXPECT_EQ(reader->return_loan(values, infos), RETCODE_OK);
    }
    std::unique_ptr<TypeSupport> type;
    DomainParticipant* participant = nullptr;
    Topic* topic = nullptr;
    Publisher* publisher = nullptr;
    Subscriber* subscriber = nullptr;
    DataWriter* writer = nullptr;
    DataReader* reader = nullptr;
};

TEST_F(NativeInitializationDds, DefaultMemberInitializationRejectsOnAndPreservesAutoCdrDelivery) {
    auto support = std::make_unique<InitializationSupport<MemberInitializedSample>>();
    auto* counters = support.get();
    ASSERT_NO_FATAL_FAILURE(start(std::move(support)));
    ASSERT_NO_FATAL_FAILURE(cdrFallback<MemberInitializedSample>());
    EXPECT_EQ(counters->constructionAttempts.load(), 0u);
    EXPECT_GT(counters->serializations.load(), 0u);
    EXPECT_GT(counters->deserializations.load(), 0u);
}

TEST_F(NativeInitializationDds, UserDefaultConstructorRejectsOnAndPreservesAutoCdrDelivery) {
    auto support = std::make_unique<InitializationSupport<ConstructorInitializedSample>>();
    auto* counters = support.get();
    ASSERT_NO_FATAL_FAILURE(start(std::move(support)));
    ASSERT_NO_FATAL_FAILURE(cdrFallback<ConstructorInitializedSample>());
    EXPECT_EQ(counters->constructionAttempts.load(), 0u);
    EXPECT_GT(counters->serializations.load(), 0u);
    EXPECT_GT(counters->deserializations.load(), 0u);
}

TEST_F(NativeInitializationDds, LegacyCustomTypeSupportRejectsOnAndPreservesAutoCdrDelivery) {
    auto support = std::make_unique<LegacyInitializingSupport>();
    auto* counters = support.get();
    ASSERT_NO_FATAL_FAILURE(start(std::move(support)));
    ASSERT_NO_FATAL_FAILURE(cdrFallback<UninitializedSample>());
    EXPECT_EQ(counters->nativeConstructions.load(), 0u);
    EXPECT_GT(counters->cdr.serializations.load(), 0u);
    EXPECT_GT(counters->cdr.deserializations.load(), 0u);
}

TEST_F(NativeInitializationDds, ReusedSingleSlotPreservesEveryByteBeforeNativeLoanPublication) {
    auto support = std::make_unique<InitializationSupport<UninitializedSample>>();
    auto* counters = support.get();
    ASSERT_NO_FATAL_FAILURE(start(std::move(support)));
    ASSERT_NE(createWriter(ON_DATA_SHARING_QOS), nullptr);
    ASSERT_NE(createReader(ON_DATA_SHARING_QOS), nullptr);
    ASSERT_TRUE(matched());
    auto pool = openPool();
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto endpoint = rtps->GetWriter(Conversion::ToGuid(writer->get_instance_handle()).entityId);
    ASSERT_TRUE(initializationEventually([&] {
        return endpoint->SelectDataSharingReaders(pool->registeredReaders()).size() == 1;
    }));
    counters->forbidPayloadCdr = true;
    SampleLoan first;
    ASSERT_EQ(writer->loan_sample(first), RETCODE_OK);
    void* address = first.data();
    std::memset(address, 0xab, sizeof(UninitializedSample));
    std::array<unsigned char, sizeof(UninitializedSample)> expected;
    std::memcpy(expected.data(), address, expected.size());
    ASSERT_EQ(writer->discard_loan(first), RETCODE_OK);
    SampleLoan reused;
    ASSERT_EQ(writer->loan_sample(reused), RETCODE_OK);
    ASSERT_EQ(reused.data(), address);
    ASSERT_EQ(std::memcmp(reused.data(), expected.data(), expected.size()), 0);
    // Fill every field directly in the borrowed storage before publication.
    auto* sample = static_cast<UninitializedSample*>(reused.data());
    sample->key = 71;
    for (uint64_t i = 0; i < 7; ++i) sample->payload[i] = 900 + i;
    ASSERT_EQ(writer->write(reused, HANDLE_NIL), RETCODE_OK);
    EXPECT_FALSE(reused);
    SequenceData<UninitializedSample> values;
    SampleInfoSeq infos;
    ReturnCode_t result = RETCODE_NO_DATA;
    ASSERT_TRUE(initializationEventually([&] {
        result = reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
        return result != RETCODE_NO_DATA;
    }));
    ASSERT_EQ(result, RETCODE_OK);
    ASSERT_EQ(values.length(), 1u);
    ASSERT_EQ(infos.length(), 1u);
    ASSERT_TRUE(values.has_loan());
    EXPECT_TRUE(infos[0].valid_data);
    EXPECT_EQ(values[0].key, 71u);
    for (uint64_t i = 0; i < 7; ++i) EXPECT_EQ(values[0].payload[i], 900 + i);
    EXPECT_EQ(counters->serializations.load(), 0u);
    EXPECT_EQ(counters->deserializations.load(), 0u);
    EXPECT_EQ(counters->copies.load(), 0u);
    EXPECT_EQ(reader->return_loan(values, infos), RETCODE_OK);
}

TEST_F(NativeInitializationDds, ConstructionFailureReturnsTheSingleSlotWithoutChangingItsBytes) {
    auto support = std::make_unique<InitializationSupport<UninitializedSample>>();
    auto* controls = support.get();
    ASSERT_NO_FATAL_FAILURE(start(std::move(support)));
    ASSERT_NE(createWriter(ON_DATA_SHARING_QOS), nullptr);
    auto pool = openPool();
    SampleLoan seed;
    ASSERT_EQ(writer->loan_sample(seed), RETCODE_OK);
    void* address = seed.data();
    std::memset(address, 0xab, sizeof(UninitializedSample));
    std::array<unsigned char, sizeof(UninitializedSample)> expected;
    std::memcpy(expected.data(), address, expected.size());
    ASSERT_EQ(writer->discard_loan(seed), RETCODE_OK);
    for (bool throwing : {false, true}) {
        SCOPED_TRACE(throwing);
        controls->failConstruction = !throwing;
        controls->throwConstruction = throwing;
        SampleLoan rejected;
        EXPECT_EQ(writer->loan_sample(rejected), RETCODE_ERROR);
        EXPECT_FALSE(rejected);
        SharedPoolStats stats;
        ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
        EXPECT_EQ(stats.free_slots, 1u);
        controls->failConstruction = controls->throwConstruction = false;
        SampleLoan recovered;
        ASSERT_EQ(writer->loan_sample(recovered), RETCODE_OK);
        ASSERT_EQ(recovered.data(), address);
        EXPECT_EQ(std::memcmp(recovered.data(), expected.data(), expected.size()), 0);
        ASSERT_EQ(writer->discard_loan(recovered), RETCODE_OK);
    }
}
#endif
}
