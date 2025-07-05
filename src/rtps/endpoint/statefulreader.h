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
#ifndef AGIDDS_RTPS_ENDPOINT_STATEFULREADER_H
#define AGIDDS_RTPS_ENDPOINT_STATEFULREADER_H

#include "rtps/endpoint/reader.h"
#include "rtps/endpoint/writerproxy.h"
#include "rtps/messages/cdrmessage_t.h"

AGIDDS_NAMESPACE_BEGIN

class StatefulReader : public Reader {
public:
	StatefulReader(const DataReaderQos& qos, const GUID_t& guid, const TopicKind_t& kind);

	virtual ~StatefulReader();

	void MatchedWriterAdd(std::shared_ptr<WriterProxy> writerProxy);

	void MatchedWriterRemove(std::shared_ptr<WriterProxy> writerProxy);

	std::shared_ptr<WriterProxy> MatchedWriterLookup(GUID_t writerGuid);

	/* wangyi */
	bool MatchedWriterIsMatched(GUID_t writerGuid);

	ReturnCode_t ReceiveDataMessage(const Receive_t&  headerMsg, const DataSubMessage_t& dataMsg, SerializedPayload data, uint32_t len);

	ReturnCode_t ReceiveDataFragMessage(const Receive_t&  headerMsg, const DataFragSubMessage_t& dataMsg, SerializedPayload data, uint32_t len);

	ReturnCode_t ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartMsg);

	ReturnCode_t ReceiveHeartFragMessage(const Receive_t&  headerInfo, const HeartBeatFragSubMessage_t& heartFragMsg);

	//int32_t ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartInfo);

	ReturnCode_t ReceiveGapMessage(const Receive_t&  headerInfo, const GapSubMessage_t& gapInfo);

	/* 提交数据 */
	ReturnCode_t Notify(std::shared_ptr<WriterProxy> proxy);

	ReturnCode_t AssertWriterLiveliness(const GUID_t& writer);

	ReturnCode_t SendAckMessage(std::shared_ptr<WriterProxy> writerProxy, AcknackSubMessage_t& ackMsg);

	ReturnCode_t SendNackFragMessage(std::shared_ptr<WriterProxy> writerProxy, NackFragSubMessage_t& nackMsg);

private:	
	void InitlastNewCacheTimer();
	void TimerExpired();
private:
	WriterProxyMap matchedWriters_;
};

AGIDDS_NAMESPACE_END

#endif
