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
#include "BasicType.h"

AGIDDS_NAMESPACE_BEGIN

const  string  USERDATA_QOS_POLICY_NAME              = "UserData";
const  string  DURABILITY_QOS_POLICY_NAME            = "Durability";
const  string  PRESENTATION_QOS_POLICY_NAME          = "Presentation";
const  string  DEADLINE_QOS_POLICY_NAME              = "Deadline";
const  string  LATENCYBUDGET_QOS_POLICY_NAME         = "LatencyBudget";
const  string  OWNERSHIP_QOS_POLICY_NAME             = "Ownership";
const  string  OWNERSHIPSTRENGTH_QOS_POLICY_NAME     = "OwnershipStrength";
const  string  LIVELINESS_QOS_POLICY_NAME            = "Liveliness";
const  string  TIMEBASEDFILTER_QOS_POLICY_NAME       = "TimeBasedFilter";
const  string  PARTITION_QOS_POLICY_NAME             = "Partition";
const  string  RELIABILITY_QOS_POLICY_NAME           = "Reliability";
const  string  DESTINATIONORDER_QOS_POLICY_NAME      = "DestinationOrder";
const  string  HISTORY_QOS_POLICY_NAME               = "History";
const  string  RESOURCELIMITS_QOS_POLICY_NAME        = "ResourceLimits";
const  string  ENTITYFACTORY_QOS_POLICY_NAME         = "EntityFactory";
const  string  WRITERDATALIFECYCLE_QOS_POLICY_NAME   = "WriterDataLifecycle";
const  string  READERDATALIFECYCLE_QOS_POLICY_NAME   = "ReaderDataLifecycle";
const  string  TOPICDATA_QOS_POLICY_NAME             = "TopicData";
const  string  GROUPDATA_QOS_POLICY_NAME             = "TransportPriority";
const  string  LIFESPAN_QOS_POLICY_NAME              = "Lifespan";
const  string  DURABILITYSERVICE_POLICY_NAME         = "DurabilityService";

const  QosPolicyId_t  INVALID_QOS_POLICY_ID = 0;
const  QosPolicyId_t  USERDATA_QOS_POLICY_ID = 1;
const  QosPolicyId_t  DURABILITY_QOS_POLICY_ID = 2;
const  QosPolicyId_t  PRESENTATION_QOS_POLICY_ID = 3;
const  QosPolicyId_t  DEADLINE_QOS_POLICY_ID = 4;
const  QosPolicyId_t  LATENCYBUDGET_QOS_POLICY_ID = 5;
const  QosPolicyId_t  OWNERSHIP_QOS_POLICY_ID = 6;
const  QosPolicyId_t  OWNERSHIPSTRENGTH_QOS_POLICY_ID = 7;
const  QosPolicyId_t  LIVELINESS_QOS_POLICY_ID = 8;
const  QosPolicyId_t  TIMEBASEDFILTER_QOS_POLICY_ID = 9;
const  QosPolicyId_t  PARTITION_QOS_POLICY_ID = 10;
const  QosPolicyId_t  RELIABILITY_QOS_POLICY_ID = 11;
const  QosPolicyId_t  DESTINATIONORDER_QOS_POLICY_ID = 12;
const  QosPolicyId_t  HISTORY_QOS_POLICY_ID = 13;
const  QosPolicyId_t  RESOURCELIMITS_QOS_POLICY_ID = 14;
const  QosPolicyId_t  ENTITYFACTORY_QOS_POLICY_ID = 15;
const  QosPolicyId_t  WRITERDATALIFECYCLE_QOS_POLICY_ID = 16;
const  QosPolicyId_t  READERDATALIFECYCLE_QOS_POLICY_ID = 17;
const  QosPolicyId_t  TOPICDATA_QOS_POLICY_ID = 18;
const  QosPolicyId_t  GROUPDATA_QOS_POLICY_ID = 19;
const  QosPolicyId_t  TRANSPORTPRIORITY_QOS_POLICY_ID = 20;
const  QosPolicyId_t  LIFESPAN_QOS_POLICY_ID = 21;
const  QosPolicyId_t  DURABILITYSERVICE_QOS_POLICY_ID = 22;

struct UserDataQosPolicy {
	Sequence<octet> value;
};

struct TopicDataQosPolicy {
	Sequence<octet> value;
};

struct GroupDataQosPolicy {
	Sequence<octet> value;
};

struct TransportPriorityQosPolicy {
	long value;
};
struct LifespanQosPolicy {
	Duration_t duration;
};

enum DurabilityQosPolicyKind {
	VOLATILE_DURABILITY_QOS,
	TRANSIENT_LOCAL_DURABILITY_QOS,
	TRANSIENT_DURABILITY_QOS,
	PERSISTENT_DURABILITY_QOS
};

struct DurabilityQosPolicy {
	DurabilityQosPolicyKind kind;
};
enum PresentationQosPolicyAccessScopeKind {
	INSTANCE_PRESENTATION_QOS,
	TOPIC_PRESENTATION_QOS,
	GROUP_PRESENTATION_QOS
};
struct PresentationQosPolicy {
	PresentationQosPolicyAccessScopeKind access_scope;
	bool coherent_access;
	bool ordered_access;

};

struct DeadlineQosPolicy {
	Duration_t period;
};

struct LatencyBudgetQosPolicy {
	Duration_t duration;
};

enum OwnershipQosPolicyKind {
	SHARED_OWNERSHIP_QOS,
	EXCLUSIVE_OWNERSHIP_QOS
};

struct OwnershipQosPolicy {
	OwnershipQosPolicyKind kind;
};

struct OwnershipStrengthQosPolicy {
	long value;
};
enum LivelinessQosPolicyKind {
	AUTOMATIC_LIVELINESS_QOS,
	MANUAL_BY_PARTICIPANT_LIVELINESS_QOS,
	MANUAL_BY_TOPIC_LIVELINESS_QOS
};
struct LivelinessQosPolicy {
	LivelinessQosPolicyKind kind;
	Duration_t lease_duration;
};

struct TimeBasedFilterQosPolicy {
	Duration_t minimum_separation;
};
struct PartitionQosPolicy {
	StringSeq name;
};

enum ReliabilityQosPolicyKind {
	BEST_EFFORT_RELIABILITY_QOS,
	RELIABLE_RELIABILITY_QOS
};

struct ReliabilityQosPolicy {
	ReliabilityQosPolicyKind kind;
	Duration_t max_blocking_time;
};

enum DestinationOrderQosPolicyKind {
	BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS,
	BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS
};

struct DestinationOrderQosPolicy {
	DestinationOrderQosPolicyKind kind;
};

enum HistoryQosPolicyKind {
	KEEP_LAST_HISTORY_QOS,
	KEEP_ALL_HISTORY_QOS
};

struct HistoryQosPolicy {
	HistoryQosPolicyKind kind;
	long depth;
};

struct ResourceLimitsQosPolicy {
	long max_samples;
	long max_instances;
	long max_samples_per_instance;
};

struct EntityFactoryQosPolicy {
	bool autoenable_created_entities;
};

struct WriterDataLifecycleQosPolicy {
	bool autodispose_unregistered_instances;
};

struct ReaderDataLifecycleQosPolicy {
	Duration_t autopurge_nowriter_samples_delay;
	Duration_t autopurge_disposed_samples_delay;
};

struct DurabilityServiceQosPolicy {
	Duration_t service_cleanup_delay;
	HistoryQosPolicyKind history_kind;
	long history_depth;
	long max_samples;
	long max_instances;
	long max_samples_per_instance;
};

struct DomainParticipantFactoryQos {
	EntityFactoryQosPolicy entity_factory;
};

struct DomainParticipantQos {
	UserDataQosPolicy user_data;
	EntityFactoryQosPolicy entity_factory;
};

struct TopicQos {
	TopicDataQosPolicy            topic_data;
	DurabilityQosPolicy           durability;
	DurabilityServiceQosPolicy    durability_service;
	DeadlineQosPolicy             deadline;
	LatencyBudgetQosPolicy        latency_budget;
	LivelinessQosPolicy           liveliness;
	ReliabilityQosPolicy          reliability;
	DestinationOrderQosPolicy     destination_order;
	HistoryQosPolicy              history;
	ResourceLimitsQosPolicy       resource_limits;
	TransportPriorityQosPolicy    transport_priority;
	LifespanQosPolicy             lifespan;
	OwnershipQosPolicy            ownership;
};

struct DataWriterQos {
	DurabilityQosPolicy          durability;
	DurabilityServiceQosPolicy   durability_service;
	DeadlineQosPolicy            deadline;
	LatencyBudgetQosPolicy       latency_budget;
	LivelinessQosPolicy          liveliness;
	ReliabilityQosPolicy         reliability;
	DestinationOrderQosPolicy    destination_order;
	HistoryQosPolicy             history;
	ResourceLimitsQosPolicy      resource_limits;
	TransportPriorityQosPolicy   transport_priority;
	LifespanQosPolicy            lifespan;
	UserDataQosPolicy            user_data;
	OwnershipQosPolicy           ownership;
	OwnershipStrengthQosPolicy   ownership_strength;
	WriterDataLifecycleQosPolicy writer_data_lifecycle;
};

struct PublisherQos {
	PresentationQosPolicy   presentation;
	PartitionQosPolicy      partition;
	GroupDataQosPolicy      group_data;
	EntityFactoryQosPolicy  entity_factory;
};

struct DataReaderQos {
	DurabilityQosPolicy           durability;
	DeadlineQosPolicy             deadline;
	LatencyBudgetQosPolicy        latency_budget;
	LivelinessQosPolicy           liveliness;
	ReliabilityQosPolicy          reliability;
	DestinationOrderQosPolicy     destination_order;
	HistoryQosPolicy              history;
	ResourceLimitsQosPolicy       resource_limits;
	UserDataQosPolicy             user_data;
	OwnershipQosPolicy            ownership;
	TimeBasedFilterQosPolicy      time_based_filter;
	ReaderDataLifecycleQosPolicy  reader_data_lifecycle;
};

struct SubscriberQos {
	PresentationQosPolicy       presentation;
	PartitionQosPolicy          partition;
	GroupDataQosPolicy          group_data;
	EntityFactoryQosPolicy      entity_factory;
};

DDS_DLL extern const DomainParticipantQos PARTICIPANT_QOS_DEFAULT;
DDS_DLL extern const TopicQos TOPIC_QOS_DEFAULT;
DDS_DLL extern const PublisherQos PUBLISHER_QOS_DEFAULT;
DDS_DLL extern const SubscriberQos SUBSCRIBER_QOS_DEFAULT;
DDS_DLL extern const DataWriterQos DATAWRITER_QOS_DEFAULT;
DDS_DLL extern const DataReaderQos DATAREADER_QOS_DEFAULT;

AGIDDS_NAMESPACE_END