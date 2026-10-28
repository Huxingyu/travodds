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
constexpr uint32_t VERSION = 5;
constexpr uint32_t MAX_READERS = 64;
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
    unsigned char guid[16];
};
struct Header {
    Prefix prefix;
    pthread_mutex_t mutex;
    alignas(4) uint32_t changed;
    uint32_t alive, poisoned, next_slot, reserved;
    uint64_t last_sequence;
    ReaderRecord readers[MAX_READERS];
};
struct alignas(64) Slot {
    uint64_t generation, sequence, pending, loaned;
    uint32_t size, state, writer_held, reserved;
    SharedSampleMetadata metadata;
};

static_assert(std::is_trivially_copyable<Header>::value, "Shared metadata must contain no owning C++ objects");
static_assert(std::is_trivially_copyable<Slot>::value, "Shared slot metadata must be POD");
}
TRAVODDS_NAMESPACE_END
#endif
#endif
