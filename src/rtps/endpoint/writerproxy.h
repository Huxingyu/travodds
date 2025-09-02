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
#ifndef TRAVODDS_RTPS_ENDPOINT_WRITERPROXY_H
#define TRAVODDS_RTPS_ENDPOINT_WRITERPROXY_H

#include "rtps/basictypertps.h"
#include "rtps/historycache/historycache.h"
#include "rtps/endpoint/changefromwriter.h"
#include "rtps/messages/cdrmessage_t.h"
#include <atomic>

TRAVODDS_NAMESPACE_BEGIN

/**
 * WriterProxy类,保存特定Writer相对于远端DataReader的状态。
 */
class WriterProxy {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] writerGuid DataWriter Guid。
	 * @param [in] groupGuid DataWriter所属订阅者Guid。
	 * @param [in] unicastLocatorList 单播地址列表。
	 * @param [in] multicastLocatorList 组播地址列表。
	 */
	WriterProxy(const GUID_t& writerGuid, const GUID_t& groupGuid, const LocatorSeq& unicastLocatorList, const LocatorSeq& multicastLocatorList);

	/**
	 * @brief 对序列号[first,last]的CacheChange进行初始化。
	 *
	 * @param [in] first 第一个序列号。
	 * @param [in] last  最后一个序列号。
	 */
	void Init(SequenceNumber_t first, SequenceNumber_t last);

	/**
	 * @brief 获取可用更改最大值的序列号最大值。
	 *
	 * @return 可用的最大序列值。
	 */
	SequenceNumber_t AvailableChangesMax();

	/**
	 * @brief 确认不可更改集合。
	 *
	 * @param [in] seqNumSeq 序列号集合。
	 * @param [in] filteredCount  标志序列号。
	 */
	void NonAvailableChangeSet(SeqNumberVec seqNumSeq, ChangeCount_t filteredCount);

	//void IrrelevantChangeSet(SequenceNumber_t seqNum);

	/**
	 * @brief 更新丢失状态的数据发送请求为新的状态。
	 *
	 * @param [in] firstAvailableSeqNum 请求序列号。
	 * @param [in] changesRemoved  是否更改为删除状态标志。
	 */
	void LostChangesUpdate(SequenceNumber_t firstAvailableSeqNum, bool changesRemoved);

	/**
	 * @brief 获取丢失状态请求序列号集合。
	 *
	 * @return 丢失状态请求序列号集合。
	 */
	SeqNumberVec MissingChanges();

	/**
	 * @brief 更新所有发送请求中状态为UNKNOWN和序列号小于lastAvailableSeqNum的发送请求状态为丢失。
	 *
	 * @param [in] lastAvailableSeqNum 上一个可用请求序列号。
	 */
	void MissingChangesUpdate(SequenceNumber_t lastAvailableSeqNum);

	/**
	 * @brief 更新序号为seqNum的发送请求为已接收。
	 *
	 * @param [in] seqNum 已接收请求序列号。
	 */
	void ReceivedChangeSet(SequenceNumber_t seqNum);

	/**
	 * @brief 获取GUID。
	 *
	 * @return 丢失状态请求序列号集合。
	 */
	GUID_t GetGuid();

	/**
	 * @brief 添加可靠通信模式下的数据缓存。
	 *
	 * @param [in] cache   数据缓存。
	 * @param [in] notFrag 接收成功标志（默认：true）。
	 * 
	 * @return 添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	int32_t AddReliableDataCache(std::shared_ptr<CacheChange> cache, bool notFrag = true);

	/**
	 * @brief 添加可靠通信模式下的数据缓存。
	 *
	 * @param [in] seq   分片数据的序号。
	 * @param [in] data  数据内容。
	 * @param [in] len   数据长度。
	 * @param [in] data  分片数据在所有的数据分片数据序号列表中的编号。
	 *
	 * @return 添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t AddFragData(const SequenceNumber_t& seq, char* data, int32_t len, int32_t num);

	/**
	 * @brief 根据HB报文判断用户可靠通信模式下用户报文发送情况。
	 *
	 * @param [in] hbMsg	  Heartbeat子报文参数。
	 * @param [in] lostCount  数据丢失情况。
	 * @param [in] set        数据序列号。
	 *
	 * @return 添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t RecvHeartbeat(const HeartBeatSubMessage_t& hbMsg, int32_t& lostCount, SequenceNumberSet_t& set);

	/**
	 * @brief 根据HBFrag报文判断用户可靠通信模式下用户大包数据分片报文发送情况。
	 *
	 * @param [in] hbMsg	  HeartbeatFrag子报文参数。
	 * @param [in] set        数据分片序列号。
	 *
	 * @return 添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t RecvHeartbeatFrag(const HeartBeatFragSubMessage_t& hbMsg, FragMentNumberSet_t& set);

	/**
	 * @brief 获取已提交的缓存。
	 *
	 * @param [in] hbMsg	  HeartbeatFrag子报文参数。
	 * @param [in] set        数据分片序列号。
	 *
	 * @return 已提交的缓存句柄。
	 */
	std::shared_ptr<CacheChange> GetSubmitCache();

	/**
	 * @brief 判断该序号是否存在。
	 *
	 * @param [in] seq 报文缓存序号。
	 *
	 * @return 存在则返回true，否则返回RETCODE_ERROR。
	 */
	bool ExistCache(const SequenceNumber_t& seq);

	/**
	 * @brief 获取Ack报文序号列表。
	 *
	 * @param [out] set Ack报文序号列表。
	 */
	void GetAckSequenceSet(SequenceNumberSet_t& set);

	/**
	 * @brief 获取NAck报文序号列表。
	 *
	 * @param [out] set NAck报文序号列表。
	 */
	void GetNackSequenceSet(std::vector<NackFragSubMessage_t>& set);

	/**
	 * @brief 获取单播通信信息列表unicastLocatorList_。
	 *
	 * @return 单播通信信息列表unicastLocatorList_。
	 */
	LocatorSeq GetUnicastLocatorList();

	/**
	 * @brief 获取多播通信信息列表multicastLocatorList_。
	 *
	 * @return 多播通信信息列表multicastLocatorList_。
	 */
	LocatorSeq GetMulticastLocatorList();

	/**
	 * @brief 从所有发送请求中过滤出状态为NOT_AVAILABLE的请求。
	 *
	 * @return 状态为NOT_AVAILABLE的请求列表。
	 */
	SeqNumberVec LostChanges();

	/**
	 * @brief 判断数据是否提交。
	 * 
	 * @return 已提交返回true，未提交返回false。
	 */
	bool IsSubmitted(const SequenceNumber_t & seq);

private:
	GUID_t remoteWriterGuid_;
	GUID_t remoteGroupGuid_;
	LocatorSeq unicastLocatorList_;
	LocatorSeq multicastLocatorList_;

	int32_t dataMaxSizeSerialized_;
	//ChangeFromWriterSeq changesFromWriter_;
	EntityId_t remoteGroupEntityId_;

	/* 最新提交的数据 */
	SequenceNumber_t  submitSeq_;

	/* 插入最新的数据序号 */
	SequenceNumber_t  insertSeq_;

	std::map<SequenceNumber_t, ChangeFromWriter> changesFromWriters_;
	
	std::shared_ptr<CacheChange> changes_from_writer;

	std::atomic<int32_t> hbCount_;

	std::atomic<int32_t> hbFragCount_;

	std::atomic<int32_t> ackSendCount_;

	std::atomic<int32_t> nackSendCount_;
};

using WriterProxyMap = std::map<GUID_t, std::shared_ptr<WriterProxy>>;

TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_RTPS_ENDPOINT_WRITERPROXY_H
