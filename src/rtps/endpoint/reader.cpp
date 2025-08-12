#include "rtps/endpoint/reader.h"
#include "rtps/endpoint/participant.h"
#include "common/log/logger.h"
#include "common/timeconversion.h"
#include "dcps/subscribe/datareaderimpl.h"
USING_AGIDDS_NAMESPACE;

Reader::Reader(const DataReaderQos& qos, const TopicKind_t& kind)
	: qos_(qos),
	  heartbeatResponseDelay_(TIME_INVALID),
	  heartbeatSuppressionDuration_(TIME_INVALID),
	  readerCache_(std::make_shared<HistoryCache>(qos.history, qos.resource_limits, kind)),
	  expectsInlineQos_(false),
	  lastSequenceNumber_(SEQUENCENUMBER_ZERO),
	  readerListener_(nullptr),
	  datareader_(nullptr)
{
	if (BEST_EFFORT == qos_.reliability.kind) {
		reliabilityLevel_ = BEST_EFFORT;
	}	
	else {
		reliabilityLevel_ = RELIABLE;
	}
		
	instanceHandles_.push_back(HANDLE_NIL);
	lastTime_ = { 0,0 };
}

Reader::~Reader()
{
}
ReturnCode_t Reader::ReadOrTake(SerializedBufferSeq& dataValues, SampleInfoSeq& sampleInfos, uint32_t maxSamples,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake)
{
	InstanceHandle_t handle = HANDLE_NIL;
	InstanceHandle_t nextHandle = HANDLE_NIL;
	ReturnCode_t ret = RETCODE_OK;
	
	/* 循环读取所有的Instance的数据 */
	do {
		if (dataValues.length() >= maxSamples) {
			break;
		}

		ret = ReadOrTakeInstance(dataValues, sampleInfos, maxSamples, nextHandle, sampleStates, viewStates, instanceStates, shouldTake);

		handle = nextHandle;
		ret = FindNextInstance(handle, nextHandle);	

	} while (RETCODE_ERROR != ret);

	if (0 >= dataValues.length()) {
		return RETCODE_NO_DATA;
	}
		
	return RETCODE_OK;
}

ReturnCode_t Reader::ReadOrTakeInstance(SerializedBufferSeq & dataValues, SampleInfoSeq & sampleInfos, uint32_t maxSamples, InstanceHandle_t handle,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake)
{
	GetRelatedDataReader()->SetStatus(false);
	ReturnCode_t ret = RETCODE_OK;
	/* 实例样本最新的状态  */
	SampleInfo info = sampleInfos_[handle];
	/* 实例样本在缓冲区最新数据的样本 */
	SampleInfo handleInfo;
	/* 实例样本读取到的最新状态 */
	SampleInfo readInfo;
	int32_t size = 0;
	/* 读取 */
	//if((0 == (viewStates & info.view_state))|| (0 == (instanceStates & info.instance_state)) ){
	//	return RETCODE_NO_DATA;
	//}

	ret = readerCache_->ReadOrTakeInstance(dataValues, sampleInfos, maxSamples, handle, sampleStates, viewStates, instanceStates, shouldTake);
	if (RETCODE_OK != ret) {
		return ret;
	}

	/* 修改状态  */
	size = sampleInfos.length();

	if (0 > size) {
		readInfo = sampleInfos[size - 1];
	}
	handleInfo = GetLastInstanceSampleInfo(handle);

	for (int32_t i = 0; i < sampleInfos.length(); i++) {
		SampleInfo sampleInfo = sampleInfos[i];

		/*  从缓冲区读取时赋值状态  */
		//sampleInfo.sample_state;
		/*  在申明Cachce空间时，获取当前Instancehandle的SampleInfo的相关信息时赋值  */
		//sampleInfo.source_timestamp;
		//sampleInfo.instance_handle;
		//sampleInfo.publication_handle;
		//sampleInfo.disposed_generation_count;
		//sampleInfo.no_writers_generation_count;
		
		/* 更新为最新的状态 */
		//sampleInfo.view_state = handleInfo.view_state;
		//sampleInfo.instance_state = handleInfo.instance_state;

		/* 读取到的数据，读取到数据个数（剩余数据的个数） */
		sampleInfo.sample_rank = --size;

		/* (读取到最新sampleInfo.disposed_generation_count + 读取到最新SampleInfo.no_writers_generation_count) - 
			(当前sampleInfo.disposed_generation_count + 当前SampleInfo.no_writers_generation_count) */
		sampleInfo.generation_rank = (readInfo.disposed_generation_count + readInfo.no_writers_generation_count) -
			(sampleInfo.disposed_generation_count + sampleInfo.no_writers_generation_count);

		/* (样本最新sampleInfo.disposed_generation_count + 样本最新SampleInfo.no_writers_generation_count) - 
			(当前sampleInfo.disposed_generation_count + 当前SampleInfo.no_writers_generation_count) */
		sampleInfo.absolute_generation_rank = (handleInfo.disposed_generation_count + handleInfo.no_writers_generation_count) -
												(sampleInfo.disposed_generation_count + sampleInfo.no_writers_generation_count);

		/* * 代表DataSample是否包含数据，或者仅用于传递实例instance_state的更改 */
		sampleInfo.valid_data = true;

	}

	/* 如果读取到的是实例的最后一个样本数据，将是实例的view状态修改为NOT_NEW_VIEW_STATE */
	//TODO  需要判断样本是最后一个数据 
	//if (readInfo.reception_sequence_number == handleInfo.reception_sequence_number) {
	//	sampleInfos_[handle].view_state = NOT_NEW_VIEW_STATE;
	//}
	GetRelatedDataReader()->NotifyReadConditions();
	return ret;
}

ReturnCode_t Reader::ReadOrTakeNextInstance(SerializedBufferSeq & dataValues, SampleInfoSeq & sampleInfos, uint32_t maxSamples, InstanceHandle_t handle,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake)
{
	InstanceHandle_t takeHandle = HANDLE_NIL;
	ReturnCode_t ret = RETCODE_ERROR;
	/* 从列表中获取大于handle的实例句柄，如果没有获取到说明没有其他数据 */
	ret = FindNextInstance(handle, takeHandle);
	if (RETCODE_OK != ret) {
		return ret;
	}
		
	return ReadOrTakeInstance(dataValues, sampleInfos, maxSamples, takeHandle, sampleStates, viewStates, instanceStates, shouldTake);
}


ReturnCode_t Reader::FindNextInstance(const InstanceHandle_t& preHander, InstanceHandle_t& instanceHandle)
{
	for (auto& handle : instanceHandles_) {
		if (preHander < handle) {
			instanceHandle = handle;
			return RETCODE_OK;
		}
	}
	return RETCODE_ERROR;
}


std::shared_ptr<CacheChange> Reader::new_change(ChangeKind_t kind, SerializedBuffer* data, InstanceHandle_t handle,InstanceHandle_t pubHandle, Time_t sourceTimespace)
{
	std::shared_ptr<CacheChange> cacheChange = std::make_shared<CacheChange>();

	auto itor = instanceHandles_.begin();
	for (; itor != instanceHandles_.end(); itor++) {
		if (handle < *itor) {
			instanceHandles_.insert(itor, handle);
			break;
		}
		if (handle == *itor) {
			break;
		}
	}

	if (itor == instanceHandles_.end()) {
		instanceHandles_.insert(itor, handle);
	}

	cacheChange->kind = kind;
	cacheChange->writerGuid = Conversion::ToGuid(pubHandle);
	cacheChange->instanceHandle = handle;
	cacheChange->sequenceNumber = lastSequenceNumber_++;
	cacheChange->serializedBuffer = data;

	cacheChange->sampleInfo = sampleInfos_[handle];
	cacheChange->sampleInfo.instance_handle = handle;
	cacheChange->sampleInfo.publication_handle = pubHandle;
	cacheChange->sampleInfo.source_timestamp = sourceTimespace;
	cacheChange->sampleInfo.sample_state = NOT_READ_SAMPLE_STATE;

	return cacheChange;
}
SampleInfo Reader::GetLastInstanceSampleInfo(const InstanceHandle_t& handle)
{
	return sampleInfo_;
}

void Reader::ChangeSampleInfoByLiveliness()
{
	for (auto sampleInfo : sampleInfos_) {
		sampleInfo.second.no_writers_generation_count++;
	}
}


/* 远端的DW调用Dispose函数，修改对应instance实例的状态 */
void Reader::ChangeSampleInfoByDWDisposed(const InstanceHandle_t& handl)
{
	for (auto sampleInfo : sampleInfos_) {
		sampleInfo.second.disposed_generation_count++;
	}
	//TODO  begin修改History的中相关实例的的样本状态；
	
	// 将样本的状态修改为  NOT_ALIVE_DISPOSED_INSTANCE_STATE

	//TODO end
	


} 

void Reader::SetReaderListener(ReaderListener* listener)
{
	readerListener_ = listener;
}

ReturnCode_t Reader::AddChanges(std::shared_ptr<CacheChange> cache)
{
	/* 判断是否有key  */
	std::map<InstanceHandle_t, CacheChangeVector>::iterator vitOut;
	SerializedBuffer* buffer = nullptr;
	/* 丢弃数据 */
	{
		std::unique_lock<std::mutex> lock(readerCache_->changeMutex_);
		if (RETCODE_OUT_OF_RESOURCES == readerCache_->FindORAddKey(cache->instanceHandle, buffer, &vitOut)) {
			return REJECTED_BY_INSTANCES_LIMIT;
		}
	}
	
	///* 根据HistoryQos的值进行设置 */
	if (KEEP_LAST_HISTORY_QOS == qos_.history.kind || BEST_EFFORT_RELIABILITY_QOS == qos_.reliability.kind) {
		std::unique_lock<std::mutex> lock(readerCache_->changeMutex_);
		auto  changesItor = readerCache_->instanceChanges_.find(cache->instanceHandle);
		if (changesItor == readerCache_->instanceChanges_.end()) {
			return REJECTED_BY_INSTANCES_LIMIT;
		}
		/*  满了，删除最旧的数据  */
		if (changesItor->second.cacheChanges.size() == changesItor->second.maxSamples) {
			auto change = changesItor->second.cacheChanges.begin();
			readerCache_->RemoveSampleCounters((*change)->sampleInfo.sample_state, changesItor->second.viewState, changesItor->second.instanceState);
			readerCache_->cacheChanges_.erase((*change)->sequenceNumber);
			changesItor->second.cacheChanges.erase(change);
		}
	}
	else {
		std::unique_lock<std::mutex> lock(readerCache_->changeMutex_);
		auto  changesItor = readerCache_->instanceChanges_.find(cache->instanceHandle);
		if (changesItor == readerCache_->instanceChanges_.end()) {
			return REJECTED_BY_INSTANCES_LIMIT;
		}

		auto time_out = std::chrono::seconds(qos_.reliability.max_blocking_time.sec) + std::chrono::nanoseconds(qos_.reliability.max_blocking_time.nanosec);
		bool ret0 = readerCache_->changeVar_.wait_for(lock, time_out,
			[&]() { return changesItor->second.cacheChanges.size() < changesItor->second.maxSamples; });

		if (!ret0) {
			return REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT;
		}

	}

	//wangyi time_based_filter 过滤判断
	if (qos_.time_based_filter.minimum_separation != Duration_t(TIME_ZERO)) {
		//wangyi time_based_filter qos加入时间判断，达到时间间隔后触发回调函数
		Duration_t ltime;
		Duration_t::now(ltime);//获取当前时间
		Duration_t costTime = ltime - lastTime_;
		if (costTime < qos_.time_based_filter.minimum_separation) {
			return RETCODE_TIME_BASED_FILTER;
		}

		lastTime_ = ltime;
		/* 更新定时器时间 */
		if (lastNewCacheTimer_ != nullptr) {
			lastNewCacheTimer_->UpdatePeriod(Conversion::ToMilliSecondsDouble(qos_.time_based_filter.minimum_separation) * 1.5);
		}
	}

	{
		std::unique_lock<std::mutex> lock(readerCache_->changeMutex_);
		//TODO ownershipStrength的值补充输入
		ReturnCode_t ret = readerCache_->UpdataSampleInstance(
			cache->instanceHandle, 
			cache->kind, 
			cache->writerGuid,
			0, //ownershipStrength  
			SHARED_OWNERSHIP_QOS == qos_.ownership.kind ? true : false
		);
		if (RETCODE_OK != ret) {
			return ret;
		}
		auto& ins = readerCache_->instanceChanges_[cache->instanceHandle];
		ins.cacheChanges.push_back(cache);
		readerCache_->AddSampleCounters(cache->sampleInfo.sample_state, ins.viewState, ins.instanceState);
	}

	/* 更新接收到数据的状态 */
	sampleInfo_ = cache->sampleInfo;
	sampleInfos_[cache->instanceHandle] = cache->sampleInfo;

	return RETCODE_OK;
}

DataReaderImpl* Reader::GetRelatedDataReader()
{
	return datareader_;
}

ReturnCode_t Reader::ProcessInlineQos(ChangeKind_t& kind, InstanceHandle_t& handle, const ParameterList_t& inlineQos)
{
	for (auto& parameter : inlineQos) {
		switch (parameter.parameterId) {
			case PID_KEY_HASH:
				memcpy(handle.keyHash.value, parameter.value.data(), MIG_RTPS_KEY_HASH_MAX_LENGTH);
				break;
			case PID_STATUS_INFO:
			{
				if (parameter.length == 4) {
					//TODO
					octet flag = parameter.value[3];
					switch (flag & (StatusInfoFlag::DISPOSED_FLAG | StatusInfoFlag::UNREGISTERED_FLAG)) {
					    case StatusInfoFlag::NONE_FLAG:
							kind = ALIVE;
						    break;
					    case StatusInfoFlag::DISPOSED_FLAG:
							kind = NOT_ALIVE_DISPOSED;
						    break;
						case StatusInfoFlag::UNREGISTERED_FLAG:
							kind = NOT_ALIVE_UNREGISTERED;
						    break;
						case (StatusInfoFlag::DISPOSED_FLAG | StatusInfoFlag::UNREGISTERED_FLAG):
							kind = NOT_ALIVE_DISPOSED_UNREGISTERED;
						    break;
						default:
						break;
					}
				}
			}
			break;
			default:
				break;
		}
	}

	return RETCODE_OK;
}


bool Reader::IsReliable()
{
	return qos_.reliability.kind == RELIABLE_RELIABILITY_QOS;
}

void Reader::SetRelatedDataReader(DataReaderImpl* reader)
{
	datareader_ = reader;
}

DataReaderQos Reader::GetQos()
{
	return qos_;
}

void Reader::GetMatchedPublications(InstanceHandleSeq& publication_handles) const
{
	participant_->GetMatchedEntitys(guid_.entityId, publication_handles);
}

bool Reader::GetMatchedPublicationData(PublicationBuiltinTopicData& publicationData, const InstanceHandle_t& remotehandle) const
{
	if (!participant_->RemoteEntityIsMatched(guid_.entityId, remotehandle)) {
		return false;
	}
	if (RETCODE_OK == participant_->GetPublicationData(publicationData, remotehandle)) {
		return true;
	}
	return false;
}
