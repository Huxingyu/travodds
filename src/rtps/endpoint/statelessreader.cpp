#include "rtps/endpoint/statelessreader.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "rtps/endpoint/participant.h"
#include <rtps/builtin/liveliness/livelinessmanager.h>
#include "common/log/logger.h"

USING_AGIDDS_NAMESPACE;


StatelessReader::StatelessReader(const DataReaderQos & qos, const GUID_t & guid, const TopicKind_t& kind)
	: Reader(qos, kind)
{
	guid_ = guid;
}

StatelessReader::~StatelessReader()
{
	WriterProxyMap writerProxys = matchWriters_;

	for (auto & writerProxy : writerProxys) {
		MatchedWriterRemove(writerProxy.first);
	}

	writerProxys.clear();
}

ReturnCode_t StatelessReader::ReceiveDataMessage(const Receive_t&  headerMsg, const DataSubMessage_t& dataMsg, SerializedPayload data, uint32_t len)
{
	ChangeKind_t kind = ALIVE;
	SerializedBuffer* buffer = nullptr;
	GUID_t guid = { headerMsg.sourceGuidPrefix, dataMsg.writerId };
	InstanceHandle_t pubHandle;
	InstanceHandle_t dataHandle = HANDLE_NIL;

	/* wangyi liveliness */
	AssertWriterLiveliness(guid);

	memcpy(pubHandle.keyHash.value, &guid, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	buffer = new SerializedBuffer;

	/* 获取标志 */
	DataMsgFlag_t dataFlags;
	dataFlags.endiannessFlag = dataMsg.header.flags & SubMessageFlag::ENDIANNESS_FLAG;
	dataFlags.inlineQosFlag = dataMsg.header.flags & SubMessageFlag::INLINE_QOS_FLAG;
	dataFlags.dataFlag = dataMsg.header.flags & SubMessageFlag::DATA_FLAG;
	dataFlags.keyFlag = dataMsg.header.flags & SubMessageFlag::KEY_FLAG;
	dataFlags.nonStandardPayloadFlag = dataMsg.header.flags & SubMessageFlag::NON_STANDARD_PAYLOAD_FLAG;


	if (dataFlags.inlineQosFlag) {
		ProcessInlineQos(kind, dataHandle, dataMsg.inlineQos);
	}

	if (!dataFlags.nonStandardPayloadFlag && nullptr != data) {

 		buffer->writer_position = len;
		buffer->buffer_size = len;
		MemoryPool::GetInstance()->getBuffer(len, buffer->origin_buffer);
		buffer->buffer = buffer->origin_buffer;
		memcpy(buffer->buffer, data, len);
	}

	std::shared_ptr<CacheChange> cache = new_change(kind, buffer, dataHandle, pubHandle, headerMsg.timestamp);
	cache->sequenceNumber = dataMsg.writerSN;

	//QOS处理
	//TODO

	if (readerListener_) {
		readerListener_->on_data_available(cache);
	}

		
	ReturnCode_t ret = AddChanges(cache);

	if (ret == RETCODE_TIME_BASED_FILTER) {
		//wangyi time_based_filter 过滤判断,未到达数据提交时间，过滤数据。
		return RETCODE_TIME_BASED_FILTER;
	}

	if (RETCODE_OK != ret && datareader_) {
		datareader_->OnSampleRejected(cache->instanceHandle, (SampleRejectedStatusKind)ret);
	}

	if (datareader_) {
		datareader_->SetLastinstanceHandle(cache);
		datareader_->OnDataAvaible();
	}
	
	return RETCODE_OK;
}

ReturnCode_t StatelessReader::ReceiveDataFragMessage(const Receive_t&  headerMsg, const DataFragSubMessage_t& dataMsg, SerializedPayload data, uint32_t len)
{
	/* wangyi 更新存活性 */
	GUID_t guid = { headerMsg.sourceGuidPrefix, dataMsg.writerId };
	AssertWriterLiveliness(guid);
	ChangeKind_t kind = ALIVE;
	SerializedBuffer* buffer = nullptr;
	InstanceHandle_t pubHandle;
	InstanceHandle_t dataHandle = HANDLE_NIL;

	memcpy(pubHandle.keyHash.value, &guid, MIG_RTPS_KEY_HASH_MAX_LENGTH);

	/* 获取标志 */
	DataFragMsgFlag_t dataFragFlags;
	dataFragFlags.endiannessFlag = dataMsg.header.flags & SubMessageFlag::ENDIANNESS_FLAG;
	dataFragFlags.inlineQosFlag = dataMsg.header.flags & SubMessageFlag::INLINE_QOS_FLAG;
	dataFragFlags.keyFlag = dataMsg.header.flags & SubMessageFlag::FRAG_KEY_FLAG;
	dataFragFlags.nonStandardPayloadFlag = dataMsg.header.flags & SubMessageFlag::FRAG_NON_STANDARD_PAYLOAD_FLAG;


	if (dataFragFlags.inlineQosFlag) {
		ProcessInlineQos(kind, dataHandle, dataMsg.inlineQos);
	}
	if (dataFragFlags.nonStandardPayloadFlag || nullptr == data) {	
		//TODO 
		return RETCODE_ERROR;
	}
	auto itor = dataFragCaches_.find(guid);
	if (itor == dataFragCaches_.end() || itor->second->sequenceNumber < dataMsg.writerSN) {
		if (itor != dataFragCaches_.end()) {
			/* 收到新的分片数据,并且新的数据序号大于之前的数据，BEST_EFFORT 模式下删除旧数据 */
			dataFragCaches_.erase(itor);
		}
		/* 申请空间 */
		buffer = new SerializedBuffer();

		MemoryPool::GetInstance()->getBuffer(dataMsg.dataFrag.sampleSize + CDR_ENDIANNESS_LEN, buffer->origin_buffer);
		buffer->buffer = buffer->origin_buffer;
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

		dataFragCaches_.emplace(guid, dataFragCache);
	}

	/* 找到对应的SerializedBuffer */
	buffer = itor->second->serializedBuffer;
	uint32_t shift = 0;

	/* 判断当前分片数据是否收到，没有收到进行封装  */
	if (0 != itor->second->fragmentSeq.count(dataMsg.dataFrag.fragmentStartingNum)) {
		shift += (dataMsg.dataFrag.fragmentStartingNum - 1) * dataMsg.dataFrag.fragmentSize;
		memcpy(buffer->buffer + shift, data, len);
		itor->second->fragmentSeq.erase(dataMsg.dataFrag.fragmentStartingNum);
	}
	else {
		//TODO  
		/* 重复接收分片数据 */
		std::cout << "recv repeat dataFrg " << __FUNCTION__ << " " << __LINE__ << std::endl;
		AGILOG(AGILOG_ERROR, "recv repeat dataFrg");
	}

	if (0 == itor->second->fragmentSeq.size()) {
		std::shared_ptr<CacheChange> dataFragCache = itor->second;
		dataFragCaches_.erase(itor);

		ReturnCode_t ret = AddChanges(dataFragCache);

		if (ret == RETCODE_TIME_BASED_FILTER) {
			//wangyi time_based_filter 过滤判断,未到达数据提交时间，过滤数据。
			return RETCODE_TIME_BASED_FILTER;
		}

		if (RETCODE_OK != ret && datareader_) {
			datareader_->OnSampleRejected(dataFragCache->instanceHandle, (SampleRejectedStatusKind)ret);
		}

		if (datareader_) {
			datareader_->SetLastinstanceHandle(dataFragCache);
			datareader_->OnDataAvaible();
		}
	}

	return RETCODE_OK;
}

ReturnCode_t StatelessReader::ReceiveHeartbeatMessage(const Receive_t& headerInfo, const HeartBeatSubMessage_t& heartMsg)
{
	static_cast<void>(headerInfo);
	static_cast<void>(heartMsg);
	return RETCODE_UNSUPPORTED;
}

ReturnCode_t StatelessReader::ReceiveHeartFragMessage(const Receive_t& headerInfo, const HeartBeatFragSubMessage_t& heartFragMsg)
{
	static_cast<void>(headerInfo);
	static_cast<void>(heartFragMsg);
	return RETCODE_UNSUPPORTED;
}

//ReturnCode_t ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartInfo);

ReturnCode_t StatelessReader::ReceiveGapMessage(const Receive_t&  headerInfo, const GapSubMessage_t& gapInfo)
{
	static_cast<void>(headerInfo);
	static_cast<void>(gapInfo);
	return RETCODE_UNSUPPORTED;
}


void StatelessReader::MatchedWriterAdd(std::shared_ptr<WriterProxy> writerData)
{
	matchWriters_[writerData->GetGuid()] = writerData;
	/* wangyi */
	Duration_t duration = TIME_INFINITE;
	if (qos_.liveliness.lease_duration < duration) {
		if (writerData->GetGuid() == this->GetGuid()) {
			return;
		}

		auto livelinessManager = participant_->GetLivelinessManager();
		if (livelinessManager != nullptr) {
			livelinessManager->GetRemoteWriterLivelinessManager()->AddWriter(LivelinesStatus(writerData->GetGuid(), qos_.liveliness));
		}
		else {
			/* TODO 日志 */
			AGILOG(AGILOG_ERROR, "livelinessManager is nullptr");
		}
	}
}

bool StatelessReader::MatchedWriterIsMatched(GUID_t writerGuid)
{
	if (0 == matchWriters_.count(writerGuid)) {
		return false;
	}
	return true;
}


void StatelessReader::MatchedWriterRemove(GUID_t writerGuid)
{
	/* wangyi */
	Duration_t duration = TIME_INFINITE;
	if (qos_.liveliness.lease_duration < duration) {
		auto livelinessManager = participant_->GetLivelinessManager();
		if (livelinessManager != nullptr) {
			livelinessManager->GetRemoteWriterLivelinessManager()->RemoveWriter(writerGuid);
		}
		else {
			/* TODO 日志 */
			AGILOG(AGILOG_ERROR, "livelinessManager is nullptr");
		}
	}
	matchWriters_.erase(writerGuid);
}

std::shared_ptr<WriterProxy> StatelessReader::MatchedWriterLookup(GUID_t writerGuid)
{
	auto itor = matchWriters_.find(writerGuid);
	if (itor == matchWriters_.end()) {
		AGILOG(AGILOG_ERROR, "writerGuid not match");
		return nullptr;
	}
	return itor->second;
}

/* wangyi */
void StatelessReader::AssertWriterLiveliness(const GUID_t& guid)
{
	Duration_t duration = TIME_INFINITE;
	if (qos_.liveliness.lease_duration < duration) {
		auto livelinessManager = participant_->GetLivelinessManager();
		if (livelinessManager != nullptr) {
			LivelinessQosPolicyKind kind;
			if (livelinessManager->GetRemoteWriterLivelinessManager()->GetWriterLiveLinessKind(guid, kind)) {
				livelinessManager->GetRemoteWriterLivelinessManager()->AssertLiveliness(guid, kind);
			}
		}
		else {
			/* TODO 日志 "Finite liveliness lease duration but LivelinessManager not enabled"); */
			AGILOG(AGILOG_ERROR, "Finite liveliness lease duration but LivelinessManager not enabled");
		}
	}
}

