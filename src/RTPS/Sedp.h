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
#include "AGIDDS_DLL.h"
#include "RTPS/DiscoveryData.h"
#include "RTPS/StatefulWriter.h"
#include "RTPS/StatefulReader.h"
#include "RTPS/Participant.h"
AGIDDS_NAMESPACE_BEGIN

class Sedp :public ReaderListener {

public:
	void AddSedpProxy(SPDPdiscoveredParticipantData & data);

	void RemoveSedpProxy(SPDPdiscoveredParticipantData & data);

	virtual void on_data_available();

private:

	Participant* participant_;

	StatefulWriter* publicationsWriter_;
	StatefulWriter* subscriptionsWriter_;
	StatefulReader* publicationsReader_;
	StatefulReader* subscriptionsReader_;
};

AGIDDS_NAMESPACE_END