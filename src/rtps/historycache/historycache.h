//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef AGIDDS_RTPS_HISTORYCACHE_HISTORYCACHE_H
#define AGIDDS_RTPS_HISTORYCACHE_HISTORYCACHE_H

#include "rtps/basictypertps.h"
#include "rtps/messages/parameter.h"
#include "dcps/subscribe/states.h"
#include "dcps/base/retcode.h"
#include "dcps/topic/typesupport.h"
#include "common/memory/memorypool.h"
#include <atomic>
#include <condition_variable>
AGIDDS_NAMESPACE_BEGIN

class Writer;

class CacheChange {
public:
	ChangeKind_t kind;
	GUID_t writerGuid;
	InstanceHandle_t instanceHandle;
	SequenceNumber_t sequenceNumber;
	//TODO 待添加
	ParameterList_t inlineQos;
	/* CacheChange的相关状态  */
	SerializedBuffer* serializedBuffer;
	SampleInfo sampleInfo;
	Time_t sourceTimespace;

	/* 分片数据 */
	/* 每个分片大小 */
	int32_t fraqmentSize;
	/* 分片数据总大小 */
	int32_t dataSize;
	/* 分片个数 */
	int32_t fragmentTatolNum;
	/* 所有的数据分片数据序号，收到一个分片数据，删除分片序号 */
	std::set<int32_t> fragmentSeq;

	CacheChange() : kind(ALIVE), 
		sequenceNumber(SEQUENCENUMBER_INVALID), 
		serializedBuffer(nullptr), 
		sourceTimespace(TIME_INVALID), 
		fraqmentSize(0),
		dataSize(0), 
		fragmentTatolNum(0),
        memoryPool(MemoryPool::GetInstance())
	{
		serializedBuffer = nullptr;
	}

	~CacheChange() {
		while(nullptr != serializedBuffer) {
			SerializedBuffer* del = serializedBuffer;
			serializedBuffer = serializedBuffer->next;
			if (del->origin_buffer) {
				memoryPool->releaseBuffer(del->origin_buffer);
			}
			delete del;
		}
	}
private:	
	std::shared_ptr<MemoryPool> memoryPool;
};

class CacheChangeVector {
public:
	CacheChangeVector(SerializedBuffer* buffer, int32_t maxSize) : keyBuffer(buffer), maxSamples(maxSize){
	}

	CacheChangeVector() : keyBuffer(nullptr), maxSamples(0) {
	}
	~CacheChangeVector() {
	}

	std::vector<std::shared_ptr<CacheChange>> cacheChanges;
	std::shared_ptr<SerializedBuffer> keyBuffer;

	
	int32_t maxSamples;

};

class HistoryCache {
public:

	HistoryCache(const HistoryQosPolicy& historyQos, const ResourceLimitsQosPolicy& resourceLimitsQos, const TopicKind_t& topicKind);
    
	~HistoryCache();

	bool IsFull();

	bool AddChange(std::shared_ptr<CacheChange> change);

	bool RemoveChange(std::shared_ptr<CacheChange> change);

	std::shared_ptr<CacheChange> GetChange(const SequenceNumber_t& seqNum);

	SequenceNumber_t GetSeqNumMin();

	SequenceNumber_t GetSeqNumMax();

	ReturnCode_t ReadOrTakeInstance(SerializedBufferSeq& dataValues, SampleInfoSeq& sampleInfos, int32_t maxSamples,InstanceHandle_t handle,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouleTake = false);

	/* instance */
	bool IsKeyRegistered(const InstanceHandle_t& handle);

	ReturnCode_t AddKeyHistory(const InstanceHandle_t& handle, SerializedBuffer*& instace);

	void DeleteKeyHistory(const InstanceHandle_t& handle);

	SerializedBuffer* get_key_value(const InstanceHandle_t& handle);

	bool IsInstancePresent(const InstanceHandle_t& handle);

	/* 判断对应的instance所保存的数据是否已经保存满 */
	bool IsFull(const InstanceHandle_t& handle);

	/* 移除对应Instance最旧的数据 */
	void RemoveOldestChangeByInstance(const InstanceHandle_t& handle);

	/* 移除对应instance所有数据 */
	void RemoveChangeByInstance(const InstanceHandle_t& handle);

	/* 等待空间添加数据  */
	bool WriterWaitFor(const std::chrono::nanoseconds& time_out, Writer* pWriter, const InstanceHandle_t& handle, bool transientLocal);

private:
	ReturnCode_t FindORAddKey(const InstanceHandle_t& handle, SerializedBuffer*& instace, std::map<InstanceHandle_t, CacheChangeVector>::iterator* vitOut);



private:
	//vector<CacheChange*> m_changes;
	/* 保存数据的容器，键值为数据的序号  */
	std::map<SequenceNumber_t, std::shared_ptr<CacheChange>> cacheChanges_;

	/* 互斥锁，互斥访问CacheChange  */
	std::mutex  changeMutex_;

	/* instance */
	std::map<InstanceHandle_t, CacheChangeVector> instanceChanges_;

	TopicKind_t topicKind_;

	ResourceLimitsQosPolicy resourceLimitesQos_;

	HistoryQosPolicy historyQos_;

	//int32_t depth_;

	int32_t maxSamples_;

	int32_t maxSamplesPerInstance_;

	std::condition_variable changeVar_;

	friend class Reader;
};

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_HISTORYCACHE_HISTORYCACHE_H
