#pragma once
#include "shared_pool.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <linux/futex.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

namespace travodds_experimental {

// This mapping owner is process-local. Shared Pool::State contains no pointers.
class Mapping {
public:
    static std::shared_ptr<Mapping> shared(size_t bytes, int fd = -1) {
        void* address = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
            MAP_SHARED | (fd < 0 ? MAP_ANONYMOUS : 0), fd, 0);
        if (address == MAP_FAILED) throw std::system_error(errno, std::generic_category(), "mmap prototype pool");
        Mapping* owner = nullptr;
        try { owner = new Mapping(address, bytes); }
        catch (...) { munmap(address, bytes); throw; }
        // shared_ptr deletes owner itself if its control-block allocation fails.
        return std::shared_ptr<Mapping>(owner);
    }
    ~Mapping() { munmap(address_, bytes_); }
    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;
    void* address() const { return address_; }
    size_t bytes() const { return bytes_; }
private:
    Mapping(void* address, size_t bytes) : address_(address), bytes_(bytes) {}
    void* address_;
    size_t bytes_;
};

enum class WaitEvent { Woken, Changed, Interrupted, TimedOut, Error };
inline bool wake_all(uint32_t& changed) {
    return syscall(SYS_futex, &changed, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0) >= 0;
}
inline WaitEvent wait_once(uint32_t& changed, uint32_t expected,
        std::chrono::steady_clock::time_point deadline) {
    using namespace std::chrono;
    const auto remaining = std::max(nanoseconds(1), std::min(
        duration_cast<nanoseconds>(deadline - steady_clock::now()), duration_cast<nanoseconds>(milliseconds(100))));
    timespec absolute{};
    if (clock_gettime(CLOCK_MONOTONIC, &absolute)) return WaitEvent::Error;
    const auto nanos = int64_t(absolute.tv_nsec) + remaining.count();
    absolute.tv_sec += nanos / 1000000000;
    absolute.tv_nsec = nanos % 1000000000;
    const auto result = syscall(SYS_futex, &changed, FUTEX_WAIT_BITSET, expected,
        &absolute, nullptr, FUTEX_BITSET_MATCH_ANY);
    if (result >= 0) return WaitEvent::Woken;
    if (errno == EAGAIN) return WaitEvent::Changed;
    if (errno == EINTR) return WaitEvent::Interrupted;
    if (errno == ETIMEDOUT) return WaitEvent::TimedOut;
    return WaitEvent::Error;
}

template<class PoolType>
class MappedPool {
public:
    using State = typename PoolType::State;
    class Loan {
    public:
        Loan() = default;
        ~Loan() { reset(); }
        Loan(const Loan&) = delete;
        Loan& operator=(const Loan&) = delete;
        Loan(Loan&& other) noexcept { *this = std::move(other); }
        Loan& operator=(Loan&& other) noexcept {
            if (this != &other) {
                reset(); owner_ = std::move(other.owner_); sample_ = other.sample_; other.sample_ = {};
            }
            return *this;
        }
        explicit operator bool() const { return sample_.data != nullptr; }
        const unsigned char* data() const { return sample_.data; }
        uint32_t size() const { return sample_.size; }
        uint64_t sequence() const { return sample_.sequence; }
        Token token() const { return sample_.token; }
        uint32_t reader() const { return sample_.reader; }
        Result reset() noexcept {
            Result result = Result::Invalid;
            if (sample_.data && owner_) {
                PoolType pool(*static_cast<State*>(owner_->address()));
                result = pool.release_reader(sample_.token, sample_.reader);
            }
            sample_ = {}; owner_.reset(); return result;
        }
    private:
        std::shared_ptr<Mapping> owner_;
        typename PoolType::Sample sample_;
        friend class MappedPool;
    };

    static MappedPool create(Session session) {
        auto owner = Mapping::shared(sizeof(State));
        auto* state = PoolType::initialize(owner->address(), owner->bytes(), session);
        if (!state) throw std::invalid_argument("Invalid prototype initialization");
        return MappedPool(std::move(owner));
    }
    static MappedPool attach(std::shared_ptr<Mapping> owner, Session session) {
        if (!owner || !PoolType::compatible(static_cast<State*>(owner->address()), owner->bytes(), session))
            throw std::invalid_argument("Incompatible prototype mapping/session");
        return MappedPool(std::move(owner));
    }
    PoolType core() const { return PoolType(*static_cast<State*>(owner_->address())); }
    std::shared_ptr<Mapping> mapping() const { return owner_; }
    bool notify() const { return wake_all(core().state().changed); }
    Result take(uint32_t reader, Loan& loan) const {
        if (loan) return Result::Invalid;
        typename PoolType::Sample sample;
        auto pool = core();
        const auto result = pool.take(reader, sample);
        if (result == Result::Ok) { loan.owner_ = owner_; loan.sample_ = sample; }
        return result;
    }
    Result wait_take(uint32_t reader, std::chrono::milliseconds timeout, Loan& loan) const {
        using namespace std::chrono;
        if (loan || reader >= PoolType::reader_count || timeout.count() < 0 || timeout > hours(24))
            return Result::Invalid;
        const auto deadline = steady_clock::now() + timeout;
        auto pool = core();
        for (;;) {
            // Snapshot BEFORE testing the predicate. A commit in either the
            // scan->wait gap or earlier makes the futex comparison fail.
            const auto expected = pool.notification_snapshot();
            const auto result = take(reader, loan);
            if (result != Result::Empty) return result;
            if (steady_clock::now() >= deadline) return Result::Timeout;
            if (wait_once(pool.state().changed, expected, deadline) == WaitEvent::Error)
                return Result::SystemError;
        }
    }
private:
    explicit MappedPool(std::shared_ptr<Mapping> owner) : owner_(std::move(owner)) {}
    std::shared_ptr<Mapping> owner_;
};

} // namespace travodds_experimental
