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
#ifndef TRAVODDS_RTPS_MESSAGES_MESSAGEPROCESSOR_H
#define TRAVODDS_RTPS_MESSAGES_MESSAGEPROCESSOR_H

#include "dcps/topic/typesupport.h"
#include "rtps/basictypertps.h"
#include "rtps/messages/cdrmessage_t.h"
#include "rtps/historycache/historycache.h"

TRAVODDS_NAMESPACE_BEGIN

//声明
class Participant;
class MessageTransmitter;
//class ReceiveBuffer;
class Endpoint;
#ifdef DDSSECURITY
class DomainParticipantImpl;
#endif // DDSSECURITY
/**
 * @brief MessageProcessor类，RTPS报文封装解析类。
 */
class MessageProcessor
{
public:

	/**
	 * @brief 构造函数。
	 *
	 * @param [in] participant Participant指针。
	 */
    MessageProcessor(Participant* participant);

	/**
	 * @brief 析构函数。
	 */
    virtual ~MessageProcessor();

	/**
	 * @brief 封装DATA报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateDATAMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		DataSubMessage_t& dataSubMessage);

	/**
	 * @brief 封装DATA 分片报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateDataFragMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		DataFragSubMessage_t & dataFragSubMessage);

	/**
	 * @brief 封装HeartBeat子报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateHeartBeatMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		HeartBeatSubMessage_t & heartBeatSubMessage);

	/**
	 * @brief 封装AckNack子报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateAckNackMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		AcknackSubMessage_t & ackMessage);

	/**
	 * @brief 封装HeartBeatFrag子报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateHeartBeatFragMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		HeartBeatFragSubMessage_t & heartBeatSubMessage);

	/**
	 * @brief 封装NackFrag子报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateNackFragMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		NackFragSubMessage_t & nackFlagMessage);

	/**
	 * @brief 封装InfoSRC子报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateINFOSRCMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		InfoSource_t infoSRC);

	/**
	 * @brief 封装Gap子报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateGapMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		GapSubMessage_t& gapSubMessage);

	/**
	 * @brief 封装Pad子报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulatePadMessage(SerializedBuffer& messageBuf,
		uint32_t& shift);

	/**
	 * @brief 封装InfoTS子报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateINFOTSMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		InfoTimestamp_t infoTS);

	/**
	 * @brief 封装InfoTS子报文，获取当前时间为时间戳。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateINFOTSMessageNow(SerializedBuffer& messageBuf,
		uint32_t& shift,
		InfoTimestamp_t infoTS);

	/**
	 * @brief 封装InfoDST子报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateINFODSTMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		InfoDestinationSubMessage_t& infoDST);

	/**
	 * @brief 封装HeartBeat报文。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] dataSubMessage 报文前缀信息。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateINFOREPLYMessage(SerializedBuffer& messageBuf,
		uint32_t & shift,
		InfoReplySubMessage_t& infoReplySubMessage);

	/**
	 * @brief 封装RTPS报文头。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] guidPrefix 发送报文的参与者guidPrefix。
	 * @param [in] version RTPS协议的版本。
	 * @param [in] vendorId 提供RTPS协议实现的供应商ID。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateMessageHeader(SerializedBuffer& messageBuf,
		uint32_t& shift,
		const GuidPrefix_t& guidPrefix,
		const ProtocolVersion_t& version,
		const VendorId_t& vendorId);

	/**
	 * @brief 封装RTPS报文头。
	 *
	 * @param [out] messageBuf 报文封装结构。
	 * @param [in,out] shift 报文封装偏移量。
	 * @param [in] guidPrefix 发送报文的参与者guidPrefix。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	static ReturnCode_t EncapsulateMessageHeader(SerializedBuffer& messageBuf,
		uint32_t& shift,
		const GuidPrefix_t& guidPrefix);


	/**
	 * @brief RTPS子报文分类解析。
	 *
	 * @param [in] message 报文内容。
	 * @param [in] messageLen 报文长度。
	 *
	 * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	 ReturnCode_t InterpretSubmessage(char*& message,
		uint32_t messageLen);

	 /**
	  * @brief 解析RTPS报文头。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretMessageHeader(char*& message,
		Receive_t& messageReceiver);

	 /**
	  * @brief 解析RTPS Data子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretDataMessage(char* message,
		SubMessageHeader_t & submessageHeader,
		Receive_t & messageReceiver);

	 /**
	  * @brief 解析RTPS Heartbeat子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretHeartbeatMessage(char* message,
		SubMessageHeader_t& submessageHeader,
		Receive_t& messageReceiver);

	 /**
	  * @brief 解析RTPS HeartbeatFrag子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretHeartbeatFragMessage(char* message,
		SubMessageHeader_t& submessageHeader, 
		Receive_t& messageReceiver);

	 /**
	  * @brief 解析RTPS AckNack子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretAckNackMessage(char* message, 
		SubMessageHeader_t& submessageHeader,
		Receive_t& messageReceiver);

	 /**
	  * @brief 解析RTPS DataFrag子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretDataFragMessage(char * message,
		SubMessageHeader_t & submessageHeader,
		Receive_t & messageReceiver);

	 /**
	  * @brief 解析RTPS INFO_TS子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretINFOTSMessage(char* message,
		SubMessageHeader_t& submessageHeader,
		Receive_t& messageReceiver);

	 /**
	  * @brief 解析RTPS INFO_SRC子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretINFOSRCMessage(char * message,
		SubMessageHeader_t & submessageHeader,
		Receive_t & vmessageReceiver);

	 /**
	  * @brief 解析RTPS INFO_DST子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretINFODSTMessage(char* message,
		SubMessageHeader_t& submessageHeader,
		Receive_t& messageReceiver);

	 /**
	  * @brief 解析RTPS InlineQos子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in,out] messageShift 报文指针偏移量。
	  * @param [out] inlineQos inlineQos解决结果。
	  * @param [in] endiannessFlag 报文大小端标志。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretInlineQosMessage(char* &message,
		uint32_t& messageShift,
		ParameterList_t & inlineQos,
		bool endiannessFlag);

	 /**
	  * @brief 解析RTPS NackFrag子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretNackFragMessage(char* message, 
		SubMessageHeader_t& submessageHeader, 
		Receive_t& messageReceiver);

	 /**
	  * @brief 解析RTPS InfoReply报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretINFOREPLYMessage(char* message, 
		 SubMessageHeader_t & submessageHeader, 
		 Receive_t & messageReceiver);

	 /**
	  * @brief 解析RTPS GAP子报文。
	  *
	  * @param [in] message 报文内容。
	  * @param [in] messageReceiver 报文解析参数存储。
	  *
	  * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	  */
	 ReturnCode_t InterpretGapMessage(char* message, 
		SubMessageHeader_t& submessageHeader, 
		Receive_t& messageReceiver);

private:
	Participant* participant_;
#ifdef DDSSECURITY
public:
	/* @brief 所属域参与者。*/
	DomainParticipantImpl* participantImpl_;
#endif // DDSSECURITY
};
TRAVODDS_NAMESPACE_END
#endif
