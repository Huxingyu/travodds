#include "controller.hpp"
#include "mapped_pool.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <exception>
#include <iostream>
#include <sstream>
#include <thread>
#include <sys/wait.h>

using namespace travodds_experimental;
using namespace std::chrono_literals;
namespace {
void check(bool valid, const char* text, int line) {
    if (!valid) { std::ostringstream out; out << "line " << line << ": " << text; throw std::runtime_error(out.str()); }
}
#define CHECK(x) check(bool(x), #x, __LINE__)
template<class P = Pool<4, 64, 2>>
struct Fixture {
    using Core = P;
    using Control = Controller<P>;
    MappedPool<P> data = MappedPool<P>::create({0xabc, 0x123});
    std::shared_ptr<Mapping> metadata = Mapping::shared(sizeof(typename Control::State));
    typename Control::State* state = Control::initialize(metadata->address(), metadata->bytes(), {0xabc, 0x123});
    Control control{data.core(), *state};
    Registration open(uint32_t id) {
        Registration registration;
        CHECK(control.apply_open({id, false}, registration) == ControlResult::Ok);
        return registration;
    }
    typename P::Reservation publish(uint64_t sequence, uint32_t targets = P::pending_mask) {
        typename P::Reservation reservation;
        CHECK(control.reserve(reservation) == Result::Ok);
        std::memcpy(reservation.data, &sequence, sizeof(sequence));
        CHECK(control.commit(reservation, sizeof(sequence), sequence, targets) == Result::Ok);
        CHECK(control.release_writer(reservation.token) == Result::Ok);
        return reservation;
    }
    uint64_t value(const typename Control::Sample& sample) {
        uint64_t out = 0; std::memcpy(&out, sample.payload.data, sizeof(out)); return out;
    }
};

void dynamic_open_close_and_pending_cleanup() {
    Fixture<> f;
    auto a = f.open(0); auto b = f.open(1);
    CHECK(f.control.open_mask() == 3);
    auto first = f.publish(1);
    CHECK(f.control.apply_close({a}) == ControlResult::Ok); // only unread pins
    CHECK(f.control.open_mask() == 2);
    typename Fixture<>::Control::Sample sample;
    CHECK(f.control.take(a, sample) == Result::Closed);
    CHECK(f.control.take(b, sample) == Result::Ok); CHECK(f.value(sample) == 1);
    CHECK(f.control.return_loan(sample) == Result::Ok);
    auto replacement = f.open(0); CHECK(replacement.generation == a.generation + 1);
    CHECK(f.control.apply_close({a}) == ControlResult::Invalid);
    f.publish(2, 1);
    CHECK(f.control.take(a, sample) == Result::Invalid);
    CHECK(f.control.take(replacement, sample) == Result::Ok); CHECK(f.value(sample) == 2);
    CHECK(f.control.return_loan(sample) == Result::Ok);
    CHECK(f.control.apply_close({replacement}) == ControlResult::Ok);
    CHECK(f.control.apply_close({b}) == ControlResult::Ok);
    CHECK(f.data.core().release_reader(first.token, 0) == Result::Invalid);
}
void close_preserves_loan_and_rejects_stale_registration() {
    using F = Fixture<Pool<1, 64, 1>>; F f; auto old = f.open(0);
    f.publish(1); typename F::Control::Sample sample;
    CHECK(f.control.take(old, sample) == Result::Ok); const auto stale = sample;
    CHECK(f.control.apply_close({old}) == ControlResult::Pending);
    CHECK(f.value(sample) == 1);
    Registration refused; CHECK(f.control.apply_open({0, false}, refused) == ControlResult::Exhausted);
    typename F::Core::Reservation full; CHECK(f.control.reserve(full) == Result::Exhausted);
    CHECK(f.control.return_loan(sample) == Result::Ok); CHECK(f.control.poll_closing() == 1);
    const auto current = f.open(0); CHECK(current.generation == old.generation + 1);
    f.publish(2); CHECK(f.control.take(current, sample) == Result::Ok);
    auto old_copy = stale; CHECK(f.control.return_loan(old_copy) == Result::Invalid);
    // Even a current sample token passed through an old registration handle is
    // refused before touching the core. This isolates registry identity checks.
    typename F::Control::Lease invalid;
    CHECK(f.control.begin_return(old, invalid) == Result::Invalid);
    CHECK(f.control.reserve(full) == Result::Exhausted); CHECK(f.value(sample) == 2);
    CHECK(f.control.return_loan(sample) == Result::Ok);
    CHECK(f.control.apply_close({current}) == ControlResult::Ok);
}
void paused_pre_take_lease_blocks_only_identity_reuse() {
    Fixture<> f; auto a = f.open(0); auto b = f.open(1); f.publish(1);
    typename Fixture<>::Control::Lease held;
    CHECK(f.control.begin_take(a, held) == Result::Ok);
    CHECK(f.control.apply_close({a}) == ControlResult::Pending);
    CHECK(f.control.open_mask() == 2);
    Registration replacement; CHECK(f.control.apply_open({0, false}, replacement) == ControlResult::Exhausted);
    typename Fixture<>::Control::Sample active;
    CHECK(f.control.take(b, active) == Result::Ok); CHECK(f.control.return_loan(active) == Result::Ok);
    for (uint64_t sequence = 2; sequence <= 1000; ++sequence) {
        f.publish(sequence); // closed a is automatically excluded
        CHECK(f.control.take(b, active) == Result::Ok); CHECK(f.value(active) == sequence);
        CHECK(f.control.return_loan(active) == Result::Ok);
    }
    typename Fixture<>::Control::Sample delayed;
    CHECK(f.control.finish_take(held, delayed) == Result::Ok);
    CHECK(delayed.payload.sequence == 1); CHECK(f.value(delayed) == 1);
    CHECK(f.control.poll_closing() == 0); // active loan retains the old identity
    CHECK(f.control.return_loan(delayed) == Result::Ok); CHECK(f.control.poll_closing() == 1);
    replacement = f.open(0); CHECK(replacement.generation == a.generation + 1);
    CHECK(f.control.begin_take(a, held) == Result::Invalid);
    CHECK(f.control.apply_close({replacement}) == ControlResult::Ok);
    CHECK(f.control.apply_close({b}) == ControlResult::Ok);
}
void delayed_return_lease_prevents_epoch_aba() {
    using F = Fixture<Pool<1, 64, 1>>; F f; auto old = f.open(0); f.publish(1);
    typename F::Control::Sample sample; CHECK(f.control.take(old, sample) == Result::Ok);
    const auto token = sample.payload.token;
    typename F::Control::Lease delayed;
    CHECK(f.control.begin_return(old, delayed) == Result::Ok);
    // Deterministic adversarial duplicate: another return clears the old pin,
    // while the delayed return has not yet reached its core CAS.
    CHECK(f.control.return_loan(sample) == Result::Ok);
    CHECK(f.control.apply_close({old}) == ControlResult::Pending);
    Registration replacement; CHECK(f.control.apply_open({0, false}, replacement) == ControlResult::Exhausted);
    CHECK(f.control.finish_return(delayed, token) == Result::Invalid);
    CHECK(f.control.poll_closing() == 1); replacement = f.open(0);
    f.publish(2); CHECK(f.control.take(replacement, sample) == Result::Ok);
    CHECK(f.control.begin_return(old, delayed) == Result::Invalid);
    CHECK(f.value(sample) == 2); CHECK(f.control.return_loan(sample) == Result::Ok);
    CHECK(f.control.apply_close({replacement}) == ControlResult::Ok);
}
void reader_death_inside_lease_does_not_stop_other_identity() {
    Fixture<> f; const auto a = f.open(0); const auto b = f.open(1); f.publish(1);
    const auto pid = fork(); CHECK(pid >= 0);
    if (!pid) {
        typename Fixture<>::Control::Lease lease;
        if (f.control.begin_take(a, lease) != Result::Ok) _exit(31);
        raise(SIGSTOP); _exit(0); // destructor intentionally absent, representing death
    }
    struct Cleanup {
        pid_t pid;
        ~Cleanup() { if (pid > 0) { kill(pid, SIGKILL); while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {} } }
    } child{pid};
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    int status = 0;
    for (;;) {
        const auto waited = waitpid(pid, &status, WUNTRACED | WNOHANG);
        if (waited == pid) break;
        if (waited < 0 && errno == EINTR) continue;
        CHECK(waited == 0);
        CHECK(std::chrono::steady_clock::now() < deadline); std::this_thread::sleep_for(1ms);
    }
    CHECK(WIFSTOPPED(status)); CHECK(f.control.apply_close({a}) == ControlResult::Pending);
    CHECK(kill(pid, SIGKILL) == 0);
    pid_t waited;
    do { waited = waitpid(pid, &status, 0); } while (waited < 0 && errno == EINTR);
    CHECK(waited == pid); child.pid = -1;
    CHECK(WIFSIGNALED(status));
    typename Fixture<>::Control::Sample sample;
    CHECK(f.control.take(b, sample) == Result::Ok); CHECK(f.control.return_loan(sample) == Result::Ok);
    for (uint64_t sequence = 2; sequence <= 1000; ++sequence) {
        f.publish(sequence); CHECK(f.control.take(b, sample) == Result::Ok);
        CHECK(f.value(sample) == sequence); CHECK(f.control.return_loan(sample) == Result::Ok);
    }
    CHECK(f.control.poll_closing() == 0);
    Registration denied; CHECK(f.control.apply_open({0, false}, denied) == ControlResult::Exhausted);
    CHECK(f.control.apply_close({b}) == ControlResult::Ok);
}
void replay_unsupported_and_registry_limits() {
    Fixture<> f; Registration registration;
    CHECK(f.control.apply_open({0, true}, registration) == ControlResult::Unsupported);
    CHECK(f.control.open_mask() == 0);
    CHECK(f.control.apply_open({9, false}, registration) == ControlResult::Invalid);
    auto& gate = f.state->registry[0].gate;
    __atomic_store_n(&gate, uint64_t(UINT32_MAX) << 32, __ATOMIC_RELEASE);
    CHECK(f.control.apply_open({0, false}, registration) == ControlResult::Exhausted);
    registration = f.open(1);
    auto& live = f.state->registry[1].gate;
    __atomic_store_n(&live, (uint64_t(registration.generation) << 32) |
        Fixture<>::Control::count_mask | uint32_t(Fixture<>::Control::Phase::Open), __ATOMIC_RELEASE);
    typename Fixture<>::Control::Lease lease;
    CHECK(f.control.begin_take(registration, lease) == Result::Exhausted);
    __atomic_store_n(&live, (uint64_t(registration.generation) << 32) |
        uint32_t(Fixture<>::Control::Phase::Open), __ATOMIC_RELEASE);
    CHECK(f.control.apply_close({registration}) == ControlResult::Ok);
    f.control.close_writer();
    CHECK(f.control.apply_open({}, registration) == ControlResult::Closed);
}
void stale_session_and_unfinished_lease_cleanup() {
    Fixture<> f; auto valid = f.open(0); auto stale = valid; ++stale.session.low;
    typename Fixture<>::Control::Lease lease;
    CHECK(f.control.begin_take(stale, lease) == Result::Invalid);
    CHECK(f.control.apply_close({stale}) == ControlResult::Invalid);
    {
        typename Fixture<>::Control::Lease unfinished;
        CHECK(f.control.begin_take(valid, unfinished) == Result::Ok);
        CHECK(f.control.apply_close({valid}) == ControlResult::Pending);
    }
    CHECK(f.control.poll_closing() == 1);
    const auto replacement = f.open(0); CHECK(replacement.generation == valid.generation + 1);
    CHECK(f.control.apply_close({replacement}) == ControlResult::Ok);
}

void threaded_close_reopen_preserves_loans_and_other_reader_progress() {
    using F = Fixture<>;
    F f;
    const auto persistent = f.open(1);
    constexpr uint32_t rounds = 128;
    constexpr uint32_t unrelated_per_round = 16;
    enum Stage : uint64_t { Ready = 1, Held = 2, Closing = 3, ClosingObserved = 4, Release = 5, Returned = 6 };
    auto ticket = [](uint32_t round, Stage stage) { return (uint64_t(round) << 8) | uint64_t(stage); };
    std::atomic<uint64_t> stage{0}, expected_sequence{0}, unrelated_published{0}, unrelated_checked{0};
    std::atomic<uint32_t> announced_generation{0}, completed{0};
    std::atomic<bool> stop{false};
    std::exception_ptr reader_error, writer_error;
    const auto deadline = std::chrono::steady_clock::now() + 15s;
    auto until = [&](auto predicate) {
        while (!predicate()) {
            CHECK(!stop.load(std::memory_order_acquire));
            CHECK(std::chrono::steady_clock::now() < deadline);
            std::this_thread::yield();
        }
    };
    auto check_payload = [](const unsigned char* bytes, uint64_t sequence) {
        uint64_t values[8]; std::memcpy(values, bytes, sizeof(values));
        for (size_t i = 0; i < 8; ++i) CHECK(values[i] == (sequence ^ (0x9e3779b97f4a7c15ULL + i)));
    };
    auto publish_payload = [&](uint64_t sequence) {
        F::Core::Reservation reservation;
        CHECK(f.control.reserve(reservation) == Result::Ok);
        uint64_t values[8];
        for (size_t i = 0; i < 8; ++i) values[i] = sequence ^ (0x9e3779b97f4a7c15ULL + i);
        std::memcpy(reservation.data, values, sizeof(values));
        CHECK(f.control.commit(reservation, sizeof(values), sequence) == Result::Ok);
        CHECK(f.control.release_writer(reservation.token) == Result::Ok);
    };
    std::thread reader([&] {
        F::Control::Sample held, stale_sample;
        Registration previous;
        try {
            for (uint32_t round = 1; round <= rounds; ++round) {
                until([&] { return stage.load(std::memory_order_acquire) == ticket(round, Ready); });
                const Registration registration{{0xabc, 0x123}, 0,
                    announced_generation.load(std::memory_order_relaxed)};
                const auto sequence = expected_sequence.load(std::memory_order_relaxed);
                if (round > 1) {
                    CHECK(registration.generation == previous.generation + 1);
                    F::Control::Sample refused;
                    CHECK(f.control.take(previous, refused) == Result::Invalid);
                    CHECK(f.control.return_loan(stale_sample) == Result::Invalid);
                }
                CHECK(f.control.take(registration, held) == Result::Ok);
                CHECK(held.payload.sequence == sequence); check_payload(held.payload.data, sequence);
                stale_sample = held; // Retain only an adversarial old token, never dereference it after return.
                stage.store(ticket(round, Held), std::memory_order_release);
                until([&] { return stage.load(std::memory_order_acquire) == ticket(round, Closing); });
                F::Control::Sample refused;
                CHECK(f.control.take(registration, refused) == Result::Closed);
                check_payload(held.payload.data, sequence);
                stage.store(ticket(round, ClosingObserved), std::memory_order_release);
                while (stage.load(std::memory_order_acquire) != ticket(round, Release)) {
                    CHECK(!stop.load(std::memory_order_acquire));
                    CHECK(std::chrono::steady_clock::now() < deadline);
                    const auto other = unrelated_published.load(std::memory_order_acquire);
                    check_payload(held.payload.data, sequence);
                    unrelated_checked.store(other, std::memory_order_release);
                    std::this_thread::yield();
                }
                check_payload(held.payload.data, sequence);
                CHECK(f.control.return_loan(held) == Result::Ok); CHECK(!held.payload.data);
                previous = registration;
                completed.fetch_add(1, std::memory_order_relaxed);
                stage.store(ticket(round, Returned), std::memory_order_release);
            }
        } catch (...) {
            reader_error = std::current_exception();
            if (held.payload.data) f.control.return_loan(held);
            stop.store(true, std::memory_order_release);
        }
    });
    uint64_t sequence = 0;
    try {
        for (uint32_t round = 1; round <= rounds; ++round) {
            const auto rotating = f.open(0);
            publish_payload(++sequence);
            F::Control::Sample other;
            CHECK(f.control.take(persistent, other) == Result::Ok);
            CHECK(other.payload.sequence == sequence); check_payload(other.payload.data, sequence);
            CHECK(f.control.return_loan(other) == Result::Ok);
            expected_sequence.store(sequence, std::memory_order_relaxed);
            announced_generation.store(rotating.generation, std::memory_order_relaxed);
            stage.store(ticket(round, Ready), std::memory_order_release);
            until([&] { return stage.load(std::memory_order_acquire) == ticket(round, Held); });
            CHECK(f.control.apply_close({rotating}) == ControlResult::Pending);
            CHECK(f.control.open_mask() == 2);
            Registration denied;
            CHECK(f.control.apply_open({0, false}, denied) == ControlResult::Exhausted);
            stage.store(ticket(round, Closing), std::memory_order_release);
            until([&] { return stage.load(std::memory_order_acquire) == ticket(round, ClosingObserved); });
            for (uint32_t i = 0; i < unrelated_per_round; ++i) {
                publish_payload(++sequence);
                CHECK(f.control.take(persistent, other) == Result::Ok);
                CHECK(other.payload.sequence == sequence); check_payload(other.payload.data, sequence);
                CHECK(f.control.return_loan(other) == Result::Ok);
                unrelated_published.store(sequence, std::memory_order_release);
                until([&] { return unrelated_checked.load(std::memory_order_acquire) == sequence; });
                CHECK(f.control.poll_closing() == 0); // The other thread still owns its old Loan.
            }
            stage.store(ticket(round, Release), std::memory_order_release);
            until([&] { return stage.load(std::memory_order_acquire) == ticket(round, Returned); });
            CHECK(f.control.poll_closing() == 1);
        }
        CHECK(f.control.apply_close({persistent}) == ControlResult::Ok);
    } catch (...) {
        writer_error = std::current_exception(); stop.store(true, std::memory_order_release);
    }
    reader.join();
    if (reader_error) std::rethrow_exception(reader_error);
    if (writer_error) std::rethrow_exception(writer_error);
    CHECK(completed.load(std::memory_order_relaxed) == rounds);
    CHECK(sequence == uint64_t(rounds) * (unrelated_per_round + 1));
    CHECK(f.control.open_mask() == 0);
}
}

int main() {
    const std::pair<const char*, void(*)()> tests[] = {
        {"dynamic_open_close_and_pending_cleanup", dynamic_open_close_and_pending_cleanup},
        {"close_preserves_loan_and_rejects_stale_registration", close_preserves_loan_and_rejects_stale_registration},
        {"paused_pre_take_lease_blocks_only_identity_reuse", paused_pre_take_lease_blocks_only_identity_reuse},
        {"delayed_return_lease_prevents_epoch_aba", delayed_return_lease_prevents_epoch_aba},
        {"reader_death_inside_lease_does_not_stop_other_identity", reader_death_inside_lease_does_not_stop_other_identity},
        {"replay_unsupported_and_registry_limits", replay_unsupported_and_registry_limits},
        {"stale_session_and_unfinished_lease_cleanup", stale_session_and_unfinished_lease_cleanup},
        {"threaded_close_reopen_preserves_loans_and_other_reader_progress", threaded_close_reopen_preserves_loans_and_other_reader_progress},
    };
    unsigned passed = 0;
    for (const auto& test : tests) {
        try { test.second(); ++passed; std::cout << "PASS " << test.first << std::endl; }
        catch (const std::exception& error) { std::cerr << "FAIL " << test.first << ": " << error.what() << std::endl; return 1; }
    }
    std::cout << "RESULT " << passed << "/" << std::size(tests) << " passed" << std::endl;
}
