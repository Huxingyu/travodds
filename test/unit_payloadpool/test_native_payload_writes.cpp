#include "gtest/gtest.h"
#include "common/memory/sharedpayloadpool.h"
#include "common/memory/sharedpayloadpool_layout.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/writer.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(__linux__) && defined(__x86_64__)
#include <fcntl.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;

// This instruction is the sole permission-changing call site admitted after
// seccomp installation. Production mprotect calls cannot open an application
// window. Keeping workers in separate execs avoids changing the test runner.
extern "C" long travodds_payload_application_mprotect(void*, size_t, int);
extern "C" const unsigned char travodds_payload_application_mprotect_return[];
asm(".text\n"
    ".type travodds_payload_application_mprotect,@function\n"
    "travodds_payload_application_mprotect:\n"
    "mov $10,%eax\n"
    "syscall\n"
    ".globl travodds_payload_application_mprotect_return\n"
    "travodds_payload_application_mprotect_return:\n"
    "ret\n"
    ".size travodds_payload_application_mprotect,.-travodds_payload_application_mprotect\n");

USING_TRAVODDS_NAMESPACE
using namespace std::chrono_literals;

namespace {
constexpr size_t pageBytes = 4096;
constexpr size_t wordsPerPage = pageBytes / sizeof(uint64_t);
constexpr uint64_t valueBase = 0x19a2000000000000ull;
constexpr int childChannel = 198;
constexpr const char* roleEnvironment = "TRAVODDS_PAYLOAD_WRITE_ROLE";

struct alignas(pageBytes) ProtectedSample {
    uint64_t keyPage[wordsPerPage];
    uint64_t body[wordsPerPage * 2];
};
static_assert(sizeof(ProtectedSample) == pageBytes * 3, "The guard must cover every sample byte");
static_assert(offsetof(ProtectedSample, body) == pageBytes, "The key must have an independent page");

void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
void ok(ReturnCode_t code, const char* operation) {
    check(code == RETCODE_OK, std::string(operation) + " returned " + std::to_string(code));
}
template<class Predicate> bool eventually(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    do {
        if (predicate()) return true;
        std::this_thread::sleep_for(2ms);
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

enum class Scenario : uint32_t { UNKEYED, KEYED, CONSTRUCTOR_STORE, READER_STORE };
enum class Kind : uint32_t { SEGMENT, CONFIG, READY, BORROWED, WRITER_DELETED, DONE, FAULT_STORE, ERROR };
struct Segment {
    uint64_t deviceMajor = 0, deviceMinor = 0, inode = 0;
    uint64_t payloadOffset = 0, payloadBytes = sizeof(ProtectedSample), segmentBytes = 0;
    uint64_t sessionHigh = 0, sessionLow = 0;
    unsigned char writer[16]{};
    uint32_t slots = 1, maxReaders = 1;
    char path[512]{}, topic[128]{};
};
struct PageWitness {
    uint64_t virtualAddress = 0, vmaStart = 0, vmaEnd = 0, vmaFileOffset = 0;
    uint64_t deviceMajor = 0, deviceMinor = 0, inode = 0;
    char permissions[5]{};
};
struct ProcessProof {
    int64_t pid = 0;
    uint64_t deviceMajor = 0, deviceMinor = 0, inode = 0, fileOffset = 0, bytes = 0;
    uint64_t virtualAddress = 0;
    uint32_t aliases = 0, guardedChecks = 0, appWriteWindows = 0, appReadWindows = 0;
    uint32_t constructions = 0, serializeCalls = 0, deserializeCalls = 0, copyCalls = 0, createCalls = 0;
    uint32_t seccompTsync = 0, blockedMprotect = 0, blockedFileMmap = 0, blockedMremap = 0;
    uint32_t readGuardFaults = 0, initialPayloadReadOnly = 0;
    uint32_t loanDiscardReloan = 0, published = 0, readReturned = 0, taken = 0;
    uint32_t writerDeletedProtected = 0, readerDeletedProtected = 0, loanReleasedProtected = 0;
    PageWitness loanPages[3]{};
};
struct Message {
    Kind kind = Kind::ERROR;
    Segment segment;
    ProcessProof writer, reader;
    int32_t faultSignal = 0;
    char error[256]{};
};
void sendMessage(int fd, const Message& message) {
    ssize_t sent;
    do { sent = send(fd, &message, sizeof(message), MSG_NOSIGNAL); } while (sent < 0 && errno == EINTR);
    check(sent == ssize_t(sizeof(message)), "send proof packet");
}
Message receiveMessage(int fd) {
    pollfd descriptor{fd, POLLIN, 0};
    int result;
    do { result = poll(&descriptor, 1, 12000); } while (result < 0 && errno == EINTR);
    check(result == 1 && (descriptor.revents & POLLIN), "proof packet timed out or peer closed");
    Message message;
    check(recv(fd, &message, sizeof(message), 0) == ssize_t(sizeof(message)), "incomplete proof packet");
    check(message.kind != Kind::ERROR, std::string("peer failure: ") + message.error);
    return message;
}
Message receiveKind(int fd, Kind kind) {
    auto message = receiveMessage(fd);
    check(message.kind == kind, "unexpected proof protocol phase");
    return message;
}

struct Child {
    pid_t pid = -1;
    int fd = -1;
    bool group = false;
    pid_t processGroup = -1;
    ~Child() {
        if (processGroup > 0) kill(-processGroup, SIGKILL);
        if (pid > 0) {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
        }
        if (fd >= 0) close(fd);
    }
    int wait() {
        int status = 0;
        check(eventually([&] {
            const auto done = waitpid(pid, &status, WNOHANG);
            check(done >= 0 || errno == EINTR, "waitpid failed");
            return done == pid;
        }), "worker failed to exit");
        pid = -1;
        return status;
    }
};
void spawnChild(Child& child, const char* role, bool group) {
    int sockets[2];
    check(socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets) == 0, "socketpair");
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, sockets[1], childChannel);
    posix_spawn_file_actions_addclose(&actions, sockets[0]);
    if (sockets[1] != childChannel) posix_spawn_file_actions_addclose(&actions, sockets[1]);
    posix_spawnattr_t attributes;
    posix_spawnattr_init(&attributes);
    if (group) {
        posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP);
        posix_spawnattr_setpgroup(&attributes, 0);
    }
    std::vector<std::string> environment;
    const std::string assignment = std::string(roleEnvironment) + '=';
    for (auto** entry = environ; *entry; ++entry)
        if (std::string(*entry).rfind(assignment, 0) != 0 &&
                std::string(*entry).rfind("GTEST_OUTPUT=", 0) != 0 &&
                std::string(*entry).rfind("XML_OUTPUT_FILE=", 0) != 0) environment.emplace_back(*entry);
    environment.push_back(assignment + role);
    std::vector<char*> env;
    for (auto& entry : environment) env.push_back(entry.data());
    env.push_back(nullptr);
    const auto* info = testing::UnitTest::GetInstance()->current_test_info();
    std::string filter = "--gtest_filter=" + std::string(info->test_suite_name()) + '.' + info->name();
    char executable[] = "/proc/self/exe";
    char* arguments[] = {executable, filter.data(), nullptr};
    const int spawned = posix_spawn(&child.pid, executable, &actions, &attributes, arguments, env.data());
    posix_spawnattr_destroy(&attributes);
    posix_spawn_file_actions_destroy(&actions);
    close(sockets[1]);
    if (spawned) close(sockets[0]);
    check(spawned == 0, "posix_spawn failed");
    child.fd = sockets[0]; child.group = group;
    child.processGroup = group ? child.pid : -1;
}

class Support : public NativeTypeSupport<ProtectedSample> {
public:
    explicit Support(bool keyed) : NativeTypeSupport("ospp.payload-write-proof.v1.pages3.align4096"), keyed_(keyed) {}
    mutable std::atomic<uint32_t> constructions{0};
    std::atomic<uint32_t> serializations{0}, deserializations{0}, copies{0}, creations{0};
    std::function<void(void*)> beforeInjectedStore;
    bool construct_native_sample(void* storage) const override {
        ++constructions;
        if (beforeInjectedStore) {
            beforeInjectedStore(storage);
            // This must fault even when the stored value happens to match.
            std::signal(SIGSEGV, SIG_DFL);
            static_cast<volatile ProtectedSample*>(storage)->body[0] = 79;
            _exit(91);
        }
        return NativeTypeSupport::construct_native_sample(storage);
    }
    NativeKeyInfo get_native_key_info() const override { return keyed_ ? NativeKeyInfo{8, 8} : NativeKeyInfo{}; }
    bool capture_native_key(void* key, const void* value) const override {
        if (!keyed_ || !key || !value) return false;
        std::memcpy(key, &static_cast<const ProtectedSample*>(value)->keyPage[0], 8);
        return true;
    }
    bool restore_native_key(void* value, const void* key) const override {
        if (!keyed_ || !key || !value) return false;
        std::memcpy(&static_cast<ProtectedSample*>(value)->keyPage[0], key, 8);
        return true;
    }
    void* create_data() override { ++creations; return nullptr; }
    void delete_data(void*) override {}
    int32_t copy_data(void*, void*) override { ++copies; return -1; }
    uint32_t get_serialized_data_size(void*, uint32_t) override { return sizeof(ProtectedSample); }
    uint32_t get_max_serialized_data_size(void*, uint32_t) override { return sizeof(ProtectedSample); }
    int32_t serialize_data(void*, CdrSerializer*, int32_t) override { ++serializations; return -1; }
    int32_t deserialize_data(void*, CdrDeserializer*, int32_t) override { ++deserializations; return -1; }
    TypeObject* get_typeobject() override { return nullptr; }
    ReturnCode_t get_instancehandle(void* value, CdrSerializer*, InstanceHandle_t& handle, bool force) override {
        return MakeKey(value, handle, force) ? RETCODE_ERROR : RETCODE_OK;
    }
    int32_t MakeKey(const void* value, InstanceHandle_t& handle, bool) override {
        handle = HANDLE_NIL;
        if (keyed_) std::memcpy(handle.keyHash.value, &static_cast<const ProtectedSample*>(value)->keyPage[0], 8);
        return 0;
    }
    bool has_key() override { return keyed_; }
    const char* get_typename() override { return "ProtectedNativePayload"; }
    void evidence(ProcessProof& proof) const {
        proof.constructions = constructions;
        proof.serializeCalls = serializations; proof.deserializeCalls = deserializations;
        proof.copyCalls = copies; proof.createCalls = creations;
        check(!proof.serializeCalls && !proof.deserializeCalls && !proof.copyCalls && !proof.createCalls,
            "a complete-payload fallback hook was invoked");
    }
private:
    bool keyed_;
};

class Session {
public:
    Session(Support& support, const char* name) {
        DomainParticipantQos qos;
        qos.transport.removeTransport(TRANSPORT_SHM_QOS);
        participant = TheParticipantFactory->create_participant(152, qos, nullptr, STATUS_MASK_NONE);
        check(participant, "create participant");
        ok(participant->registe_type(support.get_typename(), &support), "register type");
        topic = participant->create_topic(name, support.get_typename(), TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        check(topic, "create topic");
        publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE);
        check(publisher && subscriber, "create pub/sub");
    }
    ~Session() {
        if (participant) {
            participant->delete_contained_entities();
            TheParticipantFactory->delete_participant(participant);
        }
    }
    void close() {
        ok(participant->delete_contained_entities(), "delete contained entities");
        ok(TheParticipantFactory->delete_participant(participant), "delete participant");
        participant = nullptr;
    }
    DomainParticipant* participant = nullptr;
    Topic* topic = nullptr;
    Publisher* publisher = nullptr;
    Subscriber* subscriber = nullptr;
};

struct Mapping {
    uint64_t start, end, offset, deviceMajor, deviceMinor, inode;
    std::string permissions;
};
std::vector<Mapping> mappings(const Segment& segment) {
    std::ifstream input("/proc/self/maps");
    check(input.good(), "open /proc/self/maps");
    std::vector<Mapping> result;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        std::string addresses, permissions, offset, device;
        uint64_t inode = 0;
        check(bool(row >> addresses >> permissions >> offset >> device >> inode), "parse maps row");
        const auto dash = addresses.find('-'), colon = device.find(':');
        Mapping entry{std::stoull(addresses.substr(0, dash), nullptr, 16),
            std::stoull(addresses.substr(dash + 1), nullptr, 16), std::stoull(offset, nullptr, 16),
            std::stoull(device.substr(0, colon), nullptr, 16), std::stoull(device.substr(colon + 1), nullptr, 16),
            inode, permissions};
        if (inode == segment.inode && entry.deviceMajor == segment.deviceMajor && entry.deviceMinor == segment.deviceMinor)
            result.push_back(std::move(entry));
    }
    return result;
}
void installFilter() {
    std::vector<sock_filter> code;
    auto statement = [&](uint16_t op, uint32_t value) { code.push_back(BPF_STMT(op, value)); };
    auto jump = [&](uint16_t op, uint32_t value, uint8_t yes, uint8_t no) { code.push_back(BPF_JUMP(op, value, yes, no)); };
    statement(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, arch));
    jump(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0);
    statement(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS);
    statement(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr));
    const uint32_t denied = SECCOMP_RET_ERRNO | EPERM;
    const auto allowedIp = reinterpret_cast<uintptr_t>(travodds_payload_application_mprotect_return);
    jump(BPF_JMP | BPF_JEQ | BPF_K, SYS_mprotect, 0, 6);
    statement(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, instruction_pointer) + 4);
    jump(BPF_JMP | BPF_JEQ | BPF_K, uint32_t(allowedIp >> 32), 0, 3);
    statement(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, instruction_pointer));
    jump(BPF_JMP | BPF_JEQ | BPF_K, uint32_t(allowedIp), 0, 1);
    statement(BPF_RET | BPF_K, SECCOMP_RET_ALLOW);
    statement(BPF_RET | BPF_K, denied);
    // Only private anonymous allocations remain available to libc/sanitizers.
    // No new file-backed alias (including a read-only/private alias) is allowed.
    jump(BPF_JMP | BPF_JEQ | BPF_K, SYS_mmap, 0, 6);
    statement(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, args) + 3 * sizeof(uint64_t));
    jump(BPF_JMP | BPF_JSET | BPF_K, MAP_SHARED, 3, 0);
    jump(BPF_JMP | BPF_JSET | BPF_K, MAP_ANONYMOUS, 0, 2);
    statement(BPF_RET | BPF_K, SECCOMP_RET_ALLOW);
    statement(BPF_RET | BPF_K, denied);
    statement(BPF_RET | BPF_K, denied);
    for (const int call : {SYS_mremap, SYS_pread64, SYS_pwrite64, SYS_preadv, SYS_pwritev,
            SYS_preadv2, SYS_pwritev2, SYS_pkey_mprotect, SYS_remap_file_pages}) {
        jump(BPF_JMP | BPF_JEQ | BPF_K, uint32_t(call), 0, 1);
        statement(BPF_RET | BPF_K, denied);
    }
    statement(BPF_RET | BPF_K, SECCOMP_RET_ALLOW);
    sock_fprog program{static_cast<unsigned short>(code.size()), code.data()};
    check(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "PR_SET_NO_NEW_PRIVS");
    check(syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, SECCOMP_FILTER_FLAG_TSYNC, &program) == 0,
        "install process-wide seccomp TSYNC guard");
}

class PayloadGuard {
public:
    PayloadGuard(const Segment& segment, bool keyed, ProcessProof& proof)
        : segment_(segment), keyed_(keyed), proof_(proof) {
        proof_.pid = getpid();
        check(sysconf(_SC_PAGESIZE) == long(pageBytes), "This Linux x86_64 proof requires 4096-byte pages");
        check(segment_.payloadOffset % pageBytes == 0 && segment_.payloadBytes == sizeof(ProtectedSample),
            "payload and control metadata must occupy independent whole pages");
    }
    void protectEveryAlias() {
        bool found = false;
        for (const auto& mapping : mappings(segment_)) {
            const auto first = std::max(mapping.offset, segment_.payloadOffset);
            const auto last = std::min(mapping.offset + mapping.end - mapping.start,
                segment_.payloadOffset + segment_.payloadBytes);
            if (first >= last) continue;
            found = true;
            check(mapping.permissions.size() == 4 && mapping.permissions[3] == 's', "payload alias must be MAP_SHARED");
            for (uint64_t file = first; file < last; file += pageBytes) {
                check(file % pageBytes == 0, "unaligned file payload page");
                auto* address = reinterpret_cast<void*>(mapping.start + file - mapping.offset);
                check(mprotect(address, pageBytes, keyed_ && file == segment_.payloadOffset ? PROT_READ : PROT_NONE) == 0,
                    "protect all existing payload aliases");
            }
        }
        check(found, "no actual DDS payload mapping found");
        verify();
        installFilter();
        proof_.seccompTsync = 1;
    }
    void requireInitialPayloadReadOnly() {
        uint64_t covered = 0;
        for (const auto& mapping : mappings(segment_)) {
            const auto first = std::max(mapping.offset, segment_.payloadOffset);
            const auto last = std::min(mapping.offset + mapping.end - mapping.start,
                segment_.payloadOffset + segment_.payloadBytes);
            if (first >= last) continue;
            check(mapping.permissions == "r--s", "production Reader payload mapping was not initially read-only");
            covered += last - first;
        }
        check(covered >= segment_.payloadBytes, "production Reader payload mapping was incomplete");
        proof_.initialPayloadReadOnly = 1;
    }
    void verify() {
        std::map<uint64_t, uint64_t> covered;
        for (const auto& mapping : mappings(segment_)) {
            const auto first = std::max(mapping.offset, segment_.payloadOffset);
            const auto last = std::min(mapping.offset + mapping.end - mapping.start,
                segment_.payloadOffset + segment_.payloadBytes);
            if (first >= last) continue;
            const bool readable = keyed_ && first == segment_.payloadOffset;
            check(mapping.permissions == (readable ? "r--s" : "---s"), "payload alias escaped the closed framework window");
            if (readable) check(last <= segment_.payloadOffset + pageBytes, "a body page remained readable");
            covered[mapping.start - mapping.offset] += last - first;
        }
        check(!covered.empty(), "guarded payload mapping disappeared prematurely");
        for (const auto& entry : covered) check(entry.second == segment_.payloadBytes, "partial payload alias protection");
        proof_.aliases = std::max(proof_.aliases, uint32_t(covered.size()));
        ++proof_.guardedChecks;
    }
    void witness(const void* pointer) {
        const auto address = reinterpret_cast<uintptr_t>(pointer);
        check(address % pageBytes == 0, "actual Loan address is not page-aligned");
        const auto maps = mappings(segment_);
        for (size_t page = 0; page < sizeof(ProtectedSample) / pageBytes; ++page) {
            const auto current = address + page * pageBytes;
            auto found = std::find_if(maps.begin(), maps.end(), [&](const Mapping& mapping) {
                return current >= mapping.start && current + pageBytes <= mapping.end;
            });
            check(found != maps.end() && found->permissions[3] == 's' &&
                found->offset + current - found->start == segment_.payloadOffset + page * pageBytes,
                "actual Loan does not alias the expected inode/file offset");
            auto& witness = proof_.loanPages[page];
            witness.virtualAddress = current;
            witness.vmaStart = found->start; witness.vmaEnd = found->end;
            witness.vmaFileOffset = found->offset;
            witness.deviceMajor = found->deviceMajor; witness.deviceMinor = found->deviceMinor; witness.inode = found->inode;
            check(found->permissions.size() == 4, "invalid actual Loan VMA permissions");
            std::copy(found->permissions.begin(), found->permissions.end(), witness.permissions);
        }
        proof_.deviceMajor = segment_.deviceMajor; proof_.deviceMinor = segment_.deviceMinor;
        proof_.inode = segment_.inode; proof_.fileOffset = segment_.payloadOffset;
        proof_.bytes = segment_.payloadBytes; proof_.virtualAddress = address;
        verify();
    }
    void rejectionProbes(const void* pointer, int file) {
        errno = 0;
        check(mprotect(const_cast<void*>(pointer), sizeof(ProtectedSample), PROT_READ | PROT_WRITE) == -1 && errno == EPERM,
            "ordinary mprotect unexpectedly opened the guard");
        proof_.blockedMprotect = 1;
        errno = 0;
        auto* alias = mmap(nullptr, sizeof(ProtectedSample), PROT_READ, MAP_SHARED, file, off_t(segment_.payloadOffset));
        check(alias == MAP_FAILED && errno == EPERM, "new file-backed alias escaped seccomp");
        proof_.blockedFileMmap = 1;
        errno = 0;
        check(syscall(SYS_mremap, pointer, sizeof(ProtectedSample), sizeof(ProtectedSample), 0) == -1 && errno == EPERM,
            "mremap unexpectedly bypassed the guard");
        proof_.blockedMremap = 1;
        // A write-only fault probe would also pass if PROT_NONE accidentally
        // became PROT_READ. Inherit the actual protected mapping into a short
        // isolated child and require a volatile body load to fault as well.
        // No DDS/library operations run between fork and the load.
        const pid_t probe = fork();
        check(probe >= 0, "fork body-read guard probe");
        if (probe == 0) {
            std::signal(SIGSEGV, SIG_DFL);
            const auto value = static_cast<const volatile ProtectedSample*>(pointer)->body[0];
            (void)value;
            _exit(93);
        }
        int status = 0;
        pid_t reaped;
        do { reaped = waitpid(probe, &status, 0); } while (reaped < 0 && errno == EINTR);
        check(reaped == probe && WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV,
            "protected body load was not caught by PROT_NONE");
        ++proof_.readGuardFaults;
        verify();
    }
    void initialize(ProtectedSample* sample) {
        check(proof_.appWriteWindows == 0, "only one application initialization window is allowed");
        change(sample, sizeof(*sample), PROT_READ | PROT_WRITE);
        ++proof_.appWriteWindows;
        for (uint64_t i = 0; i < wordsPerPage; ++i) sample->keyPage[i] = valueBase + i;
        for (uint64_t i = 0; i < wordsPerPage * 2; ++i) sample->body[i] = valueBase + wordsPerPage + i;
        closeWindow(sample);
    }
    void validate(const ProtectedSample* sample) {
        change(const_cast<ProtectedSample*>(sample), sizeof(*sample), PROT_READ);
        ++proof_.appReadWindows;
        for (uint64_t i = 0; i < wordsPerPage; ++i) check(sample->keyPage[i] == valueBase + i, "key-page data mismatch");
        for (uint64_t i = 0; i < wordsPerPage * 2; ++i)
            check(sample->body[i] == valueBase + wordsPerPage + i, "body data mismatch");
        closeWindow(const_cast<ProtectedSample*>(sample));
    }
private:
    static void change(void* address, size_t bytes, int permissions) {
        check(travodds_payload_application_mprotect(address, bytes, permissions) == 0, "application permission window");
    }
    void closeWindow(ProtectedSample* sample) {
        change(sample, pageBytes, keyed_ ? PROT_READ : PROT_NONE);
        change(sample->body, sizeof(sample->body), PROT_NONE);
        verify();
    }
    Segment segment_;
    bool keyed_;
    ProcessProof& proof_;
};

SharedPoolDescriptor descriptor(const Segment& segment) {
    SharedPoolDescriptor result;
    static_assert(sizeof(GUID_t) == sizeof(segment.writer), "GUID must be fixed-width");
    std::memcpy(&result.writer, segment.writer, sizeof(result.writer));
    result.path = segment.path; result.session = {segment.sessionHigh, segment.sessionLow};
    result.config.payload_size = sizeof(ProtectedSample); result.config.alignment = pageBytes;
    result.config.slots = segment.slots; result.config.max_readers = segment.maxReaders;
    result.config.max_segment_bytes = 1024 * 1024; result.segment_bytes = segment.segmentBytes;
    return result;
}
Segment segmentFor(DataWriter* writer, const std::string& topic) {
    const auto& info = dynamic_cast<DataWriterImpl*>(writer)->GetBuiltinTopicData().data_sharing;
    Segment segment;
    segment.segmentBytes = info.segment_bytes; segment.sessionHigh = info.session_high; segment.sessionLow = info.session_low;
    segment.slots = info.slots; segment.maxReaders = info.max_readers;
    check(info.valid(true) && info.type.size == sizeof(ProtectedSample) && info.type.alignment == pageBytes && segment.slots == 1,
        "Writer did not advertise the page-isolated native layout");
    const auto guid = Conversion::ToGuid(writer->get_instance_handle());
    std::memcpy(segment.writer, &guid, sizeof(guid));
    const auto path = SharedPayloadPool::pathFor(guid, {info.session_high, info.session_low}, "/dev/shm");
    check(path.size() < sizeof(segment.path) && topic.size() < sizeof(segment.topic), "proof identifier too long");
    std::strcpy(segment.path, path.c_str()); std::strcpy(segment.topic, topic.c_str());
    struct stat status{};
    check(stat(path.c_str(), &status) == 0, "stat actual Writer segment");
    segment.deviceMajor = major(status.st_dev); segment.deviceMinor = minor(status.st_dev); segment.inode = status.st_ino;
    check(uint64_t(status.st_size) == segment.segmentBytes, "segment file size mismatch");
    // The production layout places all slot/control metadata before the final
    // contiguous payload array. The test does not modify that layout.
    segment.payloadOffset = segment.segmentBytes - segment.payloadBytes * segment.slots;
    check(segment.payloadOffset >= sizeof(shared_pool_detail::Header) + sizeof(shared_pool_detail::Slot),
        "payload guard would overlap control metadata");
    return segment;
}
int openWitness(const Segment& segment) {
    const int fd = open(segment.path, O_RDONLY | O_CLOEXEC);
    check(fd >= 0, "open inode witness");
    struct stat status{};
    check(fstat(fd, &status) == 0 && uint64_t(status.st_ino) == segment.inode &&
        uint64_t(major(status.st_dev)) == segment.deviceMajor && uint64_t(minor(status.st_dev)) == segment.deviceMinor,
        "descriptor path was replaced");
    return fd;
}

void readerWorker(Scenario scenario) {
    const auto config = receiveKind(childChannel, Kind::CONFIG);
    const auto segment = config.segment;
    Support support(scenario == Scenario::KEYED);
    Session session(support, segment.topic);
    DataReaderQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS;
    qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
    auto* reader = session.subscriber->create_datareader(session.topic, qos, nullptr, STATUS_MASK_NONE);
    check(reader, "create Reader");
    check(eventually([&] {
        SubscriptionMatchedStatus status;
        return reader->get_subscription_matched_status(status) == RETCODE_OK && status.current_count == 1 &&
            !mappings(segment).empty();
    }), "Reader native mapping not ready");
    const int witness = openWitness(segment);
    Message result; result.segment = segment;
    PayloadGuard guard(segment, scenario == Scenario::KEYED, result.reader);
    guard.requireInitialPayloadReadOnly();
    guard.protectEveryAlias();
    result.kind = Kind::READY; sendMessage(childChannel, result);
    SequenceData<ProtectedSample> first;
    SampleInfoSeq firstInfo;
    check(eventually([&] { return reader->get_unread_count() == 1; }), "native sample did not arrive");
    ok(reader->read(first, firstInfo, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), "protected read Loan");
    check(first.length() == 1 && first.has_loan() && firstInfo[0].valid_data, "read copied or invalid data");
    guard.witness(&first[0]); guard.rejectionProbes(&first[0], witness); guard.validate(&first[0]);
    const void* address = &first[0];
    ok(reader->return_loan(first, firstInfo), "protected return_loan");
    result.reader.readReturned = 1; guard.verify();
    SequenceData<ProtectedSample> held;
    SampleInfoSeq heldInfo;
    ok(reader->take(held, heldInfo, 1, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE), "protected take Loan");
    check(held.length() == 1 && held.has_loan() && &held[0] == address, "take changed native storage");
    result.reader.taken = 1;
    guard.witness(&held[0]); support.evidence(result.reader);
    result.kind = Kind::BORROWED; sendMessage(childChannel, result);
    if (scenario == Scenario::READER_STORE) {
        std::signal(SIGSEGV, SIG_DFL);
        const_cast<volatile ProtectedSample&>(held[0]).body[0] = 81;
        _exit(92);
    }
    receiveKind(childChannel, Kind::WRITER_DELETED);
    guard.verify(); guard.validate(&held[0]);
    ok(session.subscriber->delete_datareader(reader), "protected Reader deletion");
    result.reader.readerDeletedProtected = 1;
    guard.witness(&held[0]); guard.validate(&held[0]);
    held.unloan(); heldInfo.unloan();
    result.reader.loanReleasedProtected = 1;
    support.evidence(result.reader);
    close(witness);
    session.close();
    result.kind = Kind::DONE; sendMessage(childChannel, result);
}

void writerWorker(Scenario scenario) {
    Support support(scenario == Scenario::KEYED);
    const std::string topic = "native-payload-write-proof-" + std::to_string(getpid());
    Session session(support, topic.c_str());
    DataWriterQos qos;
    qos.data_sharing.kind = ON_DATA_SHARING_QOS; qos.data_sharing.max_samples = 1; qos.data_sharing.max_readers = 1;
    qos.data_sharing.max_segment_bytes = 1024 * 1024;
    qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS; qos.history.depth = 1;
    auto* writer = session.publisher->create_datawriter(session.topic, qos, nullptr, STATUS_MASK_NONE);
    check(writer, "create Writer");
    Message result; result.segment = segmentFor(writer, topic); result.kind = Kind::SEGMENT;
    sendMessage(childChannel, result);
    const int witness = openWitness(result.segment);
    auto observer = SharedPayloadPool::open(descriptor(result.segment));
    PayloadGuard guard(result.segment, scenario == Scenario::KEYED, result.writer);
    if (scenario == Scenario::CONSTRUCTOR_STORE) {
        guard.protectEveryAlias();
        support.beforeInjectedStore = [&](void* storage) {
            guard.witness(storage); guard.rejectionProbes(storage, witness);
            support.evidence(result.writer);
            result.kind = Kind::FAULT_STORE; sendMessage(childChannel, result);
        };
        SampleLoan loan;
        writer->loan_sample(loan);
        throw std::runtime_error("injected constructor store did not fault");
    }
    Child readerChild;
    spawnChild(readerChild, "reader", false);
    Message config; config.kind = Kind::CONFIG; config.segment = result.segment;
    sendMessage(readerChild.fd, config);
    result.reader = receiveKind(readerChild.fd, Kind::READY).reader;
    auto endpoint = dynamic_cast<DomainParticipantImpl*>(session.participant)->GetRelatedParticipant()->GetWriter(
        Conversion::ToGuid(writer->get_instance_handle()).entityId);
    check(eventually([&] { return endpoint->SelectDataSharingReaders(observer->registeredReaders()).size() == 1; }),
        "Writer did not select the actual native Reader");
    guard.protectEveryAlias();
    SampleLoan first;
    ok(writer->loan_sample(first), "protected initial loan_sample");
    guard.witness(first.data()); guard.rejectionProbes(first.data(), witness);
    void* address = first.data();
    ok(writer->discard_loan(first), "protected discard_loan"); guard.verify();
    SampleLoan writable;
    ok(writer->loan_sample(writable), "protected reused loan_sample");
    check(writable.data() == address, "single-slot reuse did not return the same native storage");
    result.writer.loanDiscardReloan = 1; guard.witness(writable.data());
    guard.initialize(static_cast<ProtectedSample*>(writable.data()));
    ok(writer->write(writable, HANDLE_NIL), "protected native publish");
    check(!writable, "successful publish did not consume Loan");
    result.writer.published = 1; guard.verify();
    result.reader = receiveKind(readerChild.fd, Kind::BORROWED).reader;
    check(result.reader.pid != result.writer.pid && result.reader.inode == result.writer.inode &&
        result.reader.deviceMajor == result.writer.deviceMajor && result.reader.deviceMinor == result.writer.deviceMinor &&
        result.reader.fileOffset == result.writer.fileOffset && result.reader.bytes == result.writer.bytes,
        "Reader and Writer do not share the same backing bytes");
    if (scenario == Scenario::READER_STORE) {
        const int status = readerChild.wait();
        check(WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV, "injected Reader store was not caught by the guard");
        result.faultSignal = SIGSEGV;
    }
    ok(session.publisher->delete_datawriter(writer), "protected Writer deletion");
    result.writer.writerDeletedProtected = 1; guard.verify();
    struct stat removed{};
    check(stat(result.segment.path, &removed) == -1 && errno == ENOENT, "Writer segment name remained after deletion");
    if (scenario != Scenario::READER_STORE) {
        Message command; command.kind = Kind::WRITER_DELETED;
        sendMessage(readerChild.fd, command);
        result.reader = receiveKind(readerChild.fd, Kind::DONE).reader;
        const int status = readerChild.wait();
        check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "Reader worker failed");
    }
    support.evidence(result.writer);
    observer.reset(); close(witness);
    session.close();
    result.kind = Kind::DONE; sendMessage(childChannel, result);
}

std::string processJson(const ProcessProof& proof) {
    std::ostringstream out;
    out << "{\"pid\":" << proof.pid << ",\"device_major\":" << proof.deviceMajor
        << ",\"device_minor\":" << proof.deviceMinor << ",\"inode\":" << proof.inode
        << ",\"file_offset\":" << proof.fileOffset << ",\"bytes\":" << proof.bytes
        << ",\"virtual_address\":" << proof.virtualAddress << ",\"aliases\":" << proof.aliases
        << ",\"guarded_checks\":" << proof.guardedChecks << ",\"app_write_windows\":" << proof.appWriteWindows
        << ",\"app_read_windows\":" << proof.appReadWindows << ",\"constructions\":" << proof.constructions
        << ",\"serialize_calls\":" << proof.serializeCalls << ",\"deserialize_calls\":" << proof.deserializeCalls
        << ",\"copy_calls\":" << proof.copyCalls << ",\"create_calls\":" << proof.createCalls
        << ",\"seccomp_tsync\":" << proof.seccompTsync << ",\"blocked_mprotect\":" << proof.blockedMprotect
        << ",\"blocked_mremap\":" << proof.blockedMremap
        << ",\"blocked_file_mmap\":" << proof.blockedFileMmap << ",\"loan_discard_reloan\":" << proof.loanDiscardReloan
        << ",\"read_guard_faults\":" << proof.readGuardFaults
        << ",\"initial_payload_read_only\":" << proof.initialPayloadReadOnly
        << ",\"published\":" << proof.published << ",\"read_returned\":" << proof.readReturned
        << ",\"taken\":" << proof.taken << ",\"writer_deleted_protected\":" << proof.writerDeletedProtected
        << ",\"reader_deleted_protected\":" << proof.readerDeletedProtected
        << ",\"loan_released_protected\":" << proof.loanReleasedProtected << ",\"loan_pages\":[";
    for (size_t page = 0; page < 3; ++page) {
        const auto& witness = proof.loanPages[page];
        if (page) out << ',';
        out << "{\"virtual_address\":" << witness.virtualAddress << ",\"vma_start\":" << witness.vmaStart
            << ",\"vma_end\":" << witness.vmaEnd << ",\"vma_file_offset\":" << witness.vmaFileOffset
            << ",\"device_major\":" << witness.deviceMajor << ",\"device_minor\":" << witness.deviceMinor
            << ",\"inode\":" << witness.inode << ",\"permissions\":\"" << witness.permissions << "\"}";
    }
    out << "]}";
    return out.str();
}
void entry(Scenario scenario, const char* name) {
    if (const auto* role = getenv(roleEnvironment)) {
        // Nested execs must not inherit the coordinator's channel.
        fcntl(childChannel, F_SETFD, FD_CLOEXEC);
        const rlimit noCore{0, 0};
        if (setrlimit(RLIMIT_CORE, &noCore) != 0) _exit(94);
        try {
            if (std::string(role) == "reader") readerWorker(scenario);
            else if (std::string(role) == "writer") writerWorker(scenario);
            else throw std::runtime_error("unknown worker role");
            _exit(0);
        } catch (const std::exception& error) {
            Message failure;
            std::snprintf(failure.error, sizeof(failure.error), "%s", error.what());
            send(childChannel, &failure, sizeof(failure), MSG_NOSIGNAL);
            _exit(1);
        }
    }
    Child writerChild;
    spawnChild(writerChild, "writer", true);
    const auto setup = receiveKind(writerChild.fd, Kind::SEGMENT);
    struct Cleanup {
        Segment segment;
        Child& child;
        ~Cleanup() {
            if (child.processGroup > 0) kill(-child.processGroup, SIGKILL);
            if (child.pid > 0) {
                kill(-child.pid, SIGKILL);
                waitpid(child.pid, nullptr, 0);
                child.pid = -1;
            }
            try { SharedPayloadPool::removeOrphan(descriptor(segment)); } catch (const std::exception&) {}
        }
    } cleanup{setup.segment, writerChild};
    auto result = receiveKind(writerChild.fd, scenario == Scenario::CONSTRUCTOR_STORE ? Kind::FAULT_STORE : Kind::DONE);
    const int status = writerChild.wait();
    bool removedOrphan = false;
    if (scenario == Scenario::CONSTRUCTOR_STORE) {
        check(WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV, "injected constructor store was not caught by the guard");
        result.faultSignal = SIGSEGV;
        removedOrphan = SharedPayloadPool::removeOrphan(descriptor(setup.segment));
        check(removedOrphan, "failed to remove the exact fault-probe orphan");
    } else check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "Writer worker failed");
    std::cout << "DS_PAYLOAD_WRITE_PROOF {\"schema\":1,\"case\":\"NativePayloadWrites." << name
        << "\",\"scenario\":\"" << (scenario == Scenario::KEYED ? "keyed" : scenario == Scenario::UNKEYED ? "unkeyed" :
            scenario == Scenario::CONSTRUCTOR_STORE ? "constructor_store_probe" : "reader_store_probe")
        << "\",\"page_bytes\":" << pageBytes << ",\"sample_bytes\":" << sizeof(ProtectedSample)
        << ",\"key_page_bytes\":" << (scenario == Scenario::KEYED ? pageBytes : 0)
        << ",\"framework_body_access\":\"PROT_NONE\",\"writer\":" << processJson(result.writer)
        << ",\"reader\":" << processJson(result.reader) << ",\"fault_signal\":" << result.faultSignal
        << ",\"exact_orphan_removed\":" << (removedOrphan ? "true" : "false")
        << ",\"hardware_dram_write_count_measured\":false}" << std::endl;
}
}

TEST(NativePayloadWrites, UnkeyedFrameworkCannotAccessProtectedNativePayload) {
    ASSERT_NO_THROW(entry(Scenario::UNKEYED, "UnkeyedFrameworkCannotAccessProtectedNativePayload"));
}
TEST(NativePayloadWrites, KeyedFrameworkReadsOnlyTheIndependentKeyPage) {
    ASSERT_NO_THROW(entry(Scenario::KEYED, "KeyedFrameworkReadsOnlyTheIndependentKeyPage"));
}
TEST(NativePayloadWrites, GuardCatchesAnInjectedConstructorPayloadStore) {
    ASSERT_NO_THROW(entry(Scenario::CONSTRUCTOR_STORE, "GuardCatchesAnInjectedConstructorPayloadStore"));
}
TEST(NativePayloadWrites, GuardCatchesAnInjectedReaderPayloadStore) {
    ASSERT_NO_THROW(entry(Scenario::READER_STORE, "GuardCatchesAnInjectedReaderPayloadStore"));
}
#else
TEST(NativePayloadWrites, RequiresLinuxX8664PageProtection) {
    FAIL() << "Native payload protection evidence requires Linux x86_64";
}
#endif
