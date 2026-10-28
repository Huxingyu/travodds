// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#ifndef TRAVODDS_COMMON_MEMORY_PAYLOADBUFFER_H
#define TRAVODDS_COMMON_MEMORY_PAYLOADBUFFER_H

#include "common/memory/payloadpool.h"
#include "cdr/travoddscdr.h"
#include "dcps/base/retcode.h"
#include <memory>

TRAVODDS_NAMESPACE_BEGIN

// Own both the process-local descriptors and their originating payload pool.
// Use for rollback until a chain has been transferred to a CacheChange.
struct PayloadBufferDeleter {
    std::shared_ptr<IPayloadPool> pool;

    void operator()(SerializedBuffer* buffer) const {
        while (buffer) {
            auto* current = buffer;
            buffer = current->next;
            if (current->origin_buffer) {
                pool->releaseBuffer(current->origin_buffer);
            }
            delete current;
        }
    }
};

using PayloadBufferPtr = std::unique_ptr<SerializedBuffer, PayloadBufferDeleter>;

// Build a CDR payload chain with RTPS fragment limits and transport headroom.
// Output is null on failure, including partial allocation failure. The first
// descriptor starts after the CDR encapsulation; restore that prefix to send.
ReturnCode_t CreateSerializedPayload(const std::shared_ptr<IPayloadPool>& pool,
    uint32_t dataLen, SerializedBuffer*& bufferHeader);

TRAVODDS_NAMESPACE_END
#endif
