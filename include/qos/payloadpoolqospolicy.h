// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#ifndef TRAVODDS_QOS_PAYLOADPOOLQOSPOLICY_H
#define TRAVODDS_QOS_PAYLOADPOOLQOSPOLICY_H
#include "travodds_dll.h"
#include <cstddef>
#include <cstdint>
#include <limits>

TRAVODDS_NAMESPACE_BEGIN

enum class PayloadMemoryPolicy {
    PREALLOCATED,
    PREALLOCATED_WITH_REALLOC,
    DYNAMIC_RESERVE,
    DYNAMIC_REUSABLE
};

// Local allocation policy, shared by Writers on the same domain/topic/type.
// Counts refer to serialized fragments, rather than DDS samples. This policy
// is independent of the native DataSharing slot budget and is not advertised.
struct PayloadPoolQosPolicy {
    PayloadMemoryPolicy policy = PayloadMemoryPolicy::DYNAMIC_REUSABLE;
    uint32_t payload_size = 0;
    uint32_t max_payloads = std::numeric_limits<uint32_t>::max();
    uint32_t initial_payloads = 0;
    size_t alignment = alignof(std::max_align_t);

    bool valid() const {
        switch (policy) {
            case PayloadMemoryPolicy::PREALLOCATED:
            case PayloadMemoryPolicy::PREALLOCATED_WITH_REALLOC:
                if (!payload_size) return false;
                break;
            case PayloadMemoryPolicy::DYNAMIC_RESERVE:
                if (initial_payloads) return false;
                break;
            case PayloadMemoryPolicy::DYNAMIC_REUSABLE:
                break;
            default: return false;
        }
        return max_payloads && initial_payloads <= max_payloads &&
            alignment >= alignof(void*) && !(alignment & (alignment - 1));
    }
    bool operator==(const PayloadPoolQosPolicy& other) const {
        return policy == other.policy && payload_size == other.payload_size &&
            max_payloads == other.max_payloads && initial_payloads == other.initial_payloads &&
            alignment == other.alignment;
    }
};
TRAVODDS_NAMESPACE_END
#endif
