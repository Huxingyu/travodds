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
    l.payload = rounded(size_t(payload), c.alignment);
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
void notify(Header* header) noexcept {
    __atomic_add_fetch(&header->changed, 1u, __ATOMIC_RELEASE);
    syscall(SYS_futex, &header->changed, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
}
struct Guard {
    Header* header;
    bool held = false;
    SharedPoolResult result = SharedPoolResult::SYSTEM_ERROR;
    explicit Guard(Header* value) : header(value) { acquired(pthread_mutex_lock(&header->mutex)); }
    ~Guard() { if (held) pthread_mutex_unlock(&header->mutex); }
    void acquired(int rc) {
        if (!rc || rc == EOWNERDEAD) {
            held = true;
            if (rc == EOWNERDEAD) {
                // No uncertain partial transaction is repaired/reused. Existing
                // loans remain mapped, and all further allocation is refused.
                header->poisoned = 1;
                header->alive = 0;
                pthread_mutex_consistent(&header->mutex);
                notify(header);
            }
            result = header->poisoned ? SharedPoolResult::CORRUPT : SharedPoolResult::OK;
        } else {
            result = rc == ENOTRECOVERABLE ? SharedPoolResult::CORRUPT : SharedPoolResult::SYSTEM_ERROR;
        }
    }
    void waitUntil(const timespec& deadline) {
        // Snapshot under the metadata lock, then release it before sleeping.
        // A publication in between makes FUTEX_WAIT_BITSET return EAGAIN,
        // preventing a lost wakeup. The deadline is absolute CLOCK_MONOTONIC.
        const auto observed = __atomic_load_n(&header->changed, __ATOMIC_ACQUIRE);
        const int unlocked = pthread_mutex_unlock(&header->mutex);
        if (unlocked) { result = SharedPoolResult::SYSTEM_ERROR; return; }
        held = false;
        const long waited = syscall(SYS_futex, &header->changed, FUTEX_WAIT_BITSET,
            observed, &deadline, nullptr, FUTEX_BITSET_MATCH_ANY);
        const int error = waited < 0 ? errno : 0;
        acquired(pthread_mutex_lock(&header->mutex));
        if (result == SharedPoolResult::OK && error && error != ETIMEDOUT && error != EAGAIN && error != EINTR)
            result = SharedPoolResult::SYSTEM_ERROR;
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
void recycle(Slot& slot) {
    if (!slot.writer_held && !slot.pending && !slot.loaned) slot.state = FREE;
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
    bool tokenMatches(const SharedSampleToken& token) const {
        return token.session == header->prefix.session && token.slot < header->prefix.slots &&
            token.generation && slot(token.slot).generation == token.generation;
    }
    bool readerMatches(uint32_t index, uint64_t generation) const {
        if (index >= header->prefix.max_readers) return false;
        const auto& reader = header->readers[index];
        return reader.active && reader.generation == generation && reader.process.pid == getpid();
    }
    void removeReader(uint32_t index) {
        const auto mask = ~bit(index);
        for (uint32_t i = 0; i < header->prefix.slots; ++i) {
            auto& current = slot(i);
            current.pending &= mask;
            current.loaned &= mask;
            recycle(current);
        }
        header->readers[index].active = 0;
        notify(header);
    }
    void reapReaders() {
        for (uint32_t i = 0; i < header->prefix.max_readers; ++i) {
            const auto& r = header->readers[i];
            if (r.active && processState(r.process.pid, r.process.start) == ProcessState::DEAD) removeReader(i);
        }
    }
    void markClosed() {
        header->alive = 0;
        for (uint32_t i = 0; i < header->prefix.slots; ++i) {
            auto& current = slot(i);
            current.writer_held = 0;
            recycle(current);
        }
        notify(header);
    }
    void checkWriter() {
        const auto& p = header->prefix.creator;
        if (header->alive && processState(p.pid, p.start) == ProcessState::DEAD) markClosed();
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
    for (uint32_t i = 0; i < config.slots; ++i) new (&impl.slot(i)) Slot{};
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
    {
        Guard guard(impl_->header);
        if (guard.result == SharedPoolResult::OK) impl_->markClosed();
    }
    unlinkOwnedFile();
}
bool SharedPayloadPool::removeOrphan(const SharedPoolDescriptor& descriptor) {
    auto pool = open(descriptor);
    auto& impl = *pool->impl_;
    const auto& process = impl.header->prefix.creator;
    if (processState(process.pid, process.start) != ProcessState::DEAD) return false;
    {
        Guard guard(impl.header);
        if (guard.result == SharedPoolResult::OK) impl.markClosed();
        else if (guard.result != SharedPoolResult::CORRUPT) return false;
    }
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
    Guard guard(impl.header);
    for (;;) {
        if (guard.result != SharedPoolResult::OK) return guard.result;
        if (!impl.header->alive) return SharedPoolResult::CLOSED;
        impl.reapReaders();
        for (uint32_t i = 0; i < descriptor_.config.slots; ++i) {
            const auto index = (uint64_t(impl.header->next_slot) + i) % descriptor_.config.slots;
            auto& current = impl.slot(uint32_t(index));
            if (current.state != FREE || current.generation == std::numeric_limits<uint64_t>::max()) continue;
            ++current.generation;
            current.state = WRITING;
            current.writer_held = 1;
            current.size = size;
            current.sequence = current.pending = current.loaned = 0;
            impl.header->next_slot = uint32_t((index + 1) % descriptor_.config.slots);
            token = {descriptor_.session, uint32_t(index), current.generation};
            buffer = impl.payload(uint32_t(index));
            return SharedPoolResult::OK;
        }
        if (Clock::now() >= deadline) return timeout.count() ? SharedPoolResult::TIMEOUT : SharedPoolResult::EXHAUSTED;
        guard.waitUntil(nextWake(deadline));
    }
}
SharedPoolResult SharedPayloadPool::publish(const SharedSampleToken& token, uint32_t size, uint64_t sequence,
        const SharedSampleMetadata& metadata, const std::vector<GUID_t>* readers) {
    auto& impl = *impl_;
    if (!impl.owner || impl.owner_pid != getpid() || size > descriptor_.config.payload_size || !sequence) return SharedPoolResult::INVALID;
    Guard guard(impl.header);
    if (guard.result != SharedPoolResult::OK) return guard.result;
    if (!impl.header->alive) return SharedPoolResult::CLOSED;
    if (!impl.tokenMatches(token)) return SharedPoolResult::INVALID;
    auto& current = impl.slot(token.slot);
    if (current.state != WRITING || !current.writer_held || size > current.size ||
        sequence <= impl.header->last_sequence) return SharedPoolResult::INVALID;
    impl.reapReaders();
    uint64_t pending = 0;
    for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i) {
        const auto& r = impl.header->readers[i];
        if (!r.active || !r.accepting) continue;
        GUID_t guid; memcpy(&guid, r.guid, 16);
        if (!readers || std::find(readers->begin(), readers->end(), guid) != readers->end()) pending |= bit(i);
    }
    current.size = size;
    current.sequence = sequence;
    current.metadata = metadata;
    current.pending = pending;
    current.loaned = 0;
    current.state = PUBLISHED;
    impl.header->last_sequence = sequence;
    notify(impl.header);
    return SharedPoolResult::OK;
}
bool SharedPayloadPool::releaseWriter(const SharedSampleToken& token) {
    auto& impl = *impl_;
    if (!impl.owner || impl.owner_pid != getpid()) return false;
    Guard guard(impl.header);
    if (guard.result != SharedPoolResult::OK || !impl.tokenMatches(token)) return false;
    auto& current = impl.slot(token.slot);
    if (!current.writer_held) return false;
    current.writer_held = 0;
    recycle(current);
    notify(impl.header);
    return true;
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
    Guard guard(impl.header);
    if (guard.result != SharedPoolResult::OK) return false;
    auto& current = impl.slot(index);
    if (!current.writer_held) return false;
    current.writer_held = 0;
    recycle(current);
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
    if (!impl.header->alive) return nullptr;
    impl.reapReaders();
    if (guid != GUID_UNKNOWN) {
        for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i) {
            const auto& r = impl.header->readers[i];
            if (r.active && r.accepting && memcmp(r.guid, &guid, 16) == 0) return nullptr;
        }
    }
    for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i) {
        auto& r = impl.header->readers[i];
        if (r.active || r.generation == std::numeric_limits<uint64_t>::max()) continue;
        // Allocate the local registration before publishing any shared state.
        auto reader = std::shared_ptr<SharedPoolReader>(new SharedPoolReader(shared_from_this(), i, r.generation + 1));
        r.process = process;
        ++r.generation;
        r.active = 1;
        r.accepting = 1;
        r.replay = replay ? 1u : 0u;
        memcpy(r.guid, &guid, 16);
        if (replay) for (uint32_t slot = 0; slot < descriptor_.config.slots; ++slot) {
            auto& sample = impl.slot(slot);
            if (sample.state == PUBLISHED && sample.writer_held) sample.pending |= bit(i);
        }
        reader->registered_ = true;
        return reader;
    }
    return nullptr;
}
std::vector<GUID_t> SharedPayloadPool::registeredReaders(bool replayOnly) {
    std::vector<GUID_t> readers;
    auto& impl = *impl_;
    Guard guard(impl.header);
    if (guard.result != SharedPoolResult::OK) return readers;
    impl.reapReaders();
    for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i) {
        const auto& r = impl.header->readers[i];
        if (!r.active || !r.accepting || (replayOnly && !r.replay)) continue;
        GUID_t guid; memcpy(&guid, r.guid, 16);
        if (guid != GUID_UNKNOWN) readers.push_back(guid);
    }
    return readers;
}
SharedPoolResult SharedPayloadPool::waitNext(const std::shared_ptr<SharedPoolReader>& reader,
        uint64_t after, std::chrono::milliseconds timeout, SharedReadLoan& output) {
    if (output || !validTimeout(timeout)) return SharedPoolResult::INVALID;
    auto& impl = *impl_;
    const auto deadline = Clock::now() + timeout;
    Guard guard(impl.header);
    for (;;) {
        if (guard.result != SharedPoolResult::OK) return guard.result;
        if (!impl.readerMatches(reader->index_, reader->generation_)) return SharedPoolResult::INVALID;
        if (!impl.header->readers[reader->index_].accepting) return SharedPoolResult::CLOSED;
        impl.checkWriter();
        uint32_t selected = descriptor_.config.slots;
        uint64_t sequence = std::numeric_limits<uint64_t>::max();
        for (uint32_t i = 0; i < descriptor_.config.slots; ++i) {
            const auto& current = impl.slot(i);
            if (current.state == PUBLISHED && (current.pending & bit(reader->index_)) &&
                current.sequence > after && (selected == descriptor_.config.slots || current.sequence < sequence)) {
                selected = i;
                sequence = current.sequence;
            }
        }
        if (selected != descriptor_.config.slots) {
            auto& current = impl.slot(selected);
            current.pending &= ~bit(reader->index_);
            current.loaned |= bit(reader->index_);
            output.reader_ = reader;
            output.token_ = {descriptor_.session, selected, current.generation};
            output.data_ = impl.payload(selected);
            output.size_ = current.size;
            output.sequence_ = current.sequence;
            output.metadata_ = current.metadata;
            return SharedPoolResult::OK;
        }
        if (!impl.header->alive) return SharedPoolResult::CLOSED;
        if (Clock::now() >= deadline) return SharedPoolResult::TIMEOUT;
        guard.waitUntil(nextWake(deadline));
    }
}
bool SharedPayloadPool::releaseReader(const SharedPoolReader& reader, const SharedSampleToken& token, bool unread) {
    auto& impl = *impl_;
    Guard guard(impl.header);
    if (guard.result != SharedPoolResult::OK || !impl.readerMatches(reader.index_, reader.generation_) || !impl.tokenMatches(token)) return false;
    auto& current = impl.slot(token.slot);
    auto& mask = unread ? current.pending : current.loaned;
    if (!(mask & bit(reader.index_))) return false;
    mask &= ~bit(reader.index_);
    recycle(current);
    notify(impl.header);
    return true;
}
void SharedPayloadPool::unregisterReader(const SharedPoolReader& reader) {
    auto& impl = *impl_;
    Guard guard(impl.header);
    if (guard.result == SharedPoolResult::OK && impl.readerMatches(reader.index_, reader.generation_)) impl.removeReader(reader.index_);
}
void SharedPayloadPool::closeReader(const SharedPoolReader& reader) {
    auto& impl = *impl_;
    Guard guard(impl.header);
    if (guard.result != SharedPoolResult::OK || !impl.readerMatches(reader.index_, reader.generation_)) return;
    impl.header->readers[reader.index_].accepting = 0;
    for (uint32_t i = 0; i < descriptor_.config.slots; ++i) {
        auto& sample = impl.slot(i);
        sample.pending &= ~bit(reader.index_);
        recycle(sample);
    }
    notify(impl.header);
}
SharedPoolResult SharedPayloadPool::statistics(SharedPoolStats& stats) {
    auto& impl = *impl_;
    stats = {};
    Guard guard(impl.header);
    if (guard.result != SharedPoolResult::OK) return guard.result;
    impl.checkWriter();
    stats.writer_alive = impl.header->alive;
    for (uint32_t i = 0; i < descriptor_.config.slots; ++i) {
        const auto& current = impl.slot(i);
        stats.free_slots += current.state == FREE;
        stats.writing_slots += current.state == WRITING;
        stats.published_slots += current.state == PUBLISHED;
        stats.pending_references += __builtin_popcountll(current.pending);
        stats.loan_references += __builtin_popcountll(current.loaned);
    }
    for (uint32_t i = 0; i < descriptor_.config.max_readers; ++i) stats.readers += impl.header->readers[i].active != 0;
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
