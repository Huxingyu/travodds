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
#ifndef TRAVODDS_NUMERIC_CONSTANTS_H
#define TRAVODDS_NUMERIC_CONSTANTS_H

#include <cstdint>  
#include <cstddef>
#include "travodds_dll.h"

TRAVODDS_NAMESPACE_BEGIN

namespace SubMessageFlag {
	constexpr uint8_t NONE_FLAG                      = 0x00; // 无标志位
	constexpr uint8_t ENDIANNESS_FLAG                = 0x01; // EndiannessFlag little-endian
	// HeaderExtension
	constexpr uint8_t LENGTH_FLAG                    = 0x02; // LengthFlag
	constexpr uint8_t TIMESTAMP_FLAG                 = 0x04; // TimestampFlag
	constexpr uint8_t UEXTENSION_FLAG                = 0x08; // UExtensionFlag
	constexpr uint8_t WEXTENSION_FLAG                = 0x10; // WExtensionFlag
	constexpr uint8_t CHECKSUM_32_FLAG               = 0x20; // ChecksumFlag CRC-32C
	constexpr uint8_t CHECKSUM_64_FLAG               = 0x40; // ChecksumFlag CRC-64/XZ
	constexpr uint8_t CHECKSUM_128_FLAG              = 0x60; // ChecksumFlag MD5
	constexpr uint8_t PARAMETERS_FALG                = 0x80; // ParametersFlag
	// AckNack
	constexpr uint8_t FINAL_FLAG                     = 0x02; // FinalFlag
	// Data
	constexpr uint8_t INLINE_QOS_FLAG                = 0x02; // InlineQoSFlag
	constexpr uint8_t DATA_FLAG                      = 0x04; // DataFlag
	constexpr uint8_t KEY_FLAG                       = 0x08; // KeyFlag
	constexpr uint8_t NON_STANDARD_PAYLOAD_FLAG      = 0x10; // NonStandardPayloadFlag
	// DataFrag
	// ... 0x02 INLINE_QOS_FLAG
	constexpr uint8_t FRAG_KEY_FLAG                  = 0x04; // KeyFlag
	constexpr uint8_t FRAG_NON_STANDARD_PAYLOAD_FLAG = 0x08; // NonStandardPayloadFlag
	// Gap
	constexpr uint8_t GROUPINFO_FLAG                 = 0x02; // GroupInfoFlag
	constexpr uint8_t FILTERED_COUNT_FLAG            = 0x04; // FilteredCountFlag
	// Heartbeat
	// ... 0x02 FINAL_FLAG
	constexpr uint8_t LIVELINESS_FLAG                = 0x04; // LivelinessFlag
	constexpr uint8_t HB_GROUPINFO_FLAG              = 0x08; // GroupInfoFlag
	// InfoReply
	constexpr uint8_t MULTICAST_FLAG                 = 0x02; // MulticastFlag
	//InfoTimestamp
	constexpr uint8_t INVALIDATE_FLAG                = 0x02; // InvalidateFlag
}

namespace StatusInfoFlag {
	constexpr uint8_t NONE_FLAG         = 0x00; // 无标志位
	constexpr uint8_t DISPOSED_FLAG     = 0x01; // DisposedFlag
	constexpr uint8_t UNREGISTERED_FLAG = 0x02; // UnregisteredFlag
	constexpr uint8_t FILTERED_FLAG     = 0x04; // FilteredFlag
}

enum EndiannessKind : uint8_t {
	CDR_BE          = 0x00,
	CDR_LE          = 0x01,
	/* 大端
	 0.............. 8............... 16............ 24.............. 32
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ +
	| 0x00            0x02           | 0x00	          0x00           |   = {PL_CDR_BE, 0}
	+-------------- + -------------- + ------------ + --------------- +
	| 0x00	     	  0x00	  	       0x00	     	  0x01           |   = uint32_t (1)
	+-------------- + -------------- + ------------ + --------------- +
	| 'a'	     	  'b'	  	       'c'	     	  'd'            |   = char[4] = {'a', 'b', 'c', 'd'}
	+-------------- + -------------- + ------------ + --------------- +
	*/
	PL_CDR_BE       = 0x02,
	/* 小端
	 0.............. 8............... 16............ 24.............. 32
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ +
	| 0x00            0x03           | 0x00	          0x00           |  = {PL_CDR_LE, 0}
	+-------------- + -------------- + ------------ + --------------- +
	| 0x01	     	  0x00	  	       0x00	     	  0x00           |  = uint32_t (1)
	+-------------- + -------------- + ------------ + --------------- +
	| 'a'	     	  'b'	  	       'c'	     	  'd'            |  = char[4] = {'a', 'b', 'c', 'd'}
	+-------------- + -------------- + ------------ + --------------- +
	*/
	PL_CDR_LE       = 0x03,
	CDR2_BE         = 0x10,
	CDR2_LE         = 0x11,
	PL_CDR2_BE      = 0x12,
	PL_CDR2_LE      = 0x13,
	D_CDR_BE        = 0x14,
	D_CDR_LE        = 0x15,
	XML             = 0x04
};

/* 系统存储 */
#if defined(_WIN32) || defined(_MSC_VER) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
constexpr bool IS_LITTLE_ENDIAN = true;
constexpr EndiannessKind PL_DEFAULT_ENDIAN = EndiannessKind::PL_CDR_LE;
constexpr EndiannessKind DEFAULT_ENDIAN = EndiannessKind::CDR_LE;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
constexpr bool IS_LITTLE_ENDIAN = false;
constexpr EndiannessKind PL_DEFAULT_ENDIAN = EndiannessKind::PL_CDR_BE;
constexpr EndiannessKind DEFAULT_ENDIAN = EndiannessKind::CDR_BE;
#else
// 其他平台：运行时检测
inline bool IsLittleEndian() {
	const int32_t num = 1;
	const uint8_t* byte = reinterpret_cast<const uint8_t*>(&num);
	return byte[0] == 1;
}
#define IS_LITTLE_ENDIAN IsLittleEndian()
#define PL_DEFAULT_ENDIAN IsLittleEndian() ? EndiannessKind::PL_CDR_LE : EndiannessKind::PL_CDR_BE
#define DEFAULT_ENDIAN IsLittleEndian() ? EndiannessKind::CDR_LE : EndiannessKind::CDR_BE
#endif

constexpr uint32_t CDR_ENDIANNESS_LEN = 4; // 大小端信息的空间 B(字节)
//子报文固定长度
constexpr uint16_t SUB_MESSAGE_LEN_PAD = 0;
constexpr uint16_t SUB_MESSAGE_LEN_GAP = 10;
constexpr uint16_t SUB_MESSAGE_LEN_INFO_TS = 8;
constexpr uint16_t SUB_MESSAGE_LEN_INFO_DST = 12;
constexpr uint16_t SUB_MESSAGE_LEN_INFO_SRC = 20;

constexpr int32_t RECEIVE_BUFFER_SIZE = 10 * 1024 * 1024;

#ifdef _DEBUG
constexpr int32_t DATAFRAG_DELAY_NUM = 20;
#else
constexpr int32_t DATAFRAG_DELAY_NUM = 100;
#endif

TRAVODDS_NAMESPACE_END
#endif
