#include "rtps/endpoint/statelesswriter.h"
#include "rtps/endpoint/participant.h"
#include "rtps/messages/messageprocessor.h"
#include "rtps/transport/messagetransmitter.h"
#include "common/time/ddstimer.h"
#include <rtps/builtin/liveliness/livelinessmanager.h>
#include "common/log/logger.h"

USING_AGIDDS_NAMESPACE;


StatelessWriter::StatelessWriter(const DataWriterQos& qos, const GUID_t& guid, const TopicKind_t& kind)
	: Writer(qos, kind), timer_(nullptr), resendDataPeriod_(TIME_INFINITE)
{
	guid_ = guid;
}

StatelessWriter::~StatelessWriter()
{

}
void StatelessWriter::MatchedReaderAdd(const DiscoveredReaderData &subData)
{
	for (auto locator : subData.unicastLocatorList) {
		ReaderLocatorAdd(locator);
	}

	for (auto locator : subData.multicastLocatorList) {
		ReaderLocatorAdd(locator);
	}
}
void StatelessWriter::ReaderLocatorAdd(Locator_t & locator)
{
	if (0 != readerLocators_.count(locator)) {
		return;
	}

	std::shared_ptr<ReaderLocator> readerLocator = std::make_shared<ReaderLocator>(locator);
	readerLocator->SetHistoryCache(writerCache_);
	readerLocators_[locator] = readerLocator;
}

void StatelessWriter::ReaderLocatorRemove(Locator_t & locator)
{
	if (0 == readerLocators_.count(locator)) {
		return;
	}

	//delete  readerLocators_[locator];
	//readerLocators_[locator] = nullptr;

	readerLocators_.erase(locator);
}

void StatelessWriter::UnsentChangesReset()
{
	for (auto& itor : readerLocators_) {
		itor.second->ResetHighestSentChange();
	}
}

ReturnCode_t StatelessWriter::DeliveryData(std::shared_ptr<CacheChange> change) 
{
	/* wnagyi  */
	Duration_t duration = TIME_INFINITE;
	if (qos_.liveliness.lease_duration < duration) {
		auto livelinessManager = participant_->GetLivelinessManager();
		if (livelinessManager != nullptr) {
			livelinessManager->GetLocalWriterLivelinessManager()->AssertLiveliness(GetGuid(),qos_.liveliness.kind);
		}
		else {
			/* TODO 日志 */
			AGILOG(AGILOG_ERROR, "livelinessManager is nullptr")
		}
	}

	if (0 == readerLocators_.size()) {
		return RETCODE_OK;
	}

	//TODO 发送报文
	std::shared_ptr<MessageTransmitter> messageTransmitter = participant_->GetMessageTransmitter();

	LocatorVec srcLocators;
	Locator_t srcLocator;
	srcLocator.kind = LOCATOR_KIND_UDPv4;
	srcLocators.push_back(srcLocator);
	//srcLocators = participant_->GetDefaultUnicastLocators();
	//TODO 封装报文


	SerializedBuffer* buffer = change->serializedBuffer;
	if (buffer->next) {
		DataFragSubMessage_t dataFragSubMessage;

		if (0 != change->inlineQos.size())
		{
			dataFragSubMessage.header.flags |= SubMessageFlag::INLINE_QOS_FLAG;
			dataFragSubMessage.inlineQos = change->inlineQos;
		}

		dataFragSubMessage.header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

		/* wangyi fastdds 适配测试默认16 */
		dataFragSubMessage.octetsToInlineQos = OCTETS_TO_INLINEQOS_LEN;
		dataFragSubMessage.extraFlags[0] = SubMessageFlag::NONE_FLAG;
		dataFragSubMessage.extraFlags[1] = SubMessageFlag::NONE_FLAG;
		dataFragSubMessage.readerId = ENTITYID_UNKNOWN;
		dataFragSubMessage.writerId = guid_.entityId;
		dataFragSubMessage.writerSN = change->sequenceNumber;
		dataFragSubMessage.dataFrag.fragmentStartingNum = 0;
		do {
			/* 分片报文处理 */
			uint32_t shift = 0;
			dataFragSubMessage.dataFrag.fragmentsInSubmessage = 1;
			dataFragSubMessage.dataFrag.fragmentSize = DATAMESSAGE_LIMIT;
			dataFragSubMessage.dataFrag.fragmentStartingNum += 1;
			dataFragSubMessage.dataFrag.sampleSize = change->dataSize;

			MessageProcessor::EncapsulateDataFragMessage(*buffer, shift, dataFragSubMessage);
			MessageProcessor::EncapsulateMessageHeader(*buffer, shift, guid_.prefix);			

			/* 发送数据 */
			for (auto& readerlocator : readerLocators_) {
				LocatorVec dstLocators;
				dstLocators.push_back(readerlocator.first);
				messageTransmitter->MessageSend(srcLocators, dstLocators, buffer->buffer - shift, buffer->buffer_size + shift);
			}

			buffer = buffer->next;

		} while (buffer);
	}
	else {

		DataSubMessage_t dataSubMessage;
		if (0 != change->serializedBuffer->buffer_size) {
			dataSubMessage.header.flags |= SubMessageFlag::DATA_FLAG;
		}

		if (0 != change->inlineQos.size())
		{
			dataSubMessage.header.flags |= SubMessageFlag::INLINE_QOS_FLAG;
			dataSubMessage.inlineQos = change->inlineQos;
		}

		dataSubMessage.header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

		/* wangyi fastdds 适配测试默认16 */
		dataSubMessage.octetsToInlineQos = OCTETS_TO_INLINEQOS_LEN;
		dataSubMessage.extraFlags[0] = SubMessageFlag::NONE_FLAG;
		dataSubMessage.extraFlags[1] = SubMessageFlag::NONE_FLAG;
		dataSubMessage.readerId = ENTITYID_UNKNOWN;
		dataSubMessage.writerId = guid_.entityId;
		dataSubMessage.writerSN = change->sequenceNumber;

		uint32_t shift = 0;

		MessageProcessor::EncapsulateDATAMessage(*change->serializedBuffer, shift, dataSubMessage);
		MessageProcessor::EncapsulateMessageHeader(*change->serializedBuffer, shift, guid_.prefix);

		for (auto& readerlocator : readerLocators_) {
			LocatorVec dstLocators;
			dstLocators.push_back(readerlocator.first);
			messageTransmitter->MessageSend(srcLocators, dstLocators, change->serializedBuffer->buffer - shift, change->serializedBuffer->buffer_size + shift);
		}
			
	}

	return RETCODE_OK;
}

void StatelessWriter::SetResendDataPeriod(Duration_t period)
{
	resendDataPeriod_ = period;

	Time_t time;
	time.sec = period.sec;
	time.nanosec = period.nanosec;
	if (time != Time_t(TIME_INFINITE)) {
		std::string n = "slw" + std::to_string(guid_.entityId.entityKind);

		timer_ = TimerManager::CreateTimer(time, [&]() { this->ResendDataPeriod(); }, n);
	}
}

void StatelessWriter::ResendDataPeriod()
{
	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();
	SequenceNumber_t maxSeq = writerCache_->GetSeqNumMax();

	for (; minSeq <= maxSeq; minSeq++) {
		std::shared_ptr<CacheChange> change = writerCache_->GetChange(minSeq);
		if (!change) {
			continue;
		}
		DeliveryData(change);
	}
}


ReturnCode_t StatelessWriter::ReceiveAckNackMessage(const Receive_t&  headerMsg, const AcknackSubMessage_t& ackMsg)
{
	static_cast<void>(headerMsg);
	static_cast<void>(ackMsg);

	return RETCODE_UNSUPPORTED;
}

ReturnCode_t StatelessWriter::ReceiveNackFragMessage(const Receive_t&  headerMsg, const NackFragSubMessage_t& nackMsg)
{
	static_cast<void>(headerMsg);
	static_cast<void>(nackMsg);

	return RETCODE_UNSUPPORTED;
}
