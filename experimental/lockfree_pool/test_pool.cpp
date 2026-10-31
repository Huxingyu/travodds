#include "mapped_pool.hpp"
#include <atomic>
#include <array>
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <csignal>
#include <sys/wait.h>

using namespace travodds_experimental;
using namespace std::chrono_literals;
namespace {
void check(bool valid, const char* text, int line) {
    if (!valid) { std::ostringstream out; out << "line " << line << ": " << text; throw std::runtime_error(out.str()); }
}
#define CHECK(x) check(bool(x), #x, __LINE__)
using Core = Pool<4, 128, 3>;
using Mapped = MappedPool<Core>;
struct Payload { uint64_t index; std::array<uint64_t, 7> checksum; };
void fill(unsigned char* data, uint64_t index) {
    auto* value = new (data) Payload{};
    value->index = index;
    for (size_t n = 0; n < value->checksum.size(); ++n) value->checksum[n] = index * 100 + n;
}
bool valid(const unsigned char* data, uint64_t index) {
    const auto* value = reinterpret_cast<const Payload*>(data);
    if (value->index != index) return false;
    for (size_t n = 0; n < value->checksum.size(); ++n)
        if (value->checksum[n] != index * 100 + n) return false;
    return true;
}
template<class C>
typename C::Reservation publish(C core, uint64_t sequence, uint32_t targets = C::pending_mask) {
    typename C::Reservation out;
    CHECK(core.reserve(out) == Result::Ok);
    fill(out.data, sequence);
    CHECK(core.commit(out, sizeof(Payload), sequence, targets) == Result::Ok);
    return out;
}
class Child {
public:
    explicit Child(pid_t id) : pid(id) { CHECK(id > 0); }
    ~Child() {
        if (pid > 0) { kill(pid, SIGKILL); while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {} }
    }
    int wait(bool stopped = false) {
        auto deadline = std::chrono::steady_clock::now() + 3s;
        for (;;) {
            int status = 0;
            const auto result = waitpid(pid, &status, WNOHANG | (stopped ? WUNTRACED : 0));
            CHECK(result >= 0 || errno == EINTR);
            if (result == pid) { if (!WIFSTOPPED(status)) pid = -1; return status; }
            CHECK(std::chrono::steady_clock::now() < deadline);
            std::this_thread::sleep_for(1ms);
        }
    }
    pid_t pid;
};

void roundtrip_and_all_pins() {
    using P = Pool<1, 128, 2>; using M = MappedPool<P>;
    auto view = M::create({1, 2}); auto core = view.core();
    auto reservation = publish(core, 1);
    CHECK(reinterpret_cast<uintptr_t>(reservation.data) % 64 == 0);
    typename M::Loan a, b;
    CHECK(view.take(0, a) == Result::Ok); CHECK(view.take(1, b) == Result::Ok);
    CHECK(a.data() == b.data()); CHECK(valid(a.data(), 1));
    typename P::Reservation blocked;
    CHECK(core.reserve(blocked) == Result::Exhausted);
    CHECK(core.release_writer(reservation.token) == Result::Ok);
    CHECK(a.reset() == Result::Ok); CHECK(core.reserve(blocked) == Result::Exhausted);
    CHECK(valid(b.data(), 1)); CHECK(b.reset() == Result::Ok);
    CHECK(core.reserve(blocked) == Result::Ok);
    CHECK(blocked.token.generation == reservation.token.generation + 1);
    CHECK(core.abort(blocked) == Result::Ok);
}
void writer_history_and_zero_targets() {
    using P = Pool<1, 128, 1>; auto view = MappedPool<P>::create({3, 4}); auto core = view.core();
    auto first = publish(core, 1, 0);
    P::Reservation blocked; CHECK(core.reserve(blocked) == Result::Exhausted);
    P::Sample no; CHECK(core.take(0, no) == Result::Empty);
    CHECK(core.release_writer(first.token) == Result::Ok);
    CHECK(core.release_writer(first.token) == Result::Invalid);
    CHECK(core.reserve(blocked) == Result::Ok); CHECK(core.abort(blocked) == Result::Ok);
}
void stale_tokens_cannot_clear_new_pins() {
    using P = Pool<1, 128, 1>; using M = MappedPool<P>;
    auto view = M::create({5, 6}); auto core = view.core();
    auto first = publish(core, 1); typename M::Loan old;
    CHECK(view.take(0, old) == Result::Ok); const auto oldToken = old.token();
    CHECK(core.release_writer(first.token) == Result::Ok); CHECK(old.reset() == Result::Ok);
    auto second = publish(core, 2); typename M::Loan current;
    CHECK(view.take(0, current) == Result::Ok); CHECK(core.release_writer(second.token) == Result::Ok);
    CHECK(core.release_reader(oldToken, 0) == Result::Invalid);
    CHECK(core.release_writer(first.token) == Result::Invalid);
    CHECK(core.discard_pending(oldToken, 0) == Result::Invalid);
    P::Reservation extra; CHECK(core.reserve(extra) == Result::Exhausted);
    CHECK(valid(current.data(), 2)); CHECK(current.reset() == Result::Ok);
    CHECK(core.reserve(extra) == Result::Ok); CHECK(core.abort(extra) == Result::Ok);
}
void invalid_tokens_and_reservations() {
    auto view = Mapped::create({7, 8}); auto core = view.core(); Core::Reservation reservation;
    CHECK(core.reserve(reservation) == Result::Ok);
    auto wrong = reservation; ++wrong.token.session.low;
    CHECK(core.commit(wrong, 1, 1) == Result::Invalid);
    CHECK(core.release_writer(reservation.token) == Result::Invalid); // WRITING requires abort
    CHECK(core.commit(reservation, 129, 1) == Result::Invalid);
    CHECK(core.commit(reservation, 1, 0) == Result::Invalid);
    CHECK(core.commit(reservation, 1, 1, 8) == Result::Invalid);
    CHECK(core.abort(reservation) == Result::Ok); CHECK(core.abort(reservation) == Result::Invalid);
    Core::Sample sample; CHECK(core.take(3, sample) == Result::Invalid);
    CHECK(!Core::compatible(&core.state(), sizeof(Core::State), {8, 8}));
    CHECK(!Core::compatible(&core.state(), sizeof(Core::State) - 1, {7, 8}));
}
void generation_and_sequence_exhaustion() {
    using P = Pool<1, 128, 1>; auto view = MappedPool<P>::create({9, 10}); auto core = view.core();
    __atomic_store_n(&core.state().storage[0].control, uint64_t(UINT32_MAX - 1) << 32, __ATOMIC_RELEASE);
    P::Reservation final; CHECK(core.reserve(final) == Result::Ok);
    CHECK(final.token.generation == UINT32_MAX); CHECK(core.abort(final) == Result::Ok);
    P::Reservation retired; CHECK(core.reserve(retired) == Result::Exhausted);
    auto other = MappedPool<P>::create({11, 12}); auto p = other.core();
    auto last = publish(p, UINT64_MAX, 0); CHECK(p.release_writer(last.token) == Result::Ok);
    CHECK(p.reserve(retired) == Result::SequenceExhausted);
}
void unpublished_payload_is_not_visible() {
    auto view = Mapped::create({13, 14}); auto core = view.core(); Core::Reservation pending;
    CHECK(core.reserve(pending) == Result::Ok); fill(pending.data, 1);
    Core::Sample sample; CHECK(core.take(0, sample) == Result::Empty);
    // Stop a commit between its control update and final watermark. This is a
    // deliberate white-box protocol test, not a public partial-commit API.
    auto& slot = core.state().storage[pending.token.slot];
    slot.size = sizeof(Payload);
    __atomic_store_n(&slot.sequence, uint64_t(1), __ATOMIC_RELAXED);
    __atomic_store_n(&slot.control, (uint64_t(pending.token.generation) << 32) | Core::writer_bit | 1,
        __ATOMIC_RELEASE);
    CHECK(core.take(0, sample) == Result::Empty);
    __atomic_store_n(&core.state().published_watermark, uint64_t(1), __ATOMIC_RELEASE);
    CHECK(core.take(0, sample) == Result::Ok); CHECK(valid(sample.data, 1));
    CHECK(core.release_writer(pending.token) == Result::Ok);
    CHECK(core.release_reader(sample.token, 0) == Result::Ok);
}
void ordered_snapshot_and_discard() {
    auto view = Mapped::create({15, 16}); auto core = view.core();
    auto first = publish(core, 1, 1); auto second = publish(core, 2, 1);
    CHECK(core.discard_pending(first.token, 0) == Result::Ok);
    CHECK(core.discard_pending(first.token, 0) == Result::Invalid);
    CHECK(core.release_writer(first.token) == Result::Ok);
    typename Mapped::Loan loan; CHECK(view.take(0, loan) == Result::Ok);
    CHECK(loan.sequence() == 2); CHECK(valid(loan.data(), 2));
    CHECK(core.release_writer(second.token) == Result::Ok); CHECK(loan.reset() == Result::Ok);
}
void mapping_outlives_view_and_close_drains() {
    using P = Pool<1, 128, 1>; using M = MappedPool<P>;
    auto view = std::make_unique<M>(M::create({17, 18})); auto core = view->core();
    auto first = publish(core, 1); core.close_writer();
    P::Reservation impossible; CHECK(core.reserve(impossible) == Result::Closed);
    typename M::Loan loan; CHECK(view->take(0, loan) == Result::Ok);
    CHECK(core.release_writer(first.token) == Result::Ok);
    typename M::Loan empty; CHECK(view->take(0, empty) == Result::Closed);
    std::weak_ptr<Mapping> mapping = view->mapping(); view.reset();
    CHECK(!mapping.expired()); CHECK(valid(loan.data(), 1));
    typename M::Loan moved(std::move(loan)); CHECK(!loan); CHECK(moved);
    CHECK(moved.reset() == Result::Ok); CHECK(mapping.expired());
}
void notification_wrap_and_pre_sleep_publication() {
    auto view = Mapped::create({19, 20}); auto core = view.core();
    __atomic_store_n(&core.state().changed, UINT32_MAX - 1, __ATOMIC_RELEASE);
    const auto observed = core.notification_snapshot();
    auto first = publish(core, 1, 1); CHECK(core.notification_snapshot() == UINT32_MAX);
    CHECK(core.release_writer(first.token) == Result::Ok); CHECK(core.notification_snapshot() == 0);
    CHECK(wait_once(core.state().changed, observed, std::chrono::steady_clock::now() + 1s) == WaitEvent::Changed);
    typename Mapped::Loan loan; CHECK(view.wait_take(0, 20ms, loan) == Result::Ok);
    CHECK(valid(loan.data(), 1)); CHECK(loan.reset() == Result::Ok);
}
void wait_timeout_and_wake() {
    auto view = Mapped::create({21, 22}); typename Mapped::Loan loan;
    CHECK(view.wait_take(0, -1ms, loan) == Result::Invalid);
    CHECK(view.wait_take(0, 0ms, loan) == Result::Timeout);
    CHECK(view.wait_take(0, 5ms, loan) == Result::Timeout);
    std::atomic<bool> entered{false}; std::atomic<bool> received{false};
    std::thread reader([&] { entered.store(true); typename Mapped::Loan item;
        received.store(view.wait_take(0, 1s, item) == Result::Ok && valid(item.data(), 1)); });
    while (!entered.load()) std::this_thread::yield();
    auto first = publish(view.core(), 1, 1); CHECK(view.notify());
    reader.join(); CHECK(received.load()); CHECK(view.core().release_writer(first.token) == Result::Ok);
}
void paused_reader_does_not_stop_other_slots() {
    using P = Pool<2, 128, 2>; using M = MappedPool<P>;
    auto view = M::create({23, 24}); auto core = view.core();
    auto first = publish(core, 1); CHECK(core.release_writer(first.token) == Result::Ok);
    const auto pid = fork(); CHECK(pid >= 0);
    if (!pid) {
        typename M::Loan held;
        if (view.take(0, held) != Result::Ok) _exit(31);
        raise(SIGSTOP);
        if (!valid(held.data(), 1) || held.reset() != Result::Ok) _exit(32);
        _exit(0);
    }
    Child child(pid); const auto stopped = child.wait(true); CHECK(WIFSTOPPED(stopped));
    typename M::Loan active; CHECK(view.take(1, active) == Result::Ok); CHECK(active.reset() == Result::Ok);
    for (uint64_t i = 2; i <= 1000; ++i) {
        auto next = publish(core, i, 2); CHECK(core.release_writer(next.token) == Result::Ok);
        CHECK(view.take(1, active) == Result::Ok); CHECK(active.sequence() == i);
        CHECK(valid(active.data(), i)); CHECK(active.reset() == Result::Ok);
    }
    CHECK(kill(pid, SIGCONT) == 0); const auto status = child.wait();
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
void paused_reader_backpressure_is_explicit() {
    using P = Pool<2, 128, 2>; using M = MappedPool<P>;
    auto view = M::create({25, 26}); auto core = view.core();
    auto a = publish(core, 1); CHECK(core.release_writer(a.token) == Result::Ok);
    auto b = publish(core, 2); CHECK(core.release_writer(b.token) == Result::Ok);
    typename M::Loan active;
    CHECK(view.take(1, active) == Result::Ok); active.reset();
    CHECK(view.take(1, active) == Result::Ok); active.reset();
    P::Reservation blocked; CHECK(core.reserve(blocked) == Result::Exhausted);
    CHECK(core.discard_pending(a.token, 0) == Result::Ok);
    CHECK(core.reserve(blocked) == Result::Ok); CHECK(core.abort(blocked) == Result::Ok);
    CHECK(core.discard_pending(b.token, 0) == Result::Ok);
}
void writer_crash_does_not_recall_existing_loan() {
    using P = Pool<1, 128, 1>; using M = MappedPool<P>;
    auto view = M::create({27, 28});
    const auto pid = fork(); CHECK(pid >= 0);
    if (!pid) { publish(view.core(), 1); raise(SIGSTOP); _exit(0); }
    Child child(pid); CHECK(WIFSTOPPED(child.wait(true)));
    typename M::Loan loan; CHECK(view.take(0, loan) == Result::Ok);
    CHECK(kill(pid, SIGKILL) == 0); CHECK(WIFSIGNALED(child.wait()));
    CHECK(valid(loan.data(), 1)); CHECK(loan.reset() == Result::Ok);
    // No automatic death reaping/takeover is implemented. The Writer pin stays.
    P::Reservation blocked; CHECK(view.core().reserve(blocked) == Result::Exhausted);
}
void threaded_multi_reader_stress() {
    using P = Pool<8, 128, 3>; using M = MappedPool<P>;
    auto view = M::create({29, 30}); constexpr uint64_t total = 3000;
    std::atomic<unsigned> failures{0}; std::array<std::atomic<uint64_t>, 3> counts{};
    std::vector<std::thread> readers;
    for (uint32_t r = 0; r < 3; ++r) readers.emplace_back([&, r] {
        uint64_t expected = 1;
        while (expected <= total) {
            typename M::Loan loan;
            const auto result = view.wait_take(r, 2s, loan);
            if (result != Result::Ok || loan.sequence() != expected || !valid(loan.data(), expected)) {
                failures.fetch_add(1); return;
            }
            ++counts[r]; ++expected;
            if ((expected + r) % 11 == 0) std::this_thread::yield();
        }
    });
    auto core = view.core(); const auto deadline = std::chrono::steady_clock::now() + 10s;
    bool producer_ok = true;
    for (uint64_t i = 1; i <= total; ++i) {
        typename P::Reservation reservation;
        while (core.reserve(reservation) == Result::Exhausted) {
            if (failures.load() || std::chrono::steady_clock::now() >= deadline) { producer_ok = false; break; }
            std::this_thread::yield();
        }
        if (!producer_ok) break;
        fill(reservation.data, i);
        if (core.commit(reservation, sizeof(Payload), i) != Result::Ok ||
                core.release_writer(reservation.token) != Result::Ok || !view.notify()) {
            producer_ok = false; break;
        }
    }
    core.close_writer(); view.notify();
    for (auto& reader : readers) reader.join();
    CHECK(producer_ok); CHECK(failures.load() == 0);
    for (auto& count : counts) CHECK(count.load() == total);
}
} // namespace

int main() {
    const std::pair<const char*, void(*)()> tests[] = {
        {"roundtrip_and_all_pins", roundtrip_and_all_pins},
        {"writer_history_and_zero_targets", writer_history_and_zero_targets},
        {"stale_tokens_cannot_clear_new_pins", stale_tokens_cannot_clear_new_pins},
        {"invalid_tokens_and_reservations", invalid_tokens_and_reservations},
        {"generation_and_sequence_exhaustion", generation_and_sequence_exhaustion},
        {"unpublished_payload_is_not_visible", unpublished_payload_is_not_visible},
        {"ordered_snapshot_and_discard", ordered_snapshot_and_discard},
        {"mapping_outlives_view_and_close_drains", mapping_outlives_view_and_close_drains},
        {"notification_wrap_and_pre_sleep_publication", notification_wrap_and_pre_sleep_publication},
        {"wait_timeout_and_wake", wait_timeout_and_wake},
        {"paused_reader_does_not_stop_other_slots", paused_reader_does_not_stop_other_slots},
        {"paused_reader_backpressure_is_explicit", paused_reader_backpressure_is_explicit},
        {"writer_crash_does_not_recall_existing_loan", writer_crash_does_not_recall_existing_loan},
        {"threaded_multi_reader_stress", threaded_multi_reader_stress},
    };
    unsigned passed = 0;
    for (const auto& test : tests) {
        try { test.second(); ++passed; std::cout << "PASS " << test.first << std::endl; }
        catch (const std::exception& error) {
            std::cerr << "FAIL " << test.first << ": " << error.what() << std::endl; return 1;
        }
    }
    std::cout << "RESULT " << passed << "/" << std::size(tests) << " passed" << std::endl;
}
