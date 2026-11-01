#include "gtest/gtest.h"
#include "common/memory/sharedpayloadpool.h"
#include "common/memory/sharedpayloadpool_layout.h"
#include "common/memory/memorypool.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/listener/datareaderlistener.h"
#include "dcps/listener/datawriterlistener.h"
#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/reader.h"
#include "rtps/endpoint/statefulreader.h"
#include "rtps/endpoint/statelessreader.h"
#include "rtps/discovery/discoverydata.h"
#include <future>
#include "rtps/endpoint/writer.h"
#include "rtps/messages/messageprocessor.h"
#include "dcps/condition/guardcondition.h"
#include "dcps/condition/waitset.h"
#include <atomic>
#include <set>
#include <thread>
#include <filesystem>
#include <cerrno>
#if defined(__linux__)
#include <spawn.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
extern char** environ;
#endif

USING_TRAVODDS_NAMESPACE
using namespace std::chrono_literals;

namespace {
struct alignas(64) NativeSample { uint64_t index; uint64_t values[7]; };
void initializeNativeSample(NativeSample& sample, uint64_t index, uint64_t lastValue = 0) {
    // Writer Loans preserve the slot bytes; applications initialize all fields
    // that either native Readers or the CDR fallback may read.
    sample.index = index;
    for (unsigned i = 0; i < 6; ++i) sample.values[i] = 0;
    sample.values[6] = lastValue;
}
class NativeSupport : public NativeTypeSupport<NativeSample> {
public:
    NativeSupport() : NativeTypeSupport("ospp.NativeSample.v1.u64x8.align64") {}
    std::atomic<bool> capable{true}, failSerialize{false}, failDeserialize{false};
    std::atomic<bool> keyed{false}, failKey{false};
    std::atomic<bool> failConstruct{false}, throwConstruct{false};
    std::atomic<bool> compactKeys{true}, failCapture{false}, throwCapture{false};
    std::atomic<bool> failRestore{false}, throwRestore{false};
    std::atomic<int> malformedKeyInfo{0};
    std::atomic<int> serializeCalls{0}, deserializeCalls{0}, copyCalls{0}, serializeSuccesses{0};
    mutable std::atomic<int> keyCaptureCalls{0}, keyRestoreCalls{0};
    NativeTypeInfo get_native_type_info() const override {
        return capable ? NativeTypeSupport::get_native_type_info() : NativeTypeInfo{};
    }
    bool construct_native_sample(void* p) const override {
        if (throwConstruct) throw std::runtime_error("injected constructor failure");
        return !failConstruct && NativeTypeSupport::construct_native_sample(p);
    }
    NativeKeyInfo get_native_key_info() const override {
        if (!keyed || !compactKeys) return {};
        switch (malformedKeyInfo) {
        case 1: return {0, alignof(uint64_t)};
        case 2: return {sizeof(NativeSample) + 1, alignof(uint64_t)};
        case 3: return {sizeof(uint64_t), 3};
        case 4: return {sizeof(uint64_t), alignof(NativeSample) * 2};
        default: return {sizeof(uint64_t), alignof(uint64_t)};
        }
    }
    bool capture_native_key(void* key, const void* data) const override {
        ++keyCaptureCalls;
        if (throwCapture) throw std::runtime_error("injected key capture failure");
        if (failCapture || !key || !data) return false;
        std::memcpy(key, &static_cast<const NativeSample*>(data)->index, sizeof(uint64_t));
        return true;
    }
    bool restore_native_key(void* data, const void* key) const override {
        ++keyRestoreCalls;
        if (throwRestore) throw std::runtime_error("injected key restore failure");
        if (failRestore || !key || !data) return false;
        std::memcpy(&static_cast<NativeSample*>(data)->index, key, sizeof(uint64_t));
        return true;
    }
    void* create_data() override { return new NativeSample{}; }
    void delete_data(void* p) override { delete static_cast<NativeSample*>(p); }
    int32_t copy_data(void* dst, void* src) override {
        ++copyCalls;
        *static_cast<NativeSample*>(dst) = *static_cast<NativeSample*>(src);
        return 0;
    }
    uint32_t get_serialized_data_size(void*, uint32_t) override { return 64; }
    uint32_t get_max_serialized_data_size(void*, uint32_t) override { return 64; }
    int32_t serialize_data(void* p, CdrSerializer* cdr, int32_t) override {
        ++serializeCalls;
        if (failSerialize) return -1;
        auto& sample = *static_cast<NativeSample*>(p);
        if (!cdr->serializeBaseType(sample.index)) return -1;
        for (auto value : sample.values) if (!cdr->serializeBaseType(value)) return -1;
        ++serializeSuccesses;
        return 0;
    }
    int32_t deserialize_data(void* p, CdrDeserializer* cdr, int32_t) override {
        ++deserializeCalls;
        if (failDeserialize) return -1;
        auto& sample = *static_cast<NativeSample*>(p);
        if (!cdr->deserializeBaseType(sample.index)) return -1;
        for (auto& value : sample.values) if (!cdr->deserializeBaseType(value)) return -1;
        return 0;
    }
    TypeObject* get_typeobject() override { return nullptr; }
    ReturnCode_t get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& h, bool md5) override {
        h = HANDLE_NIL;
        if (!keyed) return RETCODE_OK;
        if (failKey || !cdr->serializeBaseType(static_cast<NativeSample*>(data)->index) ||
            !cdr->getKeyHash(reinterpret_cast<char*>(&h), md5)) return RETCODE_ERROR;
        return RETCODE_OK;
    }
    bool has_key() override { return keyed; }
    const char* get_typename() override { return "OSPPNativeLoan"; }
    int32_t MakeKey(const void* data, InstanceHandle_t& h, bool md5) override {
        char storage[64]{};
        SerializedBuffer buffer; buffer.buffer = storage; buffer.buffer_size = sizeof(storage);
        CdrSerializer cdr(&buffer);
        return get_instancehandle(const_cast<void*>(data), &cdr, h, md5) == RETCODE_OK ? 0 : -1;
    }
};

class LoanIntegration : public testing::Test {
protected:
    void SetUp() override {
        type.keyed = std::string(testing::UnitTest::GetInstance()->current_test_info()->name()).find("Keyed") != std::string::npos;
        DomainParticipantQos qos;
        qos.transport.removeTransport(TRANSPORT_SHM_QOS);
        participant = TheParticipantFactory->create_participant(138, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(participant, nullptr);
        ASSERT_EQ(participant->registe_type(type.get_typename(), &type), RETCODE_OK);
        std::string topicName = "native-loan";
        if (std::string(testing::UnitTest::GetInstance()->current_test_info()->name()) ==
                "LiveCapabilityDiscoveryAcrossExecPreservesRtpsFallback") {
            // Delayed discovery from earlier fixtures shares the domain/type.
            // Give this exec pair its own topic, and pass it to the child.
            const auto* peerTopic = getenv("TRAVODDS_TEST_DISCOVERY_TOPIC");
            topicName = peerTopic ? peerTopic : "native-loan-discovery-" + std::to_string(getpid());
        }
        topic = participant->create_topic(topicName.c_str(), type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(topic, nullptr);
        publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_TRUE(publisher && subscriber);
        writer = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(writer, nullptr);
    }
    void TearDown() override {
        if (participant) {
            EXPECT_EQ(participant->delete_contained_entities(), RETCODE_OK);
            EXPECT_EQ(TheParticipantFactory->delete_participant(participant), RETCODE_OK);
        }
    }
    void RunNativeExec(bool reliable, bool mixed, bool late = false, bool replay = true);
    void RunMappingRecoveryExec(bool reliable);
    void RunReaderCapacity(bool native);
    void RunMultiEndpointExec(bool reliable, bool crashReader, bool forceOwnerDeath = false);
    void RunWriterCrashExec(bool reliable);
    void RunDestinationFilter(bool fragmented);
    DataReader* makeReader() {
        return subscriber->create_datareader(topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    }
    NativeSupport type;
    DomainParticipant* participant = nullptr;
    Topic* topic = nullptr;
    Publisher* publisher = nullptr;
    Subscriber* subscriber = nullptr;
    DataWriter* writer = nullptr;
};

std::shared_ptr<HistoryCache> writerHistory(DomainParticipant* participant, DataWriter* writer) {
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    return rtps->GetWriter(Conversion::ToGuid(writer->get_instance_handle()).entityId)->GetHistoryCache();
}

TEST_F(LoanIntegration, KeyedDeadlineDoesNotInventAnInstanceBeforeFirstWrite) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos qos; qos.deadline.period = {3600, 0};
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    auto history = writerHistory(participant, writer);
    dynamic_cast<DataWriterImpl*>(writer)->DeadlineTimeout();
    EXPECT_FALSE(history->IsKeyRegistered(HANDLE_NIL));
    OfferedDeadlineMissedStatus status;
    ASSERT_EQ(writer->get_offered_deadline_missed_status(status), RETCODE_OK);
    EXPECT_EQ(status.total_count, 0);
}

TEST_F(LoanIntegration, KeyedDeadlineStaleCallbackDoesNotReportBeforeTheCurrentDueTime) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos qos; qos.deadline.period = {3600, 0};
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    NativeSample value{}; value.index = 7;
    ASSERT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_OK);
    dynamic_cast<DataWriterImpl*>(writer)->DeadlineTimeout();
    OfferedDeadlineMissedStatus status;
    ASSERT_EQ(writer->get_offered_deadline_missed_status(status), RETCODE_OK);
    EXPECT_EQ(status.total_count, 0);
}

TEST_F(LoanIntegration, KeyedDeadlineUnregisterCannotResurrectAHandleOrConsumeInstanceCapacity) {
    for (auto sharing : {OFF_DATA_SHARING_QOS, ON_DATA_SHARING_QOS}) {
        SCOPED_TRACE(sharing);
        ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
        DataWriterQos qos; qos.deadline.period = {3600, 0};
        qos.data_sharing.kind = sharing;
        qos.resource_limits.max_instances = 1;
        writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(writer, nullptr);
        NativeSample value{}; value.index = 7;
        const auto handle = writer->register_instance(&value);
        ASSERT_NE(handle, HANDLE_NIL);
        ASSERT_EQ(writer->write(&value, handle), RETCODE_OK);
        ASSERT_EQ(writer->unregister_instance(nullptr, handle), RETCODE_OK);
        dynamic_cast<DataWriterImpl*>(writer)->DeadlineTimeout();
        EXPECT_EQ(writer->lookup_instance(&value), HANDLE_NIL);
        ASSERT_FALSE(writerHistory(participant, writer)->IsKeyRegistered(handle));
        EXPECT_EQ(writer->write(&value, handle), RETCODE_BAD_PARAMETER);
        OfferedDeadlineMissedStatus status;
        ASSERT_EQ(writer->get_offered_deadline_missed_status(status), RETCODE_OK);
        EXPECT_EQ(status.total_count, 0);
        value.index = 9;
        EXPECT_NE(writer->register_instance(&value), HANDLE_NIL);
    }
}

TEST_F(LoanIntegration, KeyedDeadlineDisposeSuspendsMonitoringUntilANewWrite) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos qos; qos.deadline.period = {3600, 0};
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    NativeSample value{}; value.index = 7;
    ASSERT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_OK);
    const auto handle = writer->lookup_instance(&value);
    ASSERT_NE(handle, HANDLE_NIL);
    ASSERT_EQ(writer->dispose(nullptr, handle), RETCODE_OK);
    dynamic_cast<DataWriterImpl*>(writer)->DeadlineTimeout();
    EXPECT_EQ(writer->lookup_instance(&value), handle);
    OfferedDeadlineMissedStatus status;
    ASSERT_EQ(writer->get_offered_deadline_missed_status(status), RETCODE_OK);
    EXPECT_EQ(status.total_count, 0);
    ASSERT_EQ(writer->write(&value, handle), RETCODE_OK);
    writerHistory(participant, writer)->SetNextDeadline(handle, std::chrono::steady_clock::now() - 1s);
    dynamic_cast<DataWriterImpl*>(writer)->DeadlineTimeout();
    ASSERT_EQ(writer->get_offered_deadline_missed_status(status), RETCODE_OK);
    EXPECT_EQ(status.total_count, 1);
    EXPECT_EQ(status.last_instance_handle, handle);
}

TEST_F(LoanIntegration, KeyedDeadlineRemovingTheEarliestInstanceKeepsOtherInstancesMonitored) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos qos; qos.deadline.period = {3600, 0};
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    NativeSample first{}, second{}; first.index = 1; second.index = 2;
    ASSERT_EQ(writer->write(&first, HANDLE_NIL), RETCODE_OK);
    ASSERT_EQ(writer->write(&second, HANDLE_NIL), RETCODE_OK);
    const auto a = writer->lookup_instance(&first), b = writer->lookup_instance(&second);
    auto history = writerHistory(participant, writer);
    ASSERT_TRUE(history->SetNextDeadline(a, std::chrono::steady_clock::now() - 2s));
    ASSERT_TRUE(history->SetNextDeadline(b, std::chrono::steady_clock::now() - 1s));
    ASSERT_EQ(writer->unregister_instance(nullptr, a), RETCODE_OK);
    dynamic_cast<DataWriterImpl*>(writer)->DeadlineTimeout();
    OfferedDeadlineMissedStatus status;
    ASSERT_EQ(writer->get_offered_deadline_missed_status(status), RETCODE_OK);
    EXPECT_EQ(status.total_count, 1);
    EXPECT_EQ(status.last_instance_handle, b);
    EXPECT_EQ(writer->lookup_instance(&first), HANDLE_NIL);
    EXPECT_EQ(writer->lookup_instance(&second), b);
}

TEST_F(LoanIntegration, KeyedDeadlineListenerCanUnregisterTheExpiredInstance) {
    struct Observer : DataWriterListener {
        unsigned calls = 0;
        ReturnCode_t result = RETCODE_ERROR;
        void on_offered_deadline_missed(DataWriter* writer, const OfferedDeadlineMissedStatus& status) override {
            ++calls;
            result = writer->unregister_instance(nullptr, status.last_instance_handle);
        }
    } observer;
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos qos; qos.deadline.period = {3600, 0};
    writer = publisher->create_datawriter(topic, qos, &observer, OFFERED_DEADLINE_MISSED_STATUS);
    ASSERT_NE(writer, nullptr);
    NativeSample value{}; value.index = 7;
    ASSERT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_OK);
    const auto handle = writer->lookup_instance(&value);
    ASSERT_TRUE(writerHistory(participant, writer)->SetNextDeadline(handle, std::chrono::steady_clock::now() - 1s));
    dynamic_cast<DataWriterImpl*>(writer)->DeadlineTimeout();
    EXPECT_EQ(observer.calls, 1u);
    EXPECT_EQ(observer.result, RETCODE_OK);
    EXPECT_EQ(writer->lookup_instance(&value), HANDLE_NIL);
    // The next stale wakeup neither resurrects the key nor re-enters the listener.
    dynamic_cast<DataWriterImpl*>(writer)->DeadlineTimeout();
    EXPECT_EQ(observer.calls, 1u);
    ASSERT_EQ(writer->set_listener(nullptr, STATUS_MASK_NONE), RETCODE_OK);
}

TEST(PayloadPoolHistory, DeadlineSelectionReportsOnlyArmedInstances) {
    HistoryCache history(HistoryQosPolicy{}, ResourceLimitsQosPolicy{}, TopicKind_t{WITH_KEY});
    InstanceHandle_t first, second, result;
    first.keyHash.value[0] = 1; second.keyHash.value[0] = 2;
    ASSERT_EQ(history.RegisterKey(first, {}, {}), RETCODE_OK);
    ASSERT_EQ(history.RegisterKey(second, {}, {}), RETCODE_OK);
    auto due = std::chrono::steady_clock::now() + 1h;
    EXPECT_FALSE(history.GetNextDeadline(result, due));
    history.SetNextDeadline(second, due);
    auto found = std::chrono::steady_clock::time_point{};
    EXPECT_TRUE(history.GetNextDeadline(result, found));
    EXPECT_EQ(result, second);
    EXPECT_EQ(found, due);
}

TEST(PayloadPoolHistory, DeadlineUpdateCannotRegisterAnUnknownOrRemovedInstance) {
    HistoryCache history(HistoryQosPolicy{}, ResourceLimitsQosPolicy{}, TopicKind_t{WITH_KEY});
    InstanceHandle_t handle; handle.keyHash.value[0] = 1;
    auto due = std::chrono::steady_clock::now() + 1h;
    history.SetNextDeadline(handle, due);
    EXPECT_FALSE(history.IsKeyRegistered(handle));
    ASSERT_EQ(history.RegisterKey(handle, {}, {}), RETCODE_OK);
    history.DeleteKeyHistory(handle);
    history.SetNextDeadline(handle, due);
    EXPECT_FALSE(history.IsKeyRegistered(handle));
}

TEST(PayloadPoolHistory, DeadlineTimeoutCannotRearmAnInstanceAfterItsLifecycleChanged) {
    const struct { const char* name; ChangeKind_t kind; bool writerLost; } transitions[] = {
        {"disposed", NOT_ALIVE_DISPOSED, false},
        {"unregistered", NOT_ALIVE_UNREGISTERED, false},
        {"disposed and unregistered", NOT_ALIVE_DISPOSED_UNREGISTERED, false},
        {"last Writer lost", ALIVE, true},
    };
    for (const auto& transition : transitions) {
        SCOPED_TRACE(transition.name);
        StatelessReader ingress(DataReaderQos{}, GUID_UNKNOWN, TopicKind_t{WITH_KEY});
        auto history = ingress.GetReaderCache();
        InstanceHandle_t handle; handle.keyHash.value[0] = 11;
        GUID_t source; source.prefix.value[0] = 9;
        auto alive = ingress.new_change(ALIVE, nullptr, handle, Conversion::ToInstanceHandle(source), TIME_ZERO);
        alive->sequenceNumber = {0, 1};
        ASSERT_EQ(ingress.AddChanges(alive), RETCODE_OK);
        ASSERT_TRUE(history->SetNextDeadline(handle, std::chrono::steady_clock::now() - 1s));
        InstanceHandle_t selected;
        std::chrono::steady_clock::time_point observed;
        ASSERT_TRUE(history->GetNextDeadline(selected, observed));
        ASSERT_EQ(selected, handle);
        // Model an ingress/departure between the timer's due lookup and its
        // timeout advance. The instance remains known, but is no longer ALIVE.
        if (transition.writerLost) history->RemoveRemoteWriter(source);
        else {
            auto control = ingress.new_change(transition.kind, nullptr, handle, Conversion::ToInstanceHandle(source), TIME_ZERO);
            control->sequenceNumber = {0, 2};
            ASSERT_EQ(ingress.AddChanges(control), RETCODE_OK);
        }
        EXPECT_FALSE(history->SetNextDeadline(selected, std::chrono::steady_clock::now() + 1h, true));
        EXPECT_FALSE(history->GetNextDeadline(selected, observed));
        EXPECT_TRUE(history->IsKeyRegistered(handle));
    }
}

#if defined(__linux__)
TEST_F(LoanIntegration, KeyedLifespanExpiryAndUnregisterDoNotInvalidateOrPrematurelyRecycleAReaderLoan) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.data_sharing.max_samples = 2;
    qos.history.depth = 1;
    qos.lifespan.duration = {1, 0};
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    DataReaderQos rq; rq.data_sharing.kind = ON_DATA_SHARING_QOS;
    auto* reader = subscriber->create_datareader(topic, rq, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto endpoint = rtps->GetWriter(Conversion::ToGuid(writer->get_instance_handle()).entityId);
    const auto& info = dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData().data_sharing;
    SharedPoolDescriptor descriptor;
    descriptor.writer = endpoint->GetGuid(); descriptor.session = {info.session_high, info.session_low};
    descriptor.path = SharedPayloadPool::pathFor(descriptor.writer, descriptor.session, qos.data_sharing.shared_memory_directory);
    descriptor.config.payload_size = info.type.size;
    descriptor.config.alignment = std::max(64u, info.type.alignment);
    descriptor.config.slots = info.slots; descriptor.config.max_readers = info.max_readers;
    descriptor.config.max_segment_bytes = qos.data_sharing.max_segment_bytes;
    descriptor.segment_bytes = info.segment_bytes;
    auto pool = SharedPayloadPool::open(descriptor);
    auto deadline = std::chrono::steady_clock::now() + 4s;
    while (endpoint->SelectDataSharingReaders(pool->registeredReaders()).size() != 1 &&
            std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(2ms);
    InstanceHandleSeq matchedReaders, matchedWriters;
    writer->get_matched_subscriptions(matchedReaders);
    reader->get_matched_publications(matchedWriters);
    PublicationBuiltinTopicData seenWriter; SubscriptionBuiltinTopicData seenReader;
    const auto writerQuery = reader->get_matched_publication_data(seenWriter, writer->get_instance_handle());
    const auto readerQuery = writer->get_matched_subscription_data(seenReader, reader->get_instance_handle());
    if (endpoint->SelectDataSharingReaders(pool->registeredReaders()).size() != 1) {
        auto discoveryWriter = rtps->GetWriter(ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER);
        auto discoveryReader = std::dynamic_pointer_cast<StatefulReader>(rtps->GetReader(ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER));
        auto proxy = discoveryReader->MatchedWriterLookup(discoveryWriter->GetGuid());
        const auto minimum = discoveryWriter->GetHistoryCache()->GetSeqNumMin();
        const auto maximum = discoveryWriter->GetHistoryCache()->GetSeqNumMax();
        std::cerr << "SEDP publication history " << minimum.high << ':' << minimum.low << ".."
                  << maximum.high << ':' << maximum.low << " self proxy=" << bool(proxy)
                  << " has last cache=" << (proxy && proxy->ExistCache(maximum)) << std::endl;
    }
    ASSERT_EQ(endpoint->SelectDataSharingReaders(pool->registeredReaders()).size(), 1u)
        << "registered=" << pool->registeredReaders().size() << " writer matches=" << matchedReaders.size()
        << " reader matches=" << matchedWriters.size() << " queries=" << writerQuery << "," << readerQuery
        << " advertised compatible=" << seenWriter.data_sharing.compatible_with(seenReader.data_sharing)
        << " versions=" << seenWriter.data_sharing.version << "," << seenReader.data_sharing.version
        << " segments=" << info.session_low << "," << seenWriter.data_sharing.session_low;
    type.failSerialize = type.failDeserialize = true;
    SampleLoan writable;
    ASSERT_EQ(writer->loan_sample(writable), RETCODE_OK);
    auto* value = static_cast<NativeSample*>(writable.data());
    initializeNativeSample(*value, 73, 7399);
    const auto handle = writer->register_instance(value);
    ASSERT_NE(handle, HANDLE_NIL);
    ASSERT_EQ(writer->write(writable, handle), RETCODE_OK);
    auto history = endpoint->GetHistoryCache();
    const auto sequence = history->GetSeqNumMax();
    ASSERT_NE(history->GetChange(sequence), nullptr);
    deadline = std::chrono::steady_clock::now() + 4s;
    while (reader->get_unread_count() == 0 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(2ms);
    SequenceData<NativeSample> held; SampleInfoSeq infos;
    ASSERT_EQ(reader->take(held, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    ASSERT_TRUE(held.has_loan());
    while (history->GetChange(sequence) && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(5ms);
    ASSERT_EQ(history->GetChange(sequence), nullptr); // Actual lifespan timer ran.
    ASSERT_EQ(writer->unregister_instance(nullptr, handle), RETCODE_OK);
    EXPECT_EQ(writer->lookup_instance(&held[0]), HANDLE_NIL);
    EXPECT_EQ(held[0].index, 73u);
    EXPECT_EQ(held[0].values[6], 7399u);
    SampleLoan spare, exhausted;
    ASSERT_EQ(writer->loan_sample(spare), RETCODE_OK);
    EXPECT_EQ(writer->loan_sample(exhausted), RETCODE_OUT_OF_RESOURCES);
    ASSERT_EQ(reader->return_loan(held, infos), RETCODE_OK);
    ASSERT_EQ(writer->loan_sample(exhausted), RETCODE_OK);
    EXPECT_EQ(type.serializeCalls, 0);
    EXPECT_EQ(type.deserializeCalls, 0);
}
#endif

#if defined(__linux__)
TEST_F(LoanIntegration, MatchingCallbacksCanImmediatelyQueryRemoteEndpointMetadata) {
    struct ReaderObserver : DataReaderListener {
        InstanceHandle_t expected;
        std::atomic<int> result{RETCODE_ERROR};
        std::atomic<bool> seen{false};
        void on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& status) override {
            if (status.last_publication_handle != expected || status.current_count_change <= 0) return;
            PublicationBuiltinTopicData data;
            result = reader->get_matched_publication_data(data, expected);
            seen = true;
        }
    } readerObserver;
    struct WriterObserver : DataWriterListener {
        InstanceHandle_t expected;
        std::atomic<int> result{RETCODE_ERROR};
        std::atomic<bool> seen{false};
        void on_publication_matched(DataWriter* writer, const PublicationMatchedStatus& status) override {
            if (status.last_subscription_handle != expected || status.current_count_change <= 0) return;
            SubscriptionBuiltinTopicData data;
            result = writer->get_matched_subscription_data(data, expected);
            seen = true;
        }
    } writerObserver;
    GUID_t remoteWriter{}, remoteReader{};
    remoteWriter.prefix.value[0] = remoteReader.prefix.value[0] = 0xa5;
    remoteWriter.entityId.entityKind = ENTITYKIND_USER_WRITER_NO_KEY;
    remoteReader.entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
    readerObserver.expected = Conversion::ToInstanceHandle(remoteWriter);
    writerObserver.expected = Conversion::ToInstanceHandle(remoteReader);
    DataReaderQos qos; qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
    auto* reader = subscriber->create_datareader(topic, qos, &readerObserver, SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_NE(reader, nullptr);
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto cleanup = std::shared_ptr<void>(nullptr, [&](void*) {
        writer->set_listener(nullptr, STATUS_MASK_NONE);
        reader->set_listener(nullptr, STATUS_MASK_NONE);
        rtps->RemoveRemoteWriter(remoteWriter);
        rtps->RemoveRemoteReader(remoteReader);
        subscriber->delete_datareader(reader);
    });
    ASSERT_EQ(writer->set_listener(&writerObserver, PUBLICATION_MATCHED_STATUS), RETCODE_OK);
    DiscoveredWriterData publication;
    publication.remoteWriterGuid = remoteWriter;
    publication.ddsPublicationData = dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData();
    publication.ddsPublicationData.data_sharing = {};
    ASSERT_EQ(rtps->AddRemoteWriter(publication), RETCODE_OK);
    EXPECT_TRUE(readerObserver.seen.load());
    EXPECT_EQ(readerObserver.result.load(), RETCODE_OK);
    DiscoveredReaderData subscription;
    subscription.remoteReaderGuid = remoteReader;
    subscription.ddsSubscriptionData = dynamic_cast<DataReaderImpl*>(reader)->GetBuiltinTopicData();
    subscription.ddsSubscriptionData.data_sharing = {};
    ASSERT_EQ(rtps->AddRemoteReader(subscription), RETCODE_OK);
    EXPECT_TRUE(writerObserver.seen.load());
    EXPECT_EQ(writerObserver.result.load(), RETCODE_OK);
}

TEST_F(LoanIntegration, ConfiguredPoolCapacityAndImmutableQosReachWriterAllocation) {
    DataWriterQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.data_sharing.max_samples = 2;
    qos.data_sharing.max_readers = 3;
    qos.data_sharing.max_segment_bytes = 16384;
    qos.data_sharing.domain_ids = {37, 41};
    auto* configured = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(configured, nullptr);
    DataWriterQos actual;
    ASSERT_EQ(configured->get_qos(actual), RETCODE_OK);
    EXPECT_EQ(actual.data_sharing, qos.data_sharing);
    const auto& capability = dynamic_cast<DataWriterImpl*>(configured)->GetBuiltinTopicData().data_sharing;
    EXPECT_TRUE(capability.valid(true));
    EXPECT_EQ(capability.slots, 2u);
    EXPECT_EQ(capability.max_readers, 3u);
    EXPECT_EQ(capability.domain_ids, qos.data_sharing.domain_ids);
    SampleLoan first, second, exhausted;
    ASSERT_EQ(configured->loan_sample(first), RETCODE_OK);
    ASSERT_EQ(configured->loan_sample(second), RETCODE_OK);
    EXPECT_EQ(configured->loan_sample(exhausted), RETCODE_OUT_OF_RESOURCES);
    first.reset();
    EXPECT_EQ(configured->loan_sample(exhausted), RETCODE_OK);
    actual.data_sharing.max_samples = 3;
    EXPECT_EQ(configured->set_qos(actual), RETCODE_IMMUTABLE_POLICY);
    ASSERT_EQ(configured->get_qos(actual), RETCODE_OK);
    EXPECT_EQ(actual.data_sharing.max_samples, 2u);
    EXPECT_EQ(publisher->delete_datawriter(configured), RETCODE_OK);
    // Existing reservations keep their mapped bytes until the application drops them.
    EXPECT_TRUE(second);
    second.reset(); exhausted.reset();
}

TEST_F(LoanIntegration, OffDisablesNativeAllocationAndQosDefaultsRoundTrip) {
    DataWriterQos wq;
    wq.data_sharing.kind = OFF_DATA_SHARING_QOS;
    wq.data_sharing.domain_ids = {9};
    ASSERT_EQ(publisher->set_default_datawriter_qos(wq), RETCODE_OK);
    auto* off = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(off, nullptr);
    SampleLoan loan;
    EXPECT_EQ(off->loan_sample(loan), RETCODE_UNSUPPORTED);
    NativeSample sample{};
    EXPECT_EQ(off->write(&sample, HANDLE_NIL), RETCODE_OK);
    DataWriterQos actual;
    ASSERT_EQ(off->get_qos(actual), RETCODE_OK);
    EXPECT_EQ(actual.data_sharing, wq.data_sharing);
    DataReaderQos rq;
    rq.data_sharing = wq.data_sharing;
    ASSERT_EQ(subscriber->set_default_datareader_qos(rq), RETCODE_OK);
    auto* reader = makeReader();
    ASSERT_NE(reader, nullptr);
    DataReaderQos readback;
    ASSERT_EQ(reader->get_qos(readback), RETCODE_OK);
    EXPECT_EQ(readback.data_sharing, rq.data_sharing);
    readback.data_sharing.kind = AUTO_DATA_SHARING_QOS;
    EXPECT_EQ(reader->set_qos(readback), RETCODE_IMMUTABLE_POLICY);
}

TEST_F(LoanIntegration, OnRejectsUnsupportedTypeWhileAutoKeepsOrdinaryRtps) {
    type.capable = false;
    DataWriterQos wq;
    wq.data_sharing.kind = ON_DATA_SHARING_QOS;
    EXPECT_EQ(publisher->create_datawriter(topic, wq, nullptr, STATUS_MASK_NONE), nullptr);
    DataReaderQos rq;
    rq.data_sharing.kind = ON_DATA_SHARING_QOS;
    EXPECT_EQ(subscriber->create_datareader(topic, rq, nullptr, STATUS_MASK_NONE), nullptr);
    wq.data_sharing.kind = AUTO_DATA_SHARING_QOS;
    auto* ordinary = publisher->create_datawriter(topic, wq, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(ordinary, nullptr);
    NativeSample sample{};
    EXPECT_EQ(ordinary->write(&sample, HANDLE_NIL), RETCODE_OK);
    EXPECT_EQ(type.serializeCalls, 1);
}

TEST_F(LoanIntegration, OnFailsUnavailableDirectoryOrBudgetWhileAutoAllowsFallback) {
    DataWriterQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.data_sharing.shared_memory_directory = "/proc/travodds-no-shared-pool";
    EXPECT_EQ(publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE), nullptr);
    qos.data_sharing.kind = AUTO_DATA_SHARING_QOS;
    auto* fallback = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(fallback, nullptr);
    NativeSample sample{};
    EXPECT_EQ(fallback->write(&sample, HANDLE_NIL), RETCODE_OK);
    SampleLoan loan;
    EXPECT_NE(fallback->loan_sample(loan), RETCODE_OK);
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.data_sharing.shared_memory_directory = "/dev/shm";
    qos.data_sharing.max_segment_bytes = 1;
    EXPECT_EQ(publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE), nullptr);
}

TEST_F(LoanIntegration, UsesConfiguredDirectoryAndRemovesOwnedSegmentOnDeletion) {
    char path[] = "/tmp/travodds-qos-directory-XXXXXX";
    ASSERT_NE(mkdtemp(path), nullptr);
    struct Directory {
        const char* path;
        ~Directory() { std::filesystem::remove(path); }
    } cleanup{path};
    DataWriterQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.data_sharing.shared_memory_directory = path;
    auto* configured = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(configured, nullptr);
    EXPECT_EQ(std::distance(std::filesystem::directory_iterator(path), std::filesystem::directory_iterator{}), 1);
    SampleLoan loan;
    ASSERT_EQ(configured->loan_sample(loan), RETCODE_OK);
    EXPECT_EQ(publisher->delete_datawriter(configured), RETCODE_OK);
    EXPECT_TRUE(std::filesystem::is_empty(path));
    // Unlinking the name must not invalidate this application's live Loan.
    static_cast<NativeSample*>(loan.data())->index = 57;
    EXPECT_EQ(static_cast<NativeSample*>(loan.data())->index, 57u);
}

TEST_F(LoanIntegration, LiveCapabilityDiscoveryAcrossExecPreservesRtpsFallback) {
    const auto handleHex = [](const InstanceHandle_t& handle) {
        const char digits[] = "0123456789abcdef";
        std::string text;
        for (auto byte : handle.keyHash.value) {
            text += digits[byte >> 4];
            text += digits[byte & 15];
        }
        return text;
    };
    if (getenv("TRAVODDS_TEST_DISCOVERY_CHILD")) {
        // Drop the fixture's Writer so the only discoverable publication is
        // in the other process, never a local-match shortcut.
        ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
        writer = nullptr;
        DataReaderQos qos;
        qos.data_sharing.kind = ON_DATA_SHARING_QOS;
        qos.data_sharing.domain_ids = {1};
        qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(reader, nullptr);
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        InstanceHandleSeq handles;
        while (std::chrono::steady_clock::now() < deadline) {
            ASSERT_EQ(reader->get_matched_publications(handles), RETCODE_OK);
            if (!handles.empty()) break;
            std::this_thread::sleep_for(10ms);
        }
        ASSERT_EQ(handles.size(), 1u);
        // Check the intended live peer before its data lookup. An unrelated
        // match is not evidence of this parent's capability advertisement.
        const auto* expected = getenv("TRAVODDS_TEST_DISCOVERY_WRITER");
        ASSERT_NE(expected, nullptr);
        ASSERT_EQ(handleHex(handles[0]), expected);
        PublicationBuiltinTopicData remote;
        ASSERT_EQ(reader->get_matched_publication_data(remote, handles[0]), RETCODE_OK);
        ASSERT_TRUE(remote.data_sharing.valid(true));
        EXPECT_EQ(remote.data_sharing.type, type.get_native_type_info());
        EXPECT_EQ(remote.data_sharing.slots, 64u);
        NativeSample received{}; SampleInfo info;
        ReturnCode_t result = RETCODE_NO_DATA;
        while (std::chrono::steady_clock::now() < deadline) {
            result = reader->take_next_sample(&received, info);
            if (result != RETCODE_NO_DATA) break;
            std::this_thread::sleep_for(10ms);
        }
        ASSERT_EQ(result, RETCODE_OK);
        EXPECT_TRUE(info.valid_data);
        EXPECT_EQ(received.index, 9182u);
        EXPECT_EQ(received.values[6], 7261u);
        EXPECT_GT(type.deserializeCalls, 0);
        return;
    }
    std::vector<std::string> environment;
    for (auto** p = environ; *p; ++p) environment.emplace_back(*p);
    environment.emplace_back("TRAVODDS_TEST_DISCOVERY_CHILD=1");
    environment.emplace_back("TRAVODDS_TEST_DISCOVERY_TOPIC=" + std::string(topic->get_name()));
    environment.emplace_back("TRAVODDS_TEST_DISCOVERY_WRITER=" + handleHex(writer->get_instance_handle()));
    std::vector<char*> env;
    for (auto& entry : environment) env.push_back(entry.data());
    env.push_back(nullptr);
    char executable[] = "/proc/self/exe";
    char filter[] = "--gtest_filter=LoanIntegration.LiveCapabilityDiscoveryAcrossExecPreservesRtpsFallback";
    char* args[] = {executable, filter, nullptr};
    pid_t pid = -1;
    ASSERT_EQ(posix_spawn(&pid, executable, nullptr, nullptr, args, env.data()), 0);
    struct Child {
        pid_t pid;
        ~Child() { if (pid > 0) { kill(pid, SIGKILL); waitpid(pid, nullptr, 0); } }
    } child{pid};
    const auto deadline = std::chrono::steady_clock::now() + 12s;
    InstanceHandleSeq handles;
    while (std::chrono::steady_clock::now() < deadline) {
        ASSERT_EQ(writer->get_matched_subscriptions(handles), RETCODE_OK);
        if (!handles.empty()) break;
        std::this_thread::sleep_for(10ms);
    }
    ASSERT_EQ(handles.size(), 1u);
    SubscriptionBuiltinTopicData remote;
    ASSERT_EQ(writer->get_matched_subscription_data(remote, handles[0]), RETCODE_OK);
    EXPECT_TRUE(remote.data_sharing.valid(false));
    EXPECT_FALSE(dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData().data_sharing.compatible_with(remote.data_sharing));
    NativeSample sample{};
    sample.index = 9182; sample.values[6] = 7261;
    ASSERT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
    EXPECT_GT(type.serializeCalls, 0);
    int status = 0;
    pid_t reaped = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        reaped = waitpid(pid, &status, WNOHANG);
        if (reaped != 0) break;
        std::this_thread::sleep_for(10ms);
    }
    ASSERT_EQ(reaped, pid);
    child.pid = -1;
    ASSERT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);
}

void LoanIntegration::RunNativeExec(bool reliable, bool mixed, bool late, bool replay) {
    auto byte = [](int fd, char expected, bool sending) {
        pollfd item{fd, short(sending ? POLLOUT : POLLIN), 0};
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        int ready;
        do {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now()).count();
            ready = poll(&item, 1, int(std::max<int64_t>(0, remaining)));
        } while (ready < 0 && errno == EINTR && std::chrono::steady_clock::now() < deadline);
        if (ready <= 0) {
            ADD_FAILURE() << "Control socket poll: pid=" << getpid() << " phase=" << expected
                          << " result=" << ready << " errno=" << errno << " revents=" << item.revents;
            return false;
        }
        char value = expected;
        ssize_t size;
        do {
            size = sending ? send(fd, &value, 1, MSG_NOSIGNAL) : recv(fd, &value, 1, 0);
        } while (size < 0 && errno == EINTR);
        if (size != 1 || (!sending && value != expected)) {
            ADD_FAILURE() << "Control socket transfer: pid=" << getpid() << " phase=" << expected
                          << " size=" << size << " value=" << int(value) << " errno=" << errno
                          << " revents=" << item.revents;
            return false;
        }
        return true;
    };
    if (getenv("TRAVODDS_NATIVE_EXEC_CHILD")) {
        const int fd = std::stoi(getenv("TRAVODDS_NATIVE_EXEC_CHILD"));
        close(std::stoi(getenv("TRAVODDS_NATIVE_EXEC_PARENT")));
        ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
        writer = nullptr;
        type.failSerialize = true;
        type.failDeserialize = !mixed;
        DataReaderQos qos;
        qos.data_sharing.kind = ON_DATA_SHARING_QOS;
        qos.reliability.kind = reliable ? RELIABLE_RELIABILITY_QOS : BEST_EFFORT_RELIABILITY_QOS;
        qos.history.depth = 8;
        if (late && replay) qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(reader, nullptr);
        DataReader* fallback = nullptr;
        if (mixed) {
            if (late) {
                // Join the native Reader first, then create a late OFF Reader
                // against the now-discovered Writer and its retained native history.
                InstanceHandleSeq publications;
                const auto matchDeadline = std::chrono::steady_clock::now() + 8s;
                do {
                    ASSERT_EQ(reader->get_matched_publications(publications), RETCODE_OK);
                    if (publications.size() == 1u) break;
                    std::this_thread::sleep_for(2ms);
                } while (std::chrono::steady_clock::now() < matchDeadline);
                ASSERT_EQ(publications.size(), 1u);
                // DDS matching precedes asynchronous shared registration. The
                // parent keeps payload CDR disabled until registration succeeds.
                ASSERT_TRUE(byte(fd, 'N', true));
                ASSERT_TRUE(byte(fd, 'K', false));
                if (replay) {
                    const auto ready = std::chrono::steady_clock::now() + 8s;
                    while (reader->get_unread_count() != 3 && std::chrono::steady_clock::now() < ready)
                        std::this_thread::sleep_for(2ms);
                    ASSERT_EQ(reader->get_unread_count(), 3u);
                }
            }
            qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
            fallback = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
            ASSERT_NE(fallback, nullptr);
        }
        ASSERT_TRUE(byte(fd, 'M', true));
        SequenceData<NativeSample> held;
        SampleInfoSeq heldInfo;
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        for (uint64_t index = 9182; index < 9185; ++index) {
            SequenceData<NativeSample> values;
            SampleInfoSeq infos;
            ReturnCode_t result = RETCODE_NO_DATA;
            while (std::chrono::steady_clock::now() < deadline) {
                result = reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
                if (result != RETCODE_NO_DATA) break;
                std::this_thread::sleep_for(2ms);
            }
            ASSERT_EQ(result, RETCODE_OK);
            ASSERT_EQ(values.length(), 1u);
            ASSERT_TRUE(values.has_loan());
            EXPECT_FALSE(values.has_ownership());
            EXPECT_EQ(values[0].index, index);
            EXPECT_EQ(values[0].values[6], index + 79);
            EXPECT_TRUE(infos[0].valid_data);
            if (type.keyed) {
                InstanceHandle_t expected;
                ASSERT_EQ(type.MakeKey(&values[0], expected, true), 0);
                EXPECT_EQ(infos[0].instance_handle, expected);
                EXPECT_EQ(reader->lookup_instance(&values[0]), expected);
                NativeSample snapshot{}; snapshot.values[6] = 8866;
                for (int query = 0; query < 2; ++query) {
                    ASSERT_EQ(reader->get_key_value(&snapshot, expected), RETCODE_OK);
                    EXPECT_EQ(snapshot.index, index);
                    EXPECT_EQ(snapshot.values[6], 8866u);
                }
                EXPECT_EQ(reader->get_key_value(nullptr, expected), RETCODE_BAD_PARAMETER);
            }
            if (index == 9183) {
                EXPECT_EQ(infos[0].source_timestamp.nanosec, 456789u);
                EXPECT_EQ(infos[0].source_timestamp.sec, 123);
            }
            if (index == 9182) { held = std::move(values); heldInfo = std::move(infos); }
            else ASSERT_EQ(reader->return_loan(values, infos), RETCODE_OK);
            if (fallback) {
                NativeSample copied{}; SampleInfo info;
                result = RETCODE_NO_DATA;
                while (std::chrono::steady_clock::now() < deadline) {
                    result = fallback->take_next_sample(&copied, info);
                    if (result != RETCODE_NO_DATA) break;
                    std::this_thread::sleep_for(2ms);
                }
                ASSERT_EQ(result, RETCODE_OK);
                EXPECT_EQ(copied.index, index);
                EXPECT_EQ(copied.values[6], index + 79);
            }
            if (type.keyed) ASSERT_TRUE(byte(fd, 'S', true));
        }
        NativeSample extra{}; SampleInfo extraInfo;
        EXPECT_EQ(reader->take_next_sample(&extra, extraInfo), RETCODE_NO_DATA);
        if (fallback) EXPECT_EQ(fallback->take_next_sample(&extra, extraInfo), RETCODE_NO_DATA);
        EXPECT_EQ(type.serializeCalls, 0);
        EXPECT_EQ(type.deserializeCalls, mixed ? 3 : 0);
        ASSERT_TRUE(byte(fd, 'R', true));
        if (type.keyed) {
            for (char phase : {'P', 'Q', 'U', 'V'}) {
                for (auto* target : {reader, fallback}) {
                    if (!target) continue;
                    NativeSample value{}; SampleInfo info;
                    ReturnCode_t result = RETCODE_NO_DATA;
                    const auto controlDeadline = std::chrono::steady_clock::now() + 8s;
                    while (std::chrono::steady_clock::now() < controlDeadline) {
                        result = target->take_next_sample(&value, info);
                        if (result != RETCODE_NO_DATA) break;
                        std::this_thread::sleep_for(2ms);
                    }
                    ASSERT_EQ(result, RETCODE_OK) << phase;
                    NativeSample expected{}; expected.index = 9184;
                    InstanceHandle_t expectedHandle;
                    ASSERT_EQ(type.MakeKey(&expected, expectedHandle, true), 0);
                    EXPECT_EQ(info.instance_handle, expectedHandle);
                    EXPECT_EQ(info.valid_data, phase == 'Q' || phase == 'V');
                    EXPECT_EQ(info.instance_state, phase == 'P' ? NOT_ALIVE_DISPOSED_INSTANCE_STATE :
                        phase == 'U' ? NOT_ALIVE_NO_WRITERS_INSTANCE_STATE : ALIVE_INSTANCE_STATE);
                    if (phase == 'Q' || phase == 'V') {
                        EXPECT_EQ(value.index, 9184u);
                        EXPECT_EQ(value.values[6], phase == 'Q' ? 666u : 777u);
                        EXPECT_EQ(info.disposed_generation_count, 1);
                        EXPECT_EQ(info.no_writers_generation_count, phase == 'Q' ? 0 : 1);
                    }
                }
                ASSERT_TRUE(byte(fd, phase, true));
            }
            EXPECT_EQ(type.serializeCalls, 0);
            EXPECT_EQ(type.deserializeCalls, mixed ? 5 : 0);
            if (fallback) {
                NativeSample sample{}; sample.index = 9184;
                const auto handle = fallback->lookup_instance(&sample);
                ASSERT_NE(handle, HANDLE_NIL);
                for (int query = 0; query < 2; ++query) {
                    ASSERT_EQ(fallback->get_key_value(&sample, handle), RETCODE_OK);
                    EXPECT_EQ(sample.index, 9184u);
                    EXPECT_EQ(sample.values[6], 9263u);
                }
                EXPECT_EQ(type.deserializeCalls, 7); // five takes and two explicit CDR key queries
            }
            NativeSample unknown{};
            EXPECT_EQ(reader->lookup_instance(&unknown), HANDLE_NIL);
            EXPECT_EQ(reader->lookup_instance(nullptr), HANDLE_NIL);
            EXPECT_EQ(reader->get_key_value(&unknown, HANDLE_NIL), RETCODE_BAD_PARAMETER);
            unknown.index = 9182;
            type.failKey = true;
            EXPECT_EQ(reader->lookup_instance(&unknown), HANDLE_NIL);
            type.failKey = false;
        }
        ASSERT_TRUE(byte(fd, 'D', false));
        ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
        EXPECT_EQ(held[0].index, 9182u);
        EXPECT_EQ(held[0].values[6], 9261u);
        // Reader deletion joins its native listener; keep the explicit
        // application Loan alive until the parent checks the closed pool.
        ASSERT_TRUE(byte(fd, 'C', true));
        ASSERT_TRUE(byte(fd, 'L', false));
        held.unloan(); heldInfo.unloan();
        close(fd);
        return;
    }
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.reliability.kind = reliable ? RELIABLE_RELIABILITY_QOS : BEST_EFFORT_RELIABILITY_QOS;
    qos.history.depth = 8;
    if (late) qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    qos.writer_data_lifecycle.autodispose_unregistered_instances = false;
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    type.failSerialize = !mixed || late;
    // Publish before the other process exists; discovery/mapping cannot make
    // these samples native retroactively. One ordinary write and two Loans.
    if (late) for (uint64_t index = replay ? 9182 : 8000; index < (replay ? 9185 : 8003); ++index) {
        if (index == 9183) {
            NativeSample sample{}; sample.index = index; sample.values[6] = index + 79;
            ASSERT_EQ(writer->write_w_timestamp(&sample, HANDLE_NIL, {123, 456789}), RETCODE_OK);
        } else {
            SampleLoan loan;
            ASSERT_EQ(writer->loan_sample(loan), RETCODE_OK);
            auto* sample = static_cast<NativeSample*>(loan.data());
            initializeNativeSample(*sample, index, index + 79);
            ASSERT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_OK);
        }
    }
    if (late) EXPECT_EQ(type.serializeCalls, 0);
    int sockets[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0);
    struct SocketPair { int* fds; ~SocketPair() { for (int i = 0; i < 2; ++i) if (fds[i] >= 0) close(fds[i]); } } pair{sockets};
    std::vector<std::string> environment;
    for (auto** p = environ; *p; ++p) environment.emplace_back(*p);
    environment.push_back("TRAVODDS_NATIVE_EXEC_CHILD=" + std::to_string(sockets[1]));
    environment.push_back("TRAVODDS_NATIVE_EXEC_PARENT=" + std::to_string(sockets[0]));
    std::vector<char*> env;
    for (auto& value : environment) env.push_back(value.data());
    env.push_back(nullptr);
    std::string filter = "--gtest_filter=LoanIntegration." + std::string(testing::UnitTest::GetInstance()->current_test_info()->name());
    char executable[] = "/proc/self/exe";
    char* args[] = {executable, filter.data(), nullptr};
    pid_t pid = -1;
    ASSERT_EQ(posix_spawn(&pid, executable, nullptr, nullptr, args, env.data()), 0);
    struct Child {
        pid_t pid;
        ~Child() {
            if (pid <= 0) return;
            int status = 0;
            auto reaped = waitpid(pid, &status, WNOHANG);
            if (reaped == pid) {
                ADD_FAILURE() << "Child exited before protocol completed: pid=" << pid
                              << " status=" << status
                              << " signal=" << (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
            } else if (reaped == 0) {
                kill(pid, SIGKILL);
                waitpid(pid, nullptr, 0);
            }
        }
    } child{pid};
    close(sockets[1]); sockets[1] = -1;
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto endpoint = rtps->GetWriter(Conversion::ToGuid(writer->get_instance_handle()).entityId);
    const auto& info = dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData().data_sharing;
    SharedPoolDescriptor d;
    d.writer = endpoint->GetGuid(); d.session = {info.session_high, info.session_low};
    d.path = SharedPayloadPool::pathFor(d.writer, d.session, qos.data_sharing.shared_memory_directory);
    d.config.payload_size = info.type.size; d.config.alignment = std::max(64u, info.type.alignment);
    d.config.slots = info.slots; d.config.max_readers = info.max_readers;
    d.config.max_segment_bytes = qos.data_sharing.max_segment_bytes; d.segment_bytes = info.segment_bytes;
    auto pool = SharedPayloadPool::open(d);
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    if (mixed && late) {
        ASSERT_TRUE(byte(sockets[0], 'N', false));
        while (endpoint->SelectDataSharingReaders(pool->registeredReaders()).size() != 1 &&
                std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(2ms);
        ASSERT_EQ(endpoint->SelectDataSharingReaders(pool->registeredReaders()).size(), 1u);
        EXPECT_EQ(type.serializeSuccesses.load(), 0); // initial fallback attempts were deliberately refused
        type.failSerialize = false;
        ASSERT_TRUE(byte(sockets[0], 'K', true));
    }
    ASSERT_TRUE(byte(sockets[0], 'M', false));
    InstanceHandleSeq matches;
    while (std::chrono::steady_clock::now() < deadline) {
        ASSERT_EQ(writer->get_matched_subscriptions(matches), RETCODE_OK);
        if (matches.size() == (mixed ? 2u : 1u) && endpoint->SelectDataSharingReaders(pool->registeredReaders()).size() == 1) break;
        std::this_thread::sleep_for(2ms);
    }
    ASSERT_EQ(matches.size(), mixed ? 2u : 1u);
    ASSERT_EQ(endpoint->SelectDataSharingReaders(pool->registeredReaders()).size(), 1u);
    if (!late || !replay) for (uint64_t index = 9182; index < 9185; ++index) {
        if (index == 9183) {
            NativeSample sample{}; sample.index = index; sample.values[6] = index + 79;
            ASSERT_EQ(writer->write_w_timestamp(&sample, HANDLE_NIL, {123, 456789}), RETCODE_OK);
        } else {
            SampleLoan loan;
            ASSERT_EQ(writer->loan_sample(loan), RETCODE_OK);
            auto* sample = static_cast<NativeSample*>(loan.data());
            initializeNativeSample(*sample, index, index + 79);
            ASSERT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_OK);
            EXPECT_FALSE(loan);
        }
        if (type.keyed) ASSERT_TRUE(byte(sockets[0], 'S', false));
    }
    if (late && !mixed) {
        auto history = endpoint->GetHistoryCache();
        for (auto seq = history->GetSeqNumMin(); seq <= history->GetSeqNumMax(); seq++) {
            auto cache = history->GetChange(seq);
            ASSERT_TRUE(cache);
            EXPECT_TRUE(cache->nativePayload);
            EXPECT_EQ(cache->serializedBuffer, nullptr);
        }
    }
    ASSERT_TRUE(byte(sockets[0], 'R', false));
    if (late && mixed) {
        // Failed fallback attempts before registration are not serialized CDR.
        EXPECT_EQ(type.serializeSuccesses.load(), 3);
        EXPECT_GE(type.serializeCalls.load(), 3);
    } else if (!late) EXPECT_EQ(type.serializeCalls, mixed ? 3 : 0);
    if (type.keyed) {
        NativeSample sample{}; sample.index = 9184;
        const auto handle = writer->lookup_instance(&sample);
        ASSERT_NE(handle, HANDLE_NIL);
        NativeSample snapshot{}; snapshot.values[6] = 8866;
        ASSERT_EQ(writer->get_key_value(&snapshot, handle), RETCODE_OK);
        EXPECT_EQ(snapshot.index, 9184u);
        EXPECT_EQ(snapshot.values[6], 8866u);
        ASSERT_EQ(writer->dispose(nullptr, handle), RETCODE_OK);
        ASSERT_TRUE(byte(sockets[0], 'P', false));
        sample.values[6] = 666;
        ASSERT_EQ(writer->write(&sample, handle), RETCODE_OK);
        ASSERT_TRUE(byte(sockets[0], 'Q', false));
        ASSERT_EQ(writer->unregister_instance(nullptr, handle), RETCODE_OK);
        ASSERT_TRUE(byte(sockets[0], 'U', false));
        EXPECT_EQ(writer->lookup_instance(&sample), HANDLE_NIL);
        EXPECT_EQ(writer->get_key_value(&snapshot, handle), RETCODE_BAD_PARAMETER);
        EXPECT_EQ(writer->write(&sample, handle), RETCODE_BAD_PARAMETER);
        sample.values[6] = 777;
        ASSERT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
        EXPECT_EQ(writer->lookup_instance(&sample), handle);
        ASSERT_TRUE(byte(sockets[0], 'V', false));
        EXPECT_EQ(type.serializeCalls, mixed ? 5 : 0);
    }
    if (reliable) EXPECT_EQ(writer->wait_for_acknowledgments({3, 0}), RETCODE_OK);
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    writer = nullptr;
    EXPECT_FALSE(std::filesystem::exists(d.path));
    // statistics() is a nonblocking control-plane snapshot. Stop and
    // join the child's native listener before requiring an OK result.
    ASSERT_TRUE(byte(sockets[0], 'D', true));
    ASSERT_TRUE(byte(sockets[0], 'C', false));
    SharedPoolStats closed;
    ASSERT_EQ(pool->statistics(closed), SharedPoolResult::OK);
    EXPECT_FALSE(closed.writer_alive);
    EXPECT_EQ(closed.pending_references, 0u);
    EXPECT_EQ(closed.loan_references, 1u); // only the first sample's explicit application Loan
    ASSERT_TRUE(byte(sockets[0], 'L', true));
    int status = 0; pid_t reaped = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        reaped = waitpid(pid, &status, WNOHANG);
        if (reaped != 0) break;
        std::this_thread::sleep_for(2ms);
    }
    ASSERT_EQ(reaped, pid); child.pid = -1;
    ASSERT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);
}

TEST_F(LoanIntegration, ReliableNativeLoanAcrossExecBypassesBothSerializers) { RunNativeExec(true, false); }
TEST_F(LoanIntegration, BestEffortNativeLoanAcrossExecBypassesBothSerializers) { RunNativeExec(false, false); }
TEST_F(LoanIntegration, ReliableMixedNativeAndOffReadersInSameParticipantAcrossExec) { RunNativeExec(true, true); }
TEST_F(LoanIntegration, BestEffortMixedNativeAndOffReadersInSameParticipantAcrossExec) { RunNativeExec(false, true); }

TEST_F(LoanIntegration, ReliableKeyedNativeLoanAcrossExecBypassesPayloadSerializersAndTracksInstanceLifecycle) { RunNativeExec(true, false); }
TEST_F(LoanIntegration, BestEffortKeyedNativeLoanAcrossExecBypassesPayloadSerializersAndTracksInstanceLifecycle) { RunNativeExec(false, false); }
TEST_F(LoanIntegration, ReliableKeyedMixedReadersAcrossExecPreserveControlAndReRegistration) { RunNativeExec(true, true); }
TEST_F(LoanIntegration, BestEffortKeyedMixedReadersAcrossExecPreserveControlAndReRegistration) { RunNativeExec(false, true); }

TEST_F(LoanIntegration, ReliableTransientLocalLateNativeReaderAcrossExec) { RunNativeExec(true, false, true); }
TEST_F(LoanIntegration, BestEffortTransientLocalLateNativeReaderAcrossExec) { RunNativeExec(false, false, true); }
TEST_F(LoanIntegration, ReliableTransientLocalLateMixedReadersAcrossExec) { RunNativeExec(true, true, true); }
TEST_F(LoanIntegration, BestEffortTransientLocalLateMixedReadersAcrossExec) { RunNativeExec(false, true, true); }

TEST_F(LoanIntegration, ReliableTransientWriterLateVolatileNativeReaderGetsOnlyNewDataAcrossExec) { RunNativeExec(true, false, true, false); }
TEST_F(LoanIntegration, BestEffortTransientWriterLateVolatileNativeReaderGetsOnlyNewDataAcrossExec) { RunNativeExec(false, false, true, false); }

TEST_F(LoanIntegration, BuiltinReaderWithoutApplicationTypeSkipsInstanceSnapshot) {
    DataReaderImpl builtin(topic, DATAREADER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE,
        dynamic_cast<SubscriberImpl*>(subscriber), nullptr);
    auto cache = std::make_shared<CacheChange>(); cache->kind = ALIVE;
    std::shared_ptr<SerializedBuffer> serialized;
    std::shared_ptr<void> native;
    EXPECT_EQ(builtin.CreateInstanceKey(cache, serialized, native), RETCODE_OK);
    EXPECT_FALSE(serialized);
    EXPECT_FALSE(native);
}

TEST_F(LoanIntegration, KeyedReaderKeepsIndependentFragmentedCdrSnapshotAndFreshQueryCursors) {
    DataReaderQos qos; qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
    auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);
    auto* impl = dynamic_cast<DataReaderImpl*>(reader);
    NativeSample sample{}; sample.index = 481; sample.values[6] = 912;
    char bytes[68]{}; bytes[1] = DEFAULT_ENDIAN;
    SerializedBuffer payload; payload.buffer = bytes + 4; payload.buffer_size = 64;
    CdrSerializer cdr(&payload);
    ASSERT_EQ(type.serialize_data(&sample, &cdr, DEFAULT_ENDIAN), 0);
    auto cache = std::make_shared<CacheChange>(); cache->kind = ALIVE;
    const auto pool = MemoryPool::GetInstance();
    auto part = [&](const char* data, uint32_t size) {
        PayloadBufferPtr owner(new SerializedBuffer, PayloadBufferDeleter{pool});
        if (!pool->getBuffer(size, owner->origin_buffer)) throw std::bad_alloc();
        owner->buffer = owner->origin_buffer;
        owner->buffer_size = owner->writer_position = size;
        std::memcpy(owner->buffer, data, size);
        return owner.release();
    };
    cache->serializedBuffer = part(bytes, 36);
    cache->serializedBuffer->next = part(bytes + 36, 32);
    std::shared_ptr<SerializedBuffer> serialized;
    std::shared_ptr<void> native;
    ASSERT_EQ(impl->CreateInstanceKey(cache, serialized, native), RETCODE_OK);
    ASSERT_TRUE(serialized && serialized->next);
    EXPECT_NE(serialized->origin_buffer, cache->serializedBuffer->origin_buffer);
    InstanceHandle_t handle;
    ASSERT_EQ(type.MakeKey(&sample, handle, true), 0);
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto endpoint = rtps->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
    ASSERT_EQ(endpoint->GetReaderCache()->RegisterKey(handle, serialized, native), RETCODE_OK);
    cache.reset(); serialized.reset();
    for (int query = 0; query < 2; ++query) {
        NativeSample copy{};
        ASSERT_EQ(reader->get_key_value(&copy, handle), RETCODE_OK);
        EXPECT_EQ(copy.index, 481u);
        EXPECT_EQ(copy.values[6], 912u);
    }
    type.failDeserialize = true;
    EXPECT_EQ(reader->get_key_value(&sample, handle), RETCODE_ERROR);
}

TEST_F(LoanIntegration, KeyedNativeReaderRejectsMissingKeyHooksWithoutRetainingSource) {
    type.compactKeys = false;
    auto* reader = makeReader();
    ASSERT_NE(reader, nullptr);
    auto* impl = dynamic_cast<DataReaderImpl*>(reader);
    auto source = std::make_shared<NativeSample>(); source->index = 771;
    auto cache = std::make_shared<CacheChange>(); cache->kind = ALIVE;
    cache->nativePayload = source; cache->nativeType = type.get_native_type_info();
    std::shared_ptr<SerializedBuffer> serialized;
    std::shared_ptr<void> native;
    ++cache->nativeType.size;
    EXPECT_EQ(impl->CreateInstanceKey(cache, serialized, native), RETCODE_BAD_PARAMETER);
    cache->nativeType = type.get_native_type_info();
    EXPECT_EQ(impl->CreateInstanceKey(cache, serialized, native), RETCODE_BAD_PARAMETER);
    EXPECT_FALSE(native);
    EXPECT_FALSE(serialized);
    // Even a manually injected native cache cannot select a full-sample
    // snapshot. A valid hook captures only key bytes without constructing T.
    type.compactKeys = true;
    type.failConstruct = true;
    ASSERT_EQ(impl->CreateInstanceKey(cache, serialized, native), RETCODE_OK);
    EXPECT_NE(native.get(), source.get());
    EXPECT_EQ(reinterpret_cast<uintptr_t>(native.get()) % alignof(uint64_t), 0u);
    source->index = 992;
    uint64_t key = 0;
    std::memcpy(&key, native.get(), sizeof(key));
    EXPECT_EQ(key, 771u);
    EXPECT_EQ(type.serializeCalls, 0);
    EXPECT_EQ(type.deserializeCalls, 0);
}

TEST_F(LoanIntegration, KeyedLegacyRegistrationRollsBackSerializerFailureAndReturnsIndependentKeyCopies) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos qos; qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    NativeSample sample{}; sample.index = 75; sample.values[1] = 1234;
    type.failSerialize = true;
    EXPECT_EQ(writer->register_instance(&sample), HANDLE_NIL);
    EXPECT_EQ(writer->lookup_instance(&sample), HANDLE_NIL);
    type.failSerialize = false;
    const auto handle = writer->register_instance(&sample);
    ASSERT_NE(handle, HANDLE_NIL);
    EXPECT_EQ(writer->lookup_instance(&sample), handle);
    for (int i = 0; i < 2; ++i) {
        NativeSample snapshot{};
        ASSERT_EQ(writer->get_key_value(&snapshot, handle), RETCODE_OK);
        EXPECT_EQ(snapshot.index, 75u);
        EXPECT_EQ(snapshot.values[1], 1234u);
    }
    type.failKey = true;
    EXPECT_EQ(writer->lookup_instance(&sample), HANDLE_NIL);
    EXPECT_EQ(writer->dispose(&sample, handle), RETCODE_BAD_PARAMETER);
    type.failKey = false;
    EXPECT_EQ(writer->unregister_instance(nullptr, handle), RETCODE_OK);
    EXPECT_EQ(writer->lookup_instance(&sample), HANDLE_NIL);
}

TEST_F(LoanIntegration, KeyedRegistrationValidatesInputsCapacityAndSnapshotWithoutPayloadCdr) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.resource_limits.max_instances = 1;
    qos.resource_limits.max_samples = 8;
    qos.resource_limits.max_samples_per_instance = 8;
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    type.failSerialize = type.failDeserialize = true;
    NativeSample sample{}; sample.index = 123; sample.values[0] = 456;
    EXPECT_EQ(writer->register_instance(nullptr), HANDLE_NIL);
    EXPECT_EQ(writer->register_instance_w_timestamp(&sample, {-1, 0}), HANDLE_NIL);
    EXPECT_EQ(writer->lookup_instance(&sample), HANDLE_NIL);
    type.failKey = true;
    EXPECT_EQ(writer->register_instance(&sample), HANDLE_NIL);
    EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_BAD_PARAMETER);
    type.failKey = false;
    const auto first = writer->register_instance(&sample);
    ASSERT_NE(first, HANDLE_NIL);
    EXPECT_EQ(writer->register_instance(&sample), first);
    sample.values[0] = 999;
    NativeSample snapshot{}; snapshot.values[0] = 7788;
    EXPECT_EQ(writer->get_key_value(nullptr, first), RETCODE_BAD_PARAMETER);
    EXPECT_EQ(writer->get_key_value(&snapshot, first), RETCODE_OK);
    EXPECT_EQ(snapshot.index, 123u);
    EXPECT_EQ(snapshot.values[0], 7788u);
    ++sample.index;
    EXPECT_EQ(writer->write(&sample, first), RETCODE_PRECONDITION_NOT_MET);
    EXPECT_EQ(writer->lookup_instance(&sample), HANDLE_NIL);
    EXPECT_EQ(writer->register_instance(&sample), HANDLE_NIL);
    EXPECT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OUT_OF_RESOURCES);
    EXPECT_EQ(writer->unregister_instance(nullptr, HANDLE_NIL), RETCODE_BAD_PARAMETER);
    EXPECT_EQ(writer->unregister_instance(&sample, first), RETCODE_PRECONDITION_NOT_MET);
    EXPECT_EQ(writer->unregister_instance(nullptr, first), RETCODE_OK);
    EXPECT_EQ(writer->get_key_value(&snapshot, first), RETCODE_BAD_PARAMETER);
    EXPECT_NE(writer->register_instance(&sample), HANDLE_NIL);
    EXPECT_EQ(type.serializeCalls, 0);
    EXPECT_EQ(type.deserializeCalls, 0);
    EXPECT_EQ(type.copyCalls, 0);
}

TEST_F(LoanIntegration, KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    qos.resource_limits.max_instances = 1;
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    SampleLoan loan;
    ASSERT_EQ(writer->loan_sample(loan), RETCODE_OK);
    auto* source = static_cast<NativeSample*>(loan.data());
    initializeNativeSample(*source, 371, 9876);
    type.failSerialize = type.failDeserialize = true;
    // Compact snapshots never construct/copy a complete NativeSample.
    type.failConstruct = true;
    for (int invalid = 1; invalid <= 4; ++invalid) {
        type.malformedKeyInfo = invalid;
        EXPECT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_BAD_PARAMETER);
        ASSERT_TRUE(loan);
        EXPECT_EQ(loan.data(), source);
        EXPECT_EQ(writer->lookup_instance(source), HANDLE_NIL);
    }
    type.malformedKeyInfo = 0;
    EXPECT_EQ(type.keyCaptureCalls, 0);
    type.failCapture = true;
    EXPECT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_ERROR);
    ASSERT_TRUE(loan);
    EXPECT_EQ(writer->lookup_instance(source), HANDLE_NIL);
    type.failCapture = false; type.throwCapture = true;
    EXPECT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_ERROR);
    ASSERT_TRUE(loan);
    EXPECT_EQ(source->values[6], 9876u);
    EXPECT_EQ(writer->lookup_instance(source), HANDLE_NIL);
    type.throwCapture = false;
    ASSERT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_OK);
    EXPECT_FALSE(loan);
    NativeSample query{}; query.index = 371; query.values[6] = 2244;
    const auto handle = writer->lookup_instance(&query);
    ASSERT_NE(handle, HANDLE_NIL);
    type.failRestore = true;
    EXPECT_EQ(writer->get_key_value(&query, handle), RETCODE_ERROR);
    type.failRestore = false; type.throwRestore = true;
    EXPECT_EQ(writer->get_key_value(&query, handle), RETCODE_ERROR);
    type.throwRestore = false; query.index = 0;
    EXPECT_EQ(writer->get_key_value(&query, handle), RETCODE_OK);
    EXPECT_EQ(query.index, 371u);
    EXPECT_EQ(query.values[6], 2244u);
    EXPECT_EQ(type.get_native_key_info().size, sizeof(uint64_t));
    EXPECT_EQ(type.keyCaptureCalls, 3);
    EXPECT_EQ(type.keyRestoreCalls, 3);
    EXPECT_EQ(type.copyCalls, 0);
    EXPECT_EQ(type.serializeCalls, 0);
    EXPECT_EQ(type.deserializeCalls, 0);
}

TEST_F(LoanIntegration, KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource) {
    DataReaderQos qos;
    qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
    qos.resource_limits.max_instances = 1;
    auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto endpoint = rtps->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
    const auto until = std::chrono::steady_clock::now() + 4s;
    while (!endpoint->MatchedWriterIsMatched(Conversion::ToGuid(writer->get_instance_handle())) &&
        std::chrono::steady_clock::now() < until) std::this_thread::sleep_for(2ms);
    ASSERT_TRUE(endpoint->MatchedWriterIsMatched(Conversion::ToGuid(writer->get_instance_handle())));
    type.failSerialize = type.failDeserialize = true;
    for (uint64_t attempt = 1; attempt <= 7; ++attempt) {
        auto source = std::make_shared<NativeSample>(); source->index = attempt;
        std::weak_ptr<NativeSample> sourceLifetime = source;
        InstanceHandle_t handle;
        ASSERT_EQ(type.MakeKey(source.get(), handle, true), 0);
        auto cache = endpoint->new_change(ALIVE, nullptr, handle, writer->get_instance_handle(), TIME_ZERO);
        cache->sequenceNumber = {0, uint32_t(attempt)};
        cache->nativePayload = source; cache->nativeType = type.get_native_type_info();
        type.malformedKeyInfo = attempt <= 4 ? int(attempt) : 0;
        type.failCapture = attempt == 5; type.throwCapture = attempt == 6;
        const auto result = endpoint->AddChanges(cache);
        if (attempt < 7) {
            EXPECT_EQ(result, attempt <= 4 ? RETCODE_BAD_PARAMETER : RETCODE_ERROR);
            EXPECT_FALSE(endpoint->GetReaderCache()->IsKeyRegistered(handle));
            EXPECT_FALSE(endpoint->GetReaderCache()->IsInstancePresent(handle));
            NativeSample key{};
            EXPECT_EQ(reader->get_key_value(&key, handle), RETCODE_BAD_PARAMETER);
        } else {
            ASSERT_EQ(result, RETCODE_OK);
            SequenceData<NativeSample> values; SampleInfoSeq infos;
            ASSERT_EQ(reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
            ASSERT_TRUE(values.has_loan());
            EXPECT_EQ(values[0].index, 7u);
            ASSERT_EQ(reader->return_loan(values, infos), RETCODE_OK);
            NativeSample key{}; key.values[0] = 6611;
            ASSERT_EQ(reader->get_key_value(&key, handle), RETCODE_OK);
            EXPECT_EQ(key.index, 7u); EXPECT_EQ(key.values[0], 6611u);
        }
        cache.reset(); source.reset();
        EXPECT_TRUE(sourceLifetime.expired());
    }
    EXPECT_EQ(type.keyCaptureCalls, 3);
    EXPECT_EQ(type.keyRestoreCalls, 1);
    EXPECT_EQ(type.copyCalls, 0);
    EXPECT_EQ(type.serializeCalls, 0);
    EXPECT_EQ(type.deserializeCalls, 0);
}

#endif

struct Tracked {
    static int alive;
    static bool fail;
    int value = 0;
    Tracked() { ++alive; }
    ~Tracked() { --alive; }
    Tracked& operator=(const Tracked& other) {
        if (fail) throw std::runtime_error("copy failure");
        value = other.value;
        return *this;
    }
};
int Tracked::alive = 0;
bool Tracked::fail = false;
}

namespace {
template<class Base> class IngressProbe : public Base {
public:
    using Base::Base;
    void blockIngress(std::promise<void>& entered, std::future<void>& release) {
        std::lock_guard<std::recursive_mutex> lock(this->dataMutex_);
        entered.set_value();
        release.wait();
    }
};
template<class Base> void checkLivelinessQuery() {
    GUID_t readerGuid, writerGuid;
    readerGuid.prefix.value[0] = writerGuid.prefix.value[0] = 0x5d;
    readerGuid.entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
    writerGuid.entityId.entityKind = ENTITYKIND_USER_WRITER_NO_KEY;
    IngressProbe<Base> reader(DATAREADER_QOS_DEFAULT, readerGuid, TopicKind_t{NO_KEY});
    auto proxy = std::make_shared<WriterProxy>(writerGuid, GUID_UNKNOWN, LocatorSeq{}, LocatorSeq{});
    reader.MatchedWriterAdd(proxy);
    std::promise<void> entered, release;
    auto started = entered.get_future(), unblock = release.get_future();
    std::thread ingress([&] { reader.blockIngress(entered, unblock); });
    started.wait();
    // Remote liveliness callbacks hold the assertion lock and query matching.
    // Ingress may be waiting for that same assertion lock, so these lookups
    // must finish independently of ingress.
    auto query = std::async(std::launch::async, [&] {
        return reader.MatchedWriterIsMatched(writerGuid) && reader.MatchedWriterLookup(writerGuid) == proxy;
    });
    const bool responsive = query.wait_for(500ms) == std::future_status::ready;
    release.set_value();
    ingress.join();
    EXPECT_TRUE(responsive);
    EXPECT_TRUE(query.get());
}
template<class Base> void checkBuiltinListener(const EntityId_t& entity) {
    GUID_t guid; guid.entityId = entity;
    auto reader = std::make_shared<IngressProbe<Base>>(DATAREADER_QOS_DEFAULT, guid, TopicKind_t{NO_KEY});
    std::promise<void> entered, release;
    auto started = entered.get_future(), unblock = release.get_future();
    std::thread ingress([&] { reader->blockIngress(entered, unblock); });
    started.wait();
    // Discovery can match endpoints while a builtin Reader processes SEDP.
    // No native listener belongs to that builtin endpoint, and attempting to
    // acquire its ingress here inverts the discovery/table lock order.
    auto query = std::async(std::launch::async, [&] {
        return reader->StartDataSharing(reader, DiscoveredWriterData{});
    });
    const bool responsive = query.wait_for(500ms) == std::future_status::ready;
    release.set_value();
    ingress.join();
    EXPECT_TRUE(responsive);
    EXPECT_FALSE(query.get());
}
}
TEST(ReaderDataSharing, ReliableLivelinessQueriesDoNotWaitForBusyIngress) { checkLivelinessQuery<StatefulReader>(); }
TEST(ReaderDataSharing, BestEffortLivelinessQueriesDoNotWaitForBusyIngress) { checkLivelinessQuery<StatelessReader>(); }
TEST(ReaderDataSharing, BuiltinReliableDiscoverySkipsListenerWithoutWaitingForIngress) {
    checkBuiltinListener<StatefulReader>(ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER);
}
TEST(ReaderDataSharing, BuiltinBestEffortDiscoverySkipsListenerWithoutWaitingForIngress) {
    checkBuiltinListener<StatelessReader>(ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER);
}

TEST(LoanSequence, BorrowMoveCopyAndDestructionKeepSamplePinnedWithoutDeletingItsAddress) {
    auto sample = std::make_shared<Tracked>();
    sample->value = 47;
    auto owner = std::make_shared<int>(1);
    std::weak_ptr<Tracked> weak = sample;
    SequenceData<Tracked> borrowed;
    ASSERT_TRUE(borrowed.borrow({sample.get()}, sample, owner, 1));
    EXPECT_EQ(&borrowed[0], sample.get());
    EXPECT_FALSE(borrowed.has_ownership());
    EXPECT_TRUE(borrowed.has_loan());
    EXPECT_EQ(borrowed.maximum(), 0u);
    EXPECT_FALSE(borrowed.length(2));
    EXPECT_THROW(borrowed.push_back(*sample), std::logic_error);
    SequenceData<Tracked> copied(borrowed);
    EXPECT_TRUE(copied.has_ownership());
    EXPECT_EQ(copied[0].value, 47);
    EXPECT_NE(&copied[0], &borrowed[0]);
    SequenceData<Tracked> moved(std::move(borrowed));
    EXPECT_FALSE(borrowed.has_loan());
    EXPECT_EQ(borrowed.length(), 0u);
    sample.reset();
    EXPECT_FALSE(weak.expired());
    moved.unloan();
    EXPECT_TRUE(weak.expired());
    EXPECT_EQ(copied[0].value, 47);
    copied.unloan();
    EXPECT_EQ(Tracked::alive, 0);
}

TEST(LoanSequence, AssignmentShrinksSafelyAndCopyFailurePreservesDestination) {
    {
        SequenceData<Tracked> source(1), target(5);
        source[0].value = 13;
        target[0].value = 29;
        Tracked::fail = true;
        EXPECT_THROW(target = source, std::runtime_error);
        Tracked::fail = false;
        ASSERT_EQ(target.length(), 5u);
        EXPECT_EQ(target[0].value, 29);
        EXPECT_EQ(Tracked::alive, 6);
        target = source;
        ASSERT_EQ(target.length(), 1u);
        EXPECT_EQ(target[0].value, 13);
        EXPECT_EQ(Tracked::alive, 2);
        target = target;
        target = std::move(target);
        EXPECT_EQ(target[0].value, 13);
    }
    EXPECT_EQ(Tracked::alive, 0);
}

TEST(LoanSequence, RejectsInvalidBorrowAndSeparatesPinOwnerAndPairRoles) {
    SequenceData<int> a, b;
    auto pin = std::make_shared<int>(5);
    auto owner = std::make_shared<int>(0);
    EXPECT_FALSE(a.borrow({}, pin, owner));
    EXPECT_FALSE(a.borrow({nullptr}, pin, owner));
    EXPECT_FALSE(a.borrow({pin.get()}, {}, owner));
    EXPECT_FALSE(a.borrow({pin.get()}, pin, {}));
    ASSERT_TRUE(a.borrow({pin.get()}, pin, owner, 1));
    ASSERT_TRUE(b.borrow({pin.get()}, pin, owner, 2));
    EXPECT_TRUE(a.matches_loan(b, owner, 1, 2));
    EXPECT_FALSE(a.matches_loan(a, owner, 1, 2));
    EXPECT_FALSE(a.matches_loan(b, std::make_shared<int>(0), 1, 2));
    EXPECT_FALSE(a.borrow({pin.get()}, pin, owner));
    SequenceBase& base = a;
    EXPECT_THROW(base[1], std::out_of_range);
}

TEST(LoanNotifications, ConsumedTriggerDoesNotRemainActiveBecauseOfAnOldWakeup) {
    GuardCondition condition;
    WaitSet wait;
    ASSERT_EQ(wait.attach_condition(condition), RETCODE_OK);
    ASSERT_EQ(condition.set_trigger_value(true), RETCODE_OK);
    ASSERT_EQ(condition.set_trigger_value(false), RETCODE_OK);
    ConditionSeq active;
    EXPECT_EQ(wait.wait(active, {0, 1000000}), RETCODE_TIMEOUT);
    EXPECT_TRUE(active.empty());
    ASSERT_EQ(condition.set_trigger_value(true), RETCODE_OK);
    EXPECT_EQ(wait.wait(active, {0, 1000000}), RETCODE_OK);
    ASSERT_EQ(active.size(), 1u);
    EXPECT_EQ(active[0], &condition);
    ASSERT_EQ(condition.set_trigger_value(false), RETCODE_OK);
    EXPECT_EQ(wait.wait(active, {0, 1000000}), RETCODE_TIMEOUT);
}

TEST_F(LoanIntegration, LegacyTypeCapabilityIsConservative) {
    type.capable = false;
    SampleLoan loan;
    EXPECT_EQ(writer->loan_sample(loan), RETCODE_UNSUPPORTED);
    EXPECT_FALSE(loan);
    EXPECT_EQ(loan.data(), nullptr);
    EXPECT_FALSE(NativeTypeInfo{}.loanable());
}

#if defined(__linux__)
TEST_F(LoanIntegration, WriterLoanCapacityMoveWrongWriterAndAutomaticCancellation) {
    auto* other = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(other, nullptr);
    std::vector<SampleLoan> loans(64);
    std::set<void*> pointers;
    for (auto& loan : loans) {
        ASSERT_EQ(writer->loan_sample(loan), RETCODE_OK);
        EXPECT_TRUE(pointers.insert(loan.data()).second);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(loan.data()) % alignof(NativeSample), 0u);
    }
    SampleLoan extra;
    EXPECT_EQ(writer->loan_sample(extra), RETCODE_OUT_OF_RESOURCES);
    EXPECT_EQ(writer->loan_sample(loans[0]), RETCODE_PRECONDITION_NOT_MET);
    EXPECT_EQ(other->discard_loan(loans[0]), RETCODE_PRECONDITION_NOT_MET);
    EXPECT_EQ(other->write(loans[0], HANDLE_NIL), RETCODE_PRECONDITION_NOT_MET);
    SampleLoan moved(std::move(loans[0]));
    EXPECT_FALSE(loans[0]);
    EXPECT_EQ(writer->discard_loan(loans[0]), RETCODE_PRECONDITION_NOT_MET);
    EXPECT_EQ(writer->discard_loan(moved), RETCODE_OK);
    ASSERT_EQ(writer->loan_sample(extra), RETCODE_OK);
    extra.reset();
    ASSERT_EQ(writer->loan_sample(extra), RETCODE_OK);
}

TEST_F(LoanIntegration, FailedWriteKeepsLoanAndSuccessfulRtpsFallbackConsumesIt) {
    SampleLoan loan;
    ASSERT_EQ(writer->loan_sample(loan), RETCODE_OK);
    auto* sample = static_cast<NativeSample*>(loan.data());
    initializeNativeSample(*sample, 37);
    type.failSerialize = true;
    EXPECT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_ERROR);
    ASSERT_TRUE(loan);
    EXPECT_EQ(loan.data(), sample);
    EXPECT_EQ(sample->index, 37u);
    type.failSerialize = false;
    EXPECT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_OK);
    EXPECT_EQ(type.serializeCalls, 2);
    EXPECT_FALSE(loan);
    EXPECT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_PRECONDITION_NOT_MET);
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto endpoint = rtps->GetWriter(Conversion::ToGuid(writer->get_instance_handle()).entityId);
    auto history = endpoint->GetWriterCache();
    auto change = history->GetChange(history->GetSeqNumMax());
    ASSERT_TRUE(change && change->serializedBuffer);
    SerializedBuffer bytes = *change->serializedBuffer;
    bytes.buffer += CDR_ENDIANNESS_LEN;
    bytes.buffer_size -= CDR_ENDIANNESS_LEN;
    CdrDeserializer deserializer(&bytes);
    NativeSample decoded{};
    ASSERT_EQ(type.deserialize_data(&decoded, &deserializer, DEFAULT_ENDIAN), 0);
    EXPECT_EQ(decoded.index, 37u);
}

TEST_F(LoanIntegration, ConstructorFailureReturnsEveryReservation) {
    for (int i = 0; i < 140; ++i) {
        type.throwConstruct = i % 2;
        type.failConstruct = !type.throwConstruct;
        SampleLoan loan;
        ASSERT_EQ(writer->loan_sample(loan), RETCODE_ERROR);
        EXPECT_FALSE(loan);
    }
    type.throwConstruct = type.failConstruct = false;
    SampleLoan loan;
    EXPECT_EQ(writer->loan_sample(loan), RETCODE_OK);
}

TEST_F(LoanIntegration, WriterDeletionKeepsOutstandingSampleMappedUntilLoanDestruction) {
    SampleLoan loan;
    ASSERT_EQ(writer->loan_sample(loan), RETCODE_OK);
    auto* sample = static_cast<NativeSample*>(loan.data());
    sample->index = 91;
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    writer = nullptr;
    ASSERT_TRUE(loan);
    EXPECT_EQ(static_cast<NativeSample*>(loan.data())->index, 91u);
    loan.reset();
}

TEST_F(LoanIntegration, ConcurrentWriterReservationsNeverAliasLiveSamples) {
    std::mutex mutex;
    std::set<void*> live;
    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 6; ++i) threads.emplace_back([&] {
        for (int n = 0; n < 80; ++n) {
            SampleLoan loan;
            if (writer->loan_sample(loan) != RETCODE_OK) { ++failures; return; }
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (!live.insert(loan.data()).second) ++failures;
            }
            std::this_thread::yield();
            {
                std::lock_guard<std::mutex> lock(mutex);
                live.erase(loan.data());
            }
            if (writer->discard_loan(loan) != RETCODE_OK) ++failures;
        }
    });
    for (auto& thread : threads) thread.join();
    EXPECT_EQ(failures, 0);
    EXPECT_TRUE(live.empty());
}

namespace {
struct NativeDelivery {
    std::shared_ptr<SharedPayloadPool> pool;
    std::weak_ptr<CacheChange> change;
    void* address = nullptr;
};
class NativeReaderIntegration : public LoanIntegration {
protected:
    NativeDelivery deliver(DataReader* reader, bool throughEndpoint = false, DataWriter* sourceWriter = nullptr) {
        if (!sourceWriter) sourceWriter = writer;
        NativeDelivery result;
        SharedPoolConfig config;
        config.payload_size = sizeof(NativeSample);
        config.slots = 1;
        result.pool = SharedPayloadPool::create(Conversion::ToGuid(sourceWriter->get_instance_handle()), config);
        auto mapping = SharedPayloadPool::open(result.pool->descriptor());
        auto registration = mapping->registerReader();
        if (!registration) throw std::runtime_error("Reader registration failed");
        SharedSampleToken token;
        char* buffer = nullptr;
        if (result.pool->loan(sizeof(NativeSample), token, buffer) != SharedPoolResult::OK)
            throw std::runtime_error("Native loan failed");
        type.construct_native_sample(buffer);
        auto* sample = reinterpret_cast<NativeSample*>(buffer);
        sample->index = 73;
        for (uint64_t i = 0; i < 7; ++i) sample->values[i] = 7300 + i;
        if (result.pool->publish(token, sizeof(NativeSample), 1) != SharedPoolResult::OK ||
            !result.pool->releaseWriter(token)) throw std::runtime_error("Native publish failed");
        SharedReadLoan native;
        if (registration->waitNext(0, 0ms, native) != SharedPoolResult::OK)
            throw std::runtime_error("Native receive failed");
        auto pin = std::make_shared<SharedReadLoan>(std::move(native));
        result.address = const_cast<char*>(pin->data());
        auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
        auto endpoint = rtps->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
        const auto matchedUntil = std::chrono::steady_clock::now() + 4s;
        while (!endpoint->MatchedWriterIsMatched(Conversion::ToGuid(sourceWriter->get_instance_handle())) &&
               std::chrono::steady_clock::now() < matchedUntil) std::this_thread::sleep_for(2ms);
        auto cache = endpoint->new_change(ALIVE, nullptr, handle(), sourceWriter->get_instance_handle(), TIME_ZERO);
        cache->sequenceNumber = {0, 1};
        cache->nativePayload = std::shared_ptr<void>(pin, result.address);
        cache->nativeType = type.get_native_type_info();
        const auto resultCode = throughEndpoint ? endpoint->SubmitDataChange(cache) : endpoint->AddChanges(cache);
        if (resultCode != RETCODE_OK) throw std::runtime_error("History insertion failed");
        result.change = cache;
        return result;
    }
    InstanceHandle_t handle() {
        auto value = HANDLE_NIL;
        value.keyHash.value[0] = 3;
        return value;
    }
    uint32_t freeSlots(const NativeDelivery& delivery) {
        SharedPoolStats stats;
        EXPECT_EQ(delivery.pool->statistics(stats), SharedPoolResult::OK);
        return stats.free_slots;
    }
};
}

TEST_F(NativeReaderIntegration, KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion) {
    auto* reader = makeReader();
    ASSERT_NE(reader, nullptr);
    type.failSerialize = type.failDeserialize = true;
    auto delivery = deliver(reader);
    EXPECT_EQ(type.keyCaptureCalls, 1);
    EXPECT_EQ(type.get_native_key_info().size, sizeof(uint64_t));
    SequenceData<NativeSample> values; SampleInfoSeq infos;
    ASSERT_EQ(reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    ASSERT_TRUE(values.has_loan());
    EXPECT_EQ(values[0].index, 73u);
    EXPECT_EQ(freeSlots(delivery), 0u);
    ASSERT_EQ(reader->return_loan(values, infos), RETCODE_OK);
    EXPECT_TRUE(delivery.change.expired());
    EXPECT_EQ(freeSlots(delivery), 1u);
    // The only slot can be overwritten while the known instance survives.
    SharedSampleToken replacement;
    char* buffer = nullptr;
    ASSERT_EQ(delivery.pool->loan(sizeof(NativeSample), replacement, buffer), SharedPoolResult::OK);
    ASSERT_EQ(replacement.slot, 0u);
    ASSERT_TRUE(type.construct_native_sample(buffer));
    auto* overwritten = reinterpret_cast<NativeSample*>(buffer);
    overwritten->index = 992;
    for (auto& value : overwritten->values) value = 8844;
    NativeSample key{}; key.values[6] = 5522;
    for (int query = 0; query < 2; ++query) {
        ASSERT_EQ(reader->get_key_value(&key, handle()), RETCODE_OK);
        EXPECT_EQ(key.index, 73u);
        EXPECT_EQ(key.values[6], 5522u);
    }
    ASSERT_TRUE(delivery.pool->releaseWriter(replacement));
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    writer = nullptr;
    key.index = 0;
    ASSERT_EQ(reader->get_key_value(&key, handle()), RETCODE_OK);
    EXPECT_EQ(key.index, 73u);
    EXPECT_EQ(key.values[6], 5522u);
    EXPECT_EQ(freeSlots(delivery), 1u);
    EXPECT_EQ(type.keyCaptureCalls, 1);
    EXPECT_EQ(type.keyRestoreCalls, 3);
    EXPECT_EQ(type.copyCalls, 0);
    EXPECT_EQ(type.serializeCalls, 0);
    EXPECT_EQ(type.deserializeCalls, 0);
}

namespace {
class MultiWriterDeadlineIntegration : public NativeReaderIntegration {
protected:
    void checkDeparture(bool writerLost) {
        for (auto reliability : {BEST_EFFORT_RELIABILITY_QOS, RELIABLE_RELIABILITY_QOS}) {
            SCOPED_TRACE(reliability);
            ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
            DataWriterQos wq; wq.deadline.period = {3600, 0};
            wq.writer_data_lifecycle.autodispose_unregistered_instances = false;
            writer = publisher->create_datawriter(topic, wq, nullptr, STATUS_MASK_NONE);
            auto* other = publisher->create_datawriter(topic, wq, nullptr, STATUS_MASK_NONE);
            ASSERT_TRUE(writer && other);
            DataReaderQos rq; rq.deadline.period = wq.deadline.period;
            rq.reliability.kind = reliability;
            auto* reader = subscriber->create_datareader(topic, rq, nullptr, STATUS_MASK_NONE);
            ASSERT_NE(reader, nullptr);
            auto* impl = dynamic_cast<DataReaderImpl*>(reader);
            auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
            auto endpoint = rtps->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
            const auto firstGuid = Conversion::ToGuid(writer->get_instance_handle());
            const auto secondGuid = Conversion::ToGuid(other->get_instance_handle());
            const auto limit = std::chrono::steady_clock::now() + 4s;
            while ((!endpoint->MatchedWriterIsMatched(firstGuid) || !endpoint->MatchedWriterIsMatched(secondGuid)) &&
                    std::chrono::steady_clock::now() < limit) std::this_thread::sleep_for(2ms);
            ASSERT_TRUE(endpoint->MatchedWriterIsMatched(firstGuid));
            ASSERT_TRUE(endpoint->MatchedWriterIsMatched(secondGuid));
            auto first = deliver(reader, true);
            auto second = deliver(reader, true, other);
            auto history = endpoint->GetReaderCache();
            InstanceHandle_t tracked;
            std::chrono::steady_clock::time_point due;
            ASSERT_TRUE(history->GetNextDeadline(tracked, due));
            const auto previousDue = due;
            auto depart = [&](DataWriter* departing) {
                if (writerLost) {
                    history->RemoveRemoteWriter(Conversion::ToGuid(departing->get_instance_handle()));
                } else {
                    auto cache = endpoint->new_change(NOT_ALIVE_UNREGISTERED, nullptr, handle(),
                        departing->get_instance_handle(), TIME_ZERO);
                    cache->sequenceNumber = {0, 2};
                    EXPECT_EQ(endpoint->SubmitDataChange(cache), RETCODE_OK);
                }
            };
            depart(writer);
            // unregister is not an ALIVE sample: preserve the other Writer's
            // existing deadline, rather than refreshing it or disabling it.
            ASSERT_TRUE(history->GetNextDeadline(tracked, due));
            EXPECT_EQ(tracked, handle());
            EXPECT_EQ(due, previousDue);
            ASSERT_TRUE(history->SetNextDeadline(handle(), std::chrono::steady_clock::now() - 1s));
            impl->DeadlineTimeout();
            RequestedDeadlineMissedStatus status;
            ASSERT_EQ(reader->get_requested_deadline_missed_status(status), RETCODE_OK);
            EXPECT_EQ(status.total_count, 1);
            EXPECT_EQ(status.last_instance_handle, handle());
            depart(other);
            EXPECT_FALSE(history->GetNextDeadline(tracked, due));
            impl->DeadlineTimeout();
            ASSERT_EQ(reader->get_requested_deadline_missed_status(status), RETCODE_OK);
            EXPECT_EQ(status.total_count, 1);
            ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
            ASSERT_EQ(publisher->delete_datawriter(other), RETCODE_OK);
        }
    }
};
}

TEST_F(MultiWriterDeadlineIntegration, KeyedUnregisterPreservesOtherWriterDeadline) {
    checkDeparture(false);
}
TEST_F(MultiWriterDeadlineIntegration, KeyedWriterLossSuspendsOnlyTheLastWriterDeadline) {
    checkDeparture(true);
}

TEST_F(NativeReaderIntegration, DeadlineNativeAdmissionAndDisposeAreTrackedForBothReliabilities) {
    for (auto reliability : {BEST_EFFORT_RELIABILITY_QOS, RELIABLE_RELIABILITY_QOS}) {
        SCOPED_TRACE(reliability);
        ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
        DataWriterQos wq; wq.deadline.period = {3600, 0};
        writer = publisher->create_datawriter(topic, wq, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(writer, nullptr);
        DataReaderQos rq; rq.deadline.period = wq.deadline.period;
        rq.reliability.kind = reliability;
        auto* reader = subscriber->create_datareader(topic, rq, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(reader, nullptr);
        auto* impl = dynamic_cast<DataReaderImpl*>(reader);
        auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
        auto endpoint = rtps->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
        const auto writerGuid = Conversion::ToGuid(writer->get_instance_handle());
        const auto matchedDeadline = std::chrono::steady_clock::now() + 4s;
        while (!endpoint->MatchedWriterIsMatched(writerGuid) && std::chrono::steady_clock::now() < matchedDeadline)
            std::this_thread::sleep_for(2ms);
        ASSERT_TRUE(endpoint->MatchedWriterIsMatched(writerGuid));
        impl->DeadlineTimeout();
        InstanceHandle_t tracked;
        std::chrono::steady_clock::time_point due;
        EXPECT_FALSE(endpoint->GetReaderCache()->GetNextDeadline(tracked, due));
        RequestedDeadlineMissedStatus status;
        ASSERT_EQ(reader->get_requested_deadline_missed_status(status), RETCODE_OK);
        EXPECT_EQ(status.total_count, 0);
        auto delivery = deliver(reader, true);
        ASSERT_TRUE(endpoint->GetReaderCache()->GetNextDeadline(tracked, due));
        EXPECT_EQ(tracked, handle());
        EXPECT_GT(due, std::chrono::steady_clock::now());
        impl->DeadlineTimeout();
        ASSERT_EQ(reader->get_requested_deadline_missed_status(status), RETCODE_OK);
        EXPECT_EQ(status.total_count, 0);
        SequenceData<NativeSample> held; SampleInfoSeq info;
        ASSERT_EQ(reader->take(held, info, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
        ASSERT_TRUE(held.has_loan());
        auto disposed = endpoint->new_change(NOT_ALIVE_DISPOSED, nullptr, handle(), writer->get_instance_handle(), TIME_ZERO);
        disposed->sequenceNumber = {0, 2};
        ASSERT_EQ(endpoint->SubmitDataChange(disposed), RETCODE_OK);
        EXPECT_FALSE(endpoint->GetReaderCache()->GetNextDeadline(tracked, due));
        impl->DeadlineTimeout();
        ASSERT_EQ(reader->get_requested_deadline_missed_status(status), RETCODE_OK);
        EXPECT_EQ(status.total_count, 0);
        auto revived = endpoint->new_change(ALIVE, nullptr, handle(), writer->get_instance_handle(), TIME_ZERO);
        revived->sequenceNumber = {0, 3};
        auto original = delivery.change.lock();
        ASSERT_TRUE(original);
        revived->nativePayload = original->nativePayload;
        revived->nativeType = original->nativeType;
        ASSERT_EQ(endpoint->SubmitDataChange(revived), RETCODE_OK);
        ASSERT_TRUE(endpoint->GetReaderCache()->GetNextDeadline(tracked, due));
        ASSERT_TRUE(endpoint->GetReaderCache()->SetNextDeadline(handle(), std::chrono::steady_clock::now() - 1s));
        impl->DeadlineTimeout();
        ASSERT_EQ(reader->get_requested_deadline_missed_status(status), RETCODE_OK);
        EXPECT_EQ(status.total_count, 1);
        EXPECT_EQ(status.last_instance_handle, handle());
        ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
        EXPECT_EQ(held[0].index, 73u);
        held.unloan(); info.unloan();
    }
}

TEST_F(NativeReaderIntegration, TakeBorrowsMappedStorageAndReturnLoanValidatesReaderAndPair) {
    auto* reader = makeReader();
    auto* other = makeReader();
    ASSERT_TRUE(reader && other);
    auto delivery = deliver(reader);
    type.failDeserialize = type.failSerialize = true;
    SequenceData<NativeSample> values;
    SampleInfoSeq infos, wrongInfos(1);
    ASSERT_EQ(reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    ASSERT_EQ(values.length(), 1u);
    EXPECT_EQ(&values[0], delivery.address);
    EXPECT_TRUE(values.has_loan() && infos.has_loan());
    EXPECT_FALSE(values.has_ownership());
    EXPECT_EQ(values[0].index, 73u);
    EXPECT_EQ(type.deserializeCalls, 0);
    EXPECT_EQ(type.copyCalls, 0);
    EXPECT_EQ(freeSlots(delivery), 0u);
    EXPECT_EQ(other->return_loan(values, infos), RETCODE_PRECONDITION_NOT_MET);
    EXPECT_EQ(reader->return_loan(values, wrongInfos), RETCODE_PRECONDITION_NOT_MET);
    EXPECT_EQ(reader->return_loan(infos, infos), RETCODE_PRECONDITION_NOT_MET);
    EXPECT_EQ(reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_PRECONDITION_NOT_MET);
    ASSERT_TRUE(values.has_loan());
    EXPECT_EQ(reader->return_loan(values, infos), RETCODE_OK);
    EXPECT_EQ(values.length(), 0u);
    EXPECT_EQ(infos.length(), 0u);
    EXPECT_FALSE(values.has_ownership());
    EXPECT_TRUE(delivery.change.expired());
    EXPECT_EQ(freeSlots(delivery), 1u);
    EXPECT_EQ(reader->return_loan(values, infos), RETCODE_OK);
}

TEST_F(NativeReaderIntegration, ReadAndTakeHaveIndependentApplicationAndHistoryReferences) {
    auto* reader = makeReader();
    ASSERT_NE(reader, nullptr);
    auto delivery = deliver(reader);
    SequenceData<NativeSample> first, second;
    SampleInfoSeq a, b;
    ASSERT_EQ(reader->read(first, a, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    ASSERT_EQ(reader->take(second, b, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    EXPECT_EQ(&first[0], &second[0]);
    EXPECT_EQ(reader->return_loan(first, b), RETCODE_PRECONDITION_NOT_MET);
    EXPECT_EQ(reader->return_loan(first, a), RETCODE_OK);
    EXPECT_EQ(freeSlots(delivery), 0u);
    EXPECT_EQ(second[0].index, 73u);
    EXPECT_EQ(reader->return_loan(second, b), RETCODE_OK);
    EXPECT_EQ(freeSlots(delivery), 1u);
}

TEST_F(NativeReaderIntegration, BorrowedSequencesSurviveReaderDeletionAndCopyOutIsIndependent) {
    auto* reader = makeReader();
    ASSERT_NE(reader, nullptr);
    auto delivery = deliver(reader);
    SequenceData<NativeSample> values;
    SampleInfoSeq infos;
    ASSERT_EQ(reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    SequenceData<NativeSample> copied(values);
    EXPECT_NE(&copied[0], &values[0]);
    EXPECT_TRUE(copied.has_ownership());
    ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    EXPECT_EQ(values[0].index, 73u);
    values.unloan();
    EXPECT_EQ(freeSlots(delivery), 0u); // SampleInfo still owns the paired pin.
    infos.unloan();
    EXPECT_EQ(freeSlots(delivery), 1u);
    EXPECT_EQ(copied[0].index, 73u);
}

TEST_F(NativeReaderIntegration, PreallocatedOutputUsesNativeCopyWithoutDeserialization) {
    auto* reader = makeReader();
    ASSERT_NE(reader, nullptr);
    auto delivery = deliver(reader);
    SequenceData<NativeSample> values(1);
    SampleInfoSeq infos;
    type.failDeserialize = true;
    ASSERT_EQ(reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    EXPECT_TRUE(values.has_ownership());
    EXPECT_FALSE(values.has_loan());
    EXPECT_NE(&values[0], delivery.address);
    EXPECT_EQ(values[0].index, 73u);
    EXPECT_EQ(type.copyCalls, 1);
    EXPECT_EQ(type.deserializeCalls, 0);
    EXPECT_EQ(freeSlots(delivery), 1u);
    EXPECT_EQ(reader->return_loan(values, infos), RETCODE_OK);
    EXPECT_EQ(values.length(), 1u);
}

TEST_F(NativeReaderIntegration, InvalidRequestsDoNotConsumeQueuedSample) {
    auto* reader = makeReader();
    auto* other = makeReader();
    ASSERT_TRUE(reader && other);
    auto delivery = deliver(reader);
    SequenceData<NativeSample> values;
    SampleInfoSeq infos;
    auto* foreign = other->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    ASSERT_NE(foreign, nullptr);
    EXPECT_EQ(reader->take_w_condition(values, infos, 1, nullptr), RETCODE_BAD_PARAMETER);
    EXPECT_EQ(reader->take_w_condition(values, infos, 1, foreign), RETCODE_BAD_PARAMETER);
    EXPECT_EQ(reader->take(values, infos, 0, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_BAD_PARAMETER);
    EXPECT_EQ(reader->take(infos, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_BAD_PARAMETER);
    SampleInfo singleInfo;
    EXPECT_EQ(reader->take_next_sample(nullptr, singleInfo), RETCODE_BAD_PARAMETER);
    EXPECT_FALSE(delivery.change.expired());
    ASSERT_EQ(reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    EXPECT_EQ(values[0].index, 73u);
    EXPECT_EQ(reader->return_loan(values, infos), RETCODE_OK);
    EXPECT_EQ(freeSlots(delivery), 1u);
    EXPECT_EQ(other->delete_readcondition(foreign), RETCODE_OK);
}

TEST_F(NativeReaderIntegration, ReadConditionsStayLevelTriggeredUntilMatchingHistoryIsRemoved) {
    auto* reader = makeReader();
    ASSERT_NE(reader, nullptr);
    auto delivery = deliver(reader);
    auto* first = reader->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    auto* second = reader->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    auto* unread = reader->create_readcondition(NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    ASSERT_TRUE(first && second && unread);
    EXPECT_TRUE(first->get_trigger_value());
    EXPECT_TRUE(first->get_trigger_value());
    EXPECT_TRUE(second->get_trigger_value());
    EXPECT_TRUE(unread->get_trigger_value());
    WaitSet wait;
    ASSERT_EQ(wait.attach_condition(*first), RETCODE_OK);
    ASSERT_EQ(wait.attach_condition(*second), RETCODE_OK);
    ConditionSeq active;
    ASSERT_EQ(wait.wait(active, {0, 1000000}), RETCODE_OK);
    EXPECT_EQ(active.size(), 2u);
    SequenceData<NativeSample> readValues, takeValues;
    SampleInfoSeq readInfos, takeInfos;
    ASSERT_EQ(reader->read(readValues, readInfos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    EXPECT_FALSE(unread->get_trigger_value());
    EXPECT_TRUE(first->get_trigger_value());
    ASSERT_EQ(reader->take(takeValues, takeInfos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    EXPECT_FALSE(first->get_trigger_value());
    EXPECT_FALSE(second->get_trigger_value());
    EXPECT_EQ(wait.wait(active, {0, 1000000}), RETCODE_TIMEOUT);
    EXPECT_TRUE(active.empty());
    EXPECT_EQ(freeSlots(delivery), 0u); // Application loans are not readable history.
    EXPECT_EQ(reader->return_loan(readValues, readInfos), RETCODE_OK);
    EXPECT_EQ(reader->return_loan(takeValues, takeInfos), RETCODE_OK);
    EXPECT_EQ(freeSlots(delivery), 1u);
    EXPECT_EQ(reader->delete_readcondition(first), RETCODE_OK);
    EXPECT_EQ(reader->delete_readcondition(second), RETCODE_OK);
    EXPECT_EQ(reader->delete_readcondition(unread), RETCODE_OK);
}

class NativeReadVariants : public NativeReaderIntegration, public testing::WithParamInterface<int> {};
TEST_P(NativeReadVariants, EveryReadVariantHandlesNativePayloadWithoutCdr) {
    auto* reader = makeReader();
    ASSERT_NE(reader, nullptr);
    auto delivery = deliver(reader);
    auto* condition = reader->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    ASSERT_NE(condition, nullptr);
    type.failDeserialize = true;
    SequenceData<NativeSample> values;
    SampleInfoSeq infos;
    NativeSample single{};
    SampleInfo info;
    ReturnCode_t result = RETCODE_ERROR;
    switch (GetParam()) {
    case 0: result = reader->read(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
    case 1: result = reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
    case 2: result = reader->read_w_condition(values, infos, 1, condition); break;
    case 3: result = reader->take_w_condition(values, infos, 1, condition); break;
    case 4: result = reader->read_instance(values, infos, 1, handle(), ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
    case 5: result = reader->take_instance(values, infos, 1, handle(), ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
    case 6: result = reader->read_next_instance(values, infos, 1, HANDLE_NIL, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
    case 7: result = reader->take_next_instance(values, infos, 1, HANDLE_NIL, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
    case 8: result = reader->read_next_instance_w_condition(values, infos, 1, HANDLE_NIL, condition); break;
    case 9: result = reader->take_next_instance_w_condition(values, infos, 1, HANDLE_NIL, condition); break;
    case 10: result = reader->read_next_sample(&single, info); break;
    case 11: result = reader->take_next_sample(&single, info); break;
    }
    ASSERT_EQ(result, RETCODE_OK);
    if (GetParam() >= 10) {
        EXPECT_EQ(single.index, 73u);
        EXPECT_EQ(type.copyCalls, 1);
    } else {
        ASSERT_EQ(values.length(), 1u);
        EXPECT_EQ(&values[0], delivery.address);
        EXPECT_EQ(values[0].index, 73u);
        EXPECT_EQ(type.copyCalls, 0);
        EXPECT_EQ(reader->return_loan(values, infos), RETCODE_OK);
    }
    EXPECT_EQ(type.deserializeCalls, 0);
    EXPECT_EQ(reader->delete_readcondition(condition), RETCODE_OK);
}
INSTANTIATE_TEST_SUITE_P(AllNativeReadMethods, NativeReadVariants, testing::Range(0, 12));
#endif
namespace {
bool mappingPhase(int fd, char expected, bool sending) {
    pollfd item{fd, short(sending ? POLLOUT : POLLIN), 0};
    int ready;
    do { ready = poll(&item, 1, 10000); } while (ready < 0 && errno == EINTR);
    if (ready <= 0) { ADD_FAILURE() << "Mapping protocol phase " << expected; return false; }
    char value = expected;
    ssize_t size;
    do { size = sending ? send(fd, &value, 1, MSG_NOSIGNAL) : recv(fd, &value, 1, 0); }
    while (size < 0 && errno == EINTR);
    if (size != 1 || (!sending && value != expected)) {
        ADD_FAILURE() << "Mapping protocol peer/phase " << expected << ", received " << int(value);
        return false;
    }
    return true;
}
}
void LoanIntegration::RunMappingRecoveryExec(bool reliable) {
    const auto reliability = reliable ? RELIABLE_RELIABILITY_QOS : BEST_EFFORT_RELIABILITY_QOS;
    const bool capacity = std::string(testing::UnitTest::GetInstance()->current_test_info()->name()).find("Registration") != std::string::npos;
    const bool automatic = std::string(testing::UnitTest::GetInstance()->current_test_info()->name()).find("Automatic") != std::string::npos;
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK); writer = nullptr;
    if (const auto* childFd = getenv("TRAVODDS_MAPPING_CHILD")) {
        const int fd = std::stoi(childFd);
        close(std::stoi(getenv("TRAVODDS_MAPPING_PARENT")));
        char directory[] = "/dev/shm/travodds-reader-unmapped-XXXXXX";
        ASSERT_NE(mkdtemp(directory), nullptr);
        struct Cleanup { const char* path; std::string alias; ~Cleanup() { if (!alias.empty()) unlink(alias.c_str()); rmdir(path); } } cleanup{directory, {}};
        DataReaderQos qos; qos.reliability.kind = reliability; qos.history.depth = 8;
        qos.data_sharing.kind = ON_DATA_SHARING_QOS;
        qos.data_sharing.shared_memory_directory = capacity ? "/dev/shm" : directory;
        auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(reader, nullptr);
        SequenceData<NativeSample> held; SampleInfoSeq heldInfo;
        for (int stage = 0; stage < 2; ++stage) {
            InstanceHandleSeq matches;
            const auto matchDeadline = std::chrono::steady_clock::now() + 8s;
            const auto expectedHigh = std::stoull(getenv("TRAVODDS_MAPPING_SESSION_HIGH"));
            const auto expectedLow = std::stoull(getenv("TRAVODDS_MAPPING_SESSION_LOW"));
            PublicationBuiltinTopicData remote;
            InstanceHandle_t activeRemote;
            bool matched = false;
            do {
                ASSERT_EQ(reader->get_matched_publications(matches), RETCODE_OK);
                // The fixture's retired AUTO Writer can still be in discovery.
                // A match to it does not make this VOLATILE Reader ready for
                // the actual Writer's one-shot BEST_EFFORT fallback samples.
                for (const auto& handle : matches) {
                    if (reader->get_matched_publication_data(remote, handle) == RETCODE_OK &&
                        remote.data_sharing.session_high == expectedHigh &&
                        remote.data_sharing.session_low == expectedLow) {
                        matched = true;
                        activeRemote = handle;
                        break;
                    }
                }
                if (matched) break;
                std::this_thread::sleep_for(2ms);
            } while (std::chrono::steady_clock::now() < matchDeadline);
            ASSERT_TRUE(matched);
            EXPECT_TRUE(remote.data_sharing.compatible_with(
                dynamic_cast<DataReaderImpl*>(reader)->GetBuiltinTopicData().data_sharing));
            const auto deadline = std::chrono::steady_clock::now() + 8s;
            type.deserializeCalls = 0; type.failDeserialize = stage == 1;
            ASSERT_TRUE(mappingPhase(fd, stage ? 'R' : 'F', true));
            for (uint64_t index = uint64_t(stage * 2 + 1); index <= uint64_t(stage * 2 + 2); ++index) {
                SequenceData<NativeSample> data; SampleInfoSeq info;
                ReturnCode_t result;
                do {
                    result = reader->take(data, info, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
                    if (result != RETCODE_NO_DATA) break;
                    std::this_thread::sleep_for(2ms);
                } while (std::chrono::steady_clock::now() < deadline);
                SampleLostStatus lost;
                ASSERT_EQ(reader->get_sample_lost_status(lost), RETCODE_OK);
                ASSERT_EQ(result, RETCODE_OK) << "mapping stage=" << stage << " expected index=" << index
                    << " deserialize calls=" << type.deserializeCalls << " lost=" << lost.total_count;
                ASSERT_EQ(data.length(), 1u); ASSERT_TRUE(info[0].valid_data);
                EXPECT_EQ(data.has_loan(), stage == 1);
                EXPECT_EQ(data[0].index, index); EXPECT_EQ(data[0].values[6], index + 79);
                if (index == 3) { held = std::move(data); heldInfo = std::move(info); }
                else ASSERT_EQ(reader->return_loan(data, info), RETCODE_OK);
            }
            EXPECT_EQ(type.deserializeCalls, stage ? 0 : 2);
            ASSERT_TRUE(mappingPhase(fd, stage ? 'N' : 'C', true));
            ASSERT_TRUE(mappingPhase(fd, stage ? 'D' : 'X', false));
            if (capacity && stage == 0) continue;
            if (automatic && stage == 0) {
                // Bind the existing Reader directory to the real shared filesystem.
                // The same endpoint must retry; no new discovery/Reader is created.
                SharedPoolSession session{remote.data_sharing.session_high, remote.data_sharing.session_low};
                const auto source = SharedPayloadPool::pathFor(Conversion::ToGuid(activeRemote), session, "/dev/shm");
                cleanup.alias = std::string(directory) + "/" + std::filesystem::path(source).filename().string();
                ASSERT_EQ(link(source.c_str(), cleanup.alias.c_str()), 0);
                continue;
            }
            ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK); reader = nullptr;
            if (stage == 0) {
                qos.data_sharing.shared_memory_directory = "/dev/shm";
                reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
                ASSERT_NE(reader, nullptr);
            }
        }
        ASSERT_TRUE(held.has_loan()); EXPECT_EQ(held[0].index, 3u); EXPECT_EQ(held[0].values[6], 82u);
        held.unloan(); heldInfo.unloan(); close(fd); return;
    }
    DataWriterQos qos; qos.reliability.kind = reliability; qos.history.depth = 8;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    if (capacity) qos.data_sharing.max_readers = 1;
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto endpoint = rtps->GetWriter(Conversion::ToGuid(writer->get_instance_handle()).entityId);
    const auto& info = dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData().data_sharing;
    SharedPoolDescriptor d;
    d.writer = endpoint->GetGuid(); d.session = {info.session_high, info.session_low};
    d.path = SharedPayloadPool::pathFor(d.writer, d.session, "/dev/shm");
    d.config.payload_size = info.type.size; d.config.alignment = std::max(64u, info.type.alignment);
    d.config.slots = info.slots; d.config.max_readers = info.max_readers;
    d.config.max_segment_bytes = qos.data_sharing.max_segment_bytes; d.segment_bytes = info.segment_bytes;
    auto pool = SharedPayloadPool::open(d);
    auto blocker = capacity ? pool->registerReader() : nullptr;
    if (capacity) ASSERT_TRUE(blocker);
    int sockets[2]; ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0);
    struct Sockets { int* fds; ~Sockets() { for (auto fd : {fds[0], fds[1]}) if (fd >= 0) close(fd); } } socketsGuard{sockets};
    std::vector<std::string> environment;
    for (auto** p = environ; *p; ++p) environment.emplace_back(*p);
    environment.push_back("TRAVODDS_MAPPING_CHILD=" + std::to_string(sockets[1]));
    environment.push_back("TRAVODDS_MAPPING_PARENT=" + std::to_string(sockets[0]));
    environment.push_back("TRAVODDS_MAPPING_SESSION_HIGH=" + std::to_string(info.session_high));
    environment.push_back("TRAVODDS_MAPPING_SESSION_LOW=" + std::to_string(info.session_low));
    std::vector<char*> env; for (auto& value : environment) env.push_back(value.data()); env.push_back(nullptr);
    std::string filter = "--gtest_filter=LoanIntegration." + std::string(testing::UnitTest::GetInstance()->current_test_info()->name());
    char executable[] = "/proc/self/exe"; char* args[] = {executable, filter.data(), nullptr};
    pid_t pid = -1; ASSERT_EQ(posix_spawn(&pid, executable, nullptr, nullptr, args, env.data()), 0);
    struct Child { pid_t pid; ~Child() { if (pid > 0) { kill(pid, SIGKILL); waitpid(pid, nullptr, 0); } } } child{pid};
    close(sockets[1]); sockets[1] = -1;
    for (int stage = 0; stage < 2; ++stage) {
        ASSERT_TRUE(mappingPhase(sockets[0], stage ? 'R' : 'F', false));
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        InstanceHandleSeq matches;
        do {
            ASSERT_EQ(writer->get_matched_subscriptions(matches), RETCODE_OK);
            if (matches.size() == 1u && endpoint->SelectDataSharingReaders(pool->registeredReaders()).size() == size_t(stage)) break;
            std::this_thread::sleep_for(2ms);
        } while (std::chrono::steady_clock::now() < deadline);
        ASSERT_EQ(matches.size(), 1u);
        ASSERT_EQ(endpoint->SelectDataSharingReaders(pool->registeredReaders()).size(), size_t(stage));
        type.serializeCalls = 0; type.failSerialize = stage == 1;
        for (uint64_t index = uint64_t(stage * 2 + 1); index <= uint64_t(stage * 2 + 2); ++index) {
            NativeSample sample{}; sample.index = index; sample.values[6] = index + 79;
            ASSERT_EQ(writer->write(&sample, HANDLE_NIL), RETCODE_OK);
        }
        ASSERT_TRUE(mappingPhase(sockets[0], stage ? 'N' : 'C', false));
        EXPECT_EQ(type.serializeCalls, stage ? 0 : 2);
        if (capacity && stage == 0) blocker.reset();
        if (stage == 1) { ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK); writer = nullptr; }
        ASSERT_TRUE(mappingPhase(sockets[0], stage ? 'D' : 'X', true));
    }
    int status = 0; pid_t reaped = 0;
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    do { reaped = waitpid(pid, &status, WNOHANG); if (reaped) break; std::this_thread::sleep_for(2ms); }
    while (std::chrono::steady_clock::now() < deadline);
    ASSERT_EQ(reaped, pid); child.pid = -1;
    ASSERT_TRUE(WIFEXITED(status)); EXPECT_EQ(WEXITSTATUS(status), 0);
    EXPECT_FALSE(std::filesystem::exists(d.path));
}
TEST_F(LoanIntegration, ReliableCompatibleMappingFailureFallsBackAndReaderRecreationRecoversAcrossExec) { RunMappingRecoveryExec(true); }
TEST_F(LoanIntegration, BestEffortCompatibleMappingFailureFallsBackAndReaderRecreationRecoversAcrossExec) { RunMappingRecoveryExec(false); }

TEST_F(LoanIntegration, ReliableAutomaticMappingRetryRecoversAcrossExec) { RunMappingRecoveryExec(true); }
TEST_F(LoanIntegration, BestEffortAutomaticMappingRetryRecoversAcrossExec) { RunMappingRecoveryExec(false); }

TEST_F(LoanIntegration, ReliableAutomaticRegistrationRetryRecoversAcrossExec) { RunMappingRecoveryExec(true); }
TEST_F(LoanIntegration, BestEffortAutomaticRegistrationRetryRecoversAcrossExec) { RunMappingRecoveryExec(false); }

void LoanIntegration::RunReaderCapacity(bool native) {
    DataReaderQos qos; qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
    qos.resource_limits.max_instances = 1; qos.resource_limits.max_samples = 4;
    qos.resource_limits.max_samples_per_instance = 4; qos.history.depth = 4;
    auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto endpoint = rtps->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
    auto cacheFor = [&](uint64_t index) {
        auto sample = std::make_shared<NativeSample>(); sample->index = index; sample->values[6] = index + 79;
        InstanceHandle_t handle; if (type.MakeKey(sample.get(), handle, true)) throw std::runtime_error("MakeKey failed");
        SerializedBuffer* buffer = nullptr;
        if (!native) {
            PayloadBufferPtr owner(new SerializedBuffer, PayloadBufferDeleter{MemoryPool::GetInstance()});
            if (!MemoryPool::GetInstance()->getBuffer(68, owner->origin_buffer)) throw std::bad_alloc();
            std::memset(owner->origin_buffer, 0, 4); owner->origin_buffer[1] = DEFAULT_ENDIAN;
            owner->buffer = owner->origin_buffer + 4; owner->buffer_size = 64;
            CdrSerializer cdr(owner.get());
            if (type.serialize_data(sample.get(), &cdr, DEFAULT_ENDIAN)) throw std::runtime_error("serialize failed");
            owner->buffer = owner->origin_buffer; owner->buffer_size = owner->writer_position = 68;
            buffer = owner.release();
        }
        auto cache = endpoint->new_change(ALIVE, buffer, handle, writer->get_instance_handle(), TIME_ZERO);
        cache->sequenceNumber = {0, uint32_t(index)};
        if (native) { cache->nativePayload = sample; cache->nativeType = type.get_native_type_info(); }
        return cache;
    };
    auto matchUntil = std::chrono::steady_clock::now() + 4s;
    while (!endpoint->MatchedWriterIsMatched(Conversion::ToGuid(writer->get_instance_handle())) &&
        std::chrono::steady_clock::now() < matchUntil) std::this_thread::sleep_for(2ms);
    auto first = cacheFor(1); const auto firstHandle = first->instanceHandle;
    ASSERT_EQ(endpoint->AddChanges(first), RETCODE_OK); first.reset();
    SequenceData<NativeSample> held; SampleInfoSeq info;
    ASSERT_EQ(reader->take(held, info, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    ASSERT_EQ(held.length(), 1u); EXPECT_EQ(held.has_loan(), native);
    EXPECT_EQ(held[0].index, 1u);
    // Taking and Writer disappearance preserve the known instance and key.
    endpoint->GetReaderCache()->RemoveRemoteWriter(Conversion::ToGuid(writer->get_instance_handle()));
    NativeSample key{};
    ASSERT_EQ(reader->get_key_value(&key, firstHandle), RETCODE_OK); EXPECT_EQ(key.index, 1u);
    auto second = cacheFor(2);
    EXPECT_FALSE(endpoint->GetReaderCache()->IsKeyRegistered(second->instanceHandle));
    EXPECT_EQ(endpoint->AddChanges(second), REJECTED_BY_INSTANCES_LIMIT);
    EXPECT_FALSE(endpoint->GetReaderCache()->IsKeyRegistered(second->instanceHandle));
    ASSERT_EQ(reader->return_loan(held, info), RETCODE_OK);
    EXPECT_EQ(endpoint->AddChanges(second), REJECTED_BY_INSTANCES_LIMIT);
    EXPECT_EQ(reader->get_key_value(&key, firstHandle), RETCODE_OK);
    ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    // Recreating the Reader starts a new instance budget; rejected keys did not
    // leak into the old index, and returned/preserved Loans remain independent.
    reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);
    endpoint = rtps->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
    matchUntil = std::chrono::steady_clock::now() + 4s;
    while (!endpoint->MatchedWriterIsMatched(Conversion::ToGuid(writer->get_instance_handle())) &&
        std::chrono::steady_clock::now() < matchUntil) std::this_thread::sleep_for(2ms);
    ASSERT_EQ(endpoint->AddChanges(second), RETCODE_OK);
    NativeSample received{}; SampleInfo receivedInfo;
    ASSERT_EQ(reader->take_next_sample(&received, receivedInfo), RETCODE_OK); EXPECT_EQ(received.index, 2u);
}
TEST_F(LoanIntegration, KeyedNativeReaderKnownInstanceLimitSurvivesTakeWriterLossAndLoanReturn) { RunReaderCapacity(true); }
TEST_F(LoanIntegration, KeyedCdrReaderKnownInstanceLimitSurvivesTakeWriterLossAndLoanReturn) { RunReaderCapacity(false); }

void LoanIntegration::RunMultiEndpointExec(bool reliable, bool crashReader, bool forceOwnerDeath) {
    const auto reliability = reliable ? RELIABLE_RELIABILITY_QOS : BEST_EFFORT_RELIABILITY_QOS;
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK); writer = nullptr;
    auto* otherTopic = participant->create_topic("native-loan-other", type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(otherTopic, nullptr);
    constexpr unsigned iterations = 12;
    if (const auto* childFd = getenv("TRAVODDS_MULTI_CHILD")) {
        const int fd = std::stoi(childFd), round = std::stoi(getenv("TRAVODDS_MULTI_ROUND"));
        close(std::stoi(getenv("TRAVODDS_MULTI_PARENT")));
        type.failDeserialize = true;
        DataReaderQos qos; qos.data_sharing.kind = ON_DATA_SHARING_QOS;
        qos.reliability.kind = reliability; qos.history.depth = 4;
        std::vector<DataReader*> readers;
        for (auto* t : {topic, topic, otherTopic}) {
            auto* reader = subscriber->create_datareader(t, qos, nullptr, STATUS_MASK_NONE);
            ASSERT_NE(reader, nullptr); readers.push_back(reader);
        }
        ASSERT_TRUE(mappingPhase(fd, 'R', true));
        SequenceData<NativeSample> held; SampleInfoSeq heldInfo; uint64_t heldIndex = 0;
        for (unsigned i = 1; i <= iterations; ++i) {
            for (size_t r = 0; r < readers.size(); ++r) {
                SCOPED_TRACE("iteration=" + std::to_string(i) + " reader=" + std::to_string(r));
                std::set<uint64_t> expected = r == 2 ? std::set<uint64_t>{3000 + i} : std::set<uint64_t>{1000 + i, 2000 + i};
                const auto deadline = std::chrono::steady_clock::now() + 8s;
                while (!expected.empty()) {
                    SequenceData<NativeSample> values; SampleInfoSeq infos; ReturnCode_t result;
                    do {
                        result = readers[r]->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
                        if (result != RETCODE_NO_DATA) break;
                        std::this_thread::sleep_for(2ms);
                    } while (std::chrono::steady_clock::now() < deadline);
                    ASSERT_EQ(result, RETCODE_OK); ASSERT_EQ(values.length(), 1u);
                    ASSERT_TRUE(values.has_loan()); ASSERT_TRUE(infos[0].valid_data);
                    const auto index = values[0].index;
                    ASSERT_EQ(expected.erase(index), 1u) << "Reader=" << r << " index=" << index;
                    EXPECT_EQ(values[0].values[6], index + 79);
                    if (r == 0 && i == 1 && !held.has_loan()) {
                        heldIndex = index; held = std::move(values); heldInfo = std::move(infos);
                    } else ASSERT_EQ(readers[r]->return_loan(values, infos), RETCODE_OK);
                }
            }
            ASSERT_TRUE(held.has_loan()); EXPECT_EQ(held[0].index, heldIndex);
            EXPECT_EQ(held[0].values[6], heldIndex + 79);
            ASSERT_TRUE(mappingPhase(fd, 'A', true));
        }
        EXPECT_EQ(type.deserializeCalls, 0);
        ASSERT_TRUE(mappingPhase(fd, 'F', true));
        // The parent first inspects the live Loan, then permits the fault.
        // A deterministic owner-death variant holds the actual pool mutex
        // while SIGKILL arrives; the ordinary variant can die in any phase.
        struct LockedHeader {
            shared_pool_detail::Header* header = nullptr;
            bool locked = false;
            ~LockedHeader() {
                if (locked) pthread_mutex_unlock(&header->mutex);
                if (header) munmap(header, sizeof(*header));
            }
        } heldMutex;
        if (crashReader && round == 0) {
            ASSERT_TRUE(mappingPhase(fd, 'L', false));
            if (forceOwnerDeath) {
                PublicationBuiltinTopicData remote;
                ASSERT_EQ(readers[0]->get_matched_publication_data(remote, heldInfo[0].publication_handle), RETCODE_OK);
                const auto& info = remote.data_sharing;
                const auto path = SharedPayloadPool::pathFor(Conversion::ToGuid(heldInfo[0].publication_handle),
                    {info.session_high, info.session_low}, "/dev/shm");
                int segment = open(path.c_str(), O_RDWR | O_CLOEXEC);
                ASSERT_GE(segment, 0);
                void* mapping = mmap(nullptr, sizeof(shared_pool_detail::Header), PROT_READ | PROT_WRITE, MAP_SHARED, segment, 0);
                close(segment);
                ASSERT_NE(mapping, MAP_FAILED);
                heldMutex.header = static_cast<shared_pool_detail::Header*>(mapping);
                ASSERT_EQ(heldMutex.header->prefix.magic, shared_pool_detail::MAGIC);
                ASSERT_EQ(heldMutex.header->prefix.session.high, info.session_high);
                ASSERT_EQ(heldMutex.header->prefix.session.low, info.session_low);
                const int result = pthread_mutex_lock(&heldMutex.header->mutex);
                heldMutex.locked = result == 0 || result == EOWNERDEAD;
                if (result == EOWNERDEAD) pthread_mutex_consistent(&heldMutex.header->mutex);
                ASSERT_EQ(result, 0);
            }
            ASSERT_TRUE(mappingPhase(fd, 'K', true));
        }
        ASSERT_TRUE(mappingPhase(fd, 'D', false));
        EXPECT_FALSE(crashReader && round == 0);
        for (auto* reader : readers) ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
        EXPECT_EQ(held[0].index, heldIndex); EXPECT_EQ(held[0].values[6], heldIndex + 79);
        held.unloan(); heldInfo.unloan(); ASSERT_TRUE(mappingPhase(fd, 'E', true));
        close(fd); return;
    }
    DataWriterQos qos; qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.data_sharing.max_samples = 4; qos.data_sharing.max_readers = 4;
    qos.reliability.kind = reliability; qos.history.depth = 1;
    std::vector<DataWriter*> writers; std::vector<std::shared_ptr<Writer>> endpoints;
    std::vector<std::shared_ptr<SharedPayloadPool>> pools;
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    for (auto* t : {topic, topic, otherTopic}) {
        auto* value = publisher->create_datawriter(t, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(value, nullptr); writers.push_back(value);
        auto endpoint = rtps->GetWriter(Conversion::ToGuid(value->get_instance_handle()).entityId); endpoints.push_back(endpoint);
        const auto& info = dynamic_cast<DataWriterImpl*>(value)->GetBuiltinTopicData().data_sharing;
        SharedPoolDescriptor d; d.writer = endpoint->GetGuid(); d.session = {info.session_high, info.session_low};
        d.path = SharedPayloadPool::pathFor(d.writer, d.session, "/dev/shm");
        d.config.payload_size = info.type.size; d.config.alignment = std::max(64u, info.type.alignment);
        d.config.slots = info.slots; d.config.max_readers = info.max_readers;
        d.config.max_segment_bytes = qos.data_sharing.max_segment_bytes; d.segment_bytes = info.segment_bytes;
        pools.push_back(SharedPayloadPool::open(d));
    }
    for (int round = crashReader ? 0 : 1; round <= 1; ++round) {
        int sockets[2]; ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0);
        struct Sockets { int* fds; ~Sockets() { for (auto fd : {fds[0], fds[1]}) if (fd >= 0) close(fd); } } socketGuard{sockets};
        std::vector<std::string> environment; for (auto** p = environ; *p; ++p) environment.emplace_back(*p);
        environment.push_back("TRAVODDS_MULTI_CHILD=" + std::to_string(sockets[1]));
        environment.push_back("TRAVODDS_MULTI_PARENT=" + std::to_string(sockets[0]));
        environment.push_back("TRAVODDS_MULTI_ROUND=" + std::to_string(round));
        std::vector<char*> env; for (auto& value : environment) env.push_back(value.data()); env.push_back(nullptr);
        std::string filter = "--gtest_filter=LoanIntegration." + std::string(testing::UnitTest::GetInstance()->current_test_info()->name());
        char executable[] = "/proc/self/exe"; char* args[] = {executable, filter.data(), nullptr};
        pid_t pid = -1; ASSERT_EQ(posix_spawn(&pid, executable, nullptr, nullptr, args, env.data()), 0);
        struct Child { pid_t pid; ~Child() { if (pid > 0) { kill(pid, SIGKILL); waitpid(pid, nullptr, 0); } } } child{pid};
        close(sockets[1]); sockets[1] = -1;
        ASSERT_TRUE(mappingPhase(sockets[0], 'R', false));
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        for (size_t w = 0; w < writers.size(); ++w) {
            const size_t expected = w == 2 ? 1 : 2;
            do {
                if (endpoints[w]->SelectDataSharingReaders(pools[w]->registeredReaders()).size() == expected) break;
                std::this_thread::sleep_for(2ms);
            } while (std::chrono::steady_clock::now() < deadline);
            ASSERT_EQ(endpoints[w]->SelectDataSharingReaders(pools[w]->registeredReaders()).size(), expected);
        }
        // A dead process is reaped from SHM immediately after confirmed death.
        // Its DDS discovery lease may remain; during restart permit ordinary
        // CDR for those stale RTPS destinations instead of claiming zero CDR.
        type.failSerialize = !(crashReader && round == 1); type.serializeCalls = 0;
        for (unsigned i = 1; i <= iterations; ++i) {
            for (size_t w = 0; w < writers.size(); ++w) {
                SampleLoan loan; ASSERT_EQ(writers[w]->loan_sample(loan), RETCODE_OK);
                auto* value = static_cast<NativeSample*>(loan.data());
                const uint64_t index = (w + 1) * 1000 + i;
                initializeNativeSample(*value, index, index + 79);
                ASSERT_EQ(writers[w]->write(loan, HANDLE_NIL), RETCODE_OK);
            }
            ASSERT_TRUE(mappingPhase(sockets[0], 'A', false));
        }
        ASSERT_TRUE(mappingPhase(sockets[0], 'F', false));
        if (!crashReader || round == 0) EXPECT_EQ(type.serializeCalls, 0);
        if (crashReader && round == 0) {
            uint64_t loans = 0; for (auto& pool : pools) { SharedPoolStats stats; ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK); loans += stats.loan_references; }
            EXPECT_EQ(loans, 1u);
            ASSERT_TRUE(mappingPhase(sockets[0], 'L', true));
            ASSERT_TRUE(mappingPhase(sockets[0], 'K', false));
            ASSERT_EQ(kill(pid, SIGKILL), 0); int status = 0;
            ASSERT_EQ(waitpid(pid, &status, 0), pid); child.pid = -1;
            ASSERT_TRUE(WIFSIGNALED(status)); EXPECT_EQ(WTERMSIG(status), SIGKILL);
            unsigned poisoned = 0;
            for (size_t w = 0; w < pools.size(); ++w) {
                auto& pool = pools[w];
                SharedPoolStats stats;
                const auto result = pool->statistics(stats);
                if (result == SharedPoolResult::CORRUPT) {
                    ++poisoned;
                    // An asynchronous kill may interrupt a metadata transaction.
                    // Verify safe refusal and recover by recreating the Writer,
                    // rather than expecting the poisoned pool to reap/reuse pins.
                    SampleLoan rejected;
                    EXPECT_EQ(writers[w]->loan_sample(rejected), RETCODE_ERROR);
                    EXPECT_FALSE(bool(rejected));
                    const auto previous = pool->descriptor();
                    ASSERT_EQ(publisher->delete_datawriter(writers[w]), RETCODE_OK);
                    EXPECT_FALSE(std::filesystem::exists(previous.path));
                    writers[w] = publisher->create_datawriter(w == 2 ? otherTopic : topic, qos, nullptr, STATUS_MASK_NONE);
                    ASSERT_NE(writers[w], nullptr);
                    endpoints[w] = rtps->GetWriter(Conversion::ToGuid(writers[w]->get_instance_handle()).entityId);
                    const auto& info = dynamic_cast<DataWriterImpl*>(writers[w])->GetBuiltinTopicData().data_sharing;
                    auto descriptor = previous;
                    descriptor.writer = endpoints[w]->GetGuid();
                    descriptor.session = {info.session_high, info.session_low};
                    descriptor.path = SharedPayloadPool::pathFor(descriptor.writer, descriptor.session, "/dev/shm");
                    descriptor.segment_bytes = info.segment_bytes;
                    EXPECT_FALSE(descriptor.session == previous.session);
                    EXPECT_NE(descriptor.writer, previous.writer);
                    pool = SharedPayloadPool::open(descriptor);
                    std::cout << "READER_OWNER_DEATH writer=" << w << " recreated=1\n";
                } else {
                    ASSERT_EQ(result, SharedPoolResult::OK);
                    EXPECT_TRUE(pool->registeredReaders().empty());
                    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
                    EXPECT_EQ(stats.loan_references, 0u); EXPECT_EQ(stats.pending_references, 0u);
                    EXPECT_EQ(stats.free_slots, 3u);
                }
            }
            if (forceOwnerDeath) EXPECT_GE(poisoned, 1u);
            continue;
        }
        for (size_t w = 0; w < writers.size(); ++w) {
            ASSERT_EQ(publisher->delete_datawriter(writers[w]), RETCODE_OK);
            EXPECT_FALSE(std::filesystem::exists(pools[w]->descriptor().path));
        }
        ASSERT_TRUE(mappingPhase(sockets[0], 'D', true)); ASSERT_TRUE(mappingPhase(sockets[0], 'E', false));
        int status = 0; pid_t reaped = 0;
        do { reaped = waitpid(pid, &status, WNOHANG); if (reaped) break; std::this_thread::sleep_for(2ms); }
        while (std::chrono::steady_clock::now() < deadline);
        ASSERT_EQ(reaped, pid); child.pid = -1;
        ASSERT_TRUE(WIFEXITED(status)); EXPECT_EQ(WEXITSTATUS(status), 0);
    }
}
TEST_F(LoanIntegration, ReliableMultiTopicThreeWritersThreeReadersHoldLoanAcrossRingReuseAndExec) { RunMultiEndpointExec(true, false); }
TEST_F(LoanIntegration, BestEffortMultiTopicThreeWritersThreeReadersHoldLoanAcrossRingReuseAndExec) { RunMultiEndpointExec(false, false); }
TEST_F(LoanIntegration, ReliableKilledReaderPinsReapAndNewProcessReceivesAcrossExec) { RunMultiEndpointExec(true, true); }
TEST_F(LoanIntegration, BestEffortKilledReaderPinsReapAndNewProcessReceivesAcrossExec) { RunMultiEndpointExec(false, true); }
TEST_F(LoanIntegration, ReliableKilledReaderOwnerDeathRecreatesWriterAndReceivesAcrossExec) { RunMultiEndpointExec(true, true, true); }
TEST_F(LoanIntegration, BestEffortKilledReaderOwnerDeathRecreatesWriterAndReceivesAcrossExec) { RunMultiEndpointExec(false, true, true); }

void LoanIntegration::RunWriterCrashExec(bool reliable) {
    const auto reliability = reliable ? RELIABLE_RELIABILITY_QOS : BEST_EFFORT_RELIABILITY_QOS;
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK); writer = nullptr;
    if (const auto* childFd = getenv("TRAVODDS_WRITERCRASH_CHILD")) {
        const int fd = std::stoi(childFd); close(std::stoi(getenv("TRAVODDS_WRITERCRASH_PARENT")));
        DataWriterQos qos; qos.data_sharing.kind = ON_DATA_SHARING_QOS;
        qos.reliability.kind = reliability; qos.history.depth = 1; qos.data_sharing.max_samples = 4;
        writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE); ASSERT_NE(writer, nullptr);
        auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
        auto endpoint = rtps->GetWriter(Conversion::ToGuid(writer->get_instance_handle()).entityId);
        const auto& info = dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData().data_sharing;
        SharedPoolDescriptor d; d.writer = endpoint->GetGuid(); d.session = {info.session_high, info.session_low};
        d.path = SharedPayloadPool::pathFor(d.writer, d.session, "/dev/shm");
        d.config.payload_size = info.type.size; d.config.alignment = std::max(64u, info.type.alignment);
        d.config.slots = info.slots; d.config.max_readers = info.max_readers;
        d.config.max_segment_bytes = qos.data_sharing.max_segment_bytes; d.segment_bytes = info.segment_bytes;
        auto pool = SharedPayloadPool::open(d);
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        do {
            if (endpoint->SelectDataSharingReaders(pool->registeredReaders()).size() == 1u) break;
            std::this_thread::sleep_for(2ms);
        } while (std::chrono::steady_clock::now() < deadline);
        ASSERT_EQ(endpoint->SelectDataSharingReaders(pool->registeredReaders()).size(), 1u);
        type.failSerialize = true;
        SampleLoan loan; ASSERT_EQ(writer->loan_sample(loan), RETCODE_OK);
        auto* sample = static_cast<NativeSample*>(loan.data());
        const auto index = std::stoull(getenv("TRAVODDS_WRITERCRASH_INDEX"));
        initializeNativeSample(*sample, index, index + 79);
        ASSERT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_OK);
        EXPECT_EQ(type.serializeCalls, 0); ASSERT_TRUE(mappingPhase(fd, 'F', true));
        ASSERT_TRUE(mappingPhase(fd, 'H', false)); ASSERT_TRUE(mappingPhase(fd, 'K', true));
        ASSERT_TRUE(mappingPhase(fd, 'D', false)); // Parent deliberately kills this exact Writer process.
        FAIL() << "Writer crash child must not exit normally";
    }
    DataReaderQos qos; qos.data_sharing.kind = ON_DATA_SHARING_QOS; qos.reliability.kind = reliability;
    auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE); ASSERT_NE(reader, nullptr);
    type.failDeserialize = true;
    SequenceData<NativeSample> held; SampleInfoSeq heldInfo;
    std::shared_ptr<SharedPayloadPool> firstPool; SharedPoolDescriptor firstDescriptor;
    for (uint64_t round = 0; round < 2; ++round) {
        int sockets[2]; ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0);
        struct Sockets { int* fds; ~Sockets() { for (auto fd : {fds[0], fds[1]}) if (fd >= 0) close(fd); } } socketGuard{sockets};
        std::vector<std::string> environment; for (auto** p = environ; *p; ++p) environment.emplace_back(*p);
        environment.push_back("TRAVODDS_WRITERCRASH_CHILD=" + std::to_string(sockets[1]));
        environment.push_back("TRAVODDS_WRITERCRASH_PARENT=" + std::to_string(sockets[0]));
        environment.push_back("TRAVODDS_WRITERCRASH_INDEX=" + std::to_string(5678 + round));
        std::vector<char*> env; for (auto& value : environment) env.push_back(value.data()); env.push_back(nullptr);
        std::string filter = "--gtest_filter=LoanIntegration." + std::string(testing::UnitTest::GetInstance()->current_test_info()->name());
        char executable[] = "/proc/self/exe"; char* args[] = {executable, filter.data(), nullptr};
        pid_t pid = -1; ASSERT_EQ(posix_spawn(&pid, executable, nullptr, nullptr, args, env.data()), 0);
        struct Child {
            pid_t pid; SharedPoolDescriptor descriptor; bool known = false;
            ~Child() {
                if (pid > 0) { kill(pid, SIGKILL); waitpid(pid, nullptr, 0); }
                if (known) { try { SharedPayloadPool::removeOrphan(descriptor); } catch (const std::exception&) {} }
            }
        } child{pid, {}, false};
        close(sockets[1]); sockets[1] = -1;
        ASSERT_TRUE(mappingPhase(sockets[0], 'F', false));
        SequenceData<NativeSample> values; SampleInfoSeq infos;
        const auto deadline = std::chrono::steady_clock::now() + 8s; ReturnCode_t result;
        do {
            result = reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
            if (result != RETCODE_NO_DATA) break; std::this_thread::sleep_for(2ms);
        } while (std::chrono::steady_clock::now() < deadline);
        ASSERT_EQ(result, RETCODE_OK); ASSERT_EQ(values.length(), 1u); ASSERT_TRUE(values.has_loan());
        EXPECT_EQ(values[0].index, 5678 + round); EXPECT_EQ(values[0].values[6], 5757 + round);
        PublicationBuiltinTopicData info;
        ASSERT_EQ(reader->get_matched_publication_data(info, infos[0].publication_handle), RETCODE_OK);
        SharedPoolDescriptor d; d.writer = Conversion::ToGuid(infos[0].publication_handle);
        const auto& capability = info.data_sharing;
        d.session = {capability.session_high, capability.session_low};
        d.path = SharedPayloadPool::pathFor(d.writer, d.session, "/dev/shm");
        d.config.payload_size = capability.type.size; d.config.alignment = std::max(64u, capability.type.alignment);
        d.config.slots = capability.slots; d.config.max_readers = capability.max_readers;
        d.config.max_segment_bytes = qos.data_sharing.max_segment_bytes; d.segment_bytes = capability.segment_bytes;
        child.descriptor = d; child.known = true;
        auto pool = SharedPayloadPool::open(d);
        if (round == 0) { held = std::move(values); heldInfo = std::move(infos); firstPool = pool; firstDescriptor = d; }
        else {
            EXPECT_FALSE(d.session == firstDescriptor.session); EXPECT_NE(d.path, firstDescriptor.path);
            EXPECT_NE(d.writer, firstDescriptor.writer);
            ASSERT_EQ(reader->return_loan(values, infos), RETCODE_OK);
        }
        ASSERT_TRUE(mappingPhase(sockets[0], 'H', true)); ASSERT_TRUE(mappingPhase(sockets[0], 'K', false));
        ASSERT_EQ(kill(pid, SIGKILL), 0); int status = 0;
        ASSERT_EQ(waitpid(pid, &status, 0), pid); child.pid = -1;
        ASSERT_TRUE(WIFSIGNALED(status)); EXPECT_EQ(WTERMSIG(status), SIGKILL);
        SharedPoolStats stats; ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
        EXPECT_FALSE(stats.writer_alive);
        EXPECT_EQ(stats.loan_references, round == 0 ? 1u : 0u);
        ASSERT_TRUE(SharedPayloadPool::removeOrphan(d)); child.known = false; EXPECT_FALSE(std::filesystem::exists(d.path));
        EXPECT_ANY_THROW(SharedPayloadPool::open(d));
        ASSERT_TRUE(held.has_loan()); EXPECT_EQ(held[0].index, 5678u); EXPECT_EQ(held[0].values[6], 5757u);
    }
    ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    EXPECT_EQ(held[0].index, 5678u); EXPECT_EQ(held[0].values[6], 5757u);
    held.unloan(); heldInfo.unloan();
    SharedPoolStats stats; ASSERT_EQ(firstPool->statistics(stats), SharedPoolResult::OK); EXPECT_EQ(stats.loan_references, 0u);
    EXPECT_EQ(type.deserializeCalls, 0);
}
TEST_F(LoanIntegration, ReliableKilledWriterLoanSurvivesRestartAndExactOrphanCleanupAcrossExec) { RunWriterCrashExec(true); }
TEST_F(LoanIntegration, BestEffortKilledWriterLoanSurvivesRestartAndExactOrphanCleanupAcrossExec) { RunWriterCrashExec(false); }

void LoanIntegration::RunDestinationFilter(bool fragmented) {
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    MessageProcessor processor(rtps.get());
    for (auto reliability : {BEST_EFFORT_RELIABILITY_QOS, RELIABLE_RELIABILITY_QOS}) {
        SCOPED_TRACE(reliability);
        DataReaderQos qos; qos.data_sharing.kind = OFF_DATA_SHARING_QOS; qos.reliability.kind = reliability;
        auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE); ASSERT_NE(reader, nullptr);
        const auto writerGuid = Conversion::ToGuid(writer->get_instance_handle());
        const auto readerGuid = Conversion::ToGuid(reader->get_instance_handle());
        // Fragments need a discovered WriterProxy; do not race SEDP discovery.
        InstanceHandleSeq matches;
        const auto deadline = std::chrono::steady_clock::now() + 4s;
        do {
            ASSERT_EQ(reader->get_matched_publications(matches), RETCODE_OK);
            if (std::find(matches.begin(), matches.end(), writer->get_instance_handle()) != matches.end()) break;
            std::this_thread::sleep_for(2ms);
        } while (std::chrono::steady_clock::now() < deadline);
        ASSERT_NE(std::find(matches.begin(), matches.end(), writer->get_instance_handle()), matches.end());
        for (unsigned phase = 0; phase < 3; ++phase) {
            SCOPED_TRACE(phase);
            auto bytes = std::shared_ptr<char[]>(new char[4096]{});
            SerializedBuffer payload; payload.buffer = bytes.get() + 2048 + 4; payload.buffer_size = 64;
            NativeSample sample{}; sample.index = 17 + phase; sample.values[6] = 96 + phase;
            CdrSerializer cdr(&payload); ASSERT_EQ(type.serialize_data(&sample, &cdr, DEFAULT_ENDIAN), 0);
            payload.buffer -= 4; payload.buffer_size = payload.writer_position = 68; payload.buffer[1] = DEFAULT_ENDIAN;
            Receive_t receive; receive.sourceGuidPrefix = writerGuid.prefix;
            receive.destGuidPrefix = phase == 2 ? GUIDPREFIX_UNKNOWN : readerGuid.prefix;
            if (phase == 0) receive.destGuidPrefix.value[0] ^= 0xff;
            uint32_t shift = 0;
            SubMessageHeader_t header;
            if (fragmented) {
                DataFragSubMessage_t data; data.header.flags = DEFAULT_ENDIAN;
                data.writerId = writerGuid.entityId; data.readerId = readerGuid.entityId; data.writerSN = {0, phase == 0 ? 1u : phase};
                data.dataFrag.fragmentStartingNum = 1; data.dataFrag.fragmentsInSubmessage = 1;
                data.dataFrag.fragmentSize = 68; data.dataFrag.sampleSize = 68;
                ASSERT_EQ(MessageProcessor::EncapsulateDataFragMessage(payload, shift, data), RETCODE_OK); header = data.header;
            } else {
                DataSubMessage_t data; data.header.flags = DEFAULT_ENDIAN | SubMessageFlag::DATA_FLAG;
                data.writerId = writerGuid.entityId; data.readerId = readerGuid.entityId; data.writerSN = {0, phase == 0 ? 1u : phase};
                ASSERT_EQ(MessageProcessor::EncapsulateDATAMessage(payload, shift, data), RETCODE_OK); header = data.header;
            }
            auto message = std::shared_ptr<Message_t>(new Message_t(payload.buffer - shift, payload.buffer_size + shift),
                [bytes](Message_t* p) { delete p; });
            const auto result = fragmented ? processor.InterpretDataFragMessage(message, header, receive) :
                processor.InterpretDataMessage(message, header, receive);
            EXPECT_EQ(result, RETCODE_OK);
            NativeSample received{}; SampleInfo info;
            const auto taken = reader->take_next_sample(&received, info);
            EXPECT_EQ(taken, phase == 0 ? RETCODE_NO_DATA : RETCODE_OK);
            if (phase != 0 && taken == RETCODE_OK) { EXPECT_EQ(received.index, 17 + phase); EXPECT_EQ(received.values[6], 96 + phase); }
        }
        ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    }
}
TEST_F(LoanIntegration, DataDestinationPrefixRejectsForeignParticipantButAcceptsOwnAndUnknown) { RunDestinationFilter(false); }
TEST_F(LoanIntegration, FragmentDestinationPrefixRejectsForeignParticipantButAcceptsOwnAndUnknown) { RunDestinationFilter(true); }

TEST_F(LoanIntegration, ReaderDetachWaitsForInFlightDiscoveryCallbacksForBothReliabilities) {
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    for (auto reliability : {BEST_EFFORT_RELIABILITY_QOS, RELIABLE_RELIABILITY_QOS}) {
        SCOPED_TRACE(reliability);
        // Each case needs a new announcement: explicit remote removal retires
        // the previous GUID from the discovery table.
        ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
        DataWriterQos writerQos; writerQos.data_sharing.kind = OFF_DATA_SHARING_QOS;
        writerQos.reliability.kind = RELIABLE_RELIABILITY_QOS;
        writer = publisher->create_datawriter(topic, writerQos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(writer, nullptr);
        struct Observer : DataReaderListener {
            std::atomic<bool> armed{false};
            std::promise<void> entered, resume;
            std::shared_future<void> released = resume.get_future().share();
            void release() { try { resume.set_value(); } catch (const std::future_error&) {} }
            void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus&) override {
                if (armed.exchange(false)) { entered.set_value(); released.wait(); }
            }
        } observer;
        DataReaderQos qos; qos.data_sharing.kind = OFF_DATA_SHARING_QOS; qos.reliability.kind = reliability;
        auto* reader = subscriber->create_datareader(topic, qos, &observer, SUBSCRIPTION_MATCHED_STATUS);
        ASSERT_NE(reader, nullptr);
        auto* owner = dynamic_cast<DataReaderImpl*>(reader);
        auto endpoint = rtps->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
        ASSERT_NE(endpoint, nullptr);
        struct Threads {
            Observer& observer; std::shared_ptr<Reader> endpoint; DataReaderImpl* owner;
            std::thread remover, detacher;
            ~Threads() {
                observer.release();
                if (remover.joinable()) remover.join();
                if (detacher.joinable()) detacher.join();
                endpoint->SetRelatedDataReader(owner);
                if (owner) owner->set_listener(nullptr, STATUS_MASK_NONE);
            }
        } threads{observer, endpoint, owner, {}, {}};
        InstanceHandleSeq matches;
        const auto deadline = std::chrono::steady_clock::now() + 4s;
        do {
            ASSERT_EQ(reader->get_matched_publications(matches), RETCODE_OK);
            if (std::find(matches.begin(), matches.end(), writer->get_instance_handle()) != matches.end()) break;
            std::this_thread::sleep_for(2ms);
        } while (std::chrono::steady_clock::now() < deadline);
        ASSERT_NE(std::find(matches.begin(), matches.end(), writer->get_instance_handle()), matches.end());
        auto entered = observer.entered.get_future();
        auto detached = std::make_shared<std::promise<void>>(); auto finished = detached->get_future();
        observer.armed = true;
        threads.remover = std::thread([&] { rtps->RemoveRemoteWriter(Conversion::ToGuid(writer->get_instance_handle())); });
        ASSERT_EQ(entered.wait_for(3s), std::future_status::ready);
        threads.detacher = std::thread([endpoint, detached] { endpoint->SetRelatedDataReader(nullptr); detached->set_value(); });
        // Keep the DCPS object alive for the negative run: demonstrate the
        // missing retirement barrier without deliberately freeing under a callback.
        EXPECT_EQ(finished.wait_for(50ms), std::future_status::timeout);
        observer.release();
        ASSERT_EQ(finished.wait_for(3s), std::future_status::ready);
        threads.remover.join(); threads.detacher.join();
        EXPECT_EQ(endpoint->GetRelatedDataReader(), nullptr);
        endpoint->SetRelatedDataReader(owner);
        ASSERT_EQ(reader->set_listener(nullptr, STATUS_MASK_NONE), RETCODE_OK);
        ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
        // The guard only needs the retained RTPS endpoint after actual deletion.
        threads.owner = nullptr;
    }
}

TEST(ReaderDiscoveryBarrier, BuiltinCallbacksDoNotWaitForIngressWhileUserCallbacksDo) {
    struct Probe : StatelessReader {
        using StatelessReader::StatelessReader;
        std::recursive_mutex& ingress() { return dataMutex_; }
    };
    for (bool builtin : {true, false}) {
        GUID_t guid{};
        guid.entityId = builtin ? ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER : EntityId_t{};
        if (!builtin) guid.entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
        DataReaderQos qos;
        TopicKind_t kind; kind.value = NO_KEY;
        Probe reader(qos, guid, kind);
        // SEDP holds ingress before taking the participant Reader table. A
        // match under that table must not take builtin ingress in reverse.
        std::unique_lock<std::recursive_mutex> ingress(reader.ingress());
        auto callback = std::async(std::launch::async, [&] {
            reader.NotifySubscriptionMatched(HANDLE_NIL);
        });
        const auto state = callback.wait_for(50ms);
        ingress.unlock();
        ASSERT_EQ(callback.wait_for(3s), std::future_status::ready);
        callback.get();
        EXPECT_EQ(state, builtin ? std::future_status::ready : std::future_status::timeout)
            << "builtin=" << builtin;
    }
}

// Review regressions deliberately exercise the installed, pre-fix library first.
TEST_F(NativeReaderIntegration, KeyedReviewExpiredAdmissionForNativeAndCdrBothReliabilities) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos wq; wq.data_sharing.kind = OFF_DATA_SHARING_QOS;
    wq.lifespan.duration = {1, 0};
    writer = publisher->create_datawriter(topic, wq, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    for (auto reliability : {BEST_EFFORT_RELIABILITY_QOS, RELIABLE_RELIABILITY_QOS}) {
        for (bool native : {true, false}) {
            SCOPED_TRACE(::testing::Message() << reliability << "/native=" << native);
            DataReaderQos rq; rq.data_sharing.kind = OFF_DATA_SHARING_QOS;
            rq.reliability.kind = reliability;
            auto* r = subscriber->create_datareader(topic, rq, nullptr, STATUS_MASK_NONE);
            ASSERT_NE(r, nullptr);
            auto rp = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
            auto endpoint = rp->GetReader(Conversion::ToGuid(r->get_instance_handle()).entityId);
            const auto guid = Conversion::ToGuid(writer->get_instance_handle());
            auto until = std::chrono::steady_clock::now() + 4s;
            while (!endpoint->MatchedWriterIsMatched(guid) && std::chrono::steady_clock::now() < until)
                std::this_thread::sleep_for(2ms);
            ASSERT_TRUE(endpoint->MatchedWriterIsMatched(guid));
            SerializedBuffer* buffer = nullptr;
            if (!native) {
                buffer = new SerializedBuffer;
                buffer->buffer_size = 68;
                ASSERT_TRUE(MemoryPool::GetInstance()->getBuffer(68, buffer->origin_buffer));
                buffer->buffer = buffer->origin_buffer;
                CdrSerializer cdr(buffer); NativeSample value{};
                ASSERT_EQ(type.serialize_data(&value, &cdr, DEFAULT_ENDIAN), 0);
            }
            auto cache = endpoint->new_change(ALIVE, buffer, HANDLE_NIL, writer->get_instance_handle(), {1,0});
            cache->sourceTimespace = {1,0}; cache->sequenceNumber = {0,1};
            if (native) {
                cache->nativePayload = std::make_shared<NativeSample>();
                cache->nativeType = type.get_native_type_info();
            }
            endpoint->SubmitDataChange(cache);
            SequenceData<NativeSample> values; SampleInfoSeq infos;
            EXPECT_EQ(r->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_NO_DATA);
            EXPECT_EQ(values.length(), 0u);
            auto counters = endpoint->GetReaderCache()->GetSampleCounters(false);
            EXPECT_EQ(counters.sampleNotRead + counters.sampleRead, 0u);
            EXPECT_FALSE(endpoint->GetReaderCache()->IsKeyRegistered(HANDLE_NIL));
            values.unloan(); infos.unloan();
            ASSERT_EQ(subscriber->delete_datareader(r), RETCODE_OK);
        }
    }
}

TEST_F(NativeReaderIntegration, ReviewRejectedSequencePreservesSampleAcrossAllVariants) {
    for (int variant = 0; variant != 10; ++variant) {
        SCOPED_TRACE(variant);
        auto* r = makeReader(); ASSERT_NE(r, nullptr);
        auto delivery = deliver(r);
        SequenceData<uint64_t> wrong; SampleInfoSeq infos;
        auto* c = r->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
        ASSERT_NE(c, nullptr);
        ReturnCode_t result = RETCODE_ERROR;
        switch (variant) {
        case 0: result = r->take(wrong, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
        case 1: result = r->read(wrong, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
        case 2: result = r->take_instance(wrong, infos, 1, handle(), ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
        case 3: result = r->read_instance(wrong, infos, 1, handle(), ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
        case 4: result = r->take_next_instance(wrong, infos, 1, HANDLE_NIL, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
        case 5: result = r->read_next_instance(wrong, infos, 1, HANDLE_NIL, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE); break;
        case 6: result = r->take_w_condition(wrong, infos, 1, c); break;
        case 7: result = r->read_w_condition(wrong, infos, 1, c); break;
        case 8: result = r->take_next_instance_w_condition(wrong, infos, 1, HANDLE_NIL, c); break;
        case 9: result = r->read_next_instance_w_condition(wrong, infos, 1, HANDLE_NIL, c); break;
        }
        EXPECT_EQ(result, RETCODE_BAD_PARAMETER);
        SequenceData<NativeSample> right; SampleInfoSeq rightInfos;
        EXPECT_EQ(r->take(right, rightInfos, 1, NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
        if (right.length()) EXPECT_EQ(right[0].index, 73u);
        right.unloan(); rightInfos.unloan();
        EXPECT_EQ(r->delete_readcondition(c), RETCODE_OK);
        ASSERT_EQ(subscriber->delete_datareader(r), RETCODE_OK);
    }
}

TEST_F(LoanIntegration, ReviewKeepLastCapacityMustSustainWritesOrRejectAtCreation) {
    for (unsigned slots : {1u, 2u}) {
        SCOPED_TRACE(slots);
        DataWriterQos q; q.data_sharing.kind = ON_DATA_SHARING_QOS;
        q.data_sharing.max_samples = slots; q.history.depth = 1;
        q.history.kind = KEEP_LAST_HISTORY_QOS;
        q.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        auto* w = publisher->create_datawriter(topic, q, nullptr, STATUS_MASK_NONE);
        if (!w) { EXPECT_EQ(slots, 1u); continue; }
        for (unsigned i=0; i<4; ++i) {
            NativeSample value{}; value.index=i;
            EXPECT_EQ(w->write(&value, HANDLE_NIL), RETCODE_OK);
        }
        ASSERT_EQ(publisher->delete_datawriter(w), RETCODE_OK);
    }
}

namespace {
std::shared_ptr<CacheChange> completionCache(const std::shared_ptr<Reader>& endpoint,
        NativeSupport& type, DataWriter* writer, uint64_t key, uint32_t sequence, bool native,
        ChangeKind_t kind = ALIVE, Time_t timestamp = TIME_INVALID) {
    auto value = std::make_shared<NativeSample>(); value->index = key; value->values[6] = key + 79;
    InstanceHandle_t handle;
    if (type.MakeKey(value.get(), handle, true)) throw std::runtime_error("key failure");
    SerializedBuffer* buffer = nullptr;
    if (!native && kind == ALIVE) {
        PayloadBufferPtr owner(new SerializedBuffer, PayloadBufferDeleter{MemoryPool::GetInstance()});
        if (!MemoryPool::GetInstance()->getBuffer(68, owner->origin_buffer)) throw std::bad_alloc();
        std::memset(owner->origin_buffer, 0, 4); owner->origin_buffer[1] = DEFAULT_ENDIAN;
        owner->buffer = owner->origin_buffer + 4; owner->buffer_size = 64;
        CdrSerializer cdr(owner.get());
        if (type.serialize_data(value.get(), &cdr, DEFAULT_ENDIAN)) throw std::runtime_error("serialize failure");
        owner->buffer = owner->origin_buffer; owner->buffer_size = owner->writer_position = 68;
        buffer = owner.release();
    }
    auto cache = endpoint->new_change(kind, buffer, handle, writer->get_instance_handle(), timestamp);
    cache->sequenceNumber = {0, sequence};
    if (native && kind == ALIVE) { cache->nativePayload = value; cache->nativeType = type.get_native_type_info(); }
    return cache;
}
bool completionMatch(const std::shared_ptr<Reader>& reader, DataWriter* writer) {
    const auto until = std::chrono::steady_clock::now() + 4s;
    const auto guid = Conversion::ToGuid(writer->get_instance_handle());
    while (!reader->MatchedWriterIsMatched(guid) && std::chrono::steady_clock::now() < until)
        std::this_thread::sleep_for(2ms);
    return reader->MatchedWriterIsMatched(guid);
}
}

TEST_F(LoanIntegration, KeyedFiniteAutopurgeReleasesBudgetAndPreservesOutstandingLoans) {
    for (auto reliability : {BEST_EFFORT_RELIABILITY_QOS, RELIABLE_RELIABILITY_QOS})
    for (bool native : {false, true}) for (int departure = 0; departure != 3; ++departure) {
        SCOPED_TRACE(::testing::Message() << reliability << "/native=" << native << "/departure=" << departure);
        DataReaderQos qos; qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
        qos.reliability.kind = reliability; qos.resource_limits.max_instances = 1;
        qos.reader_data_lifecycle.autopurge_disposed_samples_delay = {0, 30000000};
        qos.reader_data_lifecycle.autopurge_nowriter_samples_delay = {0, 30000000};
        auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(reader, nullptr);
        auto rp = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
        auto endpoint = rp->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
        ASSERT_TRUE(completionMatch(endpoint, writer));
        auto first = completionCache(endpoint, type, writer, 1, 1, native);
        const auto oldHandle = first->instanceHandle;
        ASSERT_EQ(endpoint->AddChanges(first), RETCODE_OK); first.reset();
        SequenceData<NativeSample> held; SampleInfoSeq heldInfo;
        ASSERT_EQ(reader->read(held, heldInfo, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
        if (departure == 2) endpoint->GetReaderCache()->RemoveRemoteWriter(Conversion::ToGuid(writer->get_instance_handle()));
        else ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint, type, writer, 1, 2, native,
            departure ? NOT_ALIVE_UNREGISTERED : NOT_ALIVE_DISPOSED)), RETCODE_OK);
        const auto until = std::chrono::steady_clock::now() + 2s;
        while (endpoint->GetReaderCache()->IsKeyRegistered(oldHandle) && std::chrono::steady_clock::now() < until)
            std::this_thread::sleep_for(2ms);
        EXPECT_FALSE(endpoint->GetReaderCache()->IsKeyRegistered(oldHandle));
        NativeSample key{};
        EXPECT_EQ(reader->get_key_value(&key, oldHandle), RETCODE_BAD_PARAMETER);
        auto counters = endpoint->GetReaderCache()->GetSampleCounters(false);
        EXPECT_EQ(counters.sampleRead + counters.sampleNotRead, 0u);
        EXPECT_EQ(held[0].index, 1u); EXPECT_EQ(held[0].values[6], 80u);
        ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint, type, writer, 2, 3, native)), RETCODE_OK);
        SequenceData<NativeSample> next; SampleInfoSeq nextInfo;
        ASSERT_EQ(reader->take(next, nextInfo, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
        EXPECT_EQ(next[0].index, 2u);
        ASSERT_EQ(reader->return_loan(held, heldInfo), RETCODE_OK);
        ASSERT_EQ(reader->return_loan(next, nextInfo), RETCODE_OK);
        ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    }
}

TEST_F(LoanIntegration, KeyedAutopurgeStateRecheckCancelsOldExpiryAndSharedWriterLoss) {
    DataReaderQos qos; qos.data_sharing.kind = OFF_DATA_SHARING_QOS;
    qos.resource_limits.max_instances = 1;
    qos.reader_data_lifecycle.autopurge_disposed_samples_delay = {0, 100000000};
    qos.reader_data_lifecycle.autopurge_nowriter_samples_delay = {0, 100000000};
    auto* reader = subscriber->create_datareader(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);
    auto endpoint = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
        Conversion::ToGuid(reader->get_instance_handle()).entityId);
    auto* other = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(other, nullptr);
    ASSERT_TRUE(completionMatch(endpoint, writer)); ASSERT_TRUE(completionMatch(endpoint, other));
    auto cache = completionCache(endpoint, type, writer, 1, 1, true);
    const auto handle = cache->instanceHandle;
    ASSERT_EQ(endpoint->AddChanges(cache), RETCODE_OK);
    ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint, type, writer, 1, 2, true, NOT_ALIVE_DISPOSED)), RETCODE_OK);
    ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint, type, writer, 1, 3, true)), RETCODE_OK);
    ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint, type, other, 1, 1, true)), RETCODE_OK);
    endpoint->GetReaderCache()->RemoveRemoteWriter(Conversion::ToGuid(writer->get_instance_handle()));
    std::vector<InstanceHandle_t> removed;
    endpoint->GetReaderCache()->MaintainReader(qos.reader_data_lifecycle, removed, std::chrono::steady_clock::now() + 2s);
    EXPECT_TRUE(removed.empty()); EXPECT_TRUE(endpoint->GetReaderCache()->IsKeyRegistered(handle));
    endpoint->GetReaderCache()->RemoveRemoteWriter(Conversion::ToGuid(other->get_instance_handle()));
    endpoint->GetReaderCache()->MaintainReader(qos.reader_data_lifecycle, removed, std::chrono::steady_clock::now() + 2s);
    EXPECT_EQ(removed.size(), 1u); EXPECT_FALSE(endpoint->GetReaderCache()->IsKeyRegistered(handle));
    // Recreate same key; a new state transition has its own full delay.
    ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint, type, writer, 1, 4, true)), RETCODE_OK);
    endpoint->MaintainHistory();
    EXPECT_TRUE(endpoint->GetReaderCache()->IsKeyRegistered(handle));
}

TEST_F(LoanIntegration, KeyedReaderLifespanRetiresHistoryByCacheIdentityAndKeepsLoanAlive) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos wq; wq.data_sharing.kind = OFF_DATA_SHARING_QOS; wq.lifespan.duration = {0, 50000000};
    writer = publisher->create_datawriter(topic, wq, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    auto* other = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(other, nullptr);
    for (auto reliability : {BEST_EFFORT_RELIABILITY_QOS, RELIABLE_RELIABILITY_QOS}) for (bool native : {false,true}) {
        DataReaderQos rq; rq.data_sharing.kind = OFF_DATA_SHARING_QOS; rq.reliability.kind = reliability;
        rq.history.depth = 8;
        auto* reader = subscriber->create_datareader(topic, rq, nullptr, STATUS_MASK_NONE);
        ASSERT_NE(reader, nullptr);
        auto endpoint = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
            Conversion::ToGuid(reader->get_instance_handle()).entityId);
        ASSERT_TRUE(completionMatch(endpoint, writer)); ASSERT_TRUE(completionMatch(endpoint, other));
        Time_t timestamp; Time_t::now(timestamp);
        auto first = completionCache(endpoint, type, writer, 1, 1, native, ALIVE, timestamp);
        ASSERT_EQ(endpoint->AddChanges(first), RETCODE_OK);
        SequenceData<NativeSample> held; SampleInfoSeq heldInfo;
        ASSERT_EQ(reader->read(held, heldInfo, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
        EXPECT_EQ(heldInfo[0].source_timestamp, timestamp);
        ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint, type, other, 1, 1, native)), RETCODE_OK);
        const auto until = std::chrono::steady_clock::now() + 2s;
        while (endpoint->GetReaderCache()->GetSampleCounters(false).sampleRead && std::chrono::steady_clock::now() < until)
            std::this_thread::sleep_for(2ms);
        EXPECT_EQ(endpoint->GetReaderCache()->GetSampleCounters(false).sampleRead, 0u);
        EXPECT_EQ(held[0].index, 1u);
        SequenceData<NativeSample> next; SampleInfoSeq nextInfo;
        ASSERT_EQ(reader->take(next, nextInfo, 8, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
        ASSERT_EQ(next.length(), 1u);
        EXPECT_EQ(nextInfo[0].publication_handle, other->get_instance_handle());
        held.unloan(); heldInfo.unloan(); next.unloan(); nextInfo.unloan(); first.reset();
        ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    }
}

TEST_F(LoanIntegration, KeyedDeadlineQosChangesOnlyRearmActiveAliveInstances) {
    NativeSample value{}; value.index = 1;
    const auto neverWritten = writer->register_instance(&value); ASSERT_NE(neverWritten, HANDLE_NIL);
    value.index = 2; ASSERT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_OK);
    const auto active = writer->lookup_instance(&value);
    value.index = 3; ASSERT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_OK);
    const auto disposed = writer->lookup_instance(&value); ASSERT_EQ(writer->dispose(&value, disposed), RETCODE_OK);
    DataWriterQos q; ASSERT_EQ(writer->get_qos(q), RETCODE_OK); q.deadline.period = {3600,0};
    ASSERT_EQ(writer->set_qos(q), RETCODE_OK);
    auto history = writerHistory(participant, writer); InstanceHandle_t selected; std::chrono::steady_clock::time_point due;
    ASSERT_TRUE(history->GetNextDeadline(selected, due)); EXPECT_EQ(selected, active);
    q.deadline.period = TIME_INFINITE; ASSERT_EQ(writer->set_qos(q), RETCODE_OK);
    EXPECT_FALSE(history->GetNextDeadline(selected, due));
    q.deadline.period = {7200,0}; ASSERT_EQ(writer->set_qos(q), RETCODE_OK);
    ASSERT_TRUE(history->GetNextDeadline(selected, due)); EXPECT_EQ(selected, active);
    ASSERT_EQ(writer->unregister_instance(nullptr, active), RETCODE_OK);
    EXPECT_FALSE(history->GetNextDeadline(selected, due));
}

TEST(TimerSynchronization, CallbackCanStopUpdateAndReleaseItsOwnTimer) {
    std::shared_ptr<Timer> timer;
    std::promise<void> entered, release, complete;
    auto gate = release.get_future().share();
    timer = TimerManager::CreateTimer({0,1000000}, [&] {
        entered.set_value(); gate.wait();
        timer->Stop(); timer->UpdatePeriod(100); timer.reset(); complete.set_value();
    }, "self.release", TIMER_SINGLE);
    ASSERT_EQ(entered.get_future().wait_for(2s), std::future_status::ready);
    release.set_value();
    ASSERT_EQ(complete.get_future().wait_for(2s), std::future_status::ready);
    EXPECT_FALSE(timer);
}

TEST(TimerSynchronization, ConcurrentPeriodUpdatesStopAndRestartDrainBeforeDestruction) {
    std::atomic<unsigned> callbacks{0};
    auto timer = TimerManager::CreateTimer({0,1000000}, [&] { ++callbacks; }, "timer.stress");
    std::thread updates([&] { for (int i=0;i<300;++i) { timer->UpdatePeriod(1+i%4); timer->GetTriggerTime(); } });
    for (int i=0;i<300;++i) { timer->Stop(); timer->ReStart(); }
    updates.join(); timer->Exit();
    const auto observed = callbacks.load(); timer.reset(); EXPECT_EQ(callbacks.load(), observed);
}

TEST_F(LoanIntegration, ConcurrentWriterQosAndTimersDoNotResurrectDisposedInstances) {
    NativeSample value{}; value.index = 42; ASSERT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_OK);
    std::atomic<unsigned> failures{0};
    std::thread callbacks([&] { for (int i=0;i<200;++i) {
        dynamic_cast<DataWriterImpl*>(writer)->DeadlineTimeout();
        dynamic_cast<DataWriterImpl*>(writer)->LifespanTimeout();
    } });
    for (int i=0;i<200;++i) {
        DataWriterQos qos; if (writer->get_qos(qos) != RETCODE_OK) ++failures;
        qos.deadline.period = i%3 ? Duration_t{3600,0} : Duration_t(TIME_INFINITE);
        qos.lifespan.duration = i%2 ? Duration_t{7200,0} : Duration_t(TIME_INFINITE);
        if (writer->set_qos(qos) != RETCODE_OK) ++failures;
    }
    callbacks.join(); EXPECT_EQ(failures.load(), 0u);
}

TEST_F(NativeReaderIntegration, ReviewCopyAllocationAndDeserializeFailuresPreserveUnreadHistory) {
    for (bool native : {false, true}) {
        auto* reader = makeReader(); ASSERT_NE(reader, nullptr);
        auto endpoint = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
            Conversion::ToGuid(reader->get_instance_handle()).entityId);
        ASSERT_TRUE(completionMatch(endpoint, writer));
        ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint, type, writer, 7, 1, native)), RETCODE_OK);
        struct RefusingSequence : SequenceData<NativeSample> {
            RefusingSequence() : SequenceData<NativeSample>(1) {}
            bool length(uint32_t) override { return false; }
        } refusing;
        SampleInfoSeq infos;
        EXPECT_EQ(reader->take(refusing, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_PRECONDITION_NOT_MET);
        SequenceData<NativeSample> output(1);
        type.failDeserialize = true;
        if (!native) EXPECT_EQ(reader->take(output, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_ERROR);
        type.failDeserialize = false;
        SequenceData<NativeSample> right; SampleInfoSeq rightInfo;
        ASSERT_EQ(reader->take(right, rightInfo, 1, NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
        EXPECT_EQ(right[0].index, 7u);
        right.unloan(); rightInfo.unloan();
        ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    }
}

TEST_F(LoanIntegration, KeyedKeepLastSingleSlotSharesBudgetWithoutOverwritingWriterLoans) {
    for (auto reliability : {BEST_EFFORT_RELIABILITY_QOS, RELIABLE_RELIABILITY_QOS}) {
        DataWriterQos q; q.data_sharing.kind = ON_DATA_SHARING_QOS;
        q.data_sharing.max_samples = 1; q.history.depth = 1; q.resource_limits.max_instances = 2;
        q.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS; q.reliability.kind = reliability;
        auto* w = publisher->create_datawriter(topic, q, nullptr, STATUS_MASK_NONE); ASSERT_NE(w, nullptr);
        SampleLoan held; ASSERT_EQ(w->loan_sample(held), RETCODE_OK);
        static_cast<NativeSample*>(held.data())->index = 99;
        NativeSample value{}; value.index = 1;
        EXPECT_EQ(w->write(&value, HANDLE_NIL), RETCODE_OUT_OF_RESOURCES);
        EXPECT_EQ(static_cast<NativeSample*>(held.data())->index, 99u);
        ASSERT_EQ(w->discard_loan(held), RETCODE_OK);
        ASSERT_EQ(w->write(&value, HANDLE_NIL), RETCODE_OK);
        for (int i=0;i<3;++i) ASSERT_EQ(w->write(&value, HANDLE_NIL), RETCODE_OK);
        value.index = 2; EXPECT_EQ(w->write(&value, HANDLE_NIL), RETCODE_OUT_OF_RESOURCES);
        value.index = 1; EXPECT_EQ(w->write(&value, HANDLE_NIL), RETCODE_OK);
        ASSERT_EQ(publisher->delete_datawriter(w), RETCODE_OK);
    }
}

TEST_F(LoanIntegration, MappingRetryCancellationAndDuplicateDiscoveryLeaveNoRegistration) {
    DataReaderQos q; q.data_sharing.kind = ON_DATA_SHARING_QOS;
    auto* reader = subscriber->create_datareader(topic, q, nullptr, STATUS_MASK_NONE); ASSERT_NE(reader, nullptr);
    auto rp = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto endpoint = rp->GetReader(Conversion::ToGuid(reader->get_instance_handle()).entityId);
    ASSERT_TRUE(completionMatch(endpoint, writer));
    endpoint->StopDataSharing(Conversion::ToGuid(writer->get_instance_handle()));
    DiscoveredWriterData remote; remote.remoteWriterGuid = Conversion::ToGuid(writer->get_instance_handle());
    remote.ddsPublicationData = dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData();
    // A valid advertised session whose directory entry is not available yet.
    remote.ddsPublicationData.data_sharing.session_low ^= 0x542;
    for (int i=0;i<5;++i) ASSERT_TRUE(endpoint->StartDataSharing(endpoint, remote));
    auto begin = std::chrono::steady_clock::now();
    ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    EXPECT_LT(std::chrono::steady_clock::now()-begin, 2s);
    endpoint.reset();
}

TEST_F(LoanIntegration, ConcurrentReaderQosDeadlineAndConditionCreationAreSynchronized) {
    auto* reader = makeReader(); ASSERT_NE(reader, nullptr);
    auto endpoint = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
        Conversion::ToGuid(reader->get_instance_handle()).entityId);
    ASSERT_TRUE(completionMatch(endpoint, writer));
    std::atomic<unsigned> failures{0};
    std::thread callbacks([&] { for (unsigned i=1;i<=200;++i) {
        if (endpoint->AddChanges(completionCache(endpoint, type, writer, 0, i, true)) != RETCODE_OK) ++failures;
        dynamic_cast<DataReaderImpl*>(reader)->DeadlineTimeout(); endpoint->MaintainHistory();
    } });
    for (unsigned i=0;i<200;++i) {
        DataReaderQos q; reader->get_qos(q);
        q.deadline.period = i%3 ? Duration_t{3600,0} : Duration_t(TIME_INFINITE);
        if (reader->set_qos(q) != RETCODE_OK) ++failures;
        auto* condition = reader->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
        if (reader->delete_readcondition(condition) != RETCODE_OK) ++failures;
    }
    callbacks.join(); EXPECT_EQ(failures.load(), 0u);
}

TEST(LoanNotifications, DestroyConditionAttachedToSeveralWaitSetsDetachesEveryEntry) {
    WaitSet first, second, third;
    {
        auto condition = std::make_unique<GuardCondition>();
        ASSERT_EQ(first.attach_condition(*condition), RETCODE_OK);
        ASSERT_EQ(second.attach_condition(*condition), RETCODE_OK);
        ASSERT_EQ(third.attach_condition(*condition), RETCODE_OK);
    }
    ConditionSeq attached;
    for (auto* wait : {&first, &second, &third}) {
        EXPECT_EQ(wait->get_conditions(attached), RETCODE_OK); EXPECT_TRUE(attached.empty());
        EXPECT_EQ(wait->wait(attached, {0,1000000}), RETCODE_TIMEOUT);
    }
}

TEST(ReaderTimestamp, InfoTsPresenceInvalidationEndianAndMissingTimestamp) {
    MessageProcessor processor(nullptr);
    Receive_t received;
    EXPECT_FALSE(received.haveTimestame); EXPECT_EQ(received.timestamp, Time_t(TIME_INVALID));
    for (bool swap : {false,true}) {
        InfoTimestamp_t ts; ts.header.flags = swap ? !DEFAULT_ENDIAN : DEFAULT_ENDIAN;
        ts.header.subMessageLength = sizeof(Time_t); ts.timestamp = {123,456};
        if (swap) { ts.timestamp.sec = int32_t(__builtin_bswap32(123)); ts.timestamp.nanosec = __builtin_bswap32(456); }
        ASSERT_EQ(processor.InterpretINFOTSMessage(reinterpret_cast<char*>(&ts), ts.header, received), RETCODE_OK);
        EXPECT_TRUE(received.haveTimestame); EXPECT_EQ(received.timestamp, (Time_t{123,456}));
    }
    SubMessageHeader_t invalid; invalid.flags = SubMessageFlag::INVALIDATE_FLAG; invalid.subMessageLength = 0;
    ASSERT_EQ(processor.InterpretINFOTSMessage(reinterpret_cast<char*>(&invalid), invalid, received), RETCODE_OK);
    EXPECT_FALSE(received.haveTimestame); EXPECT_EQ(received.timestamp, Time_t(TIME_INVALID));
    InfoTimestamp_t truncated; truncated.header.flags = DEFAULT_ENDIAN; truncated.header.subMessageLength = 4;
    EXPECT_EQ(processor.InterpretINFOTSMessage(reinterpret_cast<char*>(&truncated), truncated.header, received), RETCODE_BAD_PARAMETER);
}

TEST_F(LoanIntegration, LifespanRecheckedBeforeTimeBasedFilterDelayedCommit) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    DataWriterQos wq; wq.lifespan.duration = {0, 30000000}; wq.data_sharing.kind = OFF_DATA_SHARING_QOS;
    writer = publisher->create_datawriter(topic, wq, nullptr, STATUS_MASK_NONE); ASSERT_NE(writer, nullptr);
    for (bool native : {true,false}) {
        DataReaderQos q; q.reliability.kind = RELIABLE_RELIABILITY_QOS;
        q.data_sharing.kind = OFF_DATA_SHARING_QOS; q.time_based_filter.minimum_separation = {0,60000000};
        auto* reader = subscriber->create_datareader(topic, q, nullptr, STATUS_MASK_NONE); ASSERT_NE(reader, nullptr);
        auto endpoint = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
            Conversion::ToGuid(reader->get_instance_handle()).entityId);
        ASSERT_TRUE(completionMatch(endpoint, writer));
        ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint, type, writer, 0, 1, native)), RETCODE_OK);
        auto pending = completionCache(endpoint, type, writer, 0, 2, native);
        ASSERT_EQ(endpoint->AddChanges(pending), RETCODE_TIME_BASED_FILTER);
        const auto until = std::chrono::steady_clock::now()+2s;
        while (std::chrono::system_clock::now() < pending->expiresAt && std::chrono::steady_clock::now() < until)
            std::this_thread::sleep_for(2ms);
        endpoint->MaintainHistory();
        SequenceData<NativeSample> values; SampleInfoSeq infos;
        EXPECT_EQ(reader->take(values, infos, 8, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_NO_DATA);
        ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    }
}

TEST_F(NativeReaderIntegration, KeyedFinitePurgeCannotReuseAnOutstandingSharedReaderLoanSlot) {
    DataReaderQos q; q.data_sharing.kind = OFF_DATA_SHARING_QOS; q.resource_limits.max_instances = 1;
    q.reader_data_lifecycle.autopurge_disposed_samples_delay = {0,20000000};
    auto* reader = subscriber->create_datareader(topic, q, nullptr, STATUS_MASK_NONE); ASSERT_NE(reader, nullptr);
    auto endpoint = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
        Conversion::ToGuid(reader->get_instance_handle()).entityId);
    auto delivery = deliver(reader);
    SequenceData<NativeSample> held; SampleInfoSeq infos;
    ASSERT_EQ(reader->read(held, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_OK);
    ASSERT_TRUE(held.has_loan()); EXPECT_EQ(&held[0], delivery.address);
    auto dispose = endpoint->new_change(NOT_ALIVE_DISPOSED, nullptr, handle(), writer->get_instance_handle(), TIME_INVALID);
    dispose->sequenceNumber = {0,2}; ASSERT_EQ(endpoint->AddChanges(dispose), RETCODE_OK);
    const auto until = std::chrono::steady_clock::now()+2s;
    while (endpoint->GetReaderCache()->IsKeyRegistered(handle()) && std::chrono::steady_clock::now()<until)
        std::this_thread::sleep_for(2ms);
    EXPECT_FALSE(endpoint->GetReaderCache()->IsKeyRegistered(handle()));
    EXPECT_EQ(freeSlots(delivery), 0u); EXPECT_EQ(held[0].index, 73u);
    SharedSampleToken token; char* buffer = nullptr;
    EXPECT_EQ(delivery.pool->loan(sizeof(NativeSample), token, buffer), SharedPoolResult::EXHAUSTED);
    ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
    EXPECT_EQ(held[0].values[6], 7306u);
    held.unloan(); EXPECT_EQ(freeSlots(delivery), 0u);
    infos.unloan(); EXPECT_EQ(freeSlots(delivery), 1u);
}

TEST_F(LoanIntegration, KeyedDeadlineListenerCanQueryStatusAndChangeItsOwnQos) {
    struct Listener : DataWriterListener {
        bool called = false;
        void on_offered_deadline_missed(DataWriter* writer, const OfferedDeadlineMissedStatus& status) override {
            OfferedDeadlineMissedStatus queried;
            EXPECT_EQ(writer->get_offered_deadline_missed_status(queried), RETCODE_OK);
            EXPECT_EQ(queried.total_count, status.total_count);
            DataWriterQos q; EXPECT_EQ(writer->get_qos(q), RETCODE_OK); q.deadline.period = TIME_INFINITE;
            EXPECT_EQ(writer->set_qos(q), RETCODE_OK);
            EXPECT_EQ(writer->unregister_instance(nullptr, status.last_instance_handle), RETCODE_OK);
            called = true;
        }
    } listener;
    ASSERT_EQ(writer->set_listener(&listener, OFFERED_DEADLINE_MISSED_STATUS), RETCODE_OK);
    DataWriterQos q; writer->get_qos(q); q.deadline.period = {3600,0}; ASSERT_EQ(writer->set_qos(q), RETCODE_OK);
    NativeSample value{}; value.index = 9; ASSERT_EQ(writer->write(&value, HANDLE_NIL), RETCODE_OK);
    const auto handle = writer->lookup_instance(&value);
    ASSERT_TRUE(writerHistory(participant, writer)->SetNextDeadline(handle, std::chrono::steady_clock::now()-1s));
    dynamic_cast<DataWriterImpl*>(writer)->DeadlineTimeout(); EXPECT_TRUE(listener.called);
    writer->set_listener(nullptr, STATUS_MASK_NONE);
}

TEST_F(LoanIntegration, ReaderDeadlineListenerCanQueryStatusAndChangeItsOwnQos) {
    struct Listener : DataReaderListener {
        bool called = false;
        void on_requested_deadline_missed(DataReader* reader, const RequestedDeadlineMissedStatus& status) override {
            RequestedDeadlineMissedStatus queried;
            EXPECT_EQ(reader->get_requested_deadline_missed_status(queried), RETCODE_OK);
            EXPECT_EQ(queried.total_count, status.total_count);
            DataReaderQos q; reader->get_qos(q); q.deadline.period = TIME_INFINITE;
            EXPECT_EQ(reader->set_qos(q), RETCODE_OK); called = true;
        }
    } listener;
    DataReaderQos q; q.data_sharing.kind = OFF_DATA_SHARING_QOS;
    // Matching requires the Writer's offered deadline to be at least as strict.
    DataWriterQos wq; writer->get_qos(wq); wq.deadline.period = {3600,0}; ASSERT_EQ(writer->set_qos(wq), RETCODE_OK);
    q.deadline.period = {7200,0};
    auto* reader = subscriber->create_datareader(topic, q, &listener, REQUESTED_DEADLINE_MISSED_STATUS); ASSERT_NE(reader, nullptr);
    auto endpoint = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
        Conversion::ToGuid(reader->get_instance_handle()).entityId);
    ASSERT_TRUE(completionMatch(endpoint, writer));
    auto cache = completionCache(endpoint, type, writer, 0, 1, true); ASSERT_EQ(endpoint->AddChanges(cache), RETCODE_OK);
    dynamic_cast<DataReaderImpl*>(reader)->SetLastinstanceHandle(cache);
    q.deadline.period = {7200,0}; ASSERT_EQ(reader->set_qos(q), RETCODE_OK);
    ASSERT_TRUE(endpoint->GetReaderCache()->SetNextDeadline(cache->instanceHandle, std::chrono::steady_clock::now()-1s));
    dynamic_cast<DataReaderImpl*>(reader)->DeadlineTimeout(); EXPECT_TRUE(listener.called);
    reader->set_listener(nullptr, STATUS_MASK_NONE);
}

TEST_F(LoanIntegration, MappingNewSessionCancelsOldRetryAndDuplicateStartKeepsRegistration) {
    auto* reader = makeReader(); ASSERT_NE(reader, nullptr);
    auto endpoint = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
        Conversion::ToGuid(reader->get_instance_handle()).entityId);
    ASSERT_TRUE(completionMatch(endpoint, writer));
    DiscoveredWriterData current; current.remoteWriterGuid = Conversion::ToGuid(writer->get_instance_handle());
    current.ddsPublicationData = dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData();
    auto previous = current; previous.ddsPublicationData.data_sharing.session_low ^= 0x721;
    endpoint->StopDataSharing(current.remoteWriterGuid);
    ASSERT_TRUE(endpoint->StartDataSharing(endpoint, previous));
    EXPECT_TRUE(endpoint->DataSharingSessionChanged(current));
    ASSERT_TRUE(endpoint->StartDataSharing(endpoint, current));
    EXPECT_FALSE(endpoint->DataSharingSessionChanged(current));
    for (int i=0;i<5;++i) ASSERT_TRUE(endpoint->StartDataSharing(endpoint, current));
    type.failSerialize = true; type.failDeserialize = true;
    SampleLoan loan;
    const auto until = std::chrono::steady_clock::now()+2s;
    // Registration is asynchronous; wait for its observable pool statistic.
    const auto& info = current.ddsPublicationData.data_sharing;
    SharedPoolDescriptor descriptor;
    descriptor.writer = current.remoteWriterGuid; descriptor.session = {info.session_high, info.session_low};
    descriptor.path = SharedPayloadPool::pathFor(descriptor.writer, descriptor.session, "/dev/shm");
    descriptor.config.payload_size = info.type.size; descriptor.config.alignment = std::max(64u, info.type.alignment);
    descriptor.config.slots = info.slots; descriptor.config.max_readers = info.max_readers;
    descriptor.segment_bytes = info.segment_bytes;
    auto pool = SharedPayloadPool::open(descriptor);
    SharedPoolStats stats;
    auto rtpsWriter = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetWriter(
        current.remoteWriterGuid.entityId);
    ASSERT_TRUE(rtpsWriter);
    auto selected = [&] { return rtpsWriter->SelectDataSharingReaders(pool->registeredReaders()).size(); };
    do { pool->statistics(stats); if (stats.readers == 1 && selected() == 1) break; std::this_thread::sleep_for(2ms); }
    while (std::chrono::steady_clock::now()<until);
    ASSERT_EQ(stats.readers, 1u);
    ASSERT_EQ(selected(), 1u); // Both discovery directions precede the no-CDR phase.
    ASSERT_EQ(writer->loan_sample(loan), RETCODE_OK);
    initializeNativeSample(*static_cast<NativeSample*>(loan.data()), 72);
    ASSERT_EQ(writer->write(loan, HANDLE_NIL), RETCODE_OK);
    SequenceData<NativeSample> data; SampleInfoSeq infos;
    ReturnCode_t result;
    do { result=reader->take(data, infos, 8, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
        if (result==RETCODE_NO_DATA) std::this_thread::sleep_for(2ms); }
    while (result==RETCODE_NO_DATA && std::chrono::steady_clock::now()<until);
    ASSERT_EQ(result, RETCODE_OK); ASSERT_EQ(data.length(), 1u); EXPECT_EQ(data[0].index, 72u);
    data.unloan(); infos.unloan();
    ASSERT_EQ(reader->take(data, infos, 8, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), RETCODE_NO_DATA);
}

TEST(HistoryAccounting, RemovalPathsReleaseSamplesAndCountersTogether) {
    HistoryQosPolicy history; ResourceLimitsQosPolicy limits;
    for (int method=0;method<3;++method) {
        HistoryCache cache(history, limits, TopicKind_t{WITH_KEY});
        InstanceHandle_t key; key.keyHash.value[0]=1;
        for (unsigned sn=1;sn<=2;++sn) {
            auto change=std::make_shared<CacheChange>(); change->instanceHandle=key; change->sequenceNumber={0,sn};
            ASSERT_TRUE(cache.AddChange(change));
        }
        ASSERT_EQ(cache.GetSampleCounters(false).sampleNotRead, 2u);
        if (method==0) { ASSERT_TRUE(cache.RemoveChange({0,1})); ASSERT_TRUE(cache.RemoveChange({0,2})); }
        if (method==1) cache.RemoveChangeByInstance(key);
        if (method==2) cache.DeleteKeyHistory(key);
        EXPECT_EQ(cache.GetSampleCounters(false).sampleNotRead, 0u);
        EXPECT_EQ(cache.GetChange({0,1}), nullptr); EXPECT_EQ(cache.GetChange({0,2}), nullptr);
    }
}

TEST_F(LoanIntegration, TimeBasedFilterListenerAllowsAnotherThreadToChangeQos) {
    struct Listener : DataReaderListener {
        std::promise<void> done;
        std::thread update;
        void on_data_available(DataReader* reader) override {
            std::promise<ReturnCode_t> changed;
            auto completed=changed.get_future();
            update=std::thread([reader, result=std::move(changed)]() mutable {
                DataReaderQos q; reader->get_qos(q); q.time_based_filter.minimum_separation=TIME_ZERO;
                result.set_value(reader->set_qos(q));
            });
            EXPECT_EQ(completed.wait_for(500ms), std::future_status::ready);
            if (completed.wait_for(0ms)==std::future_status::ready) EXPECT_EQ(completed.get(), RETCODE_OK);
            done.set_value();
        }
        ~Listener() { if (update.joinable()) update.join(); }
    } listener;
    DataReaderQos q; q.data_sharing.kind=OFF_DATA_SHARING_QOS;
    q.reliability.kind=RELIABLE_RELIABILITY_QOS; q.time_based_filter.minimum_separation={0,20000000};
    auto* reader=subscriber->create_datareader(topic,q,&listener,DATA_AVAILABLE_STATUS); ASSERT_NE(reader,nullptr);
    auto endpoint=dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant()->GetReader(
        Conversion::ToGuid(reader->get_instance_handle()).entityId);
    ASSERT_TRUE(completionMatch(endpoint,writer));
    ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint,type,writer,0,1,true)),RETCODE_OK);
    ASSERT_EQ(endpoint->AddChanges(completionCache(endpoint,type,writer,0,2,true)),RETCODE_TIME_BASED_FILTER);
    ASSERT_EQ(listener.done.get_future().wait_for(2s),std::future_status::ready);
    listener.update.join();
    reader->set_listener(nullptr,STATUS_MASK_NONE);
    ASSERT_EQ(subscriber->delete_datareader(reader),RETCODE_OK);
}

TEST_F(LoanIntegration, KeepAllSharedExhaustionPreservesLoansAndAutoFallbackRecovers) {
    for (auto mode : {ON_DATA_SHARING_QOS,AUTO_DATA_SHARING_QOS})
    for (auto reliability : {RELIABLE_RELIABILITY_QOS,BEST_EFFORT_RELIABILITY_QOS}) {
        DataWriterQos q; q.data_sharing.kind=mode; q.data_sharing.max_samples=1;
        q.history.kind=KEEP_ALL_HISTORY_QOS; q.reliability.kind=reliability;
        q.durability.kind=TRANSIENT_LOCAL_DURABILITY_QOS;
        auto* w=publisher->create_datawriter(topic,q,nullptr,STATUS_MASK_NONE); ASSERT_NE(w,nullptr);
        SampleLoan held; ASSERT_EQ(w->loan_sample(held),RETCODE_OK);
        static_cast<NativeSample*>(held.data())->index=88;
        NativeSample value{}; const int before=type.serializeCalls;
        EXPECT_EQ(w->write(&value,HANDLE_NIL), mode==ON_DATA_SHARING_QOS ? RETCODE_OUT_OF_RESOURCES : RETCODE_OK);
        EXPECT_EQ(type.serializeCalls-before, mode==ON_DATA_SHARING_QOS ? 0 : 1);
        EXPECT_EQ(static_cast<NativeSample*>(held.data())->index,88u);
        ASSERT_EQ(w->discard_loan(held),RETCODE_OK);
        const int after=type.serializeCalls;
        ASSERT_EQ(w->write(&value,HANDLE_NIL),RETCODE_OK);
        EXPECT_EQ(type.serializeCalls,after);
        // Reliable KEEP_ALL retains its native history; no implicit eviction.
        if (reliability==RELIABLE_RELIABILITY_QOS && mode==ON_DATA_SHARING_QOS)
            EXPECT_EQ(w->write(&value,HANDLE_NIL),RETCODE_OUT_OF_RESOURCES);
        ASSERT_EQ(publisher->delete_datawriter(w),RETCODE_OK);
    }
}

#if defined(__linux__)
TEST_F(LoanIntegration, NativeDdsWritesAndLoansProgressWhileThePoolControlMutexIsHeld) {
    ASSERT_EQ(publisher->delete_datawriter(writer), RETCODE_OK);
    writer = nullptr;
    DataWriterQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.data_sharing.max_samples = 3;
    qos.data_sharing.max_readers = 2;
    qos.history.depth = 1;
    qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
    writer = publisher->create_datawriter(topic, qos, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(writer, nullptr);
    DataReaderQos rq;
    rq.data_sharing.kind = ON_DATA_SHARING_QOS;
    rq.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
    auto* reader = subscriber->create_datareader(topic, rq, nullptr, STATUS_MASK_NONE);
    ASSERT_NE(reader, nullptr);
    auto rtps = dynamic_cast<DomainParticipantImpl*>(participant)->GetRelatedParticipant();
    auto endpoint = rtps->GetWriter(Conversion::ToGuid(writer->get_instance_handle()).entityId);
    const auto& info = dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData().data_sharing;
    SharedPoolDescriptor d;
    d.writer = endpoint->GetGuid();
    d.session = {info.session_high, info.session_low};
    d.path = SharedPayloadPool::pathFor(d.writer, d.session, qos.data_sharing.shared_memory_directory);
    d.config.payload_size = info.type.size;
    d.config.alignment = std::max(64u, info.type.alignment);
    d.config.slots = info.slots;
    d.config.max_readers = info.max_readers;
    d.config.max_segment_bytes = qos.data_sharing.max_segment_bytes;
    d.segment_bytes = info.segment_bytes;
    auto pool = SharedPayloadPool::open(d);
    auto until = std::chrono::steady_clock::now() + 4s;
    while (endpoint->SelectDataSharingReaders(pool->registeredReaders()).size() != 1 &&
            std::chrono::steady_clock::now() < until) std::this_thread::sleep_for(2ms);
    ASSERT_EQ(endpoint->SelectDataSharingReaders(pool->registeredReaders()).size(), 1u);
    // Failed CDR paths cannot make this test appear to pass via transport
    // fallback: every application sample must cross the native backend.
    type.failSerialize = type.failDeserialize = true;
    const int fd = open(d.path.c_str(), O_RDWR | O_CLOEXEC);
    ASSERT_GE(fd, 0);
    auto* address = mmap(nullptr, sizeof(shared_pool_detail::Header),
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    ASSERT_NE(address, MAP_FAILED);
    auto* header = static_cast<shared_pool_detail::Header*>(address);
    struct MappingCleanup {
        void* address;
        ~MappingCleanup() { munmap(address, sizeof(shared_pool_detail::Header)); }
    } mappingCleanup{address};
    ASSERT_EQ(pthread_mutex_lock(&header->mutex), 0);
    // No fatal assertion is permitted while the lock is held: on a failure we
    // release it before waiting for the worker, so the old implementation
    // reports a failed progress assertion instead of hanging the test runner.
    auto exercise = std::async(std::launch::async, [&]() -> int {
        SequenceData<NativeSample> held;
        SampleInfoSeq heldInfo;
        for (uint64_t sequence = 1; sequence <= 8; ++sequence) {
            SampleLoan writable;
            if (writer->loan_sample(writable) != RETCODE_OK) return 10;
            auto* sample = static_cast<NativeSample*>(writable.data());
            sample->index = sequence;
            for (uint64_t i = 0; i < 7; ++i) sample->values[i] = sequence * 100 + i;
            if (writer->write(writable, HANDLE_NIL) != RETCODE_OK || writable) return 11;
            SequenceData<NativeSample> values;
            SampleInfoSeq infos;
            ReturnCode_t received;
            const auto deadline = std::chrono::steady_clock::now() + 1500ms;
            do {
                received = reader->take(values, infos, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
                if (received != RETCODE_NO_DATA) break;
                std::this_thread::sleep_for(1ms);
            } while (std::chrono::steady_clock::now() < deadline);
            if (received != RETCODE_OK || values.length() != 1 || !values.has_loan() ||
                !infos[0].valid_data || values[0].index != sequence) return 12;
            for (uint64_t i = 0; i < 7; ++i)
                if (values[0].values[i] != sequence * 100 + i) return 13;
            if (sequence == 1) {
                held = std::move(values);
                heldInfo = std::move(infos);
            } else {
                if (!held.has_loan() || held[0].index != 1) return 14;
                for (uint64_t i = 0; i < 7; ++i) if (held[0].values[i] != 100 + i) return 15;
                if (reader->return_loan(values, infos) != RETCODE_OK) return 16;
            }
        }
        return reader->return_loan(held, heldInfo) == RETCODE_OK ? 0 : 17;
    });
    const auto readyWhileLocked = exercise.wait_for(3s);
    const int unlocked = pthread_mutex_unlock(&header->mutex);
    EXPECT_EQ(unlocked, 0);
    EXPECT_EQ(readyWhileLocked, std::future_status::ready)
        << "Native DDS write/Reader loan waited for a control-plane lock owner";
    EXPECT_EQ(exercise.get(), 0);
    EXPECT_EQ(type.serializeCalls, 0);
    EXPECT_EQ(type.deserializeCalls, 0);
    EXPECT_EQ(type.copyCalls, 0);
    ASSERT_EQ(subscriber->delete_datareader(reader), RETCODE_OK);
}
#endif
