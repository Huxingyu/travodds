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

#include "BasicType.h"

AGIDDS_NAMESPACE_BEGIN
// ----------------------------------------------------------------------
// Status to support listeners and conditions
// ----------------------------------------------------------------------
using StatusKind = unsigned long;
using StatusMask = unsigned long;	 // bit-mask StatusKind
const  StatusKind  INCONSISTENT_TOPIC_STATUS = 0x0001 << 0;
const  StatusKind  OFFERED_DEADLINE_MISSED_STATUS = 0x0001 << 1;
const  StatusKind  REQUESTED_DEADLINE_MISSED_STATUS = 0x0001 << 2;
const  StatusKind  OFFERED_INCOMPATIBLE_QOS_STATUS = 0x0001 << 5;
const  StatusKind  REQUESTED_INCOMPATIBLE_QOS_STATUS = 0x0001 << 6;
const  StatusKind  SAMPLE_LOST_STATUS = 0x0001 << 7;
const  StatusKind  SAMPLE_REJECTED_STATUS = 0x0001 << 8;
const  StatusKind  DATA_ON_READERS_STATUS = 0x0001 << 9;
const  StatusKind  DATA_AVAILABLE_STATUS = 0x0001 << 10;
const  StatusKind  LIVELINESS_LOST_STATUS = 0x0001 << 11;
const  StatusKind  LIVELINESS_CHANGED_STATUS = 0x0001 << 12;
const  StatusKind  PUBLICATION_MATCHED_STATUS = 0x0001 << 13;
const  StatusKind  SUBSCRIPTION_MATCHED_STATUS = 0x0001 << 14;

using QosPolicyId_t = long ;

#define STATUS_MASK_NONE                                0x00000000
#define STATUS_MASK_ALL                                 0xFFFFFFFF

struct InconsistentTopicStatus {
	long		total_count;
	long		total_count_change;
};

struct SampleLostStatus {
	long		total_count;
	long		total_count_change;
};

enum SampleRejectedStatusKind {
	NOT_REJECTED,
	REJECTED_BY_INSTANCES_LIMIT,
	REJECTED_BY_SAMPLES_LIMIT,
	REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT
};

struct SampleRejectedStatus {
	long						total_count;
	long						total_count_change;
	SampleRejectedStatusKind	last_reason;
	InstanceHandle_t			last_instance_handle;
};

struct LivelinessLostStatus {
	long		total_count;
	long		total_count_change;
};

struct LivelinessChangedStatus {
	long				alive_count;
	long				not_alive_count;
	long				alive_count_change;
	long				not_alive_count_change;
	InstanceHandle_t	last_publication_handle;
};


struct OfferedDeadlineMissedStatus {
	long				total_count;
	long				total_count_change;
	InstanceHandle_t	last_instance_handle;
};

struct RequestedDeadlineMissedStatus {
	long				total_count;
	long				total_count_change;
	InstanceHandle_t	last_instance_handle;
};

struct QosPolicyCount {
	QosPolicyId_t 	policy_id;
	long				count;
};

using QosPolicyCountSeq = Sequence<QosPolicyCount>;

struct OfferedIncompatibleQosStatus {
	long					total_count;
	long					total_count_change;
	QosPolicyId_t			last_policy_id;
	QosPolicyCountSeq	policies;
};

struct RequestedIncompatibleQosStatus {
	long					total_count;
	long					total_count_change;
	QosPolicyId_t			last_policy_id;
	QosPolicyCountSeq	policies;
};

struct PublicationMatchedStatus {
	long				total_count;
	long				total_count_change;
	long				current_count;
	long				current_count_change;
	InstanceHandle_t	last_subscription_handle;
};

struct SubscriptionMatchedStatus {
	long				total_count;
	long				total_count_change;
	long				current_count;
	long				current_count_change;
	InstanceHandle_t	last_publication_handle;
};

AGIDDS_NAMESPACE_END