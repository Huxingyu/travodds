// Experimental protocol, not the production DDS/DataSharing backend.
#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <type_traits>

namespace travodds_experimental {

static_assert(__atomic_always_lock_free(sizeof(uint64_t), nullptr),
        "This prototype requires always-lock-free 64-bit atomics");
static_assert(__atomic_always_lock_free(sizeof(uint32_t), nullptr),
        "This prototype requires always-lock-free 32-bit atomics");

struct Session {
    uint64_t high = 0;
    uint64_t low = 0;
    bool operator==(const Session& b) const { return high == b.high && low == b.low; }
    bool valid() const { return high || low; }
};
struct Token {
    Session session;
    uint32_t slot = 0;
    uint32_t generation = 0;
};
enum class Result { Ok, Empty, Exhausted, Invalid, Closed, SequenceExhausted, Timeout, SystemError };

// One serialized producer for reserve/commit/abort/close; one serialized
// consumer per fixed Reader id for take/discard. Published Writer pins and
// existing Reader Loans may be returned by other threads using valid tokens.
// No participant may forge another participant's token or modify shared bytes.
template<size_t SlotCount, size_t PayloadBytes, size_t ReaderCount>
class Pool {
    static_assert(SlotCount > 0 && SlotCount <= UINT32_MAX, "Invalid slot count");
    static_assert(PayloadBytes > 0 && PayloadBytes <= UINT32_MAX, "Invalid payload size");
    static_assert(ReaderCount > 0 && ReaderCount <= 15, "At most 15 fixed Readers");
public:
    static constexpr uint64_t magic = 0x54524c46504f4f4cULL;
    static constexpr uint32_t version = 2;
    static constexpr uint32_t writer_bit = uint32_t(1) << 30;
    static constexpr uint32_t writing_bit = uint32_t(1) << 31;
    static constexpr uint32_t pending_mask = (uint32_t(1) << ReaderCount) - 1;
    static constexpr uint32_t loaned_mask = pending_mask << 15;
    static constexpr size_t slot_count = SlotCount;
    static constexpr size_t payload_bytes = PayloadBytes;
    static constexpr size_t reader_count = ReaderCount;

    struct alignas(64) Slot {
        alignas(8) uint64_t control;
        // Atomic because an unpinned scanner may observe reuse. It must not
        // inspect size/payload until its control CAS has acquired a pin.
        alignas(8) uint64_t sequence;
        uint32_t size;
        uint32_t reserved;
        alignas(64) unsigned char payload[PayloadBytes];
    };
    struct alignas(64) State {
        uint64_t magic_value;
        uint32_t layout_version;
        uint32_t slots;
        uint32_t payload_size;
        uint32_t readers;
        Session session;
        alignas(8) uint64_t published_watermark;
        alignas(4) uint32_t changed;
        alignas(4) uint32_t closed;
        // Accessed only by the serialized producer, never by Readers. Keep the
        // rotation in the mapping so copying the process-local facade does not
        // restart every allocation at slot zero. It is not a correctness gate.
        uint32_t next_slot;
        uint32_t reserved;
        Slot storage[SlotCount];
    };
    static_assert(std::is_trivially_copyable<State>::value, "Shared state must be pointer-free POD");

    struct Reservation { Token token; unsigned char* data = nullptr; };
    struct Sample {
        Token token;
        uint32_t reader = 0;
        uint32_t size = 0;
        uint64_t sequence = 0;
        const unsigned char* data = nullptr;
    };

    // Caller initializes once before publishing the mapping to other processes.
    // Other processes attach only after an external initialization handshake.
    static State* initialize(void* address, size_t bytes, Session session) {
        if (!address || reinterpret_cast<uintptr_t>(address) % alignof(State) ||
                bytes != sizeof(State) || !session.valid()) return nullptr;
        auto* state = new (address) State{};
        state->magic_value = magic;
        state->layout_version = version;
        state->slots = uint32_t(SlotCount);
        state->payload_size = uint32_t(PayloadBytes);
        state->readers = uint32_t(ReaderCount);
        state->session = session;
        return state;
    }
    static bool compatible(const State* state, size_t bytes, Session expected) {
        return state && bytes == sizeof(State) && expected.valid() &&
            reinterpret_cast<uintptr_t>(state) % alignof(State) == 0 &&
            state->magic_value == magic && state->layout_version == version &&
            state->slots == SlotCount && state->payload_size == PayloadBytes &&
            state->readers == ReaderCount && state->session == expected;
    }

    explicit Pool(State& state) : state_(&state) {}
    State& state() const { return *state_; }
    uint32_t notification_snapshot() const {
        return __atomic_load_n(&state_->changed, __ATOMIC_ACQUIRE);
    }
    uint64_t watermark() const {
        return __atomic_load_n(&state_->published_watermark, __ATOMIC_ACQUIRE);
    }

    Result reserve(Reservation& out) {
        if (out.data) return Result::Invalid;
        if (__atomic_load_n(&state_->closed, __ATOMIC_ACQUIRE)) return Result::Closed;
        if (watermark() == UINT64_MAX) return Result::SequenceExhausted;
        for (uint32_t step = 0; step < SlotCount; ++step) {
            const auto index = uint32_t((uint64_t(state_->next_slot) + step) % SlotCount);
            auto& slot = state_->storage[index];
            auto old = load(slot.control);
            if (flags(old) || generation(old) == UINT32_MAX) continue;
            const uint32_t next = generation(old) + 1;
            // Acquire the release sequence of all previous pin returns before
            // allowing the caller to overwrite payload bytes.
            if (!cas(slot.control, old, make(next, writer_bit | writing_bit))) continue;
            state_->next_slot = uint32_t((uint64_t(index) + 1) % SlotCount);
            __atomic_store_n(&slot.sequence, uint64_t(0), __ATOMIC_RELAXED);
            out.token = {state_->session, index, next};
            out.data = slot.payload;
            return Result::Ok;
        }
        return Result::Exhausted;
    }

    // No allocation, lock, process probe, or syscall in commit. The watermark
    // store is the final publication gate/linearization point. The later changed
    // increment is an advisory persistent notification. Kernel wake is separate.
    Result commit(const Reservation& sample, uint32_t size, uint64_t sequence,
            uint32_t targets = pending_mask) {
        if (!matches(sample.token) || size > PayloadBytes || !sequence ||
                (targets & ~pending_mask) || sample.data != slot(sample.token).payload)
            return Result::Invalid;
        if (__atomic_load_n(&state_->closed, __ATOMIC_ACQUIRE)) return Result::Closed;
        const auto previous = watermark();
        if (previous == UINT64_MAX) return Result::SequenceExhausted;
        if (sequence <= previous) return Result::Invalid;
        auto& current = slot(sample.token);
        auto expected = make(sample.token.generation, writer_bit | writing_bit);
        if (load(current.control) != expected) return Result::Invalid;
        current.size = size;
        __atomic_store_n(&current.sequence, sequence, __ATOMIC_RELAXED);
        if (!cas(current.control, expected, make(sample.token.generation, writer_bit | targets)))
            return Result::Invalid;
        __atomic_store_n(&state_->published_watermark, sequence, __ATOMIC_RELEASE);
        changed();
        return Result::Ok;
    }

    Result abort(const Reservation& sample) {
        if (!matches(sample.token) || sample.data != slot(sample.token).payload) return Result::Invalid;
        auto& current = slot(sample.token);
        auto expected = make(sample.token.generation, writer_bit | writing_bit);
        if (!cas(current.control, expected, make(sample.token.generation, 0))) return Result::Invalid;
        changed();
        return Result::Ok;
    }

    // Publication pin remains until all Writer/history owners have released
    // their *local* shared owner. Each shared Writer bit is returned exactly once.
    Result release_writer(const Token& token) {
        return clear_pin(token, writer_bit, true);
    }

    Result take(uint32_t reader, Sample& out) {
        if (reader >= ReaderCount || out.data) return Result::Invalid;
        const uint32_t pending = uint32_t(1) << reader;
        const uint32_t loaned = pending << 15;
        // Without this snapshot a scan could miss a newly published earlier
        // slot, see a later slot, and deliver sequence N+1 before N.
        const uint64_t ceiling = watermark();
        uint32_t selected = uint32_t(SlotCount);
        uint64_t first = UINT64_MAX;
        for (uint32_t index = 0; index < SlotCount; ++index) {
            const auto& current = state_->storage[index];
            auto control = load(current.control);
            if ((flags(control) & (pending | writing_bit)) != pending) continue;
            auto sequence = __atomic_load_n(&current.sequence, __ATOMIC_ACQUIRE);
            if (!sequence || sequence > ceiling) continue;
            if (selected == SlotCount || sequence < first) { selected = index; first = sequence; }
        }
        if (selected == SlotCount) {
            // A close racing this scan could follow a publication above the
            // snapshot ceiling. Return Empty and let the caller rescan first.
            if (__atomic_load_n(&state_->closed, __ATOMIC_ACQUIRE) && watermark() == ceiling)
                return Result::Closed;
            return Result::Empty;
        }
        auto& current = state_->storage[selected];
        auto old = load(current.control);
        // Only this serialized Reader consumes its pending bit; other Readers
        // can alter only their own bits. Every failed strong CAS means another
        // pin operation progressed. Own pending prevents reuse throughout.
        for (;;) {
            if ((flags(old) & (pending | loaned | writing_bit)) != pending) return Result::Invalid;
            auto next = (old & ~uint64_t(pending)) | loaned;
            if (cas(current.control, old, next)) break;
        }
        out.token = {state_->session, selected, generation(old)};
        out.reader = reader;
        out.size = current.size;
        out.sequence = __atomic_load_n(&current.sequence, __ATOMIC_RELAXED);
        out.data = current.payload;
        return Result::Ok;
    }

    Result release_reader(const Token& token, uint32_t reader) {
        if (reader >= ReaderCount) return Result::Invalid;
        return clear_pin(token, uint32_t(1) << (reader + 15), false);
    }
    Result discard_pending(const Token& token, uint32_t reader) {
        if (reader >= ReaderCount) return Result::Invalid;
        return clear_pin(token, uint32_t(1) << reader, false);
    }
    void close_writer() {
        __atomic_store_n(&state_->closed, uint32_t(1), __ATOMIC_RELEASE);
        changed();
    }

private:
    State* state_;
    static uint64_t load(const uint64_t& value) { return __atomic_load_n(&value, __ATOMIC_ACQUIRE); }
    static bool cas(uint64_t& target, uint64_t& expected, uint64_t value) {
        return __atomic_compare_exchange_n(&target, &expected, value, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
    }
    static uint32_t flags(uint64_t control) { return uint32_t(control); }
    static uint32_t generation(uint64_t control) { return uint32_t(control >> 32); }
    static uint64_t make(uint32_t generation, uint32_t flags) { return (uint64_t(generation) << 32) | flags; }
    bool matches(const Token& token) const {
        return token.slot < SlotCount && token.generation && token.session == state_->session;
    }
    Slot& slot(const Token& token) const { return state_->storage[token.slot]; }
    void changed() { __atomic_add_fetch(&state_->changed, uint32_t(1), __ATOMIC_RELEASE); }
    Result clear_pin(const Token& token, uint32_t bit, bool published_writer_only) {
        if (!matches(token)) return Result::Invalid;
        auto& current = slot(token);
        auto old = load(current.control);
        for (;;) {
            if (generation(old) != token.generation || !(flags(old) & bit) ||
                    (published_writer_only && (flags(old) & writing_bit))) return Result::Invalid;
            if (cas(current.control, old, old & ~uint64_t(bit))) { changed(); return Result::Ok; }
        }
    }
};

} // namespace travodds_experimental
