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
#ifndef AGIDDS_RTPS_ENDPOINT_ENDPOINT_H
#define AGIDDS_RTPS_ENDPOINT_ENDPOINT_H

#include "rtps/endpoint/rtpsentity.h"

AGIDDS_NAMESPACE_BEGIN

class Participant;

class Endpoint : public RTPSEntity {
public:
	Endpoint() 
		: participant_(nullptr), 
		  topicKind_({ WITH_KEY }),
		  reliabilityLevel_(0),
		  endpointId_(ENTITYID_UNKNOWN)
	{
	}

	void SetParticipant(Participant* participant) {
		participant_ = participant;
	};
protected:
	LocatorSeq unicastLocatorList_;
	LocatorSeq multicastLocatorList_;
	ReliabilityKind_t reliabilityLevel_;
	TopicKind_t topicKind_;
	EntityId_t endpointId_;
	Participant* participant_;
};

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_ENDPOINT_ENDPOINT_H
