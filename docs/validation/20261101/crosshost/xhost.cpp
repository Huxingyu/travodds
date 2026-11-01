// Cross-host DataSharing fallback check. Separate from the repository; built against f32da83.
// Usage: xhost writer|reader auto|on|off ordinary|loan BYTES COUNT PEER_IP PORT native|cdr
// Payload fields, order and the CDR counters use the same scheme as tools/ospp/benchmark.
#include "demo.h"
#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <thread>
#include <unistd.h>
using namespace datasharing_demo;
using namespace std::chrono_literals;
namespace {
const auto kWait = 90s;
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
    v.index = index; v.sent = 0;
    for (size_t i = 0; i < (Bytes - 16) / 8; ++i) v.fields[i] = index * 1009 + i;
}
template<size_t Bytes> void validate(const Value<Bytes>& v, uint64_t index) {
    require(v.index == index, "Duplicate/missing/out-of-order sample");
    for (size_t i = 0; i < (Bytes - 16) / 8; ++i)
        require(v.fields[i] == index * 1009 + i, "Payload corruption");
}
struct Control {
    int fd = -1;
    ~Control() { if (fd >= 0) close(fd); }
    void configure() {
        int one = 1; setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
        timeval t{120, 0}; setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof t);
    }
    void serve(int port) {
        int l = socket(AF_INET, SOCK_STREAM, 0), one = 1; setsockopt(l, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
        sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(uint16_t(port)); a.sin_addr.s_addr = INADDR_ANY;
        require(bind(l, reinterpret_cast<sockaddr*>(&a), sizeof a) == 0 && listen(l, 1) == 0, "control listen failed");
        timeval t{120, 0}; setsockopt(l, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof t);
        fd = accept(l, nullptr, nullptr); close(l); require(fd >= 0, "control accept failed"); configure();
    }
    void connectTo(const std::string& ip, int port) {
        const auto limit = std::chrono::steady_clock::now() + kWait;
        sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(uint16_t(port));
        require(inet_pton(AF_INET, ip.c_str(), &a.sin_addr) == 1, "bad peer ip");
        while (std::chrono::steady_clock::now() < limit) {
            fd = socket(AF_INET, SOCK_STREAM, 0);
            if (connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof a) == 0) { configure(); return; }
            close(fd); fd = -1; std::this_thread::sleep_for(200ms);
        }
        throw std::runtime_error("control connect failed");
    }
    void send(char c) { require(::send(fd, &c, 1, MSG_NOSIGNAL) == 1, "control send failed"); }
    void expect(char c) { char r = 0; require(recv(fd, &r, 1, 0) == 1, std::string("control timeout waiting ") + c); require(r == c, "control phase mismatch"); }
};
std::string slurp(const char* path) { std::ifstream f(path); std::string s; std::getline(f, s); return s; }
std::string machine() { utsname u{}; uname(&u); return u.machine; }
std::string kernel() { utsname u{}; uname(&u); return u.release; }
template<size_t Bytes> int run(const std::string& role, const std::string& kindName, const std::string& mode, unsigned count,
                               const std::string& ip, int port, const std::string& expect) {
    const bool useLoan = mode == "loan", native = expect == "native", writerRole = role == "writer";
    const auto kind = kindName == "on" ? ON_DATA_SHARING_QOS : kindName == "off" ? OFF_DATA_SHARING_QOS : AUTO_DATA_SHARING_QOS;
    const unsigned batch = 8;
    const std::string path = "/dev/shm";
    const std::string topicName = "xhost." + std::to_string(Bytes) + "." + std::to_string(port);
    Support<Bytes> type;
    struct Guard { DomainParticipant* p = nullptr; ~Guard() { if (p) { p->delete_contained_entities(); TheParticipantFactory->delete_participant(p); } } } owner;
    DomainParticipantQos pq; pq.transport.removeTransport(TRANSPORT_SHM_QOS);
    owner.p = TheParticipantFactory->create_participant(147, pq, nullptr, STATUS_MASK_NONE);
    require(owner.p, "create participant failed");
    check(owner.p->registe_type(type.get_typename(), &type), "register type");
    auto* topic = owner.p->create_topic(topicName.c_str(), type.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
    require(topic, "create topic failed");
    const auto started = std::chrono::steady_clock::now();
    Control peer;
    if (writerRole) {
        auto* pub = owner.p->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE); require(pub, "create publisher failed");
        DataWriterQos q; q.data_sharing.kind = kind; q.data_sharing.shared_memory_directory = path;
        q.data_sharing.max_samples = 64; q.data_sharing.max_readers = 2; q.data_sharing.max_segment_bytes = 16 * 1024 * 1024;
        q.reliability.kind = RELIABLE_RELIABILITY_QOS; q.history.depth = 16;
        auto* writer = pub->create_datawriter(topic, q, nullptr, STATUS_MASK_NONE); require(writer, "create Writer failed");
        peer.serve(port); peer.expect('R');
        InstanceHandleSeq matches; const auto limit = std::chrono::steady_clock::now() + kWait;
        do { check(writer->get_matched_subscriptions(matches), "Writer match"); if (!matches.empty()) break; std::this_thread::sleep_for(20ms); }
        while (std::chrono::steady_clock::now() < limit);
        require(matches.size() == 1, "Reader did not match");
        const double matchSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        type.blockSerialize = native;
        const bool loanCapable = kind != OFF_DATA_SHARING_QOS;
        ReturnCode_t status = RETCODE_ERROR;
        do {
            if (loanCapable) { SampleLoan probe; check(writer->loan_sample(probe), "probe Loan"); fill(*static_cast<Value<Bytes>*>(probe.data()), 0); status = writer->write(probe, HANDLE_NIL); }
            else { Value<Bytes> probe{}; fill(probe, 0); status = writer->write(&probe, HANDLE_NIL); }
            if (status == RETCODE_OK) break; std::this_thread::sleep_for(5ms);
        } while (std::chrono::steady_clock::now() < limit);
        check(status, "probe write");
        peer.expect('P'); type.serialized = 0; peer.send('S'); peer.expect('A');
        for (unsigned i = 1; i <= count; ++i) {
            if (useLoan) { SampleLoan loan; check(writer->loan_sample(loan), "Loan"); fill(*static_cast<Value<Bytes>*>(loan.data()), i); check(writer->write(loan, HANDLE_NIL), "write Loan"); }
            else { Value<Bytes> sample{}; fill(sample, i); check(writer->write(&sample, HANDLE_NIL), "write ordinary"); }
            if (i % batch == 0) peer.expect('B');
        }
        peer.expect('F');
        const unsigned serialized = type.serialized;
        check(pub->delete_datawriter(writer), "delete Writer"); peer.send('D');
        std::cout << "XHOST_RESULT {\"role\":\"writer\",\"kind\":\"" << kindName << "\",\"mode\":\"" << mode << "\",\"bytes\":" << Bytes
            << ",\"samples\":" << count << ",\"serializations\":" << serialized << ",\"match_seconds\":" << matchSeconds
            << ",\"machine\":\"" << machine() << "\",\"kernel\":\"" << kernel() << "\",\"boot_id\":\"" << slurp("/proc/sys/kernel/random/boot_id") << "\"}" << std::endl;
        return 0;
    }
    auto* sub = owner.p->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE); require(sub, "create subscriber failed");
    DataReaderQos q; q.data_sharing.kind = kind; q.data_sharing.shared_memory_directory = path;
    q.reliability.kind = RELIABLE_RELIABILITY_QOS; q.history.depth = 16;
    auto* reader = sub->create_datareader(topic, q, nullptr, STATUS_MASK_NONE); require(reader, "create Reader failed");
    peer.connectTo(ip, port);
    InstanceHandleSeq matches; const auto limit = std::chrono::steady_clock::now() + kWait;
    do { check(reader->get_matched_publications(matches), "Reader match"); if (!matches.empty()) break; std::this_thread::sleep_for(20ms); }
    while (std::chrono::steady_clock::now() < limit);
    require(matches.size() == 1, "Writer did not match");
    const double matchSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    peer.send('R');
    unsigned loans = 0;
    auto take = [&](uint64_t index) {
        SequenceData<Value<Bytes>> values; SampleInfoSeq info;
        const auto deadline = std::chrono::steady_clock::now() + kWait;
        ReturnCode_t result = RETCODE_NO_DATA;
        do {
            if (!reader->get_unread_count()) { std::this_thread::sleep_for(100us); continue; }
            result = reader->take(values, info, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
            if (result != RETCODE_NO_DATA) break;
            std::this_thread::sleep_for(100us);
        } while (std::chrono::steady_clock::now() < deadline);
        check(result, "take sample"); require(values.length() == 1 && info[0].valid_data, "Invalid sample");
        validate(values[0], index);
        if (values.has_loan()) ++loans;
        check(reader->return_loan(values, info), "return Loan");
    };
    take(0); peer.send('P'); peer.expect('S'); type.deserialized = 0; loans = 0; peer.send('A');
    for (unsigned i = 1; i <= count; ++i) { take(i); if (i % batch == 0) peer.send('B'); }
    const unsigned deserialized = type.deserialized;
    peer.send('F'); peer.expect('D');
    std::cout << "XHOST_RESULT {\"role\":\"reader\",\"kind\":\"" << kindName << "\",\"mode\":\"" << mode << "\",\"bytes\":" << Bytes
        << ",\"samples\":" << count << ",\"deserializations\":" << deserialized << ",\"loans\":" << loans << ",\"validated\":" << count
        << ",\"match_seconds\":" << matchSeconds << ",\"machine\":\"" << machine() << "\",\"kernel\":\"" << kernel()
        << "\",\"boot_id\":\"" << slurp("/proc/sys/kernel/random/boot_id") << "\"}" << std::endl;
    return 0;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 9, "usage: xhost writer|reader auto|on|off ordinary|loan BYTES COUNT PEER_IP PORT native|cdr");
        const unsigned long bytes = std::stoul(argv[4]); const unsigned count = unsigned(std::stoul(argv[5])); const int port = std::stoi(argv[7]);
        require(count >= 8 && count % 8 == 0, "count must be a multiple of 8");
        if (bytes == 64) return run<64>(argv[1], argv[2], argv[3], count, argv[6], port, argv[8]);
        if (bytes == 4096) return run<4096>(argv[1], argv[2], argv[3], count, argv[6], port, argv[8]);
        if (bytes == 65536) return run<65536>(argv[1], argv[2], argv[3], count, argv[6], port, argv[8]);
        throw std::runtime_error("Unsupported size");
    } catch (const std::exception& e) { std::cerr << "XHOST_ERROR " << e.what() << std::endl; return 1; }
}
