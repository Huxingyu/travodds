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
#ifndef AGIDDS_RTPS_MESSAGES_MESSAGEPROCESSOR_H
#define AGIDDS_RTPS_MESSAGES_MESSAGEPROCESSOR_H

#include "dcps/topic/typesupport.h"
#include "rtps/basictypertps.h"
#include "rtps/messages/cdrmessage_t.h"
#include "rtps/historycache/historycache.h"

AGIDDS_NAMESPACE_BEGIN

//声明
class Participant;
class MessageTransmitter;
//class ReceiveBuffer;
class Endpoint;

class MessageProcessor
{
public:
    //构造函数
    MessageProcessor(Participant* participant);

    //析构函数
    virtual ~MessageProcessor();

public:

	/* 从用户数据往前封装Data子报文头 */
	static ReturnCode_t EncapsulateDATAMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		DataSubMessage_t& dataSubMessage);

	static ReturnCode_t EncapsulateDataFragMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		DataFragSubMessage_t & dataFragSubMessage);

	static ReturnCode_t EncapsulateHeartBeatMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		HeartBeatSubMessage_t & heartBeatSubMessage);

	static ReturnCode_t EncapsulateAckNackMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		AcknackSubMessage_t & ackMessage);

	static ReturnCode_t EncapsulateHeartBeatFragMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		HeartBeatFragSubMessage_t & heartBeatSubMessage);

	static ReturnCode_t EncapsulateNackFragMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		NackFragSubMessage_t & nackFlagMessage);

	static ReturnCode_t EncapsulateINFOSRCMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		InfoSource_t infoSRC);

	static ReturnCode_t EncapsulateGapMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		GapSubMessage_t& gapSubMessage);

	static ReturnCode_t EncapsulatePadMessage(SerializedBuffer& messageBuf,
		uint32_t& shift);

	static ReturnCode_t EncapsulateINFOTSMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		InfoTimestamp_t infoTS);

	static ReturnCode_t EncapsulateINFOTSMessageNow(SerializedBuffer& messageBuf,
		uint32_t& shift,
		InfoTimestamp_t infoTS);

	static ReturnCode_t EncapsulateINFODSTMessage(SerializedBuffer& messageBuf,
		uint32_t& shift,
		InfoDestinationSubMessage_t& infoDST);

	static ReturnCode_t EncapsulateINFOREPLYMessage(SerializedBuffer& messageBuf,
		uint32_t & shift,
		InfoReplySubMessage_t& infoReplySubMessage);

	static ReturnCode_t EncapsulateMessageHeader(SerializedBuffer& messageBuf,
		uint32_t& shift,
		const GuidPrefix_t& guidPrefix,
		const ProtocolVersion_t& version,
		const VendorId_t& vendorId);

	static ReturnCode_t EncapsulateMessageHeader(SerializedBuffer& messageBuf,
		uint32_t& shift,
		const GuidPrefix_t& guidPrefix);


	//解析子报文
	 ReturnCode_t InterpretSubmessage(char*& message,
		uint32_t messageLen);

    //解析报文头
	 ReturnCode_t InterpretMessageHeader(char*& message,
		Receive_t& messageReceiver);

    //解析Data子报文
	 ReturnCode_t InterpretDataMessage(char* message,
		SubMessageHeader_t & submessageHeader,
		Receive_t & messageReceiver);

	//解析Heartbeat子报文
	 ReturnCode_t InterpretHeartbeatMessage(char* message,
		SubMessageHeader_t& submessageHeader,
		Receive_t& messageReceiver);

	//解析HeartbeatFrag子报文
	 ReturnCode_t InterpretHeartbeatFragMessage(char* message,
		SubMessageHeader_t& submessageHeader, 
		Receive_t& messageReceiver);

    //解析AckNack子报文
	 ReturnCode_t InterpretAckNackMessage(char* message, 
		SubMessageHeader_t& submessageHeader,
		Receive_t& messageReceiver);

	//解析DataFrag子报文
	 ReturnCode_t InterpretDataFragMessage(char * message,
		SubMessageHeader_t & submessageHeader,
		Receive_t & messageReceiver);

	//解析INFO_TS子报文
	 ReturnCode_t InterpretINFOTSMessage(char* message,
		SubMessageHeader_t& submessageHeader,
		Receive_t& messageReceiver);

	//解析INFO_SRC子报文
	 ReturnCode_t InterpretINFOSRCMessage(char * message,
		SubMessageHeader_t & submessageHeader,
		Receive_t & vmessageReceiver);

	//解析INFO_DST子报文
	 ReturnCode_t InterpretINFODSTMessage(char* message,
		SubMessageHeader_t& submessageHeader,
		Receive_t& messageReceiver);

	/* 解析InlineQos 此处报文指针为传引用 */
	 ReturnCode_t InterpretInlineQosMessage(char* &message,
		uint32_t& messageShift,
		ParameterList_t & inlineQos,
		bool endiannessFlag);

	//解析NackFragmessagev
	 ReturnCode_t InterpretNackFragMessage(char* message, 
		SubMessageHeader_t& submessageHeader, 
		Receive_t& messageReceiver);

	 ReturnCode_t InterpretINFOREPLYMessage(char* message, 
		 SubMessageHeader_t & submessageHeader, 
		 Receive_t & messageReceiver);

	//解析GAP子报文
	 ReturnCode_t InterpretGapMessage(char* message, 
		SubMessageHeader_t& submessageHeader, 
		Receive_t& messageReceiver);

private:
	Participant* participant_;

};
AGIDDS_NAMESPACE_END
#endif
