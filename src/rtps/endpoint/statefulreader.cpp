#include "rtps/endpoint/statefulreader.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "rtps/endpoint/participant.h"
#include "rtps/messages/messageprocessor.h"
#include "rtps/transport/messagetransmitter.h"
#include <rtps/builtin/liveliness/livelinessmanager.h>
#include "common/log/logger.h"
#ifdef DDSSECURITY
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/publish/publisherimpl.h"
#include <iomanip>
#endif
USING_TRAVODDS_NAMESPACE;
StatefulReader::StatefulReader(const DataReaderQos& qos, const GUID_t& guid, const TopicKind_t& kind)
	: Reader(qos, kind)
{
	guid_ = guid;
	InitlastNewCacheTimer();
}

StatefulReader::~StatefulReader()
{
	if (lastNewCacheTimer_) {
		lastNewCacheTimer_->Exit();
	}
	
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
			TRAVODDS_LOG(LOG_ERROR, "MatchedWriterAdd Err");
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
			TRAVODDS_LOG(LOG_ERROR, "MatchedWriterRemove Err");
		}
	}
	/* wangyi 删除旧数据 */
	if (qos_.ownership.kind == EXCLUSIVE_OWNERSHIP_QOS) {
		readerCache_->RemoveRemoteWriter(writerProxy->GetGuid());
	}
	matchedWriters_.erase(writerProxy->GetGuid());
}

std::shared_ptr<WriterProxy> StatefulReader::MatchedWriterLookup(GUID_t writerGuid)
{
	if (0 == matchedWriters_.count(writerGuid)) {
		TRAVODDS_LOG(LOG_ERROR, "writerGuid not exist");
		return nullptr;
	}
		
	return matchedWriters_[writerGuid];
}

bool StatefulReader::MatchedWriterIsMatched(GUID_t writerGuid)
{
	if (0 == matchedWriters_.count(writerGuid)) {
		//AGILOG(AGILOG_ERROR, "writerGuid not exist");
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

	memcpy(pubHandle.keyHash.value, &guid, MIG_RTPS_KEY_HASH_MAX_LENGTH);


	/* 获取标志 */
	DataMsgFlag_t dataFlags;
	dataFlags.endiannessFlag = dataMsg.header.flags & SubMessageFlag::ENDIANNESS_FLAG;
	dataFlags.inlineQosFlag = dataMsg.header.flags & SubMessageFlag::INLINE_QOS_FLAG;
	dataFlags.dataFlag = dataMsg.header.flags & SubMessageFlag::DATA_FLAG;
	dataFlags.keyFlag = dataMsg.header.flags & SubMessageFlag::KEY_FLAG;
	dataFlags.nonStandardPayloadFlag = dataMsg.header.flags & SubMessageFlag::NON_STANDARD_PAYLOAD_FLAG;

	std::shared_ptr<WriterProxy> proxy = nullptr;
	auto itor = matchedWriters_.find(guid);
	if (itor != matchedWriters_.end()) {
		proxy = itor->second;
	}
	if (nullptr == proxy) {
		TRAVODDS_LOG(LOG_ERROR, "proxy = nullptr");
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

	/* wangyi 记录最新信息 */
	if (qos_.time_based_filter.minimum_separation != Duration_t(TIME_ZERO)) {
		lastNewCache_ = cache;
		lastNewGuid_ = guid;
	}

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
	dfFlags.endiannessFlag = dataMsg.header.flags & SubMessageFlag::ENDIANNESS_FLAG;
	dfFlags.inlineQosFlag = dataMsg.header.flags & SubMessageFlag::INLINE_QOS_FLAG;
	dfFlags.keyFlag = dataMsg.header.flags & SubMessageFlag::FRAG_KEY_FLAG;
	dfFlags.nonStandardPayloadFlag = dataMsg.header.flags & SubMessageFlag::FRAG_NON_STANDARD_PAYLOAD_FLAG;

	memcpy(pubHandle.keyHash.value, &guid, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	
	std::shared_ptr<WriterProxy> proxy = nullptr;
	auto itor = matchedWriters_.find(guid);
	if (itor != matchedWriters_.end()) {
		proxy = itor->second;
	}
	if (nullptr == proxy) {
		TRAVODDS_LOG(LOG_ERROR, "proxy = nullptr");
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
		TRAVODDS_LOG(LOG_ERROR, "DataFrag msg err ");
		return RETCODE_ERROR;
	}

	/* 重复接收数据, 不进行提交 */
	if (proxy->IsSubmitted(dataMsg.writerSN)) {
		return RETCODE_OK;
	}
	
	/* 数据不存在，并且没有被提交过 */
	if (!proxy->ExistCache(dataMsg.writerSN)) {

		SerializedBuffer *buffer = new SerializedBuffer();

		MemoryPool::GetInstance()->getBuffer(dataMsg.dataFrag.sampleSize + CDR_ENDIANNESS_LEN, buffer->origin_buffer);
		buffer->buffer = buffer->origin_buffer;
		/* 4字节为CDR信息 */
		buffer->buffer_size = dataMsg.dataFrag.sampleSize + CDR_ENDIANNESS_LEN;
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
		ReturnCode_t ret = proxy->AddReliableDataCache(dataFragCache, false);

		/* wangyi 记录最新信息 */
		if (qos_.time_based_filter.minimum_separation != Duration_t(TIME_ZERO)) {
			lastNewCache_ = dataFragCache;
			lastNewGuid_ = guid;
		}
	}

	proxy->AddFragData(dataMsg.writerSN, (char*)data, len, dataMsg.dataFrag.fragmentStartingNum, dataMsg.dataFrag.fragmentsInSubmessage);

	Notify(proxy);

	return RETCODE_OK;
}
ReturnCode_t StatefulReader::ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartMsg)
{
	GUID_t guid(headerInfo.sourceGuidPrefix, heartMsg.writerId);
	std::shared_ptr<WriterProxy> proxy = nullptr;
	HeartbeatMsgFlag_t flags;
	flags.endiannessFlag = heartMsg.header.flags & SubMessageFlag::ENDIANNESS_FLAG;
	flags.finalFlag = heartMsg.header.flags & SubMessageFlag::FINAL_FLAG;
	flags.livelinessFlag = heartMsg.header.flags & SubMessageFlag::LIVELINESS_FLAG;
	flags.groupInfoFlag = heartMsg.header.flags & SubMessageFlag::HB_GROUPINFO_FLAG;

	auto itor = matchedWriters_.find(guid);
	if (matchedWriters_.end() == itor) {
		//AGILOG(AGILOG_ERROR, "no match writer");
		return RETCODE_ERROR;
	}
	proxy = itor->second;
	if (!proxy) {
		TRAVODDS_LOG(LOG_ERROR, "match writer is nullptr");
		return RETCODE_ERROR;
	}

	/* TODO 调用函数前进行判断，报文的有效性判断 */
	//SequenceNumber_t SEQUENCENUMBER_ZERO
	if (heartMsg.firstSN <= SEQUENCENUMBER_ZERO || heartMsg.lastSN < SEQUENCENUMBER_ZERO || heartMsg.lastSN < heartMsg.firstSN - 1) {
		TRAVODDS_LOG(LOG_ERROR, "hb data is invalid");
		return RETCODE_ERROR;
	}
	if (flags.groupInfoFlag) {
		//TODO 相关QoS不进行实现
		if (heartMsg.currentGSN <= SEQUENCENUMBER_ZERO || heartMsg.firstGSN <= SEQUENCENUMBER_ZERO || heartMsg.lastGSN <= SEQUENCENUMBER_ZERO ||
			heartMsg.lastGSN < heartMsg.firstGSN - 1 || heartMsg.currentGSN > heartMsg.firstGSN || heartMsg.currentGSN < heartMsg.lastGSN) {
			TRAVODDS_LOG(LOG_ERROR, "hb groupInfoFlags data is invalid");
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
	if (set.bitmap.size != 0 || flags.finalFlag) {
		//TODO 存在丢包，或者强制回复Ack报文。创建Ack报文进行回复 
		/* 1. 先给分片数据发送NackFrag  */
		for (int32_t i = 0; i < nackFragVec.size(); i++) {
			nackFragVec[i].readerId = guid_.entityId;
			nackFragVec[i].writerId = heartMsg.writerId;
			nackFragVec[i].count = heartMsg.count;

			SendNackFragMessage(proxy, nackFragVec[i]);
		}	
	}

	if (set.bitmap.size != 0 || !flags.finalFlag) {
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
		TRAVODDS_LOG(LOG_ERROR, "no match writer exist");
		return RETCODE_ERROR;
	}
	proxy = itor->second;
	if (!proxy) {
		TRAVODDS_LOG(LOG_ERROR, "match writer is nullptr");
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

	if (set.bitmap.size != 0) {
		//TODO 存在丢包,回复nack报文 
		if(datareader_) {
			/* 存在丢包数据,触发监听回调函数 */
			datareader_->OnSampleLost(1);
		}
		
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
	bool groupInfoFlag = (gapInfo.header.flags & SubMessageFlag::GROUPINFO_FLAG) != SubMessageFlag::NONE_FLAG;
	bool filteredCountFlag = (gapInfo.header.flags & SubMessageFlag::FILTERED_COUNT_FLAG) != SubMessageFlag::NONE_FLAG;

	/*  判断报文是否有效  */
	SequenceNumber_t seq = SEQUENCENUMBER_ZERO;
	if (gapInfo.gapStart <= seq && (0 == gapInfo.gapList.bitmap.size || gapInfo.gapList.bitmap.size > 256)) {
		TRAVODDS_LOG(LOG_ERROR, "Gap GapData is invalid")
		return RETCODE_ERROR;
	}
	if (groupInfoFlag && (gapInfo.gapStartGSN <= seq || gapInfo.gapEndGSN <= seq || gapInfo.gapEndGSN < gapInfo.gapStart - 1) ) {
		TRAVODDS_LOG(LOG_ERROR, "Gap GroupInfo is invalid")
		return RETCODE_ERROR;
	}


	GUID_t guid(headerInfo.sourceGuidPrefix, gapInfo.writerId);
	auto itor = matchedWriters_.find(guid);
	if (matchedWriters_.end() == itor) {
		//AGILOG(AGILOG_ERROR, "no match writer")
		if (guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) {
			TRAVODDS_LOG(LOG_ERROR, "GetMacthList of statelessMessageWriter failed");
		}
		return RETCODE_ERROR;
	}
	std::shared_ptr<WriterProxy> proxy = itor->second;
	if (nullptr == proxy) {
		TRAVODDS_LOG(LOG_ERROR, "match writer is nullptr")
		return RETCODE_ERROR;
	}

	SeqNumberVec seqVec;
	int32_t i = 0;
	for (seq = gapInfo.gapStart; seq < gapInfo.gapList.bitmapBase; seq++) {
		seqVec.push_back(seq);
	}
	for (auto i = 0; i < gapInfo.gapList.bitmap.size; i++) {
		if (gapInfo.gapList.bitmap.IsBitSet(i) ) {
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
		TRAVODDS_LOG(LOG_ERROR, "this writerProxy is null");
		return RETCODE_ERROR;
	}
	std::shared_ptr<CacheChange> cache = proxy->GetSubmitCache();
	while (nullptr != cache) {
#ifdef DDSSECURITY
		DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(datareader_->getParent()->get_participant());
		GUID_t readerGuid = Conversion::ToGuid(datareader_->get_instance_handle());
		bool needDecodePayload = readerGuid.IsUserDefinedEntity() 
			&& datareader_->secAttributes_.is_payload_protected  // permissions配置负载加密
			&& dp->enableDDSSec_; // dp存在安全插件且安全配置完全
		SerializedBuffer* plainData = NULL;
		if (needDecodePayload) {
			plainData = new SerializedBuffer; //TODOMEM
			//plainData->buffer_size = cache->serializedBuffer->buffer_size;
			//plainData->buffer = new char[plainData->buffer_size];
			int len = cache->serializedBuffer->buffer_size;
			plainData->writer_position = len;
			plainData->buffer_size = len;
			MemoryPool::GetInstance()->getBuffer(len, plainData->origin_buffer);
			plainData->buffer = plainData->origin_buffer;

			SecurityException secExce;
			secExce.message[0] = '\0';
			WriterSampleList* matchWriter = nullptr;
			auto itor = datareader_->writerSampleList_.find(proxy->GetGuid());
			if (itor != datareader_->writerSampleList_.end()) {
				matchWriter = itor->second;
			}
			else {
				TRAVODDS_LOG(LOG_ERROR, "matched cryptoHdl of remoteWriter is null");
				return RETCODE_ERROR;
			}
			if (!dp->secPlugin_.crypto_transform->decode_serialized_payload(
				plainData,
				cache->serializedBuffer,
				1,
				NULL, // @todo: inline qos
				0,
				datareader_->cryptoHdl_,
				matchWriter->cryptoHdl,
				&secExce))
			{
				TRAVODDS_LOG(LOG_ERROR, "decode_serialized_payload failed");
				//delete[] plainData->buffer;
				MemoryPool::GetInstance()->releaseBuffer(plainData->origin_buffer);
				return RETCODE_ERROR;
			}
			plainData->writer_position = plainData->buffer_size;
			cache->serializedBuffer = plainData;
			// TODOMEM释放原先cache->serializedBuffer
		}

#endif // DDSSECURITY

		if(datareader_ ) {
			datareader_->SetLastinstanceHandle(cache);
		}
		ret = AddChanges(cache);

		if (ret == RETCODE_TIME_BASED_FILTER) {
			//wangyi time_based_filter 过滤判断,未到达数据提交时间，过滤数据。
			cache = proxy->GetSubmitCache();
			continue;
		}

		/* wangyi 没有数据不触发回调 */
		if (qos_.ownership.kind == EXCLUSIVE_OWNERSHIP_QOS) {
			if (ret == RETCODE_EXCLUSIVE_OWNERSHIP_QOS_NOT_MAX) {
				//wangyi ownership 过滤判断,非实例所有权数据写者发送的数据，过滤该数据。
				return RETCODE_EXCLUSIVE_OWNERSHIP_QOS_NOT_MAX;
			}
		}

		/* wangyi Lifespan 超期判断，超期过滤数据 */
		if (!datareader_->CheckLifespan(cache)) {
			if (readerGuid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER) {
				TRAVODDS_LOG(LOG_DEBUG, "statelessMessageReader CheckLifespan");
			}
			cache = proxy->GetSubmitCache();
			continue;
		}

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
			TRAVODDS_LOG(LOG_ERROR, "livelinessManager is nullptr");
		}
	}
	return RETCODE_OK;
}


ReturnCode_t StatefulReader::SendAckMessage(std::shared_ptr<WriterProxy> writerProxy, AcknackSubMessage_t& ackMsg)
{
	if (!writerProxy) {
		TRAVODDS_LOG(LOG_ERROR, "writerProxy is nullptr")
		return RETCODE_ERROR;
	}
	
	LocatorSeq unicastList = writerProxy->GetUnicastLocatorList();
	LocatorSeq multicastList = writerProxy->GetMulticastLocatorList();

	char data[1224] = { 0 }; //开辟消息空间
	SerializedBuffer bufffer;
	uint32_t shift = 0;
	bufffer.buffer_size = sizeof(AcknackSubMessage_t);
	bufffer.buffer = data;
	bufffer.buffer = bufffer.buffer + BUFF_SHIFT; //先指向data的1024偏移

	//加密前的子消息长度
	uint32_t lengthAckNackMsgBefore = 0;
	uint32_t lengthInfoMsgBefore = 0;
#ifdef DDSSECURITY
	SerializedBuffer tmpPlainBuffer;
	SerializedBuffer tmpEncodeBuffer; //临时存储加密后缓冲，拷贝到encodeData后释放
	char encodeData[1224 * 2] = { 0 }; //开辟加密消息空间
	SerializedBuffer encodeBuffer; //加密情况发送的buffer

	bool encodeSubMsg = datareader_->enableSec_
		&& datareader_->secAttributes_.is_submessage_protected
		&& guid_.IsUserDefinedEntity();
	bool encodeRtpsMsg = datareader_->enableSec_
		&& datareader_->rtpsMsgProtected_
		&& guid_.IsUserDefinedEntity();
	
	//加密后的子消息长度
	uint32_t lengthAckNackMsg = 0;
	uint32_t lengthInfoMsg = 0;
#endif // DDSSECURITY

	ackMsg.header.flags = (IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG) | SubMessageFlag::FINAL_FLAG;

	// 序列化AckNack, bufffer指向AckNack头部位置
	MessageProcessor::EncapsulateAckNackMessage(bufffer, shift, ackMsg); // 出参shift为AckNack消息长度
	lengthAckNackMsgBefore = shift;
	bufffer.buffer_size = shift;

#ifdef DDSSECURITY
	// 加密序列化的AckNack
	if (encodeSubMsg) {
		// tmpPlainBuffer指向AckNack明文缓存
		tmpPlainBuffer.buffer = bufffer.buffer;
		tmpPlainBuffer.buffer_size = bufffer.buffer_size;
		//开辟tmpEncodeBuffer作为AckNack加密后暂时缓存 
		tmpEncodeBuffer.buffer_size = tmpPlainBuffer.buffer_size * 10;
		tmpEncodeBuffer.buffer = new char[tmpEncodeBuffer.buffer_size];
		EncodeSubMsgBuffer(writerProxy, tmpPlainBuffer, tmpEncodeBuffer);
		// AckNack加密后缓存拷贝到encodeData末尾
		lengthAckNackMsg = tmpEncodeBuffer.buffer_size;
		memcpy(encodeData + 1224 * 2 - lengthAckNackMsg, tmpEncodeBuffer.buffer, lengthAckNackMsg);
		delete[] tmpEncodeBuffer.buffer;
	}
#endif // DDSSECURITY

	// bufffer当前指向AckNack开始位置，往前序列化Info
	/* wangyi test 添加INFODST */
	shift = 0;
	InfoDestinationSubMessage_t dstMsg;
	dstMsg.guidPrefix = writerProxy->GetGuid().prefix;
	dstMsg.header.flags = 0x01;
	MessageProcessor::EncapsulateINFODSTMessage(bufffer, shift, dstMsg); // 出参shift为Info长度
	lengthInfoMsgBefore = shift;

#ifdef DDSSECURITY
	// 加密序列化的Info
	uint32_t infoPlainLen = shift;
	tmpPlainBuffer.buffer_size = infoPlainLen; // tmpPlainBuffer指向Info消息明文
	tmpPlainBuffer.buffer = bufffer.buffer - infoPlainLen;
	
	if (encodeSubMsg) { // Info不加密
		// 开辟tmpEncodeBuffer作为Info加密后暂时缓存 
		tmpEncodeBuffer.buffer_size = tmpPlainBuffer.buffer_size;
		tmpEncodeBuffer.buffer = tmpPlainBuffer.buffer;
		// 拷贝到encodeData
		memcpy(encodeData + 1224 * 2 - lengthInfoMsg - lengthAckNackMsg,
			tmpEncodeBuffer.buffer,
			lengthInfoMsg);
	}
#endif // DDSSECURITY

#ifdef DDSSECURITY
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(datareader_->getParent()->get_participant());
	if (encodeSubMsg) {
		shift = 0;
		// tmpPlainBuffer指向加密后的消息(包含Info和AckNack)
		tmpPlainBuffer.buffer_size = lengthInfoMsg + lengthAckNackMsg;
		tmpPlainBuffer.buffer = encodeData + 1224 * 2 - lengthInfoMsg - lengthAckNackMsg;
		MessageProcessor::EncapsulateMessageHeader(tmpPlainBuffer, shift, guid_.prefix); //这里出参shift为rtpsHeader长度
	}
	else {
		// shift为Info长度，Encapsulate结束后为rtpsHeader长度+Info长度
		MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix); //这里出参shift为rtpsHeader长度+Info长度
	}

	if (encodeRtpsMsg) {
		DatawriterCryptoHandle* partHdls = NULL;
		uint32_t partHdlsCount = 0;
		if (writerProxy == NULL)
		{
			partHdls = dp->secParticipants_.data();
			partHdlsCount = dp->secParticipants_.size();
		}
		else
		{
			partHdls = &writerProxy->partCryptoHdl_;
			partHdlsCount = 1;
		}
		if (encodeSubMsg) { // 加密子消息情况下使用encodeData
			// tmpPlainBuffer指向消息（rtpsHeader+加密Info+加密AckNack）
			tmpPlainBuffer.buffer_size = lengthInfoMsg + lengthAckNackMsg + shift;
			tmpPlainBuffer.buffer = encodeData + 1224 * 2 - lengthInfoMsg - lengthAckNackMsg - shift;
		}
		else { // 不加密子消息情况下使用bufffer
			tmpPlainBuffer.buffer_size = lengthAckNackMsg + shift;
			tmpPlainBuffer.buffer = bufffer.buffer  - shift;
		}
		
		// tmpEncodeBuffer开辟对整个消息加密后空间
		tmpEncodeBuffer.buffer_size = tmpPlainBuffer.buffer_size * 10;
		tmpEncodeBuffer.buffer = new char[tmpEncodeBuffer.buffer_size];
		SecurityException secExce;
		if (datareader_->secPlugin_ && !datareader_->secPlugin_->crypto_transform->encode_rtps_message(
			&tmpEncodeBuffer,
			&tmpPlainBuffer,
			1,
			dp->cryptoHdl_,
			partHdls,
			partHdlsCount,
			&secExce)) {
			delete[]tmpEncodeBuffer.buffer;
			return -1;
		}
		// 存到encodeData
		memcpy(encodeData + 1224 * 2 - tmpEncodeBuffer.buffer_size, tmpEncodeBuffer.buffer, tmpEncodeBuffer.buffer_size);
		// encodeBuffer指向对应的数据
		encodeBuffer.buffer = encodeData + 1224 * 2 - tmpEncodeBuffer.buffer_size;
		encodeBuffer.buffer_size = tmpEncodeBuffer.buffer_size;
		delete[]tmpEncodeBuffer.buffer;
	}
	else { // 不Rtps加密
		if (encodeSubMsg) { // 加密子消息情况下使用encodeData
			encodeBuffer.buffer = encodeData + 1224 * 2 - lengthInfoMsg - lengthAckNackMsg - sizeof(MessageHeader_t);;
			encodeBuffer.buffer_size = lengthInfoMsg + lengthAckNackMsg + sizeof(MessageHeader_t);;
		}
	}
#else
	MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix); //这里出参shift为info+rtpsHeader长度
#endif 

#ifdef DDSSECURITY
	//MessageHeader_t + INFO + AckNack  buffer.buffer - shift是字符串 buffer.buffer_size + shift是总长度
	if (encodeSubMsg || encodeRtpsMsg) {
		participant_->TransmittersSendMessage(unicastList, encodeBuffer.buffer, encodeBuffer.buffer_size);
	}
	else {
		participant_->TransmittersSendMessage(unicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
	}
#else
	participant_->TransmittersSendMessage(unicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
#endif // DDSSECURITY
	//TODO暂时屏蔽组播
	//messageTransmitter->MessageSend(srcLocators, multicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);

	return RETCODE_OK;
}


ReturnCode_t StatefulReader::SendNackFragMessage(std::shared_ptr<WriterProxy> writerProxy, NackFragSubMessage_t& nackMsg)
{
	if (!writerProxy) {
		TRAVODDS_LOG(LOG_ERROR, "writerProxy is nullptr")
		return RETCODE_ERROR;
	}

	LocatorSeq unicastList = writerProxy->GetUnicastLocatorList();
	LocatorSeq multicastList = writerProxy->GetMulticastLocatorList();

	char data[1500] = { 0 }; //开辟消息空间
	SerializedBuffer bufffer;
	uint32_t shift = 0;
	bufffer.buffer_size = sizeof(AcknackSubMessage_t);
	bufffer.buffer = data;
	bufffer.buffer = bufffer.buffer + BUFF_SHIFT; //先指向data的1024偏移
	//加密前的子消息长度
	uint32_t lengthNackFragMsgBefore = 0;

#ifdef DDSSECURITY
	SerializedBuffer tmpPlainBuffer;
	SerializedBuffer tmpEncodeBuffer; //临时存储加密后缓冲，拷贝到encodeData后释放
	char encodeData[1500 * 2] = { 0 }; //开辟加密消息空间
	SerializedBuffer encodeBuffer; //加密情况发送的buffer
	bool encodeSubMsg = datareader_->enableSec_
		&& datareader_->secAttributes_.is_submessage_protected
		&& guid_.IsUserDefinedEntity();
	bool encodeRtpsMsg = datareader_->enableSec_
		&& datareader_->rtpsMsgProtected_
		&& guid_.IsUserDefinedEntity();
	//加密后的子消息长度
	uint32_t lengthNackFragMsg = 0;
#endif // DDSSECURITY

	nackMsg.header.flags = IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

	// 序列化NackFrag, bufffer指向Nack头部位置
	MessageProcessor::EncapsulateNackFragMessage(bufffer, shift, nackMsg); // 出参shift为Nack消息长度
	lengthNackFragMsgBefore = shift;
	bufffer.buffer_size = shift;
	shift = 0;

#ifdef DDSSECURITY
	// 加密序列化的Nack
	if (encodeSubMsg) {
		// tmpPlainBuffer指向Nack明文缓存
		tmpPlainBuffer.buffer = bufffer.buffer;
		tmpPlainBuffer.buffer_size = bufffer.buffer_size;
		//开辟tmpEncodeBuffer作为Nack加密后暂时缓存 
		tmpEncodeBuffer.buffer_size = tmpPlainBuffer.buffer_size * 10;
		tmpEncodeBuffer.buffer = new char[tmpEncodeBuffer.buffer_size];
		EncodeSubMsgBuffer(writerProxy, tmpPlainBuffer, tmpEncodeBuffer);
		// Nack加密后缓存拷贝到encodeData末尾
		lengthNackFragMsg = tmpEncodeBuffer.buffer_size;
		memcpy(encodeData + 1500 * 2 - lengthNackFragMsg, tmpEncodeBuffer.buffer, lengthNackFragMsg);
		delete[] tmpEncodeBuffer.buffer;
	}
#endif // DDSSECURITY

#ifdef DDSSECURITY
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(datareader_->getParent()->get_participant());
	if (encodeSubMsg) {
		// tmpPlainBuffer指向加密后的消息
		tmpPlainBuffer.buffer_size =  lengthNackFragMsg;
		tmpPlainBuffer.buffer = encodeData + 1500 * 2 - lengthNackFragMsg;
		MessageProcessor::EncapsulateMessageHeader(tmpPlainBuffer, shift, guid_.prefix); //这里出参shift为rtpsHeader长度
	}
	else {
		MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix);
	}
	if (encodeRtpsMsg) {
		DatawriterCryptoHandle* partHdls = NULL;
		uint32_t partHdlsCount = 0;
		if (writerProxy == NULL)
		{
			partHdls = dp->secParticipants_.data();
			partHdlsCount = dp->secParticipants_.size();
		}
		else
		{
			partHdls = &writerProxy->partCryptoHdl_;
			partHdlsCount = 1;
		}
		if (encodeSubMsg) { // 加密子消息情况下使用encodeData
			// tmpPlainBuffer指向消息（rtpsHeader+加密NackFrag）
			tmpPlainBuffer.buffer_size = lengthNackFragMsg + shift;
			tmpPlainBuffer.buffer = encodeData + 1500 * 2 - lengthNackFragMsg - shift;
		}
		else { // 不加密子消息情况下使用bufffer
			tmpPlainBuffer.buffer_size = lengthNackFragMsg + shift;
			tmpPlainBuffer.buffer = bufffer.buffer - shift;
		}

		// tmpEncodeBuffer开辟对整个消息加密后空间
		tmpEncodeBuffer.buffer_size = tmpPlainBuffer.buffer_size * 10;
		tmpEncodeBuffer.buffer = new char[tmpEncodeBuffer.buffer_size];
		SecurityException secExce;
		if (datareader_->secPlugin_ && !datareader_->secPlugin_->crypto_transform->encode_rtps_message(
			&tmpEncodeBuffer,
			&tmpPlainBuffer,
			1,
			dp->cryptoHdl_,
			partHdls,
			partHdlsCount,
			&secExce)) {
			delete[]tmpEncodeBuffer.buffer;
			return -1;
		}
		// 存到encodeData
		memcpy(encodeData + 1224 * 2 - tmpEncodeBuffer.buffer_size, tmpEncodeBuffer.buffer, tmpEncodeBuffer.buffer_size);
		// encodeBuffer指向对应的数据
		encodeBuffer.buffer = encodeData + 1500 * 2 - tmpEncodeBuffer.buffer_size;
		encodeBuffer.buffer_size = tmpEncodeBuffer.buffer_size;
		delete[]tmpEncodeBuffer.buffer;
	}
	else { // 不Rtps加密
		if (encodeSubMsg) { // 加密子消息情况下使用encodeData
			encodeBuffer.buffer = encodeData + 1500 * 2  - lengthNackFragMsg - sizeof(MessageHeader_t);;
			encodeBuffer.buffer_size = lengthNackFragMsg + sizeof(MessageHeader_t);;
		}
	}
#else
	MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix);
#endif

#ifdef DDSSECURITY
	if (encodeSubMsg || encodeRtpsMsg) {
		participant_->TransmittersSendMessage(unicastList, encodeBuffer.buffer, encodeBuffer.buffer_size);
	}
	else {
		participant_->TransmittersSendMessage(unicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
	}
#else
	participant_->TransmittersSendMessage(unicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
#endif // DDSSECURITY
	//TODO暂时屏蔽组播
	//messageTransmitter->MessageSend(srcLocators, multicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);

	return RETCODE_OK;


}

void StatefulReader::InitlastNewCacheTimer()
{
	/* wangyi TIME_BASED_FILTER 最后一包数据定时器 */
	if (qos_.time_based_filter.minimum_separation != Duration_t(TIME_ZERO) && qos_.reliability.kind == RELIABLE_RELIABILITY_QOS) {
		Time_t time = TIME_INFINITE;
		lastNewCacheTimer_ = TimerManager::CreateTimer(time, [&]() {this->TimerExpired(); }, "lastNewCacheTimer");
	}
}

void StatefulReader::TimerExpired()
{
	std::shared_ptr<WriterProxy> proxy = nullptr;
	auto itor = matchedWriters_.find(lastNewGuid_);
	if (itor != matchedWriters_.end()) {
		proxy = itor->second;
	}
	if (nullptr == proxy) {
		return;
	}

	ReturnCode_t ret = AddChanges(lastNewCache_);

	if (ret == RETCODE_TIME_BASED_FILTER) {
		//wangyi time_based_filter 过滤判断,未到达数据提交时间，过滤数据。
		return;
	}

	/* wangyi 没有数据不触发回调 */
	if (qos_.ownership.kind == EXCLUSIVE_OWNERSHIP_QOS) {
		if (ret == RETCODE_EXCLUSIVE_OWNERSHIP_QOS_NOT_MAX) {
			//wangyi ownership 过滤判断,非实例所有权数据写者发送的数据，过滤该数据。
			return ;
		}
	}

	if (RETCODE_OK != ret && datareader_) {
		datareader_->OnSampleRejected(lastNewCache_->instanceHandle, (SampleRejectedStatusKind)ret);
	}

	//TODO 提交数据 
	if (readerListener_) {
		readerListener_->on_data_available(lastNewCache_);
	}
	else if (datareader_) {
		datareader_->OnDataAvaible();
	}

	/* 最新数据提交后重置等待时间 */
	Duration_t time = TIME_INFINITE;
	if (lastNewCacheTimer_ != nullptr) {
		lastNewCacheTimer_->UpdatePeriod(time);
	}
}

#ifdef DDSSECURITY
int StatefulReader::EncodeSubMsgBuffer(std::shared_ptr<WriterProxy> writerProxy, SerializedBuffer& buffer, SerializedBuffer& encodeBuffer)
{
	if (datareader_->enableSec_) {
		GUID_t writerGuid = writerProxy->GetGuid();
		if (datareader_->secAttributes_.is_submessage_protected) {
			SecurityException secExce;
			DatawriterCryptoHandle* writerHdls = NULL;
			uint32_t writerHdlsCount = 0;
			if (writerProxy == NULL)
			{
				writerHdls = datareader_->secWriters_.data();
				writerHdlsCount = datareader_->secWriters_.size();
			}
			else
			{
				writerHdls = &writerProxy->cryptoHdl_;
				writerHdlsCount = 1;
			}
			if (!datareader_->secPlugin_->crypto_transform->encode_datareader_submessage(
				&encodeBuffer,
				&buffer,
				1,
				datareader_->cryptoHdl_,
				writerHdls,
				writerHdlsCount,
				&secExce))
			{
				TRAVODDS_LOG(LOG_ERROR, "Failed to encode submessage."
					"\n  Code: %d"
					"\n  Minor code: %d"
					"\n  Message: %s",
					secExce.code,
					secExce.minor_code,
					secExce.message);
				return -1;
			}
		}
	}
	return 0;
}
#endif // DDSSECURITY
