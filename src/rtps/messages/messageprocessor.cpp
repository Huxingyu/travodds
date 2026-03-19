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
#include "common/log/logger.h"
#include <iostream>
#include <asio.hpp>
#ifdef  DDSSECURITY
#include "dcps/domain/domainparticipantimpl.h"
#include "rtps/discovery/discovery.h"
#endif //  DDSSECURITY

USING_TRAVODDS_NAMESPACE;

/************************************************************
** MessageProcessor类构造函数
*************************************************************/
MessageProcessor::MessageProcessor(Participant* participant)
	: participant_(participant)
{
#ifdef  DDSSECURITY
	participantImpl_ = participant_->participantImpl_;
#endif //  DDSSECURITY
}

MessageProcessor::~MessageProcessor()
{
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"

ReturnCode_t MessageProcessor::EncapsulateDATAMessage(SerializedBuffer& messageBuf,
											uint32_t& shift,
											DataSubMessage_t& dataSubMessage)
{
	// uint32_t bShift = shift;
	// bool endiannessFlag = (dataSubMessage.header.flags & SubMessageFlag::ENDIANNESS_FLAG) != SubMessageFlag::NONE_FLAG;
	bool inlineQosFlag = (dataSubMessage.header.flags & SubMessageFlag::INLINE_QOS_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool dataFlag = (dataSubMessage.header.flags & SubMessageFlag::DATA_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool keyFlag = (dataSubMessage.header.flags & SubMessageFlag::KEY_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool nonStandardPayloadFlag = (dataSubMessage.header.flags & SubMessageFlag::NON_STANDARD_PAYLOAD_FLAG) != SubMessageFlag::NONE_FLAG;

	dataSubMessage.header.subMessageId = DATA;

	if (inlineQosFlag) {
		//TODO  在封装的时候将PID_SENTINEL 插入inlineQos的链表中，还是在这边封装一个PID_SENTINEL
		//jdxcz 20250418 选择在封装inlineQOS封装后，封装PID_SENTINEL //

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
	
	//PND 跟fastdds保持一致
#ifdef DDSSECURITY
	if (dataSubMessage.writerId == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) {
		dataSubMessage.readerId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER;
	}
	if (dataSubMessage.writerId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
		dataSubMessage.readerId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER;
	}
	if (dataSubMessage.writerId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
		dataSubMessage.readerId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
	}
	if (dataSubMessage.writerId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER) {
		dataSubMessage.readerId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
	}
#endif
	
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
	// uint32_t bShift = shift;
	// bool endiannessFlag = (dataFragSubMessage.header.flags & SubMessageFlag::ENDIANNESS_FLAG) != SubMessageFlag::NONE_FLAG;
	bool inlineQosFlag = (dataFragSubMessage.header.flags & SubMessageFlag::INLINE_QOS_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool nonStandardPayloadFlag = (dataFragSubMessage.header.flags & SubMessageFlag::FRAG_NON_STANDARD_PAYLOAD_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool keyFlag = (dataFragSubMessage.header.flags & SubMessageFlag::FRAG_KEY_FLAG) != SubMessageFlag::NONE_FLAG;

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

	if (heartBeatSubMessage.header.flags & SubMessageFlag::HB_GROUPINFO_FLAG) {
		memcpy(messageBuf.buffer + shift, &heartBeatSubMessage, sizeof(HeartBeatSubMessage_t));
		shift += sizeof(HeartBeatSubMessage_t);
	} else {
		/* 非组多余字段 */
		uint16_t surplus = sizeof(SequenceNumber_t) * 3 + sizeof(GroupDigest_t) * 2;
		heartBeatSubMessage.header.subMessageLength = sizeof(HeartBeatSubMessage_t) - sizeof(SubMessageHeader_t) - surplus;
		memcpy(messageBuf.buffer + shift, &heartBeatSubMessage, sizeof(HeartBeatSubMessage_t) - surplus);
		shift += sizeof(HeartBeatSubMessage_t) - surplus;

	}
	//PND 跟fastdds保持一致
#ifdef DDSSECURITY
	if (heartBeatSubMessage.writerId == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) {
		heartBeatSubMessage.readerId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER;
	}
	if (heartBeatSubMessage.writerId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
		heartBeatSubMessage.readerId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER;
	}
	if (heartBeatSubMessage.writerId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER) {
		heartBeatSubMessage.readerId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
	}
	if (heartBeatSubMessage.writerId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER) {
		heartBeatSubMessage.readerId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
	}
#endif
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
	memcpy(messageBuf.buffer + shift, &ackMessage.readerSNState.bitmap.size, sizeof(uint32_t));
	shift += sizeof(uint32_t);

	if (0 != ackMessage.readerSNState.bitmap.size)
	{
		uint32_t uilen = ackMessage.readerSNState.bitmap.GetByteSize();
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
	header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

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
	memcpy(messageBuf.buffer + shift, &nackFlagMessage.fragmentNumberState.bitmap.size, sizeof(uint32_t));
	shift += sizeof(uint32_t);

	if (0 != nackFlagMessage.fragmentNumberState.bitmap.size)
	{
		uint32_t uilen = nackFlagMessage.fragmentNumberState.bitmap.GetByteSize();
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
	uint16_t size = SUB_MESSAGE_LEN_INFO_SRC;
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
	memcpy(messageBuf.buffer + shift, &gapSubMessage.gapList.bitmap.size, sizeof(uint32_t));
	shift += sizeof(uint32_t);

	if (0 != gapSubMessage.gapList.bitmap.size) {
		uint32_t uilen = gapSubMessage.gapList.bitmap.GetByteSize();
		memcpy(messageBuf.buffer + shift, &gapSubMessage.gapList.bitmap, uilen);
		shift += uilen;
	}

	bool groupInfoFlag = (gapSubMessage.header.flags & SubMessageFlag::GROUPINFO_FLAG) != SubMessageFlag::NONE_FLAG;
	bool filteredCountFlag = (gapSubMessage.header.flags & SubMessageFlag::FILTERED_COUNT_FLAG) != SubMessageFlag::NONE_FLAG;

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
	padSubMessage.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;
	padSubMessage.subMessageLength = SUB_MESSAGE_LEN_PAD;

	memcpy(messageBuf.buffer + shift, &padSubMessage, sizeof(padSubMessage));
	shift += sizeof(padSubMessage);
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::EncapsulateINFOTSMessage(SerializedBuffer& messageBuf,
											   uint32_t & shift,
											   InfoTimestamp_t infoTS)
{
	uint16_t size = SUB_MESSAGE_LEN_INFO_TS;

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

	bool invalidateFlag = (infoTS.header.flags & SubMessageFlag::INVALIDATE_FLAG) != SubMessageFlag::NONE_FLAG;

	if (invalidateFlag) {
		infoTS.timestamp = timeNow;
	}

	return EncapsulateINFOTSMessage(messageBuf, shift, infoTS);
}

ReturnCode_t MessageProcessor::EncapsulateINFODSTMessage(SerializedBuffer& messageBuf,
												uint32_t& shift,
												InfoDestinationSubMessage_t& infoDST)
{
	uint16_t size = SUB_MESSAGE_LEN_INFO_DST;
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

	uint32_t iSize = static_cast<uint32_t>(infoReplySubMessage.unicastLocatorList.size());
	memcpy(messageBuf.buffer + shift, &iSize, sizeof(uint32_t));
	shift += sizeof(uint32_t);
	for (uint32_t i = 0; i < iSize; i++) {
		memcpy(messageBuf.buffer + shift, &infoReplySubMessage.unicastLocatorList[i].kind, sizeof(int32_t));
		memcpy(messageBuf.buffer + shift + 4u, &infoReplySubMessage.unicastLocatorList[i].port, sizeof(uint32_t));
		memcpy(messageBuf.buffer + shift + 8u, &infoReplySubMessage.unicastLocatorList[i].address, 16u);
		shift += 24u;
	}
	
	bool multicastFlag = (infoReplySubMessage.header.flags & SubMessageFlag::MULTICAST_FLAG) != SubMessageFlag::NONE_FLAG;

	if (multicastFlag) {
		iSize = static_cast<uint32_t>(infoReplySubMessage.multicastLocatorList.size());
		memcpy(messageBuf.buffer + shift, &iSize, sizeof(uint32_t));
		shift += sizeof(uint32_t);
		for (uint32_t i = 0; i < iSize; i++) {
			memcpy(messageBuf.buffer + shift, &infoReplySubMessage.multicastLocatorList[i].kind, sizeof(int32_t));
			memcpy(messageBuf.buffer + shift + 4u, &infoReplySubMessage.multicastLocatorList[i].port, sizeof(uint32_t));
			memcpy(messageBuf.buffer + shift + 8u, &infoReplySubMessage.multicastLocatorList[i].address, 16u);
			shift += 24u;
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
		TRAVODDS_LOG(LOG_ERROR, "Msg received with no RTPS in header, ignoring...");
		return RETCODE_ERROR;
	}

	MessageHeader_t Header;
	memcpy(&Header, message, sizeof(Header));
	int headerLen = sizeof(Header);
	message += headerLen;

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
ReturnCode_t MessageProcessor::InterpretSubmessage(std::shared_ptr<Message_t> message)
{
	ReturnCode_t ret = RETCODE_OK;

	if (message->length < sizeof(MessageHeader_t)) {
		return ret;
	}

	Receive_t messageReceiver;

 	ret = InterpretMessageHeader(message->buffer, messageReceiver);
	if (ret != RETCODE_OK) {
		TRAVODDS_LOG(LOG_ERROR, "InterpretMessageHeader Err ret = %d", ret);
		return ret;
	}
	message->length -= sizeof(MessageHeader_t);
	
#ifdef DDSSECURITY
    // 安全扩展相关
    uint32_t secDataLength = 0;
    bool procingSecSubmsg = false;
    ParticipantCryptoHandle receiverCryptoHdl = participantImpl_->cryptoHdl_;
    GuidPrefix_t remoteGuidPrefix = messageReceiver.sourceGuidPrefix;
    ParticipantCryptoHandle senderCryptoHdl = NULL;
	std::shared_ptr<Spdp> spdp = participantImpl_->GetDiscovery()->GetSpdp();
    if (spdp) {
        senderCryptoHdl = spdp->GetRemoteParticipantCryptoHandle(&remoteGuidPrefix);
    }
    SecurityException secExce;
	SerializedBuffer secDecodedSubmsg;
#endif // DDSSECURITY

	SubMessageHeader_t subMessageHeader;

	do{
		/* 解析子报文头 */
		memcpy(&subMessageHeader, message->buffer, sizeof(SubMessageHeader_t));

		/* 判断报文字节序 与本地字节序不一致需要进行转换 */
		if (DEFAULT_ENDIAN != (subMessageHeader.flags & SubMessageFlag::ENDIANNESS_FLAG)){
			subMessageHeader.subMessageLength = ntohs(subMessageHeader.subMessageLength);
		}
			
#ifdef DDSSECURITY
        // 之前检测到SEC_PREFIX，并且当前子消息不是SEC_POSTFIX
        if (procingSecSubmsg && subMessageHeader.subMessageId != SEC_POSTFIX)
        {
            //累计安全消息长度
            secDataLength += (subMessageHeader.subMessageLength + sizeof(SubMessageHeader_t));
            subMessageHeader.subMessageId = SEC_SUB_MSG;
        }
#endif // DDSSECURITY
		switch (subMessageHeader.subMessageId) {
			case DATA:
				ret = InterpretDataMessage(message, subMessageHeader, messageReceiver);
				break;
			case HEARTBEAT:
				ret = InterpretHeartbeatMessage(message->buffer, subMessageHeader, messageReceiver);
				break;
			case ACKNACK:
				ret = InterpretAckNackMessage(message->buffer, subMessageHeader, messageReceiver);
				break;
			case DATA_FRAG:
				ret = InterpretDataFragMessage(message, subMessageHeader, messageReceiver);
				break;
			case NACK_FRAG:
				ret = InterpretNackFragMessage(message->buffer, subMessageHeader, messageReceiver);
				break;
			case GAP:
				ret = InterpretGapMessage(message->buffer, subMessageHeader, messageReceiver);
				break;
			case INFO_TS:
				ret = InterpretINFOTSMessage(message->buffer, subMessageHeader, messageReceiver);
				break;
			case INFO_SRC:
				ret = InterpretINFOSRCMessage(message->buffer, subMessageHeader, messageReceiver);
				break;
			case INFO_DST:
				ret = InterpretINFODSTMessage(message->buffer, subMessageHeader, messageReceiver);
				break;
			case PAD:
				/* ret = InterpretINFOPadMessage(message, subMessageHeader, messageReceiver); */
				break;
			case HEARTBEAT_FRAG:
				ret = InterpretHeartbeatFragMessage(message->buffer, subMessageHeader, messageReceiver);
				break;
			case INFO_REPLY:
				ret = InterpretINFOREPLYMessage(message->buffer, subMessageHeader, messageReceiver);
				break;
#ifdef DDSSECURITY
        case SEC_SUB_MSG: //不作处理，上面已经累计长度了
        {
            if (!participantImpl_->enableDDSSec_)
            {
                break;
            }
            else
            {
                break;
            }
        }
        // 遇到SEC_POSTFIX，开始处理之前累计的安全数据
        case SEC_POSTFIX:
        {
            if (!participantImpl_->enableDDSSec_ || senderCryptoHdl == SEC_NIL_HANDLE)
            {
                // not supporting security or not ready
                break;
            }
            if (!procingSecSubmsg)
            {
                // 异常状况，没有与之对应的SEC_PREFIX
                break;
            }
            //secDataLength为PREFIX到POSTFIX总长度
            DatawriterCryptoHandle writerCryptoHdl;
            DatareaderCryptoHandle readerCryptoHdl;
            SecureSubmessageCategory_t submsgCategorty;
            SerializedBuffer plainData;
            SerializedBuffer encodedData; //指向message即需要被解密的数据
            plainData.buffer_size = secDataLength; //预留secDataLength空间，最终解密肯定会小于secDataLength
            plainData.buffer = new char[secDataLength];
            encodedData.buffer = message->buffer - secDataLength; //指向从PREFIX子消息开始的整体区域
            secDataLength += (subMessageHeader.subMessageLength + sizeof(SubMessageHeader_t));
            encodedData.buffer_size = secDataLength;

            if (!participantImpl_->secPlugin_.crypto_transform->preprocess_secure_submsg(
                &writerCryptoHdl,
                &readerCryptoHdl,
                &submsgCategorty,
                &encodedData,
                1,
                receiverCryptoHdl,
                senderCryptoHdl,
                &secExce))
            {
				delete[]plainData.buffer;
				if (secExce.minor_code == 5) {
					break; //wait to init remote_participant->writers_key or readers_key
				}
				TRAVODDS_LOG(LOG_ERROR, "preprocess_secure_submsg failed."
                    "\n  Code: %d"
                    "\n  Minor code: %d"
                    "\n  Message: %s",
                    secExce.code,
                    secExce.minor_code,
                    secExce.message);
                break;
            }
            switch (submsgCategorty)
            {
            case DATAWRITER_SUBMESSAGE:
            {
                CryptoWriterSubmessageAttributes writerEncodeAttr;
                if (!participantImpl_->secPlugin_.crypto_transform->get_datawriter_submessage_encode_attributes(
                    SEC_NIL_HANDLE, &writerEncodeAttr))
                {
					TRAVODDS_LOG(LOG_ERROR, "get_datawriter_submessage_encode_attributes failed.");
                    break;
                }
                if (!participantImpl_->secPlugin_.crypto_transform->decode_datawriter_submessage(
                    &plainData,
                    &encodedData,
                    1,
                    readerCryptoHdl,
                    writerCryptoHdl,
                    &secExce))
                {
					delete[]plainData.buffer;
                    return -2;
                }
                procingSecSubmsg = false; //表示此次处理安全子消息结束

                SubMessageHeader_t tmpSubMessageHeader;
                memcpy(&tmpSubMessageHeader, plainData.buffer, sizeof(SubMessageHeader_t));

                //message减去SEC_POSTFIX
                message->buffer += sizeof(SubMessageHeader_t) + subMessageHeader.subMessageLength;
				message->length -= sizeof(SubMessageHeader_t) + subMessageHeader.subMessageLength;

                //拷贝plainData代表的子消息缓存到message
                message->buffer -= sizeof(SubMessageHeader_t) + tmpSubMessageHeader.subMessageLength;
                message->length += sizeof(SubMessageHeader_t) + tmpSubMessageHeader.subMessageLength;
                memcpy(message->buffer, plainData.buffer, plainData.buffer_size);
                delete[]plainData.buffer;
                continue;
            }
            case DATAREADER_SUBMESSAGE:
            {
                CryptoReaderSubmessageAttributes readerEncodeAttr;
                if (!participantImpl_->secPlugin_.crypto_transform->get_datareader_submessage_encode_attributes(
                    SEC_NIL_HANDLE, &readerEncodeAttr))
                {
					TRAVODDS_LOG(LOG_ERROR, "get_datareader_submessage_encode_attributes failed.");
                    break;
                }
                if (!participantImpl_->secPlugin_.crypto_transform->decode_datareader_submessage(
                    &plainData,
                    &encodedData,
                    1,
                    writerCryptoHdl,
                    readerCryptoHdl,
                    &secExce))
                {
					delete[]plainData.buffer;
                    return -2;
                }
                procingSecSubmsg = false; //表示此次处理安全子消息结束

                SubMessageHeader_t tmpSubMessageHeader;
                memcpy(&tmpSubMessageHeader, plainData.buffer, sizeof(SubMessageHeader_t));

                //message减去SEC_POSTFIX
                message->buffer += sizeof(SubMessageHeader_t) + subMessageHeader.subMessageLength;
                message->length -= sizeof(SubMessageHeader_t) + subMessageHeader.subMessageLength;

                //拷贝plainData代表的子消息缓存到message
                message->buffer -= sizeof(SubMessageHeader_t) + tmpSubMessageHeader.subMessageLength;
                message->length += sizeof(SubMessageHeader_t) + tmpSubMessageHeader.subMessageLength;
                memcpy(message->buffer, plainData.buffer, plainData.buffer_size);
                delete[]plainData.buffer;
                continue;
            }
            default:
            {
                break;
            }
            //procingSecSubmsg = false;
            }
            break;
        }
        case SEC_PREFIX:
        {
            if (!participantImpl_->enableDDSSec_)
            {
                break;
            }
            //累计SEC_PREFIX子消息长度到安全消息长度
            secDataLength = (subMessageHeader.subMessageLength + sizeof(SubMessageHeader_t));
            procingSecSubmsg = true; //表明正在处理安全子消息
            break;
        }
        //解析RTPS报文的前缀
        case SRTPS_PREFIX:
        {
            //decode_rtps_message
            if (!participantImpl_->enableDDSSec_
                || !participantImpl_->secAttributes_.is_rtps_protected
                || senderCryptoHdl == SEC_NIL_HANDLE
                || senderCryptoHdl == NULL)
            {
                return ret;
                // not supporting security or not ready
                break;
            }
            SerializedBuffer plainData;
            SerializedBuffer encodedData; //指向message即需要被解密的数据

            plainData.buffer_size = message->length + 28;
            plainData.buffer = new char[message->length + 28];

            // 需要带上消息头解析
            encodedData.buffer = message->buffer - sizeof(MessageHeader_t);
            encodedData.buffer_size = message->length + 28;
            if (!participantImpl_->secPlugin_.crypto_transform->decode_rtps_message(
                &plainData,
                &encodedData,
                1,
                receiverCryptoHdl,
                senderCryptoHdl,
                &secExce))
            {
				delete[]plainData.buffer;
                return -2;
            }

            //将plainData拷贝到message，然后再遍历解析已经解析好的rtps报文的子报文
            //当前message指向rtps头部之后, plainData缓冲区拷贝到新的rtps起始位置
            memcpy(message->buffer - sizeof(MessageHeader_t) + message->length - plainData.buffer_size,
                plainData.buffer,
                plainData.buffer_size);
            //指向新的rtps header之后区域
            message->buffer = message->buffer + message->length - plainData.buffer_size;
            message->length = plainData.buffer_size - sizeof(MessageHeader_t);
            continue;
        }
        case SRTPS_POSTFIX:
            //ret = InterpretSRTPSPOSTFIXMessage(message, subMessageHeader, messageReceiver);
            break;
#endif // DDSSECURITY
			default:
				ret = RETCODE_SUBMESSAGE_INVALID;
				break;
		}

		message->buffer += sizeof(SubMessageHeader_t) + subMessageHeader.subMessageLength;
		message->length -= sizeof(SubMessageHeader_t) + subMessageHeader.subMessageLength;
	} while (message->length > 0);

	return ret;
}


/************************************************************
** 功能描述：解析Data子报文 指针地址不同步修改，故不使用传引用
*************************************************************/
ReturnCode_t MessageProcessor::InterpretDataMessage(std::shared_ptr<Message_t> message,
	SubMessageHeader_t & submessageHeader,
	Receive_t & messageReceiver)
{
	uint32_t messageShift = 0;
	/* 解析 flags 每一位 */
	bool endiannessFlag = (submessageHeader.flags & SubMessageFlag::ENDIANNESS_FLAG) != SubMessageFlag::NONE_FLAG;
	bool inlineQosFlag = (submessageHeader.flags & SubMessageFlag::INLINE_QOS_FLAG) != SubMessageFlag::NONE_FLAG;
	bool dataFlag = (submessageHeader.flags & SubMessageFlag::DATA_FLAG) != SubMessageFlag::NONE_FLAG;
	bool keyFlag = (submessageHeader.flags & SubMessageFlag::KEY_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool nonStandardPayloadFlag = (submessageHeader.flags & SubMessageFlag::NON_STANDARD_PAYLOAD_FLAG) != SubMessageFlag::NONE_FLAG;

	if (keyFlag && dataFlag) {
		TRAVODDS_LOG(LOG_ERROR, "Message received with Data and Key Flag set, ignoring");
		return RETCODE_ERROR;
	}

	DataSubMessage_t dataSubMessage;

	//messageReceiver.nonStandardPayloadFlag = nonStandardPayloadFlag;

	memcpy(&dataSubMessage.header, message->buffer + messageShift, sizeof(SubMessageHeader_t));
	messageShift += sizeof(SubMessageHeader_t);
	memcpy(&dataSubMessage.extraFlags, message->buffer + messageShift, sizeof(SubMessageFlag_t) * 2 );
	messageShift += sizeof(SubMessageFlag_t) * 2 ;
	memcpy(&dataSubMessage.octetsToInlineQos, message->buffer + messageShift, sizeof(uint16_t));
	messageShift += sizeof(uint16_t);
	memcpy(&dataSubMessage.readerId, message->buffer + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&dataSubMessage.writerId, message->buffer + messageShift, sizeof(EntityId_t));
	messageShift += sizeof(EntityId_t);
	memcpy(&dataSubMessage.writerSN, message->buffer + messageShift, sizeof(SequenceNumber_t));
	messageShift += sizeof(SequenceNumber_t);

	/* 判断报文字节序 与本地字节序不一致需要进行转换 */
	if (IS_LITTLE_ENDIAN != endiannessFlag) {
		dataSubMessage.header.subMessageLength = ntohs(dataSubMessage.header.subMessageLength);
		dataSubMessage.octetsToInlineQos = ntohs(dataSubMessage.octetsToInlineQos);
		dataSubMessage.writerSN.high = ntohl(dataSubMessage.writerSN.high);
		dataSubMessage.writerSN.low = ntohl(dataSubMessage.writerSN.low);
	}
	//TODO 
	if (dataSubMessage.writerSN <= SEQUENCENUMBER_ZERO) {
		TRAVODDS_LOG(LOG_ERROR, "Invalid message received, bad sequence Number");
		return RETCODE_ERROR;
	}

	/*  判断是否存在inlineQos */
	if (inlineQosFlag) {
		InterpretInlineQosMessage(message->buffer, messageShift, dataSubMessage.inlineQos, endiannessFlag);
	}

	/* 判断是否有Serialized Data数据 */
	if (dataFlag || keyFlag) {
		dataSubMessage.serializedPayload.data = message->buffer + messageShift;
		/* 报文长度 */
		dataSubMessage.serializedPayload.length = dataSubMessage.header.subMessageLength + sizeof(SubMessageHeader_t) - messageShift;
		dataSubMessage.serializedPayload.originMessage = message;
		messageShift += dataSubMessage.serializedPayload.length;
	}

	std::list<EntityId_t> readerList;

	if(ENTITYID_UNKNOWN == dataSubMessage.readerId) {
		//TODO 添加处理
		GUID_t guid(messageReceiver.sourceGuidPrefix, dataSubMessage.writerId);
		participant_->GetMacthList(guid, readerList);
	} else {
		readerList.push_back(dataSubMessage.readerId);
	}
	for (const auto& entittyId : readerList) {
		std::shared_ptr<Reader> reader = participant_->GetReader(entittyId);
		if (reader) {
			reader->ReceiveDataMessage(messageReceiver, dataSubMessage, dataSubMessage.serializedPayload);
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
	bool endiannessFlag = (submessageHeader.flags & SubMessageFlag::ENDIANNESS_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool finalFlag = (submessageHeader.flags & SubMessageFlag::FINAL_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool livelinessFlag = (submessageHeader.flags & SubMessageFlag::LIVELINESS_FLAG) != SubMessageFlag::NONE_FLAG;
	bool groupInfoFlag = (submessageHeader.flags & SubMessageFlag::HB_GROUPINFO_FLAG) != SubMessageFlag::NONE_FLAG;

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
	if (IS_LITTLE_ENDIAN != endiannessFlag) {
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

	/* wangyi  Cyclone DDS 适配 */
	std::list<EntityId_t> readerList;
	if (ENTITYID_UNKNOWN == heartBeatSubMessage.readerId) {
		//TODO 添加处理
		GUID_t guid(messageReceiver.sourceGuidPrefix, heartBeatSubMessage.writerId);
		participant_->GetMacthList(guid, readerList);
	}
	else {
		readerList.push_back(heartBeatSubMessage.readerId);
	}
	for (auto entittyId : readerList) {
		std::shared_ptr<Reader> reader = participant_->GetReader(entittyId);
		if (reader) {
			reader->ReceiveHeartbeatMessage(messageReceiver, heartBeatSubMessage);
		}
	}
	//std::shared_ptr<Reader> reader = participant_->GetReader(heartBeatSubMessage.readerId);
	//if (reader) {
	//	reader->ReceiveHeartbeatMessage(messageReceiver, heartBeatSubMessage);
	//}
		
	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretHeartbeatFragMessage(char* message,
	SubMessageHeader_t& submessageHeader, 
	Receive_t& messageReceiver)
{
	static_cast<void>(submessageHeader);
	HeartBeatFragSubMessage_t heartBeatFragSubMessage;
	memcpy(&heartBeatFragSubMessage, message, sizeof(HeartBeatFragSubMessage_t));

	GUID_t writerGUID;
	GUID_t readerGUID;
	readerGUID.prefix = messageReceiver.destGuidPrefix;
	readerGUID.entityId = heartBeatFragSubMessage.readerId;
	writerGUID.prefix = messageReceiver.sourceGuidPrefix;
	writerGUID.entityId = heartBeatFragSubMessage.writerId;

	/* JDXCZ 大小端转换-收到的大小端信息和本地不同进行转换 */
	if (DEFAULT_ENDIAN != (heartBeatFragSubMessage.header.flags & SubMessageFlag::ENDIANNESS_FLAG)) {
		heartBeatFragSubMessage.header.subMessageLength = htons(heartBeatFragSubMessage.header.subMessageLength);
		heartBeatFragSubMessage.count = htonl(heartBeatFragSubMessage.count);
		heartBeatFragSubMessage.writerSN.high = htonl(heartBeatFragSubMessage.writerSN.high);
		heartBeatFragSubMessage.writerSN.low = htonl(heartBeatFragSubMessage.writerSN.low);
	}


	/* wangyi  Cyclone DDS 适配 */
	std::list<EntityId_t> readerList;
	if (ENTITYID_UNKNOWN == heartBeatFragSubMessage.readerId) {
		//TODO 添加处理
		GUID_t guid(messageReceiver.sourceGuidPrefix, heartBeatFragSubMessage.writerId);
		participant_->GetMacthList(guid, readerList);
	}
	else {
		readerList.push_back(heartBeatFragSubMessage.readerId);
	}
	for (auto entittyId : readerList) {
		std::shared_ptr<Reader> reader = participant_->GetReader(entittyId);
		if (reader) {
			reader->ReceiveHeartFragMessage(messageReceiver, heartBeatFragSubMessage);
		}
	}
	//std::shared_ptr<Reader> reader = participant_->GetReader(heartBeatFragSubMessage.readerId);
	//if (reader) {
	//	reader->ReceiveHeartFragMessage(messageReceiver, heartBeatFragSubMessage);
	//}

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretAckNackMessage(char* message, SubMessageHeader_t& submessageHeader, Receive_t& messageReceiver)
{
	static_cast<void>(submessageHeader);
	// bool endiannessFlag = (submessageHeader.flags & SubMessageFlag::ENDIANNESS_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool finalFlag = (submessageHeader.flags & SubMessageFlag::FINAL_FLAG) != SubMessageFlag::NONE_FLAG;
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
	memcpy(&ackMessage.readerSNState.bitmap.size, message + messageShift, sizeof(uint32_t));
	messageShift += sizeof(uint32_t);

	if (DEFAULT_ENDIAN != (ackMessage.header.flags & SubMessageFlag::ENDIANNESS_FLAG)) {
		ackMessage.header.subMessageLength = ntohs(ackMessage.header.subMessageLength);
		ackMessage.count = ntohs(ackMessage.count);
		ackMessage.readerSNState.bitmapBase.high = ntohl(ackMessage.readerSNState.bitmapBase.high);
		ackMessage.readerSNState.bitmapBase.low = ntohl(ackMessage.readerSNState.bitmapBase.low);
		ackMessage.readerSNState.bitmap.size = ntohl(ackMessage.readerSNState.bitmap.size);
	}

	if (0 != ackMessage.readerSNState.bitmap.size) {
		uint32_t uilen = ackMessage.readerSNState.bitmap.GetByteSize();
		memcpy(ackMessage.readerSNState.bitmap.bits, message + messageShift, uilen);
		messageShift += uilen;
		/* 大小端转换 */
		if (DEFAULT_ENDIAN != (ackMessage.header.flags & SubMessageFlag::ENDIANNESS_FLAG)) {
			for (uint32_t i = 0; i < uilen; i++) {
				ackMessage.readerSNState.bitmap.bits[i] = ntohl(ackMessage.readerSNState.bitmap.bits[i]);
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

ReturnCode_t MessageProcessor::InterpretDataFragMessage(std::shared_ptr<Message_t> message,
	SubMessageHeader_t& submessageHeader,
	Receive_t& messageReceiver)
{
	uint32_t messageShift = 0;
	uint32_t serializedPayloadLen = 0;
	bool endiannessFlag = (submessageHeader.flags & SubMessageFlag::ENDIANNESS_FLAG) != SubMessageFlag::NONE_FLAG;
	bool inlineQosFlag = (submessageHeader.flags & SubMessageFlag::INLINE_QOS_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool nonStandardPayloadFlag = (submessageHeader.flags & SubMessageFlag::FRAG_NON_STANDARD_PAYLOAD_FLAG) != SubMessageFlag::NONE_FLAG;
	bool keyFlag = (submessageHeader.flags & SubMessageFlag::FRAG_KEY_FLAG) != SubMessageFlag::NONE_FLAG;
	

	DataFragSubMessage_t dataFragSubMessage;

	//messageReceiver.nonStandardPayloadFlag = nonStandardPayloadFlag;

	//memcpy(&dataFragSubMessage.header, message + messageShift, sizeof(SubMessageHeader_t));
	//messageShift += sizeof(SubMessageHeader_t);
	//memcpy(&dataFragSubMessage.extraFlags, message + messageShift, sizeof(SubMessageFlag_t) * 2);
	//messageShift += sizeof(SubMessageFlag_t) * 2;
	//memcpy(&dataFragSubMessage.octetsToInlineQos, message + messageShift, sizeof(uint16_t));
	//messageShift += sizeof(uint16_t);
	//memcpy(&dataFragSubMessage.readerId, message + messageShift, sizeof(EntityId_t));
	//messageShift += sizeof(EntityId_t);
	//memcpy(&dataFragSubMessage.writerId, message + messageShift, sizeof(EntityId_t));
	//messageShift += sizeof(EntityId_t);
	//memcpy(&dataFragSubMessage.writerSN, message + messageShift, sizeof(SequenceNumber_t));
	//messageShift += sizeof(SequenceNumber_t);
	
	// 对于结构字节 4、2、2、4、4、8的顺序，32位或64位系统下，内存对齐无padding直接使用 memcpy
	memcpy(&dataFragSubMessage, message->buffer, sizeof(SubMessageHeader_t) + sizeof(SubMessageFlag_t) * 2 + sizeof(uint16_t) + sizeof(EntityId_t) * 2 + sizeof(SequenceNumber_t));
	messageShift = sizeof(SubMessageHeader_t) + sizeof(SubMessageFlag_t) * 2 + sizeof(uint16_t) + sizeof(EntityId_t) * 2 + sizeof(SequenceNumber_t);

	/* 判断报文字节序 与本地字节序不一致需要进行转换 */
	if (DEFAULT_ENDIAN != (dataFragSubMessage.header.flags & SubMessageFlag::ENDIANNESS_FLAG)) {
		dataFragSubMessage.header.subMessageLength = ntohs(dataFragSubMessage.header.subMessageLength);
		dataFragSubMessage.octetsToInlineQos = ntohs(dataFragSubMessage.octetsToInlineQos);
		dataFragSubMessage.writerSN.high = ntohl(dataFragSubMessage.writerSN.high);
		dataFragSubMessage.writerSN.low = ntohl(dataFragSubMessage.writerSN.low);
	}

	if (dataFragSubMessage.writerSN <= SequenceNumber_t()) {
		TRAVODDS_LOG(LOG_ERROR, "Invalid message received, bad sequence Number");
		return RETCODE_ERROR;
	}

	memcpy(&dataFragSubMessage.dataFrag, message->buffer + messageShift, sizeof(DataFrag_t));
	messageShift += sizeof(DataFrag_t);
	/* 大小端转换 */
	if (IS_LITTLE_ENDIAN != endiannessFlag) {
		dataFragSubMessage.dataFrag.fragmentStartingNum		= htonl(dataFragSubMessage.dataFrag.fragmentStartingNum);
		dataFragSubMessage.dataFrag.sampleSize				= htonl(dataFragSubMessage.dataFrag.sampleSize);
		dataFragSubMessage.dataFrag.fragmentsInSubmessage	= htons(dataFragSubMessage.dataFrag.fragmentsInSubmessage);
		dataFragSubMessage.dataFrag.fragmentSize			= htons(dataFragSubMessage.dataFrag.fragmentSize);
	}

	/* 判断是否存在inlineQos*/
	if (inlineQosFlag) {
		InterpretInlineQosMessage(message->buffer, messageShift, dataFragSubMessage.inlineQos, endiannessFlag);
	}

	if (!keyFlag) {
		dataFragSubMessage.serializedPayload.data = message->buffer + messageShift;
		/* 报文长度 */
		serializedPayloadLen = dataFragSubMessage.header.subMessageLength + sizeof(SubMessageHeader_t) - messageShift;
		dataFragSubMessage.serializedPayload.length = serializedPayloadLen;
		dataFragSubMessage.serializedPayload.originMessage = message;

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
	for (const auto& entittyId : readerList) {
		std::shared_ptr<Reader> reader = participant_->GetReader(entittyId);
		if (reader) {
			reader->ReceiveDataFragMessage(messageReceiver, dataFragSubMessage, dataFragSubMessage.serializedPayload);
		}
	}

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretINFOTSMessage(char* message, SubMessageHeader_t& submessageHeader, Receive_t& messageReceiver)
{
	static_cast<void>(submessageHeader);
	InfoTimestamp_t infoTs;

	memcpy(&infoTs, message, sizeof(InfoTimestamp_t));
	message += sizeof(InfoTimestamp_t);

	bool invalidateFlag = (infoTs.header.flags & SubMessageFlag::INVALIDATE_FLAG) != SubMessageFlag::NONE_FLAG;

	if (invalidateFlag) {
		messageReceiver.timestamp = infoTs.timestamp;
	}

	return RETCODE_OK;
}

ReturnCode_t MessageProcessor::InterpretINFOSRCMessage(char* message, SubMessageHeader_t& submessageHeader, Receive_t& messageReceiver)
{
	static_cast<void>(submessageHeader);
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
	static_cast<void>(submessageHeader);
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
		if (IS_LITTLE_ENDIAN != endiannessFlag) {
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
	// bool endiannessFlag = (submessageHeader.flags & SubMessageFlag::ENDIANNESS_FLAG) != SubMessageFlag::NONE_FLAG;
	// bool finalFlag = (submessageHeader.flags & SubMessageFlag::FINAL_FLAG) != SubMessageFlag::NONE_FLAG;
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
	memcpy(&nackFragMessage.fragmentNumberState.bitmap.size, message + messageShift, sizeof(uint32_t));
	messageShift += sizeof(uint32_t);

	/* 大小端转换 */
	if (DEFAULT_ENDIAN != (submessageHeader.flags & SubMessageFlag::ENDIANNESS_FLAG)) {
		nackFragMessage.header.subMessageLength = htons(submessageHeader.subMessageLength);
		nackFragMessage.count = htonl(nackFragMessage.count);
		nackFragMessage.writerSN.high = htonl(nackFragMessage.writerSN.high);
		nackFragMessage.writerSN.low = htonl(nackFragMessage.writerSN.low);
		nackFragMessage.fragmentNumberState.bitmapBase = htonl(nackFragMessage.fragmentNumberState.bitmapBase);
		nackFragMessage.fragmentNumberState.bitmap.size = htonl(nackFragMessage.fragmentNumberState.bitmap.size);
	}

	if (0 != nackFragMessage.fragmentNumberState.bitmap.size) {
		uint32_t uilen = nackFragMessage.fragmentNumberState.bitmap.GetByteSize();
		memcpy(nackFragMessage.fragmentNumberState.bitmap.bits, message + messageShift, uilen);
		messageShift += uilen;
		/* 大小端转换 */
		if (DEFAULT_ENDIAN != (nackFragMessage.header.flags & SubMessageFlag::ENDIANNESS_FLAG)) {
			for (uint32_t i = 0; i < uilen; i++) {
				nackFragMessage.fragmentNumberState.bitmap.bits[i] = ntohl(nackFragMessage.fragmentNumberState.bitmap.bits[i]);
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
	static_cast<void>(messageReceiver);
	uint32_t messageLen = 0;
	InfoReplySubMessage_t infoReplyMessage;
	infoReplyMessage.header = submessageHeader;
	messageLen += sizeof(SubMessageHeader_t);

	bool endiannessFlag = (submessageHeader.flags & SubMessageFlag::ENDIANNESS_FLAG) != SubMessageFlag::NONE_FLAG;
	bool multicastFlag = (submessageHeader.flags & SubMessageFlag::MULTICAST_FLAG) != SubMessageFlag::NONE_FLAG;

	uint32_t iSize = 0;
	memcpy(&iSize, message + messageLen, sizeof(uint32_t));
	messageLen += sizeof(uint32_t);
	/* 大小端转换 */
	if (IS_LITTLE_ENDIAN != endiannessFlag) {
		iSize = htonl(iSize);
	}
	infoReplyMessage.unicastLocatorList.resize(iSize);
	for (uint32_t i = 0; i < iSize; i++) {
		memcpy(&infoReplyMessage.unicastLocatorList[i].kind, message + messageLen, sizeof(int32_t));
		memcpy(&infoReplyMessage.unicastLocatorList[i].port, message + messageLen + 4u, sizeof(uint32_t));
		memcpy(&infoReplyMessage.unicastLocatorList[i].address, message + messageLen + 8u, 16u);
		messageLen += 24u;
		/* 大小端转换 */
		if (IS_LITTLE_ENDIAN != endiannessFlag) {
			infoReplyMessage.unicastLocatorList[i].kind = htonl(infoReplyMessage.unicastLocatorList[i].kind);
			infoReplyMessage.unicastLocatorList[i].port = htonl(infoReplyMessage.unicastLocatorList[i].port);
		}
	}

	if (multicastFlag) {
		memcpy(&iSize, message + messageLen, sizeof(uint32_t));
		messageLen += sizeof(uint32_t);
		/* 大小端转换 */
		if (IS_LITTLE_ENDIAN != endiannessFlag) {
			iSize = htonl(iSize);
		}
		infoReplyMessage.multicastLocatorList.resize(iSize);
		for (uint32_t i = 0; i < iSize; i++) {
			memcpy(&infoReplyMessage.multicastLocatorList[i].kind, message + messageLen, sizeof(int32_t));
			memcpy(&infoReplyMessage.multicastLocatorList[i].port, message + messageLen + 4u, sizeof(uint32_t));
			memcpy(&infoReplyMessage.multicastLocatorList[i].address, message + messageLen + 8u, 16u);
			messageLen += 24u;
			/* 大小端转换 */
			if (IS_LITTLE_ENDIAN != endiannessFlag) {
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

	bool endiannessFlag = (submessageHeader.flags & SubMessageFlag::ENDIANNESS_FLAG) != SubMessageFlag::NONE_FLAG;
	bool groupInfoFlag = (submessageHeader.flags & SubMessageFlag::GROUPINFO_FLAG) != SubMessageFlag::NONE_FLAG;
	bool filteredCountFlag = (submessageHeader.flags & SubMessageFlag::FILTERED_COUNT_FLAG) != SubMessageFlag::NONE_FLAG;

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
	memcpy(&gapSubMessage.gapList.bitmap.size, message + messageShift, sizeof(uint32_t));
	messageShift += sizeof(uint32_t);

	/* 大小端转换 */
	if (IS_LITTLE_ENDIAN != endiannessFlag) {
		gapSubMessage.header.subMessageLength = ntohs(gapSubMessage.header.subMessageLength);

		gapSubMessage.gapStart.low = ntohl(gapSubMessage.gapStart.low);
		gapSubMessage.gapStart.high = ntohl(gapSubMessage.gapStart.high);
		gapSubMessage.gapList.bitmapBase.low = ntohl(gapSubMessage.gapList.bitmapBase.low);
		gapSubMessage.gapList.bitmapBase.high = ntohl(gapSubMessage.gapList.bitmapBase.high);
	}
	if (0 != gapSubMessage.gapList.bitmap.size) {
		uint32_t uilen = gapSubMessage.gapList.bitmap.GetByteSize();
		memcpy(gapSubMessage.gapList.bitmap.bits, message + messageShift, uilen);
		messageShift += uilen;
		/* 大小端转换 */
		if (DEFAULT_ENDIAN != (gapSubMessage.header.flags & SubMessageFlag::ENDIANNESS_FLAG)) {
			for (uint32_t i = 0; i < uilen; i++) {
				gapSubMessage.gapList.bitmap.bits[i] = ntohl(gapSubMessage.gapList.bitmap.bits[i]);
			}
		}
	}

	if (groupInfoFlag) {
		memcpy(&gapSubMessage.gapStartGSN, message + messageShift, sizeof(SequenceNumber_t));
		messageShift += sizeof(SequenceNumber_t);
		memcpy(&gapSubMessage.gapEndGSN, message + messageShift, sizeof(SequenceNumber_t));
		messageShift += sizeof(SequenceNumber_t);
		/* 大小端转换 */
		if (IS_LITTLE_ENDIAN != endiannessFlag) {
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
		if (IS_LITTLE_ENDIAN != endiannessFlag) {
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

#pragma GCC diagnostic pop