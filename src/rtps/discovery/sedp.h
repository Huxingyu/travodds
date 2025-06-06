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
#ifndef AGIDDS_RTPS_DISCOVERY_SEDP_H
#define AGIDDS_RTPS_DISCOVERY_SEDP_H

#include "agidds_dll.h"
#include "rtps/discovery/discoverydata.h"
#include "rtps/endpoint/statefulwriter.h"
#include "rtps/endpoint/statefulreader.h"
#include "rtps/endpoint/participant.h"

AGIDDS_NAMESPACE_BEGIN

class Spdp;
class Sedp : public ReaderListener {

public:
	Sedp(Participant* participant, Spdp* spdp);
	
	~Sedp();
	
	void SetPublicationEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader);

	void SetSubscriptionEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader);

	void AddSedpProxy(const SPDPdiscoveredParticipantData& data);

	void RemoveSedpProxy(GUID_t guid);

	virtual void on_data_available(std::shared_ptr<CacheChange> change);

	void WritePublicationData(DiscoveredWriterData& data);

	void WriteSubscriptionData(DiscoveredReaderData& data);

	void RemovePublicationData(GUID_t& guid);

	void RemoveSubscriptionData(GUID_t& guid);

private:
	void TakeDiscoveredWriterData(DiscoveredWriterData& data, SerializedBuffer* buffer);

	void ReceiveDiscoveredWriterData(DiscoveredWriterData& data);

	void TakeDiscoveredReaderData(DiscoveredReaderData& data, SerializedBuffer* buffer);

	void ReceiveDiscoveredReaderData(DiscoveredReaderData& data);

private:
	Spdp* spdp_;
	Participant* participant_;

	std::shared_ptr<StatefulWriter> publicationsWriter_  = nullptr;
	std::shared_ptr<StatefulWriter> subscriptionsWriter_ = nullptr;
	std::shared_ptr<StatefulReader> publicationsReader_  = nullptr;
	std::shared_ptr<StatefulReader> subscriptionsReader_ = nullptr;

	std::shared_ptr<MemoryPool> memoryPool_;
};

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_DISCOVERY_SEDP_H
