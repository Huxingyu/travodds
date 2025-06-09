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
#ifndef AGIDDS_RTPS_ENDPOINT_WRITER_H
#define AGIDDS_RTPS_ENDPOINT_WRITER_H

#include "rtps/endpoint/endpoint.h"
#include "rtps/historycache/historycache.h"
#include "rtps/messages/cdrmessage_t.h"
AGIDDS_NAMESPACE_BEGIN

class DataWriterImpl;

class Writer : public Endpoint {
public:
	Writer(const DataWriterQos& qos, const TopicKind_t& kind);
	
	virtual ~Writer();

	std::shared_ptr<CacheChange> NewChange(ChangeKind_t kind, SerializedBuffer* data, InstanceHandle_t handle, const Time_t& timespace);

	ReturnCode_t AddNewCachce(std::shared_ptr<CacheChange> cacheChange);

	virtual bool WaitForAcknowledgments(const Duration_t& max_wait);

	virtual bool IsAckedByAll(SequenceNumber_t changeSeqNum);
	
	bool IsPushMode();

	std::shared_ptr<HistoryCache> GetWriteCache();

	bool DataFilter(std::shared_ptr<CacheChange> change);

	virtual ReturnCode_t DeliveryData(std::shared_ptr<CacheChange> change);

	void SetRelatedDataWriter(DataWriterImpl* datawriter);

	DataWriterImpl* GetRelatedDataWriter();

	virtual void MatchedReaderAdd(const DiscoveredReaderData &subData) = 0;

	virtual ReturnCode_t ReceiveAckNackMessage(const Receive_t&  headerMsg, const AcknackSubMessage_t& ackMsg) = 0;

	virtual ReturnCode_t ReceiveNackFragMessage(const Receive_t&  headerMsg, const NackFragSubMessage_t& nackMsg) = 0;

	/* wangyi 获取Qos */
	DataWriterQos& GetQos();

	ReturnCode_t AddBuiltinCache(std::shared_ptr<CacheChange> cacheChange);

	std::shared_ptr<HistoryCache> GetHistoryCache();

	void RemoveCacheByHandle(const InstanceHandle_t& handle);

protected:

	bool pushMode_ = true;
	Duration_t nackResponseDelay_;
	Duration_t heartBeatPeriod_;
	Duration_t nackSuppressionDuration_;
	SequenceNumber_t lastChangeSequenceNumber_;
	DataWriterQos qos_;

	std::shared_ptr<HistoryCache> writerCache_;
	std::map<InstanceHandle_t, bool> matchHandles_;
	DataWriterImpl* datawriter_ = nullptr;

};

AGIDDS_NAMESPACE_END

#endif
