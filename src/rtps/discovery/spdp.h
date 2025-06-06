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
#ifndef AGIDDS_RTPS_DISCOVERY_SPDP_H
#define AGIDDS_RTPS_DISCOVERY_SPDP_H

#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/statelesswriter.h"
#include "rtps/endpoint/statelessreader.h"
#include "rtps/discovery/discoverydata.h"
#include "rtps/discovery/sedp.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/subscribe/datareaderimpl.h"

AGIDDS_NAMESPACE_BEGIN

class Discovery;

struct ParticipantStatus_t
{
	BuiltinTopicKey_t key;
	double leaseDuration;
	double recvTime;
};

class Spdp: public ReaderListener
{
public:
	Spdp(Participant* participant,Discovery* discovery);
	
	~Spdp();

	virtual void on_data_available(std::shared_ptr<CacheChange> change);


	void SetParticipantDataWriter(std::shared_ptr<DataWriterImpl> dataWriter);

	void SetParticipantDataReader(std::shared_ptr<DataReaderImpl> dataReader);

	void RemoveParticipant(GUID_t guid);

	void WriteSpdPdiscoveredParticipantData(SPDPdiscoveredParticipantData& data);

	void RemoveSpdPdiscoveredParticipantData(GUID_t& guid);

	void AddSpdpProxy(const SPDPdiscoveredParticipantData& data);

	void RemoveSpdpProxy(const GUID_t& guid);

	void CheckRemoteParticipantDuration();

	void GetOverdueParticipant(std::vector<BuiltinTopicKey_t>& keys);

	void GetDiscoveredParticipants(InstanceHandleSeq& participant_handles);

	int GetParticipantData(BuiltinTopicKey_t& key, SPDPdiscoveredParticipantData& data);

private:

	void TakeSPDPdiscoveredParticipantData(SPDPdiscoveredParticipantData& data, SerializedBuffer* buffer);

	void ReceiveSPDPdiscoveredParticipantData(const SPDPdiscoveredParticipantData& data);

private:
	Participant* participant_;
	std::shared_ptr<StatelessWriter> writer_ = nullptr;
	std::shared_ptr<StatelessReader> reader_ = nullptr;

	SpdpDataMap remoteParticipantData_;
	std::map<BuiltinTopicKey_t, ParticipantStatus_t> remoteParticipantStatus_;
	std::mutex mutex_;

	Discovery* discovery_;
	
	std::shared_ptr<MemoryPool> memoryPool_;

	
	Duration_t leaseDuration_;
	std::shared_ptr<Timer> timer_;
};

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_DISCOVERY_SPDP_H
