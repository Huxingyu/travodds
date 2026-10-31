// Internal same-host wire layout. Change VERSION when changing this file.
#ifndef TRAVODDS_SHAREDPAYLOADPOOL_LAYOUT_H
#define TRAVODDS_SHAREDPAYLOADPOOL_LAYOUT_H
#include "common/memory/sharedpayloadpool.h"
#if defined(__linux__)
#include <pthread.h>
#include <type_traits>
TRAVODDS_NAMESPACE_BEGIN
namespace shared_pool_detail {
constexpr uint64_t MAGIC = 0x545241564f445331ull;
constexpr uint32_t VERSION = 7;
constexpr uint32_t MAX_READERS = 64;
constexpr uint32_t NO_SLOT = UINT32_MAX;
enum SlotState : uint32_t { FREE, WRITING, PUBLISHED };
struct ProcessIdentity { int64_t pid; uint64_t start; };
struct Prefix {
    uint64_t magic;
    uint32_t version, header_bytes, slot_bytes, mutex_bytes, notification_bytes;
    uint32_t payload_size, slots, max_readers, alignment;
    uint64_t segment_bytes;
    uint64_t pid_namespace_device, pid_namespace_inode;
    unsigned char writer[16];
    SharedPoolSession session;
    ProcessIdentity creator;
};
struct ReaderRecord {
    ProcessIdentity process; uint64_t generation; uint32_t active; uint32_t accepting; uint32_t replay;
    // Low two bits: OPEN(0), CLOSING(1), RETIRED(2). Higher bits count
    // short operations. A registration cannot be reused while any old
    // operation or application Loan still owns it.
    uint32_t ready;
    alignas(8) uint64_t gate;
    unsigned char guid[16];
};
struct Header {
    Prefix prefix;
    pthread_mutex_t mutex;
    alignas(4) uint32_t changed;
    uint32_t alive, poisoned, history_head, history_tail;
    // Absolute next reservation position; position % slots is its physical
    // cell. Positions never wrap, even when a pinned cell must be skipped.
    uint64_t ring_tail, last_sequence;
    // Even epochs describe a complete published-history ring. A mutator
    // holds publishing and exposes an odd epoch until every link is ready.
    uint64_t history_epoch;
    uint32_t publishing, history_count;
    ReaderRecord readers[MAX_READERS];
};
struct alignas(64) Slot {
    // Low two bits hold SlotState; 3 is the reserve-only frozen state.
    // Higher bits count short operations, independently of persistent pins.
    // generation remains a full 64-bit value and never wraps.
    uint64_t gate;
    uint64_t generation, ring_position, sequence, pending, loaned;
    uint32_t size, state, writer_held, reserved;
    // Published samples form one shared circular doubly-linked history in
    // successful DDS publication order. Unsubmitted/cancelled Loans are not
    // members. These fields are accessed with interprocess atomic operations.
    uint32_t history_previous, history_next, history_linked, reserved2;
    SharedSampleMetadata metadata;
};

static_assert(std::is_trivially_copyable<Header>::value, "Shared metadata must contain no owning C++ objects");
static_assert(std::is_trivially_copyable<Slot>::value, "Shared slot metadata must be POD");
}
TRAVODDS_NAMESPACE_END
#endif
#endif
