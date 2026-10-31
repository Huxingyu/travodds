#include "gtest/gtest.h"
#include "common/memory/sharedpayloadpool.h"
#include "common/memory/sharedpayloadpool_layout.h"
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <linux/futex.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <limits>
#include <poll.h>
#include <signal.h>
#include <stdexcept>
#include <system_error>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

USING_TRAVODDS_NAMESPACE
using namespace std::chrono_literals;
using namespace TRAVODDS::shared_pool_detail;

namespace {
GUID_t guid() {
    GUID_t result;
    result.prefix.value[0] = 0x32;
    result.entityId.entityKey[2] = 17;
    result.entityId.entityKind = ENTITYKIND_USER_WRITER_NO_KEY;
    return result;
}
SharedPoolConfig config(uint32_t slots = 1, uint32_t readers = 4) {
    SharedPoolConfig c;
    c.payload_size = 4096;
    c.slots = slots;
    c.max_readers = readers;
    return c;
}
struct Sample { uint64_t index; uint64_t values[256]; };
void fill(char* buffer, uint64_t index = 17) {
    Sample data{};
    data.index = index;
    for (size_t i = 0; i < 256; ++i) data.values[i] = index * 1000 + i;
    std::memcpy(buffer, &data, sizeof(data));
}
bool valid(const SharedReadLoan& loan, uint64_t index = 17) {
    if (!loan || loan.size() != sizeof(Sample)) return false;
    const auto* data = reinterpret_cast<const Sample*>(loan.data());
    if (data->index != index) return false;
    for (size_t i = 0; i < 256; ++i) if (data->values[i] != index * 1000 + i) return false;
    return true;
}
struct Mapping {
    int fd;
    Header* header;
    size_t bytes;
    explicit Mapping(const SharedPoolDescriptor& d) : bytes(d.segment_bytes) {
        fd = ::open(d.path.c_str(), O_RDWR | O_CLOEXEC);
        if (fd < 0) throw std::runtime_error("test mapping open failed");
        auto* data = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (data == MAP_FAILED) { ::close(fd); throw std::runtime_error("test mapping failed"); }
        header = static_cast<Header*>(data);
    }
    ~Mapping() { munmap(header, bytes); ::close(fd); }
    Slot& slot(size_t index) {
        size_t offset = (sizeof(Header) + alignof(Slot) - 1) & ~(alignof(Slot) - 1);
        return reinterpret_cast<Slot*>(reinterpret_cast<char*>(header) + offset)[index];
    }
};
struct Wire {
    char path[512]{};
    unsigned char writer[16]{};
    SharedPoolSession session;
    SharedPoolConfig config;
    uint64_t bytes = 0;
    uintptr_t writer_base = 0;
    uintptr_t writer_payload = 0;
};
Wire wire(const SharedPoolDescriptor& d, char* payload = nullptr) {
    Wire w;
    if (d.path.size() >= sizeof(w.path)) throw std::runtime_error("test descriptor path too long");
    std::memcpy(w.path, d.path.c_str(), d.path.size() + 1);
    std::memcpy(w.writer, d.writer.prefix.value, 12);
    std::memcpy(w.writer + 12, d.writer.entityId.entityKey, 3);
    w.writer[15] = d.writer.entityId.entityKind;
    w.session = d.session;
    w.config = d.config;
    w.bytes = d.segment_bytes;
    if (payload) {
        size_t offset = (sizeof(Header) + alignof(Slot) - 1) & ~(alignof(Slot) - 1);
        offset += sizeof(Slot) * d.config.slots;
        offset = (offset + d.config.alignment - 1) & ~(size_t(d.config.alignment) - 1);
        w.writer_payload = reinterpret_cast<uintptr_t>(payload);
        w.writer_base = w.writer_payload - offset;
    }
    return w;
}
SharedPoolDescriptor descriptor(const Wire& w) {
    SharedPoolDescriptor d;
    d.path = w.path;
    std::memcpy(d.writer.prefix.value, w.writer, 12);
    std::memcpy(d.writer.entityId.entityKey, w.writer + 12, 3);
    d.writer.entityId.entityKind = w.writer[15];
    d.session = w.session;
    d.config = w.config;
    d.segment_bytes = w.bytes;
    return d;
}
bool transfer(int fd, void* buffer, size_t bytes, bool sending) {
    auto* data = static_cast<char*>(buffer);
    while (bytes) {
        pollfd p{fd, short(sending ? POLLOUT : POLLIN), 0};
        int rc;
        do { rc = poll(&p, 1, 3000); } while (rc < 0 && errno == EINTR);
        if (rc <= 0) return false;
        ssize_t n = sending ? send(fd, data, bytes, MSG_NOSIGNAL) : recv(fd, data, bytes, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        data += n;
        bytes -= size_t(n);
    }
    return true;
}
bool sendByte(int fd, char c) { return transfer(fd, &c, 1, true); }
bool expectByte(int fd, char expected) { char c = 0; return transfer(fd, &c, 1, false) && c == expected; }

// Inspect a single-threaded exec child, rather than guessing with a sleep that
// it has reached the operation. Its socket acknowledgement precedes the API;
// /proc confirms it is actually parked in a futex. All these peers are single
// threaded and the acknowledged phase has no other contended synchronization.
bool waitForFutexWait(pid_t pid) {
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    std::string lastChannel;
    do {
        std::ifstream input("/proc/" + std::to_string(pid) + "/syscall");
        long number = -1;
        std::string address, operation;
        if (input >> number >> address >> operation) {
            const auto command = std::strtoul(operation.c_str(), nullptr, 0) & FUTEX_CMD_MASK;
            if (number == SYS_futex && (command == FUTEX_WAIT || command == FUTEX_WAIT_BITSET)) return true;
        }
        // Yama ptrace_scope=2 denies /proc/<child>/syscall even to its parent
        // without CAP_SYS_PTRACE. wchan still exposes the actual kernel wait
        // function here; require a futex wait rather than guessing from time
        // elapsed or accepting a generic sleeping process state.
        std::ifstream channel("/proc/" + std::to_string(pid) + "/wchan");
        if (channel >> lastChannel && lastChannel.find("futex") != std::string::npos) return true;
        poll(nullptr, 0, 1);
    } while (std::chrono::steady_clock::now() < deadline);
    fprintf(stderr, "No futex wait observed for child %ld; last wchan=%s\n", long(pid), lastChannel.c_str());
    return false;
}

struct ContendedResult {
    int result = -1;
    int64_t elapsed_micros = 0;
};

volatile sig_atomic_t signalSocket = -1;
void acknowledgeInterrupt(int) {
    const int saved = errno;
    const char marker = 'I';
    if (signalSocket >= 0) {
        const auto result = write(signalSocket, &marker, 1);
        (void)result;
    }
    errno = saved;
}

class Child {
public:
    int fd = -1;
    pid_t pid = -1;
    SharedPoolDescriptor orphan;
    explicit Child(const char* mode) {
        int sockets[2];
        if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets)) throw std::runtime_error("test socketpair failed");
        pid = fork();
        if (pid == 0) {
            ::close(sockets[0]);
            fcntl(sockets[1], F_SETFD, 0);
            const auto number = std::to_string(sockets[1]);
            execl("/proc/self/exe", "sharedpayloadpool_test", "--ds-child", mode, number.c_str(), static_cast<char*>(nullptr));
            _exit(120);
        }
        ::close(sockets[1]);
        fd = sockets[0];
        if (pid < 0) { ::close(fd); throw std::runtime_error("test fork failed"); }
    }
    ~Child() {
        if (pid > 0) { kill(pid, SIGKILL); waitpid(pid, nullptr, 0); }
        if (fd >= 0) ::close(fd);
        if (!orphan.path.empty() && access(orphan.path.c_str(), F_OK) == 0) {
            try { SharedPayloadPool::removeOrphan(orphan); } catch (...) {}
        }
    }
    int finish(bool crash = false) {
        if (crash) kill(pid, SIGKILL);
        int status = 0;
        if (waitpid(pid, &status, 0) != pid) return -1;
        pid = -1;
        return status;
    }
    // New failure-injection tests never wait indefinitely for a peer to stop
    // or exit. The parent is their watchdog; destruction kills a stuck peer.
    int waitStatus(int flags = 0) {
        const auto deadline = std::chrono::steady_clock::now() + 3s;
        do {
            int status = 0;
            const auto result = waitpid(pid, &status, flags | WNOHANG);
            if (result == pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) pid = -1;
                return status;
            }
            if (result < 0 && errno != EINTR) return -1;
            poll(nullptr, 0, 1);
        } while (std::chrono::steady_clock::now() < deadline);
        return -1;
    }
};
int childMain(const std::string& mode, int fd) {
    if (mode == "leased-reader") {
        Wire w;
        if (!transfer(fd, &w, sizeof(w), false)) return 61;
        auto pool = SharedPayloadPool::open(descriptor(w));
        auto reader = pool->registerReader();
        if (!reader || !sendByte(fd, 'R')) return 62;
        SharedReadLoan loan;
        if (reader->waitNext(1, 2s, loan) != SharedPoolResult::OK || !valid(loan, 29)) return 63;
        Mapping mapping(descriptor(w));
        // Parent owns registration zero; this independently attached child
        // owns registration one. Inject the exact suspended operation window
        // after both leases were entered, without a test hook in the hot path.
        __atomic_fetch_add(&mapping.header->readers[1].gate, uint64_t(4), __ATOMIC_ACQ_REL);
        __atomic_fetch_add(&mapping.slot(loan.token().slot).gate, uint64_t(4), __ATOMIC_ACQ_REL);
        if (!sendByte(fd, 'K') || !expectByte(fd, 'D')) return 64;
        return 65; // Parent must SIGKILL; graceful cleanup cannot repair it.
    }
    if (mode == "reader" || mode == "interruptible-reader") {
        Wire w;
        if (!transfer(fd, &w, sizeof(w), false)) return 11;
        void* reserved = MAP_FAILED;
        if (w.writer_base) {
            reserved = mmap(reinterpret_cast<void*>(w.writer_base), w.bytes, PROT_NONE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
            if (reserved == MAP_FAILED && errno != EEXIST) return 12;
        }
        auto pool = SharedPayloadPool::open(descriptor(w));
        auto reader = pool->registerReader();
        if (mode == "interruptible-reader") {
            signalSocket = fd;
            struct sigaction action{};
            action.sa_handler = acknowledgeInterrupt;
            sigemptyset(&action.sa_mask);
            // Permit EINTR when the signal interrupts FUTEX_WAIT_BITSET.
            if (sigaction(SIGUSR1, &action, nullptr)) return 16;
        }
        if (!reader || !sendByte(fd, 'R')) return 13;
        SharedReadLoan loan;
        if (reader->waitNext(0, 2s, loan) != SharedPoolResult::OK || !valid(loan) ||
            reinterpret_cast<uintptr_t>(loan.data()) == w.writer_payload || !sendByte(fd, 'L')) return 14;
        if (!expectByte(fd, 'D') || !valid(loan) || !reader->returnLoan(loan) || !sendByte(fd, 'D')) return 15;
        reader.reset();
        pool.reset();
        if (reserved != MAP_FAILED) munmap(reserved, w.bytes);
        return 0;
    }
    if (mode == "metadata-holder") {
        Wire w;
        if (!transfer(fd, &w, sizeof(w), false)) return 41;
        Mapping mapping(descriptor(w));
        for (;;) {
            char command = 0;
            if (!transfer(fd, &command, 1, false)) return 42;
            if (command == 'Q') return 0;
            if (command != 'L' || pthread_mutex_lock(&mapping.header->mutex)) return 43;
            if (!sendByte(fd, 'L') || raise(SIGSTOP)) return 44;
            if (!expectByte(fd, 'U') || pthread_mutex_unlock(&mapping.header->mutex) || !sendByte(fd, 'U')) return 45;
        }
    }
    if (mode == "contended-api-writer") {
        auto pool = SharedPayloadPool::create(guid(), config());
        auto reader = pool->registerReader();
        SharedSampleToken token; char* buffer = nullptr;
        if (!reader || pool->loan(sizeof(Sample), token, buffer) != SharedPoolResult::OK) return 51;
        fill(buffer);
        auto w = wire(pool->descriptor());
        if (!transfer(fd, &w, sizeof(w), true)) return 52;
        for (;;) {
            char command = 0;
            if (!transfer(fd, &command, 1, false)) return 53;
            if (command == 'Q') return 0;
            if (!sendByte(fd, 'A')) return 54;
            const auto begin = std::chrono::steady_clock::now();
            SharedPoolResult result;
            SharedSampleToken extra; char* bytes = nullptr;
            SharedReadLoan loan;
            if (command == 'P') result = pool->publish(token, sizeof(Sample), 1);
            else if (command == 'Z') result = pool->loan(1, extra, bytes, 0ms);
            else if (command == 'T') result = pool->loan(1, extra, bytes, 20ms);
            else if (command == 'W') result = reader->waitNext(1, 20ms, loan);
            else if (command == 'R') {
                result = reader->waitNext(0, 0ms, loan);
                if (result != SharedPoolResult::OK || !valid(loan) ||
                    !reader->returnLoan(loan) || !pool->releaseWriter(token)) result = SharedPoolResult::INVALID;
            } else if (command == 'N') {
                result = pool->loan(sizeof(Sample), extra, bytes, 0ms);
                if (result == SharedPoolResult::OK) {
                    fill(bytes, 29);
                    result = pool->publish(extra, sizeof(Sample), 2);
                    if (result != SharedPoolResult::OK || !pool->releaseWriter(extra) ||
                        reader->waitNext(1, 0ms, loan) != SharedPoolResult::OK ||
                        !valid(loan, 29) || !reader->returnLoan(loan)) result = SharedPoolResult::INVALID;
                }
            }
            else return 55;
            ContendedResult reply;
            reply.result = static_cast<int>(result);
            reply.elapsed_micros = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - begin).count();
            if (!transfer(fd, &reply, sizeof(reply), true)) return 56;
        }
    }
    if (mode == "writer") {
        auto pool = SharedPayloadPool::create(guid(), config());
        auto w = wire(pool->descriptor());
        if (!transfer(fd, &w, sizeof(w), true) || !expectByte(fd, 'R')) return 21;
        SharedSampleToken token;
        char* buffer = nullptr;
        if (pool->loan(sizeof(Sample), token, buffer) != SharedPoolResult::OK) return 22;
        fill(buffer);
        if (pool->publish(token, sizeof(Sample), 1) != SharedPoolResult::OK || !sendByte(fd, 'P')) return 23;
        if (!expectByte(fd, 'D')) return 24;
        return 0;
    }
    return 30;
}
}

// Both devices are owned and mapped by Boost; configured directories use its
// file device, while /dev/shm uses shared_memory_object.
TEST(SharedPoolBoost, ConfiguredDirectoryPreservesAlignmentBudgetAndLoanAfterClose) {
    char directory[] = "/tmp/travodds-boost-XXXXXX";
    ASSERT_NE(mkdtemp(directory), nullptr);
    struct Cleanup { const char* path; ~Cleanup() { rmdir(path); } } cleanup{directory};
    auto c = config(2);
    c.alignment = 4096;
    auto writer = SharedPayloadPool::create(guid(), c, directory);
    auto d = writer->descriptor();
    struct stat status{};
    ASSERT_EQ(stat(d.path.c_str(), &status), 0);
    EXPECT_EQ(status.st_mode & 0777, 0600);
    EXPECT_EQ(uint64_t(status.st_size), d.segment_bytes);
    c.max_segment_bytes = d.segment_bytes - 1;
    EXPECT_THROW(SharedPayloadPool::create(guid(), c, directory), std::invalid_argument);
    auto mapped = SharedPayloadPool::open(d);
    auto reader = mapped->registerReader();
    SharedSampleToken token; char* data = nullptr;
    ASSERT_EQ(writer->loan(sizeof(Sample), token, data), SharedPoolResult::OK);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(data) % 4096, 0u);
    fill(data);
    ASSERT_EQ(writer->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    SharedReadLoan loan;
    ASSERT_EQ(reader->waitNext(0, 1s, loan), SharedPoolResult::OK);
    ASSERT_TRUE(valid(loan));
    EXPECT_NE(data, loan.data());
    writer->closeWriter();
    EXPECT_FALSE(std::filesystem::exists(d.path));
    writer.reset(); reader.reset(); mapped.reset();
    EXPECT_TRUE(valid(loan));
    loan.reset();
    EXPECT_TRUE(std::filesystem::is_empty(directory));
}

TEST(SharedPoolBoost, RejectsSymlinkAndDoesNotRemoveAReplacementInode) {
    char directory[] = "/tmp/travodds-boost-XXXXXX";
    ASSERT_NE(mkdtemp(directory), nullptr);
    struct Cleanup { const char* path; ~Cleanup() { rmdir(path); } } cleanup{directory};
    auto writer = SharedPayloadPool::create(guid(), config(), directory);
    auto d = writer->descriptor();
    const auto moved = d.path + ".original";
    ASSERT_EQ(rename(d.path.c_str(), moved.c_str()), 0);
    ASSERT_EQ(symlink(moved.c_str(), d.path.c_str()), 0);
    // A symlink must be rejected before waiting on the foreign inode's
    // initialization lock. Boost file devices do not promise O_NOFOLLOW.
    const int foreign = ::open(moved.c_str(), O_RDWR | O_CLOEXEC);
    ASSERT_GE(foreign, 0);
    ASSERT_EQ(flock(foreign, LOCK_EX), 0);
    auto attempted = std::async(std::launch::async, [&] {
        try { auto mapped = SharedPayloadPool::open(d); }
        catch (const std::invalid_argument&) { return true; }
        return false;
    });
    EXPECT_EQ(attempted.wait_for(100ms), std::future_status::ready);
    EXPECT_EQ(flock(foreign, LOCK_UN), 0);
    ::close(foreign);
    EXPECT_TRUE(attempted.get());
    writer->closeWriter();
    struct stat status{};
    EXPECT_EQ(lstat(d.path.c_str(), &status), 0);
    EXPECT_TRUE(S_ISLNK(status.st_mode));
    writer.reset();
    EXPECT_EQ(unlink(d.path.c_str()), 0);
    EXPECT_EQ(unlink(moved.c_str()), 0);
}

TEST(SharedPoolBoost, FailedSizingRemovesOnlyItsNewBoostDevice) {
    char directory[] = "/tmp/travodds-boost-XXXXXX";
    ASSERT_NE(mkdtemp(directory), nullptr);
    struct Cleanup { const char* path; ~Cleanup() { rmdir(path); } } cleanup{directory};
    const pid_t pid = fork();
    ASSERT_GE(pid, 0);
    if (!pid) {
        signal(SIGXFSZ, SIG_IGN);
        const rlimit limit{1024, 1024};
        if (setrlimit(RLIMIT_FSIZE, &limit)) _exit(3);
        try { auto writer = SharedPayloadPool::create(guid(), config(), directory); }
        catch (const std::exception&) { _exit(0); }
        _exit(2);
    }
    int status = 0;
    ASSERT_EQ(waitpid(pid, &status, 0), pid);
    ASSERT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);
    EXPECT_TRUE(std::filesystem::is_empty(directory));
}

TEST(SharedPool, NamedReadersReceiveOnlySelectedDeliveriesAndMetadata) {
    auto pool = SharedPayloadPool::create(guid(), config(2));
    GUID_t a = guid(), b = guid();
    a.entityId.entityKind = b.entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
    b.entityId.entityKey[2]++;
    auto first = pool->registerReader(a);
    auto second = pool->registerReader(b);
    auto anonymous = pool->registerReader();
    ASSERT_TRUE(first && second && anonymous);
    EXPECT_FALSE(pool->registerReader(a));
    EXPECT_EQ(pool->registeredReaders(), (std::vector<GUID_t>{a, b}));
    SharedSampleToken token; char* bytes = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, bytes), SharedPoolResult::OK);
    fill(bytes);
    SharedSampleMetadata metadata;
    metadata.source_seconds = 123; metadata.source_nanoseconds = 456789;
    std::memcpy(metadata.instance, &a, 16);
    std::memcpy(metadata.related_writer, &b, 16);
    metadata.related_sequence = 0x123456789ull;
    const std::vector<GUID_t> targets{b};
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1, metadata, &targets), SharedPoolResult::OK);
    ASSERT_TRUE(pool->releaseWriter(token));
    SharedReadLoan loan;
    EXPECT_EQ(first->waitNext(0, 0ms, loan), SharedPoolResult::TIMEOUT);
    EXPECT_EQ(anonymous->waitNext(0, 0ms, loan), SharedPoolResult::TIMEOUT);
    ASSERT_EQ(second->waitNext(0, 0ms, loan), SharedPoolResult::OK);
    EXPECT_TRUE(valid(loan));
    SharedReadLoan moved(std::move(loan));
    EXPECT_EQ(moved.metadata().source_seconds, 123);
    EXPECT_EQ(moved.metadata().source_nanoseconds, 456789u);
    EXPECT_EQ(std::memcmp(moved.metadata().instance, &a, 16), 0);
    EXPECT_EQ(std::memcmp(moved.metadata().related_writer, &b, 16), 0);
    EXPECT_EQ(moved.metadata().related_sequence, 0x123456789ull);
    EXPECT_EQ(loan.metadata().related_sequence, 0u);
    ASSERT_TRUE(second->returnLoan(moved));
    EXPECT_EQ(moved.metadata().source_seconds, 0);
    SharedPoolStats stats;
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.free_slots, 2u);
}

TEST(SharedPool, ClosingReaderDropsUnreadButPreservesOutstandingLoanAndAllowsReregistration) {
    auto pool = SharedPayloadPool::create(guid(), config(2, 2));
    GUID_t readerGuid = guid(); readerGuid.entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
    auto reader = pool->registerReader(readerGuid);
    ASSERT_TRUE(reader);
    for (uint64_t sequence = 1; sequence <= 2; ++sequence) {
        SharedSampleToken token; char* bytes = nullptr;
        ASSERT_EQ(pool->loan(sizeof(Sample), token, bytes), SharedPoolResult::OK);
        fill(bytes, sequence);
        ASSERT_EQ(pool->publish(token, sizeof(Sample), sequence), SharedPoolResult::OK);
        ASSERT_TRUE(pool->releaseWriter(token));
    }
    SharedReadLoan loan, empty;
    ASSERT_EQ(reader->waitNext(0, 0ms, loan), SharedPoolResult::OK);
    reader->close(); reader->close();
    EXPECT_TRUE(pool->registeredReaders().empty());
    EXPECT_EQ(reader->waitNext(1, 0ms, empty), SharedPoolResult::CLOSED);
    EXPECT_TRUE(valid(loan, 1));
    SharedPoolStats stats;
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.pending_references, 0u);
    EXPECT_EQ(stats.loan_references, 1u);
    EXPECT_EQ(stats.free_slots, 1u);
    auto replacement = pool->registerReader(readerGuid);
    ASSERT_TRUE(replacement);
    EXPECT_FALSE(replacement->returnLoan(loan));
    reader.reset();
    EXPECT_TRUE(valid(loan, 1));
    loan.reset();
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.loan_references, 0u);
    EXPECT_EQ(stats.free_slots, 2u);
    EXPECT_EQ(stats.readers, 1u);
}

TEST(SharedPool, CloseWakesBlockedReaderWithoutWaitingForItsDeadline) {
    auto pool = SharedPayloadPool::create(guid(), config());
    auto reader = pool->registerReader();
    ASSERT_TRUE(reader);
    SharedPoolResult result = SharedPoolResult::INVALID;
    auto start = std::chrono::steady_clock::now();
    std::thread waiting([&] { SharedReadLoan loan; result = reader->waitNext(0, 5s, loan); });
    std::this_thread::sleep_for(20ms);
    reader->close();
    waiting.join();
    EXPECT_EQ(result, SharedPoolResult::CLOSED);
    EXPECT_LT(std::chrono::steady_clock::now() - start, 2s);
}

TEST(SharedPool, ReplayIncludesOnlyPublishedHistoryStillHeldByWriter) {
    auto pool = SharedPayloadPool::create(guid(), config(3));
    auto oldReader = pool->registerReader();
    ASSERT_TRUE(oldReader);
    SharedSampleToken tokens[3];
    for (uint64_t i = 0; i < 3; ++i) {
        char* bytes = nullptr;
        ASSERT_EQ(pool->loan(sizeof(Sample), tokens[i], bytes), SharedPoolResult::OK);
        fill(bytes, i + 1);
        // Slot 3 is a mutable, unsubmitted reservation.
        if (i < 2) ASSERT_EQ(pool->publish(tokens[i], sizeof(Sample), i + 1), SharedPoolResult::OK);
    }
    ASSERT_TRUE(pool->releaseWriter(tokens[1])); // Still unread for oldReader, absent from Writer history.
    GUID_t readerGuid = guid(); readerGuid.entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
    auto late = pool->registerReader(readerGuid, true);
    ASSERT_TRUE(late);
    SharedReadLoan loan;
    ASSERT_EQ(late->waitNext(0, 0ms, loan), SharedPoolResult::OK);
    EXPECT_EQ(loan.sequence(), 1u);
    EXPECT_TRUE(valid(loan, 1));
    loan.reset();
    EXPECT_EQ(late->waitNext(1, 0ms, loan), SharedPoolResult::TIMEOUT);
    ASSERT_TRUE(pool->releaseWriter(tokens[0]));
    ASSERT_TRUE(pool->releaseWriter(tokens[2]));
}

TEST(SharedPool, ReplayRegistrationIsVisibleAndReceivesAPublicationAfterTargetSnapshot) {
    auto pool = SharedPayloadPool::create(guid(), config(1));
    auto replayGuid = guid(); replayGuid.entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
    auto volatileGuid = replayGuid; volatileGuid.entityId.entityKey[1] = 8;
    EXPECT_TRUE(pool->registeredReaders().empty()); // Writer's earlier target snapshot.
    auto replay = pool->registerReader(replayGuid, true);
    auto volatileReader = pool->registerReader(volatileGuid, false);
    ASSERT_TRUE(replay && volatileReader);
    const auto retainedReaders = pool->registeredReaders(true);
    ASSERT_EQ(retainedReaders.size(), 1u);
    EXPECT_EQ(retainedReaders[0], replayGuid);
    EXPECT_EQ(pool->registeredReaders().size(), 2u);
    SharedSampleToken token; char* bytes = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, bytes), SharedPoolResult::OK);
    fill(bytes, 87);
    // Durable publication includes OPEN registrations created after the
    // target snapshot, before they become eligible to bypass RTPS.
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    SharedReadLoan a, b;
    ASSERT_EQ(replay->waitNext(0, 0ms, a), SharedPoolResult::OK);
    ASSERT_EQ(volatileReader->waitNext(0, 0ms, b), SharedPoolResult::OK);
    EXPECT_TRUE(valid(a, 87)); EXPECT_TRUE(valid(b, 87));
    replay->close();
    EXPECT_TRUE(pool->registeredReaders(true).empty());
    EXPECT_TRUE(valid(a, 87)); // Closing registration preserves an outstanding pin.
    a.reset(); b.reset();
    ASSERT_TRUE(pool->releaseWriter(token));
}

TEST(SharedPool, ReplayBecomesNativeEligibleOnlyAfterHistoryAndTransitionPublicationsArePinned) {
    auto pool = SharedPayloadPool::create(guid(), config(2, 1));
    SharedSampleToken first, second; char* bytes = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), first, bytes), SharedPoolResult::OK);
    fill(bytes, 1);
    ASSERT_EQ(pool->publish(first, sizeof(Sample), 1), SharedPoolResult::OK);
    Mapping mapped(pool->descriptor());
    // Deterministically pause the old publication lane while registration
    // enters OPEN. This is a protocol-window injection, not a timing guess.
    __atomic_store_n(&mapped.header->publishing, 1u, __ATOMIC_RELEASE);
    auto id = guid(); id.entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
    auto reader = pool->registerReader(id, true);
    __atomic_store_n(&mapped.header->publishing, 0u, __ATOMIC_RELEASE);
    ASSERT_TRUE(reader);
    EXPECT_TRUE(pool->registeredReaders().empty());
    ASSERT_EQ(pool->loan(sizeof(Sample), second, bytes), SharedPoolResult::OK);
    fill(bytes, 2);
    ASSERT_EQ(pool->publish(second, sizeof(Sample), 2), SharedPoolResult::OK);
    EXPECT_TRUE(pool->registeredReaders().empty());
    SharedReadLoan a, b;
    ASSERT_EQ(reader->waitNext(0, 0ms, a), SharedPoolResult::OK);
    EXPECT_TRUE(valid(a, 1));
    EXPECT_EQ(pool->registeredReaders(), std::vector<GUID_t>{id});
    ASSERT_TRUE(pool->releaseWriter(first));
    ASSERT_TRUE(pool->releaseWriter(second));
    ASSERT_EQ(reader->waitNext(1, 0ms, b), SharedPoolResult::OK);
    EXPECT_TRUE(valid(b, 2));
    EXPECT_TRUE(reader->returnLoan(a));
    EXPECT_TRUE(reader->returnLoan(b));
    EXPECT_EQ(reader->waitNext(2, 0ms, a), SharedPoolResult::TIMEOUT);
}

TEST(SharedPool, FrozenRecheckWindowNeverSkipsAnEarlierPendingSequenceOrItsDeadline) {
    auto pool = SharedPayloadPool::create(guid(), config(2, 1));
    auto reader = pool->registerReader();
    ASSERT_TRUE(reader);
    SharedSampleToken tokens[2];
    for (uint64_t sequence = 1; sequence <= 2; ++sequence) {
        char* bytes = nullptr;
        ASSERT_EQ(pool->loan(sizeof(Sample), tokens[sequence - 1], bytes), SharedPoolResult::OK);
        fill(bytes, sequence);
        ASSERT_EQ(pool->publish(tokens[sequence - 1], sizeof(Sample), sequence), SharedPoolResult::OK);
        ASSERT_TRUE(pool->releaseWriter(tokens[sequence - 1]));
    }
    Mapping mapped(pool->descriptor());
    auto& first = mapped.slot(tokens[0].slot);
    // Models reserve prechecking zero pins, then replay adding a pin before
    // its gate CAS: reserve must briefly freeze, recheck and restore this slot.
    __atomic_store_n(&first.gate, uint64_t(3), __ATOMIC_RELEASE);
    SharedReadLoan loan;
    const auto started = std::chrono::steady_clock::now();
    EXPECT_EQ(reader->waitNext(0, 0ms, loan), SharedPoolResult::TIMEOUT);
    EXPECT_FALSE(loan);
    EXPECT_LT(std::chrono::steady_clock::now() - started, 100ms);
    __atomic_store_n(&first.gate, uint64_t(PUBLISHED), __ATOMIC_RELEASE);
    ASSERT_EQ(reader->waitNext(0, 0ms, loan), SharedPoolResult::OK);
    EXPECT_TRUE(valid(loan, 1));
    loan.reset();
    ASSERT_EQ(reader->waitNext(1, 0ms, loan), SharedPoolResult::OK);
    EXPECT_TRUE(valid(loan, 2));
}

TEST(SharedPool, LocalDirectoryAndDiscoveredIdentityDeriveExactSegmentPath) {
    auto pool = SharedPayloadPool::create(guid(), config());
    const auto& d = pool->descriptor();
    EXPECT_EQ(SharedPayloadPool::pathFor(d.writer, d.session, "/dev/shm"), d.path);
    EXPECT_EQ(SharedPayloadPool::pathFor(d.writer, d.session, "/dev/shm/"), d.path);
    EXPECT_THROW(SharedPayloadPool::pathFor(d.writer, d.session, "relative"), std::invalid_argument);
    EXPECT_THROW(SharedPayloadPool::pathFor(d.writer, d.session, std::string("/tmp\0bad", 8)), std::invalid_argument);
    Mapping mapping(d);
    mapping.header->prefix.version = VERSION - 1;
    EXPECT_THROW(SharedPayloadPool::open(d), std::invalid_argument);
    mapping.header->prefix.version = VERSION;
}

TEST(SharedPool, NativeStorageIsAlignedAndPinnedUntilEveryOwnerReturns) {
    auto pool = SharedPayloadPool::create(guid(), config());
    auto mapping = SharedPayloadPool::open(pool->descriptor());
    auto first = mapping->registerReader();
    auto second = mapping->registerReader();
    ASSERT_TRUE(first && second);
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, buffer), SharedPoolResult::OK);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(buffer) % 64, 0u);
    fill(buffer);
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    SharedReadLoan a, b;
    ASSERT_EQ(first->waitNext(0, 0ms, a), SharedPoolResult::OK);
    ASSERT_EQ(second->waitNext(0, 0ms, b), SharedPoolResult::OK);
    EXPECT_EQ(a.data(), b.data());
    EXPECT_NE(a.data(), buffer);
    EXPECT_TRUE(valid(a) && valid(b));
    ASSERT_TRUE(pool->releaseWriter(token));
    char* extra = nullptr;
    SharedSampleToken extraToken;
    EXPECT_EQ(pool->loan(8, extraToken, extra), SharedPoolResult::EXHAUSTED);
    EXPECT_FALSE(second->returnLoan(a));
    EXPECT_TRUE(first->returnLoan(a));
    EXPECT_FALSE(first->returnLoan(a));
    EXPECT_EQ(pool->loan(8, extraToken, extra), SharedPoolResult::EXHAUSTED);
    EXPECT_TRUE(valid(b));
    b.reset();
    ASSERT_EQ(pool->loan(8, extraToken, extra), SharedPoolResult::OK);
    EXPECT_EQ(extraToken.slot, token.slot);
    EXPECT_GT(extraToken.generation, token.generation);
    EXPECT_FALSE(pool->releaseWriter(token));
    EXPECT_TRUE(pool->releaseWriter(extraToken));
}

TEST(SharedPool, UnreadDeliveryPinsStorageAndRegistrationDestructionDropsIt) {
    auto pool = SharedPayloadPool::create(guid(), config());
    auto reader = pool->registerReader();
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(10, token, buffer), SharedPoolResult::OK);
    ASSERT_EQ(pool->publish(token, 10, 1), SharedPoolResult::OK);
    ASSERT_TRUE(pool->releaseWriter(token));
    SharedPoolStats stats;
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.pending_references, 1u);
    EXPECT_EQ(stats.free_slots, 0u);
    reader.reset();
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.pending_references, 0u);
    EXPECT_EQ(stats.free_slots, 1u);
}

TEST(SharedPool, LoanRetainsReaderRegistrationAndMappingAfterWriterCloses) {
    auto pool = SharedPayloadPool::create(guid(), config());
    const auto d = pool->descriptor();
    auto mapping = SharedPayloadPool::open(d);
    auto reader = mapping->registerReader();
    std::weak_ptr<SharedPoolReader> weakReader = reader;
    std::weak_ptr<SharedPayloadPool> weakMapping = mapping;
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, buffer), SharedPoolResult::OK);
    fill(buffer);
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    SharedReadLoan loan;
    ASSERT_EQ(reader->waitNext(0, 0ms, loan), SharedPoolResult::OK);
    pool->closeWriter();
    EXPECT_NE(access(d.path.c_str(), F_OK), 0);
    reader.reset();
    mapping.reset();
    pool.reset();
    EXPECT_FALSE(weakReader.expired());
    EXPECT_FALSE(weakMapping.expired());
    EXPECT_TRUE(valid(loan));
    SharedReadLoan moved = std::move(loan);
    EXPECT_FALSE(loan);
    EXPECT_TRUE(valid(moved));
    moved.reset();
    EXPECT_TRUE(weakReader.expired());
    EXPECT_TRUE(weakMapping.expired());
}

TEST(SharedPool, NotificationIsPersistentAndOrderedAcrossRingWraps) {
    auto pool = SharedPayloadPool::create(guid(), config(3));
    auto reader = pool->registerReader();
    for (uint64_t round = 0; round < 50; ++round) {
        SharedSampleToken tokens[3];
        for (int i = 0; i < 3; ++i) {
            char* buffer = nullptr;
            ASSERT_EQ(pool->loan(sizeof(Sample), tokens[i], buffer), SharedPoolResult::OK);
            fill(buffer, round * 3 + i + 1);
            ASSERT_EQ(pool->publish(tokens[i], sizeof(Sample), round * 3 + i + 1), SharedPoolResult::OK);
            ASSERT_TRUE(pool->releaseWriter(tokens[i]));
        }
        for (uint64_t i = 0; i < 3; ++i) {
            SharedReadLoan loan;
            ASSERT_EQ(reader->waitNext(round * 3 + i, 0ms, loan), SharedPoolResult::OK);
            EXPECT_EQ(loan.sequence(), round * 3 + i + 1);
            EXPECT_TRUE(valid(loan, round * 3 + i + 1));
        }
    }
}

TEST(SharedPool, ExhaustionWaitsForLoanReturnAndTimesOutWithoutOverwriting) {
    auto pool = SharedPayloadPool::create(guid(), config());
    auto reader = pool->registerReader();
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, buffer), SharedPoolResult::OK);
    fill(buffer);
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    ASSERT_TRUE(pool->releaseWriter(token));
    SharedReadLoan pin;
    ASSERT_EQ(reader->waitNext(0, 0ms, pin), SharedPoolResult::OK);
    SharedSampleToken extraToken;
    char* extra = nullptr;
    EXPECT_EQ(pool->loan(8, extraToken, extra, 20ms), SharedPoolResult::TIMEOUT);
    EXPECT_TRUE(valid(pin));
    std::thread returning([&] { std::this_thread::sleep_for(30ms); pin.reset(); });
    const auto result = pool->loan(8, extraToken, extra, 1s);
    returning.join();
    ASSERT_EQ(result, SharedPoolResult::OK);
    EXPECT_TRUE(pool->releaseWriter(extraToken));
}

TEST(SharedPool, ConcurrentReadersReceiveEverySampleWhileSlowLoansPinRingSlots) {
    constexpr uint64_t samples = 400;
    auto pool = SharedPayloadPool::create(guid(), config(4, 3));
    std::array<std::shared_ptr<SharedPoolReader>, 3> readers;
    for (auto& reader : readers) {
        reader = SharedPayloadPool::open(pool->descriptor())->registerReader();
        ASSERT_TRUE(reader);
    }
    std::array<uint64_t, 3> received{};
    std::array<bool, 3> intact{{true, true, true}};
    std::array<std::thread, 3> threads;
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i] = std::thread([&, i] {
            for (uint64_t sequence = 1; sequence <= samples; ++sequence) {
                SharedReadLoan loan;
                if (readers[i]->waitNext(sequence - 1, 2s, loan) != SharedPoolResult::OK ||
                    loan.sequence() != sequence || !valid(loan, sequence)) {
                    intact[i] = false;
                    return;
                }
                if (i == 2 && sequence % 10 == 0) {
                    std::this_thread::sleep_for(2ms);
                    // Other Readers and Writer run while this address is pinned.
                    if (!valid(loan, sequence)) intact[i] = false;
                }
                ++received[i];
            }
        });
    }
    SharedPoolResult result = SharedPoolResult::OK;
    bool released = true;
    for (uint64_t sequence = 1; sequence <= samples; ++sequence) {
        SharedSampleToken token;
        char* buffer = nullptr;
        result = pool->loan(sizeof(Sample), token, buffer, 2s);
        if (result != SharedPoolResult::OK) break;
        fill(buffer, sequence);
        result = pool->publish(token, sizeof(Sample), sequence);
        released = pool->releaseWriter(token);
        if (result != SharedPoolResult::OK || !released) break;
    }
    // Closure wakes every waiter, including when publication failed. Always
    // join before assertions so a failing test cannot destroy a live thread.
    pool->closeWriter();
    for (auto& thread : threads) thread.join();
    EXPECT_EQ(result, SharedPoolResult::OK);
    EXPECT_TRUE(released);
    for (size_t i = 0; i < readers.size(); ++i) {
        EXPECT_TRUE(intact[i]) << "Reader " << i;
        EXPECT_EQ(received[i], samples) << "Reader " << i;
    }
    SharedPoolStats stats;
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.free_slots, 4u);
    EXPECT_EQ(stats.pending_references, 0u);
    EXPECT_EQ(stats.loan_references, 0u);
}

TEST(SharedPool, ConcurrentCallsOnOneReaderConsumeEachSampleOnceAndReleaseEveryPin) {
    constexpr uint64_t samples = 2000;
    auto pool = SharedPayloadPool::create(guid(), config(4, 1));
    auto reader = pool->registerReader();
    ASSERT_TRUE(reader);
    std::array<std::atomic<unsigned>, samples + 1> seen{};
    std::atomic<uint64_t> received{0};
    std::atomic<bool> stop{false}, intact{true};
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    auto consume = [&] {
        while (!stop.load() && received.load() < samples &&
                std::chrono::steady_clock::now() < deadline) {
            SharedReadLoan loan;
            const auto result = reader->waitNext(0, 0ms, loan);
            if (result == SharedPoolResult::TIMEOUT) { std::this_thread::yield(); continue; }
            if (result != SharedPoolResult::OK || loan.sequence() > samples ||
                    !valid(loan, loan.sequence())) { intact = false; break; }
            if (seen[loan.sequence()].fetch_add(1) != 0) intact = false;
            ++received;
        }
    };
    std::thread first(consume), second(consume);
    bool published = true;
    for (uint64_t sequence = 1; sequence <= samples; ++sequence) {
        SharedSampleToken token; char* buffer = nullptr;
        if (pool->loan(sizeof(Sample), token, buffer, 1s) != SharedPoolResult::OK) { published = false; break; }
        fill(buffer, sequence);
        if (pool->publish(token, sizeof(Sample), sequence) != SharedPoolResult::OK ||
                !pool->releaseWriter(token)) { published = false; break; }
    }
    while (published && received.load() < samples && intact.load() &&
            std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    stop = true;
    first.join(); second.join();
    EXPECT_TRUE(published);
    EXPECT_TRUE(intact.load());
    EXPECT_EQ(received.load(), samples);
    for (uint64_t i = 1; i <= samples; ++i) EXPECT_EQ(seen[i].load(), 1u) << i;
    SharedPoolStats stats;
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.free_slots, 4u);
    EXPECT_EQ(stats.pending_references, 0u);
    EXPECT_EQ(stats.loan_references, 0u);
}

TEST(SharedPool, RejectsWrongSessionOversizedStaleAndRepeatedOperations) {
    auto pool = SharedPayloadPool::create(guid(), config());
    auto reader = pool->registerReader();
    SharedSampleToken token;
    char* buffer = nullptr;
    EXPECT_EQ(pool->loan(4097, token, buffer), SharedPoolResult::INVALID);
    ASSERT_EQ(pool->loan(64, token, buffer), SharedPoolResult::OK);
    auto wrong = token;
    wrong.session.low ^= 1;
    EXPECT_EQ(pool->publish(wrong, 64, 1), SharedPoolResult::INVALID);
    EXPECT_FALSE(pool->releaseWriter(wrong));
    EXPECT_EQ(pool->publish(token, 4097, 1), SharedPoolResult::INVALID);
    EXPECT_EQ(pool->publish(token, 64, 0), SharedPoolResult::INVALID);
    EXPECT_EQ(pool->publish(token, 64, 1), SharedPoolResult::OK);
    EXPECT_EQ(pool->publish(token, 64, 2), SharedPoolResult::INVALID);
    EXPECT_FALSE(reader->discard(wrong));
    EXPECT_TRUE(reader->discard(token));
    EXPECT_FALSE(reader->discard(token));
    EXPECT_TRUE(pool->releaseWriter(token));
    EXPECT_FALSE(pool->releaseWriter(token));
    ASSERT_EQ(pool->loan(64, wrong, buffer), SharedPoolResult::OK);
    EXPECT_EQ(pool->publish(wrong, 64, 1), SharedPoolResult::INVALID);
    EXPECT_TRUE(pool->releaseWriter(wrong));
    SharedReadLoan empty;
    EXPECT_EQ(reader->waitNext(0, -1ms, empty), SharedPoolResult::INVALID);
}

TEST(SharedPool, RawPoolInterfaceRejectsInteriorForeignAndRepeatedReturns) {
    auto pool = SharedPayloadPool::create(guid(), config());
    auto other = SharedPayloadPool::create(guid(), config());
    char* buffer = nullptr;
    ASSERT_TRUE(pool->getBuffer(16, buffer));
    char* alias = buffer;
    char* interior = buffer + 1;
    EXPECT_FALSE(pool->releaseBuffer(interior));
    EXPECT_FALSE(other->releaseBuffer(alias));
    EXPECT_TRUE(pool->releaseBuffer(buffer));
    EXPECT_EQ(buffer, nullptr);
    EXPECT_FALSE(pool->releaseBuffer(alias));
}

TEST(SharedPool, ReaderCapacityRecoversAndReusedRegistrationHasNoOldPendingData) {
    auto pool = SharedPayloadPool::create(guid(), config(1, 1));
    auto reader = pool->registerReader();
    ASSERT_TRUE(reader);
    EXPECT_EQ(pool->registerReader(), nullptr);
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(1, token, buffer), SharedPoolResult::OK);
    ASSERT_EQ(pool->publish(token, 1, 1), SharedPoolResult::OK);
    reader.reset();
    auto fresh = pool->registerReader();
    ASSERT_TRUE(fresh);
    SharedReadLoan loan;
    EXPECT_EQ(fresh->waitNext(0, 0ms, loan), SharedPoolResult::TIMEOUT);
    EXPECT_TRUE(pool->releaseWriter(token));
}

TEST(SharedPool, GenerationAndSequenceLimitsNeverWrapIntoStaleTokens) {
    auto pool = SharedPayloadPool::create(guid(), config());
    auto reader = pool->registerReader();
    Mapping mapping(pool->descriptor());
    mapping.slot(0).generation = std::numeric_limits<uint64_t>::max() - 1;
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(1, token, buffer), SharedPoolResult::OK);
    EXPECT_EQ(token.generation, std::numeric_limits<uint64_t>::max());
    ASSERT_EQ(pool->publish(token, 1, std::numeric_limits<uint64_t>::max()), SharedPoolResult::OK);
    SharedReadLoan loan;
    ASSERT_EQ(reader->waitNext(0, 0ms, loan), SharedPoolResult::OK);
    EXPECT_EQ(loan.sequence(), std::numeric_limits<uint64_t>::max());
    ASSERT_TRUE(pool->releaseWriter(token));
    loan.reset();
    EXPECT_EQ(pool->loan(1, token, buffer), SharedPoolResult::EXHAUSTED);
}

TEST(SharedPool, NativeGenerationCrosses32BitBoundaryWithoutRetiringOrAcceptingOldToken) {
    auto pool = SharedPayloadPool::create(guid(), config());
    auto reader = pool->registerReader();
    ASSERT_TRUE(reader);
    Mapping mapping(pool->descriptor());
    // No operations are concurrent with this bounded fault injection. A
    // production generation must not inherit the prototype's 32-bit limit.
    mapping.slot(0).generation = uint64_t(std::numeric_limits<uint32_t>::max()) - 1;
    SharedSampleToken previous{};
    for (uint64_t sequence = 1; sequence <= 3; ++sequence) {
        SharedSampleToken token; char* bytes = nullptr;
        ASSERT_EQ(pool->loan(sizeof(Sample), token, bytes), SharedPoolResult::OK);
        EXPECT_EQ(token.generation, uint64_t(std::numeric_limits<uint32_t>::max()) - 1 + sequence);
        if (sequence > 1) {
            EXPECT_FALSE(pool->releaseWriter(previous));
            EXPECT_FALSE(reader->discard(previous));
        }
        fill(bytes, sequence);
        ASSERT_EQ(pool->publish(token, sizeof(Sample), sequence), SharedPoolResult::OK);
        ASSERT_TRUE(pool->releaseWriter(token));
        SharedReadLoan loan;
        ASSERT_EQ(reader->waitNext(sequence - 1, 0ms, loan), SharedPoolResult::OK);
        EXPECT_TRUE(valid(loan, sequence));
        ASSERT_TRUE(reader->returnLoan(loan));
        previous = token;
    }
}

TEST(SharedPool, All64ReaderBitsSupportTargetingAndReplayIntoTheHighestBit) {
    auto pool = SharedPayloadPool::create(guid(), config(2, 64));
    std::array<std::shared_ptr<SharedPoolReader>, 64> readers;
    std::array<GUID_t, 64> names;
    for (size_t index = 0; index < 63; ++index) {
        names[index] = guid();
        names[index].entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
        names[index].entityId.entityKey[1] = static_cast<unsigned char>(index);
        readers[index] = pool->registerReader(names[index]);
        ASSERT_TRUE(readers[index]) << index;
    }
    SharedSampleToken retained; char* bytes = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), retained, bytes), SharedPoolResult::OK);
    fill(bytes, 91);
    const std::vector<GUID_t> nobody;
    ASSERT_EQ(pool->publish(retained, sizeof(Sample), 1, {}, &nobody), SharedPoolResult::OK);
    names[63] = guid();
    names[63].entityId.entityKind = ENTITYKIND_USER_READER_NO_KEY;
    names[63].entityId.entityKey[1] = 63;
    readers[63] = pool->registerReader(names[63], true);
    ASSERT_TRUE(readers[63]);
    EXPECT_FALSE(pool->registerReader());
    SharedReadLoan replay;
    ASSERT_EQ(readers[63]->waitNext(0, 0ms, replay), SharedPoolResult::OK);
    EXPECT_TRUE(valid(replay, 91));
    EXPECT_EQ(replay.sequence(), 1u);
    ASSERT_TRUE(pool->releaseWriter(retained));
    ASSERT_TRUE(readers[63]->returnLoan(replay));

    SharedSampleToken current;
    ASSERT_EQ(pool->loan(sizeof(Sample), current, bytes), SharedPoolResult::OK);
    fill(bytes, 92);
    const std::vector<GUID_t> highestOnly{names[63]};
    ASSERT_EQ(pool->publish(current, sizeof(Sample), 2, {}, &highestOnly), SharedPoolResult::OK);
    ASSERT_TRUE(pool->releaseWriter(current));
    for (size_t index = 0; index < 63; ++index) {
        SharedReadLoan empty;
        EXPECT_EQ(readers[index]->waitNext(0, 0ms, empty), SharedPoolResult::TIMEOUT) << index;
    }
    SharedReadLoan loan;
    ASSERT_EQ(readers[63]->waitNext(1, 0ms, loan), SharedPoolResult::OK);
    EXPECT_TRUE(valid(loan, 92));
    SharedPoolStats stats;
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.readers, 64u);
    EXPECT_EQ(stats.loan_references, 1u);
    ASSERT_TRUE(readers[63]->returnLoan(loan));
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.free_slots, 2u);
}

TEST(SharedPool, ClosingPinnedReaderDefersIdentityReuseUntilCrossThreadLoanReturn) {
    auto pool = SharedPayloadPool::create(guid(), config(1, 1));
    auto reader = pool->registerReader();
    ASSERT_TRUE(reader);
    SharedSampleToken old; char* bytes = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), old, bytes), SharedPoolResult::OK);
    fill(bytes, 31);
    ASSERT_EQ(pool->publish(old, sizeof(Sample), 1), SharedPoolResult::OK);
    ASSERT_TRUE(pool->releaseWriter(old));
    SharedReadLoan held;
    ASSERT_EQ(reader->waitNext(0, 0ms, held), SharedPoolResult::OK);
    reader->close();
    std::weak_ptr<SharedPoolReader> oldRegistration = reader;
    reader.reset();
    EXPECT_FALSE(oldRegistration.expired());
    EXPECT_FALSE(pool->registerReader());
    SharedSampleToken extra;
    EXPECT_EQ(pool->loan(1, extra, bytes), SharedPoolResult::EXHAUSTED);
    bool intact = false;
    std::thread returning([loan = std::move(held), &intact]() mutable {
        intact = valid(loan, 31);
        loan.reset();
    });
    returning.join();
    EXPECT_TRUE(intact);
    EXPECT_TRUE(oldRegistration.expired());
    auto replacement = pool->registerReader();
    ASSERT_TRUE(replacement);
    ASSERT_EQ(pool->loan(sizeof(Sample), extra, bytes), SharedPoolResult::OK);
    EXPECT_GT(extra.generation, old.generation);
    fill(bytes, 32);
    ASSERT_EQ(pool->publish(extra, sizeof(Sample), 2), SharedPoolResult::OK);
    EXPECT_FALSE(replacement->discard(old));
    EXPECT_FALSE(pool->releaseWriter(old));
    ASSERT_TRUE(pool->releaseWriter(extra));
    SharedReadLoan current;
    ASSERT_EQ(replacement->waitNext(1, 0ms, current), SharedPoolResult::OK);
    EXPECT_TRUE(valid(current, 32));
}

TEST(SharedPool, StaleConcurrentClosersCannotRetireAReplacementRegistrationOrItsLoan) {
    auto pool = SharedPayloadPool::create(guid(), config(1, 1));
    bool sound = true;
    for (uint64_t sequence = 1; sequence <= 128 && sound; ++sequence) {
        auto old = pool->registerReader();
        ASSERT_TRUE(old);
        std::atomic<bool> stop{false};
        auto closeOld = [&] { while (!stop.load()) { old->close(); std::this_thread::yield(); } };
        std::thread first(closeOld), second(closeOld);
        std::shared_ptr<SharedPoolReader> replacement;
        const auto deadline = std::chrono::steady_clock::now() + 2s;
        while (!replacement && std::chrono::steady_clock::now() < deadline) {
            replacement = pool->registerReader();
            std::this_thread::yield();
        }
        SharedReadLoan loan;
        SharedSampleToken token; char* bytes = nullptr;
        bool writerPin = false;
        if (!replacement || pool->loan(sizeof(Sample), token, bytes) != SharedPoolResult::OK) sound = false;
        else {
            writerPin = true;
            fill(bytes, sequence);
            if (pool->publish(token, sizeof(Sample), sequence) != SharedPoolResult::OK ||
                    replacement->waitNext(0, 0ms, loan) != SharedPoolResult::OK || !valid(loan, sequence)) sound = false;
        }
        stop = true;
        first.join(); second.join();
        if (writerPin && !pool->releaseWriter(token)) sound = false;
        if (loan && (!valid(loan, sequence) || !replacement->returnLoan(loan))) sound = false;
        EXPECT_TRUE(sound) << "registration replacement round " << sequence;
    }
    SharedPoolStats stats;
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.readers, 0u);
    EXPECT_EQ(stats.free_slots, 1u);
}

TEST(SharedPool, OwnerDeathPoisonsSegmentAndPreservesAlreadyBorrowedBytes) {
    auto pool = SharedPayloadPool::create(guid(), config());
    auto reader = pool->registerReader();
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, buffer), SharedPoolResult::OK);
    fill(buffer);
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    SharedReadLoan loan;
    ASSERT_EQ(reader->waitNext(0, 0ms, loan), SharedPoolResult::OK);
    Mapping mapping(pool->descriptor());
    pid_t pid = fork();
    ASSERT_GE(pid, 0);
    if (pid == 0) {
        if (pthread_mutex_lock(&mapping.header->mutex)) _exit(31);
        mapping.slot(0).pending = ~uint64_t(0); // Deliberately incomplete metadata transaction.
        _exit(0);
    }
    int status = 0;
    ASSERT_EQ(waitpid(pid, &status, 0), pid);
    ASSERT_EQ(status, 0);
    SharedPoolStats stats;
    EXPECT_EQ(pool->statistics(stats), SharedPoolResult::CORRUPT);
    EXPECT_EQ(pool->loan(1, token, buffer), SharedPoolResult::CORRUPT);
    EXPECT_TRUE(valid(loan));
}

TEST(SharedPool, ValidatesConfigFileHeaderSessionAndNamespaceBeforeUsingLocks) {
    auto c = config();
    c.slots = 0;
    EXPECT_THROW(SharedPayloadPool::create(guid(), c), std::invalid_argument);
    c = config(); c.max_readers = 65;
    EXPECT_THROW(SharedPayloadPool::create(guid(), c), std::invalid_argument);
    c = config(); c.alignment = 3;
    EXPECT_THROW(SharedPayloadPool::create(guid(), c), std::invalid_argument);
    c = config(); c.max_segment_bytes = 1;
    EXPECT_THROW(SharedPayloadPool::create(guid(), c), std::invalid_argument);
    EXPECT_THROW(SharedPayloadPool::create(GUID_t{}, config()), std::invalid_argument);
    auto pool = SharedPayloadPool::create(guid(), config());
    auto d = pool->descriptor();
    d.segment_bytes++;
    EXPECT_THROW(SharedPayloadPool::open(d), std::invalid_argument);
    d = pool->descriptor(); d.session.low ^= 1;
    EXPECT_THROW(SharedPayloadPool::open(d), std::invalid_argument);
    Mapping mapping(pool->descriptor());
    const auto original = mapping.header->prefix;
    mapping.header->prefix.magic = 0;
    EXPECT_THROW(SharedPayloadPool::open(pool->descriptor()), std::invalid_argument);
    mapping.header->prefix = original;
    mapping.header->prefix.pid_namespace_inode++;
    EXPECT_THROW(SharedPayloadPool::open(pool->descriptor()), std::invalid_argument);
    mapping.header->prefix = original;
    ASSERT_EQ(ftruncate(mapping.fd, off_t(pool->descriptor().segment_bytes - 1)), 0);
    EXPECT_THROW(SharedPayloadPool::open(pool->descriptor()), std::invalid_argument);
    ASSERT_EQ(ftruncate(mapping.fd, off_t(pool->descriptor().segment_bytes)), 0);
    EXPECT_FALSE(SharedPayloadPool::removeOrphan(pool->descriptor()));
}

TEST(SharedPool, ClosureDrainsQueuedDataThenReportsClosedAndRejectsNewReaders) {
    auto pool = SharedPayloadPool::create(guid(), config());
    auto reader = pool->registerReader();
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, buffer), SharedPoolResult::OK);
    fill(buffer);
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    pool->closeWriter();
    EXPECT_EQ(pool->registerReader(), nullptr);
    SharedReadLoan loan;
    EXPECT_EQ(reader->waitNext(0, 1s, loan), SharedPoolResult::OK);
    EXPECT_TRUE(valid(loan));
    loan.reset();
    EXPECT_EQ(reader->waitNext(1, 1s, loan), SharedPoolResult::CLOSED);
    EXPECT_EQ(pool->loan(1, token, buffer), SharedPoolResult::CLOSED);
}

TEST(SharedPool, RestartUsesNewSessionAndOldDescriptorCannotAttachNewWriter) {
    auto old = SharedPayloadPool::create(guid(), config());
    const auto descriptor = old->descriptor();
    old->closeWriter();
    auto next = SharedPayloadPool::create(guid(), config());
    EXPECT_FALSE(descriptor.session == next->descriptor().session);
    EXPECT_NE(descriptor.path, next->descriptor().path);
    EXPECT_THROW(SharedPayloadPool::open(descriptor), std::system_error);
}

// A stopped control-plane owner must not block already registered data-plane
// participants. This does not claim the whole DDS write path is lock-free.
TEST(SharedPoolBoundaries, PausedMetadataOwnerDoesNotBlockPublicationBorrowReturnOrSlotReuse) {
    // Keep the owner process isolated: a regression cannot strand a future or
    // a join in this test runner. Destroy the holder before the Writer so even
    // an assertion failure first releases/poisons the shared robust mutex.
    Child writer("contended-api-writer");
    Wire w;
    ASSERT_TRUE(transfer(writer.fd, &w, sizeof(w), false));
    writer.orphan = descriptor(w);
    Child holder("metadata-holder");
    ASSERT_TRUE(transfer(holder.fd, &w, sizeof(w), true));
    const std::array<char, 6> commands{{'P', 'Z', 'T', 'W', 'R', 'N'}};
    const std::array<SharedPoolResult, 6> expected{{SharedPoolResult::OK,
        SharedPoolResult::EXHAUSTED, SharedPoolResult::TIMEOUT, SharedPoolResult::TIMEOUT,
        SharedPoolResult::OK, SharedPoolResult::OK}};
    for (size_t index = 0; index < commands.size(); ++index) {
        SCOPED_TRACE(commands[index]);
        ASSERT_TRUE(sendByte(holder.fd, 'L'));
        ASSERT_TRUE(expectByte(holder.fd, 'L'));
        const int stopped = holder.waitStatus(WUNTRACED);
        ASSERT_GE(stopped, 0);
        ASSERT_TRUE(WIFSTOPPED(stopped));
        ASSERT_EQ(WSTOPSIG(stopped), SIGSTOP);
        ASSERT_TRUE(sendByte(writer.fd, commands[index]));
        ASSERT_TRUE(expectByte(writer.fd, 'A'));
        pollfd completion{writer.fd, POLLIN, 0};
        int ready;
        do { ready = poll(&completion, 1, 2000); } while (ready < 0 && errno == EINTR);
        ASSERT_EQ(ready, 1) << "Data-plane operation waited for the stopped control-plane mutex owner";
        ContendedResult result;
        ASSERT_TRUE(transfer(writer.fd, &result, sizeof(result), false));
        EXPECT_EQ(result.result, static_cast<int>(expected[index]));
        EXPECT_EQ(kill(holder.pid, 0), 0); // Stopped is not owner death.
        EXPECT_LT(result.elapsed_micros, 2000000);
        // Only release the lock after the full API result has arrived. An old
        // implementation cannot pass by completing after SIGCONT/unlock.
        ASSERT_EQ(kill(holder.pid, SIGCONT), 0);
        ASSERT_TRUE(sendByte(holder.fd, 'U'));
        ASSERT_TRUE(expectByte(holder.fd, 'U'));
    }
    ASSERT_TRUE(sendByte(holder.fd, 'Q'));
    EXPECT_EQ(holder.waitStatus(), 0);
    ASSERT_TRUE(sendByte(writer.fd, 'Q'));
    EXPECT_EQ(writer.waitStatus(), 0);
}

TEST(SharedPoolProcesses, NotificationCounterWrapWakesSleepingReaderAndPreservesLoan) {
    auto pool = SharedPayloadPool::create(guid(), config());
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, buffer), SharedPoolResult::OK);
    Mapping mapping(pool->descriptor());
    ASSERT_EQ(pthread_mutex_lock(&mapping.header->mutex), 0);
    __atomic_store_n(&mapping.header->changed, std::numeric_limits<uint32_t>::max(), __ATOMIC_RELEASE);
    ASSERT_EQ(pthread_mutex_unlock(&mapping.header->mutex), 0);
    Child child("reader");
    auto w = wire(pool->descriptor(), buffer);
    ASSERT_TRUE(transfer(child.fd, &w, sizeof(w), true));
    ASSERT_TRUE(expectByte(child.fd, 'R'));
    ASSERT_TRUE(waitForFutexWait(child.pid));
    fill(buffer);
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    EXPECT_EQ(__atomic_load_n(&mapping.header->changed, __ATOMIC_ACQUIRE), 0u);
    ASSERT_TRUE(expectByte(child.fd, 'L'));
    ASSERT_TRUE(pool->releaseWriter(token));
    SharedSampleToken extra; char* bytes = nullptr;
    EXPECT_EQ(pool->loan(1, extra, bytes), SharedPoolResult::EXHAUSTED);
    ASSERT_TRUE(sendByte(child.fd, 'D'));
    ASSERT_TRUE(expectByte(child.fd, 'D'));
    ASSERT_EQ(child.waitStatus(), 0);
    ASSERT_EQ(pool->loan(1, extra, bytes), SharedPoolResult::OK);
    EXPECT_GT(extra.generation, token.generation);
    EXPECT_TRUE(pool->releaseWriter(extra));
}

TEST(SharedPoolProcesses, SignalDeliveredToWaitingReaderDoesNotLoseNextPublication) {
    auto pool = SharedPayloadPool::create(guid(), config());
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, buffer), SharedPoolResult::OK);
    Child child("interruptible-reader");
    auto w = wire(pool->descriptor(), buffer);
    ASSERT_TRUE(transfer(child.fd, &w, sizeof(w), true));
    ASSERT_TRUE(expectByte(child.fd, 'R'));
    ASSERT_TRUE(waitForFutexWait(child.pid));
    ASSERT_EQ(kill(child.pid, SIGUSR1), 0);
    ASSERT_TRUE(expectByte(child.fd, 'I')); // Installed handler has actually run.
    ASSERT_TRUE(waitForFutexWait(child.pid));
    fill(buffer);
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    ASSERT_TRUE(expectByte(child.fd, 'L'));
    ASSERT_TRUE(pool->releaseWriter(token));
    ASSERT_TRUE(sendByte(child.fd, 'D'));
    ASSERT_TRUE(expectByte(child.fd, 'D'));
    EXPECT_EQ(child.waitStatus(), 0);
}

TEST(SharedPoolProcesses, ExecReaderMapsAtDifferentAddressAndPinsTheSameNativeSample) {
    auto pool = SharedPayloadPool::create(guid(), config());
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, buffer), SharedPoolResult::OK);
    Child child("reader");
    auto w = wire(pool->descriptor(), buffer);
    ASSERT_TRUE(transfer(child.fd, &w, sizeof(w), true));
    ASSERT_TRUE(expectByte(child.fd, 'R'));
    fill(buffer);
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    ASSERT_TRUE(expectByte(child.fd, 'L'));
    ASSERT_TRUE(pool->releaseWriter(token));
    SharedSampleToken extraToken;
    char* extra = nullptr;
    EXPECT_EQ(pool->loan(8, extraToken, extra), SharedPoolResult::EXHAUSTED);
    ASSERT_TRUE(sendByte(child.fd, 'D'));
    ASSERT_TRUE(expectByte(child.fd, 'D'));
    EXPECT_EQ(child.finish(), 0);
    ASSERT_EQ(pool->loan(8, extraToken, extra), SharedPoolResult::OK);
    EXPECT_TRUE(pool->releaseWriter(extraToken));
}

TEST(SharedPoolProcesses, DeadReaderPinsAreReapedOnlyAfterConfirmedProcessDeath) {
    auto pool = SharedPayloadPool::create(guid(), config());
    SharedSampleToken token;
    char* buffer = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), token, buffer), SharedPoolResult::OK);
    Child child("reader");
    auto w = wire(pool->descriptor(), buffer);
    ASSERT_TRUE(transfer(child.fd, &w, sizeof(w), true));
    ASSERT_TRUE(expectByte(child.fd, 'R'));
    fill(buffer);
    ASSERT_EQ(pool->publish(token, sizeof(Sample), 1), SharedPoolResult::OK);
    ASSERT_TRUE(expectByte(child.fd, 'L'));
    ASSERT_TRUE(pool->releaseWriter(token));
    SharedSampleToken extraToken;
    char* extra = nullptr;
    EXPECT_EQ(pool->loan(8, extraToken, extra, 20ms), SharedPoolResult::TIMEOUT);
    EXPECT_TRUE(WIFSIGNALED(child.finish(true)));
    // Process probing belongs to control-plane maintenance, not the native
    // reserve/publish fast path. Confirmed death is reaped before retrying.
    SharedPoolStats stats;
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    ASSERT_EQ(pool->loan(8, extraToken, extra), SharedPoolResult::OK);
    EXPECT_GT(extraToken.generation, token.generation);
    EXPECT_TRUE(pool->releaseWriter(extraToken));
}

TEST(SharedPoolProcesses, DeathInsideReaderOperationPoisonsSessionWithoutRevokingExistingLoans) {
    auto pool = SharedPayloadPool::create(guid(), config(2, 2));
    auto parentReader = pool->registerReader();
    ASSERT_TRUE(parentReader);
    SharedSampleToken first; char* bytes = nullptr;
    ASSERT_EQ(pool->loan(sizeof(Sample), first, bytes), SharedPoolResult::OK);
    fill(bytes, 17);
    ASSERT_EQ(pool->publish(first, sizeof(Sample), 1), SharedPoolResult::OK);
    ASSERT_TRUE(pool->releaseWriter(first));
    SharedReadLoan preserved;
    ASSERT_EQ(parentReader->waitNext(0, 0ms, preserved), SharedPoolResult::OK);
    ASSERT_TRUE(valid(preserved, 17));
    const auto oldSession = pool->descriptor().session;
    Child child("leased-reader");
    auto w = wire(pool->descriptor());
    ASSERT_TRUE(transfer(child.fd, &w, sizeof(w), true));
    ASSERT_TRUE(expectByte(child.fd, 'R'));
    SharedSampleToken second;
    ASSERT_EQ(pool->loan(sizeof(Sample), second, bytes), SharedPoolResult::OK);
    fill(bytes, 29);
    ASSERT_EQ(pool->publish(second, sizeof(Sample), 2), SharedPoolResult::OK);
    ASSERT_TRUE(pool->releaseWriter(second));
    ASSERT_TRUE(expectByte(child.fd, 'K'));
    ASSERT_TRUE(WIFSIGNALED(child.finish(true)));
    SharedPoolStats stats;
    EXPECT_EQ(pool->statistics(stats), SharedPoolResult::CORRUPT);
    EXPECT_EQ(pool->loan(1, second, bytes), SharedPoolResult::CORRUPT);
    EXPECT_TRUE(valid(preserved, 17));
    pool->closeWriter();
    auto restarted = SharedPayloadPool::create(guid(), config(2, 2));
    EXPECT_FALSE(restarted->descriptor().session == oldSession);
    SharedSampleToken fresh;
    EXPECT_EQ(restarted->loan(1, fresh, bytes), SharedPoolResult::OK);
    EXPECT_TRUE(restarted->releaseWriter(fresh));
    EXPECT_TRUE(valid(preserved, 17));
}

TEST(SharedPoolProcesses, WriterCrashPreservesLoanDetectsClosureAndAllowsExactOrphanCleanup) {
    Child child("writer");
    Wire w;
    ASSERT_TRUE(transfer(child.fd, &w, sizeof(w), false));
    auto d = descriptor(w);
    child.orphan = d;
    auto pool = SharedPayloadPool::open(d);
    auto reader = pool->registerReader();
    ASSERT_TRUE(reader);
    ASSERT_TRUE(sendByte(child.fd, 'R'));
    ASSERT_TRUE(expectByte(child.fd, 'P'));
    SharedReadLoan loan;
    ASSERT_EQ(reader->waitNext(0, 1s, loan), SharedPoolResult::OK);
    EXPECT_TRUE(WIFSIGNALED(child.finish(true)));
    SharedReadLoan empty;
    EXPECT_EQ(reader->waitNext(1, 1s, empty), SharedPoolResult::CLOSED);
    EXPECT_TRUE(valid(loan));
    EXPECT_TRUE(SharedPayloadPool::removeOrphan(d));
    EXPECT_NE(access(d.path.c_str(), F_OK), 0);
    EXPECT_TRUE(valid(loan));
    loan.reset();
    SharedPoolStats stats;
    ASSERT_EQ(pool->statistics(stats), SharedPoolResult::OK);
    EXPECT_EQ(stats.free_slots, 1u);
}

int main(int argc, char** argv) {
    if (argc == 4 && std::string(argv[1]) == "--ds-child") {
        try { return childMain(argv[2], std::stoi(argv[3])); }
        catch (const std::exception& error) { fprintf(stderr, "DS child: %s\n", error.what()); return 99; }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
