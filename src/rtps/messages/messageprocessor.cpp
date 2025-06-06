#include "rtps/messages/messageprocessor.h"
#include "rtps/endpoint/endpoint.h"
#include "rtps/endpoint/reader.h"
#include "rtps/endpoint/writer.h"
#include "rtps/endpoint/statefulwriter.h"
#include "rtps/endpoint/statefulreader.h"
#include "rtps/endpoint/writerproxy.h"
#include "rtps/endpoint/readerproxy.h"
#include "rtps/endpoint/participant.h"
#include "rtps/messages/parameter.h"
#include <iostream>
#include <asio.hpp>
USING_AGIDDS_NAMESPACE;

/************************************************************
** MessageProcessor类构造函数
*************************************************************/
MessageProcessor::MessageProcessor(Participant* participant)
	: participant_(participant)
{
}

MessageProcessor::~MessageProcessor()
{
}

ReturnCode_t MessageProcessor::EncapsulateDATAMessage(SerializedBuffer& messageBuf,
											uint32_t& shift,
											DataSubMessage_t& dataSubMessage)
{
	uint32_t bShift = shift;
	bool endiannessFlag = (dataSubMessage.header.flags & BIT(0)) != 0;
	bool inlineQosFlag = (dataSubMessage.header.flags & BIT(1)) != 0;
	bool dataFlag = (dataSubMessage.header.flags & BIT(2)) != 0;
	bool keyFlag = (dataSubMessage.header.flags & BIT(3)) != 0;
	bool nonStandardPayloadFlag = (dataSubMessage.header.flags & BIT(4)) != 0;

	dataSubMessage.header.subMessageId = DATA;

	if (inlineQosFlag) {
		//TODO  在封装的时候将PID_SENTINEL 插入inlineQos的链表中，还是在这边封装一个PID_SENTINEL
		//jdxcz 20250418 选择在封装inlineQOS封装后，封装PID_SENTINEL

		Parameter_t param;
		param.parameterId = PID_SENTINEL;
		param.length = 0;

		shift += sizeof(int16_t);
		memcpy(messageBuf.buffer - shift, (char*)&param.length, sizeof(int16_t));

		shift += sizeof(ParameterId_t);
		memcpy(messageBuf.buffer - shift, (char*)&param.parameterId, sizeof(ParameterId_t));

		/* 封装InlineQoS内容 */
		for (ParameterList_t::iterator itParameter = dataSubMessage.inlineQos.begin();
			itParameter != dataSubMessage.inlineQos.end(); itParameter++) {
			shift += itParameter->length;
			memcpy(messageBuf.buffer - shift, itParameter->value.data(), itParameter->length);
			
			shift += sizeof(int16_t);
			memcpy(messageBuf.buffer - shift, (char*)&itParameter->length, sizeof(int16_t));
			
			shift += sizeof(ParameterId_t);
			memcpy(messageBuf.buffer - shift, (char*)&itParameter->parameterId, sizeof(ParameterId_t));
			
		}
	}
	shift += sizeof(SequenceNumber_t);
	memcpy(messageBuf.buffer - shift, &dataSubMessage.writerSN, sizeof(SequenceNumber_t));
	
	shift += sizeof(EntityId_t);
	memcpy(messageBuf.buffer - shift, &dataSubMessage.writerId, sizeof(EntityId_t));
	
	shift += sizeof(EntityId_t);
	memcpy(messageBuf.buffer - shift, &dataSubMessage.readerId, sizeof(EntityId_t));
	
	shift += sizeof(uint16_t);
	memcpy(messageBuf.buffer - shift, &dataSubMessage.octetsToInlineQos, sizeof(uint16_t));
	
	shift += sizeof(SubMessageFlag_t) * 2;
	memcpy(messageBuf.buffer - shift, dataSubMessage.extraFlags, sizeof(SubMessageFlag_t) * 2);
	
	/* 设置报文实际长度 */
	dataSubMessage.header.subMessageLength = shift + messageBuf.buffer_size;
	shift += sizeof(SubMessageHeader_t);
	memcpy(messageBuf.buffer - shift, &dataSubMessage.header, sizeof(SubMessageHeader_t));

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateDataFragMessage(SerializedBuffer& messageBuf,
												uint32_t& shift,
												DataFragSubMessage_t& dataFragSubMessage)
{
	uint32_t bShift = shift;
	bool endiannessFlag = (dataFragSubMessage.header.flags & BIT(0)) != 0;
	bool inlineQosFlag = (dataFragSubMessage.header.flags & BIT(1)) != 0;
	bool nonStandardPayloadFlag = (dataFragSubMessage.header.flags & BIT(2)) != 0;
	bool keyFlag = (dataFragSubMessage.header.flags & BIT(3)) != 0;

	dataFragSubMessage.header.subMessageId = DATA_FRAG;

	if (inlineQosFlag) {
		//TODO  在封装的时候将PID_SENTINEL 插入inlineQos的链表中，还是在这边封装一个PID_SENTINEL
		//jdxcz 20250418 选择在封装inlineQOS封装后，封装PID_SENTINEL

		Parameter_t param;
		param.parameterId = PID_SENTINEL;
		param.length = 0;

		shift += sizeof(int16_t);
		memcpy(messageBuf.buffer - shift, (char*)&param.length, sizeof(int16_t));

		shift += sizeof(ParameterId_t);
		memcpy(messageBuf.buffer - shift, (char*)&param.parameterId, sizeof(ParameterId_t));
		/* 封装InlineQoS内容 */
		for (ParameterList_t::iterator itParameter = dataFragSubMessage.inlineQos.begin();
			itParameter != dataFragSubMessage.inlineQos.end(); itParameter++) {
			shift += itParameter->length;
			memcpy(messageBuf.buffer - shift, itParameter->value.data(), itParameter->length);
			
			shift += sizeof(int16_t);
			memcpy(messageBuf.buffer - shift, (char*)&itParameter->length, sizeof(int16_t));
			
			shift += sizeof(ParameterId_t);
			memcpy(messageBuf.buffer - shift, (char*)&itParameter->parameterId, sizeof(ParameterId_t));
			
		}
	}
	shift += sizeof(DataFrag_t);
	memcpy(messageBuf.buffer - shift, &dataFragSubMessage.dataFrag, sizeof(DataFrag_t));
	
	shift += sizeof(SequenceNumber_t);
	memcpy(messageBuf.buffer - shift, &dataFragSubMessage.writerSN, sizeof(SequenceNumber_t));
	
	shift += sizeof(EntityId_t);
	memcpy(messageBuf.buffer - shift, &dataFragSubMessage.writerId, sizeof(EntityId_t));
	
	shift += sizeof(EntityId_t);
	memcpy(messageBuf.buffer - shift, &dataFragSubMessage.readerId, sizeof(EntityId_t));
	
	shift += sizeof(uint16_t);
	memcpy(messageBuf.buffer - shift, &dataFragSubMessage.octetsToInlineQos, sizeof(uint16_t));
	
	shift += sizeof(SubMessageFlag_t) * 2;
	memcpy(messageBuf.buffer - shift, dataFragSubMessage.extraFlags, sizeof(SubMessageFlag_t) * 2);
	
	/* 计算报文长度 */
	dataFragSubMessage.header.subMessageLength = shift + messageBuf.buffer_size;
	shift += sizeof(SubMessageHeader_t);
	memcpy(messageBuf.buffer - shift, &dataFragSubMessage.header, sizeof(SubMessageHeader_t));
	
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateHeartBeatMessage(SerializedBuffer& messageBuf,
												uint32_t& shift,
												HeartBeatSubMessage_t& heartBeatSubMessage)
{
	heartBeatSubMessage.header.subMessageId = HEARTBEAT;
	heartBeatSubMessage.header.subMessageLength = sizeof(HeartBeatSubMessage_t) - sizeof(SubMessageHeader_t);

	if (heartBeatSubMessage.header.flags & FLAG_BIT_3) {
		memcpy(messageBuf.buffer + shift, &heartBeatSubMessage, sizeof(HeartBeatSubMessage_t));
		shift += sizeof(HeartBeatSubMessage_t);
	} else {
		heartBeatSubMessage.header.subMessageLength = sizeof(HeartBeatSubMessage_t) - sizeof(SubMessageHeader_t) -32; 
		memcpy(messageBuf.buffer + shift, &heartBeatSubMessage, sizeof(HeartBeatSubMessage_t) - 32);
		shift += sizeof(HeartBeatSubMessage_t) - 32;

	}

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateAckNackMessage(SerializedBuffer& messageBuf,
												uint32_t& shift,
												AcknackSubMessage_t& ackMessage)
{
	ackMessage.header.subMessageId = ACKNACK;
	uint32_t bShift = shift;

	memcpy(messageBuf.buffer + shift, &ackMessage.header, sizeof(SubMessageHeader_t));
	shift += sizeof(SubMessageHeader_t);
	uint32_t sizeShift = shift;
	memcpy(messageBuf.buffer + shift, &ackMessage.readerId, sizeof(ackMessage.readerId));
	shift += sizeof(ackMessage.readerId);
	memcpy(messageBuf.buffer + shift, &ackMessage.writerId, sizeof(ackMessage.writerId));
	shift += sizeof(ackMessage.writerId);
	memcpy(messageBuf.buffer + shift, &ackMessage.readerSNState.bitmapBase, sizeof(SequenceNumber_t));
	shift += sizeof(SequenceNumber_t);
	memcpy(messageBuf.buffer + shift, &ackMessage.readerSNState.numBit, sizeof(uint32_t));
	shift += sizeof(uint32_t);

	if (0 != ackMessage.readerSNState.numBit)
	{
		uint32_t uilen = (((ackMessage.readerSNState.numBit - 1) >> 5) + 1) * 4;
		memcpy(messageBuf.buffer + shift, &ackMessage.readerSNState.bitmap, uilen);
		shift += uilen;
	}

	memcpy(messageBuf.buffer + shift, &ackMessage.count, sizeof(uint32_t));
	shift += sizeof(uint32_t);

	/* 修改子报文的长度 */
	ackMessage.header.subMessageLength = shift - sizeShift;
	memcpy(messageBuf.buffer + bShift, (char*)&ackMessage.header, sizeof(SubMessageHeader_t));

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateHeartBeatFragMessage(SerializedBuffer& messageBuf,
													uint32_t& shift,
													HeartBeatFragSubMessage_t& heartBeatFragSubMessage)
{
	uint32_t bShift = shift;
	heartBeatFragSubMessage.header.subMessageId = HEARTBEAT_FRAG;
	memcpy(messageBuf.buffer + shift, &heartBeatFragSubMessage.header, sizeof(SubMessageHeader_t));
	shift += sizeof(SubMessageHeader_t);
	uint32_t sizeShift = shift;
	memcpy(messageBuf.buffer + shift, &heartBeatFragSubMessage.readerId, sizeof(EntityId_t));
	shift += sizeof(EntityId_t);
	memcpy(messageBuf.buffer + shift, &heartBeatFragSubMessage.writerId, sizeof(EntityId_t));
	shift += sizeof(EntityId_t);
	memcpy(messageBuf.buffer + shift, &heartBeatFragSubMessage.writerSN, sizeof(SequenceNumber_t));
	shift += sizeof(SequenceNumber_t);
	memcpy(messageBuf.buffer + shift, &heartBeatFragSubMessage.lastFragmentNum, sizeof(FragmentNumber_t));
	shift += sizeof(FragmentNumber_t);
	memcpy(messageBuf.buffer + shift, &heartBeatFragSubMessage.count, sizeof(uint32_t));
	shift += sizeof(uint32_t);

	/* 修改子报文的长度 */
	heartBeatFragSubMessage.header.subMessageLength = shift - sizeShift;
	memcpy(messageBuf.buffer + bShift, (char*)&heartBeatFragSubMessage.header, sizeof(SubMessageHeader_t));

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateNackFragMessage(SerializedBuffer& messageBuf,
												uint32_t& shift,
												NackFragSubMessage_t& nackFlagMessage)
{
	uint32_t bShift = shift;
	SubMessageHeader_t header;
	header.subMessageId = NACK_FRAG;
	header.flags |= 0x01;

	memcpy(messageBuf.buffer + shift, &header, sizeof(header));
	shift += sizeof(header);
	uint32_t sizeShift = shift;
	memcpy(messageBuf.buffer + shift, &nackFlagMessage.readerId, sizeof(nackFlagMessage.readerId));
	shift += sizeof(nackFlagMessage.readerId);
	memcpy(messageBuf.buffer + shift, &nackFlagMessage.writerId, sizeof(nackFlagMessage.writerId));
	shift += sizeof(nackFlagMessage.writerId);
	memcpy(messageBuf.buffer + shift, &nackFlagMessage.writerSN, sizeof(SequenceNumber_t));
	shift += sizeof(SequenceNumber_t);
	memcpy(messageBuf.buffer + shift, &nackFlagMessage.fragmentNumberState.bitmapBase, sizeof(FragmentNumber_t));
	shift += sizeof(FragmentNumber_t);
	memcpy(messageBuf.buffer + shift, &nackFlagMessage.fragmentNumberState.numBit, sizeof(uint32_t));
	shift += sizeof(uint32_t);

	if (0 != nackFlagMessage.fragmentNumberState.numBit)
	{
		uint32_t uilen = (((nackFlagMessage.fragmentNumberState.numBit - 1) >> 5) + 1) * 4;
		memcpy(messageBuf.buffer + shift, &nackFlagMessage.fragmentNumberState.bitmap, uilen);
		shift += uilen;
	}

	memcpy(messageBuf.buffer + shift, &nackFlagMessage.count, sizeof(uint32_t));
	shift += sizeof(uint32_t);

	/* 修改子报文的长度 */
	header.subMessageLength = shift - sizeShift;
	memcpy(messageBuf.buffer + bShift, (char*)&header, sizeof(header));
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateINFOSRCMessage(SerializedBuffer& messageBuf,
												uint32_t& shift,
												InfoSource_t infoSRC)
{
	uint16_t size = 20;
	infoSRC.header.subMessageId = INFO_SRC;
	infoSRC.header.subMessageLength = size;

	shift += sizeof(InfoSource_t);
	memcpy(messageBuf.buffer - shift, &infoSRC, sizeof(InfoSource_t));
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateGapMessage(SerializedBuffer& messageBuf,
											uint32_t& shift,
											GapSubMessage_t& gapSubMessage)
{
	uint32_t bShift = shift;
	//SubMessageHeader_t header = gapSubMessage.header;

	gapSubMessage.header.subMessageId = GAP;

	memcpy(messageBuf.buffer + shift, &gapSubMessage.header, sizeof(SubMessageHeader_t));
	shift += sizeof(SubMessageHeader_t);
	uint32_t sizeShift = shift;
	memcpy(messageBuf.buffer + shift, &gapSubMessage.readerId, sizeof(EntityId_t));
	shift += sizeof(EntityId_t);
	memcpy(messageBuf.buffer + shift, &gapSubMessage.writerId, sizeof(EntityId_t));
	shift += sizeof(EntityId_t);
	memcpy(messageBuf.buffer + shift, &gapSubMessage.gapStart, sizeof(SequenceNumber_t));
	shift += sizeof(SequenceNumber_t);
	memcpy(messageBuf.buffer + shift, &gapSubMessage.gapList.bitmapBase, sizeof(SequenceNumber_t));
	shift += sizeof(SequenceNumber_t);
	memcpy(messageBuf.buffer + shift, &gapSubMessage.gapList.numBit, sizeof(uint32_t));
	shift += sizeof(uint32_t);

	if (0 != gapSubMessage.gapList.numBit) {
		uint32_t uilen = (((gapSubMessage.gapList.numBit - 1) >> 5) + 1) * 4;
		memcpy(messageBuf.buffer + shift, &gapSubMessage.gapList.bitmap, uilen);
		shift += uilen;
	}

	bool groupInfoFlag = (gapSubMessage.header.flags & BIT(1)) != 0;
	bool filteredCountFlag = (gapSubMessage.header.flags & BIT(2)) != 0;

	if (groupInfoFlag) {
		memcpy(messageBuf.buffer + shift, &gapSubMessage.gapStartGSN, sizeof(SequenceNumber_t));
		shift += sizeof(SequenceNumber_t);
		memcpy(messageBuf.buffer + shift, &gapSubMessage.gapEndGSN, sizeof(SequenceNumber_t));
		shift += sizeof(SequenceNumber_t);
	}
	if (filteredCountFlag) {
		memcpy(messageBuf.buffer + shift, &gapSubMessage.filteredCount, sizeof(ChangeCount_t));
		shift += sizeof(ChangeCount_t);
	}

	/* 修改子报文的长度 */
	gapSubMessage.header.subMessageLength = shift - sizeShift;
	memcpy(messageBuf.buffer + bShift, (char*)&gapSubMessage.header, sizeof(SubMessageHeader_t));
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulatePadMessage(SerializedBuffer& messageBuf,
											uint32_t& shift)
{
	SubMessageHeader_t padSubMessage;
	padSubMessage.subMessageId = PAD;
	padSubMessage.flags |= 0x01;
	padSubMessage.subMessageLength = 0;

	memcpy(messageBuf.buffer + shift, &padSubMessage, sizeof(padSubMessage));
	shift += sizeof(padSubMessage);
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateINFOTSMessage(SerializedBuffer& messageBuf,
											   uint32_t & shift,
											   InfoTimestamp_t infoTS)
{
	uint16_t size = 8;

	infoTS.header.subMessageId = INFO_TS;
	infoTS.header.subMessageLength = size;

	shift += sizeof(InfoTimestamp_t);
	memcpy(messageBuf.buffer - shift, &infoTS, sizeof(InfoTimestamp_t));
	
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateINFOTSMessageNow(SerializedBuffer& messageBuf,
													uint32_t& shift,
													InfoTimestamp_t infoTS)
{
	Time_t timeNow;
	Time_t::now(timeNow);

	bool invalidateFlag = (infoTS.header.flags & BIT(1)) != 0;

	if (invalidateFlag) {
		infoTS.timestamp = timeNow;
	}

	return EncapsulateINFOTSMessage(messageBuf, shift, infoTS);
}

ReturnCode_t MessageProcessor::EncapsulateINFODSTMessage(SerializedBuffer& messageBuf,
												uint32_t& shift,
												InfoDestinationSubMessage_t& infoDST)
{
	uint16_t size = 12;
	infoDST.header.subMessageLength = size;
	infoDST.header.subMessageId = INFO_DST;
	
	shift += sizeof(InfoDestinationSubMessage_t);
	memcpy(messageBuf.buffer - shift, &infoDST, sizeof(InfoDestinationSubMessage_t));
	
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateINFOREPLYMessage(SerializedBuffer& messageBuf,
	uint32_t& shift,
	InfoReplySubMessage_t& infoReplySubMessage)
{
	infoReplySubMessage.header.subMessageId = INFO_REPLY;
	uint32_t bShift = shift;
	memcpy(messageBuf.buffer + shift, &infoReplySubMessage.header, sizeof(SubMessageHeader_t));
	shift += sizeof(SubMessageHeader_t);
	uint32_t sizeShift = shift;

	uint32_t iSize = infoReplySubMessage.unicastLocatorList.size();
	memcpy(messageBuf.buffer + shift, &iSize, sizeof(uint32_t));
	shift += sizeof(uint32_t);
	for (int32_t i = 0; i < iSize; i++) {
		memcpy(messageBuf.buffer + shift, &infoReplySubMessage.unicastLocatorList[i], sizeof(Locator_t));
		shift += sizeof(Locator_t);
	}
	
	bool multicastFlag = (infoReplySubMessage.header.flags & BIT(1)) != 0;

	if (multicastFlag) {
		iSize = infoReplySubMessage.multicastLocatorList.size();
		memcpy(messageBuf.buffer + shift, &iSize, sizeof(uint32_t));
		shift += sizeof(uint32_t);
		for (int32_t i = 0; i < iSize; i++) {
			memcpy(messageBuf.buffer + shift, &infoReplySubMessage.multicastLocatorList[i], sizeof(Locator_t));
			shift += sizeof(Locator_t);
		}
	}


	/* 修改子报文的长度 */
	infoReplySubMessage.header.subMessageLength = shift - sizeShift;
	memcpy(messageBuf.buffer + bShift, (char*)&infoReplySubMessage.header, sizeof(SubMessageHeader_t));
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateMessageHeader(SerializedBuffer& messageBuf,
												uint32_t& shift,
												const GuidPrefix_t& guidPrefix,
												const ProtocolVersion_t& version,
												const VendorId_t& vendorId)
{
	MessageHeader_t header;
	memcpy(header.protocol, PROTOCOL_RTPS, sizeof(ProtocolId_t));
	header.version = version;
	header.vendorId[0] = vendorId[0];
	header.vendorId[1] = vendorId[1];
	memcpy(header.guidPrefix.value, guidPrefix.value, sizeof(GuidPrefix_t));

	shift += sizeof(MessageHeader_t);
	memcpy(messageBuf.buffer - shift, &header, sizeof(MessageHeader_t));
	
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateMessageHeader(SerializedBuffer& messageBuf,
											    uint32_t& shift,
												const GuidPrefix_t& guidPrefix)
{
	return EncapsulateMessageHeader(messageBuf, shift, guidPrefix, PROTOCOLVERSION, VENDORID_PROSIMA);
}

/************************************************************
** 功能描述：解析报文头 指针地址同步修改，故参数使用传引用
*************************************************************/
ReturnCode_t MessageProcessor::InterpretMessageHeader(char*& message, Receive_t& messageReceiver)
{
	if (message[0] != 'R' || message[1] != 'T' ||
		message[2] != 'P' || message[3] != 'S') {
		std::cout << "Msg received with no RTPS in header, ignoring..." << std::endl;
		return RETCODE_ERROR;
	}

	MessageHeader_t Header;
	memcpy(&Header, message, sizeof(Header));
	message += sizeof(Header);

	memcpy(messageReceiver.sourceGuidPrefix.value, Header.guidPrefix.value, sizeof(GuidPrefix_t));
	messageReceiver.sourceVersion = Header.version;
	messageReceiver.sourceVendorId[0] = Header.vendorId[0];
	messageReceiver.sourceVendorId[1] = Header.vendorId[1];
	messageReceiver.haveTimestame = false;
	return RETCODE_OK;
}

/************************************************************
** 功能描述：解析子报文 指针地址同步修改，故参数使用传引用
*************************************************************/
ReturnCode_t MessageProcessor::InterpretSubmessage(char *& message, uint32_t messageLen)
{
	ReturnCode_t ret = RETCODE_OK;

	Receive_t messageReceiver;

	ret = InterpretMessageHeader(message, messageReceiver);
	if (ret != RETCODE_OK) {
		return ret;
	}
	messageLen -= sizeof(MessageHeader_t);

	SubMessageHeader_t subMessageHeader;

	do{
		/* 解析子报文头 */
		memcpy(&subMessageHeader, message, sizeof(SubMessageHeader_t));

		/* 判断报文字节序 与本地字节序不一致需要进行转换 */
		if (TypeSupport::IsLittleEndian() != (subMessageHeader.flags & 0x01)){
			subMessageHeader.subMessageLength = ntohs(subMessageHeader.subMessageLength);
		}
			
		switch (subMessageHeader.subMessageId) {
			case DATA:
				ret = InterpretDataMessage(message, subMessageHeader, messageReceiver);
				break;
			case HEARTBEAT:
				ret = InterpretHeartbeatMessage(message, subMessageHeader, messageReceiver);
				break;
			case ACKNACK:
				ret = InterpretAckNackMessage(message, subMessageHeader, messageReceiver);
				break;
			case DATA_FRAG:
				ret = InterpretDataFragMessage(message, subMessageHeader, messageReceiver);
				break;
			case NACK_FRAG:
				ret = InterpretNackFragMessage(message, subMessageHeader, messageReceiver);
				break;
			case GAP:
				ret = InterpretGapMessage(message, subMessageHeader, messageReceiver);
				break;
			case INFO_TS:
				ret = InterpretINFOTSMessage(message, subMessageHeader, messageReceiver);
				break;
			case INFO_SRC:
				ret = InterpretINFOSRCMessage(message, subMessageHeader, messageReceiver);
				break;
			case INFO_DST:
				ret = InterpretINFODSTMessage(message, subMessageHeader, messageReceiver);
				break;
			case PAD:
				/* ret = InterpretINFOPadMessage(message, subMessageHeader, messageReceiver); */
				break;
			case HEARTBEAT_FRAG:
				ret = InterpretHeartbeatFragMessage(message, subMessageHeader, messageReceiver);
				break;
			case INFO_REPLY:
				ret = InterpretINFOREPLYMessage(message, subMessageHeader, messageReceiver);
				break;
			default:
				ret = RETCODE_SUBMESSAGE_INVALID;
				break;
		}

		message += sizeof(SubMessageHeader_t) + subMessageHeader.subMessageLength;
		messageLen -= sizeof(SubMessageHeader_t) + subMessageHeader.subMessageLength;
	} while (messageLen > 0);

	return ret;
}


/************************************************************
** 功能描述：解析Data子报文 指针地址不同步修改，故不使用传引用
*************************************************************/
ReturnCode_t MessageProcessor::InterpretDataMessage(char * message,
	SubMessageHeader_t & submessageHeader,
	Receive_t & messageReceiver)
{
	uint32_t messageShift = 0;
	uint32_t serializedPayloadLen = 0;
	//解析 flags 每一位
	bool endiannessFlag = (submessageHeader.flags & BIT(0)) != 0;
	bool inlineQosFlag = (submessageHeader.flags & BIT(1)) != 0;
	bool dataFlag = (submessageHeader.flags & BIT(2)) != 0;
	bool keyFlag = (submessageHeader.flags & BIT(3)) != 0;
	bool nonStandardPayloadFlag = (submessageHeader.flags & BIT(4)) != 0;

	if (keyFlag && dataFlag) {
		std::cout << "Message received with Data and Key Flag set, ignoring"<<std::endl;
		return RETCODE_ERROR;
	}

	DataSubMessage_t dataSubMessage;

	//messageReceiver.nonStandardPayloadFlag = nonStandardPayloadFlag;

	memcpy(&dataSubMessage.header, message + messageShift, sizeof(SubMessageHeader_t));
	messageShift += sizeof(SubMessageHeader_t);
	memcpy(&dataSubMessage.extraFlags, message + messageShift, sizeof(SubMessageFlag_t) * 2 );
	messageShift += sizeof(SubMessageFlag_t) * 2 ;
	memcpy(&dataSubMessage.octetsToInlineQos, message + messageShift, sizeof(uint16_t));
	messageShift += sizeof(uint16_t);
	memcpy(&dataSubMessage.readerId, message + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&dataSubMessage.writerId, message + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&dataSubMessage.writerSN, message + messageShift, sizeof(SequenceNumber_t));
	messageShift += sizeof(SequenceNumber_t);

	/* 判断报文字节序 与本地字节序不一致需要进行转换 */
	if (TypeSupport::IsLittleEndian() != endiannessFlag) {
		dataSubMessage.header.subMessageLength = ntohs(dataSubMessage.header.subMessageLength);
		dataSubMessage.octetsToInlineQos = ntohs(dataSubMessage.octetsToInlineQos);
		dataSubMessage.writerSN.high = ntohl(dataSubMessage.writerSN.high);
		dataSubMessage.writerSN.low = ntohl(dataSubMessage.writerSN.low);
	}
	//TODO 
	if (dataSubMessage.writerSN <= SequenceNumber_t()) {
		std::cout<<"Invalid message received, bad sequence Number"<<std::endl;
		return RETCODE_ERROR;
	}

	/*  判断是否存在inlineQos */
	if (inlineQosFlag) {
		InterpretInlineQosMessage(message, messageShift, dataSubMessage.inlineQos, endiannessFlag);
	}

	/* 判断是否有Serialized Data数据 */
	if (dataFlag || keyFlag) {
		dataSubMessage.serializedPayload = (unsigned char*)message + messageShift;
		/* 报文长度 */
		serializedPayloadLen = dataSubMessage.header.subMessageLength + sizeof(SubMessageHeader_t) - messageShift;
		messageShift += serializedPayloadLen;
	} else {
		dataSubMessage.serializedPayload = NULL;
	}

	std::list<EntityId_t> readerList;

	if(ENTITYID_UNKNOWN == dataSubMessage.readerId) {
		//TODO 添加处理
		GUID_t guid(messageReceiver.sourceGuidPrefix, dataSubMessage.writerId);
		participant_->GetMacthList(guid, readerList);
	} else {
		readerList.push_back(dataSubMessage.readerId);
	}
	for (auto entittyId : readerList) {
		std::shared_ptr<Reader> reader = participant_->GetReader(entittyId);
		if (reader) {
			reader->ReceiveDataMessage(messageReceiver, dataSubMessage, dataSubMessage.serializedPayload, serializedPayloadLen);
		}
	}
	return RETCODE_OK;
}

/************************************************************
** 功能描述：解析HeartBeat子报文 指针地址不同步修改，故不使用传引用
*************************************************************/
ReturnCode_t MessageProcessor::InterpretHeartbeatMessage(char * message,
	SubMessageHeader_t& submessageHeader,
	Receive_t& messageReceiver)
{
	bool endiannessFlag = (submessageHeader.flags & BIT(0)) != 0;
	bool finalFlag = (submessageHeader.flags & BIT(1)) != 0;
	bool livelinessFlag = (submessageHeader.flags & BIT(2)) != 0;
	bool groupInfoFlag = (submessageHeader.flags & BIT(3)) != 0;

	HeartBeatSubMessage_t heartBeatSubMessage;
	heartBeatSubMessage.header = submessageHeader;
	
	memcpy(&heartBeatSubMessage, message, submessageHeader.subMessageLength + 4ull);


	GUID_t writerGUID;
	GUID_t readerGUID;
	readerGUID.prefix = messageReceiver.destGuidPrefix;
	readerGUID.entityId = heartBeatSubMessage.readerId;
	writerGUID.prefix = messageReceiver.sourceGuidPrefix;
	writerGUID.entityId = heartBeatSubMessage.writerId;

	/* 大小端转换-收到的大小端信息和本地不同进行转换 */
	if (TypeSupport::IsLittleEndian() != endiannessFlag) {
		heartBeatSubMessage.header.subMessageLength = htons(heartBeatSubMessage.header.subMessageLength);
		heartBeatSubMessage.count = htonl(heartBeatSubMessage.count);
		heartBeatSubMessage.firstSN.high = htonl(heartBeatSubMessage.firstSN.high);
		heartBeatSubMessage.firstSN.low = htonl(heartBeatSubMessage.firstSN.low);
		heartBeatSubMessage.lastSN.high = htonl(heartBeatSubMessage.lastSN.high);
		heartBeatSubMessage.lastSN.low = htonl(heartBeatSubMessage.lastSN.low);
		if (groupInfoFlag) {
			heartBeatSubMessage.currentGSN.high = htonl(heartBeatSubMessage.currentGSN.high);
			heartBeatSubMessage.currentGSN.low = htonl(heartBeatSubMessage.currentGSN.low);
			heartBeatSubMessage.firstGSN.high = htonl(heartBeatSubMessage.firstGSN.high);
			heartBeatSubMessage.firstGSN.low = htonl(heartBeatSubMessage.firstGSN.low);
			heartBeatSubMessage.lastGSN.high = htonl(heartBeatSubMessage.lastGSN.high);
			heartBeatSubMessage.lastGSN.low = htonl(heartBeatSubMessage.lastGSN.low);
		}
	}

	std::shared_ptr<Reader> reader = participant_->GetReader(heartBeatSubMessage.readerId);
	if (reader) {
		reader->ReceiveHeartbeatMessage(messageReceiver, heartBeatSubMessage);
	}
		
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretHeartbeatFragMessage(char* message,
	SubMessageHeader_t& submessageHeader, 
	Receive_t& messageReceiver)
{
	HeartBeatFragSubMessage_t heartBeatFragSubMessage;
	memcpy(&heartBeatFragSubMessage, message, sizeof(HeartBeatFragSubMessage_t));

	GUID_t writerGUID;
	GUID_t readerGUID;
	readerGUID.prefix = messageReceiver.destGuidPrefix;
	readerGUID.entityId = heartBeatFragSubMessage.readerId;
	writerGUID.prefix = messageReceiver.sourceGuidPrefix;
	writerGUID.entityId = heartBeatFragSubMessage.writerId;

	/* JDXCZ 大小端转换-收到的大小端信息和本地不同进行转换 */
	if (TypeSupport::IsLittleEndian() != (heartBeatFragSubMessage.header.flags & 0x01)) {
		heartBeatFragSubMessage.header.subMessageLength = htons(heartBeatFragSubMessage.header.subMessageLength);
		heartBeatFragSubMessage.count = htonl(heartBeatFragSubMessage.count);
		heartBeatFragSubMessage.writerSN.high = htonl(heartBeatFragSubMessage.writerSN.high);
		heartBeatFragSubMessage.writerSN.low = htonl(heartBeatFragSubMessage.writerSN.low);
	}

	std::shared_ptr<Reader> reader = participant_->GetReader(heartBeatFragSubMessage.readerId);
	if (reader) {
		reader->ReceiveHeartFragMessage(messageReceiver, heartBeatFragSubMessage);
	}

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretAckNackMessage(char* message, SubMessageHeader_t& submessageHeader, Receive_t& messageReceiver)
{
	bool endiannessFlag = (submessageHeader.flags & BIT(0)) != 0;
	bool finalFlag = (submessageHeader.flags & BIT(1)) != 0;
	AcknackSubMessage_t ackMessage{};
	uint32_t messageShift = 0;
	memcpy(&ackMessage.header, message + messageShift, sizeof(SubMessageHeader_t));
	messageShift += sizeof(SubMessageHeader_t);
	memcpy(&ackMessage.readerId, message + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&ackMessage.writerId, message + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);

	memcpy(&ackMessage.readerSNState.bitmapBase, message + messageShift, sizeof(SequenceNumber_t));
	messageShift += sizeof(SequenceNumber_t);
	memcpy(&ackMessage.readerSNState.numBit, message + messageShift, sizeof(uint32_t));
	messageShift += sizeof(uint32_t);

	if (TypeSupport::IsLittleEndian() != (ackMessage.header.flags & 0x01)) {
		ackMessage.header.subMessageLength = ntohs(ackMessage.header.subMessageLength);
		ackMessage.count = ntohs(ackMessage.count);
		ackMessage.readerSNState.bitmapBase.high = ntohl(ackMessage.readerSNState.bitmapBase.high);
		ackMessage.readerSNState.bitmapBase.low = ntohl(ackMessage.readerSNState.bitmapBase.low);
		ackMessage.readerSNState.numBit = ntohl(ackMessage.readerSNState.numBit);
	}

	if (0 != ackMessage.readerSNState.numBit) {
		uint32_t uilen = (((ackMessage.readerSNState.numBit - 1) >> 5) + 1) * 4;
		memcpy(ackMessage.readerSNState.bitmap, message + messageShift, uilen);
		messageShift += uilen;
		/* 大小端转换 */
		if (TypeSupport::IsLittleEndian() != (ackMessage.header.flags & 0x01)) {
			for (int32_t i = 0; i < uilen; i++) {
				ackMessage.readerSNState.bitmap[i] = ntohl(ackMessage.readerSNState.bitmap[i]);
			}
		}
	}

	memcpy(&ackMessage.count, message + messageShift, sizeof(Count_t));
	messageShift += sizeof(Count_t);

	GUID_t writerGUID;
	GUID_t readerGUID;
	readerGUID.prefix = messageReceiver.destGuidPrefix;
	readerGUID.entityId = ackMessage.readerId;
	writerGUID.prefix = messageReceiver.sourceGuidPrefix;
	writerGUID.entityId = ackMessage.writerId;

	std::shared_ptr<Writer> writer = participant_->GetWriter(ackMessage.writerId);
	if (writer) {
		writer->ReceiveAckNackMessage(messageReceiver, ackMessage);
	}

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretDataFragMessage(char* message,
	SubMessageHeader_t& submessageHeader,
	Receive_t& messageReceiver)
{
	uint32_t messageShift = 0;
	uint32_t serializedPayloadLen = 0;
	bool endiannessFlag = (submessageHeader.flags & BIT(0)) != 0;
	bool inlineQosFlag = (submessageHeader.flags & BIT(1)) != 0;
	bool nonStandardPayloadFlag = (submessageHeader.flags & BIT(2)) != 0;
	bool keyFlag = (submessageHeader.flags & BIT(3)) != 0;
	

	DataFragSubMessage_t dataFragSubMessage;

	//messageReceiver.nonStandardPayloadFlag = nonStandardPayloadFlag;

	memcpy(&dataFragSubMessage.header, message + messageShift, sizeof(SubMessageHeader_t));
	messageShift += sizeof(SubMessageHeader_t);
	memcpy(&dataFragSubMessage.extraFlags, message + messageShift, sizeof(SubMessageFlag_t) * 2);
	messageShift += sizeof(SubMessageFlag_t) * 2;
	memcpy(&dataFragSubMessage.octetsToInlineQos, message + messageShift, sizeof(uint16_t));
	messageShift += sizeof(uint16_t);
	memcpy(&dataFragSubMessage.readerId, message + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&dataFragSubMessage.writerId, message + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&dataFragSubMessage.writerSN, message + messageShift, sizeof(SequenceNumber_t));
	messageShift += sizeof(SequenceNumber_t);

	/* 判断报文字节序 与本地字节序不一致需要进行转换 */
	if (TypeSupport::IsLittleEndian() != (dataFragSubMessage.header.flags & 0x01)) {
		dataFragSubMessage.header.subMessageLength = ntohs(dataFragSubMessage.header.subMessageLength);
		dataFragSubMessage.octetsToInlineQos = ntohs(dataFragSubMessage.octetsToInlineQos);
		dataFragSubMessage.writerSN.high = ntohl(dataFragSubMessage.writerSN.high);
		dataFragSubMessage.writerSN.low = ntohl(dataFragSubMessage.writerSN.low);
	}

	if (dataFragSubMessage.writerSN <= SequenceNumber_t()) {
		std::cout << "Invalid message received, bad sequence Number" << std::endl;
		return RETCODE_ERROR;
	}

	memcpy(&dataFragSubMessage.dataFrag, message + messageShift, sizeof(DataFrag_t));
	messageShift += sizeof(DataFrag_t);
	/* 大小端转换 */
	if (TypeSupport::IsLittleEndian() != endiannessFlag) {
		dataFragSubMessage.dataFrag.fragmentStartingNum		= htonl(dataFragSubMessage.dataFrag.fragmentStartingNum);
		dataFragSubMessage.dataFrag.sampleSize				= htonl(dataFragSubMessage.dataFrag.sampleSize);
		dataFragSubMessage.dataFrag.fragmentsInSubmessage	= htons(dataFragSubMessage.dataFrag.fragmentsInSubmessage);
		dataFragSubMessage.dataFrag.fragmentSize			= htons(dataFragSubMessage.dataFrag.fragmentSize);
	}

	/* 判断是否存在inlineQos*/
	if (inlineQosFlag) {
		InterpretInlineQosMessage(message, messageShift, dataFragSubMessage.inlineQos, endiannessFlag);
	}

	if (!keyFlag) {
		dataFragSubMessage.serializedPayload = (unsigned char*)message + messageShift;
		/* 报文长度 */
		serializedPayloadLen = dataFragSubMessage.header.subMessageLength + sizeof(SubMessageHeader_t) - messageShift;
		messageShift += serializedPayloadLen;
	} else {
	}
	std::list<EntityId_t> readerList;

	if (ENTITYID_UNKNOWN == dataFragSubMessage.readerId) {
		//TODO 添加处理
		GUID_t guid(messageReceiver.sourceGuidPrefix, dataFragSubMessage.writerId);
		participant_->GetMacthList(guid, readerList);
	} else {
		readerList.push_back(dataFragSubMessage.readerId);
	}
	for (auto entittyId : readerList) {
		std::shared_ptr<Reader> reader = participant_->GetReader(entittyId);
		if (reader) {
			reader->ReceiveDataFragMessage(messageReceiver, dataFragSubMessage, dataFragSubMessage.serializedPayload, serializedPayloadLen);
		}
	}

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretINFOTSMessage(char* message, SubMessageHeader_t& submessageHeader, Receive_t& messageReceiver)
{
	InfoTimestamp_t infoTs;

	memcpy(&infoTs, message, sizeof(InfoTimestamp_t));
	message += sizeof(InfoTimestamp_t);

	bool invalidateFlag = (infoTs.header.flags & BIT(1)) != 0;

	if (invalidateFlag) {
		messageReceiver.timestamp = infoTs.timestamp;
	}

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretINFOSRCMessage(char* message, SubMessageHeader_t& submessageHeader, Receive_t& messageReceiver)
{
	InfoSource_t InfoSrc;

	memcpy(&InfoSrc, message, sizeof(InfoSource_t));
	message += sizeof(InfoSource_t);
	messageReceiver.sourceVendorId[0] = InfoSrc.vendorId[0];
	messageReceiver.sourceVendorId[1] = InfoSrc.vendorId[1];
	messageReceiver.sourceVersion = InfoSrc.version;

	messageReceiver.sourceGuidPrefix = InfoSrc.guidPrefix;
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretINFODSTMessage(char* message,
	SubMessageHeader_t& submessageHeader, 
	Receive_t& messageReceiver)
{
	InfoDestinationSubMessage_t InfoDst;

	memcpy(&InfoDst, message, sizeof(InfoDestinationSubMessage_t));
	message += sizeof(InfoDestinationSubMessage_t);

	messageReceiver.destGuidPrefix = InfoDst.guidPrefix;

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretInlineQosMessage(char*& message,
	uint32_t& messageShift,
	ParameterList_t& inlineQos,
	bool endiannessFlag)
{
	Parameter_t parameter;

	memcpy(&parameter, message + messageShift, sizeof(parameter.parameterId) + sizeof(parameter.length));
	messageShift += sizeof(parameter.parameterId) + sizeof(parameter.length);

	do {
		/* 判断报文字节序 与本地字节序不一致需要进行转换 */
		if (TypeSupport::IsLittleEndian() != endiannessFlag) {
			parameter.parameterId = ntohs(parameter.parameterId);
			parameter.length = ntohs(parameter.length);
		}

		parameter.value.assign(message + messageShift, message + messageShift + parameter.length);
		messageShift += parameter.length;

		inlineQos.push_back(parameter);

		memcpy(&parameter, message + messageShift, sizeof(parameter.parameterId) + sizeof(parameter.length));
		messageShift += sizeof(parameter.parameterId) + sizeof(parameter.length);

	} while ((ParameterId_t)ParameterId::PID_SENTINEL != parameter.parameterId);

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretNackFragMessage(char* message, SubMessageHeader_t& submessageHeader, Receive_t& messageReceiver)
{
	NackFragSubMessage_t nackFragMessage{};
	bool endiannessFlag = (submessageHeader.flags & BIT(0)) != 0;
	bool finalFlag = (submessageHeader.flags & BIT(1)) != 0;
	uint32_t messageShift = 0;
	memcpy(&nackFragMessage.header, message + messageShift, sizeof(SubMessageHeader_t));
	messageShift += sizeof(SubMessageHeader_t);
	memcpy(&nackFragMessage.readerId, message + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&nackFragMessage.writerId, message + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&nackFragMessage.writerSN, message + messageShift, sizeof(SequenceNumber_t));
	messageShift += sizeof(SequenceNumber_t);

	memcpy(&nackFragMessage.fragmentNumberState.bitmapBase, message + messageShift, sizeof(FragmentNumber_t));
	messageShift += sizeof(FragmentNumber_t);
	memcpy(&nackFragMessage.fragmentNumberState.numBit, message + messageShift, sizeof(uint32_t));
	messageShift += sizeof(uint32_t);

	/* 大小端转换 */
	if (TypeSupport::IsLittleEndian() != (submessageHeader.flags & 0x01)) {
		nackFragMessage.header.subMessageLength = htons(submessageHeader.subMessageLength);
		nackFragMessage.count = htonl(nackFragMessage.count);
		nackFragMessage.writerSN.high = htonl(nackFragMessage.writerSN.high);
		nackFragMessage.writerSN.low = htonl(nackFragMessage.writerSN.low);
		nackFragMessage.fragmentNumberState.bitmapBase = htonl(nackFragMessage.fragmentNumberState.bitmapBase);
		nackFragMessage.fragmentNumberState.numBit = htonl(nackFragMessage.fragmentNumberState.numBit);
	}

	if (0 != nackFragMessage.fragmentNumberState.numBit) {
		uint32_t uilen = (((nackFragMessage.fragmentNumberState.numBit - 1) >> 5) + 1) * 4;
		memcpy(nackFragMessage.fragmentNumberState.bitmap, message + messageShift, uilen);
		messageShift += uilen;
		/* 大小端转换 */
		if (TypeSupport::IsLittleEndian() != (nackFragMessage.header.flags & 0x01)) {
			for (int32_t i = 0; i < uilen; i++) {
				nackFragMessage.fragmentNumberState.bitmap[i] = ntohl(nackFragMessage.fragmentNumberState.bitmap[i]);
			}
		}
	}

	memcpy(&nackFragMessage.count, message + messageShift, sizeof(Count_t));
	messageShift += sizeof(Count_t);

	GUID_t writerGUID;
	GUID_t readerGUID;
	readerGUID.prefix = messageReceiver.destGuidPrefix;
	readerGUID.entityId = nackFragMessage.readerId;
	writerGUID.prefix = messageReceiver.sourceGuidPrefix;
	writerGUID.entityId = nackFragMessage.writerId;

	std::shared_ptr<Writer> writer = participant_->GetWriter(nackFragMessage.writerId);
	if (writer) {
		writer->ReceiveNackFragMessage(messageReceiver, nackFragMessage);
	}

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretINFOREPLYMessage(char* message, 
	SubMessageHeader_t& submessageHeader, 
	Receive_t& messageReceiver)
{
	uint32_t messageLen = 0;
	InfoReplySubMessage_t infoReplyMessage;
	infoReplyMessage.header = submessageHeader;
	messageLen += sizeof(SubMessageHeader_t);

	bool endiannessFlag = (submessageHeader.flags & BIT(0)) != 0;
	bool multicastFlag = (submessageHeader.flags & BIT(1)) != 0;

	uint32_t iSize = 0;
	memcpy(&iSize, message + messageLen, sizeof(uint32_t));
	messageLen += sizeof(uint32_t);
	/* 大小端转换 */
	if (TypeSupport::IsLittleEndian() != endiannessFlag) {
		iSize = htonl(iSize);
	}
	infoReplyMessage.unicastLocatorList.resize(iSize);
	for (int32_t i = 0; i < iSize; i++) {
		memcpy(&infoReplyMessage.unicastLocatorList[i], message + messageLen, sizeof(Locator_t));
		messageLen += sizeof(Locator_t);
		/* 大小端转换 */
		if (TypeSupport::IsLittleEndian() != endiannessFlag) {
			infoReplyMessage.unicastLocatorList[i].kind = htonl(infoReplyMessage.unicastLocatorList[i].kind);
			infoReplyMessage.unicastLocatorList[i].port = htonl(infoReplyMessage.unicastLocatorList[i].port);
		}
	}

	if (multicastFlag) {
		memcpy(&iSize, message + messageLen, sizeof(uint32_t));
		messageLen += sizeof(uint32_t);
		/* 大小端转换 */
		if (TypeSupport::IsLittleEndian() != endiannessFlag) {
			iSize = htonl(iSize);
		}
		infoReplyMessage.multicastLocatorList.resize(iSize);
		for (int32_t i = 0; i < iSize; i++) {
			memcpy(&infoReplyMessage.multicastLocatorList[i], message + messageLen, sizeof(Locator_t));
			messageLen += sizeof(Locator_t);
			/* 大小端转换 */
			if (TypeSupport::IsLittleEndian() != endiannessFlag) {
				infoReplyMessage.unicastLocatorList[i].kind = htonl(infoReplyMessage.unicastLocatorList[i].kind);
				infoReplyMessage.unicastLocatorList[i].port = htonl(infoReplyMessage.unicastLocatorList[i].port);
			}
		}
	}
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretGapMessage(char* message, SubMessageHeader_t& submessageHeader, Receive_t& messageReceiver)
{
	GapSubMessage_t gapSubMessage;

	bool endiannessFlag = (submessageHeader.flags & BIT(0)) != 0;
	bool groupInfoFlag = (submessageHeader.flags & BIT(1)) != 0;
	bool filteredCountFlag = (submessageHeader.flags & BIT(2)) != 0;

	uint32_t messageShift = 0;
	memcpy(&gapSubMessage.header, message + messageShift, sizeof(SubMessageHeader_t));
	messageShift += sizeof(SubMessageHeader_t);
	memcpy(&gapSubMessage.readerId, message + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&gapSubMessage.writerId, message + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&gapSubMessage.gapStart, message + messageShift, sizeof(SequenceNumber_t));
	messageShift += sizeof(SequenceNumber_t);
	memcpy(&gapSubMessage.gapList.bitmapBase, message + messageShift, sizeof(SequenceNumber_t));
	messageShift += sizeof(SequenceNumber_t);
	memcpy(&gapSubMessage.gapList.numBit, message + messageShift, sizeof(uint32_t));
	messageShift += sizeof(uint32_t);

	/* 大小端转换 */
	if (TypeSupport::IsLittleEndian() != endiannessFlag) {
		gapSubMessage.header.subMessageLength = ntohs(gapSubMessage.header.subMessageLength);

		gapSubMessage.gapStart.low = ntohl(gapSubMessage.gapStart.low);
		gapSubMessage.gapStart.high = ntohl(gapSubMessage.gapStart.high);
		gapSubMessage.gapList.bitmapBase.low = ntohl(gapSubMessage.gapList.bitmapBase.low);
		gapSubMessage.gapList.bitmapBase.high = ntohl(gapSubMessage.gapList.bitmapBase.high);
	}
	if (0 != gapSubMessage.gapList.numBit) {
		uint32_t uilen = (((gapSubMessage.gapList.numBit - 1) >> 5) + 1) * 4;
		memcpy(gapSubMessage.gapList.bitmap, message + messageShift, uilen);
		messageShift += uilen;
		/* 大小端转换 */
		if (TypeSupport::IsLittleEndian() != (gapSubMessage.header.flags & 0x01)) {
			for (int32_t i = 0; i < uilen; i++) {
				gapSubMessage.gapList.bitmap[i] = ntohl(gapSubMessage.gapList.bitmap[i]);
			}
		}
	}

	if (groupInfoFlag) {
		memcpy(&gapSubMessage.gapStartGSN, message + messageShift, sizeof(SequenceNumber_t));
		messageShift += sizeof(SequenceNumber_t);
		memcpy(&gapSubMessage.gapEndGSN, message + messageShift, sizeof(SequenceNumber_t));
		messageShift += sizeof(SequenceNumber_t);
		/* 大小端转换 */
		if (TypeSupport::IsLittleEndian() != endiannessFlag) {
			gapSubMessage.gapStartGSN.low = ntohl(gapSubMessage.gapStartGSN.low);
			gapSubMessage.gapStartGSN.high = ntohl(gapSubMessage.gapStartGSN.high);
			gapSubMessage.gapEndGSN.low = ntohl(gapSubMessage.gapEndGSN.low);
			gapSubMessage.gapEndGSN.high = ntohl(gapSubMessage.gapEndGSN.high);
		}
	}

	if (filteredCountFlag) {
		memcpy(&gapSubMessage.filteredCount, message + messageShift, sizeof(ChangeCount_t));
		messageShift += sizeof(ChangeCount_t);

		/* 大小端转换 */
		if (TypeSupport::IsLittleEndian() != endiannessFlag) {
			gapSubMessage.filteredCount.low = ntohl(gapSubMessage.filteredCount.low);
			gapSubMessage.filteredCount.high = ntohl(gapSubMessage.filteredCount.high);
		}

	}

	GUID_t writerGUID;
	GUID_t readerGUID;
	readerGUID.prefix = messageReceiver.destGuidPrefix;
	readerGUID.entityId = gapSubMessage.readerId;
	writerGUID.prefix = messageReceiver.sourceGuidPrefix;
	writerGUID.entityId = gapSubMessage.writerId;

	std::list<EntityId_t> readerList;

	if (ENTITYID_UNKNOWN == gapSubMessage.readerId) {
		//TODO 添加处理
		GUID_t guid(messageReceiver.sourceGuidPrefix, gapSubMessage.writerId);
		participant_->GetMacthList(guid, readerList);
	} else {
		readerList.push_back(gapSubMessage.readerId);
	}
	for (auto& entittyId : readerList) {
		std::shared_ptr<Reader> reader = participant_->GetReader(entittyId);
		std::shared_ptr<StatefulReader> statefulReader = std::dynamic_pointer_cast<StatefulReader>(reader);
		if (statefulReader) {
			statefulReader->ReceiveGapMessage(messageReceiver, gapSubMessage);
		}
	}

	return RETCODE_OK;
}
