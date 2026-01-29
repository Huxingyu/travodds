#include "rtps/messages/parameter.h"
#include "common/log/logger.h"
USING_TRAVODDS_NAMESPACE;
ParameterList::ParameterList(const octet* msg, const uint32_t& length)
	: msgEndian_(PL_DEFAULT_ENDIAN)
{
	if (length < 4) {
		TRAVODDS_LOG(LOG_ERROR, "length[%d] < 4", length);
		return;
	}
	msgEndian_ = static_cast<EndiannessKind>(*(msg + 1));
	LoadMessage(msg + 4, length - 4);
}

ParameterList::ParameterList(const SPDPdiscoveredParticipantData& data, const EndiannessKind& endian)
	: msgEndian_(endian)
{
	//PID_USER_DATA : UserDataQosPolicy
	if (!data.ddsParticipantData.user_data.value.empty()) {
		AddParameter(ParameterId::PID_USER_DATA, data.ddsParticipantData.user_data);
	}
	//PID_SYSTEM_INFO : SystemInfoQosPolicy
	if(!data.ddsParticipantData.sys_info.empty()) {
		AddParameter(ParameterId::PID_PROPERTY_LIST, data.ddsParticipantData.sys_info);
	}

#ifdef DDSSECURITY
	AddParameter(ParameterId::PID_IDENTITY_TOKEN, data.ddsParticipantData.identity_token);

	AddParameter(ParameterId::PID_PERMISSIONS_TOKEN, data.ddsParticipantData.permissions_token);

	AddParameter(ParameterId::PID_PARTICIPANT_SECURITY_INFO, data.ddsParticipantData.security_info);
#endif // DDSSECURITY

	//PID_DOMAIN_ID : DomainId_t
	if (data.domainId != DOMAIN_ID_UNKNOWN) {
		AddParameter(ParameterId::PID_DOMAIN_ID, data.domainId);
	}
	//ID_DOMAIN_TAG : string
	if (!data.domainTag.empty()) {
		AddParameter(ParameterId::PID_DOMAIN_TAG, data.domainTag);
	}
	//PID_PROTOCOL_VERSION : ProtocolVersion_t
	AddParameter(ParameterId::PID_PROTOCOL_VERSION, data.protocolVersion);
	//PID_VENDORID : VendorId_t
	if (data.vendorId[0] != VENDORID_UNKNOWN[0] || data.vendorId[1] != VENDORID_UNKNOWN[1]) {
		AddParameter(ParameterId::PID_VENDORID, data.vendorId);
	}
	//PID_DEFAULT_UNICAST_LOCATOR : LocatorSeq
	if (!data.defaultUnicastLocatorList.empty()) {
		AddParameter(ParameterId::PID_DEFAULT_UNICAST_LOCATOR, data.defaultUnicastLocatorList);
	}
	//PID_DEFAULT_MULTICAST_LOCATOR : LocatorSeq
	if (!data.defaultMulticastLocatorList.empty()) {
		AddParameter(ParameterId::PID_DEFAULT_MULTICAST_LOCATOR, data.defaultMulticastLocatorList);
	}
	//PID_METATRAFFIC_UNICAST_LOCATOR : LocatorSeq
	if (!data.metatrafficUnicastLocatorList.empty()) {
		AddParameter(ParameterId::PID_METATRAFFIC_UNICAST_LOCATOR, data.metatrafficUnicastLocatorList);
	}
	//PID_METATRAFFIC_UNICAST_LOCATOR : LocatorSeq
	if (!data.metatrafficMulticastLocatorList.empty()) {
		AddParameter(ParameterId::PID_METATRAFFIC_MULTICAST_LOCATOR, data.metatrafficMulticastLocatorList);
	}
	//PID_EXPECTS_INLINE_QOS : bool
	if (data.expectsInlineQos)
	{
		AddParameter(ParameterId::PID_EXPECTS_INLINE_QOS, true);
	}
	//PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT : Count_t
	AddParameter(ParameterId::PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT, data.manualLivelinessCount);
	//PID_BUILTIN_ENDPOINT_SET : BuiltinEndpointSet_t
	AddParameter(ParameterId::PID_BUILTIN_ENDPOINT_SET, data.availableBuiltinEndpoints);
	//PID_BUILTIN_ENDPOINT_QOS : BuiltinEndpointQos_t
	AddParameter(ParameterId::PID_BUILTIN_ENDPOINT_QOS, data.builtinEndpointQos);
	//PID_PARTICIPANT_LEASE_DURATION : Duration_t
	AddParameter(ParameterId::PID_PARTICIPANT_LEASE_DURATION, data.leaseDuration);
	//PID_PARTICIPANT_GUID : BuiltinTopicKey_t(GUID_t)
	AddParameter(ParameterId::PID_PARTICIPANT_GUID, data.ddsParticipantData.key);
}

ParameterList::ParameterList(const DiscoveredWriterData& data, const EndiannessKind& endian)
	: msgEndian_(endian)
{
	const PublicationBuiltinTopicData& pubData = data.ddsPublicationData;
	//PID_USER_DATA : UserDataQosPolicy
	if (!pubData.user_data.value.empty()) {
		AddParameter(ParameterId::PID_USER_DATA, pubData.user_data);
	}
	//PID_TOPIC_NAME : string
	if (!pubData.topic_name.empty()) {
		AddParameter(ParameterId::PID_TOPIC_NAME, pubData.topic_name);
	}
	//PID_TYPE_NAME : string
	if (!pubData.type_name.empty()) {
		AddParameter(ParameterId::PID_TYPE_NAME, pubData.type_name);
	}
	//PID_GROUP_DATA : GroupDataQosPolicy
	if (!pubData.group_data.value.empty()) {
		AddParameter(ParameterId::PID_GROUP_DATA, pubData.group_data);
	}
	//PID_TOPIC_DATA : TopicDataQosPolicy
	if (!pubData.topic_data.value.empty()) {
		AddParameter(ParameterId::PID_TOPIC_DATA, pubData.topic_data);
	}
	//PID_DURABILITY : DurabilityQosPolicy
	AddParameter(ParameterId::PID_DURABILITY, pubData.durability);
	//PID_DURABILITY_SERVICE : DurabilityServiceQosPolicy
	AddParameter(ParameterId::PID_DURABILITY_SERVICE, pubData.durability_service);
	//PID_DEADLINE : DeadlineQosPolicy
	AddParameter(ParameterId::PID_DEADLINE, pubData.deadline);
	//PID_LATENCY_BUDGET : LatencyBudgetQosPolicy
	AddParameter(ParameterId::PID_LATENCY_BUDGET, pubData.latency_budget);
	//PID_LIVELINESS : LivelinessQosPolicy
	AddParameter(ParameterId::PID_LIVELINESS, pubData.liveliness);
	//PID_RELIABILITY : ReliabilityQosPolicy
	AddParameter(ParameterId::PID_RELIABILITY, pubData.reliability);
	//PID_LIFESPAN : LifespanQosPolicy
	AddParameter(ParameterId::PID_LIFESPAN, pubData.lifespan);
	//PID_DESTINATION_ORDER : DestinationOrderQosPolicy
	AddParameter(ParameterId::PID_DESTINATION_ORDER, pubData.destination_order);
	//PID_OWNERSHIP : OwnershipQosPolicy
	AddParameter(ParameterId::PID_OWNERSHIP, pubData.ownership);
	//PID_OWNERSHIP_STRENGTH : OwnershipStrengthQosPolicy
	AddParameter(ParameterId::PID_OWNERSHIP_STRENGTH, pubData.ownership_strength);
	//PID_PRESENTATION : PresentationQosPolicy
	AddParameter(ParameterId::PID_PRESENTATION, pubData.presentation);
	//PID_PARTITION : PartitionQosPolicy
	if (!pubData.partition.name.empty()) {
		AddParameter(ParameterId::PID_PARTITION, pubData.partition);
	}
	//PID_UNICAST_LOCATOR : LocatorSeq
	if (!data.unicastLocatorList.empty()) {
		AddParameter(ParameterId::PID_UNICAST_LOCATOR, data.unicastLocatorList);
	}
	//PID_MULTICAST_LOCATOR  : LocatorSeq
	if (!data.multicastLocatorList.empty()) {
		AddParameter(ParameterId::PID_MULTICAST_LOCATOR, data.multicastLocatorList);
	}
	//PID_PARTICIPANT_GUID : BuiltinTopicKey_t
	AddParameter(ParameterId::PID_PARTICIPANT_GUID, pubData.participant_key);
	//PID_GROUP_GUID : GUID_t
	if (data.remoteGroupGuid != GUID_UNKNOWN) {
		AddParameter(ParameterId::PID_GROUP_GUID, data.remoteGroupGuid);
	}
	//PID_GROUP_ENTITY_ID : EntityId_t
	if (data.remoteGroupGuid.entityId != ENTITYID_UNKNOWN) {
		AddParameter(ParameterId::PID_GROUP_ENTITY_ID, data.remoteGroupGuid.entityId);
	}
	//PID_ENDPOINT_GUID : BuiltinTopicKey_t
	AddParameter(ParameterId::PID_ENDPOINT_GUID, pubData.key);
	//PID_DATA_MAX_SIZE_SERIALIZED : int32_t
	AddParameter(ParameterId::PID_TYPE_MAX_SIZE_SERIALIZED, data.dataMaxSizeSerialized);
#ifdef DDSSECURITY
	// PID_ENDPOINT_SECURITY_INFO : 安全相关
	AddParameter(ParameterId::PID_ENDPOINT_SECURITY_INFO, pubData.security_info);
#endif //DDSSECURITY
}

ParameterList::ParameterList(const DiscoveredReaderData& data, const EndiannessKind& endian)
	: msgEndian_(endian)
{
	const SubscriptionBuiltinTopicData& subData = data.ddsSubscriptionData;
	//PID_USER_DATA : UserDataQosPolicy
	if (!subData.user_data.value.empty()) {
		AddParameter(ParameterId::PID_USER_DATA, subData.user_data);
	}
	//PID_TOPIC_NAME : string
	if (!subData.topic_name.empty()) {
		AddParameter(ParameterId::PID_TOPIC_NAME, subData.topic_name);
	}
	//PID_TYPE_NAME : string
	if (!subData.type_name.empty()) {
		AddParameter(ParameterId::PID_TYPE_NAME, subData.type_name);
	}
	//PID_GROUP_DATA : GroupDataQosPolicy
	if (!subData.group_data.value.empty()) {
		AddParameter(ParameterId::PID_GROUP_DATA, subData.group_data);
	}
	//PID_TOPIC_DATA : TopicDataQosPolicy
	if (!subData.topic_data.value.empty()) {
		AddParameter(ParameterId::PID_TOPIC_DATA, subData.topic_data);
	}
	//PID_DURABILITY : DurabilityQosPolicy
	AddParameter(ParameterId::PID_DURABILITY, subData.durability);
	//PID_DEADLINE : DeadlineQosPolicy
	AddParameter(ParameterId::PID_DEADLINE, subData.deadline);
	//PID_LATENCY_BUDGET : LatencyBudgetQosPolicy
	AddParameter(ParameterId::PID_LATENCY_BUDGET, subData.latency_budget);
	//PID_LIVELINESS : LivelinessQosPolicy
	AddParameter(ParameterId::PID_LIVELINESS, subData.liveliness);
	//PID_RELIABILITY : ReliabilityQosPolicy
	AddParameter(ParameterId::PID_RELIABILITY, subData.reliability);
	//PID_DESTINATION_ORDER : DestinationOrderQosPolicy
	AddParameter(ParameterId::PID_DESTINATION_ORDER, subData.destination_order);
	//PID_OWNERSHIP : OwnershipQosPolicy
	AddParameter(ParameterId::PID_OWNERSHIP, subData.ownership);
	//PID_PRESENTATION : PresentationQosPolicy
	AddParameter(ParameterId::PID_PRESENTATION, subData.presentation);
	//PID_PARTITION : PartitionQosPolicy
	if (!subData.partition.name.empty()) {
		AddParameter(ParameterId::PID_PARTITION, subData.partition);
	}
	//PID_TIME_BASED_FILTER : TimeBasedFilterQosPolicy
	AddParameter(ParameterId::PID_TIME_BASED_FILTER, subData.time_based_filter);
	//PID_UNICAST_LOCATOR : LocatorSeq
	if (!data.unicastLocatorList.empty()) {
		AddParameter(ParameterId::PID_UNICAST_LOCATOR, data.unicastLocatorList);
	}
	//PID_MULTICAST_LOCATOR  : LocatorSeq
	if (!data.multicastLocatorList.empty()) {
		AddParameter(ParameterId::PID_MULTICAST_LOCATOR, data.multicastLocatorList);
	}
	//PID_PARTICIPANT_GUID : BuiltinTopicKey_t
	AddParameter(ParameterId::PID_PARTICIPANT_GUID, subData.participant_key);
	//PID_GROUP_GUID : GUID_t
	if (data.remoteGroupGuid != GUID_UNKNOWN) {
		AddParameter(ParameterId::PID_GROUP_GUID, data.remoteGroupGuid);
	}
	//PID_GROUP_ENTITY_ID : EntityId_t
	if (data.remoteGroupGuid.entityId != ENTITYID_UNKNOWN) {
		AddParameter(ParameterId::PID_GROUP_ENTITY_ID, data.remoteGroupGuid.entityId);
	}
	//PID_ENDPOINT_GUID : BuiltinTopicKey_t
	AddParameter(ParameterId::PID_ENDPOINT_GUID, subData.key);
	//PID_CONTENT_FILTER_PROPERTY : ContentFilterProperty_t
	/* wangyi fastdds 适配 添加过滤 */
	if (0 < data.contentFilter.relatedTopicName.size() && 0 < data.contentFilter.contentFilteredTopicName.size() && 0 < data.contentFilter.filterClassName.size() && 0 < data.contentFilter.filterExpression.size()) {
		AddParameter(ParameterId::PID_CONTENT_FILTER_PROPERTY, data.contentFilter);
	}
#ifdef DDSSECURITY
	// PID_ENDPOINT_SECURITY_INFO : 安全相关
	AddParameter(ParameterId::PID_ENDPOINT_SECURITY_INFO, subData.security_info);
#endif //DDSSECURITY
}

ParameterList::ParameterList(const DiscoveredTopicData& data, const EndiannessKind& endian)
	: msgEndian_(endian)
{
	const TopicBuiltinTopicData& topicData = data.ddsTopicData;
	//PID_TOPIC_NAME : string
	if (!topicData.name.empty()) {
		AddParameter(ParameterId::PID_TOPIC_NAME, topicData.name);
	}
	//PID_TYPE_NAME : string
	if (!topicData.type_name.empty()) {
		AddParameter(ParameterId::PID_TYPE_NAME, topicData.type_name);
	}
	//PID_TOPIC_DATA : TopicDataQosPolicy
	if (!topicData.topic_data.value.empty()) {
		AddParameter(ParameterId::PID_TOPIC_DATA, topicData.topic_data);
	}
	//PID_DURABILITY : DurabilityQosPolicy
	AddParameter(ParameterId::PID_DURABILITY, topicData.durability);
	//PID_DURABILITY_SERVICE : DurabilityServiceQosPolicy
	AddParameter(ParameterId::PID_DURABILITY_SERVICE, topicData.durability_service);
	//PID_DEADLINE : DeadlineQosPolicy
	AddParameter(ParameterId::PID_DEADLINE, topicData.deadline);
	//PID_LATENCY_BUDGET : LatencyBudgetQosPolicy
	AddParameter(ParameterId::PID_LATENCY_BUDGET, topicData.latency_budget);
	//PID_LIVELINESS : LivelinessQosPolicy
	AddParameter(ParameterId::PID_LIVELINESS, topicData.liveliness);
	//PID_RELIABILITY : ReliabilityQosPolicy
	AddParameter(ParameterId::PID_RELIABILITY, topicData.reliability);
	//PID_LIFESPAN : LifespanQosPolicy
	AddParameter(ParameterId::PID_LIFESPAN, topicData.lifespan);
	//PID_DESTINATION_ORDER : DestinationOrderQosPolicy
	AddParameter(ParameterId::PID_DESTINATION_ORDER, topicData.destination_order);
	//PID_HISTORY : HistoryQosPolicy
	AddParameter(ParameterId::PID_DESTINATION_ORDER, topicData.history);
	//PID_RESOURCE_LIMITS : ResourceLimitsQosPolicy
	AddParameter(ParameterId::PID_RESOURCE_LIMITS, topicData.resource_limits);
	//PID_OWNERSHIP : OwnershipQosPolicy
	AddParameter(ParameterId::PID_OWNERSHIP, topicData.ownership);
	//PID_ENDPOINT_GUID : BuiltinTopicKey_t
	AddParameter(ParameterId::PID_ENDPOINT_GUID, topicData.key);
}

void ParameterList::GetDiscoveredData(SPDPdiscoveredParticipantData& data)
{
	//PID_USER_DATA : UserDataQosPolicy
	GetParameter(ParameterId::PID_USER_DATA, data.ddsParticipantData.user_data);
	//PID_PROPERTY_LIST : PropertySeq
	GetParameter(ParameterId::PID_PROPERTY_LIST, data.ddsParticipantData.sys_info);
#ifdef DDSSECURITY
	GetParameter(ParameterId::PID_IDENTITY_TOKEN, data.ddsParticipantData.identity_token);
	GetParameter(ParameterId::PID_PERMISSIONS_TOKEN, data.ddsParticipantData.permissions_token);
	GetParameter(ParameterId::PID_PARTICIPANT_SECURITY_INFO, data.ddsParticipantData.security_info);
	//contains_security_data由域参与者是否带安全决定
	data.ddsParticipantData.contains_security_data = data.ddsParticipantData.security_info.participant_security_atributes != 0;
#endif // DDSSECURITY
	//PID_DOMAIN_ID : DomainId_t
	GetParameter(ParameterId::PID_DOMAIN_ID, data.domainId);
	//ID_DOMAIN_TAG : string
	GetParameter(ParameterId::PID_DOMAIN_TAG, data.domainTag);
	//PID_PROTOCOL_VERSION : ProtocolVersion_t
	GetParameter(ParameterId::PID_PROTOCOL_VERSION, data.protocolVersion);
	//PID_VENDORID : VendorId_t
	GetParameter(ParameterId::PID_VENDORID, data.vendorId);
	//PID_DEFAULT_UNICAST_LOCATOR : LocatorSeq
	GetParameter(ParameterId::PID_DEFAULT_UNICAST_LOCATOR, data.defaultUnicastLocatorList);
	//PID_DEFAULT_MULTICAST_LOCATOR : LocatorSeq
	GetParameter(ParameterId::PID_DEFAULT_MULTICAST_LOCATOR, data.defaultMulticastLocatorList);
	//PID_METATRAFFIC_UNICAST_LOCATOR : LocatorSeq
	GetParameter(ParameterId::PID_METATRAFFIC_UNICAST_LOCATOR, data.metatrafficUnicastLocatorList);
	//PID_METATRAFFIC_UNICAST_LOCATOR : LocatorSeq
	GetParameter(ParameterId::PID_METATRAFFIC_MULTICAST_LOCATOR, data.metatrafficMulticastLocatorList);
	//PID_EXPECTS_INLINE_QOS : bool
	GetParameter(ParameterId::PID_EXPECTS_INLINE_QOS, data.expectsInlineQos);
	//PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT : Count_t
	GetParameter(ParameterId::PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT, data.manualLivelinessCount);
	//PID_BUILTIN_ENDPOINT_SET : BuiltinEndpointSet_t
	GetParameter(ParameterId::PID_BUILTIN_ENDPOINT_SET, data.availableBuiltinEndpoints);
	//PID_BUILTIN_ENDPOINT_QOS : BuiltinEndpointQos_t
	GetParameter(ParameterId::PID_BUILTIN_ENDPOINT_QOS, data.builtinEndpointQos);
	//PID_PARTICIPANT_LEASE_DURATION : Duration_t
	GetParameter(ParameterId::PID_PARTICIPANT_LEASE_DURATION, data.leaseDuration);
	//PID_PARTICIPANT_GUID : BuiltinTopicKey_t(GUID_t)
	GetParameter(ParameterId::PID_PARTICIPANT_GUID, data.ddsParticipantData.key);
}

void ParameterList::GetDiscoveredData(DiscoveredWriterData& data)
{
	PublicationBuiltinTopicData& pubData = data.ddsPublicationData;
	//PID_USER_DATA : UserDataQosPolicy
	GetParameter(ParameterId::PID_USER_DATA, pubData.user_data);
	//PID_TOPIC_NAME : string
	GetParameter(ParameterId::PID_TOPIC_NAME, pubData.topic_name);
	//PID_TYPE_NAME : string
	GetParameter(ParameterId::PID_TYPE_NAME, pubData.type_name);
	//PID_GROUP_DATA : GroupDataQosPolicy
	GetParameter(ParameterId::PID_GROUP_DATA, pubData.group_data);
	//PID_TOPIC_DATA : TopicDataQosPolicy
	GetParameter(ParameterId::PID_TOPIC_DATA, pubData.topic_data);
	//PID_DURABILITY : DurabilityQosPolicy
	GetParameter(ParameterId::PID_DURABILITY, pubData.durability);
	//PID_DURABILITY_SERVICE : DurabilityServiceQosPolicy
	GetParameter(ParameterId::PID_DURABILITY_SERVICE, pubData.durability_service);
	//PID_DEADLINE : DeadlineQosPolicy
	GetParameter(ParameterId::PID_DEADLINE, pubData.deadline);
	//PID_LATENCY_BUDGET : LatencyBudgetQosPolicy
	GetParameter(ParameterId::PID_LATENCY_BUDGET, pubData.latency_budget);
	//PID_LIVELINESS : LivelinessQosPolicy
	GetParameter(ParameterId::PID_LIVELINESS, pubData.liveliness);
	//PID_RELIABILITY : ReliabilityQosPolicy
	GetParameter(ParameterId::PID_RELIABILITY, pubData.reliability);
	//PID_LIFESPAN : LifespanQosPolicy
	GetParameter(ParameterId::PID_LIFESPAN, pubData.lifespan);
	//PID_DESTINATION_ORDER : DestinationOrderQosPolicy
	GetParameter(ParameterId::PID_DESTINATION_ORDER, pubData.destination_order);
	//PID_OWNERSHIP : OwnershipQosPolicy
	GetParameter(ParameterId::PID_OWNERSHIP, pubData.ownership);
	//PID_OWNERSHIP_STRENGTH : OwnershipStrengthQosPolicy
	GetParameter(ParameterId::PID_OWNERSHIP_STRENGTH, pubData.ownership_strength);
	//PID_PRESENTATION : PresentationQosPolicy
	GetParameter(ParameterId::PID_PRESENTATION, pubData.presentation);
	//PID_PARTITION : PartitionQosPolicy
	GetParameter(ParameterId::PID_PARTITION, pubData.partition);
	//PID_UNICAST_LOCATOR : LocatorSeq
	GetParameter(ParameterId::PID_UNICAST_LOCATOR, data.unicastLocatorList);
	//PID_MULTICAST_LOCATOR  : LocatorSeq
	GetParameter(ParameterId::PID_MULTICAST_LOCATOR, data.multicastLocatorList);
	//PID_PARTICIPANT_GUID : BuiltinTopicKey_t
	GetParameter(ParameterId::PID_PARTICIPANT_GUID, pubData.participant_key);
	//PID_GROUP_GUID : GUID_t
	GetParameter(ParameterId::PID_GROUP_GUID, data.remoteGroupGuid);
	//PID_GROUP_ENTITY_ID : EntityId_t
	GetParameter(ParameterId::PID_GROUP_ENTITY_ID, data.remoteGroupGuid.entityId);
	//PID_ENDPOINT_GUID : BuiltinTopicKey_t
	GetParameter(ParameterId::PID_ENDPOINT_GUID, pubData.key);
	//PID_DATA_MAX_SIZE_SERIALIZED : int32_t
	GetParameter(ParameterId::PID_TYPE_MAX_SIZE_SERIALIZED, data.dataMaxSizeSerialized);
#ifdef DDSSECURITY
	GetParameter(ParameterId::PID_ENDPOINT_SECURITY_INFO, pubData.security_info);
#endif //DDSSECURITY
}

void ParameterList::GetDiscoveredData(DiscoveredReaderData& data)
{
	SubscriptionBuiltinTopicData& subData = data.ddsSubscriptionData;
	//PID_USER_DATA : UserDataQosPolicy
	GetParameter(ParameterId::PID_USER_DATA, subData.user_data);
	//PID_TOPIC_NAME : string
	GetParameter(ParameterId::PID_TOPIC_NAME, subData.topic_name);
	//PID_TYPE_NAME : string
	GetParameter(ParameterId::PID_TYPE_NAME, subData.type_name);
	//PID_GROUP_DATA : GroupDataQosPolicy
	GetParameter(ParameterId::PID_GROUP_DATA, subData.group_data);
	//PID_TOPIC_DATA : TopicDataQosPolicy
	GetParameter(ParameterId::PID_TOPIC_DATA, subData.topic_data);
	//PID_DURABILITY : DurabilityQosPolicy
	GetParameter(ParameterId::PID_DURABILITY, subData.durability);
	//PID_DEADLINE : DeadlineQosPolicy
	GetParameter(ParameterId::PID_DEADLINE, subData.deadline);
	//PID_LATENCY_BUDGET : LatencyBudgetQosPolicy
	GetParameter(ParameterId::PID_LATENCY_BUDGET, subData.latency_budget);
	//PID_LIVELINESS : LivelinessQosPolicy
	GetParameter(ParameterId::PID_LIVELINESS, subData.liveliness);
	//PID_RELIABILITY : ReliabilityQosPolicy
	GetParameter(ParameterId::PID_RELIABILITY, subData.reliability);
	//PID_DESTINATION_ORDER : DestinationOrderQosPolicy
	GetParameter(ParameterId::PID_DESTINATION_ORDER, subData.destination_order);
	//PID_OWNERSHIP : OwnershipQosPolicy
	GetParameter(ParameterId::PID_OWNERSHIP, subData.ownership);
	//PID_PRESENTATION : PresentationQosPolicy
	GetParameter(ParameterId::PID_PRESENTATION, subData.presentation);
	//PID_PARTITION : PartitionQosPolicy
	GetParameter(ParameterId::PID_PARTITION, subData.partition);
	//PID_TIME_BASED_FILTER : TimeBasedFilterQosPolicy
	GetParameter(ParameterId::PID_TIME_BASED_FILTER, subData.time_based_filter);
	//PID_UNICAST_LOCATOR : LocatorSeq
	GetParameter(ParameterId::PID_UNICAST_LOCATOR, data.unicastLocatorList);
	//PID_MULTICAST_LOCATOR  : LocatorSeq
	GetParameter(ParameterId::PID_MULTICAST_LOCATOR, data.multicastLocatorList);
	//PID_PARTICIPANT_GUID : BuiltinTopicKey_t
	GetParameter(ParameterId::PID_PARTICIPANT_GUID, subData.participant_key);
	//PID_GROUP_GUID : GUID_t
	GetParameter(ParameterId::PID_GROUP_GUID, data.remoteGroupGuid);
	//PID_GROUP_ENTITY_ID : EntityId_t
	GetParameter(ParameterId::PID_GROUP_ENTITY_ID, data.remoteGroupGuid.entityId);
	//PID_ENDPOINT_GUID : BuiltinTopicKey_t
	GetParameter(ParameterId::PID_ENDPOINT_GUID, subData.key);
	//PID_CONTENT_FILTER_PROPERTY : ContentFilterProperty_t
	GetParameter(ParameterId::PID_CONTENT_FILTER_PROPERTY, data.contentFilter);
#ifdef DDSSECURITY
	GetParameter(ParameterId::PID_ENDPOINT_SECURITY_INFO, subData.security_info);
#endif //DDSSECURITY
}

#ifdef DDSSECURITY
void ParameterList::GetDiscoveredSecureData(DiscoveredWriterSecureData& data)
{
	PublicationBuiltinTopicSecureData& pubData = data.ddsPublicationData;
	//PID_USER_DATA : UserDataQosPolicy
	GetParameter(ParameterId::PID_USER_DATA, pubData.user_data);
	//PID_TOPIC_NAME : string
	GetParameter(ParameterId::PID_TOPIC_NAME, pubData.topic_name);
	//PID_TYPE_NAME : string
	GetParameter(ParameterId::PID_TYPE_NAME, pubData.type_name);
	//PID_GROUP_DATA : GroupDataQosPolicy
	GetParameter(ParameterId::PID_GROUP_DATA, pubData.group_data);
	//PID_TOPIC_DATA : TopicDataQosPolicy
	GetParameter(ParameterId::PID_TOPIC_DATA, pubData.topic_data);
	//PID_DURABILITY : DurabilityQosPolicy
	GetParameter(ParameterId::PID_DURABILITY, pubData.durability);
	//PID_DURABILITY_SERVICE : DurabilityServiceQosPolicy
	GetParameter(ParameterId::PID_DURABILITY_SERVICE, pubData.durability_service);
	//PID_DEADLINE : DeadlineQosPolicy
	GetParameter(ParameterId::PID_DEADLINE, pubData.deadline);
	//PID_LATENCY_BUDGET : LatencyBudgetQosPolicy
	GetParameter(ParameterId::PID_LATENCY_BUDGET, pubData.latency_budget);
	//PID_LIVELINESS : LivelinessQosPolicy
	GetParameter(ParameterId::PID_LIVELINESS, pubData.liveliness);
	//PID_RELIABILITY : ReliabilityQosPolicy
	GetParameter(ParameterId::PID_RELIABILITY, pubData.reliability);
	//PID_LIFESPAN : LifespanQosPolicy
	GetParameter(ParameterId::PID_LIFESPAN, pubData.lifespan);
	//PID_DESTINATION_ORDER : DestinationOrderQosPolicy
	GetParameter(ParameterId::PID_DESTINATION_ORDER, pubData.destination_order);
	//PID_OWNERSHIP : OwnershipQosPolicy
	GetParameter(ParameterId::PID_OWNERSHIP, pubData.ownership);
	//PID_OWNERSHIP_STRENGTH : OwnershipStrengthQosPolicy
	GetParameter(ParameterId::PID_OWNERSHIP_STRENGTH, pubData.ownership_strength);
	//PID_PRESENTATION : PresentationQosPolicy
	GetParameter(ParameterId::PID_PRESENTATION, pubData.presentation);
	//PID_PARTITION : PartitionQosPolicy
	GetParameter(ParameterId::PID_PARTITION, pubData.partition);

	//新增解析出结构体
	GetParameter(ParameterId::PID_DATA_TAGS, pubData.data_tags);
	GetParameter(ParameterId::PID_ENDPOINT_SECURITY_INFO, pubData.security_info);

	//PID_UNICAST_LOCATOR : LocatorSeq
	GetParameter(ParameterId::PID_UNICAST_LOCATOR, data.unicastLocatorList);
	//PID_MULTICAST_LOCATOR  : LocatorSeq
	GetParameter(ParameterId::PID_MULTICAST_LOCATOR, data.multicastLocatorList);
	//PID_PARTICIPANT_GUID : BuiltinTopicKey_t
	GetParameter(ParameterId::PID_PARTICIPANT_GUID, pubData.participant_key);
	//PID_GROUP_GUID : GUID_t
	GetParameter(ParameterId::PID_GROUP_GUID, data.remoteGroupGuid);
	//PID_GROUP_ENTITY_ID : EntityId_t
	GetParameter(ParameterId::PID_GROUP_ENTITY_ID, data.remoteGroupGuid.entityId);
	//PID_ENDPOINT_GUID : BuiltinTopicKey_t
	GetParameter(ParameterId::PID_ENDPOINT_GUID, pubData.key);
	//PID_DATA_MAX_SIZE_SERIALIZED : int32_t
	GetParameter(ParameterId::PID_TYPE_MAX_SIZE_SERIALIZED, data.dataMaxSizeSerialized);
}

void ParameterList::GetDiscoveredSecureData(DiscoveredReaderSecureData& data)
{
	SubscriptionBuiltinTopicSecureData& subData = data.ddsSubscriptionData;
	//PID_USER_DATA : UserDataQosPolicy
	GetParameter(ParameterId::PID_USER_DATA, subData.user_data);
	//PID_TOPIC_NAME : string
	GetParameter(ParameterId::PID_TOPIC_NAME, subData.topic_name);
	//PID_TYPE_NAME : string
	GetParameter(ParameterId::PID_TYPE_NAME, subData.type_name);
	//PID_GROUP_DATA : GroupDataQosPolicy
	GetParameter(ParameterId::PID_GROUP_DATA, subData.group_data);
	//PID_TOPIC_DATA : TopicDataQosPolicy
	GetParameter(ParameterId::PID_TOPIC_DATA, subData.topic_data);
	//PID_DURABILITY : DurabilityQosPolicy
	GetParameter(ParameterId::PID_DURABILITY, subData.durability);
	//PID_DEADLINE : DeadlineQosPolicy
	GetParameter(ParameterId::PID_DEADLINE, subData.deadline);
	//PID_LATENCY_BUDGET : LatencyBudgetQosPolicy
	GetParameter(ParameterId::PID_LATENCY_BUDGET, subData.latency_budget);
	//PID_LIVELINESS : LivelinessQosPolicy
	GetParameter(ParameterId::PID_LIVELINESS, subData.liveliness);
	//PID_RELIABILITY : ReliabilityQosPolicy
	GetParameter(ParameterId::PID_RELIABILITY, subData.reliability);
	//PID_DESTINATION_ORDER : DestinationOrderQosPolicy
	GetParameter(ParameterId::PID_DESTINATION_ORDER, subData.destination_order);
	//PID_OWNERSHIP : OwnershipQosPolicy
	GetParameter(ParameterId::PID_OWNERSHIP, subData.ownership);
	//PID_PRESENTATION : PresentationQosPolicy
	GetParameter(ParameterId::PID_PRESENTATION, subData.presentation);
	//PID_PARTITION : PartitionQosPolicy
	GetParameter(ParameterId::PID_PARTITION, subData.partition);

	GetParameter(ParameterId::PID_DATA_TAGS, subData.data_tags);
	GetParameter(ParameterId::PID_ENDPOINT_SECURITY_INFO, subData.security_info);

	//PID_TIME_BASED_FILTER : TimeBasedFilterQosPolicy
	GetParameter(ParameterId::PID_TIME_BASED_FILTER, subData.time_based_filter);
	//PID_UNICAST_LOCATOR : LocatorSeq
	GetParameter(ParameterId::PID_UNICAST_LOCATOR, data.unicastLocatorList);
	//PID_MULTICAST_LOCATOR  : LocatorSeq
	GetParameter(ParameterId::PID_MULTICAST_LOCATOR, data.multicastLocatorList);
	//PID_PARTICIPANT_GUID : BuiltinTopicKey_t
	GetParameter(ParameterId::PID_PARTICIPANT_GUID, subData.participant_key);
	//PID_GROUP_GUID : GUID_t
	GetParameter(ParameterId::PID_GROUP_GUID, data.remoteGroupGuid);
	//PID_GROUP_ENTITY_ID : EntityId_t
	GetParameter(ParameterId::PID_GROUP_ENTITY_ID, data.remoteGroupGuid.entityId);
	//PID_ENDPOINT_GUID : BuiltinTopicKey_t
	GetParameter(ParameterId::PID_ENDPOINT_GUID, subData.key);
	//PID_CONTENT_FILTER_PROPERTY : ContentFilterProperty_t
	GetParameter(ParameterId::PID_CONTENT_FILTER_PROPERTY, data.contentFilter);
}
#endif // DDSSECURITY

void ParameterList::GetDiscoveredData(DiscoveredTopicData& data)
{
	TopicBuiltinTopicData& topicData = data.ddsTopicData;
	//PID_TOPIC_NAME : string
	GetParameter(ParameterId::PID_TOPIC_NAME, topicData.name);
	//PID_TYPE_NAME : string
	GetParameter(ParameterId::PID_TYPE_NAME, topicData.type_name);
	//PID_TOPIC_DATA : TopicDataQosPolicy
	GetParameter(ParameterId::PID_TOPIC_DATA, topicData.topic_data);
	//PID_DURABILITY : DurabilityQosPolicy
	GetParameter(ParameterId::PID_DURABILITY, topicData.durability);
	//PID_DURABILITY_SERVICE : DurabilityServiceQosPolicy
	GetParameter(ParameterId::PID_DURABILITY_SERVICE, topicData.durability_service);
	//PID_DEADLINE : DeadlineQosPolicy
	GetParameter(ParameterId::PID_DEADLINE, topicData.deadline);
	//PID_LATENCY_BUDGET : LatencyBudgetQosPolicy
	GetParameter(ParameterId::PID_LATENCY_BUDGET, topicData.latency_budget);
	//PID_LIVELINESS : LivelinessQosPolicy
	GetParameter(ParameterId::PID_LIVELINESS, topicData.liveliness);
	//PID_RELIABILITY : ReliabilityQosPolicy
	GetParameter(ParameterId::PID_RELIABILITY, topicData.reliability);
	//PID_LIFESPAN : LifespanQosPolicy
	GetParameter(ParameterId::PID_LIFESPAN, topicData.lifespan);
	//PID_DESTINATION_ORDER : DestinationOrderQosPolicy
	GetParameter(ParameterId::PID_DESTINATION_ORDER, topicData.destination_order);
	//PID_HISTORY : HistoryQosPolicy
	GetParameter(ParameterId::PID_DESTINATION_ORDER, topicData.history);
	//PID_RESOURCE_LIMITS : ResourceLimitsQosPolicy
	GetParameter(ParameterId::PID_RESOURCE_LIMITS, topicData.resource_limits);
	//PID_OWNERSHIP : OwnershipQosPolicy
	GetParameter(ParameterId::PID_OWNERSHIP, topicData.ownership);
	//PID_ENDPOINT_GUID : BuiltinTopicKey_t
	GetParameter(ParameterId::PID_ENDPOINT_GUID, topicData.key);
}

uint32_t ParameterList::GetMessageSize()
{
	uint32_t size = 4;
	for (auto& param : parameterList_) {
		size += 4 + param.second->MessageSize();
	}
	return size + 4;
}
bool ParameterList::WriterToMessage(octet* msg, uint16_t& allowLength)
{
	if (allowLength < GetMessageSize()) {
		return false;
	}
	msg[0] = 0;
	msg[1] = static_cast<octet>(msgEndian_);
	msg[2] = 0;
	msg[3] = 0;

	uint32_t offset = 4;
	for (auto& param : parameterList_) {
		uint16_t c = allowLength;
		if (!param.second->AddToMessage(msg + offset, allowLength)) {
			return false;
		}
		offset += c - allowLength;
	}
	//PID_SENTINEL
	unsigned char sentinel[2] = { 0x0,0x1 };
	if (msgEndian_ == EndiannessKind::PL_CDR_BE) {
		(msg + offset)[0] = sentinel[0];
		(msg + offset)[1] = sentinel[1];
	} else {
		(msg + offset)[0] = sentinel[1];
		(msg + offset)[1] = sentinel[0];
	}
	(msg + offset)[2] = 0;
	(msg + offset)[3] = 0;
	return true;
}

void ParameterList::LoadMessage(const octet* msg, const uint32_t& length)
{
	uint32_t offset = 0;
	bool sentinel = true;
	while (offset < length && sentinel) {
		ParameterId pid; 
		uint16_t len;
		ReadUInt16(msg + offset, (uint16_t&)pid);
		ReadUInt16(msg + offset + 2, len);
		
		offset += 4;
		switch (pid) {
		case ParameterId::PID_PAD: {
			break;
		}
		case ParameterId::PID_SENTINEL: {
			sentinel = false;
			break;
		}
		case ParameterId::PID_USER_DATA: {
			UserDataQosPolicy data;
			uint32_t size = 0;
			ReadUInt32(msg + offset, size);
			if (size != 0) {
				data.value.resize(size);
				std::copy(msg + offset + 4, msg + offset + 4 + size, data.value.begin());
			}
			AddParameter(ParameterId::PID_USER_DATA, data);
			break;
		}
		case ParameterId::PID_TOPIC_NAME: {
			std::string name;
			ReaderString(msg + offset, name);
			AddParameter(ParameterId::PID_TOPIC_NAME, name);
			break;
		}
		case ParameterId::PID_TYPE_NAME: {
			std::string name;
			ReaderString(msg + offset, name);
			AddParameter(ParameterId::PID_TYPE_NAME, name);
			break;
		}
		case ParameterId::PID_GROUP_DATA: {
			GroupDataQosPolicy data;
			uint32_t size = 0;
			ReadUInt32(msg + offset, size);
			if (size != 0) {
				data.value.resize(size);
				std::copy(msg + offset + 4, msg + offset + 4 + size, data.value.begin());
			}
			AddParameter(ParameterId::PID_GROUP_DATA, data);
			break;
		}
		case ParameterId::PID_TOPIC_DATA: {
			TopicDataQosPolicy data;
			uint32_t size = 0;
			ReadUInt32(msg + offset, size);
			if (size != 0) {
				data.value.resize(size);
				std::copy(msg + offset + 4, msg + offset + 4 + size, data.value.begin());
			}
			AddParameter(ParameterId::PID_TOPIC_DATA, data);
			break;
		}
		case ParameterId::PID_DURABILITY: {
			DurabilityQosPolicy data;
			data.kind = static_cast<DurabilityQosPolicyKind>(*(msg + offset));
			AddParameter(ParameterId::PID_DURABILITY, data);
			break;
		}
		case ParameterId::PID_DURABILITY_SERVICE: {
			DurabilityServiceQosPolicy data;
			ReadInt32(msg + offset, data.service_cleanup_delay.sec);
			ReadUInt32(msg + offset + 4, data.service_cleanup_delay.nanosec);
			data.history_kind = static_cast<HistoryQosPolicyKind>(*(msg + offset + 8));
			ReadInt32(msg + offset + 12, data.history_depth);
			ReadInt32(msg + offset + 16, data.max_samples);
			ReadInt32(msg + offset + 20, data.max_instances);
			ReadInt32(msg + offset + 24, data.max_samples_per_instance);
			AddParameter(ParameterId::PID_DURABILITY_SERVICE, data);
			break;
		}
		case ParameterId::PID_DEADLINE: {
			DeadlineQosPolicy data;
			ReadInt32(msg + offset, data.period.sec);
			ReadUInt32(msg + offset + 4, data.period.nanosec);
			AddParameter(ParameterId::PID_DEADLINE, data);
			break;
		}
		case ParameterId::PID_LATENCY_BUDGET: {
			LatencyBudgetQosPolicy data;
			ReadInt32(msg + offset, data.duration.sec);
			ReadUInt32(msg + offset + 4, data.duration.nanosec);
			AddParameter(ParameterId::PID_LATENCY_BUDGET, data);
			break;
		}
		case ParameterId::PID_LIVELINESS: {
			LivelinessQosPolicy data;
			data.kind = static_cast<LivelinessQosPolicyKind>(*(msg + offset));
			ReadInt32(msg + offset + 4, data.lease_duration.sec);
			ReadUInt32(msg + offset + 8, data.lease_duration.nanosec);
			AddParameter(ParameterId::PID_LIVELINESS, data);
			break;
		}
		case ParameterId::PID_RELIABILITY: {
			ReliabilityQosPolicy data;
			data.kind = static_cast<ReliabilityQosPolicyKind>(*(msg + offset));
			ReadInt32(msg + offset + 4, data.max_blocking_time.sec);
			ReadUInt32(msg + offset + 8, data.max_blocking_time.nanosec);
			AddParameter(ParameterId::PID_RELIABILITY, data);
			break;
		}
		case ParameterId::PID_LIFESPAN: {
			LifespanQosPolicy data;
			ReadInt32(msg + offset, data.duration.sec);
			ReadUInt32(msg + offset + 4, data.duration.nanosec);
			AddParameter(ParameterId::PID_LIFESPAN, data);
			break;
		}
		case ParameterId::PID_DESTINATION_ORDER: {
			DestinationOrderQosPolicy data;
			data.kind = static_cast<DestinationOrderQosPolicyKind>(*(msg + offset));
			AddParameter(ParameterId::PID_DESTINATION_ORDER, data);
			break;
		}
		case ParameterId::PID_HISTORY: {
			HistoryQosPolicy data;
			data.kind = static_cast<HistoryQosPolicyKind>(*(msg + offset));
			ReadInt32(msg + offset + 4, data.depth);
			AddParameter(ParameterId::PID_HISTORY, data);
			break;
		}
		case ParameterId::PID_RESOURCE_LIMITS: {
			ResourceLimitsQosPolicy data;
			ReadInt32(msg + offset, data.max_samples);
			ReadInt32(msg + offset + 4, data.max_instances);
			ReadInt32(msg + offset + 8, data.max_samples_per_instance);
			AddParameter(ParameterId::PID_RESOURCE_LIMITS, data);
			break;
		}
		case ParameterId::PID_OWNERSHIP: {
			OwnershipQosPolicy data;
			data.kind = static_cast<OwnershipQosPolicyKind>(*(msg + offset));
			AddParameter(ParameterId::PID_OWNERSHIP, data);
			break;
		}
		case ParameterId::PID_OWNERSHIP_STRENGTH: {
			OwnershipStrengthQosPolicy data;
			ReadInt32(msg + offset, data.value);
			AddParameter(ParameterId::PID_OWNERSHIP_STRENGTH, data);
			break;
		}
		case ParameterId::PID_PRESENTATION: {
			PresentationQosPolicy data;
			data.access_scope = static_cast<PresentationQosPolicyAccessScopeKind>(*(msg + offset));
			data.coherent_access = static_cast<bool>(*(msg + offset + 4));
			data.ordered_access = static_cast<bool>(*(msg + offset + 5));
			AddParameter(ParameterId::PID_PRESENTATION, data);
			break;
		}
		case ParameterId::PID_PARTITION: {
			PartitionQosPolicy data;
			uint32_t num = 0;
			ReadUInt32(msg + offset, num);
			if (0 == num) {
				break;
			}
			data.name.resize(num);
			uint32_t inOffset = 4;

			for (uint32_t i = 0; i < num; i++) {
				inOffset += ReaderString(msg + offset + inOffset, data.name[i]);
			}
			AddParameter(ParameterId::PID_PARTITION, data);
			break;
		}
		case ParameterId::PID_TIME_BASED_FILTER: {
			TimeBasedFilterQosPolicy data;
			ReadInt32(msg + offset, data.minimum_separation.sec);
			ReadUInt32(msg + offset + 4, data.minimum_separation.nanosec);
			AddParameter(ParameterId::PID_TIME_BASED_FILTER, data);
			break;
		}
		case ParameterId::PID_TRANSPORT_PRIORITY: {
			TransportPriorityQosPolicy data;
			ReadInt32(msg + offset, data.value);
			AddParameter(ParameterId::PID_TRANSPORT_PRIORITY, data);
			break;
		}
		case ParameterId::PID_DOMAIN_ID: {
			DomainId_t id;
			ReadUInt32(msg + offset, id);
			AddParameter(ParameterId::PID_DOMAIN_ID, id);
			break;
		}
		case ParameterId::PID_DOMAIN_TAG: {
			std::string tag;
			ReaderString(msg + offset, tag);
			AddParameter(ParameterId::PID_DOMAIN_TAG, tag);
			break;
		}
		case ParameterId::PID_PROTOCOL_VERSION: {
			ProtocolVersion_t data;
			memcpy(&data, msg + offset, 2);
			AddParameter(ParameterId::PID_PROTOCOL_VERSION, data);
			break;
		}
		case ParameterId::PID_VENDORID: {
			VendorId_t id = { *(msg + offset),*(msg + offset + 1) };
			AddParameter(ParameterId::PID_VENDORID, id);
			break;
		}
		case ParameterId::PID_UNICAST_LOCATOR: {
			Locator_t data;
			ReaderLocator(msg + offset, data);
			AddParameter(ParameterId::PID_UNICAST_LOCATOR, data);
			break;
		}
		case ParameterId::PID_MULTICAST_LOCATOR: {
			Locator_t data;
			ReaderLocator(msg + offset, data);
			AddParameter(ParameterId::PID_MULTICAST_LOCATOR, data);
			break;
		}
		case ParameterId::PID_DEFAULT_UNICAST_LOCATOR: {
			Locator_t data;
			ReaderLocator(msg + offset, data);
			AddParameter(ParameterId::PID_DEFAULT_UNICAST_LOCATOR, data);
			break;
		}
		case ParameterId::PID_DEFAULT_MULTICAST_LOCATOR: {
			Locator_t data;
			ReaderLocator(msg + offset, data);
			AddParameter(ParameterId::PID_DEFAULT_MULTICAST_LOCATOR, data);
			break;
		}
		case ParameterId::PID_METATRAFFIC_UNICAST_LOCATOR: {
			Locator_t data;
			ReaderLocator(msg + offset, data);
			AddParameter(ParameterId::PID_METATRAFFIC_UNICAST_LOCATOR, data);
			break;
		}
		case ParameterId::PID_METATRAFFIC_MULTICAST_LOCATOR: {
			Locator_t data;
			ReaderLocator(msg + offset, data);
			AddParameter(ParameterId::PID_METATRAFFIC_MULTICAST_LOCATOR, data);
			break;
		}
		case ParameterId::PID_EXPECTS_INLINE_QOS: {
			bool data = static_cast<bool>(*(msg + offset));
			AddParameter(ParameterId::PID_EXPECTS_INLINE_QOS, data);
			break;
		}
		case ParameterId::PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT: {
			Count_t data;
			ReadInt32(msg + offset, data);
			AddParameter(ParameterId::PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT, data);
			break;
		}
		case ParameterId::PID_PARTICIPANT_LEASE_DURATION: {
			Duration_t data;
			ReadInt32(msg + offset, data.sec);
			ReadUInt32(msg + offset + 4, data.nanosec);
			AddParameter(ParameterId::PID_PARTICIPANT_LEASE_DURATION, data);
			break;
		}
		case ParameterId::PID_CONTENT_FILTER_PROPERTY: {
			ContentFilterProperty_t data;
			uint32_t inOffset = 0;
			inOffset += ReaderString(msg + offset, data.contentFilteredTopicName);
			inOffset += ReaderString(msg + offset + inOffset, data.relatedTopicName);
			inOffset += ReaderString(msg + offset + inOffset, data.filterClassName);
			inOffset += ReaderString(msg + offset + inOffset, data.filterExpression);
			uint32_t num = 0;
			ReadUInt32(msg + offset + inOffset, num);
			data.expressionParameters.resize(num);
			inOffset += 4;
			for (uint32_t i = 0; i < num; i++) {
				inOffset += ReaderString(msg + offset + inOffset, data.expressionParameters[i]);
			}
			AddParameter(ParameterId::PID_CONTENT_FILTER_PROPERTY, data);
			break;
		}
		case ParameterId::PID_PARTICIPANT_GUID: {
			BuiltinTopicKey_t key;
			memcpy(key.value, msg + offset, 16);
			AddParameter(ParameterId::PID_PARTICIPANT_GUID, key);
			break;
		}
		case ParameterId::PID_GROUP_GUID: {
			GUID_t data;
			memcpy(&data, msg + offset, 16);
			AddParameter(ParameterId::PID_GROUP_GUID, data);
			break;
		}
		case ParameterId::PID_GROUP_ENTITY_ID: {
			EntityId_t data;
			memcpy(&data, msg + offset, 4);
			AddParameter(ParameterId::PID_GROUP_ENTITY_ID, data);
			break;
		}
		case ParameterId::PID_BUILTIN_ENDPOINT_SET: {
			BuiltinEndpointSet_t data;
			ReadUInt32(msg + offset, data);
			AddParameter(ParameterId::PID_BUILTIN_ENDPOINT_SET, data);
			break;
		}
		case ParameterId::PID_BUILTIN_ENDPOINT_QOS: {
			BuiltinEndpointQos_t data;
			ReadUInt32(msg + offset, data);
			AddParameter(ParameterId::PID_BUILTIN_ENDPOINT_QOS, data);
			break;
		}
		case ParameterId::PID_PROPERTY_LIST: {
			PropertySeq data;
			uint32_t num = 0;
			ReadUInt32(msg + offset, num);
			if (0 == num) {
				break;
			}
			data.resize(num);
			uint32_t inOffset = 4;

			for (uint32_t i = 0; i < num; i++) {
				inOffset += ReaderString(msg + offset + inOffset, data[i].name);
				inOffset += ReaderString(msg + offset + inOffset, data[i].value);
			}
			AddParameter(ParameterId::PID_PROPERTY_LIST, data);
			break;
		}
		case ParameterId::PID_TYPE_MAX_SIZE_SERIALIZED: {
			int32_t data;
			ReadInt32(msg + offset, data);
			AddParameter(ParameterId::PID_TYPE_MAX_SIZE_SERIALIZED, data);
			break;
		}
		case ParameterId::PID_ENTITY_NAME: {
			EntityName_t name;
			ReaderString(msg + offset, name);
			AddParameter(ParameterId::PID_ENTITY_NAME, name);
			break;
		}
		case ParameterId::PID_ENDPOINT_GUID: {
			BuiltinTopicKey_t key;
			memcpy(key.value, msg + offset, 16);
			AddParameter(ParameterId::PID_ENDPOINT_GUID, key);
			break;
		}
		case ParameterId::PID_CONTENT_FILTER_INFO: {
			ContentFilterInfo_t data;
			uint32_t num = 0;
			ReadUInt32(msg + offset, num);
			uint32_t inOffset = 4;
			data.filterResult.resize(num);
			for (uint32_t i = 0; i < num; i++) {
				inOffset += ReadInt32(msg + offset + inOffset, data.filterResult[i]);
			}
			ReadUInt32(msg + offset + inOffset, num);
			inOffset += 4;
			data.filterSignatures.resize(num);
			for (uint32_t i = 0; i < num; i++) {
				inOffset += ReadInt32(msg + offset + inOffset, data.filterSignatures[i][0]);
				inOffset += ReadInt32(msg + offset + inOffset, data.filterSignatures[i][1]);
				inOffset += ReadInt32(msg + offset + inOffset, data.filterSignatures[i][2]);
				inOffset += ReadInt32(msg + offset + inOffset, data.filterSignatures[i][3]);
			}
			AddParameter(ParameterId::PID_CONTENT_FILTER_INFO, data);
			break;
		}
		case ParameterId::PID_COHERENT_SET: {
			SequenceNumber_t data;
			ReadInt32(msg + offset, data.high);
			ReadUInt32(msg + offset + 4, data.low);
			AddParameter(ParameterId::PID_COHERENT_SET, data);
			break;
		}
		case ParameterId::PID_DIRECTED_WRITE: {
			GUID_t data;
			memcpy(&data, msg + offset, 16);
			AddParameter(ParameterId::PID_DIRECTED_WRITE, data);
			break;
		}
		case ParameterId::PID_ORIGINAL_WRITER_INFO: {
			OriginalWriterInfo_t data;
			memcpy(&data.originalWriterGUID, msg + offset, 16);
			ReadInt32(msg + offset + 16, data.originalWriterSN.high);
			ReadUInt32(msg + offset + 20, data.originalWriterSN.low);
			uint32_t inLen = len - 24;
			uint32_t inOffset = offset + 24;
			ParameterId inId;
			ReadUInt16(msg + inOffset, (uint16_t&)inId);

			while (inId != ParameterId::PID_SENTINEL && inOffset < inLen) {
				Parameter_t par;
				ReadUInt16(msg + inOffset, (uint16_t&)par.parameterId);
				ReadUInt16(msg + inOffset + 2, par.length);
				par.value.resize(par.length);
				std::copy(msg + inOffset + 4, msg + inOffset + 4 + par.length, par.value.begin());

				data.originalWriterQos.push_back(std::move(par));
				inOffset += 4 + (par.length + 3u) & ~3u;
				ReadUInt16(msg + inOffset, (uint16_t&)inId);
			}
			AddParameter(ParameterId::PID_ORIGINAL_WRITER_INFO, data);
			break;
		}
		case ParameterId::PID_GROUP_COHERENT_SET: {
			SequenceNumber_t data;
			ReadInt32(msg + offset, data.high);
			ReadUInt32(msg + offset + 4, data.low);
			AddParameter(ParameterId::PID_GROUP_COHERENT_SET, data);
			break;
		}
		case ParameterId::PID_GROUP_SEQ_NUM: {
			SequenceNumber_t data;
			ReadInt32(msg + offset, data.high);
			ReadUInt32(msg + offset + 4, data.low);
			AddParameter(ParameterId::PID_GROUP_SEQ_NUM, data);
			break;
		}
		case ParameterId::PID_WRITER_GROUP_INFO: {
			WriterGroupInfo_t data;
			memcpy(data.writerSet, msg + offset, 4);
			AddParameter(ParameterId::PID_WRITER_GROUP_INFO, data);
			break;
		}
		case ParameterId::PID_SECURE_WRITER_GROUP_INFO: {
			WriterGroupInfo_t data;
			memcpy(data.writerSet, msg + offset, 4);
			AddParameter(ParameterId::PID_SECURE_WRITER_GROUP_INFO, data);
			break;
		}
		case ParameterId::PID_KEY_HASH: {
			KeyHash_t data;
			memcpy(data, msg + offset, 16);
			AddParameter(ParameterId::PID_KEY_HASH, data);
			break;
		}
		case ParameterId::PID_STATUS_INFO: {
			StatusInfo_t data;
			memcpy(data, msg + offset, 4);
			AddParameter(ParameterId::PID_STATUS_INFO, data);
			break;
		}
#ifdef DDSSECURITY
		case ParameterId::PID_PERMISSIONS_TOKEN: {
			DataHolder data;
			uint32_t inOffset = 0;
			inOffset += ReaderString(msg + offset + inOffset, data.class_id); //解析class id
			uint32_t num = 0;
			ReadUInt32(msg + offset + inOffset, num); //properties长度
			inOffset += 4;
			//逐个生成properties元素
			for (uint32_t i = 0; i < num; i++) {
				Property_t curProperty;
				inOffset += ReaderString(msg + offset + inOffset, curProperty.name);
				inOffset += ReaderString(msg + offset + inOffset, curProperty.value);
				data.properties.push_back(curProperty);
			}

			ReadUInt32(msg + offset + inOffset, num); //binary_properties长度
			inOffset += 4;
			//逐个生成binary_properties元素
			for (uint32_t i = 0; i < num; i++) {
				BinaryProperty_t curBinaryProperty;
				inOffset += ReaderString(msg + offset + inOffset, curBinaryProperty.name);
				uint32_t binary_properties_value_num;
				ReadUInt32(msg + offset + inOffset, binary_properties_value_num);
				inOffset += 4;
				for (uint32_t j = 0; j < binary_properties_value_num; j++) {
					curBinaryProperty.value[j] = *(msg + offset + inOffset);
					inOffset += 1;
				}
				data.binary_properties.push_back(curBinaryProperty);
			}
			AddParameter(ParameterId::PID_PERMISSIONS_TOKEN, data);
			break;
		}
		case ParameterId::PID_IDENTITY_TOKEN: {
			DataHolder data;
			uint32_t inOffset = 0;
			inOffset += ReaderString(msg + offset + inOffset, data.class_id); //解析class id
			uint32_t num = 0;
			ReadUInt32(msg + offset + inOffset, num); //properties长度
			inOffset += 4;
			//逐个生成properties元素
			for (uint32_t i = 0; i < num; i++) {
				Property_t curProperty;
				inOffset += ReaderString(msg + offset + inOffset, curProperty.name);
				inOffset += ReaderString(msg + offset + inOffset, curProperty.value);
				data.properties.push_back(curProperty);
			}

			ReadUInt32(msg + offset + inOffset, num); //binary_properties长度
			inOffset += 4;
			//逐个生成binary_properties元素
			for (uint32_t i = 0; i < num; i++) {
				BinaryProperty_t curBinaryProperty;
				inOffset += ReaderString(msg + offset + inOffset, curBinaryProperty.name);
				uint32_t binary_properties_value_num;
				ReadUInt32(msg + offset + inOffset, binary_properties_value_num);
				inOffset += 4;
				for (uint32_t j = 0; j < binary_properties_value_num; j++) {
					curBinaryProperty.value[j] = *(msg + offset + inOffset);
					inOffset += 1;
				}
				data.binary_properties.push_back(curBinaryProperty);
			}
			AddParameter(ParameterId::PID_IDENTITY_TOKEN, data);
			break;
		}
		case ParameterId::PID_PARTICIPANT_SECURITY_INFO: {
			ParticipantSecurityInfo data;
			uint32_t inOffset = 0;
			ReadUInt32(msg + offset + inOffset, data.participant_security_atributes);
			inOffset += 4;
			ReadUInt32(msg + offset + inOffset, data.plugin_participant_security_attributes);
			inOffset += 4;
			AddParameter(ParameterId::PID_PARTICIPANT_SECURITY_INFO, data);
			break;
		}
		case ParameterId::PID_DATA_TAGS: {
			DataTags_t data;
			uint32_t inOffset = 0;

			uint32_t num = 0;
			ReadUInt32(msg + offset + inOffset, num); //tags长度
			//tags.length(num);
			inOffset += 4;

			//逐个生成tags元素
			for (uint32_t i = 0; i < num; i++) {
				Tag_t temp;
				inOffset += ReaderString(msg + offset + inOffset, temp.name);
				inOffset += ReaderString(msg + offset + inOffset, temp.value);

				data.tags.push_back(temp);
			}
			AddParameter(ParameterId::PID_DATA_TAGS, data);
			break;
		}
		case ParameterId::PID_ENDPOINT_SECURITY_INFO: {
			EndpointSecurityInfo data;
			uint32_t inOffset = 0;
			ReadUInt32(msg + offset + inOffset, data.endpoint_security_attributes);
			inOffset += 4;
			ReadUInt32(msg + offset + inOffset, data.plugin_endpoint_security_attributes);
			inOffset += 4;
			AddParameter(ParameterId::PID_ENDPOINT_SECURITY_INFO, data);
			break;
		}
#endif // DDSSECURITY
		default: 
			break;
		}
 		offset += len;
	}
}


uint32_t ParameterList::GetParameterList(ParameterList_t& list)
{
	for (auto& it : parameterList_) {
		Parameter_t parameter;
		parameter.parameterId = static_cast<ParameterId_t>(it.second->pid_);
		parameter.length = it.second->length_;
		parameter.value.resize(parameter.length);
		it.second->AddToMessage(parameter.value.data(), parameter.length);
		list.push_back(parameter);
	}
	return list.size();
}

uint32_t ParameterList::ReadInt16(const octet* msg, int16_t& i16)
{
	octet* dest = (octet*)&i16;
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(dest, msg, 2);
	} else {
		dest[0] = msg[1];
		dest[1] = msg[0];
	}
	return 2u;
}
uint32_t ParameterList::ReadUInt16(const octet* msg, uint16_t& ui16)
{
	octet* dest = (octet*)&ui16;
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(dest, msg, 2);
	} else {
		dest[0] = msg[1];
		dest[1] = msg[0];
	}
	return 2u;

}
uint32_t ParameterList::ReadInt32(const octet* msg, int32_t& i32)
{
	octet* dest = (octet*)&i32;
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(dest, msg, 4);
	} else {
		dest[0] = msg[3];
		dest[1] = msg[2];
		dest[2] = msg[1];
		dest[3] = msg[0];
	}
	return 4u;
}
uint32_t ParameterList::ReadUInt32(const octet* msg, uint32_t& ui32)
{
	octet* dest = (octet*)&ui32;
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(dest, msg, 4);
	} else {
		dest[0] = msg[3];
		dest[1] = msg[2];
		dest[2] = msg[1];
		dest[3] = msg[0];
	}
	return 4u;
}


uint32_t ParameterList::ReadInt(const octet* msg, octet* dest, const octet len)
{
	if (msgEndian_ == PL_DEFAULT_ENDIAN) {
		memcpy(dest, msg, len);
	} else {
		for (int32_t i = 0; i < len; i++) {
			dest[i] = msg[len - 1 - i];
		}
	}
	return len;
}

uint32_t ParameterList::ReaderString(const octet* msg, std::string& str)
{
	uint32_t size = 0;
	ReadUInt32(msg, size);
	if (size != 0) {
		str.resize(size);
		std::copy(msg + 4, msg + 4 + size, str.begin());
		if(str.back() == '\0') {
			str.pop_back(); 
		}
	}
	return 4u + (size + 3u) & ~3u;
}

uint32_t ParameterList::ReaderLocator(const octet* msg, Locator_t& loc)
{
	ReadInt32(msg, loc.kind);
	ReadUInt32(msg + 4, loc.port);
	memcpy(loc.address, msg + 8, 16);
	return 24u;
}

