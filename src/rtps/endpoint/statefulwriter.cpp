#include "rtps/endpoint/statefulwriter.h"
#include "rtps/endpoint/participant.h"
#include "rtps/messages/messageprocessor.h"
#include "rtps/transport/messagetransmitter.h"
#include "common/time/ddstimer.h"
#include "common/threads/threadpool.h"
#include "dcps/publish/datawriterimpl.h"
#include <rtps/builtin/liveliness/livelinessmanager.h>
#include "common/log/logger.h"
USING_AGIDDS_NAMESPACE;

StatefulWriter::StatefulWriter(const DataWriterQos& qos, const GUID_t& guid, const TopicKind_t& kind)
	: Writer(qos, kind)
{
	guid_ = guid;

	Time_t heartBeatPeriod = { 0, 200 * 1000 * 1000 };
	std::string n = "sfw" + std::to_string(guid.entityId.entityKind);
	timer_ = TimerManager::CreateTimer(heartBeatPeriod, [&]() { this->WriteHeartbeat(); }, n);
	//timer_->Start();
}

StatefulWriter::~StatefulWriter() {

}
void StatefulWriter::MatchedReaderAdd(const DiscoveredReaderData &subData)
{
	bool active = subData.ddsSubscriptionData.reliability.kind == BEST_EFFORT_RELIABILITY_QOS ? false : true;
	std::shared_ptr<ReaderProxy> proxy = std::make_shared<ReaderProxy>(subData.remoteReaderGuid, subData.remoteGroupGuid, subData.unicastLocatorList, subData.multicastLocatorList, active);
	
	SequenceNumber_t maxSeq = writerCache_->GetSeqNumMax();
	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();

	if (maxSeq != SEQUENCENUMBER_UNKNOWN) {
		if (TRANSIENT_LOCAL_DURABILITY_QOS == subData.ddsSubscriptionData.durability.kind && TRANSIENT_LOCAL_DURABILITY_QOS == qos_.durability.kind) {
			proxy->InitChangeForReader(minSeq);
		}
		else {
			proxy->InitChangeForReader(maxSeq + 1);
		}
	}

	MatchedReaderAdd(proxy);

}
void StatefulWriter::MatchedReaderAdd(std::shared_ptr<ReaderProxy> readerProxy)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	readerProxy->SetHistoryCache(writerCache_);
	matchedReaders_[readerProxy->GetGuid()] = readerProxy;

	//participant_->GetThreadPool()->submit(
	//	[=]() {
	//		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//		WriteHeartbeat();
	//	});
}

void StatefulWriter::MatchedReaderRemove(std::shared_ptr<ReaderProxy> readerProxy)
{

	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	matchedReaders_.erase(readerProxy->GetGuid());
}

std::shared_ptr<ReaderProxy> StatefulWriter::MatchedReaderLookup(GUID_t readerGuid)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	return matchedReaders_[readerGuid];
}

bool StatefulWriter::WaitForAcknowledgments(const Duration_t & max_wait)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	SequenceNumber_t sequenceNumber = writerCache_->GetSeqNumMax();
	//TODO 超时机制待实现
	for (auto reader : matchedReaders_) {
		if (false == reader.second->IsAcked(sequenceNumber)) {
			return false;
		}
	}
	return true;
}

bool StatefulWriter::IsAckedByAll(SequenceNumber_t changeSeqNum)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	for (auto readerProxy : matchedReaders_) {
		if (false == readerProxy.second->IsAcked(changeSeqNum)) {
			return false;
		}
	}
	return true;
}

ReturnCode_t StatefulWriter::DeliveryData(std::shared_ptr<CacheChange> change)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	/* wnagyi  */
	Duration_t duration = TIME_INFINITE;
	if (qos_.liveliness.lease_duration < duration) {
		auto livelinessManager = participant_->GetLivelinessManager();
		if (livelinessManager != nullptr) {
			livelinessManager->GetLocalWriterLivelinessManager()->AssertLiveliness(GetGuid(),qos_.liveliness.kind);
		}
		else {
			/* TODO 日志 */
			AGILOG(AGILOG_ERROR, "messageTransmitter is nullptr");
		}
	}

	std::vector<std::shared_ptr<ReaderProxy>> readerProxys;
	for (auto& readerProxy : matchedReaders_) {
		readerProxys.push_back(readerProxy.second);
	}
	DeliveryDataToReader(change, readerProxys);

	return RETCODE_OK;
}

ChangeForReader StatefulWriter::CreateChangeForReader(std::shared_ptr<CacheChange> change)
{
	ChangeForReader changeForReader;
	changeForReader.sequenceNumber = change->sequenceNumber;

	if (true == IsPushMode()) {
		changeForReader.status = UNSENT;
	} else {
		changeForReader.status = UNACKNOWLEDGED;
	}

	if (true == DataFilter(change)) {
		changeForReader.isRelevant = false;
	} else {
		changeForReader.isRelevant = true;
	}

	return changeForReader;
}


ReturnCode_t StatefulWriter::ReceiveAckNackMessage(const Receive_t& headerMsg, const AcknackSubMessage_t& ackMsg)
{
	std::unique_lock<std::mutex> lock(readerPorxyMutex_);

	std::shared_ptr<ReaderProxy> proxy = nullptr;
	GUID_t guid(headerMsg.sourceGuidPrefix, ackMsg.readerId);

	auto itor = matchedReaders_.find(guid);
	if (matchedReaders_.end() == itor) {
		AGILOG(AGILOG_ERROR, "no match reader");
		return RETCODE_ERROR;
	}
	proxy = itor->second;
	if (!proxy) {
		AGILOG(AGILOG_ERROR, "match readerproxy is nullptr");
		return RETCODE_ERROR;
	}

	ReturnCode_t ret = proxy->RecvAcknack(ackMsg);
	if (RETCODE_RECEIVE_DUPLICATE == ret) {
		// 返回RETCODE_RECEIVE_DUPLICATE，表示Ack报文接收重复，不处理
		return RETCODE_OK;
	}

	if (qos_.durability.kind != TRANSIENT_LOCAL_DURABILITY_QOS) {
		lock.unlock();
		RemoveAllAckedData();
		lock.lock();
	}

	//返回值为1
	if (RETCODE_REPEAT_REQUEXT == ret) {
		// TODO 为RETCODE_REPEAT_REQUEXT，需要重传数据
		ReSendData(proxy);
	}
	/* 需要强制回复一个HB报文 */
	if (ackMsg.header.flags & 0x02) {
		//TODO 
		WriteHeartbeat(proxy);
	}

	return RETCODE_OK;
}

ReturnCode_t StatefulWriter::ReceiveNackFragMessage(const Receive_t& headerMsg, const NackFragSubMessage_t& nackMsg)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	std::shared_ptr<ReaderProxy> proxy = nullptr;
	GUID_t guid(headerMsg.sourceGuidPrefix, nackMsg.readerId);

	auto itor = matchedReaders_.find(guid);
	if (matchedReaders_.end() == itor) {
		AGILOG(AGILOG_ERROR, "no match reader");
		return RETCODE_ERROR;
	}
	proxy = itor->second;
	if (!proxy) {
		AGILOG(AGILOG_ERROR, "match readerproxy is nullptr");
		return RETCODE_ERROR;
	}

	ReturnCode_t ret = proxy->RecvNackFrag(nackMsg);
	if (RETCODE_RECEIVE_DUPLICATE == ret) {
		// 返回RETCODE_RECEIVE_DUPLICATE，表示nack报文接收重复，不处理
		return RETCODE_OK;
	}

	ReSendDataFrag(proxy, nackMsg);

	//需要强制回复一个HBFrag报文
	if (nackMsg.header.flags & 0x02) {
		//TODO 
	}


	return RETCODE_OK;
}

ReturnCode_t StatefulWriter::DeliveryDataToReader(std::shared_ptr<CacheChange> change, const std::vector<std::shared_ptr<ReaderProxy>>& readerProxys)
{
	if (nullptr == change || 0 == readerProxys.size()) {
		return RETCODE_OK;
	}

	DataSubMessage_t dataSubMessage;
	LocatorVec srcLocators;
	Locator_t srcLocator;
	srcLocator.kind = LOCATOR_KIND_UDPv4;

	srcLocators.push_back(srcLocator);
	//TODO 发送报文
	std::shared_ptr<MessageTransmitter> messageTransmitter = participant_->GetMessageTransmitter();

	SerializedBuffer* buffer = change->serializedBuffer;
	if (buffer->next) {
		/* 分片数据处理 */
		DataFragSubMessage_t dataFragSubMessage;
		if (0 != change->serializedBuffer->buffer_size) {
			//TODO jdxcz 需要确认什么时候标志位设置BIT(3)
			dataFragSubMessage.header.flags |= BIT(2);
		}

		if (0 != change->inlineQos.size())
		{
			dataFragSubMessage.header.flags |= BIT(1);
			dataFragSubMessage.inlineQos = change->inlineQos;
		}

		dataFragSubMessage.header.flags |= BIT(0);

		dataFragSubMessage.octetsToInlineQos = 0;
		dataFragSubMessage.extraFlags[0] = 0;
		dataFragSubMessage.extraFlags[1] = 0;
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
			
			LocatorVec dstLocators;
			for (auto& reader : readerProxys) {

				LocatorSeq& locators = reader->GetUnicastLocatorList();

				dstLocators.insert(dstLocators.end(), locators.begin(), locators.end());
			}

			std::sort(dstLocators.begin(), dstLocators.end());
			dstLocators.erase(std::unique(dstLocators.begin(), dstLocators.end()), dstLocators.end());

			messageTransmitter->MessageSend(srcLocators, dstLocators, buffer->buffer - shift, buffer->buffer_size + shift);

			buffer = buffer->next;
		} while (buffer);

	}
	else {
		if (0 != change->serializedBuffer->buffer_size) {
			//TODO jdxcz 需要确认什么时候标志位设置BIT(3)
			dataSubMessage.header.flags |= BIT(2);
		}

		if (0 != change->inlineQos.size()) {
			dataSubMessage.header.flags |= BIT(1);
			dataSubMessage.inlineQos = change->inlineQos;
		}

		dataSubMessage.header.flags |= TypeSupport::IsLittleEndian();

		dataSubMessage.octetsToInlineQos = 0;
		dataSubMessage.extraFlags[0] = 0;
		dataSubMessage.extraFlags[1] = 0;
		dataSubMessage.readerId = ENTITYID_UNKNOWN;
		dataSubMessage.writerId = guid_.entityId;
		dataSubMessage.writerSN = change->sequenceNumber;

		LocatorVec srcLocators;
		Locator_t srcLocator;
		srcLocator.kind = LOCATOR_KIND_UDPv4;

		srcLocators.push_back(srcLocator);
		//TODO 发送报文
		std::shared_ptr<MessageTransmitter> messageTransmitter = participant_->GetMessageTransmitter();

		uint32_t shift = 0;

		MessageProcessor::EncapsulateDATAMessage(*change->serializedBuffer, shift, dataSubMessage);

		MessageProcessor::EncapsulateMessageHeader(*change->serializedBuffer, shift, guid_.prefix);

		//for (auto reader : readerProxys) {

		//	LocatorVec dstLocators = reader->GetUnicastLocatorList();
		//	messageTransmitter->MessageSend(srcLocators, dstLocators, change->serializedBuffer->buffer - shift, change->serializedBuffer->buffer_size + shift);
		//}

		LocatorVec dstLocators;
		for (auto& reader : readerProxys) {

			LocatorSeq& locators = reader->GetUnicastLocatorList();

			dstLocators.insert(dstLocators.end(), locators.begin(), locators.end());
		}

		std::sort(dstLocators.begin(), dstLocators.end());
		dstLocators.erase(std::unique(dstLocators.begin(), dstLocators.end()), dstLocators.end());
		messageTransmitter->MessageSend(srcLocators, dstLocators, change->serializedBuffer->buffer - shift, change->serializedBuffer->buffer_size + shift);

	}

	return RETCODE_OK;
}

ReturnCode_t StatefulWriter::WriteHeartbeat()
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);
	HeartBeatSubMessage_t hbMsg;

	for (auto& readerProxy : matchedReaders_) {
		WriteHeartbeat(readerProxy.second);
	}
	return RETCODE_OK;
}

/* wangyi 发送 liveliness 心跳 */
ReturnCode_t StatefulWriter::WriteHeartbeat(bool liveliness)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	HeartBeatSubMessage_t hbMsg;

	for (auto& writerProxy : matchedReaders_) {
		WriteHeartbeat(writerProxy.second, liveliness);
	}
	return RETCODE_OK;
}

ReturnCode_t StatefulWriter::WriteHeartbeat(std::shared_ptr<ReaderProxy> readerProxy, bool liveliness)
{
	//TODO 此函数可以发送HeartBeat报文,当时后续需要优化改进

	if (!readerProxy) {
		PRINTF_ERROR();
		AGILOG(AGILOG_ERROR, "readerProxy is nullptr");
		return RETCODE_ERROR;
	}

	// statelessreader 不发送heartbeat
	if (false == readerProxy->isActive()) {
		return RETCODE_OK;
	}
		

	HeartBeatSubMessage_t hbMsg;
	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();
	SequenceNumber_t maxSeq = writerCache_->GetSeqNumMax();
	SequenceNumber_t relateSeq = readerProxy->GetRelatedSequenceNumber();

	if (SEQUENCENUMBER_ZERO == maxSeq) {
		return RETCODE_OK;
	}

	LocatorSeq unicastList = readerProxy->GetUnicastLocatorList();
	LocatorSeq multicastList = readerProxy->GetMulticastLocatorList();

	LocatorVec srcLocators;
	Locator_t srcLocator;
	srcLocator.kind = LOCATOR_KIND_UDPv4;
	srcLocators.push_back(srcLocator);

	std::shared_ptr<MessageTransmitter> messageTransmitter = participant_->GetMessageTransmitter();

	if (!messageTransmitter) {
		PRINTF_ERROR();
		AGILOG(AGILOG_ERROR, "messageTransmitter is nullptr");
		return RETCODE_ERROR;
	}
	char data[1224] = { 0 };
	SerializedBuffer bufffer;
	uint32_t shift = 0;
	bufffer.buffer_size = sizeof(HeartBeatSubMessage_t);
	bufffer.buffer = data;
	bufffer.buffer = bufffer.buffer + 1024;

/*	HeartbeatMsgFlag_t flag;
	flag.groupInfoFlag = 1;
	flag.groupInfoFlag = 1; */
	hbMsg.header.flags = TypeSupport::IsLittleEndian() |  0x2  ;
	/* wangyi liveliness  */
	if (liveliness) {
		hbMsg.header.flags |= BIT(2);
	}
	hbMsg.readerId = readerProxy->GetGuid().entityId;
	hbMsg.writerId = guid_.entityId;
	hbMsg.firstSN = minSeq;
	hbMsg.lastSN = maxSeq;
	hbMsg.count = readerProxy->HeaertSendCountAdd();

	MessageProcessor::EncapsulateHeartBeatMessage(bufffer, shift, hbMsg);

	bufffer.buffer_size = shift;
	shift = 0;

	/* 封装一个Gap报文 */
	if (relateSeq > minSeq) {
		/*  minseq 和 relateSeq 之间的序号与对应的代理无关，发送Gap报文 */
		GapSubMessage_t gapMsg;
		gapMsg.header.subMessageId = GAP;
		gapMsg.header.subMessageLength = 10; //TODO
		gapMsg.header.flags = TypeSupport::IsLittleEndian();
		gapMsg.writerId = guid_.entityId;
		gapMsg.readerId = readerProxy->GetGuid().entityId;
		gapMsg.gapStart = minSeq;
		gapMsg.gapList.bitmapBase = relateSeq;
		gapMsg.gapList.numBit = 0;
		int size = 0;
		
		size = sizeof(gapMsg.header) +  sizeof(gapMsg.writerId) * 2 +  sizeof(gapMsg.gapStart) + sizeof(gapMsg.gapList.bitmapBase)
			+ sizeof(gapMsg.gapList.numBit);
		bufffer.buffer = bufffer.buffer - size;
		
		MessageProcessor::EncapsulateGapMessage(bufffer, shift, gapMsg);
		bufffer.buffer_size += shift;
		shift = 0;
	}

	MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix);

	messageTransmitter->MessageSend(srcLocators, unicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
	//TODO 暂时屏蔽组播
	// messageTransmitter->MessageSend(srcLocators, multicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
	
	return RETCODE_OK;
}


ReturnCode_t StatefulWriter::WriteHeartFragbeat(std::shared_ptr<ReaderProxy> readerProxy)
{
	//TODO 
	AGILOG(AGILOG_ERROR, " not support interface");
	return RETCODE_UNSUPPORTED;
}

ReturnCode_t StatefulWriter::ReSendData(std::shared_ptr<ReaderProxy> readerProxy)
{
	SeqNumberVec gapVec;
	SequenceNumber_t lastGapseq;
	SeqNumberVec seqVec = readerProxy->RequestedChanges();
	std::vector<std::shared_ptr<ReaderProxy>> readerProxys;
	readerProxys.push_back(readerProxy);

	for(auto& seqNumber : seqVec)
	{
		std::shared_ptr<CacheChange> change = writerCache_->GetChange(seqNumber);
		
		if (!change.get()) {
			gapVec.push_back(seqNumber);
			continue;
		}
		/* 如果是分片报文，并且已经通过NackFrag报文请求数据，不在重复发送报文 */
		if (0 != change->fragmentTatolNum && readerProxy->IsNackFrag(seqNumber)) {
			continue;
		}

		DeliveryDataToReader(change, readerProxys);
	}

	SendGap(gapVec, readerProxy);

	WriteHeartbeat(readerProxy);

	return RETCODE_OK;
}

void StatefulWriter::RemoveAllAckedData()
{
	auto seq = writerCache_->GetSeqNumMin();
	bool remove = true;
	while (seq != SEQUENCENUMBER_UNKNOWN) {
		IsAckedByAll(seq);
		if (IsAckedByAll(seq)) {
			writerCache_->RemoveChange(writerCache_->GetChange(seq));
		}
		else {
			break;
		}
		seq = writerCache_->GetSeqNumMin();
	}
}

void StatefulWriter::SendGap(const SeqNumberVec& seqVec, std::shared_ptr<ReaderProxy> proxy)
{
	if (!proxy.get() || 0 == seqVec.size()) {
		return;
	}
	//TODO 
	GapSubMessage_t gapMsg;
	gapMsg.header.subMessageId = GAP;
	gapMsg.header.subMessageLength = 10; //TODO
	gapMsg.header.flags = TypeSupport::IsLittleEndian();
	gapMsg.writerId = guid_.entityId;
	gapMsg.readerId = proxy->GetGuid().entityId;
	gapMsg.gapStart = seqVec[0];
	gapMsg.gapList.bitmapBase = gapMsg.gapStart + 1;
	int32_t i = 0;
	for (i = 1; i < seqVec.size(); i++) {
		if (seqVec[i] - gapMsg.gapList.bitmapBase > 0) {
			break;
		 }
		gapMsg.gapList.bitmapBase = seqVec[i] + 1;
	}
	for ( ; i < seqVec.size(); i++) {
		int32_t s = seqVec[i] - gapMsg.gapList.bitmapBase;
		if (s >= 255) {
			break;
		}
		gapMsg.gapList.numBit = s + 1;
		gapMsg.gapList.bitmap[s / 32] |= 0x80000000 >> (s % 32);
	}
	LocatorVec srcLocators;
	Locator_t srcLocator;
	srcLocator.kind = LOCATOR_KIND_UDPv4;

	srcLocators.push_back(srcLocator);
	//TODO 发送报文
	std::shared_ptr<MessageTransmitter> messageTransmitter = participant_->GetMessageTransmitter();

	LocatorVec dstLocators = proxy->GetUnicastLocatorList();

	uint32_t shift = 0;	
	SerializedBuffer bufffer;
	char data[1224] = { 0 };
	bufffer.buffer_size = sizeof(HeartBeatSubMessage_t);
	bufffer.buffer = data;
	bufffer.buffer = bufffer.buffer + 1024;

	MessageProcessor::EncapsulateGapMessage(bufffer, shift, gapMsg);
	bufffer.buffer_size = shift;
	shift = 0;
	MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix);

	messageTransmitter->MessageSend(srcLocators, dstLocators, bufffer.buffer - shift, bufffer.buffer_size + shift);
}

/* 重传分片数据 */
ReturnCode_t StatefulWriter::ReSendDataFrag(std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& datafrag)
{
	ReturnCode_t ret = RETCODE_OK;

	/* 找到对应的序号的数据 */
	std::shared_ptr<CacheChange> cache =  writerCache_->GetChange(datafrag.writerSN);
	if (cache) {
		std::shared_ptr<MessageTransmitter> messageTransmitter = participant_->GetMessageTransmitter();

		DataFragSubMessage_t dataFragSubMessage;
		if (0 != cache->serializedBuffer->buffer_size) {
			//TODO jdxcz 需要确认什么时候标志位设置BIT(3)
			dataFragSubMessage.header.flags |= BIT(2);
		}

		if (0 != cache->inlineQos.size())
		{
			dataFragSubMessage.header.flags |= BIT(1);
			dataFragSubMessage.inlineQos = cache->inlineQos;
		}

		dataFragSubMessage.header.flags |= BIT(0);

		dataFragSubMessage.octetsToInlineQos = 0;
		dataFragSubMessage.extraFlags[0] = 0;
		dataFragSubMessage.extraFlags[1] = 0;
		dataFragSubMessage.readerId = ENTITYID_UNKNOWN;
		dataFragSubMessage.writerId = guid_.entityId;
		dataFragSubMessage.writerSN = cache->sequenceNumber;
		dataFragSubMessage.dataFrag.fragmentStartingNum = 0;

		SerializedBuffer* buffer = cache->serializedBuffer;
		LocatorVec srcLocators;
		Locator_t srcLocator;
		srcLocator.kind = LOCATOR_KIND_UDPv4;

		srcLocators.push_back(srcLocator);
		LocatorVec dstLocators = readerProxy->GetUnicastLocatorList();
		int num = 0;
		for (int32_t i = 1; i <= cache->fragmentTatolNum && num < datafrag.fragmentNumberState.numBit; i++, num) {

			if (i == datafrag.fragmentNumberState.bitmapBase + num) {

				if (datafrag.fragmentNumberState.bitmap[num] & 0x80000000 >> (num % 32)) {
					uint32_t shift = 0;
					dataFragSubMessage.dataFrag.fragmentsInSubmessage = 1;
					dataFragSubMessage.dataFrag.fragmentSize = DATAMESSAGE_LIMIT;
					dataFragSubMessage.dataFrag.fragmentStartingNum = i;
					dataFragSubMessage.dataFrag.sampleSize = cache->dataSize;
					MessageProcessor::EncapsulateDataFragMessage(*buffer, shift, dataFragSubMessage);

					MessageProcessor::EncapsulateMessageHeader(*buffer, shift, guid_.prefix);
					messageTransmitter->MessageSend(srcLocators, dstLocators, buffer->buffer - shift, buffer->buffer_size + shift);
				}
				num++;
			}
			buffer = buffer->next;
		}
	}
	else {
		/*  数据因为某种原因被删除，需要发送Gap报文 */
		SeqNumberVec seqVec;
		seqVec.push_back(datafrag.writerSN);
		SendGap(seqVec, readerProxy);
	}

	return ret;
}
