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
#include "BuiltinTopicData.h"
#include "RTPS/BasicTypeRTPS.h"

AGIDDS_NAMESPACE_BEGIN

struct SPDPdiscoveredParticipantData
{
	ParticipantBuiltinTopicData ddsParticipantData;
	//participantProxy participantProxy;
	ProtocolVersion_t protocolVersion;
	GuidPrefix_t guidPrefix;
	VendorId_t vendorId;
	bool expectsInlineQos;
	LocatorSeq metatrafficUnicastLocatorList;
	LocatorSeq metatrafficMulticastLocatorList;
	LocatorSeq defaultUnicastLocatorList;
	LocatorSeq defaultMulticastLocatorList;
	BuiltinEndpointSet_t availableBuiltinEndpoints;
	Count_t manualLivelinessCount;

	Duration_t leaseDuration;
};

using SpdpDataMap = map<BuiltinTopicKey_t, SPDPdiscoveredParticipantData>;

struct DiscoveredWriterData
{
	PublicationBuiltinTopicData ddsPublicationData;
	GUID_t remoteWriterGuid;
	LocatorSeq unicastLocatorList;
	LocatorSeq multicastLocatorList;
};

struct DiscoveredReaderData
{
	SubscriptionBuiltinTopicData ddsSubscriptionData;
	GUID_t remoteReaderGuid_;
	LocatorSeq unicastLocatorList_;
	LocatorSeq multicastLocatorList_;
	bool expectsInlineQos_;
	ContentFilterProperty_t contentFilterProperty;
};

struct DiscoveredTopicData
{
	TopicBuiltinTopicData ddsTopicData;
};



AGIDDS_NAMESPACE_END