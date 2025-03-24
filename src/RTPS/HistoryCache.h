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
#pragma once

#include "RTPS/BasicTypeRTPS.h"
#include "BuiltinTopicData.h"
#include "States.h"
#include "RetCode.h"
#include "DDS/TypeSupport.h"
#include "System/MemoryPool/MemoryPool.h"
AGIDDS_NAMESPACE_BEGIN

struct CacheChange {

	ChangeKind_t kind;
	GUID_t writerGuid;
	InstanceHandle_t instanceHandle;
	SequenceNumber_t sequenceNumber;
	/* CacheChange的相关状态  */
	SerializedBuffer* serializedBuffer;
	SampleInfo sampleInfo;

	CacheChange() {
		serializedBuffer = nullptr;
	}

	~CacheChange() {
		while(nullptr != serializedBuffer) {
			SerializedBuffer* del = serializedBuffer;
			serializedBuffer = serializedBuffer->next;
			del->buffer -= BUFF_SHIFT;
			MemoryPool::GetInstance().releaseBuffer(del->buffer);
			delete del;
		}
	}
};

class CacheChangeVector {
public:
	CacheChangeVector()
	{
		keyBuffer = new SerializedBuffer();
	}

	~CacheChangeVector()
	{
		delete keyBuffer;
		keyBuffer = NULL;
	}

	vector<CacheChange*> cacheChanges;
	SerializedBuffer* keyBuffer;
};

class HistoryCache {
public:

	HistoryCache(HistoryQosPolicy qos);

	bool IsFull();

	bool AddChange(CacheChange* change);

	bool RemoveChange(CacheChange* change);

	CacheChange* GetChange(SequenceNumber_t& seqNum);

	SequenceNumber_t GetSeqNumMin();

	SequenceNumber_t GetSeqNumMax();

	
	/* read 获取数据 */
	ReturnCode_t ReadOrTake(SerializedBufferSeq& dataValues, SampleInfoSeq& sampleInfos, int maxSamples,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouleTake = false);

	ReturnCode_t ReadOrTakeInstance(SerializedBufferSeq& dataValues, SampleInfoSeq& sampleInfos, int maxSamples,InstanceHandle_t handle,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouleTake = false);

	/* instance */
	bool IsKeyRegistered(const InstanceHandle_t& handle);

	bool AddKeyHistory(const InstanceHandle_t& handle, SerializedBuffer*& instace);

	void DeleteKeyHistory(const InstanceHandle_t& handle);

	SerializedBuffer* get_key_value(const InstanceHandle_t& handle);

	bool IsInstancePresent(const InstanceHandle_t& handle);

private:
	bool FindORAddKey(const InstanceHandle_t& handle, SerializedBuffer *& instace, std::map<InstanceHandle_t, CacheChangeVector>::iterator* vitOut);

private:
	//vector<CacheChange*> m_changes;
	/* 保存数据的容器，键值为数据的序号  */
	map<SequenceNumber_t, CacheChange*> cacheChanges_;

	/* 互斥锁，互斥访问CacheChange  */
	std::mutex changeMutex_;

	/* instance */
	std::map<InstanceHandle_t, CacheChangeVector> instanceChanges_;

	TopicKind_t topicKind_;

	ResourceLimitsQosPolicy resourceLimitedQos_;

	HistoryQosPolicy qos_;

	int depth_;
};

AGIDDS_NAMESPACE_END
