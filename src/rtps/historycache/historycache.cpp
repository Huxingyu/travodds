#include <algorithm>
#include "rtps/historycache/historycache.h"
#include "rtps/endpoint/writer.h"
#include "common/log/logger.h"
USING_TRAVODDS_NAMESPACE;

HistoryCache::HistoryCache(const HistoryQosPolicy& historyQos, const ResourceLimitsQosPolicy& resourceLimitsQos, const TopicKind_t& topicKind)
	: historyQos_(historyQos), resourceLimitesQos_(resourceLimitsQos), topicKind_(topicKind)
{
	if (resourceLimitesQos_.max_samples <= 0) {
		resourceLimitesQos_.max_samples = std::numeric_limits<int32_t>::max();
	}
	if (resourceLimitesQos_.max_instances <= 0) {
		resourceLimitesQos_.max_instances = std::numeric_limits<int32_t>::max();
	}
	if (resourceLimitesQos_.max_samples_per_instance <= 0) {
		resourceLimitesQos_.max_samples_per_instance = std::numeric_limits<int32_t>::max();
	}

	if (NO_KEY == topicKind_.value) {
		/* 无key的情况下  */
		if (KEEP_LAST_HISTORY_QOS == historyQos_.kind) {
			maxSamples_ = historyQos_.depth;
		} else {
			maxSamples_ = resourceLimitesQos_.max_samples;
		}
		maxSamplesPerInstance_ = maxSamples_;
		instanceChanges_.insert(std::make_pair(HANDLE_NIL, CacheChangeVector(nullptr, maxSamplesPerInstance_)));
	}
	else {
		maxSamples_ = resourceLimitsQos.max_samples;
		maxSamplesPerInstance_ = resourceLimitsQos.max_samples_per_instance;
		if (KEEP_LAST_HISTORY_QOS == historyQos_.kind) {			
			maxSamplesPerInstance_ = historyQos_.depth;
		}
	}
}

HistoryCache::~HistoryCache()
{
	cacheChanges_.clear();
}

bool HistoryCache::IsFull()
{
	if (cacheChanges_.size() >= maxSamples_) {
		return true;
	}

	return false;
}
bool HistoryCache::AddChange(std::shared_ptr<CacheChange> change)
{
	if (nullptr == change) {
		return false;
	}

	changeMutex_.lock();

	cacheChanges_[change->sequenceNumber] = change;
	auto& ins = instanceChanges_[change->instanceHandle];
	ins.cacheChanges.push_back(change);
	//AddSampleCounters(change->sampleInfo.sample_state, 
	//	ins.viewState, ins.instanceState);

	changeMutex_.unlock();

	return true;
}

bool HistoryCache::RemoveChange(const SequenceNumber_t& seqNum)
{
	{
		std::lock_guard<std::mutex> lock(changeMutex_);
		auto change = cacheChanges_.find(seqNum);
		if (change == cacheChanges_.end()) {
			return false;
		}
		auto itor = instanceChanges_.find(change->second->instanceHandle);
		if (itor != instanceChanges_.end()) {
			auto& cacheChanges = itor->second.cacheChanges;
			cacheChanges.erase(
				std::remove_if(cacheChanges.begin(), cacheChanges.end(), [&change](const auto& cachedPtr) {
				return cachedPtr.get() == change->second.get();
				}),
				cacheChanges.end()
			);
		}
		cacheChanges_.erase(change);
	}

	changeVar_.notify_all();

	return true;
}

std::shared_ptr<CacheChange> HistoryCache::GetChange(const SequenceNumber_t& seqNum)
{
	std::shared_ptr<CacheChange> pCacheChange = nullptr;

	changeMutex_.lock();

	auto itor = cacheChanges_.find(seqNum);
	if (cacheChanges_.end() != itor) {
		pCacheChange = itor->second;
	}
		
	changeMutex_.unlock();

	return pCacheChange;

}

std::shared_ptr<CacheChange> HistoryCache::GetMinSourceTimespace()
{
	std::shared_ptr<CacheChange> MinSourceTimespaceCache = nullptr;

	for (auto itor = instanceChanges_.begin(); itor != instanceChanges_.end();itor++) {
		if (itor->second.cacheChanges.empty()) continue;

		// 在当前 vector 中查找最小值
		auto it = std::min_element(itor->second.cacheChanges.begin(), itor->second.cacheChanges.end(),
			[](const auto& a, const auto& b) {
			if (!a) return false;  // 确保 a 有效
			if (!b) return true;   // b 无效时 a 总是更小
			return a->sourceTimespace < b->sourceTimespace;
		});

		// 更新最小值
		if (*it && (!MinSourceTimespaceCache || (*it)->sourceTimespace < MinSourceTimespaceCache->sourceTimespace)) {
			MinSourceTimespaceCache = *it;
		}
	}

	return MinSourceTimespaceCache;
}

SequenceNumber_t HistoryCache::GetSeqNumMin()
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	if (0 == cacheChanges_.size()) {
		return SEQUENCENUMBER_FIRST;
	}

	return cacheChanges_.begin()->first;
}

SequenceNumber_t HistoryCache::GetSeqNumMax()
{
	std::unique_lock<std::mutex> lock(changeMutex_);

	if (0 == cacheChanges_.size()) {
		return SEQUENCENUMBER_ZERO;
	}

	return cacheChanges_.rbegin()->first;
}

ReturnCode_t HistoryCache::ReadOrTakeInstance(SerializedBufferSeq& dataValues, SampleInfoSeq& sampleInfos, int32_t maxSamples, InstanceHandle_t handle,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouleTake)
{
	changeMutex_.lock();
	auto itor = instanceChanges_.find(handle);
	if (itor != instanceChanges_.end()) {
		for (auto cache = itor->second.cacheChanges.begin(); cache != itor->second.cacheChanges.end(); ) {
			/* 判断状态是否符合,数据状态不符合不条件数据  */
			if (0 == (sampleStates & (*cache)->sampleInfo.sample_state) || 
				0 == (itor->second.instanceState & instanceStates) ||
				0 == (itor->second.viewState & viewStates)) {
				cache++;
				continue;
			}
			/* jdxcz question 给SampleInfo 进行赋值 */
			SampleInfo sampleInfo;
			sampleInfo = (*cache)->sampleInfo;
			sampleInfo.view_state = itor->second.viewState;
			sampleInfo.instance_state = itor->second.instanceState;
			if ((*cache)->kind == ALIVE) {
				sampleInfo.valid_data = true;
			}
			sampleInfo.write_sequence_number = (*cache)->sequenceNumber.low;
			sampleInfo.source_timestamp = (*cache)->sourceTimespace;
			dataValues.push_back(*(*cache)->serializedBuffer);
			sampleInfos.push_back(sampleInfo);

			/* 修改数据的状态  */
			UpdateState(READ_SAMPLE_STATE, *cache);
			UpdateState(NOT_NEW_VIEW_STATE, itor->second);

			/* 获取后删除数据 */
			if (shouleTake) {
				cache = itor->second.cacheChanges.erase(cache);
			}
			else {
				cache++;
			}
			/* 获取的数据个数 大于 max_samples设置，停止获取数据  */
			if (dataValues.length() == maxSamples) {
				break;
			}
		}
	}

	changeMutex_.unlock();

	return dataValues.length() == 0 ? RETCODE_NO_DATA : RETCODE_OK;
}


ReturnCode_t HistoryCache::AddKeyHistory(const InstanceHandle_t& handle, SerializedBuffer*& instace)
{
	ReturnCode_t result = RETCODE_OK;
	if (topicKind_.value == NO_KEY)	{
		return result;
	}

	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor;
	result = FindORAddKey(handle,instace, &itor);
	if (RETCODE_OK == result)	{
		instace = itor->second.keyBuffer.get();
	}
	return result;
}

ReturnCode_t HistoryCache::FindORAddKey(const InstanceHandle_t& handle, SerializedBuffer*& instace, std::map<InstanceHandle_t, CacheChangeVector>::iterator* vitOut)
{
	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor;
	itor = instanceChanges_.find(handle);
	if (itor != instanceChanges_.end()) {
		*vitOut = itor;
		return RETCODE_OK;
	}

	if (static_cast<int32_t>(instanceChanges_.size()) < resourceLimitesQos_.max_instances) {
		itor = instanceChanges_.insert(std::make_pair(handle, CacheChangeVector(instace, maxSamplesPerInstance_))).first;
		/* TODO 深拷贝 待实现 itor->second.keyBuffer.Copy(instace, false); */
		*vitOut = itor;
		return RETCODE_OK;
	}
	TRAVODDS_LOG(LOG_ERROR, "instanceChanges_.size[%d] >=  resourceLimitesQos_.max_instances[%d]", instanceChanges_.size(), resourceLimitesQos_.max_instances);
	return RETCODE_OUT_OF_RESOURCES;
}

void HistoryCache::AddSampleCounters(const SampleStateKind& sampleState, const ViewStateKind& viewState, const InstanceStateKind& instanceState)
{
	counters_.sampleRead += (sampleState == READ_SAMPLE_STATE) ? 1 : 0;
	counters_.sampleNotRead += (sampleState == NOT_READ_SAMPLE_STATE) ? 1 : 0;
	counters_.viewNew += (viewState == NEW_VIEW_STATE) ? 1 : 0;
	counters_.viewNotNew += (viewState == NOT_NEW_VIEW_STATE) ? 1 : 0;
	counters_.instanceAlive += (instanceState == ALIVE_INSTANCE_STATE) ? 1 : 0;
	counters_.instanceDisposed += (instanceState == NOT_ALIVE_DISPOSED_INSTANCE_STATE) ? 1 : 0;
	counters_.instanceNoWriters += (instanceState == NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) ? 1 : 0;
	counters_.change.store(true);
}

void HistoryCache::RemoveSampleCounters(const SampleStateKind& sampleState, const ViewStateKind& viewState, const InstanceStateKind& instanceState)
{
	counters_.sampleRead -= (sampleState == READ_SAMPLE_STATE) ? 1 : 0;
	counters_.sampleNotRead -= (sampleState == NOT_READ_SAMPLE_STATE) ? 1 : 0;
	counters_.viewNew -= (viewState == NEW_VIEW_STATE) ? 1 : 0;
	counters_.viewNotNew -= (viewState == NOT_NEW_VIEW_STATE) ? 1 : 0;
	counters_.instanceAlive -= (instanceState == ALIVE_INSTANCE_STATE) ? 1 : 0;
	counters_.instanceDisposed -= (instanceState == NOT_ALIVE_DISPOSED_INSTANCE_STATE) ? 1 : 0;
	counters_.instanceNoWriters -= (instanceState == NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) ? 1 : 0;
	counters_.change.store(true);
}

ReturnCode_t HistoryCache::UpdataSampleInstance(const InstanceHandle_t& handle, const ChangeKind_t& changeKind, const GUID_t& writerGuid, const int32_t ownershipStrength, bool shared)
{
	ReturnCode_t ret = RETCODE_OK;
	auto& insChange = instanceChanges_[handle];
	
	if(!shared) {
		if(insChange.aliveWriter.empty()) {
			insChange.aliveWriter.insert({ writerGuid,ownershipStrength });
		} 
		else {
			if(insChange.aliveWriter.begin()->first != writerGuid && insChange.aliveWriter.begin()->second >= ownershipStrength) {
				return RETCODE_EXCLUSIVE_OWNERSHIP_QOS_NOT_MAX;
			}
			if (insChange.aliveWriter.begin()->first != writerGuid) {
				insChange.aliveWriter.erase(insChange.aliveWriter.begin());
			}
			insChange.aliveWriter[writerGuid] = ownershipStrength;
		}
	}

	switch (changeKind)
	{
	case ALIVE:
		if (insChange.instanceState & (NOT_ALIVE_NO_WRITERS_INSTANCE_STATE | NOT_ALIVE_DISPOSED_INSTANCE_STATE)) {
			UpdateState(NEW_VIEW_STATE, insChange);
		}
		UpdateState(ALIVE_INSTANCE_STATE, insChange);
		break;

	case NOT_ALIVE_DISPOSED:
		UpdateState(NOT_ALIVE_DISPOSED_INSTANCE_STATE, insChange);
		break;

	case NOT_ALIVE_DISPOSED_UNREGISTERED:
	case NOT_ALIVE_UNREGISTERED:
		insChange.aliveWriter.erase(writerGuid);
		if(insChange.aliveWriter.empty()) {
			UpdateState(NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, insChange);
		} else {
			UpdateState(NOT_ALIVE_DISPOSED_INSTANCE_STATE, insChange);
		}	
		break;
	default:
		// TODO log error
		break;
	}
	return ret;
}

void HistoryCache::UpdateState(const SampleStateKind& state, const std::shared_ptr<CacheChange>& cache)
{
	if(cache->sampleInfo.sample_state == state) {
		return;
	}
	cache->sampleInfo.sample_state = state;
	bool isRead = (state & READ_SAMPLE_STATE);
	counters_.sampleRead += isRead ? 1 : -1;
	counters_.sampleNotRead += isRead ? -1 : 1;
	counters_.change.store(true);
}

void HistoryCache::UpdateState(const ViewStateKind& state, CacheChangeVector& cacheChange)
{
	if(cacheChange.viewState == state) {
		return;
	}
	cacheChange.viewState = state;
	bool isNew = (state & NEW_VIEW_STATE);
	counters_.viewNew += (isNew ? 1 : -1) * cacheChange.cacheChanges.size();
	counters_.viewNotNew += (isNew ? -1 : 1) * cacheChange.cacheChanges.size();
	counters_.change.store(true);
}

void HistoryCache::UpdateState(const InstanceStateKind& state, CacheChangeVector& cacheChange)
{
	if(cacheChange.instanceState == state) {
		return;
	}
	cacheChange.instanceState = state;
	bool isAlive = state == ALIVE_INSTANCE_STATE;
	bool isDisposed = state == NOT_ALIVE_DISPOSED_INSTANCE_STATE;
	bool isNoWriters = state == NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
	counters_.instanceAlive += (isAlive ? 1 : -1) * cacheChange.cacheChanges.size();
	counters_.instanceDisposed += (isDisposed ? 1 : -1) * cacheChange.cacheChanges.size();
	counters_.instanceNoWriters += (isNoWriters ? 1 : -1) * cacheChange.cacheChanges.size();
	counters_.change.store(true);
}

bool HistoryCache::IsKeyRegistered(const InstanceHandle_t& handle)
{
	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor;
	itor = instanceChanges_.find(handle);
	return itor != instanceChanges_.end();
}

SerializedBuffer * HistoryCache::get_key_value(const InstanceHandle_t& handle)
{
	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor = instanceChanges_.find(handle);
	if (itor != instanceChanges_.end()) {
		return itor->second.keyBuffer.get();
	}
	return nullptr;
}

bool HistoryCache::IsInstancePresent(const InstanceHandle_t& handle)
{
	return instanceChanges_.find(handle) != instanceChanges_.end();
}

void HistoryCache::DeleteKeyHistory(const InstanceHandle_t& handle)
{
	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor = instanceChanges_.find(handle);
	if (itor != instanceChanges_.end()) {
		instanceChanges_.erase(itor);
	}
}

bool HistoryCache::IsFull(const InstanceHandle_t& handle)
{
	std::unique_lock<std::mutex> lock(changeMutex_);

	auto  changesItor = instanceChanges_.find(handle);
	if (changesItor == instanceChanges_.end()) {
		return false;
	}
	return changesItor->second.cacheChanges.size() == changesItor->second.maxSamples;
}
void HistoryCache::RemoveOldestChangeByInstance(const InstanceHandle_t& handle)
{
	std::unique_lock<std::mutex> lock(changeMutex_);

	auto  changesItor = instanceChanges_.find(handle);
	if (changesItor == instanceChanges_.end()) {
		return;
	}
	auto change = changesItor->second.cacheChanges.begin();
	cacheChanges_.erase((*change)->sequenceNumber);
	changesItor->second.cacheChanges.erase(change);
}

void HistoryCache::RemoveChangeByInstance(const InstanceHandle_t& handle)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	auto  changesItor = instanceChanges_.find(handle);
	if (changesItor == instanceChanges_.end()) {
		return;
	}

	for (auto cache : changesItor->second.cacheChanges) {
		cacheChanges_.erase(cache->sequenceNumber);
	}

	changesItor->second.cacheChanges.clear();
	
}

bool HistoryCache::WriterWaitFor(const std::chrono::nanoseconds& time_out, Writer* pWriter,const InstanceHandle_t& handle, bool transientLocal)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	auto changesItor = instanceChanges_.find(handle);
	if (changesItor == instanceChanges_.end()) {
		return false;
	}
	//TODO jd 待优化 
	return changeVar_.wait_for(lock, time_out,
		[&]() { 
			if (transientLocal) {
				/* 判断当前的Instance对应的最小序号是否可以删除， 可以删除 */
				auto change = changesItor->second.cacheChanges[0];
				SequenceNumber_t seq = change->sequenceNumber;
				lock.unlock();
				/* 如果当前序号已经被所有的DataReader确认，那么删除数据 */
				if (pWriter->IsAckedByAll(seq)) {

					lock.lock();
					/*  RELIABLE & KEPP_ALL & TRANSENT_LOCAL 在此处删除数据 */
					cacheChanges_.erase(change->sequenceNumber);
					auto itor = instanceChanges_.find(change->instanceHandle);
					if (itor != instanceChanges_.end()) {
						for (auto cache = itor->second.cacheChanges.begin(); cache != itor->second.cacheChanges.end(); cache++) {
							if (cache->get() == change.get()) {
								itor->second.cacheChanges.erase(cache);
								break;
							}
						}
					}
					return true;
				}
				else {
					lock.lock();
					return false;
				}
			}
			else {
				return changesItor->second.cacheChanges.size() < changesItor->second.maxSamples;
			}
		});
	

}

SampleCounters HistoryCache::GetSampleCounters(bool trigger)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	SampleCounters counters = counters_;
	if (trigger) {
		counters_.change.store(false);
	}
	return counters;
}

bool HistoryCache::GetNextDeadline(InstanceHandle_t& handle, std::chrono::steady_clock::time_point& nextDeadline)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	if (instanceChanges_.empty()) {
		return false;
	}
	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor = 
		std::min_element(instanceChanges_.begin(), instanceChanges_.end(),
			[](const auto& a, const auto& b) {
			return a.second.nextDeadline < b.second.nextDeadline;
			}
		);
	handle = itor->first;
	nextDeadline = itor->second.nextDeadline;
	return false;
}

void HistoryCache::SetNextDeadline(const InstanceHandle_t& handle, const std::chrono::steady_clock::time_point& nextDeadline, bool timeout)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	auto& readinstance = instanceChanges_[handle];
	readinstance.nextDeadline = nextDeadline;

	if (timeout) {
		//std::map<GUID_t, int32_t>::iterator itor =
		//	std::min_element(readinstance.aliveWriter.begin(), readinstance.aliveWriter.end(),
		//		[](const std::pair<GUID_t, int32_t>& a, const std::pair<GUID_t, int32_t>& b) {
		//			return a.second < b.second;
		//		}
		//	);
		if (readinstance.aliveWriter.empty()) {
			UpdateState(NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, readinstance);
		} else {
			UpdateState(NOT_ALIVE_DISPOSED_INSTANCE_STATE, readinstance);
		}
	}
}

void HistoryCache::UpAllDeadline(const std::chrono::steady_clock::time_point& nextDeadline)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	for (auto& it : instanceChanges_) {
		it.second.nextDeadline = nextDeadline;
	}
}

int32_t HistoryCache::GetInstanceLostcounts(const std::shared_ptr<CacheChange>& cache)
{
	int32_t lostcounts = 0;
	std::unique_lock<std::mutex> lock(changeMutex_);
	auto& readinstance = instanceChanges_[cache->instanceHandle];
	lostcounts = cache->sequenceNumber - readinstance.lastSeqNum -1;
	readinstance.lastSeqNum = cache->sequenceNumber;
	return lostcounts;
}

void HistoryCache::RemoveRemoteWriter(const GUID_t& guid)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	for (auto& ins : instanceChanges_) {
		ins.second.aliveWriter.erase(guid);
		if (ins.second.aliveWriter.empty()) {
			ins.second.instanceState = NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
		}
	}
}
