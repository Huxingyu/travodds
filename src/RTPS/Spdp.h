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
#include "RTPS/Participant.h"
#include "RTPS/StatelessWriter.h"
#include "RTPS/StatefulReader.h"
#include "RTPS/DiscoveryData.h"
#include "RTPS/Sedp.h"
#include "DDS/DataWriterImpl.h"
#include "DDS/DataReaderImpl.h"
AGIDDS_NAMESPACE_BEGIN

class Spdp: public ReaderListener
{
	Spdp(Participant* participant);

	void SetParticipantDataWriter(DataWriterImpl* dataWriter);

	void SetParticipantDataReqader(DataReaderImpl* dataReader);

	virtual void on_data_available();

private:
	void TakeSPDPdiscoveredParticipantData(SPDPdiscoveredParticipantData & data);

	void ReceiveSPDPdiscoveredParticipantData(SPDPdiscoveredParticipantData & data);

private:
	Participant* participant_;
	StatelessWriter* writer_;
	StatefulReader* reader_;

	SpdpDataMap remoteParticipantData_;

	Sedp* sedp_;
};

AGIDDS_NAMESPACE_END