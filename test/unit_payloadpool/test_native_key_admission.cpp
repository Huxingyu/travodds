#include "gtest/gtest.h"
#include "common/memory/sharedpayloadpool.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/writer.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

#if defined(__linux__)
USING_TRAVODDS_NAMESPACE
using namespace std::chrono_literals;

namespace {
struct alignas(64) AdmissionSample {
    uint64_t key;
    uint64_t payload[7];
};
static_assert(sizeof(AdmissionSample) == 64, "Keep the explicit native layout contract stable");

enum class KeyLayout { Missing, Compact, ZeroSize, Oversized, InvalidAlignment, Overaligned };

class AdmissionSupport : public NativeTypeSupport<AdmissionSample> {
public:
    AdmissionSupport() : NativeTypeSupport("ospp.StrictKeyAdmission.v1.u64x8.align64") {}
    KeyLayout keyLayout = KeyLayout::Compact;
    std::atomic<bool> forbidPayloadCdr{false};
    std::atomic<int> serializations{0}, deserializations{0}, copies{0};
    mutable std::atomic<int> keyCaptures{0}, keyRestores{0};

    NativeKeyInfo get_native_key_info() const override {
        switch (keyLayout) {
        case KeyLayout::Missing: return {};
        case KeyLayout::ZeroSize: return {0, alignof(uint64_t)};
        case KeyLayout::Oversized: return {sizeof(AdmissionSample) + 1, alignof(uint64_t)};
        case KeyLayout::InvalidAlignment: return {sizeof(uint64_t), 3};
        case KeyLayout::Overaligned: return {sizeof(uint64_t), alignof(AdmissionSample) * 2};
        default: return {sizeof(uint64_t), alignof(uint64_t)};
        }
    }
    bool capture_native_key(void* snapshot, const void* sample) const override {
        ++keyCaptures;
        if (keyLayout != KeyLayout::Compact) return false;
        std::memcpy(snapshot, &static_cast<const AdmissionSample*>(sample)->key, sizeof(uint64_t));
        return true;
    }
    bool restore_native_key(void* sample, const void* snapshot) const override {
        ++keyRestores;
        if (keyLayout != KeyLayout::Compact) return false;
        std::memcpy(&static_cast<AdmissionSample*>(sample)->key, snapshot, sizeof(uint64_t));
        return true;
    }
    void* create_data() override { return new AdmissionSample{}; }
    void delete_data(void* sample) override { delete static_cast<AdmissionSample*>(sample); }
    int32_t copy_data(void* destination, void* source) override {
        ++copies;
        *static_cast<AdmissionSample*>(destination) = *static_cast<AdmissionSample*>(source);
        return 0;
    }
    uint32_t get_serialized_data_size(void*, uint32_t) override { return sizeof(AdmissionSample); }
    uint32_t get_max_serialized_data_size(void*, uint32_t) override { return sizeof(AdmissionSample); }
    int32_t serialize_data(void* sample, CdrSerializer* cdr, int32_t) override {
        ++serializations;
        if (forbidPayloadCdr) return -1;
        const auto& value = *static_cast<AdmissionSample*>(sample);
        if (!cdr->serializeBaseType(value.key)) return -1;
        for (auto field : value.payload) if (!cdr->serializeBaseType(field)) return -1;
        return 0;
    }
    int32_t deserialize_data(void* sample, CdrDeserializer* cdr, int32_t) override {
        ++deserializations;
        if (forbidPayloadCdr) return -1;
        auto& value = *static_cast<AdmissionSample*>(sample);
        if (!cdr->deserializeBaseType(value.key)) return -1;
        for (auto& field : value.payload) if (!cdr->deserializeBaseType(field)) return -1;
        return 0;
    }
    TypeObject* get_typeobject() override { return nullptr; }
    ReturnCode_t get_instancehandle(void* sample, CdrSerializer* cdr,
            InstanceHandle_t& handle, bool forceMd5) override {
        handle = HANDLE_NIL;
        if (!cdr->serializeBaseType(static_cast<AdmissionSample*>(sample)->key) ||
            !cdr->getKeyHash(reinterpret_cast<char*>(&handle), forceMd5)) return RETCODE_ERROR;
        return RETCODE_OK;
    }
    int32_t MakeKey(const void* sample, InstanceHandle_t& handle, bool forceMd5) override {
        char bytes[16]{};
        SerializedBuffer buffer;
        buffer.buffer = bytes;
        buffer.buffer_size = sizeof(bytes);
        CdrSerializer cdr(&buffer);
        return get_instancehandle(const_cast<void*>(sample), &cdr, handle, forceMd5) == RETCODE_OK ? 0 : -1;
    }
    bool has_key() override { return true; }
    const char* get_typename() override { return "StrictNativeKeyAdmission"; }
};

template<class Predicate> bool eventually(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    do {
        if (predicate()) return true;
        std::this_thread::sleep_for(2ms);
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

AdmissionSample sampleValue() {
    AdmissionSample sample{};
    sample.key = 87;
    for (uint64_t i = 0; i < 7; ++i) sample.payload[i] = 1000 + i;
    return sample;
}

class NativeKeyAdmission : public testing::Test {
protected:
    void SetUp() override {
        DomainParticipantQos qos;
        qos.transport.removeTransport(TRANSPORT_SHM_QOS);
        writerParticipant = TheParticipantFactory->create_participant(143, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(writerParticipant, nullptr);
        readerParticipant = TheParticipantFactory->create_participant(143, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(readerParticipant, nullptr);
        ASSERT_EQ(writerParticipant->registe_type(writerType.get_typename(), &writerType), RETCODE_OK);
        ASSERT_EQ(readerParticipant->registe_type(readerType.get_typename(), &readerType), RETCODE_OK);
        writerTopic = writerParticipant->create_topic("strict-key-admission", writerType.get_typename(),
            TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        readerTopic = readerParticipant->create_topic("strict-key-admission", readerType.get_typename(),
            TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_TRUE(writerTopic && readerTopic);
        publisher = writerParticipant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        subscriber = readerParticipant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_TRUE(publisher && subscriber);
    }
    void TearDown() override {
        for (auto* participant : {readerParticipant, writerParticipant}) {
            if (!participant) continue;
            EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);
            EXPECT_EQ(TheParticipantFactory->delete_participant(participant), RETCODE_OK);
        }
    }
    DataWriter* createWriter(DataSharingQosPolicyKind kind) {
        DataWriterQos qos;
        qos.data_sharing.kind = kind;
        qos.data_sharing.max_samples = 4;
        qos.data_sharing.max_readers = 2;
        qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
        qos.history.depth = 1;
        writer = publisher->create_datawriter(writerTopic, qos, nullptr, STATUS_MASK_NONE);
        return writer;
    }
    DataReader* createReader(DataSharingQosPolicyKind kind) {
        DataReaderQos qos;
        qos.data_sharing.kind = kind;
        qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
        reader = subscriber->create_datareader(readerTopic, qos, nullptr, STATUS_MASK_NONE);
        return reader;
    }
    bool matched() {
        return eventually([&] {
            PublicationMatchedStatus offered;
            SubscriptionMatchedStatus requested;
            return writer->get_publication_matched_status(offered) == RETCODE_OK &&
                reader->get_subscription_matched_status(requested) == RETCODE_OK &&
                offered.current_count == 1 && requested.current_count == 1;
        });
    }
    DataSharingEndpointInfo writerInfo() {
        return dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData().data_sharing;
    }
    DataSharingEndpointInfo readerInfo() {
        return dynamic_cast<DataReaderImpl*>(reader)->GetBuiltinTopicData().data_sharing;
    }
    bool take(SequenceData<AdmissionSample>& values, SampleInfoSeq& infos) {
        ReturnCode_t result = RETCODE_NO_DATA;
        const bool received = eventually([&] {
            result = reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
            return result != RETCODE_NO_DATA;
        });
        EXPECT_EQ(result, RETCODE_OK);
        return received && result == RETCODE_OK;
    }
    void checkSample(const SequenceData<AdmissionSample>& values, const SampleInfoSeq& infos) {
        ASSERT_EQ(values.length(), 1u);
        ASSERT_EQ(infos.length(), 1u);
        EXPECT_TRUE(infos[0].valid_data);
        EXPECT_EQ(values[0].key, 87u);
        for (uint64_t i = 0; i < 7; ++i) EXPECT_EQ(values[0].payload[i], 1000 + i);
    }
    void cdrDelivery() {
        ASSERT_TRUE(matched());
        auto value = sampleValue();
        ASSERT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_OK);
        SequenceData<AdmissionSample> values;
        SampleInfoSeq infos;
        ASSERT_TRUE(take(values, infos));
        ASSERT_NO_FATAL_FAILURE(checkSample(values, infos));
        EXPECT_FALSE(values.has_loan());
        EXPECT_GT(writerType.serializations.load(), 0);
        EXPECT_GT(readerType.deserializations.load(), 0);
        // CDR fallback still supports the keyed DDS APIs, even when compact
        // native metadata is missing or malformed.
        AdmissionSample key{};
        ASSERT_EQ(reader->get_key_value(&key, infos[0].instance_handle), RETCODE_OK);
        EXPECT_EQ(key.key, 87u);
        EXPECT_EQ(reader->return_loan(values, infos), RETCODE_OK);
    }
    void checkNoNativeAdvertisement() {
        EXPECT_EQ(writerInfo().version, 0u);
        EXPECT_EQ(readerInfo().version, 0u);
        EXPECT_FALSE(writerInfo().valid(true));
        EXPECT_FALSE(readerInfo().valid(false));
        SampleLoan loan;
        EXPECT_EQ(writer->loan_sample(loan), RETCODE_UNSUPPORTED);
        EXPECT_FALSE(loan);
    }

    AdmissionSupport writerType, readerType;
    DomainParticipant *writerParticipant = nullptr, *readerParticipant = nullptr;
    Topic *writerTopic = nullptr, *readerTopic = nullptr;
    Publisher* publisher = nullptr;
    Subscriber* subscriber = nullptr;
    DataWriter* writer = nullptr;
    DataReader* reader = nullptr;
};

TEST_F(NativeKeyAdmission, OnRejectsFixedKeyedSupportsWithoutCompactKeyMetadata) {
    writerType.keyLayout = readerType.keyLayout = KeyLayout::Missing;
    ASSERT_TRUE(writerType.get_native_type_info().loanable());
    ASSERT_TRUE(readerType.get_native_type_info().loanable());
    EXPECT_EQ(createWriter(ON_DATA_SHARING_QOS), nullptr);
    EXPECT_EQ(createReader(ON_DATA_SHARING_QOS), nullptr);
    EXPECT_EQ(writerType.keyCaptures.load(), 0);
    EXPECT_EQ(readerType.keyCaptures.load(), 0);
}

TEST_F(NativeKeyAdmission, AutoMissingKeyMetadataAdvertisesNoNativeAndDeliversCdr) {
    writerType.keyLayout = readerType.keyLayout = KeyLayout::Missing;
    ASSERT_NE(createWriter(AUTO_DATA_SHARING_QOS), nullptr);
    ASSERT_NE(createReader(AUTO_DATA_SHARING_QOS), nullptr);
    checkNoNativeAdvertisement();
    cdrDelivery();
    EXPECT_EQ(writerType.keyCaptures.load(), 0);
    EXPECT_EQ(readerType.keyCaptures.load(), 0);
}

TEST_F(NativeKeyAdmission, CompactWriterAndMissingKeyReaderMatchAndDeliverThroughRtps) {
    readerType.keyLayout = KeyLayout::Missing;
    ASSERT_NE(createWriter(ON_DATA_SHARING_QOS), nullptr);
    ASSERT_NE(createReader(AUTO_DATA_SHARING_QOS), nullptr);
    EXPECT_TRUE(writerInfo().valid(true));
    EXPECT_EQ(readerInfo().version, 0u);
    EXPECT_FALSE(writerInfo().compatible_with(readerInfo()));
    cdrDelivery();
    EXPECT_EQ(readerType.keyCaptures.load(), 0);
}

TEST_F(NativeKeyAdmission, MissingKeyWriterAndCompactReaderMatchAndDeliverThroughRtps) {
    writerType.keyLayout = KeyLayout::Missing;
    ASSERT_NE(createWriter(AUTO_DATA_SHARING_QOS), nullptr);
    ASSERT_NE(createReader(ON_DATA_SHARING_QOS), nullptr);
    EXPECT_EQ(writerInfo().version, 0u);
    EXPECT_TRUE(readerInfo().valid(false));
    EXPECT_FALSE(writerInfo().compatible_with(readerInfo()));
    SampleLoan loan;
    EXPECT_EQ(writer->loan_sample(loan), RETCODE_UNSUPPORTED);
    cdrDelivery();
    EXPECT_EQ(writerType.keyCaptures.load(), 0);
}

TEST_F(NativeKeyAdmission, CompactPairUsesNativeWriterAndReaderLoansWithoutPayloadCdrOrCopies) {
    ASSERT_NE(createWriter(ON_DATA_SHARING_QOS), nullptr);
    ASSERT_NE(createReader(ON_DATA_SHARING_QOS), nullptr);
    ASSERT_TRUE(matched());
    ASSERT_TRUE(writerInfo().compatible_with(readerInfo()));
    auto rtps = dynamic_cast<DomainParticipantImpl*>(writerParticipant)->GetRelatedParticipant();
    auto endpoint = rtps->GetWriter(Conversion::ToGuid(writer->get_instance_handle()).entityId);
    const auto& info = writerInfo();
    DataWriterQos configured;
    ASSERT_EQ(writer->get_qos(configured), RETCODE_OK);
    SharedPoolDescriptor descriptor;
    descriptor.writer = endpoint->GetGuid();
    descriptor.session = {info.session_high, info.session_low};
    descriptor.path = SharedPayloadPool::pathFor(descriptor.writer, descriptor.session,
        configured.data_sharing.shared_memory_directory);
    descriptor.config.payload_size = info.type.size;
    descriptor.config.alignment = std::max(64u, info.type.alignment);
    descriptor.config.slots = info.slots;
    descriptor.config.max_readers = info.max_readers;
    descriptor.config.max_segment_bytes = configured.data_sharing.max_segment_bytes;
    descriptor.segment_bytes = info.segment_bytes;
    auto pool = SharedPayloadPool::open(descriptor);
    ASSERT_TRUE(eventually([&] {
        return endpoint->SelectDataSharingReaders(pool->registeredReaders()).size() == 1;
    }));
    writerType.forbidPayloadCdr = readerType.forbidPayloadCdr = true;
    SampleLoan loan;
    ASSERT_EQ(writer->loan_sample(loan), RETCODE_OK);
    auto* sample = static_cast<AdmissionSample*>(loan.data());
    sample->key = 87;
    for (uint64_t i = 0; i < 7; ++i) sample->payload[i] = 1000 + i;
    ASSERT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_OK);
    EXPECT_FALSE(loan);
    SequenceData<AdmissionSample> values;
    SampleInfoSeq infos;
    ASSERT_TRUE(take(values, infos));
    ASSERT_TRUE(values.has_loan());
    ASSERT_NO_FATAL_FAILURE(checkSample(values, infos));
    AdmissionSample key{};
    key.payload[6] = 7654;
    ASSERT_EQ(reader->get_key_value(&key, infos[0].instance_handle), RETCODE_OK);
    EXPECT_EQ(key.key, 87u);
    EXPECT_EQ(key.payload[6], 7654u);
    EXPECT_EQ(writerType.keyCaptures.load(), 1);
    EXPECT_EQ(readerType.keyCaptures.load(), 1);
    EXPECT_EQ(writerType.serializations.load(), 0);
    EXPECT_EQ(readerType.deserializations.load(), 0);
    EXPECT_EQ(writerType.copies.load(), 0);
    EXPECT_EQ(readerType.copies.load(), 0);
    EXPECT_EQ(reader->return_loan(values, infos), RETCODE_OK);
}

class NativeKeyMalformedAdmission : public NativeKeyAdmission,
        public testing::WithParamInterface<KeyLayout> {};

TEST_P(NativeKeyMalformedAdmission, OnRejectsMalformedCompactKeyMetadataBeforeNativeEnable) {
    writerType.keyLayout = readerType.keyLayout = GetParam();
    ASSERT_TRUE(writerType.get_native_type_info().loanable());
    ASSERT_FALSE(writerType.get_native_key_info().valid_for(writerType.get_native_type_info()));
    EXPECT_EQ(createWriter(ON_DATA_SHARING_QOS), nullptr);
    EXPECT_EQ(createReader(ON_DATA_SHARING_QOS), nullptr);
    EXPECT_EQ(writerType.keyCaptures.load(), 0);
    EXPECT_EQ(readerType.keyCaptures.load(), 0);
}

TEST_P(NativeKeyMalformedAdmission, AutoMalformedKeyMetadataKeepsOrdinaryCdrEndpointsUsable) {
    writerType.keyLayout = readerType.keyLayout = GetParam();
    ASSERT_NE(createWriter(AUTO_DATA_SHARING_QOS), nullptr);
    ASSERT_NE(createReader(AUTO_DATA_SHARING_QOS), nullptr);
    checkNoNativeAdvertisement();
    cdrDelivery();
    EXPECT_EQ(writerType.keyCaptures.load(), 0);
    EXPECT_EQ(readerType.keyCaptures.load(), 0);
}

INSTANTIATE_TEST_SUITE_P(MalformedLayouts, NativeKeyMalformedAdmission,
    testing::Values(KeyLayout::ZeroSize, KeyLayout::Oversized,
        KeyLayout::InvalidAlignment, KeyLayout::Overaligned),
    [](const testing::TestParamInfo<KeyLayout>& parameter) {
        switch (parameter.param) {
        case KeyLayout::ZeroSize: return "ZeroSize";
        case KeyLayout::Oversized: return "Oversized";
        case KeyLayout::InvalidAlignment: return "InvalidAlignment";
        default: return "Overaligned";
        }
    });
}
#endif
