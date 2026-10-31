// Optional experimental control plane. The frozen shared_pool.hpp is unchanged.
#pragma once
#include "shared_pool.hpp"
#include <stdexcept>

namespace travodds_experimental {

enum class ControlResult { Ok, Pending, Exhausted, Invalid, Unsupported, Closed };
struct Registration {
    Session session;
    uint32_t reader = 0;
    uint32_t generation = 0;
};
struct OpenRequest {
    // UINT32_MAX asks the single Writer/controller to choose a free identity.
    uint32_t requested_reader = UINT32_MAX;
    bool replay = false;
};
struct CloseRequest { Registration registration; };

// Only the serialized Writer lane applies requests, polls closing identities,
// and reserves/commits/aborts. Reader operations merely take operation leases.
// Request delivery/discovery is intentionally outside this prototype: values
// above must reach apply_* through the application's own control channel.
template<class Core>
class Controller {
public:
    enum class Phase : uint32_t { Free = 0, Open = 1, Closing = 2 };
    static constexpr uint64_t magic = 0x54524c464354524cULL;
    static constexpr uint32_t version = 1;
    static constexpr uint32_t phase_mask = 3;
    static constexpr uint32_t count_unit = 4;
    static constexpr uint32_t count_mask = UINT32_MAX & ~phase_mask;
    struct alignas(64) Entry { alignas(8) uint64_t gate; };
    struct alignas(64) State {
        uint64_t magic_value;
        uint32_t layout_version;
        uint32_t readers;
        Session session;
        Entry registry[Core::reader_count];
    };
    static_assert(std::is_trivially_copyable<State>::value, "Control mapping must be POD");

    class Lease {
    public:
        Lease() = default;
        ~Lease() { reset(); }
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        bool active() const { return owner_ != nullptr; }
    private:
        State* owner_ = nullptr;
        Registration registration_;
        bool returning_ = false;
        void reset() noexcept {
            if (!owner_) return;
            auto& target = owner_->registry[registration_.reader].gate;
            auto old = load(target);
            while (epoch(old) == registration_.generation && (uint32_t(old) & count_mask)) {
                if (cas(target, old, old - count_unit)) break;
            }
            owner_ = nullptr;
        }
        friend class Controller;
    };
    struct Sample {
        Registration registration;
        typename Core::Sample payload;
    };

    static State* initialize(void* address, size_t bytes, Session session) {
        if (!address || bytes != sizeof(State) || !session.valid() ||
                reinterpret_cast<uintptr_t>(address) % alignof(State)) return nullptr;
        auto* state = new (address) State{};
        state->magic_value = magic;
        state->layout_version = version;
        state->readers = uint32_t(Core::reader_count);
        state->session = session;
        return state;
    }
    static bool compatible(const State* state, size_t bytes, Session session) {
        return state && bytes == sizeof(State) &&
            reinterpret_cast<uintptr_t>(state) % alignof(State) == 0 &&
            state->magic_value == magic && state->layout_version == version &&
            state->readers == Core::reader_count && state->session == session && session.valid();
    }
    Controller(Core core, State& state) : core_(core), state_(&state) {
        if (!compatible(&state, sizeof(State), core.state().session))
            throw std::invalid_argument("Incompatible experimental registry");
    }

    ControlResult apply_open(const OpenRequest& request, Registration& out) {
        if (request.replay) return ControlResult::Unsupported;
        if (__atomic_load_n(&core_.state().closed, __ATOMIC_ACQUIRE)) return ControlResult::Closed;
        if (request.requested_reader != UINT32_MAX && request.requested_reader >= Core::reader_count)
            return ControlResult::Invalid;
        const auto begin = request.requested_reader == UINT32_MAX ? 0 : request.requested_reader;
        const auto end = request.requested_reader == UINT32_MAX ? Core::reader_count : size_t(begin + 1);
        for (size_t index = begin; index < end; ++index) {
            auto& gate = state_->registry[index].gate;
            auto old = load(gate);
            if (phase(old) != Phase::Free || (uint32_t(old) & count_mask) || epoch(old) == UINT32_MAX) continue;
            if (!identity_unused(uint32_t(index))) continue;
            const auto generation = epoch(old) + 1;
            if (!cas(gate, old, word(generation, Phase::Open))) continue;
            out = {state_->session, uint32_t(index), generation};
            return ControlResult::Ok;
        }
        return ControlResult::Exhausted;
    }

    // Close never waits for a Reader. Old operations/Loans can leave this one
    // identity Closing; the Writer can keep committing for other open Readers.
    ControlResult apply_close(const CloseRequest& request) {
        const auto& registration = request.registration;
        if (!matches(registration)) return ControlResult::Invalid;
        auto& gate = state_->registry[registration.reader].gate;
        auto old = load(gate);
        for (;;) {
            if (epoch(old) != registration.generation || phase(old) == Phase::Free) return ControlResult::Invalid;
            if (phase(old) == Phase::Closing) break;
            auto next = (old & ~uint64_t(phase_mask)) | uint32_t(Phase::Closing);
            if (cas(gate, old, next)) break;
        }
        return poll_one(registration.reader);
    }
    unsigned poll_closing() {
        unsigned reclaimed = 0;
        for (uint32_t reader = 0; reader < Core::reader_count; ++reader)
            if (phase(load(state_->registry[reader].gate)) == Phase::Closing &&
                    poll_one(reader) == ControlResult::Ok) ++reclaimed;
        return reclaimed;
    }
    uint32_t open_mask() const {
        uint32_t mask = 0;
        for (uint32_t reader = 0; reader < Core::reader_count; ++reader)
            if (phase(load(state_->registry[reader].gate)) == Phase::Open) mask |= uint32_t(1) << reader;
        return mask;
    }
    Phase phase_of(uint32_t reader) const {
        return reader < Core::reader_count ? phase(load(state_->registry[reader].gate)) : Phase::Free;
    }

    Result reserve(typename Core::Reservation& out) { return core_.reserve(out); }
    Result commit(const typename Core::Reservation& sample, uint32_t size, uint64_t sequence,
            uint32_t requested_targets = Core::pending_mask) {
        if (requested_targets & ~Core::pending_mask) return Result::Invalid;
        return core_.commit(sample, size, sequence, requested_targets & open_mask());
    }
    Result abort(const typename Core::Reservation& sample) { return core_.abort(sample); }
    Result release_writer(const Token& token) { return core_.release_writer(token); }
    void close_writer() { core_.close_writer(); }

    // Explicit begin/finish also permits deterministic pause-window tests. An
    // abandoned/paused lease pins only its registration identity. Caller must
    // keep both mappings alive throughout each Lease and all returned Samples.
    Result begin_take(const Registration& registration, Lease& lease) const {
        return begin(registration, false, lease);
    }
    Result finish_take(Lease& lease, Sample& out) const {
        if (!owns(lease, false) || out.payload.data) return Result::Invalid;
        auto core = core_; // A local facade only copies the shared State pointer.
        const auto result = core.take(lease.registration_.reader, out.payload);
        if (result == Result::Ok) out.registration = lease.registration_;
        lease.reset();
        return result;
    }
    Result take(const Registration& registration, Sample& out) const {
        if (out.payload.data) return Result::Invalid;
        Lease lease;
        auto result = begin_take(registration, lease);
        return result == Result::Ok ? finish_take(lease, out) : result;
    }
    Result begin_return(const Registration& registration, Lease& lease) const {
        return begin(registration, true, lease);
    }
    Result finish_return(Lease& lease, const Token& token) const {
        if (!owns(lease, true)) return Result::Invalid;
        auto core = core_;
        const auto result = core.release_reader(token, lease.registration_.reader);
        lease.reset();
        return result;
    }
    Result return_loan(Sample& sample) const {
        if (!sample.payload.data || sample.payload.reader != sample.registration.reader) return Result::Invalid;
        Lease lease;
        const auto entered = begin_return(sample.registration, lease);
        if (entered != Result::Ok) return entered;
        const auto result = finish_return(lease, sample.payload.token);
        if (result == Result::Ok) sample = {};
        return result;
    }
    Result discard_pending(const Registration& registration, const Token& token) const {
        Lease lease;
        const auto entered = begin_take(registration, lease);
        if (entered != Result::Ok) return entered;
        auto core = core_;
        const auto result = core.discard_pending(token, registration.reader);
        lease.reset();
        return result;
    }

private:
    Core core_;
    State* state_;
    static uint64_t load(const uint64_t& value) { return __atomic_load_n(&value, __ATOMIC_ACQUIRE); }
    static bool cas(uint64_t& target, uint64_t& expected, uint64_t value) {
        return __atomic_compare_exchange_n(&target, &expected, value, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
    }
    static uint32_t epoch(uint64_t gate) { return uint32_t(gate >> 32); }
    static Phase phase(uint64_t gate) { return Phase(uint32_t(gate) & phase_mask); }
    static uint64_t word(uint32_t epoch, Phase phase) { return (uint64_t(epoch) << 32) | uint32_t(phase); }
    bool matches(const Registration& registration) const {
        return registration.reader < Core::reader_count && registration.generation &&
            registration.session == state_->session;
    }
    bool owns(const Lease& lease, bool returning) const {
        return lease.owner_ == state_ && lease.returning_ == returning;
    }
    bool identity_unused(uint32_t reader) const {
        const auto bit = uint32_t(1) << reader;
        for (const auto& slot : core_.state().storage)
            if (uint32_t(load(slot.control)) & (bit | (bit << 15))) return false;
        return true;
    }
    Result begin(const Registration& registration, bool returning, Lease& lease) const {
        if (lease.active() || !matches(registration)) return Result::Invalid;
        auto& gate = state_->registry[registration.reader].gate;
        auto old = load(gate);
        for (;;) {
            if (epoch(old) != registration.generation) return Result::Invalid;
            if (phase(old) != Phase::Open && !(returning && phase(old) == Phase::Closing)) return Result::Closed;
            if ((uint32_t(old) & count_mask) == count_mask) return Result::Exhausted;
            if (cas(gate, old, old + count_unit)) break;
        }
        lease.owner_ = state_;
        lease.registration_ = registration;
        lease.returning_ = returning;
        return Result::Ok;
    }
    ControlResult poll_one(uint32_t reader) {
        auto& gate = state_->registry[reader].gate;
        auto old = load(gate);
        if (phase(old) != Phase::Closing) return ControlResult::Invalid;
        if (uint32_t(old) & count_mask) return ControlResult::Pending;
        const uint32_t pending = uint32_t(1) << reader;
        const uint32_t loaned = pending << 15;
        // No new take/discard can enter Closing. A return-only lease might enter
        // after the count observation, but it cannot create/remove pending.
        // Final gate CAS still refuses recycling if that return is in flight.
        for (uint32_t index = 0; index < Core::slot_count; ++index) {
            auto control = load(core_.state().storage[index].control);
            if (uint32_t(control) & pending) {
                const Token token{state_->session, index, uint32_t(control >> 32)};
                if (core_.discard_pending(token, reader) != Result::Ok) return ControlResult::Pending;
            }
        }
        for (const auto& slot : core_.state().storage)
            if (uint32_t(load(slot.control)) & (pending | loaned)) return ControlResult::Pending;
        if (!cas(gate, old, word(epoch(old), Phase::Free))) return ControlResult::Pending;
        return ControlResult::Ok;
    }
};

} // namespace travodds_experimental
