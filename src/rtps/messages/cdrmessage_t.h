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
#ifndef AGIDDS_RTPS_MESSAGES_CDRMESSAGE_T_H
#define AGIDDS_RTPS_MESSAGES_CDRMESSAGE_T_H
#include "rtps/basictypertps.h"
#include "rtps/messages/parameter.h"
#include "dcps/topic/typesupport.h"

#include <cstdlib>
#include <bitset>

AGIDDS_NAMESPACE_BEGIN

#define RTPSMESSAGE_DEFAULT_SIZE 10500

// using std::bitset;

// const uint32_t DATAMESSAGE_LIMIT = 63 * 1024;

/**
* @ brief 定义子报文元素 SubmessageElement
*/

struct Receive_t {
	ProtocolVersion_t sourceVersion;
	VendorId_t sourceVendorId;
	GuidPrefix_t sourceGuidPrefix;
	GuidPrefix_t destGuidPrefix;
	LocatorVec unicastReplyLocatorList;
	LocatorVec multicastReplyLocatorList;
	bool haveTimestame;
	Time_t timestamp;
	uint32_t messageLen;
};

struct Bitmap_t {
	uint32_t bits[8];
	uint32_t size;  // 位数
	Bitmap_t() : bits{0}, size(0){}
	// 获取占用的字节数
	uint32_t GetByteSize() const {
		return (((size - 1u) >> 5u) + 1u) * 4u;
	}
	// 设置某一位为1
	void SetBit(const uint32_t& s) {
		bits[s / 32] |= (0x80000000 >> (s % 32));
	}
	// 检查某一位是否为1
	bool IsBitSet(const uint32_t& s) const {
		return (bits[s / 32] & 0x80000000 >> (s % 32)) != 0;
	}
};

struct SequenceNumberSet_t {
	SequenceNumber_t bitmapBase;
	// bitmap.size表示从BitmapBase-1起的有效位
	Bitmap_t bitmap;

	SequenceNumberSet_t() 
		: bitmapBase(SEQUENCENUMBER_ZERO) {
	}
};

struct FragMentNumberSet_t {
	FragmentNumber_t bitmapBase;
	Bitmap_t bitmap;

	FragMentNumberSet_t()
		: bitmapBase(0) {
	}
};

typedef unsigned char* SerializedPayload;

/**
* @ brief 定义报文类型
*/
typedef octet SubMessageKind_t;
const SubMessageKind_t PAD = 0x01;
const SubMessageKind_t ACKNACK = 0x06;
const SubMessageKind_t HEARTBEAT = 0x07;
const SubMessageKind_t GAP = 0x08;
const SubMessageKind_t INFO_TS = 0x09;
const SubMessageKind_t INFO_SRC = 0x0c;
const SubMessageKind_t INFO_REPLY_IP4 = 0x0d;
const SubMessageKind_t INFO_DST = 0x0e;
const SubMessageKind_t INFO_REPLY = 0x0f;
const SubMessageKind_t NACK_FRAG = 0x12;
const SubMessageKind_t HEARTBEAT_FRAG = 0x13;
const SubMessageKind_t DATA = 0x15;
const SubMessageKind_t DATA_FRAG = 0x16;

#define FLAG_BIT_0 1
#define FLAG_BIT_1 2
#define FLAG_BIT_2 4
#define FLAG_BIT_3 8
#define FLAG_BIT_4 16
#define FLAG_BIT_5 32
#define FLAG_BIT_6 64
#define FLAG_BIT_7 128

/**
* @ brief 定义报文头_MESSAGE_HEADER
*/
struct MessageHeader_t {
	ProtocolId_t protocol;
	ProtocolVersion_t version;
	VendorId_t vendorId;
	GuidPrefix_t guidPrefix;
};

/**
* @ brief 定义子报文头 SubMessageFlag_t
*/
using SubMessageFlag_t = octet;
struct SubMessageHeader_t {
	SubMessageKind_t subMessageId;
	//Flag 第一位始终为 EndiannessFlag 其他各位根据报文类型而定
	SubMessageFlag_t flags;
	uint16_t subMessageLength;

	SubMessageHeader_t()
	{
		subMessageId = 0;
		flags = SubMessageFlag::NONE_FLAG;
		subMessageLength = 0;
	}
};

/**
* @ brief 定义AckNack子报文
* @       Header中Flag第二位为 FinalFlag
*/
#pragma pack(4)
struct AcknackSubMessage_t {
	SubMessageHeader_t header;
	EntityId_t readerId;
	EntityId_t writerId;
	SequenceNumberSet_t readerSNState;
	Count_t count;
};
#pragma pack()


/* MsgFlag_t */
struct MsgFlag_t {
	SubMessageFlag_t endiannessFlag;
};

struct DataMsgFlag_t : MsgFlag_t {
	// |X|X|X|N|K|D|Q|E|
	SubMessageFlag_t inlineQosFlag;
	SubMessageFlag_t dataFlag;
	SubMessageFlag_t keyFlag;
	SubMessageFlag_t nonStandardPayloadFlag;
};

struct DataFragMsgFlag_t : MsgFlag_t {
	// |X|X|X|X|N|K|Q|E|
	SubMessageFlag_t inlineQosFlag;
	SubMessageFlag_t keyFlag;
	SubMessageFlag_t nonStandardPayloadFlag;
};

struct HeartbeatMsgFlag_t : MsgFlag_t {
	// |X|X|X|X|G|L|F|E| 
	SubMessageFlag_t finalFlag;
	SubMessageFlag_t livelinessFlag;
	SubMessageFlag_t groupInfoFlag;
};
struct GapMsgFlag_t : MsgFlag_t {
	// ||X|X|X|X|X|F|G|E|
	SubMessageFlag_t groupInfoFlag;
	SubMessageFlag_t filteredCountFlag;
};
struct AcknackMsgFlag_t : MsgFlag_t {
	SubMessageFlag_t finalFlag;
};
struct InfoReolyMsgFlag_t : MsgFlag_t {
	SubMessageFlag_t multicastFlag;
};
struct InfoTimestampMsgFlag_t : MsgFlag_t {
	SubMessageFlag_t invalidateFlag;
};

using HeartbeatMsgFragFlag_t = MsgFlag_t;
using InfoDestinationtMsgFragFlag_t = MsgFlag_t;
using InfoSourceMsgFlag_t = MsgFlag_t;
using NackMsgFlag_t = MsgFlag_t;
/**
* @ brief 定义Data子报文
* @       Header中Flag第二位为 InlineQosFlag //暂不实现
* @       第三位为 DataFlag
* @       第四位为 KeyFlag
*/
struct DataSubMessage_t {
	SubMessageHeader_t header;
	SubMessageFlag_t extraFlags[2];
	uint16_t octetsToInlineQos;
	EntityId_t readerId;
	EntityId_t writerId;
	SequenceNumber_t writerSN;
	ParameterList_t inlineQos; 
	SerializedPayload serializedPayload;
};

/**
* @ brief 定义Heartbeat子报文
* @       Header中Flag第二位为 FinalFlag
* @       第三位为 LivenessFlag
*/
struct HeartBeatSubMessage_t {
	SubMessageHeader_t header;
	EntityId_t readerId;
	EntityId_t writerId;
	SequenceNumber_t firstSN;
	SequenceNumber_t lastSN;
	Count_t count;
	SequenceNumber_t currentGSN;
	SequenceNumber_t firstGSN;
	SequenceNumber_t lastGSN;
	GroupDigest_t writerSet;
	GroupDigest_t secureWriterSet;

};

/**
* @ brief 定义HeartbeatFrag子报文
* @       Header中Flag第二位为 FinalFlag
* @       第三位为 LivenessFlag
*/
struct HeartBeatFragSubMessage_t {
	SubMessageHeader_t header;
	EntityId_t readerId;
	EntityId_t writerId;
	SequenceNumber_t writerSN;
	FragmentNumber_t lastFragmentNum;
	Count_t count;
};

/**
* @ brief 定义InfoDestination子报文
*/
struct InfoDestinationSubMessage_t {
	SubMessageHeader_t header;
	GuidPrefix_t guidPrefix;
};

/**
* @ brief 定义InfoReply子报文
* @       Header中Flag第二位为 MulticastFlag
*/
struct InfoReplySubMessage_t
{
	SubMessageHeader_t header;
	LocatorSeq unicastLocatorList;
	LocatorSeq multicastLocatorList;
};

/**
* @ brief 定义InfoReplyIp4子报文
* @       Header中Flag第二位为 MulticastFlag
*/

//struct InfoReplySubmessage_t {
//	SubMessageHeader_t header;
//	LocatorUDPv4_t unicastLocatorList;
//	LocatorUDPv4_t multicastLocatorList;
//};


/**
* @ brief 定义InfoSource子报文
*/
struct InfoSource_t {
	SubMessageHeader_t header;
	long unused;
	ProtocolVersion_t version;
	VendorId_t vendorId;
	GuidPrefix_t guidPrefix;
};

/**
* @ brief 定义InfoTimestamp子报文
* @       Header中Flag第二位为 InvalidateFlag
*/
struct InfoTimestamp_t {
	SubMessageHeader_t header;
	Time_t timestamp;
};

struct DataFrag_t {
	uint32_t fragmentStartingNum; //分片中起始frag的分片号
	uint16_t fragmentsInSubmessage; //分片中包含的frag个数
	uint16_t fragmentSize; //每个分片的大小
	uint32_t sampleSize; //数据原始大小
};
/**
* @ brief 定义DataFrag子报文
* @       Header中Flag第二位为 InlineQosFlag //暂不实现
* @       第三位为 DataFlag
* @       第四位为 KeyFlag
*/
struct DataFragSubMessage_t {
	SubMessageHeader_t header;
	SubMessageFlag_t extraFlags[2];
	uint16_t octetsToInlineQos;
	EntityId_t readerId;
	EntityId_t writerId;
	SequenceNumber_t writerSN;

	//TODO
	ParameterList_t inlineQos;
	DataFrag_t dataFrag;
	SerializedPayload serializedPayload;
};

/**
* @ brief 定义NackFrag子报文
* @       Header中Flag第一位为 EndiannessFlag
* @       Header中Flag第二位为 InvalidateFlag
*/
struct NackFragSubMessage_t {
	SubMessageHeader_t header;
	EntityId_t readerId;
	EntityId_t writerId;
	SequenceNumber_t writerSN;
	FragMentNumberSet_t fragmentNumberState;
	Count_t count;

	NackFragSubMessage_t()
		: writerSN(SEQUENCENUMBER_ZERO), count(0) {
	}
};

/**
* @ brief 定义GAP子报文
* @       Header中Flag第一位为 EndiannessFlag
* @       Header中Flag第二位为 InvalidateFlag
*/
struct GapSubMessage_t {
	SubMessageHeader_t header;
	EntityId_t readerId;
	EntityId_t writerId;
	SequenceNumber_t gapStart;
	SequenceNumberSet_t gapList;
	SequenceNumber_t gapStartGSN;
	SequenceNumber_t gapEndGSN;
	ChangeCount_t filteredCount;
};


AGIDDS_NAMESPACE_END

#endif
