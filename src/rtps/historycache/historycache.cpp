#include <algorithm>
#include "rtps/historycache/historycache.h"
#include "rtps/endpoint/writer.h"
#include "common/log/logger.h"
USING_TRAVODDS_NAMESPACE;

HistoryCache::HistoryCache(const HistoryQosPolicy& historyQos, const ResourceLimitsQosPolicy& resourceLimitsQos, const TopicKind_t& topicKind)
	: topicKind_(topicKind), resourceLimitesQos_(resourceLimitsQos), historyQos_(historyQos)
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
	if (cacheChanges_.size() >= static_cast<size_t>(maxSamples_)) {
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
	AddSampleCounters(change->sampleInfo.sample_state, ins.viewState, ins.instanceState);

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
            if (std::find(cacheChanges.begin(), cacheChanges.end(), change->second) != cacheChanges.end())
                RemoveSampleCounters(change->second->sampleInfo.sample_state, itor->second.viewState, itor->second.instanceState);
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
	std::lock_guard<std::mutex> lock(changeMutex_);

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

ReturnCode_t HistoryCache::ReadOrTakeInstance(std::vector<std::shared_ptr<CacheChange>>& dataValues, SampleInfoSeq& sampleInfos, int32_t maxSamples, InstanceHandle_t handle,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouleTake, bool preview)
{
    PurgeExpired();
	std::unique_lock<std::mutex> selectionLock(changeMutex_);
	auto itor = instanceChanges_.find(handle);
	if (itor != instanceChanges_.end()) {
		int32_t presize = static_cast<int32_t>( dataValues.size());
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
			sampleInfo.absolute_generation_rank =(* itor->second.cacheChanges.rbegin())->sampleInfo.disposed_generation_count
				+ (*itor->second.cacheChanges.rbegin())->sampleInfo.no_writers_generation_count - itor->second.disposedGenerationCount - itor->second.noWritersGenerationCount;

			if ((*cache)->kind == ALIVE) {
				sampleInfo.valid_data = true;
			}
			sampleInfo.write_sequence_number = (*cache)->sequenceNumber.low;
			sampleInfo.source_timestamp = (*cache)->sourceTimespace;
			sampleInfo.related_sample_identity = (*cache)->related_sample_identity;
			dataValues.push_back(*cache);
			sampleInfos.push_back(sampleInfo);

			if (!preview) {
			/* 修改数据的状态  */
			UpdateState(READ_SAMPLE_STATE, *cache);
			UpdateState(NOT_NEW_VIEW_STATE, itor->second);

			/* 获取后删除数据 */
			if (shouleTake) {
				RemoveSampleCounters((*cache)->sampleInfo.sample_state,
					itor->second.viewState, itor->second.instanceState);
				// Some insertion paths also index the sample by sequence. Remove
				// that reference, but keep dataValues' reference through the read.
				auto indexed = cacheChanges_.find((*cache)->sequenceNumber);
				if (indexed != cacheChanges_.end() && indexed->second == *cache) {
					cacheChanges_.erase(indexed);
				}
				cache = itor->second.cacheChanges.erase(cache);
			}
			else {
				cache++;
			}
			} else { ++cache; }
			/* 获取的数据个数 大于 max_samples设置，停止获取数据  */
			if (dataValues.size() == static_cast<size_t>(maxSamples)) {
				break;
			}
		}
		int32_t cursize = static_cast<int32_t>(dataValues.size());
		int32_t dis = cursize - presize;

		for (int32_t i = presize; i < cursize - 1; i++) {
			sampleInfos[i].sample_rank = dis - (i - presize) - 1;
			sampleInfos[i].generation_rank = sampleInfos[cursize - 1].disposed_generation_count + sampleInfos[cursize - 1].no_writers_generation_count - sampleInfos[i].disposed_generation_count - sampleInfos[i].no_writers_generation_count;
		}
	}
	selectionLock.unlock();
	if (shouleTake) {
		changeVar_.notify_all();
	}

	return dataValues.size() == 0 ? RETCODE_NO_DATA : RETCODE_OK;
}


ReturnCode_t HistoryCache::AddKeyHistory(const InstanceHandle_t& handle, SerializedBuffer*& instace)
{
	std::lock_guard<std::mutex> lock(changeMutex_);
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

ReturnCode_t HistoryCache::RegisterKey(const InstanceHandle_t& handle,
        const std::shared_ptr<SerializedBuffer>& serialized, const std::shared_ptr<void>& native)
{
    std::lock_guard<std::mutex> lock(changeMutex_);
    auto it = instanceChanges_.find(handle);
    if (it == instanceChanges_.end()) {
        if (instanceChanges_.size() >= static_cast<size_t>(resourceLimitesQos_.max_instances))
            return RETCODE_OUT_OF_RESOURCES;
        it = instanceChanges_.emplace(handle, CacheChangeVector(nullptr, maxSamplesPerInstance_)).first;
    }
    if (!it->second.keyBuffer && !it->second.nativeKey) {
        it->second.keyBuffer = serialized;
        it->second.nativeKey = native;
    }
    return RETCODE_OK;
}

std::shared_ptr<void> HistoryCache::GetNativeKey(const InstanceHandle_t& handle)
{
    std::lock_guard<std::mutex> lock(changeMutex_);
    auto it = instanceChanges_.find(handle);
    return it == instanceChanges_.end() ? nullptr : it->second.nativeKey;
}

std::shared_ptr<SerializedBuffer> HistoryCache::GetSerializedKey(const InstanceHandle_t& handle)
{
    std::lock_guard<std::mutex> lock(changeMutex_);
    auto it = instanceChanges_.find(handle);
    return it == instanceChanges_.end() ? nullptr : it->second.keyBuffer;
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

	if (insChange.aliveWriter.empty()) {
		insChange.aliveWriter.insert({ writerGuid,ownershipStrength });
		if (!shared) {
			insChange.maxOwnershipStrength = { writerGuid,ownershipStrength };
		}
	} else {
		insChange.aliveWriter[writerGuid] = ownershipStrength;

		if (!shared && insChange.maxOwnershipStrength.first != writerGuid && insChange.maxOwnershipStrength.second >= ownershipStrength) {
			if (changeKind == NOT_ALIVE_UNREGISTERED || changeKind == NOT_ALIVE_DISPOSED_UNREGISTERED) {
				insChange.aliveWriter.erase(writerGuid);
			}
			return RETCODE_EXCLUSIVE_OWNERSHIP_QOS_NOT_MAX;
		}
		if (!shared) {
			insChange.maxOwnershipStrength.first = writerGuid;
			insChange.maxOwnershipStrength.second = ownershipStrength;
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
	case NOT_ALIVE_UNREGISTERED:
		insChange.aliveWriter.erase(writerGuid);
		if (insChange.aliveWriter.empty()) {
			if (insChange.instanceState != NOT_ALIVE_DISPOSED_INSTANCE_STATE) UpdateState(NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, insChange);
			insChange.maxOwnershipStrength = { GUID_UNKNOWN, 0 };
			break;
		}
		if (!shared && insChange.maxOwnershipStrength.first == writerGuid) {
			insChange.maxOwnershipStrength = { GUID_UNKNOWN, 0 };
			for (const auto& aliveWriter : insChange.aliveWriter) {
				if (aliveWriter.second > insChange.maxOwnershipStrength.second) {
					insChange.maxOwnershipStrength = aliveWriter;
				}
			}
		}
		break;
	case NOT_ALIVE_DISPOSED_UNREGISTERED:
		UpdateState(NOT_ALIVE_DISPOSED_INSTANCE_STATE, insChange);
		insChange.aliveWriter.erase(writerGuid);
     	if (!shared && insChange.maxOwnershipStrength.first == writerGuid) {
			insChange.maxOwnershipStrength = { GUID_UNKNOWN, 0 };
			for (const auto& aliveWriter : insChange.aliveWriter) {
				if (aliveWriter.second > insChange.maxOwnershipStrength.second) {
					insChange.maxOwnershipStrength = aliveWriter;
				}
			}
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
	if (cacheChange.instanceState == ALIVE_INSTANCE_STATE && state == NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
		counters_.instanceAlive -=  cacheChange.cacheChanges.size();
		counters_.instanceDisposed += cacheChange.cacheChanges.size();
	} else if (cacheChange.instanceState == ALIVE_INSTANCE_STATE && state == NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
		counters_.instanceAlive -= cacheChange.cacheChanges.size();
		counters_.instanceNoWriters += cacheChange.cacheChanges.size();
	} else if (cacheChange.instanceState == NOT_ALIVE_DISPOSED_INSTANCE_STATE && state == NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
		counters_.instanceDisposed -= cacheChange.cacheChanges.size();
		counters_.instanceNoWriters += cacheChange.cacheChanges.size();
	} else if (cacheChange.instanceState == NOT_ALIVE_DISPOSED_INSTANCE_STATE && state == ALIVE_INSTANCE_STATE) {
		counters_.instanceDisposed -= cacheChange.cacheChanges.size();
		counters_.instanceAlive += cacheChange.cacheChanges.size();
		cacheChange.disposedGenerationCount++;
	} else if (cacheChange.instanceState == NOT_ALIVE_NO_WRITERS_INSTANCE_STATE && state == ALIVE_INSTANCE_STATE) {
		counters_.instanceNoWriters -= cacheChange.cacheChanges.size();
		counters_.instanceAlive += cacheChange.cacheChanges.size();
		cacheChange.noWritersGenerationCount++;
	} else if (cacheChange.instanceState == NOT_ALIVE_NO_WRITERS_INSTANCE_STATE && state == NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
		counters_.instanceNoWriters -= cacheChange.cacheChanges.size();
		counters_.instanceDisposed += cacheChange.cacheChanges.size();
	}
	cacheChange.instanceState = state;
    cacheChange.stateChanged = std::chrono::steady_clock::now();
    if (state != ALIVE_INSTANCE_STATE) cacheChange.deadlineActive = false;
	counters_.change.store(true);
}

bool HistoryCache::IsKeyRegistered(const InstanceHandle_t& handle)
{
	std::lock_guard<std::mutex> lock(changeMutex_);
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
	std::lock_guard<std::mutex> lock(changeMutex_);
	return instanceChanges_.find(handle) != instanceChanges_.end();
}

void HistoryCache::DeleteKeyHistory(const InstanceHandle_t& handle)
{
	std::lock_guard<std::mutex> lock(changeMutex_);
	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor = instanceChanges_.find(handle);
	if (itor != instanceChanges_.end()) {
        for (const auto& cache : itor->second.cacheChanges) {
            RemoveSampleCounters(cache->sampleInfo.sample_state, itor->second.viewState, itor->second.instanceState);
            auto indexed = cacheChanges_.find(cache->sequenceNumber);
            if (indexed != cacheChanges_.end() && indexed->second == cache) cacheChanges_.erase(indexed);
        }
		instanceChanges_.erase(itor);
        changeVar_.notify_all();
	}
}

bool HistoryCache::IsFull(const InstanceHandle_t& handle)
{
	std::unique_lock<std::mutex> lock(changeMutex_);

	auto  changesItor = instanceChanges_.find(handle);
	if (changesItor == instanceChanges_.end()) {
		return false;
	}
	return changesItor->second.cacheChanges.size() == static_cast<size_t>( changesItor->second.maxSamples);
}
void HistoryCache::RemoveOldestChangeByInstance(const InstanceHandle_t& handle)
{
	std::unique_lock<std::mutex> lock(changeMutex_);

	auto  changesItor = instanceChanges_.find(handle);
	if (changesItor == instanceChanges_.end()) {
		return;
	}
	if (changesItor->second.cacheChanges.empty()) return;
	auto change = changesItor->second.cacheChanges.begin();
	RemoveSampleCounters((*change)->sampleInfo.sample_state, changesItor->second.viewState, changesItor->second.instanceState);
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
        RemoveSampleCounters(cache->sampleInfo.sample_state, changesItor->second.viewState, changesItor->second.instanceState);
        auto indexed = cacheChanges_.find(cache->sequenceNumber);
        if (indexed != cacheChanges_.end() && indexed->second == cache) cacheChanges_.erase(indexed);
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
                                RemoveSampleCounters(change->sampleInfo.sample_state, itor->second.viewState, itor->second.instanceState);
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
				return changesItor->second.cacheChanges.size() < static_cast<size_t>(changesItor->second.maxSamples);
			}
		});
	

}

SampleCounters HistoryCache::GetSampleCounters(bool trigger)
{
    PurgeExpired();
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
    if (itor->second.nextDeadline == std::chrono::steady_clock::time_point::max()) return false;
	handle = itor->first;
	nextDeadline = itor->second.nextDeadline;
	return true;
}

bool HistoryCache::SetNextDeadline(const InstanceHandle_t& handle, const std::chrono::steady_clock::time_point& nextDeadline, bool timeout)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
    auto found = instanceChanges_.find(handle);
    if (found == instanceChanges_.end()) return false;
	auto& readinstance = found->second;
    // Reader ingress/Writer departure can change the instance state after
    // the timer selected its due deadline. Check and advance under one lock.
    if (timeout && readinstance.instanceState != ALIVE_INSTANCE_STATE) {
        readinstance.nextDeadline = std::chrono::steady_clock::time_point::max();
        return false;
    }
	readinstance.nextDeadline = nextDeadline;
    if (!timeout) readinstance.deadlineActive = nextDeadline != std::chrono::steady_clock::time_point::max();

	if (timeout) {
		if (readinstance.maxOwnershipStrength.first != GUID_UNKNOWN) {
			readinstance.aliveWriter.erase(readinstance.maxOwnershipStrength.first);
			readinstance.maxOwnershipStrength = { GUID_t(), 0 };
			if (readinstance.aliveWriter.empty()) {
				UpdateState(NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, readinstance);
			} else  {
				for (const auto& aliveWriter : readinstance.aliveWriter) {
					if (aliveWriter.second > readinstance.maxOwnershipStrength.second) {
						readinstance.maxOwnershipStrength = aliveWriter;
					}
				}
			}
		}
	}
    return true;
}

bool HistoryCache::UpdateReaderDeadline(const InstanceHandle_t& handle, ChangeKind_t kind,
        const std::chrono::steady_clock::time_point& nextDeadline)
{
    std::lock_guard<std::mutex> lock(changeMutex_);
    auto found = instanceChanges_.find(handle);
    if (found == instanceChanges_.end()) return false;
    auto& instance = found->second;
    if (instance.instanceState != ALIVE_INSTANCE_STATE) {
        instance.nextDeadline = std::chrono::steady_clock::time_point::max();
    } else if (kind == ALIVE) {
        instance.nextDeadline = nextDeadline;
        instance.deadlineActive = true;
    }
    return true;
}

void HistoryCache::UpAllDeadline(const std::chrono::steady_clock::time_point& nextDeadline)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	for (auto& it : instanceChanges_) {
        if (it.second.deadlineActive && it.second.instanceState == ALIVE_INSTANCE_STATE)
            it.second.nextDeadline = nextDeadline;
	}
}

bool HistoryCache::GetNextTimeBasedFilte(InstanceHandle_t& handle, std::chrono::steady_clock::time_point& nextTime)
{
    std::lock_guard<std::mutex> lock(changeMutex_);
    bool found = false;
    for (const auto& entry : instanceChanges_) {
        if (entry.second.lastCache && (!found || entry.second.nextTimeBasedFilte < nextTime)) {
            found = true; handle = entry.first; nextTime = entry.second.nextTimeBasedFilte;
        }
    }
    return found;
}

void HistoryCache::SetNextTimeBasedFilte(const InstanceHandle_t& handle, std::shared_ptr<CacheChange>& cache)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
    auto found = instanceChanges_.find(handle);
    if (found == instanceChanges_.end()) { cache.reset(); return; }
	auto& readinstance = found->second;
	readinstance.nextTimeBasedFilte = std::chrono::steady_clock::time_point();
	cache = std::move(readinstance.lastCache);
}

void HistoryCache::RsetAllTimeBasedFilte()
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	for (auto& it : instanceChanges_) {
		it.second.nextTimeBasedFilte = std::chrono::steady_clock::time_point();
		it.second.lastCache.reset();
	}
}

void HistoryCache::RemoveRemoteWriter(const GUID_t& guid)
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	for (auto& ins : instanceChanges_) {
		if (!ins.second.aliveWriter.erase(guid)) continue;
		if (ins.second.aliveWriter.empty()) {
			ins.second.nextDeadline = std::chrono::steady_clock::time_point::max();
			ins.second.maxOwnershipStrength = { GUID_UNKNOWN, 0 };
			if (ins.second.instanceState == ALIVE_INSTANCE_STATE) UpdateState(NOT_ALIVE_NO_WRITERS_INSTANCE_STATE, ins.second);
		} else if (ins.second.maxOwnershipStrength.first == guid) {
			ins.second.maxOwnershipStrength = { GUID_UNKNOWN, 0 };
			for (const auto& aliveWriter : ins.second.aliveWriter) {
				if (aliveWriter.second > ins.second.maxOwnershipStrength.second) {
					ins.second.maxOwnershipStrength = aliveWriter;
				}
			}
		}
	}
}

void HistoryCache::CommitSamples(const std::vector<std::shared_ptr<CacheChange>>& changes, bool take)
{
    {
        std::lock_guard<std::mutex> lock(changeMutex_);
        for (const auto& change : changes) {
            auto instance = instanceChanges_.find(change->instanceHandle);
            if (instance == instanceChanges_.end()) continue;
            auto& ins = instance->second;
            auto pos = std::find(ins.cacheChanges.begin(), ins.cacheChanges.end(), change);
            if (pos == ins.cacheChanges.end()) continue;
            UpdateState(READ_SAMPLE_STATE, change);
            UpdateState(NOT_NEW_VIEW_STATE, ins);
            if (take) {
                RemoveSampleCounters(change->sampleInfo.sample_state, ins.viewState, ins.instanceState);
                auto indexed = cacheChanges_.find(change->sequenceNumber);
                if (indexed != cacheChanges_.end() && indexed->second == change) cacheChanges_.erase(indexed);
                ins.cacheChanges.erase(pos);
            }
        }
    }
    if (take) changeVar_.notify_all();
}

bool HistoryCache::MaintainReader(const ReaderDataLifecycleQosPolicy& policy,
        std::vector<InstanceHandle_t>& removed, std::chrono::steady_clock::time_point now)
{
    bool changed = false;
    const auto wallNow = std::chrono::system_clock::now();
    {
        std::lock_guard<std::mutex> lock(changeMutex_);
        for (auto it = instanceChanges_.begin(); it != instanceChanges_.end();) {
            auto& ins = it->second;
            auto delay = Duration_t(TIME_INFINITE);
            if (ins.instanceState == NOT_ALIVE_DISPOSED_INSTANCE_STATE)
                delay = policy.autopurge_disposed_samples_delay;
            else if (ins.instanceState == NOT_ALIVE_NO_WRITERS_INSTANCE_STATE && ins.aliveWriter.empty())
                delay = policy.autopurge_nowriter_samples_delay;
            const bool purge = delay != Duration_t(TIME_INFINITE) &&
                now >= ins.stateChanged && now - ins.stateChanged >= std::chrono::nanoseconds(delay.ToNs());
            for (auto cache = ins.cacheChanges.begin(); cache != ins.cacheChanges.end();) {
                if (purge || (*cache)->expiresAt <= wallNow) {
                    RemoveSampleCounters((*cache)->sampleInfo.sample_state, ins.viewState, ins.instanceState);
                    auto indexed = cacheChanges_.find((*cache)->sequenceNumber);
                    if (indexed != cacheChanges_.end() && indexed->second == *cache) cacheChanges_.erase(indexed);
                    cache = ins.cacheChanges.erase(cache);
                    changed = true;
                } else ++cache;
            }
            if (ins.lastCache && (purge || ins.lastCache->expiresAt <= wallNow)) {
                ins.lastCache.reset(); changed = true;
            }
            if (purge) {
                removed.push_back(it->first);
                it = instanceChanges_.erase(it);
                changed = true;
            } else ++it;
        }
    }
    if (changed) changeVar_.notify_all();
    return changed;
}
void HistoryCache::PurgeExpired() {
    std::vector<InstanceHandle_t> unused;
    MaintainReader(ReaderDataLifecycleQosPolicy{}, unused);
}

void HistoryCache::MarkDeadlineActive(const InstanceHandle_t& handle, bool alive) {
    std::lock_guard<std::mutex> lock(changeMutex_);
    auto found = instanceChanges_.find(handle);
    if (found != instanceChanges_.end() && alive && found->second.instanceState == ALIVE_INSTANCE_STATE)
        found->second.deadlineActive = true;
}
