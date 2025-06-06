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

class ReaderProxy {
public:
	ReaderProxy(const GUID_t& readerGuid, const GUID_t groupGuid, const LocatorSeq& unicastLocatorList, const LocatorSeq& multicastLocatorList,bool active);

	void AckedChangesSet(SequenceNumber_t committedSeqNum);

	void RequestedChangesSet(SeqNumberVec reqSeqNumSet);

	SequenceNumber_t NextRequestedChange();

	SequenceNumber_t NextUnsentChange();

	bool UnsentChanges();

	bool UnackedChanges();

	SeqNumberVec RequestedChanges();

	GUID_t GetGuid();

	bool IsAcked(SequenceNumber_t number);

	void SetHistoryCache(std::shared_ptr<HistoryCache> writerCache);

	bool AddChange(ChangeForReader change);

	int32_t RecvAcknack(const AcknackSubMessage_t& ackMsg);
	
	ReturnCode_t RecvNackFrag(const NackFragSubMessage_t& nackMsg);


	LocatorSeq GetUnicastLocatorList();

	LocatorSeq GetMulticastLocatorList();

	bool isActive();

	int32_t HeaertSendCountAdd();

	/* 将指定的需要设置为Acked  */
	void AckedChangeSet(SequenceNumber_t committedSeqNum);
	/* 初始化和匹配DataReader相关数据的初始序列号,relatedSeq之前的序列号与相关代理无关 */
	void InitChangeForReader(SequenceNumber_t relatedSeq);

	SequenceNumber_t GetRelatedSequenceNumber();

	/* 移除所有已经被确认的序号 */
	void RemoveFragNackCounts(const SequenceNumber_t& ackSeq);

	/* 判断数据是否已经通过NackFrag报文进行重传  */
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
