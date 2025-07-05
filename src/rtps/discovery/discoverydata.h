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
#ifndef AGIDDS_RTPS_DISCOVERY_DISCOVERYDATA_H
#define AGIDDS_RTPS_DISCOVERY_DISCOVERYDATA_H

#include "type/builtintopicdata.h"
#include "rtps/basictypertps.h"

AGIDDS_NAMESPACE_BEGIN

struct SPDPdiscoveredParticipantData
{
	ParticipantBuiltinTopicData ddsParticipantData;
	/* rtps v2.5 Table 8.78 */
	DomainId_t domainId; /* 接收SPDPdiscoveredParticipantData的本地参与者的domainId */
	std::string domainTag;
	ProtocolVersion_t protocolVersion;
	//GuidPrefix_t guidPrefix;
	VendorId_t vendorId;
	bool expectsInlineQos;
	LocatorSeq metatrafficUnicastLocatorList;
	LocatorSeq metatrafficMulticastLocatorList;
	LocatorSeq defaultUnicastLocatorList;
	LocatorSeq defaultMulticastLocatorList;
	BuiltinEndpointSet_t availableBuiltinEndpoints;
	BuiltinEndpointQos_t builtinEndpointQos;
	Count_t manualLivelinessCount;

	Duration_t leaseDuration;
};

using SpdpDataMap = std::map<BuiltinTopicKey_t, SPDPdiscoveredParticipantData>;

struct DiscoveredWriterData
{
	PublicationBuiltinTopicData ddsPublicationData;
	GUID_t remoteWriterGuid;
	/* rtps v2.5 Figure 8.30 */
	GUID_t remoteGroupGuid;
	LocatorSeq unicastLocatorList;
	LocatorSeq multicastLocatorList;
	int32_t dataMaxSizeSerialized;
};

struct DiscoveredReaderData
{
	SubscriptionBuiltinTopicData ddsSubscriptionData;
	/* rtps v2.5 Figure 8.30 */
	GUID_t remoteReaderGuid;
	GUID_t remoteGroupGuid;
	LocatorSeq unicastLocatorList;
	LocatorSeq multicastLocatorList;
	ContentFilterProperty_t contentFilter;
};

struct DiscoveredTopicData
{
	TopicBuiltinTopicData ddsTopicData;
};

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_DISCOVERY_DISCOVERYDATA_H
