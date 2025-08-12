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
#ifndef AGIDDS_RTPS_DISCOVERY_BUILTINTOPICDATA_H
#define AGIDDS_RTPS_DISCOVERY_BUILTINTOPICDATA_H

#include "qos/qos.h"

AGIDDS_NAMESPACE_BEGIN

struct ParticipantBuiltinTopicData
{
	BuiltinTopicKey_t 			key;
	UserDataQosPolicy 			user_data;
	PropertySeq                 sys_info;
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

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_RTPS_DISCOVERY_BUILTINTOPICDATA_H