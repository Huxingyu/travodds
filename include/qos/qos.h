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
#ifndef AGIDDS_QOS_QOS_H
#define AGIDDS_QOS_QOS_H

#include "agidds_dll.h"
#include "type/basictype.h"

AGIDDS_NAMESPACE_BEGIN

const  std::string  USERDATA_QOS_POLICY_NAME              = "UserData";
const  std::string  DURABILITY_QOS_POLICY_NAME            = "Durability";
const  std::string  PRESENTATION_QOS_POLICY_NAME          = "Presentation";
const  std::string  DEADLINE_QOS_POLICY_NAME              = "Deadline";
const  std::string  LATENCYBUDGET_QOS_POLICY_NAME         = "LatencyBudget";
const  std::string  OWNERSHIP_QOS_POLICY_NAME             = "Ownership";
const  std::string  OWNERSHIPSTRENGTH_QOS_POLICY_NAME     = "OwnershipStrength";
const  std::string  LIVELINESS_QOS_POLICY_NAME            = "Liveliness";
const  std::string  TIMEBASEDFILTER_QOS_POLICY_NAME       = "TimeBasedFilter";
const  std::string  PARTITION_QOS_POLICY_NAME             = "Partition";
const  std::string  RELIABILITY_QOS_POLICY_NAME           = "Reliability";
const  std::string  DESTINATIONORDER_QOS_POLICY_NAME      = "DestinationOrder";
const  std::string  HISTORY_QOS_POLICY_NAME               = "History";
const  std::string  RESOURCELIMITS_QOS_POLICY_NAME        = "ResourceLimits";
const  std::string  ENTITYFACTORY_QOS_POLICY_NAME         = "EntityFactory";
const  std::string  WRITERDATALIFECYCLE_QOS_POLICY_NAME   = "WriterDataLifecycle";
const  std::string  READERDATALIFECYCLE_QOS_POLICY_NAME   = "ReaderDataLifecycle";
const  std::string  TOPICDATA_QOS_POLICY_NAME             = "TopicData";
const  std::string  GROUPDATA_QOS_POLICY_NAME             = "TransportPriority";
const  std::string  LIFESPAN_QOS_POLICY_NAME              = "Lifespan";
const  std::string  DURABILITYSERVICE_POLICY_NAME         = "DurabilityService";

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
	int32_t value;

	TransportPriorityQosPolicy() : value(0) {
	}
};
struct LifespanQosPolicy {
	Duration_t duration;

	LifespanQosPolicy() : duration(TIME_INFINITE) {

	}
};

enum DurabilityQosPolicyKind : octet {
	VOLATILE_DURABILITY_QOS,
	TRANSIENT_LOCAL_DURABILITY_QOS,
	TRANSIENT_DURABILITY_QOS,
	PERSISTENT_DURABILITY_QOS
};

struct DurabilityQosPolicy {

	DurabilityQosPolicyKind kind;

	DurabilityQosPolicy() : kind(VOLATILE_DURABILITY_QOS) {

	}
};
enum PresentationQosPolicyAccessScopeKind : octet {
	INSTANCE_PRESENTATION_QOS,
	TOPIC_PRESENTATION_QOS,
	GROUP_PRESENTATION_QOS
};
struct PresentationQosPolicy {
	PresentationQosPolicyAccessScopeKind access_scope;
	bool coherent_access;
	bool ordered_access;

	PresentationQosPolicy() : access_scope(INSTANCE_PRESENTATION_QOS), coherent_access(false), ordered_access(false) {

	}

};

struct DeadlineQosPolicy {
	Duration_t period;

	DeadlineQosPolicy() : period(TIME_INFINITE) {

	}
};

struct LatencyBudgetQosPolicy {
	Duration_t duration;

	/* wangyi fastdds 适配默认值修改为TIME_ZERO */
	LatencyBudgetQosPolicy() : duration(TIME_ZERO) {

	}
};

enum OwnershipQosPolicyKind : octet {
	SHARED_OWNERSHIP_QOS,
	EXCLUSIVE_OWNERSHIP_QOS
};

struct OwnershipQosPolicy {
	OwnershipQosPolicyKind kind;

	OwnershipQosPolicy() : kind(SHARED_OWNERSHIP_QOS) {

	}
};

struct OwnershipStrengthQosPolicy {
	int32_t value;

	OwnershipStrengthQosPolicy() : value(0) {

	}
};
enum LivelinessQosPolicyKind : octet {
	AUTOMATIC_LIVELINESS_QOS,
	MANUAL_BY_PARTICIPANT_LIVELINESS_QOS,
	MANUAL_BY_TOPIC_LIVELINESS_QOS
};
struct LivelinessQosPolicy {
	LivelinessQosPolicyKind kind;
	Duration_t lease_duration;

	LivelinessQosPolicy() : kind(AUTOMATIC_LIVELINESS_QOS), lease_duration(TIME_INFINITE) {

	}
};

struct TimeBasedFilterQosPolicy {
	Duration_t minimum_separation;

	TimeBasedFilterQosPolicy() : minimum_separation(TIME_ZERO) {

	}
};
struct PartitionQosPolicy {
	StringSeq name;
};

enum ReliabilityQosPolicyKind : octet {
	BEST_EFFORT_RELIABILITY_QOS = 1,
	RELIABLE_RELIABILITY_QOS
};

struct ReliabilityQosPolicy {
	ReliabilityQosPolicyKind kind;
	Duration_t max_blocking_time;

	ReliabilityQosPolicy() : kind(BEST_EFFORT_RELIABILITY_QOS), max_blocking_time{0, 100000} {
	}
};

enum DestinationOrderQosPolicyKind : octet {
	BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS,
	BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS
};

struct DestinationOrderQosPolicy {
	DestinationOrderQosPolicyKind kind;

	DestinationOrderQosPolicy() : kind(BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS) {

	}
};

enum HistoryQosPolicyKind : octet {
	KEEP_LAST_HISTORY_QOS,
	KEEP_ALL_HISTORY_QOS
};

struct HistoryQosPolicy {
	HistoryQosPolicyKind kind;
	int32_t depth;

	HistoryQosPolicy() : kind(KEEP_LAST_HISTORY_QOS), depth(1){

	}
};

struct ResourceLimitsQosPolicy {
	int32_t max_samples;
	int32_t max_instances;
	int32_t max_samples_per_instance;

	ResourceLimitsQosPolicy() : max_samples(LENGTH_UNLIMITED), max_instances(LENGTH_UNLIMITED), max_samples_per_instance(LENGTH_UNLIMITED){

	}
};

struct EntityFactoryQosPolicy {
	bool autoenable_created_entities;

	EntityFactoryQosPolicy() : autoenable_created_entities(true) {

	}
};

struct WriterDataLifecycleQosPolicy {
	bool autodispose_unregistered_instances;

	WriterDataLifecycleQosPolicy() : autodispose_unregistered_instances(true) {

	}
};

struct ReaderDataLifecycleQosPolicy {
	Duration_t autopurge_nowriter_samples_delay;
	Duration_t autopurge_disposed_samples_delay;

	ReaderDataLifecycleQosPolicy() :
		autopurge_nowriter_samples_delay(TIME_INFINITE),
		autopurge_disposed_samples_delay(TIME_INFINITE) {

	}
};

struct DurabilityServiceQosPolicy {
	Duration_t service_cleanup_delay;
	HistoryQosPolicyKind history_kind;
	int32_t history_depth;
	int32_t max_samples;
	int32_t max_instances;
	int32_t max_samples_per_instance;

	DurabilityServiceQosPolicy() :
		service_cleanup_delay(TIME_ZERO),
		history_kind(KEEP_LAST_HISTORY_QOS),
		history_depth(1),
		max_samples(LENGTH_UNLIMITED),
		max_instances(LENGTH_UNLIMITED),
		max_samples_per_instance(LENGTH_UNLIMITED) {

	}
};

struct DomainParticipantFactoryQos {
	EntityFactoryQosPolicy entity_factory;

	DomainParticipantFactoryQos() {
	}
};

struct DomainParticipantQos {
	UserDataQosPolicy user_data;
	EntityFactoryQosPolicy entity_factory;

	DomainParticipantQos() {
	}
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

	TopicQos() {
	}
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

	DataWriterQos() {
		reliability.kind = RELIABLE_RELIABILITY_QOS;
	}
};

struct PublisherQos {
	PresentationQosPolicy   presentation;
	PartitionQosPolicy      partition;
	GroupDataQosPolicy      group_data;
	EntityFactoryQosPolicy  entity_factory;

	PublisherQos() {
	}
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

	DataReaderQos() {
	}
};

struct SubscriberQos {
	PresentationQosPolicy       presentation;
	PartitionQosPolicy          partition;
	GroupDataQosPolicy          group_data;
	EntityFactoryQosPolicy      entity_factory;

	SubscriberQos() {
	}
};

DDS_DLL extern const DomainParticipantQos PARTICIPANT_QOS_DEFAULT;
DDS_DLL extern const TopicQos TOPIC_QOS_DEFAULT;
DDS_DLL extern const PublisherQos PUBLISHER_QOS_DEFAULT;
DDS_DLL extern const SubscriberQos SUBSCRIBER_QOS_DEFAULT;
DDS_DLL extern const DataWriterQos DATAWRITER_QOS_DEFAULT;
DDS_DLL extern const DataReaderQos DATAREADER_QOS_DEFAULT;

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_QOS_QOS_H
