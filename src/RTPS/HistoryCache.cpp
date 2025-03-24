#include "HistoryCache.h"

USING_AGIDDS_NAMESPACE;

HistoryCache::HistoryCache(HistoryQosPolicy qos)
{
	qos_ = qos;
	depth_ = 1024;

	if (KEEP_LAST_HISTORY_QOS == qos_.kind) {
		depth_ = qos_.depth;
	}
}

bool HistoryCache::IsFull()
{
	if (cacheChanges_.size() >= depth_) {
		return true;
	}

	return false;
}

bool HistoryCache::AddChange(CacheChange* change)
{
	if (nullptr == change) {
		return false;
	}

	changeMutex_.lock();

	cacheChanges_[change->sequenceNumber] = change;

	changeMutex_.unlock();

	return true;
}

bool HistoryCache::RemoveChange(CacheChange* change)
{
	if (nullptr == change) {
		return false;
	}

	changeMutex_.lock();

	cacheChanges_.erase(change->sequenceNumber);

	delete change;

	changeMutex_.unlock();

	return true;
}

CacheChange* HistoryCache::GetChange(SequenceNumber_t& seqNum)
{
	CacheChange* pCacheChange = nullptr;

	changeMutex_.lock();

	if (cacheChanges_.end() != cacheChanges_.find(seqNum)) {
		pCacheChange = cacheChanges_[seqNum];
	}
		
	changeMutex_.unlock();

	return pCacheChange;

}

SequenceNumber_t HistoryCache::GetSeqNumMin()
{

	if (0 == cacheChanges_.size()) {
		return SEQUENCENUMBER_UNKNOWN;
	}

	return cacheChanges_.begin()->first;
}

SequenceNumber_t HistoryCache::GetSeqNumMax()
{
	if (0 == cacheChanges_.size()) {
		return SEQUENCENUMBER_UNKNOWN;
	}

	return cacheChanges_.rbegin()->first;
}

ReturnCode_t HistoryCache::ReadOrTake(SerializedBufferSeq& dataValues, SampleInfoSeq& sampleInfos, int maxSamples,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouleTake)
{
	changeMutex_.lock();

	for (auto itor : cacheChanges_) {
		/* 判断状态是否符合,数据状态不符合不条件数据  */
		if ((0 != (sampleStates & itor.second->sampleInfo.sample_state)) || 
			(0 != (viewStates & itor.second->sampleInfo.view_state)) || 
			(0 != (instanceStates & itor.second->sampleInfo.instance_state))) {
			continue;
		}
			

		/* jdxcz question 给SampleInfo 进行赋值 */
		SampleInfo sample_info;
		sample_info = itor.second->sampleInfo;
		
		dataValues.push_back(*itor.second->serializedBuffer);
		sampleInfos.push_back(sample_info);

		/* 修改数据的状态  */
		itor.second->sampleInfo.sample_state = READ_SAMPLE_STATE;

		/* 获取后删除数据 */
		if (shouleTake) {
			RemoveChange(itor.second);
		}
			
		/* 获取的数据个数 大于 max_samples设置，停止获取数据  */
		if (dataValues.length() == maxSamples) {
			break;
		}
			
	}

	changeMutex_.unlock();

	return RETCODE_OK;
}

ReturnCode_t HistoryCache::ReadOrTakeInstance(SerializedBufferSeq& dataValues, SampleInfoSeq& sampleInfos, int maxSamples, InstanceHandle_t handle,
	SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouleTake)
{
	changeMutex_.lock();

	for (auto itor : cacheChanges_) {
		//itor.second;
		if (handle != itor.second->instanceHandle) {
			continue;
		}

		/* 判断状态是否符合,数据状态不符合不条件数据  */
		if ((0 != (sampleStates & itor.second->sampleInfo.sample_state)) || 
			(0 != (viewStates & itor.second->sampleInfo.view_state)) || 
			(0 != (instanceStates & itor.second->sampleInfo.instance_state))) {
			continue;
		}
		
		/* jdxcz question 给SampleInfo 进行赋值 */
		SampleInfo sample_info;
		sample_info = itor.second->sampleInfo;

		dataValues.push_back(*itor.second->serializedBuffer);
		sampleInfos.push_back(sample_info);

		/* 修改数据的状态  */
		itor.second->sampleInfo.sample_state = READ_SAMPLE_STATE;
		itor.second->sampleInfo.view_state = NOT_NEW_VIEW_STATE;

		/* 获取后删除数据 */
		if (shouleTake) {
			RemoveChange(itor.second);
		}
			
		/* 获取的数据个数 大于 max_samples设置，停止获取数据  */
		if (dataValues.length() == maxSamples) {
			break;
		}
			
	}

	changeMutex_.unlock();

	return RETCODE_OK;
}


bool HistoryCache::AddKeyHistory(const InstanceHandle_t & handle, SerializedBuffer *& instace)
{

	if (topicKind_.value == NO_KEY)	{
		return false;
	}

	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor;
	bool result = FindORAddKey(handle,instace, &itor);
	if (result)	{
		instace = itor->second.keyBuffer;
	}
	return result;
}

bool HistoryCache::FindORAddKey(const InstanceHandle_t & handle, SerializedBuffer *& instace, std::map<InstanceHandle_t, CacheChangeVector>::iterator * vitOut)
{
	static_cast<void>(instace);

	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor;
	itor = instanceChanges_.find(handle);
	if (itor != instanceChanges_.end()) {
		*vitOut = itor;
		return true;
	}

	if (static_cast<int>(instanceChanges_.size()) < resourceLimitedQos_.max_instances) {
		itor = instanceChanges_.insert(std::make_pair(handle, CacheChangeVector())).first;
		*vitOut = itor;
		return true;
	}

	return false;
}

bool HistoryCache::IsKeyRegistered(const InstanceHandle_t & handle)
{
	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor;
	itor = instanceChanges_.find(handle);
	return itor != instanceChanges_.end();
}

SerializedBuffer * HistoryCache::get_key_value(const InstanceHandle_t & handle)
{
	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor = instanceChanges_.find(handle);
	if (itor != instanceChanges_.end()) {
		return itor->second.keyBuffer;
	}
	return nullptr;
}

bool HistoryCache::IsInstancePresent(const InstanceHandle_t & handle)
{
	return instanceChanges_.find(handle) != instanceChanges_.end();
}

void HistoryCache::DeleteKeyHistory(const InstanceHandle_t & handle)
{
	std::map<InstanceHandle_t, CacheChangeVector>::iterator itor = instanceChanges_.find(handle);
	if (itor != instanceChanges_.end()) {
		instanceChanges_.erase(itor);
	}
}
