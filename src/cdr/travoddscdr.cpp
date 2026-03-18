//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "common/log/logger.h"
#include "cdr/travoddscdr.h"
#include "cdr/md5.h"

USING_TRAVODDS_NAMESPACE

SerializedBuffer* SerializedBuffer::writeData(char* data, uint32_t length) 
{
    if (length == 0) {
        return this;
    }
    // 当前块足够，直接写入
    if (bufferSpace() >= length) {
        memcpy(buffer + writer_position, data, length);
        writer_position += length;
        return this;
    }
    // 看总体的空间是否足够
    uint32_t curTotalSpace = bufferTotalSpace();
    if (curTotalSpace < length) {
        TRAVODDS_LOG(LOG_ERROR, "CdrSerializer", "Not enough space to write data, need %u, but only %u", length, curTotalSpace);
        return NULL;
    }
    // 总体空间足够，依次写入
    uint32_t offset = 0;
    SerializedBuffer* curBuffer = this;
    while (length > 0) {
        uint32_t writeLength = curBuffer->bufferSpace();
        bool needNext = writeLength < length;
        if (!needNext) {
            writeLength = length;
        }
        memcpy(curBuffer->buffer + curBuffer->writer_position, data + offset, writeLength);
        curBuffer->writer_position += writeLength;
        offset += writeLength;
        length -= writeLength;
        if (needNext) {
            // 如果当前块写满了，换下一个块
            curBuffer = curBuffer->next;
        }
    }
    return curBuffer;
}

SerializedBuffer* SerializedBuffer::readData(char* data, uint32_t length) 
{
    if (length == 0) {
        return this;
    }
    // 当前块足够，直接读取
    if (bufferLength() >= length) {
        if (data) {
            memcpy(data, buffer + reader_position, length);
        }
        reader_position += length;
        return this;
    }
    // 看总体的长度是否足够
    uint32_t curTotalLength = bufferTotalLength();
    if (curTotalLength < length) {
        TRAVODDS_LOG(LOG_ERROR, "CdrSerializer", "Not enough length to read data, need %u, but only %u", length, curTotalLength);
        return NULL;
    }
    // 总体长度足够，依次读取
    uint32_t offset = 0;
    SerializedBuffer* curBuffer = this;
    while (length > 0) {
        uint32_t readLength = curBuffer->bufferLength();
        bool needNext = readLength < length;
        if (!needNext) {
            readLength = length;
        }
        if (data) {
            memcpy(data + offset, curBuffer->buffer + curBuffer->reader_position, readLength);
        }
        curBuffer->reader_position += readLength;
        offset += readLength;
        length -= readLength;
        if (needNext) {
            // 如果当前块读满了，换下一个块
            curBuffer = curBuffer->next;
        }
    }
    return curBuffer;
}

bool CdrSerializer::isLittleEndian() const
{
    union agicdr_endian_test {
        uint32_t i;
        char c[sizeof(uint32_t)];
    };
    agicdr_endian_test test;
    test.i = 1;
    return test.c[0] == 1;
}

uint32_t CdrSerializer::getRemainingSpace() const
{
    uint32_t remainingSpace = 0;
    SerializedBuffer* current = curProcessMsgBlock_;
    while (current) {
        remainingSpace += current->bufferSpace();
        current = current->next;
    }
    return remainingSpace;
}

CdrSerializer::CdrSerializer(SerializedBuffer* buffer, bool littleEndian)
    : tgaMsgBlock_(buffer),
    curProcessMsgBlock_(buffer), 
    totalSpace_(0), 
    index_(0), 
    useLittleEndian_(littleEndian)
{
    SerializedBuffer* current = buffer;
    while (current)
    {
        totalSpace_ += current->buffer_size;
        current = current->next;
    }
}

bool CdrSerializer::convertUft32ToUtf16(const wchar_t utf32Value, uint16_t* utf16Value)
{
    uint32_t codePoint = static_cast<uint32_t>(utf32Value);
    if (codePoint < 0x10000) {
        utf16Value[0] = static_cast<uint16_t>(codePoint);
    } else if (codePoint <= 0x10FFFF) {
        // 转换为surrogate pair
        codePoint -= 0x10000;
        utf16Value[0] = static_cast<uint16_t>((codePoint >> 10) + 0xD800);
        utf16Value[1] = static_cast<uint16_t>((codePoint & 0x3FF) + 0xDC00);
    } else {
        TRAVODDS_LOG(LOG_ERROR, "Invalid wchar_t value: %u\n", codePoint);
        return false;
    }
    return true;
}

bool CdrSerializer::serializeBaseType(wchar_t value) 
{
    if (sizeof(wchar_t) == 2) {
        // 如果是utf16编码，如windows，直接序列化
        return serializeBaseType<uint16_t>(static_cast<uint16_t>(value));
    } else if (sizeof(wchar_t) == 4) {
        // 如果是utf32编码，如linux，转化为utf16编码后序列化
        uint16_t utf16Value[2] = {0, 0};
        if (!convertUft32ToUtf16(value, utf16Value)) {
            return false;
        }
        // 当前仅考虑utf16与uft32编码相同的宽字符
        return serializeBaseType<uint16_t>(utf16Value[0]);
    } else {
        TRAVODDS_LOG(LOG_ERROR, "Unsupported wchar_t size: %zu\n", sizeof(wchar_t));
        return false;
    }
}

bool CdrSerializer::serializeBaseTypeArray(const wchar_t* value, uint32_t length) 
{
    if (length == 0) {
        return true;
    }
    if (sizeof(wchar_t) == 2) {
        // 如果是utf16编码，如windows，直接序列化
        return serializeBaseTypeArray<uint16_t>(reinterpret_cast<const uint16_t*>(value), length);
    } else if (sizeof(wchar_t) == 4) {
        // 如果是utf32编码，如linux，转化为utf16编码后序列化
        for (uint32_t i = 0; i < length; i++) {
            if (!serializeBaseType(value[i])) {
                TRAVODDS_LOG(LOG_ERROR, "serialize wchar_t array index[%u] failed.\n", i);
                return false;
            }
        }
        return true;
    } else {
        TRAVODDS_LOG(LOG_ERROR, "Unsupported wchar_t size: %zu\n", sizeof(wchar_t));
        return false;
    }
}

bool CdrSerializer::serializeLongDouble(LongDouble value) 
{
    // uint32_t ldSize = sizeof(long double);
    static char s_alignedData[8] = { 0 };
    // 16字节按照8字节对齐
    uint32_t offset = (~index_ + 1) & (8 - 1);
    index_ += offset;
    curProcessMsgBlock_->writeData(s_alignedData, offset);
    // 填充0
    if (useLittleEndian_ == isLittleEndian()) {
        return serializeBaseTypeArray(value.c, sizeof(LongDouble));
    } else {
        // 交换value.c的字节序
        LongDouble tmpValue = value;
        // 交换字节序
        for (size_t i = 0; i < sizeof(LongDouble) / 2; ++i) {
            tmpValue.c[i] = tmpValue.c[sizeof(LongDouble) - 1 - i];
        }
        return serializeBaseTypeArray(tmpValue.c, sizeof(LongDouble));
    }
}

bool CdrSerializer::serializeLongDoubleArray(const LongDouble* value, uint32_t length) 
{
    for (uint32_t i = 0; i < length; ++i) {
        if (!serializeLongDouble(value[i])) {
            return false;
        }
    }
    return true;
}

bool CdrSerializer::serializeString(const char* value, uint32_t length)
{
    if (!serializeBaseType<uint32_t>(length)) {
        return false;
    }
    if (!serializeBaseTypeArray<char>(value, length)) {
        return false;
    }
    return true;
}

bool CdrSerializer::serializeString(const std::string& value)
{
    return serializeString(value.c_str(), static_cast<uint32_t>(value.size() + 1));
}

bool CdrSerializer::serializeWString(const wchar_t* value, uint32_t length)
{
    if (!serializeBaseType<uint32_t>(length)) {
        return false;
    }
    if (!serializeBaseTypeArray(value, length)) {
        return false;
    }
    return true;
}

bool CdrSerializer::serializeWString(const std::wstring& value)
{
    // 协议说wstring不应该包含null字符，所以这里不加1
    return serializeWString(value.c_str(), static_cast<uint32_t>(value.size()));
}

bool CdrSerializer::getKeyHash(char* keyHash, bool forceMd5)
{
    (void)forceMd5;
	MD5 md5;
	SerializedBuffer* current = tgaMsgBlock_;
	// 依次更新到md5中
	while (current) {
		md5.update((uint8_t*)current->buffer, current->writer_position);
		current = current->next;
	}
	md5.finalize((uint8_t*)keyHash);
	return true;
}

uint32_t CdrSerializer::getBaseTypeSize(uint32_t typeSize, uint32_t currentAlignment)
{
    uint32_t initialAlignment = currentAlignment;
    // 16字节的按照8字节对齐
    currentAlignment += ((~currentAlignment + 1) & ((typeSize == 16) ? 7 : typeSize - 1)) + typeSize;
    return currentAlignment - initialAlignment;
}

uint32_t CdrSerializer::getBaseTypeArraySize(uint32_t typeSize, uint32_t length, uint32_t currentAlignment)
{
    if (length == 0) {
        return 0;
    }
    uint32_t initialAlignment = currentAlignment;
    // 16字节的按照8字节对齐
    currentAlignment += ((~currentAlignment + 1) & ((typeSize == 16) ? 7 : typeSize - 1)) + typeSize;
    currentAlignment += (length - 1) * typeSize;
    return currentAlignment - initialAlignment;
}

uint32_t CdrSerializer::getStringSize(uint32_t length, uint32_t currentAlignment)
{
    uint32_t initialAlignment = currentAlignment;
    currentAlignment += ((~currentAlignment + 1) & 3) + 4;
    currentAlignment += length;
    return currentAlignment - initialAlignment;
}

uint32_t CdrSerializer::getWStringSize(uint32_t length, uint32_t currentAlignment)
{
    uint32_t initialAlignment = currentAlignment;
    currentAlignment += ((~currentAlignment + 1) & 3) + 4;
    currentAlignment += getBaseTypeArraySize(sizeof(wchar_t), length, currentAlignment);
    return currentAlignment - initialAlignment;
}

char* CdrSerializer::serializeParameterHeader(bool useExtended, bool mustUnderstand, uint32_t memberId)
{
    (void)mustUnderstand;
    char* lenBlock = nullptr;
    unsigned short tmpShort = 0;
    uint32_t lengthSize = 2;
    if (useExtended) {
        tmpShort |= 0x3f00;
        tmpShort |= 0x4000;
        if (!serializeBaseType<unsigned short>(tmpShort)) {
            return nullptr;
        }
        index_ = 2;
        tmpShort = 8;
        if (!serializeBaseType<unsigned short>(tmpShort)) {
            return nullptr;
        }
        if (!serializeBaseType<unsigned short>(memberId)) {
            return nullptr;
        }
        lengthSize = 4;
    } else {
        tmpShort |= memberId;
        if (!serializeBaseType<unsigned short>(tmpShort)) {
            return nullptr;
        }
    }
    tmpShort = 0;
    if (curProcessMsgBlock_->writer_position + lengthSize <= curProcessMsgBlock_->buffer_size) {
        lenBlock = curProcessMsgBlock_->buffer + curProcessMsgBlock_->writer_position;
    }
    if (!serializeBaseType<unsigned short>(tmpShort)) {
        return nullptr;
    }
    index_ = 0;
    return lenBlock;
}

void CdrSerializer::parameterLenSerialize(char* lenAddr, uint32_t length, unsigned short lengthSize)
{
    uint32_t actualLen = length;
    uint32_t offset = (actualLen & 3) == 0 ? 0 : (4 - (actualLen & 3));
    if (offset > 0) {
        char alignBuf[3] = { 0 };
        if (!serializeBaseTypeArray<char>(alignBuf, offset)) {
            return;
        }
        actualLen += offset;
    }
    unsigned short len16 = actualLen;
    if (useLittleEndian_ != isLittleEndian()) {
        actualLen = __builtin_bswap32(actualLen);
        len16 = __builtin_bswap16(len16);
    }
    if (lengthSize == 2) {
        memcpy(lenAddr, &len16, lengthSize);
    } else {
        memcpy(lenAddr, &actualLen, lengthSize);
    }
}

void CdrSerializer::aheadWriterPosition(uint32_t length)
{
    curProcessMsgBlock_->writer_position += length;
}

CdrDeserializer::CdrDeserializer(SerializedBuffer* buffer, bool littleEndian)
    : tgaMsgBlock_(buffer),
      curProcessMsgBlock_(buffer),
      totalRemains_(0),
      index_(0),
      useLittleEndian_(littleEndian)
{
    SerializedBuffer* current = buffer;
    while (current) {
        totalRemains_ += current->buffer_size;
        current = current->next;
    }
}

bool CdrDeserializer::convertUft16ToUtf32(const uint16_t utf16Value, wchar_t& utf32Value)
{
    // 当前仅考虑utf16编码和utf32编码相同的情况
    utf32Value = static_cast<uint32_t>(utf16Value);
    return true;
}

bool CdrDeserializer::deserializeBaseType(wchar_t& value)
{
    if (sizeof(wchar_t) == 2) {
        // 如果是utf16编码，如windows，直接序列化
        uint16_t int16Val;
        if (deserializeBaseType<uint16_t>(int16Val)) {
            value = static_cast<wchar_t>(int16Val);
            return true;
        }
        return false;
    }
    else if (sizeof(wchar_t) == 4) {
        // 如果是utf32编码，如linux，反序列化后转化为utf32
        uint16_t int16Val;
        if (!deserializeBaseType<uint16_t>(int16Val)) {
            return false;
        }
        if (!convertUft16ToUtf32(int16Val, value)) {
            return false;
        }
        return true;
    }
    else {
        TRAVODDS_LOG(LOG_ERROR, "Unsupported wchar_t size: %zu\n", sizeof(wchar_t));
        return false;
    }
}

bool CdrDeserializer::deserializeBaseTypeArray(wchar_t* value, uint32_t length)
{
    if (length == 0) {
        return true;
    }
    if (sizeof(wchar_t) == 2) {
        // 如果是utf16编码，如windows，直接序列化
        return deserializeBaseTypeArray<uint16_t>(reinterpret_cast<uint16_t*>(value), length);
    }
    else if (sizeof(wchar_t) == 4) {
        // 如果是utf32编码，如linux，转化为utf16编码后序列化
        for (uint32_t i = 0; i < length; i++) {
            if (!deserializeBaseType(value[i])) {
                TRAVODDS_LOG(LOG_ERROR, "deserialize wchar_t array index[%u] failed.\n", i);
                return false;
            }
        }
        return true;
    }
    else {
        TRAVODDS_LOG(LOG_ERROR, "Unsupported wchar_t size: %zu\n", sizeof(wchar_t));
        return false;
    }
}

bool CdrDeserializer::isLittleEndian() const
{
    union agicdr_endian_test {
        uint32_t i;
        char c[sizeof(uint32_t)];
    };
    agicdr_endian_test test;
    test.i = 1;
    return (test.c[0] == 1);
}

bool CdrDeserializer::deserializeString(char* value, uint32_t length)
{
    uint32_t strLen = 0;
    if (!deserializeBaseType<uint32_t>(strLen)) {
        return false;
    }
    if (strLen > length) {
        return false;
    }
    SerializedBuffer* retBuffer = curProcessMsgBlock_->readData(value, strLen);
    if (!retBuffer) {
        return false;
    }
    curProcessMsgBlock_ = retBuffer;
    totalRemains_ -= strLen;
    index_ += strLen;
    return true;
}

bool CdrDeserializer::deserializeString(std::string& value)
{
    uint32_t strLen = 0;
    if (!deserializeBaseType<uint32_t>(strLen)) {
        return false;
    }
    value.resize(strLen);
    if (!deserializeBaseTypeArray<char>(&value[0], strLen)) {
        return false;
    }
    // 去除末尾的/0
    if (strLen > 0) {
        value.resize(strLen - 1);
    }
    return true;
}

bool CdrDeserializer::deserializeWString(std::wstring& value)
{
    uint32_t strLen = 0;
    if (!deserializeBaseType<uint32_t>(strLen)) {
        return false;
    }
    value.resize(strLen);
    if (!deserializeBaseTypeArray(&value[0], strLen)) {
        return false;
    }
    return true;
}

uint32_t CdrDeserializer::getBufferLength() const
{
    return totalRemains_;
}

bool CdrDeserializer::deserializeParameterHeader(uint32_t& memberId, uint32_t& length,
    bool& useExtended, bool& mustUnderstand)
{
    unsigned short pid = 0;
    if (!deserializeBaseType<unsigned short>(pid)) {
        return false;
    }
    useExtended = ((pid & 0x4000) != 0);
    mustUnderstand = false; 
    if (useExtended) {
        unsigned short lengthShort = 0;
        if (!deserializeBaseType<unsigned short>(lengthShort)) {
            return false;
        }
        unsigned short mid = 0;
        if (!deserializeBaseType<unsigned short>(mid)) {
            return false;
        }
        memberId = mid;
        unsigned short ln = 0;
        if (!deserializeBaseType<unsigned short>(ln)) {
            return false;
        }
        length = ln;
    } else {
        memberId = pid & 0x3FFF;
        unsigned short ln = 0;
        if (!deserializeBaseType<unsigned short>(ln)) {
            return false;
        }
        length = ln;
    }
    return true;
}

bool CdrDeserializer::aheadReaderPosition(uint32_t beginIndex, uint32_t length)
{
    uint32_t readLen = beginIndex - totalRemains_;
    if (readLen > length) {
        return false;
    }
    length -= readLen;
    index_ += length;
    totalRemains_ -= length;
    while (curProcessMsgBlock_ && length > 0) {
        uint32_t curlen = curProcessMsgBlock_->bufferLength();
        uint32_t actualLen = length < curlen ? length : curlen;
        SerializedBuffer* retBuf = curProcessMsgBlock_->readData(nullptr, actualLen);
        if (!retBuf) {
            return false;
        }
        if (actualLen != length) {
            // 如果当前块读满了，换下一个块
            curProcessMsgBlock_ = curProcessMsgBlock_->next;
        }
        length -= actualLen;
    }
	return true;
}
