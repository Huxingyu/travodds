// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#ifndef TRAVODDS_COMMON_MEMORY_PAYLOADPOOL_H
#define TRAVODDS_COMMON_MEMORY_PAYLOADPOOL_H

#include "type/basictype.h"

TRAVODDS_NAMESPACE_BEGIN

/**
 * @brief Allocation boundary for history payload storage.
 *
 * Callers must return buffers to the pool that allocated them, and keep that
 * pool alive until the last buffer is returned. This interface does not imply
 * that storage is shared, suitably aligned for arbitrary objects, or loanable.
 */
class IPayloadPool {
public:
    virtual ~IPayloadPool() = default;

    /** Allocate at least size bytes; buffer is null on allocation failure. */
    virtual bool getBuffer(uint32_t size, char*& buffer) = 0;

    /** Return a buffer allocated by this pool, clearing it on success. */
    virtual bool releaseBuffer(char*& buffer) = 0;
};

TRAVODDS_NAMESPACE_END
#endif
