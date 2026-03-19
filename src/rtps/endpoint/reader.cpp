#include "rtps/endpoint/reader.h"
#include "rtps/endpoint/participant.h"
#include "common/log/logger.h"
#include "common/timeconversion.h"
#include "dcps/subscribe/datareaderimpl.h"
USING_TRAVODDS_NAMESPACE;

Reader::Reader(const DataReaderQos& qos, const TopicKind_t& kind)
	: heartbeatResponseDelay_(TIME_INVALID),
	  heartbeatSuppressionDuration_(TIME_INVALID),
	  readerCache_(std::make_shared<HistoryCache>(qos.history, qos.resource_limits, kind)),
	  expectsInlineQos_(false),
	  lastSequenceNumber_(SEQUENCENUMBER_ZERO),
	  readerListener_(nullptr),
	  qos_(qos),
	  datareader_(nullptr)
{
	if (BEST_EFFORT == qos_.reliability.kind) {
		reliabilityLevel_ = BEST_EFFORT;
	}	
	else {
		reliabilityLevel_ = RELIABLE;
	}
		
	instanceHandles_.push_back(HANDLE_NIL);
}

Reader::~Reader()
{
}
ReturnCode_t Reader::ReadOrTake(std::vector<std::shared_ptr<CacheChange>>& dataValues, SampleInfoSeq& sampleInfos, uint32_t maxSamples,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake)
{
	InstanceHandle_t handle = HANDLE_NIL;
	InstanceHandle_t nextHandle = HANDLE_NIL;
	ReturnCode_t ret = RETCODE_OK;
	
	/* 循环读取所有的Instance的数据 */
	do {
		if (dataValues.size() >= maxSamples) {
			break;
		}

		ret = ReadOrTakeInstance(dataValues, sampleInfos, maxSamples, nextHandle, sampleStates, viewStates, instanceStates, shouldTake);

		handle = nextHandle;
		ret = FindNextInstance(handle, nextHandle);	

	} while (RETCODE_ERROR != ret);

	if (0 >= dataValues.size()) {
		return RETCODE_NO_DATA;
	}
		
	return RETCODE_OK;
}

ReturnCode_t Reader::ReadOrTakeInstance(std::vector<std::shared_ptr<CacheChange>>& dataValues, SampleInfoSeq & sampleInfos, uint32_t maxSamples, InstanceHandle_t handle,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake)
{
	GetRelatedDataReader()->SetStatus(false);
	ReturnCode_t ret = RETCODE_OK;

	ret = readerCache_->ReadOrTakeInstance(dataValues, sampleInfos, maxSamples, handle, sampleStates, viewStates, instanceStates, shouldTake);
	if (RETCODE_OK != ret) {
		return ret;
	}

	GetRelatedDataReader()->NotifyReadConditions();
	return ret;
}

ReturnCode_t Reader::ReadOrTakeNextInstance(std::vector<std::shared_ptr<CacheChange>>& dataValues, SampleInfoSeq & sampleInfos, uint32_t maxSamples, InstanceHandle_t handle,
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


std::shared_ptr<CacheChange> Reader::new_change(ChangeKind_t kind, SerializedBuffer* data, const InstanceHandle_t& handle, const InstanceHandle_t& pubHandle, const Time_t& sourceTimespace)
{
	std::shared_ptr<CacheChange> cacheChange = std::make_shared<CacheChange>();

	auto itor = instanceHandles_.begin();
	for (; itor != instanceHandles_.end(); itor++) {
		if (handle < *itor) {
			instanceHandles_.insert(itor, handle);
			readerCache_->instanceChanges_.insert(std::make_pair(handle, CacheChangeVector(nullptr, readerCache_->maxSamplesPerInstance_)));
			break;
		}
		if (handle == *itor) {
			break;
		}
	}

	if (itor == instanceHandles_.end()) {
		readerCache_->instanceChanges_.insert(std::make_pair(handle, CacheChangeVector(nullptr, readerCache_->maxSamplesPerInstance_)));
		instanceHandles_.insert(itor, handle);
	}

	cacheChange->kind = kind;
	cacheChange->writerGuid = Conversion::ToGuid(pubHandle);
	cacheChange->instanceHandle = handle;
	cacheChange->sequenceNumber = lastSequenceNumber_++;
	cacheChange->serializedBuffer = data;

	cacheChange->sampleInfo.instance_handle = handle;
	cacheChange->sampleInfo.publication_handle = pubHandle;
	cacheChange->sampleInfo.source_timestamp = sourceTimespace;
	cacheChange->sampleInfo.sample_state = NOT_READ_SAMPLE_STATE;

	return cacheChange;
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
		/*  满了，删除最旧的数据  */
		if (vitOut->second.cacheChanges.size() == static_cast<size_t>(vitOut->second.maxSamples)) {
			auto change = vitOut->second.cacheChanges.begin();
			readerCache_->RemoveSampleCounters((*change)->sampleInfo.sample_state, vitOut->second.viewState, vitOut->second.instanceState);
			readerCache_->cacheChanges_.erase((*change)->sequenceNumber);
			vitOut->second.cacheChanges.erase(change);
		}
	}
	else {
		std::unique_lock<std::mutex> lock(readerCache_->changeMutex_);
		auto time_out = std::chrono::seconds(qos_.reliability.max_blocking_time.sec) + std::chrono::nanoseconds(qos_.reliability.max_blocking_time.nanosec);
		bool ret0 = readerCache_->changeVar_.wait_for(lock, time_out,
			[&]() { return vitOut->second.cacheChanges.size() < static_cast<size_t>(vitOut->second.maxSamples); });

		if (!ret0) {
			return REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT;
		}

	}

	/* TODO wangyi 暂时以接收时间为源时间戳 */
	Time_t::now(cache->sourceTimespace);//获取当前时间

	//wangyi time_based_filter 过滤判断
	if (qos_.time_based_filter.minimum_separation != Duration_t(TIME_ZERO)) {
		auto now = std::chrono::steady_clock::now();
		//wangyi time_based_filter qos加入时间判断，达到时间间隔后触发回调函数
		if (now < vitOut->second.nextTimeBasedFilte) {
			vitOut->second.lastCache = cache;
			return RETCODE_TIME_BASED_FILTER;
		}
		vitOut->second.lastCache.reset();
		vitOut->second.nextTimeBasedFilte = now + std::chrono::nanoseconds(qos_.time_based_filter.minimum_separation.ToNs());

		/* 更新定时器时间 */
		if (lastNewCacheTimer_) {
			std::chrono::steady_clock::time_point nextTime;
			readerCache_->GetNextTimeBasedFilte(lastInstance_, nextTime);
			auto period = nextTime - std::chrono::steady_clock::now();
			lastNewCacheTimer_->UpdatePeriod(std::chrono::duration_cast<std::chrono::milliseconds>(period).count());
		}
	}


	/* wangyi 获取远端数据写者QoS */
	PublicationBuiltinTopicData publicationData;
	if (EXCLUSIVE_OWNERSHIP_QOS == qos_.ownership.kind) {
		GetMatchedPublicationData(publicationData, Conversion::ToInstanceHandle(cache->writerGuid));
	}
	{
		std::unique_lock<std::mutex> lock(readerCache_->changeMutex_);
		
		ReturnCode_t ret = readerCache_->UpdataSampleInstance(
			cache->instanceHandle, 
			cache->kind, 
			cache->writerGuid,
			publicationData.ownership_strength.value, //ownershipStrength  
			SHARED_OWNERSHIP_QOS == qos_.ownership.kind ? true : false
		);
		if (RETCODE_OK != ret) {
			return ret;
		}
		auto& ins = readerCache_->instanceChanges_[cache->instanceHandle];
		ins.cacheChanges.push_back(cache);
		readerCache_->AddSampleCounters(cache->sampleInfo.sample_state, ins.viewState, ins.instanceState);

		cache->sampleInfo.disposed_generation_count = ins.disposedGenerationCount;
		cache->sampleInfo.no_writers_generation_count = ins.noWritersGenerationCount;
	}

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

const DataReaderQos& Reader::GetQos()
{
	return qos_;
}

void Reader::SetQos(const DataReaderQos& qos)
{
	if (qos.time_based_filter.minimum_separation != qos_.time_based_filter.minimum_separation) {
		readerCache_->RsetAllTimeBasedFilte();
	}
	qos_ = qos;
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

std::shared_ptr<HistoryCache> Reader::GetReaderCache()
{
	return readerCache_;
}
