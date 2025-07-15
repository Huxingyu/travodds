#include "rtps/endpoint/statefulwriter.h"
#include "rtps/endpoint/participant.h"
#include "rtps/messages/messageprocessor.h"
#include "rtps/transport/messagetransmitter.h"
#include "common/time/ddstimer.h"
#include "common/threads/threadpool.h"
#include "dcps/publish/datawriterimpl.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "rtps/flowcontrol/flowcontroller.h"
#include "common/log/logger.h"
#include <chrono>
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
	if (timer_) {
		timer_->Exit();
	}
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

	auto itor = matchedReaders_.find(readerGuid);
	if (itor == matchedReaders_.end()) {
		AGILOG(AGILOG_ERROR, "no match reader");
		return nullptr;
	}
	return itor->second;
}

ReturnCode_t StatefulWriter::WaitForAcknowledgments(const Duration_t & max_wait)
{
	{
		std::lock_guard<std::mutex> lock(readerPorxyMutex_);
		if (CheckAllAcked()) {
			return RETCODE_OK;
		}
	}
	std::chrono::nanoseconds maxW(max_wait.ToNs());
	std::unique_lock<std::mutex> ackLock(allAckedMtx_);
	auto result = condAllAcked_.wait_for(ackLock, maxW);
	return result == std::cv_status::no_timeout ? RETCODE_OK : RETCODE_TIMEOUT;
}

bool StatefulWriter::CheckAllAcked()
{
	SequenceNumber_t sequenceNumber = writerCache_->GetSeqNumMax();
	for (auto& reader : matchedReaders_) {
		if (false == reader.second->IsAllAcked(sequenceNumber)) {
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

ReturnCode_t StatefulWriter::DeliveryData(std::shared_ptr<CacheChange> change, bool flowControl)
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
	DeliveryDataToReader(change, readerProxys, flowControl);

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
	} else if (RETCODE_OK == ret) {
		if(CheckAllAcked()) {
			condAllAcked_.notify_all();
		}
	}

	if (qos_.durability.kind != TRANSIENT_LOCAL_DURABILITY_QOS) {
		lock.unlock();
		RemoveAllAckedData();
		lock.lock();
	}

	//返回值为1
	if (RETCODE_REPEAT_REQUEXT == ret) {
		// TODO 为RETCODE_REPEAT_REQUEXT，需要重传数据
		flowController_->ReSendData(this,proxy);
	}
	/* 需要强制回复一个HB报文 */
	if (ackMsg.header.flags & !SubMessageFlag::FINAL_FLAG) {
		//TODO 
		WriteHeartbeat(proxy, true);
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

	flowController_->ReSendDataFrag(this, proxy, nackMsg);

	//需要强制回复一个HBFrag报文
	if (nackMsg.header.flags & SubMessageFlag::FINAL_FLAG) {
		//TODO 
	}


	return RETCODE_OK;
}

ReturnCode_t StatefulWriter::DeliveryDataToReader(std::shared_ptr<CacheChange> change, const std::vector<std::shared_ptr<ReaderProxy>>& readerProxys, bool flowControl)
{
	if (nullptr == change || 0 == readerProxys.size()) {
		return RETCODE_OK;
	}

	DataSubMessage_t dataSubMessage;

	SerializedBuffer* buffer = change->serializedBuffer;
	if (buffer->next) {
		/* 分片数据处理 */
		DataFragSubMessage_t dataFragSubMessage;
		if (0 != change->inlineQos.size())
		{
			dataFragSubMessage.header.flags |= SubMessageFlag::INLINE_QOS_FLAG;
			dataFragSubMessage.inlineQos = change->inlineQos;
		}

		dataFragSubMessage.header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

		/* wangyi fastdds 适配测试默认16 */
		dataFragSubMessage.octetsToInlineQos = OCTETS_TO_INLINEQOS_LEN;
		dataFragSubMessage.extraFlags[0] = 0;
		dataFragSubMessage.extraFlags[1] = 0;
		dataFragSubMessage.readerId = ENTITYID_UNKNOWN;
		dataFragSubMessage.writerId = guid_.entityId;
		dataFragSubMessage.writerSN = change->sequenceNumber;
		dataFragSubMessage.dataFrag.fragmentStartingNum = 0;
		uint32_t sendFragCount = 0;
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

			if (flowControl) {
				participant_->FlowRateLimitedMessageSend(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift);
			} else {
				participant_->TransmittersSendMessage(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift);
			}

			/* TODO 添加延时 */
			sendFragCount++;
			if (0 == sendFragCount % DATAFRAG_DELAY_NUM) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			buffer = buffer->next;
		} while (buffer);

	}
	else {
		if (0 != change->serializedBuffer->buffer_size) {
			dataSubMessage.header.flags |= SubMessageFlag::DATA_FLAG;
		}

		if (0 != change->inlineQos.size()) {
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
		if (flowControl) {
			participant_->FlowRateLimitedMessageSend(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift);
		} else {
			participant_->TransmittersSendMessage(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift);
		}
	}

	return RETCODE_OK;
}

ReturnCode_t StatefulWriter::WriteHeartbeat()
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);
	HeartBeatSubMessage_t hbMsg;

	for (auto& readerProxy : matchedReaders_) {
		WriteHeartbeat(readerProxy.second, false);
	}
	return RETCODE_OK;
}

/* wangyi 发送 liveliness 心跳 */
ReturnCode_t StatefulWriter::WriteHeartbeat(bool liveliness)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	HeartBeatSubMessage_t hbMsg;

	for (auto& writerProxy : matchedReaders_) {
		WriteHeartbeat(writerProxy.second, true,liveliness);
	}
	return RETCODE_OK;
}

ReturnCode_t StatefulWriter::WriteHeartbeat(std::shared_ptr<ReaderProxy> readerProxy, bool force, bool liveliness)
{
	//TODO 此函数可以发送HeartBeat报文,当时后续需要优化改进

	if (!readerProxy) {
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

	/* 在非强制回复HB的情况下，如果所有数据被确认，没有数据需要被确认不发送心跳  */
	if (!force && (SEQUENCENUMBER_ZERO >= maxSeq || !readerProxy->UnackedChanges())) {
		return RETCODE_OK;
	}

	LocatorSeq unicastList = readerProxy->GetUnicastLocatorList();
	LocatorSeq multicastList = readerProxy->GetMulticastLocatorList();

	char data[1224] = { 0 };
	SerializedBuffer bufffer;
	uint32_t shift = 0;
	bufffer.buffer_size = sizeof(HeartBeatSubMessage_t);
	bufffer.buffer = data;
	bufffer.buffer = bufffer.buffer + BUFF_SHIFT;

/*	HeartbeatMsgFlag_t flag;
	flag.groupInfoFlag = 1;
	flag.groupInfoFlag = 1; */
	hbMsg.header.flags = (IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG);
	/* wangyi liveliness  */
	if (liveliness) {
		hbMsg.header.flags |= SubMessageFlag::LIVELINESS_FLAG;
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
		gapMsg.header.subMessageLength = SUB_MESSAGE_LEN_GAP; //TODO
		gapMsg.header.flags = IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;
		gapMsg.writerId = guid_.entityId;
		gapMsg.readerId = readerProxy->GetGuid().entityId;
		gapMsg.gapStart = minSeq;
		gapMsg.gapList.bitmapBase = relateSeq;
		gapMsg.gapList.bitmap.size = 0;
		int size = 0;
		
		size = sizeof(gapMsg.header) +  sizeof(gapMsg.writerId) * 2 +  sizeof(gapMsg.gapStart) + sizeof(gapMsg.gapList.bitmapBase)
			+ sizeof(gapMsg.gapList.bitmap.size);
		bufffer.buffer = bufffer.buffer - size;
		
		MessageProcessor::EncapsulateGapMessage(bufffer, shift, gapMsg);
		bufffer.buffer_size += shift;
		shift = 0;
	}

	MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix);

	participant_->TransmittersSendMessage(unicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
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

ReturnCode_t StatefulWriter::ReSendData(std::shared_ptr<ReaderProxy> readerProxy, bool flowControl)
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

		DeliveryDataToReader(change, readerProxys, flowControl);
	}

	SendGap(gapVec, readerProxy);

	WriteHeartbeat(readerProxy, false);

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
	gapMsg.header.subMessageLength = SUB_MESSAGE_LEN_GAP; //TODO
	gapMsg.header.flags = IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;
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
		gapMsg.gapList.bitmap.size = s + 1;
		gapMsg.gapList.bitmap.SetBit(s); 
	}

	LocatorVec dstLocators = proxy->GetUnicastLocatorList();

	uint32_t shift = 0;	
	SerializedBuffer bufffer;
	char data[1224] = { 0 };
	bufffer.buffer_size = sizeof(HeartBeatSubMessage_t);
	bufffer.buffer = data;
	bufffer.buffer = bufffer.buffer + BUFF_SHIFT;

	MessageProcessor::EncapsulateGapMessage(bufffer, shift, gapMsg);
	bufffer.buffer_size = shift;
	shift = 0;
	MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix);

	participant_->TransmittersSendMessage(dstLocators, bufffer.buffer - shift, bufffer.buffer_size + shift);
}

/* 重传分片数据 */
ReturnCode_t StatefulWriter::ReSendDataFrag(std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& datafrag, bool flowControl)
{
	ReturnCode_t ret = RETCODE_OK;

	/* 找到对应的序号的数据 */
	std::shared_ptr<CacheChange> cache =  writerCache_->GetChange(datafrag.writerSN);
	if (cache) {

		DataFragSubMessage_t dataFragSubMessage;

		if (0 != cache->inlineQos.size()) {
			dataFragSubMessage.header.flags |= SubMessageFlag::INLINE_QOS_FLAG;
			dataFragSubMessage.inlineQos = cache->inlineQos;
		}

		dataFragSubMessage.header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

		/* wangyi fastdds 适配测试默认16 */
		dataFragSubMessage.octetsToInlineQos = OCTETS_TO_INLINEQOS_LEN;
		dataFragSubMessage.extraFlags[0] = SubMessageFlag::NONE_FLAG;
		dataFragSubMessage.extraFlags[1] = SubMessageFlag::NONE_FLAG;
		dataFragSubMessage.readerId = ENTITYID_UNKNOWN;
		dataFragSubMessage.writerId = guid_.entityId;
		dataFragSubMessage.writerSN = cache->sequenceNumber;
		dataFragSubMessage.dataFrag.fragmentStartingNum = 0;

		SerializedBuffer* buffer = cache->serializedBuffer;
		LocatorVec dstLocators = readerProxy->GetUnicastLocatorList();

        for(uint32_t i = 1; i < datafrag.fragmentNumberState.bitmapBase; i++)
        {
            if(nullptr != buffer){
                buffer = buffer->next;
            }
        }

        if(nullptr == buffer){
            AGILOG(AGILOG_ERROR, "buffer is nullptr");
			return RETCODE_ERROR;
        }

		/* TODO  */
		int32_t fragSendCount = 0;
		for (int32_t num = 0; (num + datafrag.fragmentNumberState.bitmapBase) <= cache->fragmentTatolNum && num < datafrag.fragmentNumberState.bitmap.size; num++) {
			if (datafrag.fragmentNumberState.bitmap.IsBitSet(num)) {

		        if(nullptr == buffer){
		            AGILOG(AGILOG_ERROR, "buffer is nullptr");
					return RETCODE_ERROR;
		        }
			
				uint32_t shift = 0;
				dataFragSubMessage.dataFrag.fragmentsInSubmessage = 1;
				dataFragSubMessage.dataFrag.fragmentSize = DATAMESSAGE_LIMIT;
				dataFragSubMessage.dataFrag.fragmentStartingNum = num + datafrag.fragmentNumberState.bitmapBase;
				dataFragSubMessage.dataFrag.sampleSize = cache->dataSize;
				MessageProcessor::EncapsulateDataFragMessage(*buffer, shift, dataFragSubMessage);
				
				MessageProcessor::EncapsulateMessageHeader(*buffer, shift, guid_.prefix);
				if (flowControl) {
					participant_->FlowRateLimitedMessageSend(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift);
				} else {
					participant_->TransmittersSendMessage(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift);
				}
				
				fragSendCount++;

				if (0 == fragSendCount % DATAFRAG_DELAY_NUM) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
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
