// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include "common/memory/sharedpayloadpool.h"
#include "common/memory/sharedpayloadpool_layout.h"
#include "rtps/transport/interprocesstype.h"
#if defined(__linux__)
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <limits>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <climits>
#include <new>
#include <pthread.h>
#include <sstream>
#include <stdexcept>
#include <sys/file.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <system_error>

TRAVODDS_NAMESPACE_BEGIN
using namespace shared_pool_detail;
namespace {
struct Layout { size_t slots, payload, stride, bytes; };

void syscheck(int code, const char* message) {
    if (code) throw std::system_error(code, std::generic_category(), message);
}
size_t rounded(size_t value, size_t alignment) {
    if (value > std::numeric_limits<size_t>::max() - alignment + 1) throw std::invalid_argument("Shared pool layout overflow");
    return (value + alignment - 1) & ~(alignment - 1);
}
Layout layout(const SharedPoolConfig& c) {
    const long page = sysconf(_SC_PAGESIZE);
    if (!c.payload_size || !c.slots || c.slots > 1048576 || !c.max_readers || c.max_readers > MAX_READERS ||
        c.alignment < alignof(void*) || (c.alignment & (c.alignment - 1)) || page <= 0 || c.alignment > uint64_t(page)) {
        throw std::invalid_argument("Invalid shared pool size/count/alignment");
    }
    Layout l{};
    l.slots = rounded(sizeof(Header), alignof(Slot));
    const uint64_t payload = uint64_t(l.slots) + uint64_t(c.slots) * sizeof(Slot);
    if (payload > std::numeric_limits<size_t>::max()) throw std::invalid_argument("Shared metadata overflow");
    // Every Reader maps the whole payload region read-only while keeping
    // control metadata writable. Separate their pages even for small T.
    l.payload = rounded(size_t(payload), size_t(page));
    l.stride = rounded(c.payload_size, c.alignment);
    const uint64_t total = uint64_t(l.payload) + uint64_t(l.stride) * c.slots;
    if (total > c.max_segment_bytes || total > std::numeric_limits<size_t>::max() || total > uint64_t(std::numeric_limits<off_t>::max())) {
        throw std::invalid_argument("Shared pool exceeds segment budget");
    }
    l.bytes = size_t(total);
    return l;
}
// Missing or zombie processes are dead; unreadable/invalid /proc data is unknown.
enum class ProcessState { LIVE, DEAD, UNKNOWN };
ProcessState processState(int64_t pid, uint64_t expected, uint64_t* observed = nullptr) {
    const auto name = "/proc/" + std::to_string(pid) + "/stat";
    int fd = ::open(name.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return errno == ENOENT || errno == ESRCH ? ProcessState::DEAD : ProcessState::UNKNOWN;
    char buffer[4096];
    ssize_t count;
    do { count = ::read(fd, buffer, sizeof(buffer)); } while (count < 0 && errno == EINTR);
    ::close(fd);
    if (count <= 0) return ProcessState::UNKNOWN;
    std::string text(buffer, size_t(count));
    auto end = text.rfind(')');
    if (end == std::string::npos) return ProcessState::UNKNOWN;
    std::istringstream fields(text.substr(end + 1));
    char state = 0;
    fields >> state;
    std::string ignored;
    for (int field = 4; field < 22; ++field) fields >> ignored;
    uint64_t start = 0;
    if (!(fields >> start) || !start) return ProcessState::UNKNOWN;
    if (observed) *observed = start;
    if (state == 'Z' || state == 'X' || (expected && start != expected)) return ProcessState::DEAD;
    return ProcessState::LIVE;
}
ProcessIdentity identity() {
    // /proc must use the caller's PID numbering; otherwise numeric PID probes
    // could identify an unrelated process in an outer namespace.
    char self[32]{};
    ssize_t length = readlink("/proc/self", self, sizeof(self) - 1);
    if (length <= 0 || std::string(self, size_t(length)) != std::to_string(getpid())) {
        throw std::runtime_error("Shared pool requires matching /proc PID namespace");
    }
    ProcessIdentity result{getpid(), 0};
    if (processState(result.pid, 0, &result.start) != ProcessState::LIVE) {
        throw std::runtime_error("Shared pool requires readable process start identity");
    }
    return result;
}
std::string filename(const GUID_t& guid, const SharedPoolSession& session) {
    std::ostringstream name;
    name << "travodds_ds_" << std::hex << std::setfill('0');
    for (auto octet : guid.prefix.value) name << std::setw(2) << unsigned(octet);
    for (auto octet : guid.entityId.entityKey) name << std::setw(2) << unsigned(octet);
    name << std::setw(2) << unsigned(guid.entityId.entityKind) << '_';
    name << std::setw(16) << session.high << std::setw(16) << session.low << ".shm";
    return name.str();
}
void guidBytes(const GUID_t& guid, unsigned char* output) {
    std::memcpy(output, guid.prefix.value, 12);
    std::memcpy(output + 12, guid.entityId.entityKey, 3);
    output[15] = guid.entityId.entityKind;
}
void fileLock(int fd, int operation) {
    int rc;
    do { rc = flock(fd, operation); } while (rc < 0 && errno == EINTR);
    if (rc) syscheck(errno, "Shared pool initialization lock");
}
void splitPath(const std::string& path, std::string& directory, std::string& name) {
    auto slash = path.rfind('/');
    if (slash == std::string::npos || path[0] != '/' || slash + 1 == path.size()) throw std::invalid_argument("Shared pool needs an absolute file path");
    directory = slash ? path.substr(0, slash) : "/";
    name = path.substr(slash + 1);
}
// A killed process must not leave a userspace condition-variable waiter lock
// behind. The kernel owns futex wait queues and removes waiters on process exit.
static_assert(__atomic_always_lock_free(sizeof(uint32_t), nullptr), "Shared notification needs lock-free 32-bit atomics");
static_assert(__atomic_always_lock_free(sizeof(uint64_t), nullptr), "Shared pool needs lock-free 64-bit atomics");
template<class T> T load(const T& value) { return __atomic_load_n(&value, __ATOMIC_SEQ_CST); }
template<class T> void store(T& value, T next) { __atomic_store_n(&value, next, __ATOMIC_SEQ_CST); }
template<class T> bool cas(T& value, T& old, T next) {
    return __atomic_compare_exchange_n(&value, &old, next, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}
constexpr uint64_t PHASE_MASK = 3, OPERATION = 4, FROZEN = 3;
constexpr uint64_t READER_OPEN = 0, READER_CLOSING = 1, READER_RETIRED = 2;

struct Consumer {
    std::atomic_flag& flag;
    bool held;
    explicit Consumer(std::atomic_flag& value, bool acquire = true)
        : flag(value), held(acquire && !flag.test_and_set(std::memory_order_acquire)) {}
    ~Consumer() { if (held) flag.clear(std::memory_order_release); }
};

// Generation is checked only after entering this gate. A concurrent reserve
// cannot modify either payload or generation until the last operation exits.
struct SlotLease {
    Slot* value = nullptr;
    SlotLease() = default;
    SlotLease(const SlotLease&) = delete;
    SlotLease& operator=(const SlotLease&) = delete;
    ~SlotLease() { reset(); }
    bool acquire(Slot& slot, bool writing = false) {
        auto old = load(slot.gate);
        for (;;) {
            const auto phase = old & PHASE_MASK;
            if (phase == FROZEN || phase == FREE || (!writing && phase != PUBLISHED) ||
                    old > UINT64_MAX - OPERATION) return false;
            if (cas(slot.gate, old, old + OPERATION)) { value = &slot; return true; }
        }
    }
    void reset() {
        if (value) __atomic_fetch_sub(&value->gate, OPERATION, __ATOMIC_RELEASE);
        value = nullptr;
    }
};
struct ReaderLease {
    ReaderRecord* value = nullptr;
    ReaderLease() = default;
    ReaderLease(const ReaderLease&) = delete;
    ReaderLease& operator=(const ReaderLease&) = delete;
    ~ReaderLease() { reset(); }
    bool acquire(ReaderRecord& reader, bool returning = false, uint64_t generation = 0) {
        auto old = load(reader.gate);
        for (;;) {
            const auto phase = old & PHASE_MASK;
            if ((phase != READER_OPEN && !(returning && phase == READER_CLOSING)) ||
                    old > UINT64_MAX - OPERATION) return false;
            if (cas(reader.gate, old, old + OPERATION)) break;
        }
        value = &reader;
        if (!load(reader.active) || (generation && (load(reader.generation) != generation || reader.process.pid != getpid()))) {
            reset(); return false;
        }
        return true;
    }
    void reset() {
        if (value) __atomic_fetch_sub(&value->gate, OPERATION, __ATOMIC_RELEASE);
        value = nullptr;
    }
};
void slotPhase(Slot& slot, uint32_t phase) {
    auto old = load(slot.gate);
    while (!cas(slot.gate, old, (old & ~PHASE_MASK) | phase)) {}
    store(slot.state, phase);
}
struct Publication {
    Header* header;
    bool held;
    explicit Publication(Header* h, bool acquire = true) : header(h), held(false) {
        uint32_t expected = 0;
        held = acquire && cas(header->publishing, expected, uint32_t(1));
    }
    ~Publication() { if (held) store(header->publishing, uint32_t(0)); }
};
// Only the holder of Publication may construct an active mutation. Epochs
// protect shared links, not application payload; all link accesses are atomic.
// The last even value retires structural mutation instead of wrapping to an
// old snapshot identity. Existing samples and Loan returns remain usable.
struct HistoryMutation {
    Header* header;
    uint64_t completed_epoch = 0;
    SharedPoolResult result = SharedPoolResult::OK;
    bool active = false;
    HistoryMutation(Header* h, bool begin = true) : header(h) {
        if (!begin) return;
        const auto epoch = load(header->history_epoch);
        if (epoch >= UINT64_MAX - 1) result = SharedPoolResult::EXHAUSTED;
        else if (epoch & 1) result = SharedPoolResult::CORRUPT;
        else {
            completed_epoch = epoch + 2;
            store(header->history_epoch, epoch + 1);
            active = true;
        }
    }
    ~HistoryMutation() { if (active) store(header->history_epoch, completed_epoch); }
};
enum class HistorySnapshot { STABLE, RETRY, CORRUPT };
void notify(Header* header) noexcept {
    __atomic_add_fetch(&header->changed, 1u, __ATOMIC_RELEASE);
    syscall(SYS_futex, &header->changed, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
}
struct Guard {
    Header* header;
    bool held = false;
    SharedPoolResult result = SharedPoolResult::SYSTEM_ERROR;
    explicit Guard(Header* value, bool nonblocking = false) : header(value) {
        const int rc = nonblocking ? pthread_mutex_trylock(&header->mutex) : pthread_mutex_lock(&header->mutex);
        if (rc == EBUSY) result = SharedPoolResult::TIMEOUT;
        else acquired(rc);
    }
    ~Guard() { if (held) pthread_mutex_unlock(&header->mutex); }
    void acquired(int rc) {
        if (!rc || rc == EOWNERDEAD) {
            held = true;
            if (rc == EOWNERDEAD) {
                // No uncertain partial transaction is repaired/reused. Existing
                // loans remain mapped, and all further allocation is refused.
                store(header->poisoned, uint32_t(1));
                store(header->alive, uint32_t(0));
                pthread_mutex_consistent(&header->mutex);
                notify(header);
            }
            result = load(header->poisoned) ? SharedPoolResult::CORRUPT : SharedPoolResult::OK;
        } else {
            result = rc == ENOTRECOVERABLE ? SharedPoolResult::CORRUPT : SharedPoolResult::SYSTEM_ERROR;
        }
    }
};
using Clock = std::chrono::steady_clock;
bool validTimeout(std::chrono::milliseconds timeout) { return timeout.count() >= 0 && timeout <= std::chrono::hours(24); }
timespec nextWake(Clock::time_point deadline) {
    auto remaining = std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - Clock::now());
    remaining = std::max(std::chrono::nanoseconds(1), std::min(remaining,
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(100))));
    timespec result{};
    clock_gettime(CLOCK_MONOTONIC, &result);
    const auto nanos = int64_t(result.tv_nsec) + remaining.count();
    result.tv_sec += nanos / 1000000000;
    result.tv_nsec = nanos % 1000000000;
    return result;
}
uint64_t bit(uint32_t index) { return uint64_t(1) << index; }
bool unpinned(const Slot& slot) { return !load(slot.writer_held) && !load(slot.pending) && !load(slot.loaned); }
void sleepUntil(Header* header, uint32_t observed, const timespec& deadline) {
    syscall(SYS_futex, &header->changed, FUTEX_WAIT_BITSET, observed, &deadline, nullptr, FUTEX_BITSET_MATCH_ANY);
}
}

struct SharedPayloadPool::Impl {
    int fd = -1, directory_fd = -1;
    bool owner = false, initialized = false;
    int64_t owner_pid = 0;
    dev_t device = 0;
    ino_t inode = 0;
    std::string name;
    // Boost owns both the device handle and mapped region. fd is borrowed
    // solely for initialization flock, allocation reservation and validation.
    std::unique_ptr<InterprocessType::shared_memory_object> shared;
    std::unique_ptr<InterprocessType::shared_file> file;
    std::unique_ptr<InterprocessType::mapped_region> region;
    bool posix_shared = false;
    std::string device_path;
    void* mapping = nullptr;
    void openDevice(bool create) {
        struct stat configured{}, standard{};
        if (fstat(directory_fd, &configured)) syscheck(errno, "Stat shared directory");
        posix_shared = stat("/dev/shm", &standard) == 0 &&
            standard.st_dev == configured.st_dev && standard.st_ino == configured.st_ino;
        device_path = "/proc/self/fd/" + std::to_string(directory_fd) + "/" + name;
        if (posix_shared) {
            if (create) shared = std::make_unique<InterprocessType::shared_memory_object>(
                InterprocessType::create_only, name.c_str(), InterprocessType::read_write,
                InterprocessType::permissions(0600));
            else shared = std::make_unique<InterprocessType::shared_memory_object>(
                InterprocessType::open_only, name.c_str(), InterprocessType::read_write);
            fd = InterprocessType::ipcdetail::file_handle_from_mapping_handle(shared->get_mapping_handle());
        } else {
            if (create) file = std::make_unique<InterprocessType::shared_file>(
                InterprocessType::create_only, device_path.c_str(), InterprocessType::read_write,
                InterprocessType::permissions(0600));
            else file = std::make_unique<InterprocessType::shared_file>(
                InterprocessType::open_only, device_path.c_str(), InterprocessType::read_write);
            fd = InterprocessType::ipcdetail::file_handle_from_mapping_handle(file->get_mapping_handle());
        }
        if (create) {
            struct stat created{};
            if (fstat(fd, &created)) syscheck(errno, "Stat created Boost segment");
            device = created.st_dev;
            inode = created.st_ino;
            owner = true;
        }
        if (fcntl(fd, F_SETFD, FD_CLOEXEC)) syscheck(errno, "Set shared device close-on-exec");
    }
    void mapDevice(size_t bytes) {
        if (shared) region = std::make_unique<InterprocessType::mapped_region>(
            *shared, InterprocessType::read_write, 0, bytes);
        else region = std::make_unique<InterprocessType::mapped_region>(
            *file, InterprocessType::read_write, 0, bytes);
        mapping = region->get_address();
    }
    Layout memory{};
    Header* header = nullptr;
    ~Impl() {
        if (owner && owner_pid == getpid()) unlinkFile();
        region.reset();
        file.reset();
        shared.reset();
        if (directory_fd >= 0) ::close(directory_fd);
    }
    void unlinkFile() {
        struct stat status{};
        if (directory_fd >= 0 && fstatat(directory_fd, name.c_str(), &status, AT_SYMLINK_NOFOLLOW) == 0 &&
            status.st_dev == device && status.st_ino == inode) {
            if (posix_shared) InterprocessType::shared_memory_object::remove(name.c_str());
            else InterprocessType::shared_file::remove(device_path.c_str());
        }
    }
    Slot& slot(uint32_t index) const {
        return reinterpret_cast<Slot*>(static_cast<char*>(mapping) + memory.slots)[index];
    }
    char* payload(uint32_t index) const { return static_cast<char*>(mapping) + memory.payload + memory.stride * index; }
    bool appendHistory(uint32_t index) {
        auto& current = slot(index);
        const auto count = load(header->history_count);
        const auto head = load(header->history_head), tail = load(header->history_tail);
        if (load(current.history_linked) || count >= header->prefix.slots) return false;
        if (!count) {
            if (head != NO_SLOT || tail != NO_SLOT) return false;
            store(current.history_previous, index);
            store(current.history_next, index);
            store(current.history_linked, uint32_t(1));
            store(header->history_head, index);
            store(header->history_tail, index);
        } else {
            if (head >= header->prefix.slots || tail >= header->prefix.slots ||
                    load(slot(head).history_linked) != 1 || load(slot(tail).history_linked) != 1 ||
                    load(slot(head).history_previous) != tail || load(slot(tail).history_next) != head) return false;
            store(current.history_previous, tail);
            store(current.history_next, head);
            store(current.history_linked, uint32_t(1));
            store(slot(tail).history_next, index);
            store(slot(head).history_previous, index);
            store(header->history_tail, index);
        }
        store(header->history_count, count + 1);
        return true;
    }
    bool unlinkHistory(uint32_t index) {
        auto& current = slot(index);
        if (!load(current.history_linked)) return true;
        const auto count = load(header->history_count);
        const auto head = load(header->history_head), tail = load(header->history_tail);
        const auto previous = load(current.history_previous), next = load(current.history_next);
        if (!count || count > header->prefix.slots || load(current.history_linked) != 1 ||
                head >= header->prefix.slots || tail >= header->prefix.slots) return false;
        if (count == 1) {
            if (head != index || tail != index || previous != index || next != index) return false;
            store(header->history_head, NO_SLOT);
            store(header->history_tail, NO_SLOT);
        } else {
            if (previous >= header->prefix.slots || next >= header->prefix.slots ||
                    previous == index || next == index ||
                    load(slot(previous).history_linked) != 1 || load(slot(next).history_linked) != 1 ||
                    load(slot(previous).history_next) != index || load(slot(next).history_previous) != index) return false;
            store(slot(previous).history_next, next);
            store(slot(next).history_previous, previous);
            if (head == index) store(header->history_head, next);
            if (tail == index) store(header->history_tail, previous);
        }
        store(header->history_count, count - 1);
        store(current.history_linked, uint32_t(0));
        store(current.history_previous, NO_SLOT);
        store(current.history_next, NO_SLOT);
        return true;
    }
    template<class Visit> HistorySnapshot historySnapshot(Visit visit) {
        const auto epoch = load(header->history_epoch);
        if (epoch & 1) return HistorySnapshot::RETRY;
        const auto completed = [&](bool valid) {
            if (load(header->history_epoch) != epoch) return HistorySnapshot::RETRY;
            return valid ? HistorySnapshot::STABLE : HistorySnapshot::CORRUPT;
        };
        const auto count = load(header->history_count);
        const auto head = load(header->history_head), tail = load(header->history_tail);
        if (count > header->prefix.slots) return completed(false);
        if (!count) return completed(head == NO_SLOT && tail == NO_SLOT);
        if (head >= header->prefix.slots || tail >= header->prefix.slots) return completed(false);
        auto index = head, previous = tail;
        uint64_t previousSequence = 0;
        for (uint32_t visited = 0; visited < count; ++visited) {
            if (index >= header->prefix.slots) return completed(false);
            auto& current = slot(index);
            // Traversal reads atomic metadata under the epoch. Visitors that
            // add pins must lease only the nodes they actually need to pin;
            // leasing unrelated nodes can make a zero-timeout loan fail.
            if ((load(current.gate) & PHASE_MASK) != PUBLISHED || load(current.history_linked) != 1 ||
                    load(current.history_previous) != previous) return completed(false);
            const auto sequence = load(current.sequence);
            if (!sequence || sequence <= previousSequence) return completed(false);
            visit(index, current);
            previousSequence = sequence;
            previous = index;
            index = load(current.history_next);
        }
        return completed(index == head && previous == tail);
    }
    bool tokenMatches(const SharedSampleToken& token) const {
        return token.session == header->prefix.session && token.slot < header->prefix.slots &&
            token.generation && load(slot(token.slot).generation) == token.generation;
    }
    SharedPoolResult health() const {
        if (load(header->poisoned)) return SharedPoolResult::CORRUPT;
        return load(header->alive) ? SharedPoolResult::OK : SharedPoolResult::CLOSED;
    }
    void poison() {
        store(header->poisoned, uint32_t(1));
        store(header->alive, uint32_t(0));
        notify(header);
    }
    // Called only after closing the registration to new operations. Existing
    // takes may finish; either this scan observes their pending bit or the
    // publisher observes CLOSING and removes a late pending bit itself.
    void clearPending(uint32_t index, bool loans = false) {
        const auto mask = ~bit(index);
        for (uint32_t i = 0; i < header->prefix.slots; ++i) {
            auto& current = slot(i);
            SlotLease lease;
            if (!lease.acquire(current, true)) continue;
            __atomic_fetch_and(&current.pending, mask, __ATOMIC_ACQ_REL);
            if (loans) __atomic_fetch_and(&current.loaned, mask, __ATOMIC_ACQ_REL);
        }
    }
    void tryRetireReader(uint32_t index) {
        auto& reader = header->readers[index];
        ReaderLease retirement;
        if (!retirement.acquire(reader, true) ||
                load(reader.gate) != READER_CLOSING + OPERATION) return;
        // New takes and publications cannot enter CLOSING. Wait by retrying
        // maintenance, never by blocking, until every previously entered
        // operation has left before dropping unread pins. This prevents a
        // close from racing the pending-to-loaned transfer.
        clearPending(index);
        const auto mask = bit(index);
        for (uint32_t i = 0; i < header->prefix.slots; ++i) {
            const auto& current = slot(i);
            if ((load(current.pending) | load(current.loaned)) & mask) return;
        }
        // Keep our own operation lease throughout the scan. Without it a
        // second closer could retire/reopen/close this index while we pause,
        // making an old CLOSING-only CAS retire an unrelated registration.
        uint64_t expected = READER_CLOSING + OPERATION;
        if (cas(reader.gate, expected, READER_RETIRED)) {
            retirement.value = nullptr; // The successful CAS consumed it.
            store(reader.active, uint32_t(0));
        }
    }
    void closeRegistration(uint32_t index) {
        auto& reader = header->readers[index];
        store(reader.accepting, uint32_t(0));
        auto gate = load(reader.gate);
        while ((gate & PHASE_MASK) == READER_OPEN &&
                !cas(reader.gate, gate, (gate & ~PHASE_MASK) | READER_CLOSING)) {}
        tryRetireReader(index);
        notify(header);
    }
    void removeReader(uint32_t index) {
        closeRegistration(index);
        auto& reader = header->readers[index];
        // A killed process can have died while owning a short slot lease.
        // There is no safe way to guess/subtract its count. Retire the entire
        // session conservatively rather than silently losing capacity or
        // overwriting an old Loan. Long-lived Loans do not hold these gates.
        if (load(reader.gate) & ~PHASE_MASK) { poison(); return; }
        clearPending(index, true);
        tryRetireReader(index);
        notify(header);
    }
    void reapReaders() {
        for (uint32_t i = 0; i < header->prefix.max_readers; ++i) {
            const auto& r = header->readers[i];
            if (load(r.active) && processState(r.process.pid, r.process.start) == ProcessState::DEAD)
                removeReader(i);
            else if (load(r.active) && !load(r.accepting)) {
                tryRetireReader(i);
            }
        }
    }
    void markClosed() {
        store(header->alive, uint32_t(0));
        for (uint32_t i = 0; i < header->prefix.slots; ++i)
            store(slot(i).writer_held, uint32_t(0));
        notify(header);
    }
    void checkWriter() {
        const auto& p = header->prefix.creator;
        if (load(header->alive) && processState(p.pid, p.start) == ProcessState::DEAD) markClosed();
    }
    bool finishRegistration(uint32_t index) {
        auto& reader = header->readers[index];
        if (load(reader.ready) == 1) return true;
        ReaderLease registration;
        if (!registration.acquire(reader)) return false;
        uint32_t expected = 0;
        if (!cas(reader.ready, expected, uint32_t(2))) return expected == 1;
        // An old publication might have captured targets before OPEN. Its
        // release of publishing must be observed before the replay snapshot.
        // Future publications already see this OPEN registration. Never wait
        // for that old publisher or hold the metadata mutex on this path.
        if (load(header->publishing)) { store(reader.ready, uint32_t(0)); return false; }
        const auto watermark = load(header->last_sequence);
        if (load(reader.replay)) {
            bool busy = false;
            const auto snapshot = historySnapshot([&](uint32_t, Slot& current) {
                if (!load(current.writer_held) || load(current.sequence) > watermark) return;
                SlotLease lease;
                if (!lease.acquire(current)) { busy = true; return; }
                // The Writer may have released or reused this node between
                // selection and the lease. Recheck under the lease before
                // adding a persistent pin; the epoch also validates reuse.
                if (load(current.writer_held) && load(current.sequence) <= watermark) {
                    __atomic_fetch_or(&current.pending, bit(index), __ATOMIC_ACQ_REL);
                    if (!load(reader.accepting))
                        __atomic_fetch_and(&current.pending, ~bit(index), __ATOMIC_ACQ_REL);
                }
            });
            if (busy || snapshot != HistorySnapshot::STABLE) {
                store(reader.ready, uint32_t(0));
                if (snapshot == HistorySnapshot::CORRUPT) poison();
                return false;
            }
        }
        store(reader.ready, uint32_t(1));
        notify(header);
        return true;
    }

};

std::shared_ptr<SharedPayloadPool> SharedPayloadPool::create(const GUID_t& writer,
        const SharedPoolConfig& config, const std::string& directory) try {
    auto memory = layout(config);
    if (writer == GUID_t{}) throw std::invalid_argument("Shared pool requires Writer GUID");
    if (directory.empty() || directory[0] != '/' || directory.find('\0') != std::string::npos)
        throw std::invalid_argument("Shared directory must be absolute and contain no NUL");
    auto pool = std::shared_ptr<SharedPayloadPool>(new SharedPayloadPool);
    pool->impl_ = std::make_unique<Impl>();
    auto& impl = *pool->impl_;
    impl.owner_pid = getpid();
    const auto creator = identity();
    auto& d = pool->descriptor_;
    d.writer = writer;
    d.config = config;
    d.segment_bytes = memory.bytes;
    size_t filled = 0;
    while (filled < sizeof(d.session)) {
        ssize_t rc = getrandom(reinterpret_cast<char*>(&d.session) + filled, sizeof(d.session) - filled, 0);
        if (rc < 0 && errno == EINTR) continue;
        if (rc <= 0) syscheck(rc < 0 ? errno : EIO, "Shared session randomness");
        filled += size_t(rc);
    }
    impl.name = filename(writer, d.session);
    d.path = directory + (directory.back() == '/' ? "" : "/") + impl.name;
    impl.directory_fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (impl.directory_fd < 0) syscheck(errno, "Open shared directory");
    impl.openDevice(true);
    struct stat status{};
    if (fstat(impl.fd, &status)) syscheck(errno, "Stat shared segment");
    impl.device = status.st_dev;
    impl.inode = status.st_ino;
    impl.owner = true;
    fileLock(impl.fd, LOCK_EX);
    // Reserve backing storage before touching the mapping, so ENOSPC is an
    // ordinary creation error rather than a later SIGBUS in a Reader.
    if (impl.shared) impl.shared->truncate(off_t(memory.bytes));
    else impl.file->truncate(off_t(memory.bytes));
    // Reserve even when Boost's platform truncate only calls ftruncate.
    syscheck(posix_fallocate(impl.fd, 0, off_t(memory.bytes)), "Reserve shared segment");
    impl.memory = memory;
    impl.mapDevice(memory.bytes);
    impl.header = new (impl.mapping) Header{};
    auto* h = impl.header;
    pthread_mutexattr_t ma;
    syscheck(pthread_mutexattr_init(&ma), "Initialize shared mutex attributes");
    int rc = pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
    if (!rc) rc = pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);
    if (!rc) rc = pthread_mutex_init(&h->mutex, &ma);
    pthread_mutexattr_destroy(&ma);
    syscheck(rc, "Initialize shared robust mutex");
    for (uint32_t i = 0; i < config.slots; ++i) {
        auto* current = new (&impl.slot(i)) Slot{};
        current->history_previous = NO_SLOT;
        current->history_next = NO_SLOT;
    }
    h->history_head = h->history_tail = NO_SLOT;
    auto& p = h->prefix;
    p.version = VERSION;
    p.header_bytes = sizeof(Header);
    p.slot_bytes = sizeof(Slot);
    p.mutex_bytes = sizeof(pthread_mutex_t);
    p.notification_bytes = sizeof(uint32_t);
    p.payload_size = config.payload_size;
    p.slots = config.slots;
    p.max_readers = config.max_readers;
    p.alignment = config.alignment;
    p.segment_bytes = memory.bytes;
    guidBytes(writer, p.writer);
    p.session = d.session;
    p.creator = creator;
    struct stat pidNamespace{};
    if (stat("/proc/self/ns/pid", &pidNamespace)) syscheck(errno, "Read PID namespace");
    p.pid_namespace_device = pidNamespace.st_dev;
    p.pid_namespace_inode = pidNamespace.st_ino;
    h->alive = 1;
    for (uint32_t i = 0; i < config.max_readers; ++i) h->readers[i].gate = READER_RETIRED;
    p.magic = MAGIC;
    impl.initialized = true;
    fileLock(impl.fd, LOCK_UN);
    return pool;
} catch (const InterprocessType::interprocess_exception& error) {
    throw std::system_error(error.get_native_error() ? error.get_native_error() : EIO,
        std::generic_category(), error.what());
}

std::string SharedPayloadPool::pathFor(const GUID_t& writer, const SharedPoolSession& session,
        const std::string& directory) {
    if (directory.empty() || directory.front() != '/' || directory.find('\0') != std::string::npos)
        throw std::invalid_argument("Shared directory must be absolute and contain no NUL");
    return directory + (directory.back() == '/' ? "" : "/") + filename(writer, session);
}

std::shared_ptr<SharedPayloadPool> SharedPayloadPool::open(const SharedPoolDescriptor& descriptor) try {
    const auto memory = layout(descriptor.config);
    if (descriptor.segment_bytes != memory.bytes) throw std::invalid_argument("Shared segment descriptor length mismatch");
    std::string directory, name;
    splitPath(descriptor.path, directory, name);
    if (name != filename(descriptor.writer, descriptor.session)) throw std::invalid_argument("Shared segment name/session mismatch");
    auto pool = std::shared_ptr<SharedPayloadPool>(new SharedPayloadPool);
    pool->descriptor_ = descriptor;
    pool->impl_ = std::make_unique<Impl>();
    auto& impl = *pool->impl_;
    impl.directory_fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (impl.directory_fd < 0) syscheck(errno, "Open shared directory");
    impl.name = name;
    impl.openDevice(false);
    struct stat status{};
    if (fstat(impl.fd, &status)) syscheck(errno, "Stat shared segment");
    struct stat named{};
    if (fstatat(impl.directory_fd, name.c_str(), &named, AT_SYMLINK_NOFOLLOW) ||
        !S_ISREG(named.st_mode) || named.st_dev != status.st_dev || named.st_ino != status.st_ino ||
        !S_ISREG(status.st_mode) || status.st_uid != geteuid() || (status.st_mode & 0022) ||
        status.st_size != off_t(memory.bytes)) throw std::invalid_argument("Invalid shared segment file/size/permissions");
    fileLock(impl.fd, LOCK_SH | LOCK_NB);
    Prefix p{};
    if (pread(impl.fd, &p, sizeof(p), 0) != ssize_t(sizeof(p))) throw std::invalid_argument("Truncated shared segment header");
    const auto self = identity();
    static_cast<void>(self);
    struct stat pidNamespace{};
    if (stat("/proc/self/ns/pid", &pidNamespace)) syscheck(errno, "Read Reader PID namespace");
    unsigned char writer[16];
    guidBytes(descriptor.writer, writer);
    if (p.magic != MAGIC || p.version != VERSION || p.header_bytes != sizeof(Header) || p.slot_bytes != sizeof(Slot) ||
        p.mutex_bytes != sizeof(pthread_mutex_t) || p.notification_bytes != sizeof(uint32_t) ||
        p.payload_size != descriptor.config.payload_size || p.slots != descriptor.config.slots ||
        p.max_readers != descriptor.config.max_readers || p.alignment != descriptor.config.alignment ||
        p.segment_bytes != memory.bytes || !(p.session == descriptor.session) || std::memcmp(p.writer, writer, 16) ||
        p.creator.pid <= 0 || !p.creator.start ||
        p.pid_namespace_device != uint64_t(pidNamespace.st_dev) || p.pid_namespace_inode != uint64_t(pidNamespace.st_ino)) throw std::invalid_argument("Incompatible shared segment header/session");
    impl.memory = memory;
    impl.mapDevice(memory.bytes);
    impl.header = static_cast<Header*>(impl.mapping);
    if (std::memcmp(&p, &impl.header->prefix, sizeof(p))) throw std::invalid_argument("Shared header changed during mapping");
    // A Reader needs writable registration/pin metadata, but never a writable
    // native payload. File-backed MAP_SHARED pages still alias Writer storage;
    // page protection prevents accidental stores through this entire mapping.
    if (mprotect(static_cast<char*>(impl.mapping) + memory.payload,
            memory.bytes - memory.payload, PROT_READ)) syscheck(errno, "Protect Reader payload pages");
    impl.device = status.st_dev;
    impl.inode = status.st_ino;
    impl.initialized = true;
    fileLock(impl.fd, LOCK_UN);
    return pool;
} catch (const InterprocessType::interprocess_exception& error) {
    throw std::system_error(error.get_native_error() ? error.get_native_error() : EIO,
        std::generic_category(), error.what());
}

SharedPayloadPool::~SharedPayloadPool() {
    if (impl_ && impl_->initialized && impl_->owner && impl_->owner_pid == getpid()) closeWriter();
}
void SharedPayloadPool::unlinkOwnedFile() { impl_->unlinkFile(); }
void SharedPayloadPool::closeWriter() {
    if (!impl_->owner || impl_->owner_pid != getpid()) return;
    impl_->markClosed();
    unlinkOwnedFile();
}
bool SharedPayloadPool::removeOrphan(const SharedPoolDescriptor& descriptor) {
    auto pool = open(descriptor);
    auto& impl = *pool->impl_;
    const auto& process = impl.header->prefix.creator;
    if (processState(process.pid, process.start) != ProcessState::DEAD) return false;
    impl.markClosed();
    impl.unlinkFile();
    return true;
}

SharedPoolResult SharedPayloadPool::loan(uint32_t size, SharedSampleToken& token, char*& buffer,
        std::chrono::milliseconds timeout) {
    buffer = nullptr;
    token = {};
    auto& impl = *impl_;
    if (!impl.owner || impl.owner_pid != getpid() || size > descriptor_.config.payload_size || !validTimeout(timeout)) return SharedPoolResult::INVALID;
    const auto deadline = Clock::now() + timeout;
    for (;;) {
        const auto observed = load(impl.header->changed);
        const auto health = impl.health();
        if (health != SharedPoolResult::OK) return health;
        const auto start = load(impl.header->ring_tail);
        if (start == UINT64_MAX || load(impl.header->history_epoch) >= UINT64_MAX - 1)
            return SharedPoolResult::EXHAUSTED;
        bool retryReservation = false;
        for (uint32_t i = 0; i < descriptor_.config.slots; ++i) {
            // UINT64_MAX is the exhausted cursor, never an issued position.
            if (uint64_t(i) >= UINT64_MAX - start) break;
            const auto position = start + i;
            const auto index = uint32_t(position % descriptor_.config.slots);
            auto& current = impl.slot(index);
            // Do not freeze a slot carrying any unread delivery: a scanner
            // must never temporarily skip an earlier pending sequence.
            if (!unpinned(current)) continue;
            const bool linked = load(current.history_linked) != 0;
            Publication publication(impl.header, linked);
            if (linked && !publication.held) continue;
            // A linked cell must become unstable before it becomes FROZEN.
            // Thus a stable even snapshot can reject a malformed state rather
            // than silently hiding it with a fallback scan.
            HistoryMutation mutation(impl.header, linked);
            if (mutation.result != SharedPoolResult::OK) return mutation.result;
            auto gate = load(current.gate);
            if ((gate & ~PHASE_MASK) || (gate & PHASE_MASK) == FROZEN || !cas(current.gate, gate, FROZEN)) continue;
            if (!unpinned(current) || load(current.generation) == UINT64_MAX) {
                store(current.gate, gate); continue;
            }
            // A publisher may have linked a previously unlinked candidate
            // before its freeze. Retry through the structural mutation lane.
            if (!linked && load(current.history_linked)) {
                store(current.gate, gate); retryReservation = true; break;
            }
            auto expected = start;
            if (!cas(impl.header->ring_tail, expected, position + 1)) {
                store(current.gate, gate); retryReservation = true; break;
            }
            if (linked && !impl.unlinkHistory(index)) {
                store(current.gate, gate);
                impl.poison();
                return SharedPoolResult::CORRUPT;
            }
            const auto generation = load(current.generation) + 1;
            store(current.generation, generation);
            store(current.ring_position, position);
            store(current.writer_held, uint32_t(1));
            store(current.pending, uint64_t(0));
            store(current.loaned, uint64_t(0));
            store(current.sequence, uint64_t(0));
            current.size = size;
            store(current.state, uint32_t(WRITING));
            store(current.gate, uint64_t(WRITING));
            token = {descriptor_.session, index, generation};
            buffer = impl.payload(index);
            return SharedPoolResult::OK;
        }
        if (Clock::now() >= deadline) return timeout.count() ? SharedPoolResult::TIMEOUT : SharedPoolResult::EXHAUSTED;
        if (retryReservation) continue;
        sleepUntil(impl.header, observed, nextWake(deadline));
    }
}
SharedPoolResult SharedPayloadPool::publish(const SharedSampleToken& token, uint32_t size, uint64_t sequence,
        const SharedSampleMetadata& metadata, const std::vector<GUID_t>* readers) {
    auto& impl = *impl_;
    if (!impl.owner || impl.owner_pid != getpid() || size > descriptor_.config.payload_size || !sequence ||
            !(token.session == descriptor_.session) || token.slot >= descriptor_.config.slots) return SharedPoolResult::INVALID;
    const auto health = impl.health();
    if (health != SharedPoolResult::OK) return health;
    // DDS serializes publication with its existing Writer lane. Direct callers
    // contending for this lane fail immediately, rather than waiting for a
    // stopped publisher or a metadata-lock holder.
    Publication publication(impl.header);
    if (!publication.held) return SharedPoolResult::EXHAUSTED;
    auto& current = impl.slot(token.slot);
    SlotLease lease;
    if (!lease.acquire(current, true) || !impl.tokenMatches(token) ||
            (load(current.gate) & PHASE_MASK) != WRITING || !load(current.writer_held) || size > current.size ||
            sequence <= load(impl.header->last_sequence)) return SharedPoolResult::INVALID;
    HistoryMutation mutation(impl.header);
    if (mutation.result != SharedPoolResult::OK) return mutation.result;
    // Every possible rejection precedes append's first link store. Once the
    // slot is appended, the remainder of commit cannot fail or copy payload.
    if (!impl.appendHistory(token.slot)) {
        impl.poison();
        return SharedPoolResult::CORRUPT;
    }
    ReaderLease registrations[MAX_READERS];
    uint64_t pending = 0;
    for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i) {
        auto& reader = impl.header->readers[i];
        if (!registrations[i].acquire(reader)) continue;
        GUID_t guid; memcpy(&guid, reader.guid, 16);
        if (load(reader.accepting) && (!readers || std::find(readers->begin(), readers->end(), guid) != readers->end()))
            pending |= bit(i);
        else registrations[i].reset();
    }
    current.size = size;
    store(current.sequence, sequence);
    current.metadata = metadata;
    store(current.pending, pending);
    // A close racing target selection either clears the bit itself or is
    // observed here; the registration lease prevents identity reuse meanwhile.
    for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i)
        if ((pending & bit(i)) && !load(impl.header->readers[i].accepting))
            __atomic_fetch_and(&current.pending, ~bit(i), __ATOMIC_ACQ_REL);
    slotPhase(current, PUBLISHED);
    store(impl.header->last_sequence, sequence);
    notify(impl.header);
    return SharedPoolResult::OK;
}
bool SharedPayloadPool::releaseWriter(const SharedSampleToken& token) {
    auto& impl = *impl_;
    if (!impl.owner || impl.owner_pid != getpid() || !(token.session == descriptor_.session) || token.slot >= descriptor_.config.slots) return false;
    auto& current = impl.slot(token.slot);
    SlotLease lease;
    if (!lease.acquire(current, true) || !impl.tokenMatches(token)) return false;
    const bool released = __atomic_exchange_n(&current.writer_held, uint32_t(0), __ATOMIC_ACQ_REL) != 0;
    if (released) notify(impl.header);
    return released;
}
bool SharedPayloadPool::getBuffer(uint32_t size, char*& buffer) {
    SharedSampleToken token;
    return loan(size, token, buffer) == SharedPoolResult::OK;
}
bool SharedPayloadPool::releaseBuffer(char*& buffer) {
    auto& impl = *impl_;
    if (!impl.owner || impl.owner_pid != getpid() || !buffer) return false;
    const auto begin = reinterpret_cast<uintptr_t>(impl.payload(0));
    const auto address = reinterpret_cast<uintptr_t>(buffer);
    if (address < begin || address - begin >= impl.memory.stride * descriptor_.config.slots ||
        (address - begin) % impl.memory.stride) return false;
    const auto index = uint32_t((address - begin) / impl.memory.stride);
    auto& current = impl.slot(index);
    SlotLease lease;
    if (!lease.acquire(current, true) || !__atomic_exchange_n(&current.writer_held, uint32_t(0), __ATOMIC_ACQ_REL)) return false;
    buffer = nullptr;
    notify(impl.header);
    return true;
}

std::shared_ptr<SharedPoolReader> SharedPayloadPool::registerReader(const GUID_t& guid, bool replay) {
    const auto process = identity();
    auto& impl = *impl_;
    Guard guard(impl.header);
    if (guard.result != SharedPoolResult::OK) return nullptr;
    impl.checkWriter();
    if (impl.health() != SharedPoolResult::OK) return nullptr;
    impl.reapReaders();
    if (impl.health() != SharedPoolResult::OK) return nullptr;
    if (guid != GUID_UNKNOWN) for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i) {
        auto& r = impl.header->readers[i];
        ReaderLease lease;
        if (lease.acquire(r) && load(r.accepting) && memcmp(r.guid, &guid, 16) == 0) return nullptr;
    }
    for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i) {
        auto& r = impl.header->readers[i];
        if (load(r.active) || load(r.gate) != READER_RETIRED || load(r.generation) == UINT64_MAX) continue;
        const auto generation = load(r.generation) + 1;
        auto reader = std::shared_ptr<SharedPoolReader>(new SharedPoolReader(shared_from_this(), i, generation));
        r.process = process;
        store(r.generation, generation);
        store(r.active, uint32_t(1));
        store(r.accepting, uint32_t(1));
        store(r.replay, replay ? uint32_t(1) : uint32_t(0));
        store(r.ready, replay ? uint32_t(0) : uint32_t(1));
        memcpy(r.guid, &guid, 16);
        store(r.gate, READER_OPEN);
        reader->registered_ = true;
        if (replay && !impl.finishRegistration(i) && load(impl.header->poisoned)) return nullptr;
        return reader;
    }
    return nullptr;
}
std::vector<GUID_t> SharedPayloadPool::registeredReaders(bool replayOnly) {
    std::vector<GUID_t> readers;
    auto& impl = *impl_;
    if (impl.health() != SharedPoolResult::OK) return readers;
    for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i) {
        auto& r = impl.header->readers[i];
        ReaderLease lease;
        if (!lease.acquire(r) || !load(r.accepting) || load(r.ready) != 1 ||
                (replayOnly && !load(r.replay))) continue;
        GUID_t guid; memcpy(&guid, r.guid, 16);
        if (guid != GUID_UNKNOWN) readers.push_back(guid);
    }
    return readers;
}
SharedPoolResult SharedPayloadPool::waitNext(const std::shared_ptr<SharedPoolReader>& reader,
        uint64_t after, std::chrono::milliseconds timeout, SharedReadLoan& output) {
    if (output || !validTimeout(timeout)) return SharedPoolResult::INVALID;
    Consumer consumer(reader->consuming_);
    if (!consumer.held) return SharedPoolResult::TIMEOUT;
    auto& impl = *impl_;
    const auto deadline = Clock::now() + timeout;
    for (;;) {
        const auto observed = load(impl.header->changed);
        if (load(impl.header->poisoned)) return SharedPoolResult::CORRUPT;
        if (reader->index_ >= descriptor_.config.max_readers) return SharedPoolResult::INVALID;
        auto& registration = impl.header->readers[reader->index_];
        ReaderLease operation;
        if (!operation.acquire(registration, false, reader->generation_)) {
            if (load(registration.generation) == reader->generation_ && !load(registration.accepting)) return SharedPoolResult::CLOSED;
            return SharedPoolResult::INVALID;
        }
        if (!load(registration.accepting)) return SharedPoolResult::CLOSED;
        const bool ready = impl.finishRegistration(reader->index_);
        if (load(impl.header->poisoned)) return SharedPoolResult::CORRUPT;
        // A close observed before this snapshot makes its final watermark
        // visible. If close happens after the scan, rescan before reporting
        // CLOSED so its last pending publications are drained.
        const bool writerAlive = load(impl.header->alive) != 0;
        const auto watermark = load(impl.header->last_sequence);
        uint32_t selected = descriptor_.config.slots;
        uint64_t sequence = UINT64_MAX, generation = 0;
        const auto mask = bit(reader->index_);
        bool retryScan = false;
        if (ready) {
            // The normal delivery path follows the shared published ring.
            // Only an in-progress/changed epoch permits the conservative
            // physical scan below; a stable malformed ring is an error.
            const auto snapshot = impl.historySnapshot([&](uint32_t index, Slot& current) {
                const auto candidate = load(current.sequence);
                if (selected == descriptor_.config.slots && (load(current.pending) & mask) &&
                        candidate > after && candidate <= watermark) {
                    selected = index; sequence = candidate; generation = load(current.generation);
                }
            });
            if (snapshot == HistorySnapshot::CORRUPT) return SharedPoolResult::CORRUPT;
            if (snapshot == HistorySnapshot::RETRY) {
                selected = descriptor_.config.slots;
                sequence = UINT64_MAX;
                generation = 0;
                for (uint32_t i = 0; i < descriptor_.config.slots; ++i) {
                    auto& current = impl.slot(i);
                    // An unselected node must not acquire a short pin either:
                    // this fallback can race the reserving Writer's odd epoch.
                    if (!(load(current.pending) & mask)) continue;
                    SlotLease lease;
                    if (!lease.acquire(current)) {
                        // An earlier pending sample may be in reserve's
                        // frozen/recheck window. Never skip it or wait on
                        // the stopped mutator's gate.
                        if (load(current.pending) & mask) retryScan = true;
                        continue;
                    }
                    const auto candidate = load(current.sequence);
                    if ((load(current.pending) & mask) && candidate > after && candidate <= watermark &&
                            (selected == descriptor_.config.slots || candidate < sequence)) {
                        selected = i; sequence = candidate; generation = load(current.generation);
                    }
                }
            }
        }
        if (!retryScan && selected != descriptor_.config.slots) {
            auto& current = impl.slot(selected);
            SlotLease lease;
            if (lease.acquire(current) && load(current.generation) == generation &&
                    (load(current.pending) & mask)) {
                const auto oldLoans = __atomic_fetch_or(&current.loaned, mask, __ATOMIC_ACQ_REL);
                if (!(oldLoans & mask)) {
                    const auto oldPending = __atomic_fetch_and(&current.pending, ~mask, __ATOMIC_ACQ_REL);
                    if (oldPending & mask) {
                        output.reader_ = reader;
                        output.token_ = {descriptor_.session, selected, generation};
                        output.data_ = impl.payload(selected);
                        output.size_ = current.size;
                        output.sequence_ = sequence;
                        output.metadata_ = current.metadata;
                        return SharedPoolResult::OK;
                    }
                    __atomic_fetch_and(&current.loaned, ~mask, __ATOMIC_ACQ_REL);
                }
            }
            // A reserve/recheck window may have intervened. Recheck the
            // deadline below rather than spinning past a zero timeout.
            retryScan = true;
        }
        operation.reset();
        if (!writerAlive && !retryScan) return SharedPoolResult::CLOSED;
        if (writerAlive && !load(impl.header->alive)) continue;
        // Death probing is an empty-reader control action, never part of
        // Writer reserve/commit/return or registeredReaders target selection.
        if (writerAlive) {
            impl.checkWriter();
            if (!load(impl.header->alive)) continue;
        }
        if (Clock::now() >= deadline) return SharedPoolResult::TIMEOUT;
        sleepUntil(impl.header, observed, nextWake(deadline));
    }
}
bool SharedPayloadPool::releaseReader(const SharedPoolReader& reader, const SharedSampleToken& token, bool unread) {
    auto& impl = *impl_;
    if (reader.index_ >= descriptor_.config.max_readers || !(token.session == descriptor_.session) || token.slot >= descriptor_.config.slots) return false;
    auto& registration = impl.header->readers[reader.index_];
    // A discard competes with take for the same unread bit. Loan return
    // only clears an already-owned loan bit and may run concurrently.
    Consumer consumer(reader.consuming_, unread);
    if (unread && !consumer.held) return false;
    ReaderLease operation;
    if (!operation.acquire(registration, !unread, reader.generation_)) return false;
    auto& current = impl.slot(token.slot);
    SlotLease lease;
    if (!lease.acquire(current) || !impl.tokenMatches(token)) return false;
    auto& mask = unread ? current.pending : current.loaned;
    const auto removed = __atomic_fetch_and(&mask, ~bit(reader.index_), __ATOMIC_ACQ_REL) & bit(reader.index_);
    if (removed) notify(impl.header);
    return removed != 0;
}
void SharedPayloadPool::unregisterReader(const SharedPoolReader& reader) {
    auto& impl = *impl_;
    if (reader.index_ >= descriptor_.config.max_readers) return;
    auto& registration = impl.header->readers[reader.index_];
    ReaderLease operation;
    if (!operation.acquire(registration, true, reader.generation_)) return;
    impl.closeRegistration(reader.index_);
    operation.reset();
    impl.tryRetireReader(reader.index_);
}
void SharedPayloadPool::closeReader(const SharedPoolReader& reader) {
    auto& impl = *impl_;
    if (reader.index_ >= descriptor_.config.max_readers) return;
    auto& registration = impl.header->readers[reader.index_];
    ReaderLease operation;
    if (!operation.acquire(registration, true, reader.generation_)) return;
    impl.closeRegistration(reader.index_);
    operation.reset();
    impl.tryRetireReader(reader.index_);
}
SharedPoolResult SharedPayloadPool::statistics(SharedPoolStats& stats) {
    auto& impl = *impl_;
    stats = {};
    Guard guard(impl.header, true);
    if (guard.result != SharedPoolResult::OK) return guard.result;
    impl.checkWriter();
    impl.reapReaders();
    if (load(impl.header->poisoned)) return SharedPoolResult::CORRUPT;
    stats.writer_alive = load(impl.header->alive) != 0;
    uint64_t oldestPosition = UINT64_MAX;
    bool pinnedPosition = false;
    for (uint32_t i = 0; i < descriptor_.config.slots; ++i) {
        const auto& current = impl.slot(i);
        const bool free = unpinned(current);
        const auto phase = load(current.gate) & PHASE_MASK;
        stats.free_slots += free;
        stats.writing_slots += !free && phase == WRITING;
        stats.published_slots += !free && phase == PUBLISHED;
        stats.pending_references += __builtin_popcountll(load(current.pending));
        stats.loan_references += __builtin_popcountll(load(current.loaned));
        if (!free) {
            oldestPosition = std::min(oldestPosition, load(current.ring_position));
            pinnedPosition = true;
        }
    }
    stats.ring_tail = load(impl.header->ring_tail);
    stats.ring_head = pinnedPosition ? oldestPosition : stats.ring_tail;
    uint32_t historySamples = 0;
    uint64_t firstSequence = 0, lastSequence = 0;
    const auto snapshot = impl.historySnapshot([&](uint32_t, Slot& current) {
        const auto sequence = load(current.sequence);
        if (!historySamples) firstSequence = sequence;
        ++historySamples;
        lastSequence = sequence;
    });
    if (snapshot == HistorySnapshot::CORRUPT) return SharedPoolResult::CORRUPT;
    if (snapshot == HistorySnapshot::STABLE) {
        stats.history_samples = historySamples;
        stats.history_first_sequence = firstSequence;
        stats.history_last_sequence = lastSequence;
    }
    for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i) {
        if (load(impl.header->readers[i].active)) {
            impl.finishRegistration(i);
            if (load(impl.header->poisoned)) return SharedPoolResult::CORRUPT;
            ++stats.readers;
        }
    }
    return SharedPoolResult::OK;
}

SharedPoolReader::SharedPoolReader(std::shared_ptr<SharedPayloadPool> pool, uint32_t index, uint64_t generation)
    : pool_(std::move(pool)), index_(index), generation_(generation) {}
SharedPoolReader::~SharedPoolReader() { if (registered_) pool_->unregisterReader(*this); }
SharedPoolResult SharedPoolReader::waitNext(uint64_t after, std::chrono::milliseconds timeout, SharedReadLoan& output) {
    return pool_->waitNext(shared_from_this(), after, timeout, output);
}
bool SharedPoolReader::returnLoan(SharedReadLoan& loan) {
    if (loan.reader_.get() != this) return false;
    auto keepAlive = loan.reader_;
    if (!pool_->releaseReader(*this, loan.token_, false)) return false;
    loan.reader_.reset();
    loan.reset();
    return true;
}
bool SharedPoolReader::discard(const SharedSampleToken& sample) { return pool_->releaseReader(*this, sample, true); }
void SharedPoolReader::close() { pool_->closeReader(*this); }
SharedReadLoan::~SharedReadLoan() { reset(); }
SharedReadLoan::SharedReadLoan(SharedReadLoan&& other) noexcept { *this = std::move(other); }
SharedReadLoan& SharedReadLoan::operator=(SharedReadLoan&& other) noexcept {
    if (this != &other) {
        reset();
        reader_ = std::move(other.reader_);
        token_ = other.token_;
        data_ = other.data_;
        size_ = other.size_;
        sequence_ = other.sequence_;
        metadata_ = other.metadata_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.sequence_ = 0;
        other.token_ = {};
        other.metadata_ = {};
    }
    return *this;
}
void SharedReadLoan::reset() {
    if (reader_) reader_->pool_->releaseReader(*reader_, token_, false);
    reader_.reset();
    data_ = nullptr;
    size_ = 0;
    sequence_ = 0;
    token_ = {};
    metadata_ = {};
}

TRAVODDS_NAMESPACE_END
#endif
