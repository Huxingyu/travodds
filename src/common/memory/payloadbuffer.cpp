// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#include "common/memory/payloadbuffer.h"
#include "rtps/basictypertps.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

TRAVODDS_NAMESPACE_BEGIN

ReturnCode_t CloneSerializedPayload(const std::shared_ptr<IPayloadPool>& pool,
        const SerializedBuffer* source, SerializedBuffer*& bufferHeader)
{
    bufferHeader = nullptr;
    if (!pool || !source) return RETCODE_BAD_PARAMETER;
    PayloadBufferPtr chain(nullptr, PayloadBufferDeleter{pool});
    SerializedBuffer* tail = nullptr;
    for (; source; source = source->next) {
        if (!source->buffer || !source->buffer_size) return RETCODE_BAD_PARAMETER;
        PayloadBufferPtr part(new (std::nothrow) SerializedBuffer, PayloadBufferDeleter{pool});
        if (!part || !pool->getBuffer(source->buffer_size, part->origin_buffer))
            return RETCODE_APPLY_MEM_ERR;
        part->buffer = part->origin_buffer;
        part->buffer_size = source->buffer_size;
        part->writer_position = source->buffer_size;
        std::memcpy(part->buffer, source->buffer, source->buffer_size);
        auto* current = part.get();
        if (tail) tail->next = part.release();
        else chain = std::move(part);
        tail = current;
    }
    bufferHeader = chain.release();
    return RETCODE_OK;
}

ReturnCode_t CreateSerializedPayload(const std::shared_ptr<IPayloadPool>& pool,
        uint32_t dataLen, SerializedBuffer*& bufferHeader)
{
	bufferHeader = nullptr;
	if (!pool || dataLen > std::numeric_limits<uint32_t>::max() - CDR_ENDIANNESS_LEN) {
		return RETCODE_BAD_PARAMETER;
	}
	dataLen += CDR_ENDIANNESS_LEN;
	PayloadBufferPtr chain(nullptr, PayloadBufferDeleter{pool});
	SerializedBuffer* tail = nullptr;
	do {
		PayloadBufferPtr part(new (std::nothrow) SerializedBuffer, PayloadBufferDeleter{pool});
		if (!part) {
			return RETCODE_APPLY_MEM_ERR;
		}
		part->buffer_size = std::min<uint32_t>(dataLen, DATAMESSAGE_LIMIT);
		dataLen -= part->buffer_size;
		if (!pool->getBuffer(part->buffer_size + 2 * BUFF_SHIFT, part->origin_buffer)) {
			return RETCODE_APPLY_MEM_ERR;
		}
		part->buffer = part->origin_buffer + BUFF_SHIFT;
		auto* next = part.get();
		if (!chain) {
			memset(part->buffer, 0, CDR_ENDIANNESS_LEN);
			part->buffer[1] = DEFAULT_ENDIAN;
			part->buffer += CDR_ENDIANNESS_LEN;
			part->buffer_size -= CDR_ENDIANNESS_LEN;
			chain = std::move(part);
		} else {
			tail->next = part.release();
		}
		tail = next;
	} while (dataLen != 0);

	bufferHeader = chain.release();
	return RETCODE_OK;
}

TRAVODDS_NAMESPACE_END
