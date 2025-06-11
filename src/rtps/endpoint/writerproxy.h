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
#ifndef AGIDDS_RTPS_ENDPOINT_WRITERPROXY_H
#define AGIDDS_RTPS_ENDPOINT_WRITERPROXY_H

#include "rtps/basictypertps.h"
#include "rtps/historycache/historycache.h"
#include "rtps/endpoint/changefromwriter.h"
#include "rtps/messages/cdrmessage_t.h"
#include <atomic>

AGIDDS_NAMESPACE_BEGIN

class WriterProxy {
public:
	WriterProxy(const GUID_t& writerGuid, const GUID_t& groupGuid, const LocatorSeq& unicastLocatorList, const LocatorSeq& multicastLocatorList);

	void Init(SequenceNumber_t first, SequenceNumber_t last);

	SequenceNumber_t AvailableChangesMax();

	void NonAvailableChangeSet(SeqNumberVec seqNumSeq, ChangeCount_t filteredCount);

	//void IrrelevantChangeSet(SequenceNumber_t seqNum);

	void LostChangesUpdate(SequenceNumber_t firstAvailableSeqNum, bool changesRemoved);

	SeqNumberVec MissingChanges();

	void MissingChangesUpdate(SequenceNumber_t lastAvailableSeqNum);

	void ReceivedChangeSet(SequenceNumber_t seqNum);

	GUID_t GetGuid();

	int32_t AddReliableDataCache(std::shared_ptr<CacheChange> cache, bool notFrag = true);

	/* seq:分片数据的序号 data:数据内容 len */
	ReturnCode_t AddFragData(const SequenceNumber_t& seq, char* data, int32_t len, int32_t num);

	ReturnCode_t RecvHeartbeat(const HeartBeatSubMessage_t& hbMsg, int32_t& lostCount, SequenceNumberSet_t& set);

	ReturnCode_t RecvHeartbeatFrag(const HeartBeatFragSubMessage_t& hbMsg, FragMentNumberSet_t& set);

	std::shared_ptr<CacheChange> GetSubmitCache();

	/* 判断序号是否存在  */
	bool ExistCache(const SequenceNumber_t& seq);

	void GetAckSequenceSet(SequenceNumberSet_t& set);

	void GetNackSequenceSet(std::vector<NackFragSubMessage_t>& set);


	LocatorSeq GetUnicastLocatorList();

	LocatorSeq GetMulticastLocatorList();

	SeqNumberVec LostChanges();

	/* wangyi 获取远端remoteWriterGuid_ */
	GUID_t GetRemoteWriterGuid();

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

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_ENDPOINT_WRITERPROXY_H
