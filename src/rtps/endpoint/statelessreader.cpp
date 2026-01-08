#include "rtps/endpoint/statelessreader.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "rtps/endpoint/participant.h"
#include <rtps/builtin/liveliness/livelinessmanager.h>
#include "common/log/logger.h"
#include "dcps/domain/domainparticipantimpl.h"
#ifdef DDSSECURITY
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/publish/publisherimpl.h"
#endif
USING_TRAVODDS_NAMESPACE;


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

ReturnCode_t StatelessReader::ReceiveDataMessage(const Receive_t&  headerMsg, const DataSubMessage_t& dataMsg, const SerializedPayload& data)
{
	std::lock_guard<std::mutex> guard(dataMutex_);
	ChangeKind_t kind = ALIVE;
	CacheSerializedBuffer* buffer = nullptr;
	GUID_t guid = { headerMsg.sourceGuidPrefix, dataMsg.writerId };
	InstanceHandle_t pubHandle;
	InstanceHandle_t dataHandle = HANDLE_NIL;

	/* wangyi liveliness */
	AssertWriterLiveliness(guid);

	memcpy(pubHandle.keyHash.value, &guid, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	buffer = new CacheSerializedBuffer;

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

	if (!dataFlags.nonStandardPayloadFlag && nullptr != data.data) {

 		buffer->writer_position = data.length;
		buffer->buffer_size = data.length;
		if (data.originMessage->onlyCopy){
		    /* 只允许拷贝，不允许获得所有权 */
			MemoryPool::GetInstance()->getBuffer(data.length, buffer->origin_buffer);
			buffer->buffer = buffer->origin_buffer;
			memcpy(buffer->buffer, data.data, data.length);
		} else {
		    buffer->buffer = data.data;
			buffer->originMessage = data.originMessage;
		}

	}

#ifdef DDSSECURITY
	//负载解密
	GUID_t readerGuid = Conversion::ToGuid(datareader_->get_instance_handle());
	SerializedBuffer* plainData = new SerializedBuffer;
	plainData->writer_position = data.length;
	plainData->buffer_size = data.length;
	MemoryPool::GetInstance()->getBuffer(data.length, plainData->origin_buffer);
	plainData->buffer = plainData->origin_buffer;
	if (readerGuid.IsUserDefinedEntity() && datareader_->secAttributes_.is_payload_protected) {
		SecurityException secExce;
		secExce.message[0] = '\0';
		WriterSampleList* matchWriter = nullptr;
		auto itor = datareader_->writerSampleList_.find(guid);
		if (itor != datareader_->writerSampleList_.end()) {
			matchWriter = itor->second;
		}
		else {
			MemoryPool::GetInstance()->releaseBuffer(plainData->origin_buffer);
			TRAVODDS_LOG(LOG_ERROR, "cryptoHdl of WriterSampleList matchWriter = nullptr");
			return -1;
		}
		DomainParticipantImpl* participant = dynamic_cast<DomainParticipantImpl*>(datareader_->getParent()->get_participant());
		if (!participant->secPlugin_.crypto_transform->decode_serialized_payload(
			plainData,
			buffer,
			1,
			NULL, // @todo: inline qos
			0,
			datareader_->cryptoHdl_,
			//proxy->m_cryptoHdl,
			matchWriter->cryptoHdl,
			&secExce))
		{
			//delete[] plainData->buffer;
			MemoryPool::GetInstance()->releaseBuffer(plainData->origin_buffer);
			return -4;
		}
		plainData->writer_position = plainData->buffer_size;
		MemoryPool::GetInstance()->releaseBuffer(buffer->origin_buffer);
	}
	std::shared_ptr<CacheChange> cache;
	if (readerGuid.IsUserDefinedEntity() && datareader_->secAttributes_.is_payload_protected) {
		cache = new_change(kind, plainData, dataHandle, pubHandle, headerMsg.timestamp);
	}
	else {
		cache = new_change(kind, buffer, dataHandle, pubHandle, headerMsg.timestamp);
	}
#else
	std::shared_ptr<CacheChange> cache = new_change(kind, buffer, dataHandle, pubHandle, headerMsg.timestamp);
#endif // DDSSECURITY

	cache->sequenceNumber = dataMsg.writerSN;

	//QOS处理
	//TODO
	if (datareader_) {
		datareader_->SetLastinstanceHandle(cache);

		int32_t lostcount = datareader_->GetInstanceLostcounts(cache);
		if (lostcount > 0) {
			datareader_->OnSampleLost(lostcount);
		}
		if(datareader_->ValidateFilterData(cache->serializedBuffer)){
		    return RETCODE_OK;
		};
	}

	if (readerListener_) {
		readerListener_->on_data_available(cache);
	}

		
	ReturnCode_t ret = AddChanges(cache);

	if (ret == RETCODE_TIME_BASED_FILTER) {
		//wangyi time_based_filter 过滤判断,未到达数据提交时间，过滤数据。
		return RETCODE_TIME_BASED_FILTER;
	}

	/* wangyi 没有数据不触发回调 */
	if (qos_.ownership.kind == EXCLUSIVE_OWNERSHIP_QOS) {
		if (ret == RETCODE_EXCLUSIVE_OWNERSHIP_QOS_NOT_MAX) {
			//wangyi ownership 过滤判断,非实例所有权数据写者发送的数据，过滤该数据。
			return RETCODE_EXCLUSIVE_OWNERSHIP_QOS_NOT_MAX;
		}
	}

	if (RETCODE_OK != ret && datareader_) {
		datareader_->OnSampleRejected(cache->instanceHandle, (SampleRejectedStatusKind)ret);
	}

	if (datareader_) {
		/* wangyi Lifespan 超期判断，超期过滤数据 */
		if (!datareader_->CheckLifespan(cache)) {
			return RETCODE_LIFESPAN_TIMEOUT;
		}
		datareader_->OnDataAvaible();
	}
	
	return RETCODE_OK;
}

ReturnCode_t StatelessReader::ReceiveDataFragMessage(const Receive_t&  headerMsg, const DataFragSubMessage_t& dataMsg, const SerializedPayload& data)
{
	std::lock_guard<std::mutex> guard(dataMutex_);
	/* wangyi 更新存活性 */
	GUID_t guid = { headerMsg.sourceGuidPrefix, dataMsg.writerId };
	AssertWriterLiveliness(guid);


	ChangeKind_t kind = ALIVE;
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
	if (dataFragFlags.nonStandardPayloadFlag || nullptr == data.data) {	
		//TODO 
		return RETCODE_ERROR;
	}
	auto itor = dataFragCaches_.find(guid);
	if (itor == dataFragCaches_.end() || itor->second->sequenceNumber < dataMsg.writerSN) {
		if (itor != dataFragCaches_.end()) {
			/* 收到新的分片数据,并且新的数据序号大于之前的数据，BEST_EFFORT 模式下删除旧数据 */
			dataFragCaches_.erase(itor);
		}
		///* 申请空间 */

		std::shared_ptr<CacheChange> dataFragCache = new_change(kind, nullptr, dataHandle, pubHandle, headerMsg.timestamp);

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

		itor = dataFragCaches_.emplace(guid, dataFragCache).first;
	}
	/* 找到对应的SerializedBuffer */
	//buffer = itor->second->serializedBuffer;

	/* 判断当前分片数据是否收到，没有收到进行封装  */
	if (itor->second->fragmentSeq.erase(dataMsg.dataFrag.fragmentStartingNum) > 0) {
		CacheSerializedBuffer* buffer = new CacheSerializedBuffer();
		buffer->writer_position = data.length;
		buffer->buffer_size = data.length;
		if (data.originMessage->onlyCopy) {
			/* 只允许拷贝，不允许获得所有权 */
			MemoryPool::GetInstance()->getBuffer(data.length, buffer->origin_buffer);
			buffer->buffer = buffer->origin_buffer;
			memcpy(buffer->buffer, data.data, data.length);
		} else {
			buffer->buffer = data.data;
			buffer->originMessage = data.originMessage;
		}
		itor->second->fragmentBufferMap.insert({dataMsg.dataFrag.fragmentStartingNum, buffer});
	} else {
		//TODO  
		/* 重复接收分片数据 */
		// std::cout << "recv repeat dataFrg " << __FUNCTION__ << " " << __LINE__ << std::endl;
		TRAVODDS_LOG(LOG_ERROR, "recv repeat dataFrg");
	}

	// yuguxu 移除其他包含的所有分片号
	for (uint32_t i = dataMsg.dataFrag.fragmentStartingNum + 1; i < dataMsg.dataFrag.fragmentStartingNum + dataMsg.dataFrag.fragmentsInSubmessage  ; i++) {
		itor->second->fragmentSeq.erase(i);
	}

	if (itor->second->fragmentSeq.empty()) {
		std::shared_ptr<CacheChange> dataFragCache = itor->second;
		/* 串联所有分片 */
		for (auto fragItor = dataFragCache->fragmentBufferMap.begin(); fragItor != std::prev(dataFragCache->fragmentBufferMap.end()); fragItor++) {
			fragItor->second->next = std::next(fragItor)->second;
		}
		dataFragCache->serializedBuffer = dataFragCache->fragmentBufferMap.begin()->second;
		dataFragCache->fragmentBufferMap.clear();
		dataFragCaches_.erase(itor);

#ifdef DDSSECURITY
		//dataFragCache的SerializedBuffer* serializedBuffer负载解密
        GUID_t readerGuid = Conversion::ToGuid(datareader_->get_instance_handle());
        bool needDecodePayload = readerGuid.IsUserDefinedEntity() && datareader_->secAttributes_.is_payload_protected;
        SerializedBuffer *plainData = NULL;
        if (needDecodePayload) {
            //plainData = new SerializedBuffer;
            //plainData->buffer_size = dataFragCache->serializedBuffer->buffer_size;
            //plainData->buffer = new char[plainData->buffer_size];
			plainData = new SerializedBuffer;
			plainData->writer_position = dataFragCache->serializedBuffer->buffer_size;
			plainData->buffer_size = dataFragCache->serializedBuffer->buffer_size;
			MemoryPool::GetInstance()->getBuffer(dataFragCache->serializedBuffer->buffer_size, plainData->origin_buffer);
			plainData->buffer = plainData->origin_buffer;
            SecurityException secExce;
            secExce.message[0] = '\0';
            WriterSampleList* matchWriter = nullptr;
            auto itor = datareader_->writerSampleList_.find(guid);
            if (itor != datareader_->writerSampleList_.end()) {
                matchWriter = itor->second;
            }
            else {
				MemoryPool::GetInstance()->releaseBuffer(plainData->origin_buffer);
				TRAVODDS_LOG(LOG_ERROR, "m_cryptoHdl of WriterSampleList matchWriter = nullptr");
                return -1;
            }
			DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(datareader_->getParent()->get_participant());
            if (!dp->secPlugin_.crypto_transform->decode_serialized_payload(
                plainData,
                dataFragCache->serializedBuffer,
                1,
                NULL, // @todo: inline qos
                0,
                datareader_->cryptoHdl_,
                //proxy->m_cryptoHdl,
                matchWriter->cryptoHdl,
                &secExce))
            {
                //delete[] plainData->buffer;
				MemoryPool::GetInstance()->releaseBuffer(plainData->origin_buffer);
                return -4;
            }
            plainData->writer_position = plainData->buffer_size;
			//MEM 替换之前释放原先的
			MemoryPool::GetInstance()->releaseBuffer(dataFragCache->serializedBuffer->origin_buffer);
            dataFragCache->serializedBuffer = plainData;
        }
#endif // DDSSECURITY
		if(datareader_) {
			datareader_->SetLastinstanceHandle(dataFragCache);

			int32_t lostcount = datareader_->GetInstanceLostcounts(dataFragCache);
			if (lostcount > 0) {
				datareader_->OnSampleLost(lostcount);
			}
			if (datareader_->ValidateFilterData(dataFragCache->serializedBuffer)) {
				return RETCODE_OK;
			};
		}

		ReturnCode_t ret = AddChanges(dataFragCache);

		if (ret == RETCODE_TIME_BASED_FILTER) {
			//wangyi time_based_filter 过滤判断,未到达数据提交时间，过滤数据。
			return RETCODE_TIME_BASED_FILTER;
		}

		/* wangyi 没有数据不触发回调 */
		if (qos_.ownership.kind == EXCLUSIVE_OWNERSHIP_QOS) {
			if (ret == RETCODE_EXCLUSIVE_OWNERSHIP_QOS_NOT_MAX) {
				//wangyi ownership 过滤判断,非实例所有权数据写者发送的数据，过滤该数据。
				return RETCODE_EXCLUSIVE_OWNERSHIP_QOS_NOT_MAX;
			}
		}

		if (RETCODE_OK != ret && datareader_) {
			datareader_->OnSampleRejected(dataFragCache->instanceHandle, (SampleRejectedStatusKind)ret);
		}

		if (datareader_) {
			/* wangyi Lifespan 超期判断，超期过滤数据 */
			if (!datareader_->CheckLifespan(dataFragCache)) {
				return RETCODE_LIFESPAN_TIMEOUT;
			}
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
			TRAVODDS_LOG(LOG_ERROR, "livelinessManager is nullptr");
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
			TRAVODDS_LOG(LOG_ERROR, "livelinessManager is nullptr");
		}
	}
	/* wangyi 删除旧数据 */
	if (qos_.ownership.kind == EXCLUSIVE_OWNERSHIP_QOS) {
		readerCache_->RemoveRemoteWriter(writerGuid);
	}
	matchWriters_.erase(writerGuid);
}

std::shared_ptr<WriterProxy> StatelessReader::MatchedWriterLookup(GUID_t writerGuid)
{
	auto itor = matchWriters_.find(writerGuid);
	if (itor == matchWriters_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "writerGuid not match");
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
			TRAVODDS_LOG(LOG_ERROR, "Finite liveliness lease duration but LivelinessManager not enabled");
		}
	}
}

