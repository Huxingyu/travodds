#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/publish/publisher.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "dcps/topic/typesupport.h"
#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/reader.h"
#include <future>
#include <thread>

USING_TRAVODDS_NAMESPACE

namespace {
class BindingSupport : public TypeSupport {
public:
    void* create_data() override { return new uint64_t{}; }
    void delete_data(void* value) override { delete static_cast<uint64_t*>(value); }
    int32_t copy_data(void* destination, void* source) override {
        *static_cast<uint64_t*>(destination) = *static_cast<uint64_t*>(source);
        return 0;
    }
    uint32_t get_serialized_data_size(void*, uint32_t alignment) override {
        return CdrSerializer::getBaseTypeSize(sizeof(uint64_t), alignment);
    }
    uint32_t get_max_serialized_data_size(void* value, uint32_t alignment) override {
        return get_serialized_data_size(value, alignment);
    }
    int32_t serialize_data(void* value, CdrSerializer* cdr, int32_t) override {
        return cdr->serializeBaseType(*static_cast<uint64_t*>(value)) ? 0 : -1;
    }
    int32_t deserialize_data(void* value, CdrDeserializer* cdr, int32_t) override {
        return cdr->deserializeBaseType(*static_cast<uint64_t*>(value)) ? 0 : -1;
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
    const char* get_typename() override { return "ReaderBindingU64"; }
};

class ReaderBinding : public testing::TestWithParam<ReliabilityQosPolicyKind> {
protected:
    void SetUp() override {
        DomainParticipantQos participantQos;
        participantQos.transport.removeTransport(TRANSPORT_SHM_QOS);
        participant = TheParticipantFactory->create_participant(149, participantQos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(participant, nullptr);
        ASSERT_EQ(participant->registe_type(type.get_typename(), &type), RETCODE_OK);
        auto* topic = participant->create_topic("reader-binding", type.get_typename(),
            TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(topic, nullptr);
        auto* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(subscriber, nullptr);
        DataReaderQos qos;
        qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
        qos.reliability.kind = GetParam();
        auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(reader, nullptr);
        owner = dynamic_cast<DataReaderImpl*>(reader);
        ASSERT_NE(owner, nullptr);
        endpoint = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
            Conversion::ToGuid(reader->get_instance_handle()).entityId);
        ASSERT_TRUE(endpoint);
        ASSERT_EQ(endpoint->GetRelatedDataReader(), owner);
    }
    void TearDown() override {
        if (participant) {
            EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);
            EXPECT_EQ(TheParticipantFactory->delete_participant(participant), RETCODE_OK);
        }
        endpoint.reset();
    }
    template<class Inspect> void inspectDuringRepeatedBinding(Inspect inspect) {
        std::promise<void> release;
        auto start = release.get_future().share();
        // CreateReader has already published this exact binding. The old
        // enable path performed it a second time while discovery could read
        // the owner, handle and builtin key. One common start gives both
        // branches the initial publication, but no happens-before edge from
        // either branch's accesses to the other's. TSan detects the old
        // writes even if the scheduler happens to run the branches in order.
        std::thread rebinder([&, start] {
            start.wait();
            owner->SetRelatedReader(endpoint);
        });
        std::thread discovery([&, start] {
            start.wait();
            inspect();
        });
        release.set_value();
        rebinder.join();
        discovery.join();
    }
    BindingSupport type;
    DomainParticipant* participant = nullptr;
    DataReaderImpl* owner = nullptr;
    std::shared_ptr<Reader> endpoint;
};

TEST_P(ReaderBinding, PublishedOwnerPointerIsReadOnlyDuringRepeatedBinding) {
    DataReaderImpl* observed = nullptr;
    inspectDuringRepeatedBinding([&] { observed = endpoint->GetRelatedDataReader(); });
    EXPECT_EQ(observed, owner);
}

TEST_P(ReaderBinding, PublishedBuiltinKeyIsReadOnlyDuringRepeatedBinding) {
    const auto expected = owner->GetBuiltinTopicData();
    SubscriptionBuiltinTopicData observed;
    inspectDuringRepeatedBinding([&] { observed = owner->GetBuiltinTopicData(); });
    EXPECT_EQ(observed.key, expected.key);
    EXPECT_EQ(observed.topic_name, expected.topic_name);
    EXPECT_EQ(observed.type_name, expected.type_name);
}

TEST_P(ReaderBinding, PublishedHandleIsReadOnlyWhileMatchingDuringRepeatedBinding) {
    const auto expected = owner->get_instance_handle();
    GUID_t writer{};
    writer.prefix.value[0] = 0xa9;
    writer.entityId.entityKind = ENTITYKIND_USER_WRITER_NO_KEY;
    const auto publication = Conversion::ToInstanceHandle(writer);
    InstanceHandle_t observed;
    inspectDuringRepeatedBinding([&] {
        // StatusManager reads the same immutable handle before taking its
        // mutex. Inspect it before Notify's ingress barrier so that the old
        // post-barrier handle rewrite is tested regardless of mutex order.
        observed = owner->get_instance_handle();
        endpoint->NotifySubscriptionMatched(publication);
    });
    EXPECT_EQ(observed, expected);
    SubscriptionMatchedStatus status;
    ASSERT_EQ(owner->get_subscription_matched_status(status), RETCODE_OK);
    EXPECT_EQ(status.total_count, 1);
    EXPECT_EQ(status.current_count, 1);
    EXPECT_EQ(status.last_publication_handle, publication);
}

INSTANTIATE_TEST_SUITE_P(BothReliabilities, ReaderBinding,
    testing::Values(BEST_EFFORT_RELIABILITY_QOS, RELIABLE_RELIABILITY_QOS));

TEST(WriterFilter, RemovingAnAbsentReaderDoesNotRaceAnOrdinaryWrite) {
    BindingSupport type;
    DomainParticipantQos participantQos;
    participantQos.transport.removeTransport(TRANSPORT_SHM_QOS);
    auto* participant = TheParticipantFactory->create_participant(149, participantQos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(participant, nullptr);
    auto cleanup = std::shared_ptr<void>(nullptr, [&](void*) {
        EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);
        EXPECT_EQ(TheParticipantFactory->delete_participant(participant), RETCODE_OK);
    });
    ASSERT_EQ(participant->registe_type(type.get_typename(), &type), RETCODE_OK);
    auto* topic = participant->create_topic("writer-filter-removal", type.get_typename(),
        TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(topic, nullptr);
    auto* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(publisher, nullptr);
    DataWriterQos qos;
    qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
    auto* writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    auto* impl = dynamic_cast<DataWriterImpl*>(writer);
    ASSERT_NE(impl, nullptr);
    GUID_t absent{};
    absent.prefix.value[0] = 0xa9;
    absent.entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
    uint64_t sample = 42;
    ReturnCode_t result = RETCODE_ERROR;
    std::promise<void> release;
    auto start = release.get_future().share();
    // Removing a departing Reader calls erase even when no SQL filter was
    // installed. The old empty-map erase wrote the tree metadata concurrently
    // with write's empty check. These two branches have no happens-before edge.
    std::thread writing([&, start] {
        start.wait();
        result = writer->write(&sample, HANDLE_NIL);
    });
    std::thread removing([&, start] {
        start.wait();
        impl->RemoveReaderFilter(absent);
    });
    release.set_value();
    writing.join();
    removing.join();
    EXPECT_EQ(result, RETCODE_OK);
}
}
