// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include "demo.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <spawn.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <thread>
#include <unistd.h>
extern char** environ;
using namespace datasharing_demo;
using namespace std::chrono_literals;
namespace {
uint64_t nowNs() {
    return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
long peakRss() { rusage r{}; getrusage(RUSAGE_SELF, &r); return r.ru_maxrss; }
template<size_t Bytes> struct alignas(64) Value { uint64_t index, sent; uint64_t fields[(Bytes - 16) / 8]; };
template<size_t Bytes> class Support : public NativeTypeSupport<Value<Bytes>> {
public:
    Support() : NativeTypeSupport<Value<Bytes>>("travodds.benchmark.v1.u64-array." + std::to_string(Bytes) + ".align64") {}
    std::atomic<unsigned> serialized{0}, deserialized{0};
    bool blockSerialize = false;
    void* create_data() override { return new Value<Bytes>{}; }
    void delete_data(void* p) override { delete static_cast<Value<Bytes>*>(p); }
    int32_t copy_data(void* a, void* b) override { *static_cast<Value<Bytes>*>(a) = *static_cast<Value<Bytes>*>(b); return 0; }
    uint32_t get_serialized_data_size(void*, uint32_t) override { return Bytes; }
    uint32_t get_max_serialized_data_size(void*, uint32_t) override { return Bytes; }
    int32_t serialize_data(void* p, CdrSerializer* cdr, int32_t) override {
        ++serialized; if (blockSerialize) return -1;
        auto* v = static_cast<Value<Bytes>*>(p);
        return cdr->serializeBaseType(v->index) && cdr->serializeBaseType(v->sent) &&
            cdr->serializeBaseTypeArray(v->fields, (Bytes - 16) / 8) ? 0 : -1;
    }
    int32_t deserialize_data(void* p, CdrDeserializer* cdr, int32_t) override {
        ++deserialized; auto* v = static_cast<Value<Bytes>*>(p);
        return cdr->deserializeBaseType(v->index) && cdr->deserializeBaseType(v->sent) &&
            cdr->deserializeBaseTypeArray(v->fields, (Bytes - 16) / 8) ? 0 : -1;
    }
    TypeObject* get_typeobject() override { return nullptr; }
    ReturnCode_t get_instancehandle(void*, CdrSerializer*, InstanceHandle_t& h, bool) override { h = HANDLE_NIL; return RETCODE_OK; }
    int32_t MakeKey(const void*, InstanceHandle_t& h, bool) override { h = HANDLE_NIL; return 0; }
    bool has_key() override { return false; }
    const char* get_typename() override { return "DataSharingBenchmark"; }
};
template<size_t Bytes> void fill(Value<Bytes>& v, uint64_t index) {
    v.index = index;
    for (size_t i = 0; i < (Bytes - 16) / 8; ++i) v.fields[i] = index * 1009 + i;
    v.sent = nowNs();
}
template<size_t Bytes> void validate(const Value<Bytes>& v, uint64_t index) {
    require(v.index == index, "Duplicate/missing/out-of-order sample");
    for (size_t i = 0; i < (Bytes - 16) / 8; ++i)
        require(v.fields[i] == index * 1009 + i, "Payload corruption");
}
struct Child {
    pid_t pid = -1;
    ~Child() { if (pid > 0) { kill(pid, SIGKILL); waitpid(pid, nullptr, 0); } }
    void finish() {
        int status = 0;
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        while (std::chrono::steady_clock::now() < deadline) {
            if (waitpid(pid, &status, WNOHANG) == pid) {
                pid = -1; require(WIFEXITED(status) && WEXITSTATUS(status) == 0, "Reader failed"); return;
            }
            std::this_thread::sleep_for(2ms);
        }
        throw std::runtime_error("Reader exit timeout");
    }
};
struct Directory {
    std::string path;
    Directory() { char p[] = "/dev/shm/travodds-bench-XXXXXX"; require(mkdtemp(p), "mkdtemp failed"); path = p; }
    ~Directory() { rmdir(path.c_str()); }
};
template<size_t Bytes> int run(int argc, char** argv) {
    const std::string mode = argv[1];
    const bool sharing = mode != "rtps", useLoan = mode == "loan";
    const unsigned count = unsigned(std::stoul(argv[3])), warm = 32, batch = 8;
    require(count >= 8 && count <= 4096 && count % batch == 0, "Count must be 8..4096 and divisible by 8");
    const bool child = argc == 8;
    Directory directory;
    const std::string path = child ? argv[6] : directory.path;
    const std::string topicName = child ? argv[7] : "benchmark." + std::to_string(getpid());
    Support<Bytes> type;
    struct ParticipantGuard {
        DomainParticipant* p = nullptr;
        ~ParticipantGuard() { if (p) { p->delete_contained_entities(); TheParticipantFactory->delete_participant(p); } }
    } owner;
    DomainParticipantQos pq; pq.transport.removeTransport(TRANSPORT_SHM_QOS);
    owner.p = TheParticipantFactory->create_participant(147, pq, nullptr, STATUS_MASK_NONE);
    require(owner.p, "create participant failed");
    check(owner.p->registe_type(type.get_typename(), &type), "register type");
    auto* topic = owner.p->create_topic(topicName.c_str(), type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    require(topic, "create topic failed");
    const auto sharingKind = sharing ? ON_DATA_SHARING_QOS : OFF_DATA_SHARING_QOS;
    if (child) {
        Peer peer(std::stoi(argv[4])); close(std::stoi(argv[5]));
        auto* sub = owner.p->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        require(sub, "create subscriber failed");
        DataReaderQos q; q.data_sharing.kind = sharingKind; q.data_sharing.shared_memory_directory = path;
        q.reliability.kind = RELIABLE_RELIABILITY_QOS; q.history.depth = 16;
        auto* reader = sub->create_datareader(topic, q, nullptr, STATUS_MASK_NONE); require(reader, "create Reader failed");
        const auto deadline = std::chrono::steady_clock::now() + 8s;
        InstanceHandleSeq matches;
        do { check(reader->get_matched_publications(matches), "Reader match"); if (!matches.empty()) break; std::this_thread::sleep_for(2ms); }
        while (std::chrono::steady_clock::now() < deadline);
        require(matches.size() == 1, "Writer did not match"); peer.send('R');
        auto take = [&](uint64_t index, std::vector<uint64_t>* latencies) {
            SequenceData<Value<Bytes>> values; SampleInfoSeq info;
            const auto limit = std::chrono::steady_clock::now() + 8s;
            ReturnCode_t result;
            do {
                if (!reader->get_unread_count()) { result = RETCODE_NO_DATA; std::this_thread::sleep_for(50us); continue; }
                result = reader->take(values, info, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
                if (result != RETCODE_NO_DATA) break; std::this_thread::sleep_for(50us); }
            while (std::chrono::steady_clock::now() < limit);
            check(result, "take sample"); require(values.length() == 1 && info[0].valid_data, "Invalid sample");
            require(values.has_loan() == sharing, "Unexpected payload path");
            const auto received = nowNs(); validate(values[0], index);
            require(received >= values[0].sent, "Non-monotonic sample clock");
            if (latencies) latencies->push_back(received - values[0].sent);
            check(reader->return_loan(values, info), "return Loan");
        };
        take(0, nullptr); peer.send('P'); peer.expect('S'); type.serialized = 0; type.deserialized = 0; peer.send('A');
        std::vector<uint64_t> latencies; latencies.reserve(count);
        for (unsigned i = 1; i <= warm + count; ++i) {
            take(i, i > warm ? &latencies : nullptr);
            if (i % batch == 0) peer.send('B');
        }
        require(type.deserialized == (sharing ? 0 : warm + count), "Incorrect deserializer count");
        std::sort(latencies.begin(), latencies.end());
        auto quantile = [&](double q) { return double(latencies[size_t(std::ceil(q * count)) - 1]) / 1000.0; };
        std::cout << "BENCH_RESULT {\"role\":\"reader\",\"mode\":\"" << mode << "\",\"bytes\":" << Bytes
            << ",\"samples\":" << count << ",\"warmup\":" << warm << ",\"batch\":" << batch
            << ",\"pid\":" << getpid() << ",\"deserializations\":" << type.deserialized
            << ",\"latency_p50_us\":" << quantile(.5) << ",\"latency_p95_us\":" << quantile(.95)
            << ",\"latency_p99_us\":" << quantile(.99) << ",\"peak_rss_kib\":" << peakRss() << "}" << std::endl;
        peer.send('F'); peer.expect('D'); return 0;
    }
    auto* pub = owner.p->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE); require(pub, "create publisher failed");
    DataWriterQos q; q.data_sharing.kind = sharingKind; q.data_sharing.shared_memory_directory = path;
    q.data_sharing.max_samples = 64; q.data_sharing.max_readers = 2; q.data_sharing.max_segment_bytes = 16 * 1024 * 1024;
    q.reliability.kind = RELIABLE_RELIABILITY_QOS; q.history.depth = 16;
    auto* writer = pub->create_datawriter(topic, q, nullptr, STATUS_MASK_NONE); require(writer, "create Writer failed");
    int sockets[2]; require(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "socketpair failed");
    Peer peer(sockets[0]); Child process;
    std::vector<std::string> arguments{"/proc/self/exe", mode, std::to_string(Bytes), std::to_string(count),
        std::to_string(sockets[1]), std::to_string(sockets[0]), path, topicName};
    std::vector<char*> args; for (auto& a : arguments) args.push_back(a.data()); args.push_back(nullptr);
    const auto spawned = posix_spawn(&process.pid, "/proc/self/exe", nullptr, nullptr, args.data(), environ);
    close(sockets[1]); require(spawned == 0, "spawn failed"); peer.expect('R');
    InstanceHandleSeq matches; const auto limit = std::chrono::steady_clock::now() + 8s;
    do { check(writer->get_matched_subscriptions(matches), "Writer match"); if (!matches.empty()) break; std::this_thread::sleep_for(2ms); }
    while (std::chrono::steady_clock::now() < limit);
    require(matches.size() == 1, "Reader did not match");
    type.blockSerialize = sharing;
    if (sharing) {
        SampleLoan probe; check(writer->loan_sample(probe), "probe Loan"); fill(*static_cast<Value<Bytes>*>(probe.data()), 0);
        ReturnCode_t status;
        do { status = writer->write(probe, HANDLE_NIL); if (status == RETCODE_OK) break; std::this_thread::sleep_for(2ms); }
        while (std::chrono::steady_clock::now() < limit);
        check(status, "native probe");
    } else { Value<Bytes> probe{}; fill(probe, 0); check(writer->write(&probe, HANDLE_NIL), "RTPS probe"); }
    peer.expect('P'); peer.send('S'); peer.expect('A'); type.serialized = 0; type.deserialized = 0;
    uint64_t started = 0;
    for (unsigned i = 1; i <= warm + count; ++i) {
        if (i == warm + 1) started = nowNs();
        if (useLoan) { SampleLoan loan; check(writer->loan_sample(loan), "Loan"); fill(*static_cast<Value<Bytes>*>(loan.data()), i); check(writer->write(loan, HANDLE_NIL), "write Loan"); }
        else { Value<Bytes> sample{}; fill(sample, i); check(writer->write(&sample, HANDLE_NIL), "write ordinary"); }
        if (i % batch == 0) peer.expect('B');
    }
    const double seconds = double(nowNs() - started) / 1e9;
    peer.expect('F'); require(type.serialized == (sharing ? 0 : warm + count), "Incorrect serializer count");
    check(pub->delete_datawriter(writer), "delete Writer"); peer.send('D'); process.finish();
    require(std::filesystem::is_empty(path), "Benchmark left shared segment");
    std::cout << "BENCH_RESULT {\"role\":\"writer\",\"mode\":\"" << mode << "\",\"bytes\":" << Bytes
        << ",\"samples\":" << count << ",\"warmup\":" << warm << ",\"batch\":" << batch
        << ",\"pid\":" << getpid() << ",\"serializations\":" << type.serialized
        << ",\"seconds\":" << seconds << ",\"samples_per_second\":" << count / seconds
        << ",\"mib_per_second\":" << double(count) * Bytes / seconds / (1024 * 1024)
        << ",\"peak_rss_kib\":" << peakRss() << ",\"directory\":\"" << path << "\"}" << std::endl;
    return 0;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 4 || argc == 8, "Usage: benchmark rtps|ordinary|loan 64|4096|65536 COUNT");
        const std::string mode = argv[1]; require(mode == "rtps" || mode == "ordinary" || mode == "loan", "Unknown mode");
        const auto bytes = std::stoul(argv[2]);
        if (bytes == 64) return run<64>(argc, argv);
        if (bytes == 4096) return run<4096>(argc, argv);
        if (bytes == 65536) return run<65536>(argc, argv);
        throw std::runtime_error("Unsupported sample size");
    } catch (const std::exception& e) { std::cerr << "Benchmark failed: " << e.what() << '\n'; return 1; }
}
