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

#ifndef AGICDR_H
#define AGICDR_H

#include <string>
#include <cstring>
#include "agidds_dll.h"

#define PID_LIST_END 0xFFFFFFFF

#ifdef _WIN32
#define __builtin_bswap16 _byteswap_ushort
#define __builtin_bswap32 _byteswap_ulong
#define __builtin_bswap64 _byteswap_uint64
// 取消对编译器的警告C4244
#pragma warning(disable: 4244)
#endif

// 由于各个平台的long double大小不一致，使用union对齐为16字节
union LongDouble {
    long double ld;
    char c[16];
	bool operator<(const LongDouble& other) const {
		return ld < other.ld;
	}
};

class DDS_DLL SerializedBuffer {
public:	
    SerializedBuffer() 
    {
        buffer = nullptr;
		origin_buffer = nullptr;
        buffer_size = 0;
        writer_position = 0;
        reader_position = 0;
        next = nullptr;
    }

    /**
     * @brief 写入数据。
     * 
     * @param data 数据指针。
     * @param length 数据长度。
     * 
     * @return 返回数据写入到的缓冲区。
     */
    SerializedBuffer* writeData(char* data, uint32_t length);

    /** 
     * @brief 读取数据。
     * 
     * @param data 数据指针。
     * @param length 数据长度。
     * 
     * @return 返回数据读取到的缓冲区。
     */
    SerializedBuffer* readData(char* data, uint32_t length);

    uint32_t bufferSpace() 
    {
        return buffer_size - writer_position;
    }

    uint32_t bufferTotalSpace() 
    {
        uint32_t totalSpace = 0;
        for (SerializedBuffer* buffer = this; buffer != nullptr; buffer = buffer->next) {
            totalSpace += buffer->bufferSpace();
        }
        return totalSpace;
    }

    uint32_t bufferLength() 
    {
        return writer_position - reader_position;
    }

    uint32_t bufferTotalLength() 
    {
        uint32_t totalLength = 0;
        for (SerializedBuffer* buffer = this; buffer != nullptr; buffer = buffer->next) {
            totalLength += buffer->bufferLength();
        }
        return totalLength;
    }

public:
	/** @brief 有效数据首地址。 */
    char *buffer;
	/** @brief 内存空间首地址。 */
	char* origin_buffer;
    /** @brief 缓冲区大小。 */
	uint32_t buffer_size;
	/** @brief 已写入位置。 */
	uint32_t writer_position;
	/** @brief 已读取位置。 */
	uint32_t reader_position;
	/** @brief 下一个缓冲区。 */
    SerializedBuffer *next;
};

/**
 * @brief 序列化器。
 */
class DDS_DLL AGIDDSCdrSerializer
{
public:
    /**
     * @brief 构造函数。
     *
     * @param buffer 目标缓冲区。
     * @param littleEndian 是否小端字节序。
     */
    AGIDDSCdrSerializer(SerializedBuffer* buffer, bool littleEndian = true);

    ~AGIDDSCdrSerializer() = default;

    bool serializeLongDouble(LongDouble value);

    bool serializeLongDoubleArray(const LongDouble* value, uint32_t length);
    /**
     * @brief 序列化基本数类型
     * 
     * @tparam T 基本数类型
     * @param value 基本数值
     * 
     * @return 序列化是否成功。
     */
    template<typename T>
    bool serializeBaseType(T value) 
    {
        uint32_t typeSize = sizeof(T);
        static char s_alignedData[15] = { 0 };
        uint32_t offset = (~index_ + 1) & (typeSize - 1);
        SerializedBuffer* retBuffer = curProcessMsgBlock_->writeData(s_alignedData, offset);
        if (!retBuffer) {
            return false;
        }
        curProcessMsgBlock_ = retBuffer;
        index_ += offset;
        totalSpace_ -= offset;
        if (useLittleEndian_ == isLittleEndian()) {
            // 本机端序与要求编码端序相同，直接拷贝进入缓冲区
            retBuffer = curProcessMsgBlock_->writeData((char*)&value, typeSize);
            if (!retBuffer) {
                return false;
            }
            curProcessMsgBlock_ = retBuffer;
        } else {
           T tmpValue = 0;
           switch (typeSize) {
           case 2:
               tmpValue = __builtin_bswap16(*(uint16_t*)&value);
               break;
           case 4:
               tmpValue = __builtin_bswap32(*(uint32_t*)&value);
               break;
           case 8:
               tmpValue = __builtin_bswap64(*(uint64_t*)&value);
               break;
           default:
               return false;
           }
           // 本机端序与要求编码端序相同，直接拷贝进入缓冲区
           retBuffer = curProcessMsgBlock_->writeData((char*)&tmpValue, typeSize);
           if (!retBuffer) {
               return false;
           }
           curProcessMsgBlock_ = retBuffer;
        }
        index_ += typeSize;
        totalSpace_ -= typeSize;
        return true;
    }

    /**
     * @brief 序列化字符串
     * 
     * @param value 字符串
     * @param length 字符串长度
     * 
     * @return 序列化是否成功。
     */
    bool serializeString(const char* value, uint32_t length);

    /**
     * @brief 序列化字符串
     * 
     * @param value 字符串
     * @param length 字符串长度
     * 
     * @return 序列化是否成功。
     */
    bool serializeString(const std::string& value);

    /**
     * @brief 序列化字符串
     *
     * @param value 字符串
     * @param length 字符串长度
     *
     * @return 序列化是否成功。
     */
    bool serializeWString(const wchar_t* value, uint32_t length);

    /**
     * @brief 序列化字符串
     * 
     * @param value 字符串
     * @param length 字符串长度
     * 
     * @return 序列化是否成功。
     */
    bool serializeWString(const std::wstring& value);

    /**
     * @brief 序列化基本数据类型数组。
     * 
     * @tparam T 基本数据类型。
     * @param value 基本数据类型数组。
     * 
     * @return 序列化是否成功。
     */
    template<typename T>
    bool serializeBaseTypeArray(const T* value, uint32_t length) 
    {
        if (length == 0) {
           return true;
        }
        uint32_t typeSize = sizeof(T);
        if (typeSize == 1) {
           // 字节数组时，无需考虑对齐，以及端序
           SerializedBuffer* retBuffer = curProcessMsgBlock_->writeData((char*)value, length);
           if (!retBuffer) {
               return false;
           }
           curProcessMsgBlock_ = retBuffer;
           index_ += length;
           totalSpace_ -= length;
           return true;
        }
        // 其余数组需要第一个元素需要考虑对齐以及端序
        // 第一个数据考虑对齐
        if (!serializeBaseType<T>(value[0])) {
           return false;
        }
        // 如果只有一个元素，则处理完成
        if (length <= 1) {
           return true;
        }
        // 剩下的数据
        if (useLittleEndian_ && isLittleEndian()) {
            // 本机端序与要求编码端序相同，直接拷贝进入缓冲区
            SerializedBuffer* retBuffer = curProcessMsgBlock_->writeData((char*)&value[1], typeSize * (length - 1));
            if (!retBuffer) {
                return false;
            }
            curProcessMsgBlock_ = retBuffer;
            index_ += typeSize * (length - 1);
            totalSpace_ -= typeSize * (length - 1);
        } else {
           for (uint32_t i = 1; i < length; ++i) {
               if (!serializeBaseType<T>(value[i])) {
                   return false;
               }
           }
        }
        return true;
    }

    /**
     * @brief 计算序列化所需空间。
     * 
     * @param typeSize 数据类型大小。
     * @param currentAlignment 当前对齐。
     * 
     * @return 序列化所需空间。
     */
    static uint32_t getBaseTypeSize(uint32_t typeSize, uint32_t currentAlignment);

    /**
     * @brief 计算基本数据类型数组序列化所需空间。
     * 
     * @param typeSize 数据类型大小。
     * @param length 数组长度。
     * @param currentAlignment 当前对齐。
     * 
     * @return 序列化所需空间。
     */
    static uint32_t getBaseTypeArraySize(uint32_t typeSize, uint32_t length, uint32_t currentAlignment);

    /**
     * @brief 计算序列化指定字符串长度所需空间。
     * 
     * @param length 字符串长度。
     * @param currentAlignment 当前对齐。
     * 
     * @return 序列化所需空间。
     */
    static uint32_t getStringSize(uint32_t length, uint32_t currentAlignment);

    /**
     * @brief 计算序列化指定宽字符串长度所需空间。
     * 
     * @param length 字符串长度。
     * @param currentAlignment 当前对齐。
     * 
     * @return 序列化所需空间。
     */
    static uint32_t getWStringSize(uint32_t length, uint32_t currentAlignment);

    /**
     * @brief 序列化参数头，用于扩展CDR模式。
     *
     * @param [in,out]  cdr     指定序列化器。
     * @param   useExtended     是否使用PID_EXTENDED.
     * @param   mustUnderstand  是否必须理解该参数。
     * @param   memberId        参数对应的成员的memberId.
     *
     * @return  NULL表示失败，否则表示回填的长度的地址。
     */
    char *serializeParameterHeader(bool useExtended, bool mustUnderstand, uint32_t memberId);

    /**
     * @brief   回填Parameter的长度。
     *
     * @param [in,out]  lenAddr 参数长度的地址。
     * @param   length          需要填的参数长度。
     * @param   lengthSize      参数长度的字节数。
     *
     * @return  0表示成功，小于0表示失败。
     */
    void parameterLenSerialize(
        char *lenAddr,
        uint32_t length,
        unsigned short lengthSize);

    /**
     * @brief   手动调整CDR序列化器的写指针。
     *
     * @param [in,out]  self    指明目标。
     * @param   length          调整量。
     */
    void aheadWriterPosition(uint32_t length);

    /**
     * @brief 判断是否小端字节序。
     */
    bool isLittleEndian() const;

    /**
     * @brief 获取剩余空间。
     * 
     * @return 剩余空间。
     */
    uint32_t getRemainingSpace() const;
    
    /**
     * @brief 从序列化的内存中获取keyHash的值。
     * 
     * @param [out] keyHash keyHash的值。
     * @param forceMd5 是否强制使用MD5。
     * 
     * @return 是否成功。
     */
    bool getKeyHash(char* keyHash, bool forceMd5 = false);
private:
    /** @brief	序列化目标缓冲区，由SerializedBuffer链表组成，该域表示链表的头指针。 */
    SerializedBuffer* tgaMsgBlock_;

    /**
     * @brief	当前正在处理的SerializedBuffer缓冲区.
     *
     * @details	因为缓冲区可能是SerializedBuffer组成的链表，利用这个字段可以减少链表遍历以查找当前处理到哪个SerializedBuffer的开销。
     */
    SerializedBuffer* curProcessMsgBlock_;

    /** @brief	当前还剩下的总的字节数。 */
    uint32_t totalSpace_;

    /**
     * @brief	该字段从0开始表示已经使用了多少个字节，该信息用来计算对齐，例如当前使用了7个字节，
     * 			此时需要序列化一个4字节的类型，那么可以很简单的计算出需要4-(7%4) = 1个字节的对齐空间.
     */
    uint32_t index_;
    
    /** @brief  指明编码方式，true表示使用小端，false表示大端编码 */
    bool useLittleEndian_;
};

/**
 * @brief 反序列化器。
 */
class DDS_DLL AGIDDSCdrDeserializer {
public:
    AGIDDSCdrDeserializer(SerializedBuffer* buffer, bool littleEndian = true);
    ~AGIDDSCdrDeserializer() = default;

    bool deserializeLongDouble(LongDouble& value) 
    {
        static char s_alignedData[8] = { 0 };
        uint32_t ldSize = sizeof(long double);
        // 先读取补齐的数据
        bool ret = deserializeBaseTypeArray<char>(s_alignedData, 16 - ldSize);
        if (!ret) {
            return false;
        }
        return deserializeBaseType(value.ld);
    }
    
    bool deserializeLongDoubleArray(LongDouble* value, uint32_t length) 
    {
        for (uint32_t i = 0; i < length; ++i) {
            if (!deserializeLongDouble(value[i])) {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    bool deserializeBaseType(T& value) 
    {
        uint32_t typeSize = sizeof(T);
        // 对齐数据
        uint32_t offset = alignSize(typeSize);
        char s_alignedData[15] = { 0 };
        SerializedBuffer* retBuffer = curProcessMsgBlock_->readData(nullptr, offset);
        if (!retBuffer) {
            return false;
        }
        curProcessMsgBlock_ = retBuffer;
        index_ += offset;
        totalRemains_ -= offset;
        // 读取真正的负载
        retBuffer = curProcessMsgBlock_->readData((char*)&value, typeSize);
        if (!retBuffer) {
            return false;
        }
        curProcessMsgBlock_ = retBuffer;
        totalRemains_ -= typeSize;
        index_ += typeSize;
        // 端序转换
        if (useLittleEndian_ != isLittleEndian()) {
            switch (typeSize) {
            case 2:
                value = __builtin_bswap16(*(uint16_t*)&value);
                break;
            case 4:
                value = __builtin_bswap32(*(uint32_t*)&value);
                break;
            case 8:
                value = __builtin_bswap64(*(uint64_t*)&value);
                break;
            default:
                return false;
            }
        }
        return true;
    }

    template<typename T>
    bool deserializeBaseTypeArray(T* value, uint32_t length) 
    {
        if (length == 0) {
            return true;
        }
        uint32_t typeSize = sizeof(T);
        if (typeSize == 1) {
            // 字节数组时，无需考虑对齐，以及端序
            SerializedBuffer* retBuffer = curProcessMsgBlock_->readData((char*)value, length);
            if (!retBuffer) {
                return false;
            }
            curProcessMsgBlock_ = retBuffer;
            index_ += length;
            totalRemains_ -= length;
            return true;
        }
        // 其余数组需要第一个元素需要考虑对齐以及端序
        // 第一个数据考虑对齐
        if (!deserializeBaseType<T>(value[0])) {
            return false;
        }
        // 如果只有一个元素，则处理完成
        if (length <= 1) {
            return true;
        }
        // 剩下的数据
        if (useLittleEndian_ && isLittleEndian()) {
            // 本机端序与要求编码端序相同，直接拷贝进入缓冲区
            SerializedBuffer* retBuffer = curProcessMsgBlock_->readData((char*)&value[1], typeSize * (length - 1));
            if (!retBuffer) {
                return false;
            }
            curProcessMsgBlock_ = retBuffer;
            index_ += typeSize * (length - 1);
            totalRemains_ -= typeSize * (length - 1);
        }
        else {
            for (uint32_t i = 1; i < length; ++i) {
                if (!deserializeBaseType<T>(value[i])) {
                    return false;
                }
            }
        }
        return true;
    }

    bool deserializeString(char* value, uint32_t length);

    bool deserializeString(std::string& value);

    bool deserializeWString(std::wstring& value);

    bool isLittleEndian() const;

    uint32_t getBufferLength() const;

    /**
     * @brief 反序列化参数头，用于扩展CDR模式。
     *
     * @param   [in,out] memberId       参数对应的成员的memberId.
     * @param   [in,out] length         参数长度。
     * @param   [in,out] useExtended     是否使用PID_EXTENDED.
     * @param   [in,out] mustUnderstand  是否必须理解该参数。
     *
     * @return  NULL表示失败，否则表示回填的长度的地址。
     */
    bool deserializeParameterHeader(uint32_t& memberId, uint32_t& length, bool& useExtended, bool& mustUnderstand); 

    /**
     * @brief   手动调整CDR序列化器的读指针。
     *
     * @param   beginIndex      已读取的长度。
     * @param   length          调整量。
     * 
     * @return  是否成功。
     */
    bool aheadReaderPosition(uint32_t beginIndex, uint32_t length);

    uint32_t alignSize(uint32_t typeSize) 
    {
        return ((~index_ + 1) & (typeSize - 1));
    }
private:
    SerializedBuffer* tgaMsgBlock_;
    SerializedBuffer* curProcessMsgBlock_;
    uint32_t totalRemains_;
    uint32_t index_;
    bool useLittleEndian_;
};

#endif // AGICDR_H