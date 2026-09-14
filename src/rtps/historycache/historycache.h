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
#ifndef TRAVODDS_RTPS_HISTORYCACHE_HISTORYCACHE_H
#define TRAVODDS_RTPS_HISTORYCACHE_HISTORYCACHE_H

#include "rtps/basictypertps.h"
#include "rtps/messages/parameter.h"
#include "rtps/messages/cdrmessage_t.h"
#include "dcps/subscribe/states.h"
#include "dcps/base/retcode.h"
#include "dcps/topic/typesupport.h"
#include "common/memory/memorypool.h"
#include <atomic>
#include <condition_variable>
TRAVODDS_NAMESPACE_BEGIN

class Writer;

struct SampleCounters
{
	// READ_SAMPLE_STATE
	uint64_t sampleRead = 0;
	// NOT_READ_SAMPLE_STATE
	uint64_t sampleNotRead = 0;

	// NEW_VIEW_STATE
	uint64_t viewNew = 0;
	// NOT_NEW_VIEW_STATE
	uint64_t viewNotNew = 0;

	// ALIVE_INSTANCE_STATE
	uint64_t instanceAlive = 0;
	// NOT_ALIVE_DISPOSED_INSTANCE_STATE
	uint64_t instanceDisposed = 0;
	// NOT_ALIVE_NO_WRITERS_INSTANCE_STATE
	uint64_t instanceNoWriters = 0;
	/* 用于查看值是否被更改 */
	std::atomic_bool change;

	SampleCounters()
		: change(false) {

	}

	SampleCounters(const SampleCounters& other)
		: sampleRead(other.sampleRead),
		sampleNotRead(other.sampleNotRead),
        viewNew(other.viewNew),
        viewNotNew(other.viewNotNew),
        instanceAlive(other.instanceAlive),
        instanceDisposed(other.instanceDisposed),
        instanceNoWriters(other.instanceNoWriters),
        change(other.change.load()) {
	}

	SampleCounters& operator=(const SampleCounters& other)
	{
		if (this != &other) {
			sampleRead = other.sampleRead;
			sampleNotRead = other.sampleNotRead;
			viewNew = other.viewNew;
			viewNotNew = other.viewNotNew;
			instanceAlive = other.instanceAlive;
			instanceDisposed = other.instanceDisposed;
			instanceNoWriters = other.instanceNoWriters;
			change.store(other.change.load());
		}
		return *this;
	}
};

class CacheSerializedBuffer : public SerializedBuffer {
public:
	std::shared_ptr<Message_t> originMessage;
	CacheSerializedBuffer()
		: SerializedBuffer()
		, originMessage(nullptr) {
	}
	virtual ~CacheSerializedBuffer() {
	}
};

/**
 * @brief CacheChange 类, 包含有关特定缓存更改的信息。
 */
class CacheChange {
public:
	ChangeKind_t kind;
	GUID_t writerGuid;
	/* 实例 */
	InstanceHandle_t instanceHandle;
	SequenceNumber_t sequenceNumber;
	//TODO 待添加
	ParameterList_t inlineQos;
	/* CacheChange的相关状态  */
	SerializedBuffer* serializedBuffer;
	/* 此处 view_state,instance_state ,sample_rank, generation_rank, absolute_generation_rank字段无效 */
	SampleInfo sampleInfo;
	Time_t sourceTimespace;

	/* 分片数据 */
	/* 每个分片大小 */
	uint32_t fraqmentSize;
	/* 分片数据总大小 */
	uint32_t dataSize;
	/* 分片个数 */
	uint32_t fragmentTatolNum;
	/* 所有的数据分片数据序号，收到一个分片数据，删除分片序号 */
	std::set<uint32_t> fragmentSeq;
	std::map<uint32_t, SerializedBuffer*> fragmentBufferMap;
	/* 需要过滤的订阅者 */
	std::set<GUID_t> filterReaders_;

	// Related Sample Identity (Extension for RPC)
	SampleIdentity related_sample_identity;

	/**
	 * @brief 构造函数。
	 */
	CacheChange() : kind(ALIVE), 
		sequenceNumber(SEQUENCENUMBER_INVALID), 
		serializedBuffer(nullptr), 
		sourceTimespace(TIME_INVALID), 
		fraqmentSize(0),
		dataSize(0), 
		fragmentTatolNum(0),
        memoryPool(MemoryPool::GetInstance())
	{
	}

	/**
	 * @brief 析构函数。
	 */
	~CacheChange() {
		for (const auto& it : fragmentBufferMap) {
			delete it.second;
		}
		fragmentBufferMap.clear();
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

	/**
	 * @brief 构造函数。
	 */
	CacheChangeVector(SerializedBuffer* buffer, int32_t maxSize) : keyBuffer(buffer), maxSamples(maxSize) {
	}

	/**
	 * @brief 构造函数。
	 */
	CacheChangeVector() : keyBuffer(nullptr), maxSamples(0) {
	}

	/**
	 * @brief 析构函数。
	 */
	~CacheChangeVector() {
	}

	std::vector<std::shared_ptr<CacheChange>> cacheChanges;
	std::shared_ptr<SerializedBuffer> keyBuffer;

	/* 实例再创或新创后，未被读过 */
	ViewStateKind viewState = ViewStateKind::NEW_VIEW_STATE;
	/* 实例状态，alive表示实例还存在，disposed表示实例被删除，no_writers表示实例没有写入者 */
	InstanceStateKind instanceState = InstanceStateKind::ALIVE_INSTANCE_STATE;

	/* 代表实例在收到样本时实例由DataWriter显式处理后变为存活状态的次数 */
	int32_t 					disposedGenerationCount = 0;
	/* 代表实例因为没有写入者而被处理后重新变为存活状态的次数 */
	int32_t 					noWritersGenerationCount = 0;

	std::chrono::steady_clock::time_point nextDeadline;
	/* guid  OwnershipStrength*/
	std::map<GUID_t, int32_t> aliveWriter;
	std::pair<GUID_t, int32_t> maxOwnershipStrength;

	std::chrono::steady_clock::time_point nextTimeBasedFilte;
	/* 最新数据, cache未添加 */
	std::shared_ptr<CacheChange> lastCache;

	int32_t maxSamples;
};

class HistoryCache {
public:

	/**
	 * @brief 构造函数。
	 */
	HistoryCache(const HistoryQosPolicy& historyQos, const ResourceLimitsQosPolicy& resourceLimitsQos, const TopicKind_t& topicKind);
    
	/**
	 * @brief 析构函数。
	 */
	~HistoryCache();

	/**
	 * @brief 内存满判定函数。
	 * 
	 * @return 获取成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	bool IsFull();

	/**
	 * @brief 添加发送报文请求。
	 *
	 * @param [in] change 发送报文请求。
	 * 
	 * @return 添加成功则返回true，否则返回false。
	 */
	bool AddChange(std::shared_ptr<CacheChange> change);

	/**
	 * @brief 删除发送报文请求。
	 *
	 * @param [in] seqNum 发送报文请求序号。
	 *
	 * @return 删除成功则返回true，否则返回false。
	 */
	bool RemoveChange(const SequenceNumber_t& seqNum);

	/**
	 * @brief 根据报文发送序号获取报文发送请求。
	 *
	 * @param [in] seqNum 报文发送序号。
	 *
	 * @return 返回报文发送请求。
	 */
	std::shared_ptr<CacheChange> GetChange(const SequenceNumber_t& seqNum);

	/**
	 * @brief 根据报文源时间戳获取报文源时间戳最小的报文缓存。
	 *
	 * @return 返回报文缓存。
	 */
	std::shared_ptr<CacheChange> GetMinSourceTimespace();

	/**
	 * @brief 从保存数据的容器中获取报文序号最小值。
	 *
	 * @return 返回报文序号最小值。
	 */
	SequenceNumber_t GetSeqNumMin();

	/**
	 * @brief 从保存数据的容器中获取报文序号最大值。
	 *
	 * @return 返回报文序号最大值。
	 */
	SequenceNumber_t GetSeqNumMax();
	
	/**
	 * @brief 读取所有的实例的数据。
	 *
	 * @param [in,out] dataValues     将返回接收到的数据样本的LoanableCollection对象。
	 * @param [in,out] sampleInfos    将返回接收到的样本信息的SampleInfoSeq对象。
	 * @param [in]     maxSamples     要返回的最大样本数。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 则将返回尽可能多的样本，直到达到上述限制。
	 * @param [in]     handle         实例句柄。
	 * @param [in]     sampleStates   只有sampleStates与其中一个匹配的数据样本才会返回。
	 * @param [in]     viewStates     只有viewStates与其中一个匹配的数据样本才会返回。
	 * @param [in]     instanceStates 仅返回instanceStates与其中一个匹配的数据样本。
	 * @param [in]     shouldTake     获取后是否删除数据标志。
	 *
	 * @return 读取成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t ReadOrTakeInstance(std::vector<std::shared_ptr<CacheChange>>& dataValues, SampleInfoSeq& sampleInfos, int32_t maxSamples,InstanceHandle_t handle,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouleTake = false);

	/**
	 * @brief 判断键值是否已注册。
	 *
	 * @param [in] handle 需判定的句柄。
	 *
	 * @return 已注册返回true，未注册返回false。
	 */
	bool IsKeyRegistered(const InstanceHandle_t& handle);

	/**
	 * @brief 添加键值及其对应实例到键值管理容器。
	 *
	 * @param [in] handle 包含键值的句柄。
	 * @param [in] instace 正在注册实例的样本的序列化有效样本。
	 *
	 * @return 读取成功返回RETCODE_OK，否则返回RETCODE_OUT_OF_RESOURCES
	 */
	ReturnCode_t AddKeyHistory(const InstanceHandle_t& handle, SerializedBuffer*& instace);

	/**
	 * @brief 从键值管理容器中删除对应的键值及其实例。
	 *
	 * @param [in] handle 包含键值的句柄。
	 */
	void DeleteKeyHistory(const InstanceHandle_t& handle);


	/**
	 * @brief 从键值管理容器中获取对应键值的实例。
	 *
	 * @param [in] handle 包含键值的句柄。
	 *
	 * @return 键值对应的实例样本。
	 */
	SerializedBuffer* get_key_value(const InstanceHandle_t& handle);

	/**
	 * @brief 在键值管理容器中判定对应键值的实例是否存在。
	 *
	 * @param [in] handle 包含键值的句柄。
	 *
	 * @return 存在则返回true，不存在则返回false。
	 */
	bool IsInstancePresent(const InstanceHandle_t& handle);

	/**
	 * @brief 判断对应的instance所保存的数据容器是否已经保存满。
	 *
	 * @param [in] handle 包含键值的句柄。
	 *
	 * @return 已满则返回true，否则返回false。
	 */
	bool IsFull(const InstanceHandle_t& handle);

	/**
	 * @brief 移除对应Instance最旧的数据。
	 *
	 * @param [in] handle 包含键值的句柄。
	 */
	void RemoveOldestChangeByInstance(const InstanceHandle_t& handle);

	/**
	 * @brief 移除对应instance所有数据。
	 *
	 * @param [in] handle 包含键值的句柄。
	 */
	void RemoveChangeByInstance(const InstanceHandle_t& handle);

	/**
	 * @brief 等待空间添加数据。
	 *
	 * @param [in] time_out       数据等待时间。
	 * @param [in] pWriter        数据发送者指针。
	 * @param [in] handle         数据句柄。
	 * @param [in] transientLocal 数据持久化是否为本地模式标志。
	 * 
	 * @return 已满则返回true，否则返回false。
	 */
	bool WriterWaitFor(const std::chrono::nanoseconds& time_out, Writer* pWriter, const InstanceHandle_t& handle, bool transientLocal);

	/**
	 * @brief 获取样本计数器。
	 *
	 * @param [in] trigger 触发器，用于触发ReadConditionImpl读取标志。
	 *
	 * @return 样本计数器对象。
	 */
	SampleCounters GetSampleCounters(bool trigger = true);

	/**
	 * @brief 获取下一个截止时间。
	 *
	 * @param [out] handle       通信实例句柄。
	 * @param [out] nextDeadline 下一个截止时间。
	 *
	 * @return 获取成功返回true，获取失败返回false。
	 */
	bool GetNextDeadline(InstanceHandle_t& handle, std::chrono::steady_clock::time_point& nextDeadline);

	/**
	 * @brief 重新设置实例句柄handle对应的截止时间。
	 *
	 * @param [in] handle       通信实例句柄。
	 * @param [in] nextDeadline 新的下一个截止日期。
	 */
	void SetNextDeadline(const InstanceHandle_t& handle, const std::chrono::steady_clock::time_point& nextDeadline, bool timeout = false);

	/**
	 * @brief 重新设置所有实例句柄handle对应的截止时间。
	 *
	 * @param [in] nextDeadline 新的下一个截止日期。
	 */
	void UpAllDeadline(const std::chrono::steady_clock::time_point& nextDeadline);

	bool GetNextTimeBasedFilte(InstanceHandle_t& handle, std::chrono::steady_clock::time_point& nextTime);
	void SetNextTimeBasedFilte(const InstanceHandle_t& handle, std::shared_ptr<CacheChange>& cache);
	void RsetAllTimeBasedFilte();

	void RemoveRemoteWriter(const GUID_t& guid);

private:
	/**
	 * @brief 在instanceChanges_中查找键或在未找到时尝试添加键的函数。
	 *
	 * @param [in] handle         数据句柄。
	 * @param [in] instace        正在注册实例的样本的序列化有效样本。
	 * @param [out] vitOut        指向给定键值的映射迭代器。
	 * 
	 * @return 读取成功返回RETCODE_OK，否则返回RETCODE_OUT_OF_RESOURCES
	 */
	ReturnCode_t FindORAddKey(const InstanceHandle_t& handle, SerializedBuffer*& instace, std::map<InstanceHandle_t, CacheChangeVector>::iterator* vitOut);

	/**
	 * @brief 添加样本计数器。
	 *
	 * @param [in] sampleState   SampleStateKind位掩码。
	 * @param [in] viewState     ViewStateKind位掩码。
	 * @param [in] instanceState 实例句柄。
	 */
	void AddSampleCounters(const SampleStateKind& sampleState, const ViewStateKind& viewState, const InstanceStateKind& instanceState);

	/**
	 * @brief 删除样本计数器。
	 *
	 * @param [in] sampleState   SampleStateKind位掩码。
	 * @param [in] viewState     ViewStateKind位掩码。
	 * @param [in] instanceState 实例句柄。
	 */
	void RemoveSampleCounters(const SampleStateKind& sampleState, const ViewStateKind& viewState, const InstanceStateKind& instanceState);

	/**
	 * @brief 更新样本实例句柄。
	 *
	 * @param [in] handle			 实例句柄。
	 * @param [in] changeKind		 请求状态。
	 * @param [in] writerGuid		 数据写者GUID。
	 * @param [in] ownershipStrength 所有权强度。
	 * @param [in] shared            是否为SHARED模式。
	 * 
	 * @return 读取成功返回RETCODE_OK，否则返回RETCODE_OUT_OF_RESOURCES
	 */
	ReturnCode_t UpdataSampleInstance(const InstanceHandle_t& handle, const ChangeKind_t& changeKind, const GUID_t& writerGuid, const int32_t ownershipStrength, bool shared);

	/**
	 * @brief 更新样本状态。
	 *
	 * @param [in]		state	新的SampleStateKind样本状态。
	 * @param [in，out] cache	样本。
	 */
	void UpdateState(const SampleStateKind& state, const std::shared_ptr<CacheChange>& cache);

	/**
	 * @brief 更新样本状态。
	 *
	 * @param [in]		state		新的ViewStateKind样本状态。
	 * @param [in，out] cacheChange	样本。
	 */
	void UpdateState(const ViewStateKind& state, CacheChangeVector& cacheChange);

	/**
	 * @brief 更新样本状态。
	 *
	 * @param [in]		state		新的InstanceStateKind样本状态。
	 * @param [in，out] cacheChange	样本。
	 */
	void UpdateState(const InstanceStateKind& state, CacheChangeVector& cacheChange);

private:
    
	SampleCounters counters_;

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

	/* wangyi lifespanQos 添加，要到期的数据 */
	std::shared_ptr<CacheChange> MinSourceTimespaceCache_ = nullptr;
	/* 互斥锁，互斥访问MinSourceTimespaceCache_  */
	std::mutex  MinSourceTimespaceCacheMutex_;

	friend class Reader;
};

TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_RTPS_HISTORYCACHE_HISTORYCACHE_H
