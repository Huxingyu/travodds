#include "rtps/endpoint/statefulreader.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "rtps/endpoint/participant.h"
#include "rtps/messages/messageprocessor.h"
#include "rtps/transport/messagetransmitter.h"
#include <rtps/builtin/liveliness/livelinessmanager.h>

USING_AGIDDS_NAMESPACE;
StatefulReader::StatefulReader(const DataReaderQos& qos, const GUID_t& guid, const TopicKind_t& kind)
	: Reader(qos, kind)
{
	guid_ = guid;
}

StatefulReader::~StatefulReader()
{
	WriterProxyMap writerProxys = matchedWriters_;

	for (auto & writerProxy : writerProxys) {
		MatchedWriterRemove(writerProxy.second);
	}

	writerProxys.clear();
}

void StatefulReader::MatchedWriterAdd(std::shared_ptr<WriterProxy> writerProxy)
{
	matchedWriters_[writerProxy->GetGuid()] = writerProxy;
	/* wangyi */
	Duration_t duration = TIME_INFINITE;
	if (qos_.liveliness.lease_duration < duration) {
		auto livelinessManager = participant_->GetLivelinessManager();
		if (livelinessManager != nullptr) {
			livelinessManager->GetRemoteWriterLivelinessManager()->AddWriter(LivelinesStatus(writerProxy->GetGuid(), qos_.liveliness));
		}
		else {
			/* TODO 日志 */
		}
	}
}

void StatefulReader::MatchedWriterRemove(std::shared_ptr<WriterProxy> writerProxy)
{
	/* wangyi */
	Duration_t duration = TIME_INFINITE;
	if (qos_.liveliness.lease_duration < duration) {
		auto livelinessManager = this->participant_->GetLivelinessManager();
		if (livelinessManager != nullptr) {
			livelinessManager->GetRemoteWriterLivelinessManager()->RemoveWriter(writerProxy->GetGuid());
		}
		else {
			/* TODO 日志 */
		}
	}
	matchedWriters_.erase(writerProxy->GetGuid());
}

std::shared_ptr<WriterProxy> StatefulReader::MatchedWriterLookup(GUID_t writerGuid)
{
	if(0 == matchedWriters_.count(writerGuid))
		return nullptr;

	return matchedWriters_[writerGuid];
}

bool StatefulReader::MatchedWriterIsMatched(GUID_t writerGuid)
{
	if (0 == matchedWriters_.count(writerGuid)) {
		return false;
	}
	return true;
}

ReturnCode_t StatefulReader::ReceiveDataMessage(const Receive_t&  headerMsg, const DataSubMessage_t& dataMsg, SerializedPayload data, uint32_t len)
{
	ChangeKind_t kind = ALIVE;
	SerializedBuffer* buffer = nullptr;
	GUID_t guid = { headerMsg.sourceGuidPrefix, dataMsg.writerId};
	InstanceHandle_t pubHandle;
	InstanceHandle_t dataHandle = HANDLE_NIL;

	/* wangyi 更新存活性 */
	AssertWriterLiveliness(guid);

	memcpy(pubHandle.keyHash.value, &guid, 16);


	/* 获取标志 */
	DataMsgFlag_t dataFlags;
	dataFlags.endiannessFlag = dataMsg.header.flags & FLAG_BIT_0;
	dataFlags.inlineQosFlag = dataMsg.header.flags & FLAG_BIT_1;
	dataFlags.dataFlag = dataMsg.header.flags & FLAG_BIT_2;
	dataFlags.keyFlag = dataMsg.header.flags & FLAG_BIT_3;
	dataFlags.nonStandardPayloadFlag = dataMsg.header.flags & FLAG_BIT_4;

	std::shared_ptr<WriterProxy> proxy = nullptr;
	auto itor = matchedWriters_.find(guid);
	if (itor != matchedWriters_.end()) {
		proxy = itor->second;
	}
	if (nullptr == proxy) {
		return RETCODE_ERROR;
	}

	if (dataFlags.inlineQosFlag) {
		ProcessInlineQos(kind, dataHandle, dataMsg.inlineQos);
	}
	buffer = new SerializedBuffer;

	if (!dataFlags.nonStandardPayloadFlag && nullptr != data) {
		MemoryPool::GetInstance()->getBuffer(len, buffer->origin_buffer);
		buffer->buffer = buffer->origin_buffer;
		buffer->writer_position = len;
		buffer->buffer_size = len;
		memcpy(buffer->buffer, data, len);
	}

	std::shared_ptr<CacheChange> cache = new_change(kind, buffer, dataHandle, pubHandle, headerMsg.timestamp);
	cache->sequenceNumber = dataMsg.writerSN;
	cache->inlineQos = dataMsg.inlineQos;

	ReturnCode_t ret = RETCODE_OK;

	ret = proxy->AddReliableDataCache(cache);
		
	if (RETCODE_OK == ret) {
		Notify(proxy);
	}
	
	return ret;
}

ReturnCode_t StatefulReader::ReceiveDataFragMessage(const Receive_t&  headerMsg, const DataFragSubMessage_t& dataMsg, SerializedPayload data, uint32_t len)
{
	ChangeKind_t kind = ALIVE;
	GUID_t guid = { headerMsg.sourceGuidPrefix, dataMsg.writerId };
	InstanceHandle_t pubHandle = HANDLE_NIL;
	InstanceHandle_t dataHandle = HANDLE_NIL;

	/* wangyi 更新存活性 */
	AssertWriterLiveliness(guid);

	DataFragMsgFlag_t dfFlags;
	dfFlags.endiannessFlag = dataMsg.header.flags & FLAG_BIT_0;
	dfFlags.inlineQosFlag = dataMsg.header.flags & FLAG_BIT_1;
	dfFlags.keyFlag = dataMsg.header.flags & FLAG_BIT_2;
	dfFlags.nonStandardPayloadFlag = dataMsg.header.flags & FLAG_BIT_3;

	memcpy(pubHandle.keyHash.value, &guid, 16);
	
	std::shared_ptr<WriterProxy> proxy = nullptr;
	auto itor = matchedWriters_.find(guid);
	if (itor != matchedWriters_.end()) {
		proxy = itor->second;
	}
	if (nullptr == proxy) {
		return RETCODE_ERROR;
	}

	//TODO 存在inlineQos
	if (dfFlags.inlineQosFlag) {
		ProcessInlineQos(kind, dataHandle, dataMsg.inlineQos);
	}
	/* 理应存在数据，但是实际数据为空 */
	if (kind != ALIVE || (dfFlags.nonStandardPayloadFlag || nullptr == data) ) {
		//TODO  
		/**  DataFrag必须存在数据，状态为ALIVE  **/
		std::cout << "DataFrag msg err " << __FUNCTION__ << " " << __LINE__ << std::endl;
		return RETCODE_ERROR;
	}


	if (!proxy->ExistCache(dataMsg.writerSN)) {

		SerializedBuffer *buffer = new SerializedBuffer();

		MemoryPool::GetInstance()->getBuffer(dataMsg.dataFrag.sampleSize + 4, buffer->origin_buffer);
		buffer->buffer = buffer->origin_buffer;
		/* 4字节为CDR信息 */
		buffer->buffer_size = dataMsg.dataFrag.sampleSize + 4;

		std::shared_ptr<CacheChange> dataFragCache = new_change(kind, buffer, dataHandle, pubHandle, headerMsg.timestamp);

		dataFragCache->sequenceNumber = dataMsg.writerSN;
		dataFragCache->dataSize = dataMsg.dataFrag.sampleSize;
		dataFragCache->fraqmentSize = dataMsg.dataFrag.fragmentSize;
		dataFragCache->fragmentTatolNum = dataMsg.dataFrag.sampleSize / dataMsg.dataFrag.fragmentSize;
		if (0 != dataMsg.dataFrag.sampleSize % dataMsg.dataFrag.fragmentSize) {
			dataFragCache->fragmentTatolNum += 1;
		}
		for (int32_t i = 1; i <= dataFragCache->fragmentTatolNum; i++) {
			dataFragCache->fragmentSeq.insert(i);
		}
		proxy->AddReliableDataCache(dataFragCache, false);
	}

	if (1 != dataMsg.dataFrag.fragmentsInSubmessage) {
		//TODO
		std::cout << "DataFrag msg err " << __FUNCTION__ << " " << __LINE__ << std::endl;
		return RETCODE_OK;
	}
	proxy->AddFragData(dataMsg.writerSN, (char*)data, len, dataMsg.dataFrag.fragmentStartingNum);

	Notify(proxy);

	return RETCODE_OK;
}
ReturnCode_t StatefulReader::ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartMsg)
{
	GUID_t guid(headerInfo.sourceGuidPrefix, heartMsg.writerId);
	std::shared_ptr<WriterProxy> proxy = nullptr;
	HeartbeatMsgFlag_t flags;
	flags.endiannessFlag = heartMsg.header.flags & FLAG_BIT_0;
	flags.finalFlag = heartMsg.header.flags & FLAG_BIT_1;
	flags.livelinessFlag = heartMsg.header.flags & FLAG_BIT_2;
	flags.groupInfoFlag = heartMsg.header.flags & FLAG_BIT_3;

	auto itor = matchedWriters_.find(guid);
	if (matchedWriters_.end() == itor) {
		printf("fun:%s line:%d error\n", __func__, __LINE__);
		return RETCODE_ERROR;
	}
	proxy = itor->second;
	if (!proxy) {
		printf("fun:%s line:%d error\n", __func__, __LINE__);
		return RETCODE_ERROR;
	}

	/* TODO 调用函数前进行判断，报文的有效性判断 */
	//SequenceNumber_t SEQUENCENUMBER_ZERO
	if (heartMsg.firstSN <= SEQUENCENUMBER_ZERO || heartMsg.lastSN < SEQUENCENUMBER_ZERO || heartMsg.lastSN < heartMsg.firstSN - 1) {
		printf("fun:%s line:%d error\n", __func__, __LINE__);
		return RETCODE_ERROR;
	}
	if (flags.groupInfoFlag) {
		//TODO 相关QoS不进行实现
		if (heartMsg.currentGSN <= SEQUENCENUMBER_ZERO || heartMsg.firstGSN <= SEQUENCENUMBER_ZERO || heartMsg.lastGSN <= SEQUENCENUMBER_ZERO ||
			heartMsg.lastGSN < heartMsg.firstGSN - 1 || heartMsg.currentGSN > heartMsg.firstGSN || heartMsg.currentGSN < heartMsg.lastGSN) {
			printf("groupInfoFlags fun:%s line:%d error\n", __func__, __LINE__);
			return RETCODE_ERROR;
		}
	}

	/* 更新存活性 */
	if (flags.livelinessFlag) {
		AssertWriterLiveliness(guid);
	}

	/* 根据解析报文获取解析相关的数据 */
	int32_t lostCount = 0;
	AcknackSubMessage_t ackMsg;
	ackMsg.count = 0;
	SequenceNumberSet_t& set = ackMsg.readerSNState;

	ReturnCode_t ret = proxy->RecvHeartbeat(heartMsg, lostCount, set);
	if (RETCODE_RECEIVE_DUPLICATE == ret) {
		// 20250331 RETCODE_RECEIVE_DUPLICATE表示当前序号的HB重复接收，不进行处理
		return RETCODE_OK;
	}

	if (lostCount && datareader_) {
		/* 存在丢包数据,触发监听回调函数 */ 
		datareader_->OnSampleLost(lostCount);	
	}
	std::vector<NackFragSubMessage_t> nackFragVec;
	proxy->GetNackSequenceSet(nackFragVec);
	/*  发送Nack的报文  */
	if (set.numBit != 0 || flags.finalFlag) {
		//TODO 存在丢包，或者强制回复Ack报文。创建Ack报文进行回复 
		/* 1. 先给分片数据发送NackFrag  */
		for (int32_t i = 0; i < nackFragVec.size(); i++) {
			nackFragVec[i].readerId = guid_.entityId;
			nackFragVec[i].writerId = heartMsg.writerId;
			nackFragVec[i].count = heartMsg.count;

			SendNackFragMessage(proxy, nackFragVec[i]);
		}	
	}

	if (set.numBit != 0 || flags.finalFlag) {
		//TODO 存在丢包，或者强制回复Ack报文。创建Ack报文进行回复 
		ackMsg.count = heartMsg.count;
		ackMsg.readerId = guid_.entityId;
		ackMsg.writerId = heartMsg.writerId;

		/* 1. 先给分片数据发送NackFrag  */
		SendAckMessage(proxy, ackMsg);	
	}

	/* 看是否有新的数据提交 */
	Notify(proxy);

	return RETCODE_OK;
}

ReturnCode_t StatefulReader::ReceiveHeartFragMessage(const Receive_t&  headerInfo, const HeartBeatFragSubMessage_t& heartFragMsg)
{

	GUID_t guid(headerInfo.sourceGuidPrefix, heartFragMsg.writerId);
	std::shared_ptr<WriterProxy> proxy = nullptr;

	auto itor = matchedWriters_.find(guid);
	if (matchedWriters_.end() == itor) {
		printf("fun:%s line:%d error", __func__, __LINE__);
		return RETCODE_ERROR;
	}
	proxy = itor->second;
	if (!proxy) {
		printf("fun:%s line:%d error", __func__, __LINE__);
		return RETCODE_ERROR;
	}

	/* 根据解析报文获取解析相关的数据 */
	NackFragSubMessage_t nackMsg;
	FragMentNumberSet_t& set = nackMsg.fragmentNumberState;
	ReturnCode_t ret = proxy->RecvHeartbeatFrag(heartFragMsg, set);
	if (RETCODE_RECEIVE_DUPLICATE == ret) {
		// 20250331 RETCODE_RECEIVE_DUPLICATE 表示当前序号的HB重复接收，不进行处理
		return RETCODE_OK;
	}

	if (set.numBit != 0) {
		//TODO 存在丢包,回复nack报文 
		
	}

	/* 看是否有新的数据提交 */
	Notify(proxy);

	return RETCODE_OK;
}

ReturnCode_t StatefulReader::ReceiveGapMessage(const Receive_t&  headerInfo, const GapSubMessage_t& gapInfo)
{
	if (BEST_EFFORT_RELIABILITY_QOS == qos_.reliability.kind) {
		return RETCODE_OK;
	}
	bool groupInfoFlag = gapInfo.header.flags & 02;
	bool filteredCountFlag = gapInfo.header.flags & 04;

	/*  判断报文是否有效  */
	SequenceNumber_t seq = SEQUENCENUMBER_ZERO;
	if (gapInfo.gapStart <= seq && (0 == gapInfo.gapList.numBit || gapInfo.gapList.numBit > 256)) {
		PRINTF_ERROR();
		return RETCODE_ERROR;
	}
	if (groupInfoFlag && (gapInfo.gapStartGSN <= seq || gapInfo.gapEndGSN <= seq || gapInfo.gapEndGSN < gapInfo.gapStart - 1) ) {
		PRINTF_ERROR();
		return RETCODE_ERROR;
	}


	GUID_t guid(headerInfo.sourceGuidPrefix, gapInfo.writerId);
	auto itor = matchedWriters_.find(guid);
	if (matchedWriters_.end() == itor) {
		PRINTF_ERROR();
		return RETCODE_ERROR;
	}
	std::shared_ptr<WriterProxy> proxy = itor->second;
	if (nullptr == proxy) {
		PRINTF_ERROR();
		return RETCODE_ERROR;
	}

	SeqNumberVec seqVec;
	int32_t i = 0;
	for (seq = gapInfo.gapStart; seq < gapInfo.gapList.bitmapBase; seq++) {
		seqVec.push_back(seq);
	}
	for (auto i = 0; i < gapInfo.gapList.numBit; i++) {
		if (gapInfo.gapList.bitmap[i / 32] & (0x80000000 >> (i % 32) ) ){
			seq = gapInfo.gapList.bitmapBase + i;
			seqVec.push_back(seq);
		}	
	}
	proxy->NonAvailableChangeSet(seqVec, gapInfo.filteredCount);

	Notify(proxy);

	return RETCODE_OK;
}


ReturnCode_t StatefulReader::Notify(std::shared_ptr<WriterProxy> proxy)
{
	ReturnCode_t ret = RETCODE_OK;
	if (!proxy) {
		return RETCODE_ERROR;
	}

	std::shared_ptr<CacheChange> cache = proxy->GetSubmitCache();
	while (nullptr != cache) {
		ret = AddChanges(cache);
		if (RETCODE_OK != ret && datareader_) {
			datareader_->OnSampleRejected(cache->instanceHandle, (SampleRejectedStatusKind)ret);
		}

		//TODO 提交数据 
		if (readerListener_) {
			readerListener_->on_data_available(cache);
		}
		else if (datareader_) {
			datareader_->OnDataAvaible();
		}

		cache = proxy->GetSubmitCache();
	}

	return RETCODE_OK;
}

ReturnCode_t StatefulReader::AssertWriterLiveliness(const GUID_t& writer)
{
	Duration_t duration = TIME_INFINITE;
	if (qos_.liveliness.lease_duration < duration) {
		auto livelinessManager = participant_->GetLivelinessManager();
		if (livelinessManager != nullptr) {
			LivelinessQosPolicyKind kind;
			if (livelinessManager->GetRemoteWriterLivelinessManager()->GetWriterLiveLinessKind(writer, kind)) {
				livelinessManager->GetRemoteWriterLivelinessManager()->AssertLiveliness(writer, kind);
			}
		}
		else {
			/* TODO 日志 */
		}
	}
	return RETCODE_OK;
}


ReturnCode_t StatefulReader::SendAckMessage(std::shared_ptr<WriterProxy> writerProxy, AcknackSubMessage_t& ackMsg)
{
	if (!writerProxy) {
		PRINTF_ERROR();
		return RETCODE_ERROR;
	}
	
	LocatorSeq unicastList = writerProxy->GetUnicastLocatorList();
	LocatorSeq multicastList = writerProxy->GetMulticastLocatorList();

	LocatorVec srcLocators;
	Locator_t srcLocator;
	srcLocator.kind = LOCATOR_KIND_UDPv4;
	srcLocators.push_back(srcLocator);

	std::shared_ptr<MessageTransmitter> messageTransmitter = participant_->GetMessageTransmitter();

	if (!messageTransmitter) {
		PRINTF_ERROR();
		return RETCODE_ERROR;
	}
	char data[1224] = { 0 };
	SerializedBuffer bufffer;
	uint32_t shift = 0;
	bufffer.buffer_size = sizeof(AcknackSubMessage_t);
	bufffer.buffer = data;
	bufffer.buffer = bufffer.buffer + 1024;

	ackMsg.header.flags = 0x01;

	MessageProcessor::EncapsulateAckNackMessage(bufffer, shift, ackMsg);

	bufffer.buffer_size = shift;
	shift = 0;
	MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix);

	messageTransmitter->MessageSend(srcLocators, unicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
	//TODO暂时屏蔽组播
	//messageTransmitter->MessageSend(srcLocators, multicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);

	return RETCODE_OK;
}


ReturnCode_t StatefulReader::SendNackFragMessage(std::shared_ptr<WriterProxy> writerProxy, NackFragSubMessage_t& nackMsg)
{
	if (!writerProxy) {
		PRINTF_ERROR();
		return RETCODE_ERROR;
	}

	LocatorSeq unicastList = writerProxy->GetUnicastLocatorList();
	LocatorSeq multicastList = writerProxy->GetMulticastLocatorList();

	LocatorVec srcLocators;
	Locator_t srcLocator;
	srcLocator.kind = LOCATOR_KIND_UDPv4;
	srcLocators.push_back(srcLocator);

	std::shared_ptr<MessageTransmitter> messageTransmitter = participant_->GetMessageTransmitter();

	if (!messageTransmitter) {
		PRINTF_ERROR();
		return RETCODE_ERROR;
	}
	char data[1500] = { 0 };
	SerializedBuffer bufffer;
	uint32_t shift = 0;
	bufffer.buffer_size = sizeof(AcknackSubMessage_t);
	bufffer.buffer = data;
	bufffer.buffer = bufffer.buffer + 1024;

	nackMsg.header.flags = 0x01;

	MessageProcessor::EncapsulateNackFragMessage(bufffer, shift, nackMsg);

	bufffer.buffer_size = shift;
	shift = 0;
	MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix);

	messageTransmitter->MessageSend(srcLocators, unicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
	//TODO暂时屏蔽组播
	//messageTransmitter->MessageSend(srcLocators, multicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);

	return RETCODE_OK;


}