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
#ifndef AGIDDS_RTPS_ENDPOINT_STATELESSREADER_H
#define AGIDDS_RTPS_ENDPOINT_STATELESSREADER_H

#include "rtps/endpoint/reader.h"
#include "rtps/endpoint/writerproxy.h"

AGIDDS_NAMESPACE_BEGIN

//struct RemoteWriterData {
//	GUID_t writerGuid;
//	OwnershipQosPolicyKind ownershipKind;
//	int32_t ownership_strength;
//	LivelinessQosPolicyKind livelinessKind;
//	Duration_t lease_duration;
//
//	Duration_t recvTime;
//};

class StatelessReader : public Reader {
public:
	StatelessReader(const DataReaderQos& qos, const GUID_t& guid, const TopicKind_t& kind);

	virtual ~StatelessReader();

	ReturnCode_t ReceiveDataMessage(const Receive_t&  headerMsg, const DataSubMessage_t& dataMsg, SerializedPayload data, uint32_t len);

	ReturnCode_t ReceiveDataFragMessage(const Receive_t&  headerMsg, const DataFragSubMessage_t& dataMsg, SerializedPayload data, uint32_t len);

	ReturnCode_t ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartMsg);

	ReturnCode_t ReceiveHeartFragMessage(const Receive_t&  headerInfo, const HeartBeatFragSubMessage_t& heartFragMsg);

	//ReturnCode_t ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartInfo);

	ReturnCode_t ReceiveGapMessage(const Receive_t&  headerInfo, const GapSubMessage_t& gapInfo);

	void MatchedWriterAdd(std::shared_ptr<WriterProxy> writerData);

	bool MatchedWriterIsMatched(GUID_t writerGuid);

	void MatchedWriterRemove(GUID_t writerGuid);

	std::shared_ptr<WriterProxy> MatchedWriterLookup(GUID_t writerGuid);

	void AssertWriterLiveliness(const GUID_t & guid);

private:
	std::map<GUID_t, std::shared_ptr<WriterProxy>> matchWriters_;

	std::map<GUID_t, std::shared_ptr<CacheChange>> dataFragCaches_;
};

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_ENDPOINT_STATELESSREADER_H
