#include "rtps/historycache/historycache.h"
#include "rtps/endpoint/writer.h"
USING_AGIDDS_NAMESPACE;

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

	instanceChanges_[change->instanceHandle].cacheChanges.push_back(change);

	changeMutex_.unlock();

	return true;
}

bool HistoryCache::RemoveChange(std::shared_ptr<CacheChange> change)
{
	if (nullptr == change) {
		return false;
	}

	changeMutex_.lock();
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
	changeMutex_.unlock();

	changeVar_.notify_all();

	return true;
}

std::shared_ptr<CacheChange> HistoryCache::GetChange(const SequenceNumber_t& seqNum)
{
	std::shared_ptr<CacheChange> pCacheChange = nullptr;

	changeMutex_.lock();

	if (cacheChanges_.end() != cacheChanges_.find(seqNum)) {
		pCacheChange = cacheChanges_[seqNum];
	}
		
	changeMutex_.unlock();

	return pCacheChange;

}

SequenceNumber_t HistoryCache::GetSeqNumMin()
{
	std::unique_lock<std::mutex> lock(changeMutex_);
	if (0 == cacheChanges_.size()) {
		return SEQUENCENUMBER_UNKNOWN;
	}

	return cacheChanges_.begin()->first;
}

SequenceNumber_t HistoryCache::GetSeqNumMax()
{
	std::unique_lock<std::mutex> lock(changeMutex_);

	if (0 == cacheChanges_.size()) {
		return SEQUENCENUMBER_UNKNOWN;
	}

	return cacheChanges_.rbegin()->first;
}

ReturnCode_t HistoryCache::ReadOrTakeInstance(SerializedBufferSeq& dataValues, SampleInfoSeq& sampleInfos, int32_t maxSamples, InstanceHandle_t handle,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouleTake)
{
	changeMutex_.lock();
	auto itor = instanceChanges_.find(handle);
	if (itor != instanceChanges_.end() ) {
		
		for (auto cache = itor->second.cacheChanges.begin(); cache != itor->second.cacheChanges.end(); ) {


			/* 判断状态是否符合,数据状态不符合不条件数据  */
			if ((0 == (sampleStates & (*cache)->sampleInfo.sample_state)) ||
				(0 == (viewStates & (*cache)->sampleInfo.view_state)) ||
				(0 == (instanceStates & (*cache)->sampleInfo.instance_state))) {
				cache++;
				continue;
			}
			/* jdxcz question 给SampleInfo 进行赋值 */
			SampleInfo sampleInfo;
			sampleInfo = (*cache)->sampleInfo;
			if ((*cache)->kind == ALIVE) {
				sampleInfo.valid_data = true;
			}
			dataValues.push_back(*(*cache)->serializedBuffer);
			sampleInfos.push_back(sampleInfo);

			/* 修改数据的状态  */
			(*cache)->sampleInfo.sample_state = READ_SAMPLE_STATE;
			(*cache)->sampleInfo.view_state = NOT_NEW_VIEW_STATE;
			/* 获取后删除数据 */
			if (shouleTake) {
				if ((*cache)->serializedBuffer) {
					delete (*cache)->serializedBuffer;
					(*cache)->serializedBuffer = nullptr;
				}
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

	return RETCODE_OK;
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

	return RETCODE_OUT_OF_RESOURCES;
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