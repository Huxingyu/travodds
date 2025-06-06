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

#include "rtps/endpoint/writer.h"
#include "rtps/endpoint/readerproxy.h"

AGIDDS_NAMESPACE_BEGIN

class Timer;

class StatefulWriter : public Writer {
public:
	StatefulWriter(const DataWriterQos& qos, const GUID_t& guid, const TopicKind_t& kind);

	virtual ~StatefulWriter();

	virtual void MatchedReaderAdd(const DiscoveredReaderData &subData);

	void MatchedReaderAdd(std::shared_ptr<ReaderProxy> readerProxy);

	void MatchedReaderRemove(std::shared_ptr<ReaderProxy> readerProxy);

	std::shared_ptr<ReaderProxy> MatchedReaderLookup(GUID_t readerGuid);

	virtual bool WaitForAcknowledgments(const Duration_t &max_wait);

	virtual bool IsAckedByAll(SequenceNumber_t changeSeqNum);

	virtual ReturnCode_t DeliveryData(std::shared_ptr<CacheChange> change);

	virtual ReturnCode_t ReceiveAckNackMessage(const Receive_t& headerMsg, const AcknackSubMessage_t& ackMsg);

	virtual ReturnCode_t ReceiveNackFragMessage(const Receive_t& headerMsg, const NackFragSubMessage_t& nackMsg);

	ReturnCode_t DeliveryDataToReader(std::shared_ptr<CacheChange> change, const std::vector<std::shared_ptr<ReaderProxy>>& readerProxys);

	ReturnCode_t WriteHeartbeat();
	/* wangyi 发送 liveliness 心跳 */
	ReturnCode_t WriteHeartbeat(bool liveliness);

	ReturnCode_t WriteHeartbeat(std::shared_ptr<ReaderProxy> readerProxy, bool liveliness = false);

	ReturnCode_t WriteHeartFragbeat(std::shared_ptr<ReaderProxy> readerProxy);

	ReturnCode_t ReSendData(std::shared_ptr<ReaderProxy> readerProxy);

	/* 重传分片数据 */
	ReturnCode_t ReSendDataFrag(std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& FragSet);
private:
	ChangeForReader CreateChangeForReader(std::shared_ptr<CacheChange> change);

	/* 移除所有的已经确认的数据 */
	void RemoveAllAckedData();

	void SendGap(const SeqNumberVec& seqVec, std::shared_ptr<ReaderProxy>);


private:
	std::map<GUID_t, std::shared_ptr<ReaderProxy>> matchedReaders_;

	std::shared_ptr<Timer> timer_;

	std::mutex readerPorxyMutex_;
};

AGIDDS_NAMESPACE_END
