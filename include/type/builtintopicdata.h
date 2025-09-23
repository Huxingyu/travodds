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
#ifndef TRAVODDS_RTPS_DISCOVERY_BUILTINTOPICDATA_H
#define TRAVODDS_RTPS_DISCOVERY_BUILTINTOPICDATA_H

#include "qos/qos.h"
#ifdef DDSSECURITY
//#include "rtps/basictypertps.h"
#include "security/securitycommon.h"
#endif // DDSSECURITY

TRAVODDS_NAMESPACE_BEGIN

struct ParticipantBuiltinTopicData
{
	BuiltinTopicKey_t 			key;
	UserDataQosPolicy 			user_data;
	PropertySeq                 sys_info;
#ifdef DDSSECURITY
	/** @brief 指明该Participant内置主题数据中是否包含安全相关数据。 */
	bool contains_security_data;
	/** @brief The identity token */
	IdentityToken identity_token;
	/** @brief The permissions token */
	PermissionsToken permissions_token;
	/** @brief Information describing the participant security */
	ParticipantSecurityInfo security_info;
#endif // DDSSECURITY
};

struct TopicBuiltinTopicData {
	BuiltinTopicKey_t 			key;
	std::string 				name;
	std::string 				type_name;
	DurabilityQosPolicy 		durability;
	DurabilityServiceQosPolicy 	durability_service;
	DeadlineQosPolicy 			deadline;
	LatencyBudgetQosPolicy 		latency_budget;
	LivelinessQosPolicy 		liveliness;
	ReliabilityQosPolicy 		reliability;
	TransportPriorityQosPolicy 	transport_priority;
	LifespanQosPolicy 			lifespan;
	DestinationOrderQosPolicy 	destination_order;
	HistoryQosPolicy 			history;
	ResourceLimitsQosPolicy 	resource_limits;
	OwnershipQosPolicy			ownership;
	TopicDataQosPolicy 			topic_data;
};

struct PublicationBuiltinTopicData {
	BuiltinTopicKey_t 			key;
	BuiltinTopicKey_t 			participant_key;
	std::string 				topic_name;
	std::string 				type_name;
	DurabilityQosPolicy 		durability;
	DurabilityServiceQosPolicy 	durability_service;
	DeadlineQosPolicy 			deadline;
	LatencyBudgetQosPolicy 		latency_budget;
	LivelinessQosPolicy 		liveliness;
	ReliabilityQosPolicy 		reliability;
	LifespanQosPolicy 			lifespan;
	UserDataQosPolicy 			user_data;
	OwnershipQosPolicy 			ownership;
	OwnershipStrengthQosPolicy 	ownership_strength;
	DestinationOrderQosPolicy 	destination_order;
	PresentationQosPolicy 		presentation;
	PartitionQosPolicy 			partition;
	TopicDataQosPolicy 			topic_data;
	GroupDataQosPolicy 			group_data;
};

#ifdef DDSSECURITY
struct PublicationBuiltinTopicSecureData : public PublicationBuiltinTopicData {
	/** @brief The data tags */
	DataTags_t data_tags;
	/** @brief 实体安全相关信息 */
	EndpointSecurityInfo security_info;
	/** @brief 发现过程是否需要被保护 */
	bool protected_discovery;
};
#endif // DDSSECURITY

struct SubscriptionBuiltinTopicData {
	BuiltinTopicKey_t 			key;
	BuiltinTopicKey_t 			participant_key;
	std::string 				topic_name;
	std::string 				type_name;
	DurabilityQosPolicy 		durability;
	DeadlineQosPolicy 			deadline;
	LatencyBudgetQosPolicy 		latency_budget;
	LivelinessQosPolicy 		liveliness;
	ReliabilityQosPolicy 		reliability;
	OwnershipQosPolicy 			ownership;
	DestinationOrderQosPolicy 	destination_order;
	UserDataQosPolicy 			user_data;
	TimeBasedFilterQosPolicy 	time_based_filter;
	PresentationQosPolicy 		presentation;
	PartitionQosPolicy 			partition;
	TopicDataQosPolicy 			topic_data;
	GroupDataQosPolicy 			group_data;
};

#ifdef DDSSECURITY
struct SubscriptionBuiltinTopicSecureData : SubscriptionBuiltinTopicData {
	/** @brief The data tags */
	DataTags_t data_tags;
	/** @brief 实体安全相关信息 */
	EndpointSecurityInfo security_info;
	/** @brief 发现过程是否需要被保护 */
	bool protected_discovery;
};
#endif // DDSSECURITY

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_RTPS_DISCOVERY_BUILTINTOPICDATA_H
