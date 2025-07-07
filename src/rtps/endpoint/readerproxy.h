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

#include "rtps/endpoint/changeforreader.h"
#include "rtps/historycache/historycache.h"
#include "rtps/messages/cdrmessage_t.h"

#include <atomic>

AGIDDS_NAMESPACE_BEGIN
class Writer;

/**
 * ReaderProxy类,保存特定Reader相对于远端DataWriter的状态。
 * （尽力通信模式）
 */
class ReaderProxy {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] readerGuid DataReader Guid。
	 * @param [in] groupGuid DataReader所属订阅者Guid。
	 * @param [in] unicastLocatorList 单播地址列表。
	 * @param [in] multicastLocatorList 组播地址列表。
	 * @param [in] active 是否已确认标志。
	 */
	ReaderProxy(const GUID_t& readerGuid, const GUID_t groupGuid, const LocatorSeq& unicastLocatorList, const LocatorSeq& multicastLocatorList,bool active);
	
	/**
	 * @brief 已确认请求集合修改。
	 *
	 * @param [in] committedSeqNum 已确认请求序列号。
	 */
	void AckedChangesSet(SequenceNumber_t committedSeqNum);

	/**
	 * @brief 设置请求报文的序号列表。
	 *
	 * @param [in] reqSeqNumSet 请求报文的序号列表。
	 */
	void RequestedChangesSet(SeqNumberVec reqSeqNumSet);

	/**
	 * @brief 获取下一个请求报文的序号。
	 *
	 * @return 下一个请求报文的序号。
	 */
	SequenceNumber_t NextRequestedChange();

	/**
	 * @brief 获取下一个未发送报文的序号。
	 *
	 * @return 下一个未发送报文的序号。
	 */
	SequenceNumber_t NextUnsentChange();

	/**
	 * @brief 判定是否存在未发送报文请求。
	 *
	 * @return 是则返回true，否则返回false。
	 */
	bool UnsentChanges();

	/**
	 * @brief 判定是否存在未确认的报文请求。
	 *
	 * @return 是则返回true，否则返回false。
	 */
	bool UnackedChanges();
	
	/**
	 * @brief 获取所有请求报文的序号。
	 *
	 * @return 所有请求报文的序号列表。
	 */
	SeqNumberVec RequestedChanges();

	/**
	 * @brief 获取远端DataReader Guid信息。
	 *
	 * @return 返回远端DataReader Guid配置信息。
	 */
	GUID_t GetGuid();

	/**
	 * @brief 判定报文请求是否已确认。
	 *
	 * @return 是则返回true，否则返回false。
	 */
	bool IsAcked(SequenceNumber_t number);

	/* 验证是否被全确认 */
	bool IsAllAcked(const SequenceNumber_t& maxNumber) const;

	/**
	 * @brief 设置要发送的报文缓冲区。
	 *
	 * @param [in] writerCache 请求报文缓冲区指针。
	 */
	void SetHistoryCache(std::shared_ptr<HistoryCache> writerCache);

	/**
	 * @brief 添加发送报文请求。
	 *
	 * @param [in] change 发送报文请求。
	 * 
	 * @return 添加成功则返回true，否则返回false。
	 */
	bool AddChange(ChangeForReader change);

	/**
	 * @brief 处理接收到的用户报文确认报文。
	 *
	 * @param [in] ackMsg 确认报文。
	 *
	 * @return 处理成功则返回true，否则返回false。
	 */
	int32_t RecvAcknack(const AcknackSubMessage_t& ackMsg);
	
	/**
	 * @brief 处理接收到的NackFrag报文。
	 *
	 * @param [in] ackMsg 分片数据确认报文。
	 *
	 * @return 处理成功则返回true，否则返回false。
	 */
	ReturnCode_t RecvNackFrag(const NackFragSubMessage_t& nackMsg);

	/**
	 * @brief 获取单播地址列表。
	 *
	 * @return 单播地址列表。
	 */
	LocatorSeq& GetUnicastLocatorList();

	/**
	 * @brief 获取组播地址列表。
	 *
	 * @return 组播地址列表。
	 */
	LocatorSeq& GetMulticastLocatorList();

	/**
	 * @brief 获取组播地址列表。
	 *
	 * @return 组播地址列表。
	 */
	bool isActive();

	/**
	 * @brief 对HEARTBEAT报文发送次数进行计数。
	 *
	 * @return 计数值。
	 */
	int32_t HeaertSendCountAdd();

	/**
	 * @brief 将指定的需要设置为Acked。
	 *
	 * @param [in] committedSeqNum 报文序列号。
	 */
	void AckedChangeSet(SequenceNumber_t committedSeqNum);
	/**
	 * @brief 初始化和匹配DataReader相关数据的初始序列号。
	 * relatedSeq之前的序列号与相关代理无关。
	 *
	 * @param [in] relatedSeq 报文序列号。
	 */
	void InitChangeForReader(SequenceNumber_t relatedSeq);

	/**
	 * @brief 获取已确认数据序列号。
	 *
	 * @return 报文序列号。
	 */
	SequenceNumber_t GetRelatedSequenceNumber();

	/**
	 * @brief 移除所有已经被确认的序号。
	 *
	 * @param [in] ackSeq 已确认报文序列号。
	 */
	void RemoveFragNackCounts(const SequenceNumber_t& ackSeq);

	/**
	 * @brief 判断数据是否已经通过NackFrag报文进行重传。
	 *
	 * @param [in] ackSeq 需确认报文序列号。
	 *
	 * @return 已重传则返回true，否则返回false。
	 */
	bool IsNackFrag(const SequenceNumber_t& ackSeq);

private:
	GUID_t remoteReaderGuid_;
	GUID_t remoteGroupGuid_;
	LocatorSeq unicastLocatorList_;
	LocatorSeq multicastLocatorList_;
	SequenceNumber_t highestSentChangeSN_;
	SeqNumberVec requestedChanges_;
	SeqNumberVec acknowledgeChanges_;
	bool expectsInlineQos_;

	// true: statefulreader false: statelessreader
	bool isActive_;

	std::shared_ptr<HistoryCache> writerCache_;

	std::atomic<int32_t> ackCount_;
	std::atomic<int32_t> hbSendCount_;

	/* 序号数据接收到对应的nackFrag报文  */
	std::map<SequenceNumber_t, int32_t>  nackCounts_;

	/* 被确认的最小序号，小于这个序号的数据都已经被确认  */
	SequenceNumber_t ackedSeq_;

	/* 给匹配DataReader相关联的序号, 如果相关HistoryCache中的数据存在需要 > relatedSeqnum, 需要发送一个Gap报文   */
	SequenceNumber_t relatedSeqnum_;

	///* 保存和匹配的Reader相关的每个CacheChange的状态 */
	//std::map<SequenceNumber_t, ChangeForReader>  changeForReaders_;
};

AGIDDS_NAMESPACE_END
