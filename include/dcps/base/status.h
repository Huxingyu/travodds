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
#ifndef AGIDDS_DCPS_BASE_STATUS_H
#define AGIDDS_DCPS_BASE_STATUS_H

#include "type/basictype.h"

AGIDDS_NAMESPACE_BEGIN
// ----------------------------------------------------------------------
// Status to support listeners and conditions
// ----------------------------------------------------------------------
using StatusKind = uint32_t;
using StatusMask = uint32_t;	 // bit-mask StatusKind
const StatusKind  INCONSISTENT_TOPIC_STATUS = 0x0001 << 0;
const StatusKind  OFFERED_DEADLINE_MISSED_STATUS = 0x0001 << 1;
const StatusKind  REQUESTED_DEADLINE_MISSED_STATUS = 0x0001 << 2;
const StatusKind  OFFERED_INCOMPATIBLE_QOS_STATUS = 0x0001 << 5;
const StatusKind  REQUESTED_INCOMPATIBLE_QOS_STATUS = 0x0001 << 6;
const StatusKind  SAMPLE_LOST_STATUS = 0x0001 << 7;
const StatusKind  SAMPLE_REJECTED_STATUS = 0x0001 << 8;
const StatusKind  DATA_ON_READERS_STATUS = 0x0001 << 9;
const StatusKind  DATA_AVAILABLE_STATUS = 0x0001 << 10;
const StatusKind  LIVELINESS_LOST_STATUS = 0x0001 << 11;
const StatusKind  LIVELINESS_CHANGED_STATUS = 0x0001 << 12;
const StatusKind  PUBLICATION_MATCHED_STATUS = 0x0001 << 13;
const StatusKind  SUBSCRIPTION_MATCHED_STATUS = 0x0001 << 14;

using QosPolicyId_t = int32_t ;

#define STATUS_MASK_NONE                                0x00000000
#define STATUS_MASK_ALL                                 0xFFFFFFFF

struct InconsistentTopicStatus {
	int32_t		total_count;
	int32_t		total_count_change;

	InconsistentTopicStatus() :total_count(0), total_count_change(0) {

	}
};

struct SampleLostStatus {
	int32_t		total_count;
	int32_t		total_count_change;

	SampleLostStatus() :total_count(0), total_count_change(0) {

	}

};

enum SampleRejectedStatusKind {
	NOT_REJECTED,
	REJECTED_BY_INSTANCES_LIMIT,
	REJECTED_BY_SAMPLES_LIMIT,
	REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT
};

struct SampleRejectedStatus {
	int32_t						total_count;
	int32_t						total_count_change;
	SampleRejectedStatusKind	last_reason;
	InstanceHandle_t			last_instance_handle;

	SampleRejectedStatus() :total_count(0), total_count_change(0), last_reason(NOT_REJECTED){

	}
};

struct LivelinessLostStatus {
	int32_t		total_count;
	int32_t		total_count_change;

	LivelinessLostStatus() :total_count(0), total_count_change(0) {

	}
};

struct LivelinessChangedStatus {
	int32_t				alive_count;
	int32_t				not_alive_count;
	int32_t				alive_count_change;
	int32_t				not_alive_count_change;
	InstanceHandle_t	last_publication_handle;

	LivelinessChangedStatus() :alive_count(0), not_alive_count(0), alive_count_change(0), not_alive_count_change(0) {

	}
};


struct OfferedDeadlineMissedStatus {
	int32_t				total_count;
	int32_t				total_count_change;
	InstanceHandle_t	last_instance_handle;

	OfferedDeadlineMissedStatus() :total_count(0), total_count_change(0) {

	}
};

struct RequestedDeadlineMissedStatus {
	int32_t				total_count;
	int32_t				total_count_change;
	InstanceHandle_t	last_instance_handle;

	RequestedDeadlineMissedStatus() :total_count(0), total_count_change(0) {

	}
};

struct QosPolicyCount {
	QosPolicyId_t 	policy_id;
	int32_t				count;
};

using QosPolicyCountSeq = Sequence<QosPolicyCount>;

struct OfferedIncompatibleQosStatus {
	int32_t					total_count;
	int32_t					total_count_change;
	QosPolicyId_t			last_policy_id;
	QosPolicyCountSeq	policies;

	OfferedIncompatibleQosStatus() :total_count(0), total_count_change(0), last_policy_id(0) {

	}
};

struct RequestedIncompatibleQosStatus {
	int32_t					total_count;
	int32_t					total_count_change;
	QosPolicyId_t			last_policy_id;
	QosPolicyCountSeq	policies;

	RequestedIncompatibleQosStatus() :total_count(0), total_count_change(0),last_policy_id(0) {

	}
};

struct PublicationMatchedStatus {
	int32_t				total_count;
	int32_t				total_count_change;
	int32_t				current_count;
	int32_t				current_count_change;
	InstanceHandle_t	last_subscription_handle;

	PublicationMatchedStatus() :total_count(0), total_count_change(0), current_count(0), current_count_change(0){

	}
};

struct SubscriptionMatchedStatus {
	int32_t				total_count;
	int32_t				total_count_change;
	int32_t				current_count;
	int32_t				current_count_change;
	InstanceHandle_t	last_publication_handle;

	SubscriptionMatchedStatus() :total_count(0), total_count_change(0), current_count(0), current_count_change(0) {

	}
};

AGIDDS_NAMESPACE_END
#endif // ! AGIDDS_DCPS_BASE_STATUS_H
