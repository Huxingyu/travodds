#include "qos/qospolicycheck.h"
#include "common/log/logger.h" 
#include "rtps/endpoint/participant.h"
USING_TRAVODDS_NAMESPACE

/* 危险正则表达式检测，防止恶意输入！ */
bool QosPolicyCheck::IsDangerousPattern(const std::string& pattern)
{
	// 检测指数级复杂度的正则
	if (pattern.find("(a+)+") != std::string::npos ||
		pattern.find("(.*)*") != std::string::npos) {
		return true;
	}

	// 检测深层嵌套
	int depth = 0;
	for (char c : pattern) {
		if (c == '(') depth++;
		else if (c == ')') depth--;
		if (depth > 10) return true;
	}

	// 检测长重复模式
	if (pattern.find("{1000}") != std::string::npos) {
		return true;
	}

	return false;
}

ReturnCode_t QosPolicyCheck::CheckQos(const DomainParticipantFactoryQos& qos)
{
	/* yuguxu 所包含QoS无规范限制 */
	static_cast<void>(qos);
	return RETCODE_OK;
}

ReturnCode_t QosPolicyCheck::CheckQos(const DomainParticipantQos& qos)
{
	if (qos.user_data.value.size() > BYTE_ORDER_LENGTH_MAX) {
		TRAVODDS_LOG(LOG_ERROR, "Error user_data.value out of resources");
		return RETCODE_OUT_OF_RESOURCES;
	}
	if (qos.dynamic_adjust.score_threshold_factor < 0 || qos.dynamic_adjust.score_threshold_factor > 80) {
		TRAVODDS_LOG(LOG_ERROR, "Error dynamic_adjust.score_threshold_factor out of resources");
		return RETCODE_OUT_OF_RESOURCES;
	}
	return RETCODE_OK;
}

ReturnCode_t QosPolicyCheck::CheckQos(const PublisherQos& qos)
{
	if (qos.partition.name.size() > PARTITON_SIZE_MAX) {
		TRAVODDS_LOG(LOG_ERROR, "Error partition.name size out of resources, max length %d.", PARTITON_SIZE_MAX);
		return RETCODE_OUT_OF_RESOURCES;
	}
	for (const auto& it : qos.partition.name) {
		if (it.size() > BYTE_ORDER_LENGTH_MAX) {
			TRAVODDS_LOG(LOG_ERROR, "Error partition.name string out of resources, max length %d.", BYTE_ORDER_LENGTH_MAX);
			return RETCODE_OUT_OF_RESOURCES;
		}
		/* wangyi 添加对字符检测 */
		/* 禁止控制字符 */
		for (char c : it) {
			if (iscntrl(static_cast<unsigned char>(c))) {
				TRAVODDS_LOG(LOG_ERROR, "Error partition.name : The partition name contains control characters.");
				return RETCODE_OUT_OF_RESOURCES;
			}
		}

		/* 检查危险模式 */ 
		if (Participant::IsRegexPattern(it) && IsDangerousPattern(it)) {
			TRAVODDS_LOG(LOG_ERROR, "Error partition.name : Potential dangerous regular expression patterns.");
			return RETCODE_OUT_OF_RESOURCES;
		}
	}
	if (qos.group_data.value.size() > BYTE_ORDER_LENGTH_MAX) {
		TRAVODDS_LOG(LOG_ERROR, "Error group_data.value out of resources, max length %d.", BYTE_ORDER_LENGTH_MAX);
		return RETCODE_OUT_OF_RESOURCES;
	}
	return RETCODE_OK;
}

ReturnCode_t QosPolicyCheck::CheckQos(const SubscriberQos& qos)
{
	if (qos.partition.name.size() > PARTITON_SIZE_MAX) {
		TRAVODDS_LOG(LOG_ERROR, "Error partition.name size out of resources, max length %d.", PARTITON_SIZE_MAX);
		return RETCODE_OUT_OF_RESOURCES;
	}
	for (const auto& it : qos.partition.name) {
		if (it.size() > BYTE_ORDER_LENGTH_MAX) {
			TRAVODDS_LOG(LOG_ERROR, "Error partition.name string out of resources, max length %d.", BYTE_ORDER_LENGTH_MAX);
			return RETCODE_OUT_OF_RESOURCES;
		}
	}
	if (qos.group_data.value.size() > BYTE_ORDER_LENGTH_MAX) {
		TRAVODDS_LOG(LOG_ERROR, "Error group_data.value out of resources, max length %d.", BYTE_ORDER_LENGTH_MAX);
		return RETCODE_OUT_OF_RESOURCES;
	}
	return RETCODE_OK;
}

ReturnCode_t QosPolicyCheck::CheckQos(const TopicQos& qos)
{
	if (qos.topic_data.value.size() > BYTE_ORDER_LENGTH_MAX) {
		TRAVODDS_LOG(LOG_ERROR, "Error topic_data.value out of resources, max length %d.", BYTE_ORDER_LENGTH_MAX);
		return RETCODE_OUT_OF_RESOURCES;
	}

	ReturnCode_t ret = RETCODE_OK;
	/* HISTORY, RESOURCE_LIMITS */
	ret = CheckHistoryAndResourcueLimits(qos.history, qos.resource_limits);
	if (RETCODE_OK != ret) {
		return ret;
	}

	return RETCODE_OK;
}

ReturnCode_t QosPolicyCheck::CheckQos(const DataWriterQos& qos)
{
	if (!qos.data_sharing.valid()) return RETCODE_INCONSISTENT_POLICY;
	if (!qos.payload_pool.valid()) return RETCODE_INCONSISTENT_POLICY;
	if (qos.user_data.value.size() > BYTE_ORDER_LENGTH_MAX) {
		TRAVODDS_LOG(LOG_ERROR, "Error user_data.value out of resources, max length %d.", BYTE_ORDER_LENGTH_MAX);
		return RETCODE_OUT_OF_RESOURCES;
	}
	ReturnCode_t ret = RETCODE_OK;
	/* HISTORY, RESOURCE_LIMITS */
	ret = CheckHistoryAndResourcueLimits(qos.history, qos.resource_limits);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos History and ResourceLimits Err");
		return ret;
	}

	return RETCODE_OK;
}

ReturnCode_t QosPolicyCheck::CheckQos(const DataReaderQos& qos)
{
    auto validDelay = [](const Duration_t& d) {
        return d == Duration_t(TIME_INFINITE) || (d.sec >= 0 && d.nanosec < 1000000000u);
    };
    if (!validDelay(qos.reader_data_lifecycle.autopurge_nowriter_samples_delay) ||
        !validDelay(qos.reader_data_lifecycle.autopurge_disposed_samples_delay)) return RETCODE_INCONSISTENT_POLICY;
	if (!qos.data_sharing.valid()) return RETCODE_INCONSISTENT_POLICY;
	if (qos.user_data.value.size() > BYTE_ORDER_LENGTH_MAX) {
		TRAVODDS_LOG(LOG_ERROR, "Error user_data.value out of resources, max length %d.", BYTE_ORDER_LENGTH_MAX);
		return RETCODE_OUT_OF_RESOURCES;
	}
	ReturnCode_t ret = RETCODE_OK;
	/* HISTORY, RESOURCE_LIMITS */
	ret = CheckHistoryAndResourcueLimits(qos.history,qos.resource_limits);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos History and ResourceLimits Err");
		return ret;
	}
	/* DEADLINE, TIME_BASED_FILTER */
	ret = CheckDeadlineAndTimeBasedFilter(qos.deadline,qos.time_based_filter);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "CheckQos DeadLine and TimeBased Err");
		return ret;
	}
	return RETCODE_OK;
}

ReturnCode_t QosPolicyCheck::CheckHistoryAndResourcueLimits(const HistoryQosPolicy& historyQos, const ResourceLimitsQosPolicy& resourceLimitsQos)
{
	if (historyQos.kind == KEEP_LAST_HISTORY_QOS && historyQos.depth <= 0) {
		TRAVODDS_LOG(LOG_ERROR, "Error historyQos.kind == KEEP_LAST_HISTORY_QOS && historyQos.depth <= 0");
		return RETCODE_INCONSISTENT_POLICY;
	}
	/* history.kind == KEEP_LAST_HISTORY_QOS && history.depth > 0 */
	if (historyQos.kind == KEEP_LAST_HISTORY_QOS && 
	    historyQos.depth > resourceLimitsQos.max_samples_per_instance && 
		resourceLimitsQos.max_samples_per_instance != static_cast<int32_t>(LENGTH_UNLIMITED)) {
		TRAVODDS_LOG(LOG_ERROR, "historyQos.depth[%d] > resourceLimitsQos.max_samples_per_instance[%d] && resourceLimitsQos.max_samples_per_instance[%d] != static_cast<int32_t>(LENGTH_UNLIMITED)", historyQos.depth, resourceLimitsQos.max_samples_per_instance, resourceLimitsQos.max_samples_per_instance);
		return RETCODE_INCONSISTENT_POLICY;
	}
	if (resourceLimitsQos.max_samples > 0 &&
		((resourceLimitsQos.max_instances <= 0 || resourceLimitsQos.max_samples_per_instance <= 0) ||
		(resourceLimitsQos.max_samples < (resourceLimitsQos.max_instances * resourceLimitsQos.max_samples_per_instance)))) {
		TRAVODDS_LOG(LOG_ERROR, "resourceLimitsQos.max_samples[%d] > 0 && \
			((resourceLimitsQos.max_instances[%d] <= 0 || resourceLimitsQos.max_samples_per_instance[%d] <= 0) || \
			(resourceLimitsQos.max_samples[%d] < (resourceLimitsQos.max_instances[%d] * resourceLimitsQos.max_samples_per_instance[%d]))", resourceLimitsQos.max_samples, resourceLimitsQos.max_instances, resourceLimitsQos.max_samples_per_instance, resourceLimitsQos.max_samples,resourceLimitsQos.max_instances, resourceLimitsQos.max_samples_per_instance);
		return RETCODE_INCONSISTENT_POLICY;
	}
	return RETCODE_OK;
}

ReturnCode_t QosPolicyCheck::CheckDeadlineAndTimeBasedFilter(const DeadlineQosPolicy& deadline, const TimeBasedFilterQosPolicy& timeBasedFilter)
{
	/* 设置必须是“deadline period > = minimum_separation” */
	if (deadline.period < timeBasedFilter.minimum_separation) {
		TRAVODDS_LOG(LOG_ERROR, "Error deadline.period[%d.%d] < timeBasedFilter.minimum_separation[%d.%d]", deadline.period.sec, deadline.period.nanosec, timeBasedFilter.minimum_separation.sec, timeBasedFilter.minimum_separation.nanosec);
		return RETCODE_INCONSISTENT_POLICY;
	}
	return RETCODE_OK;
}


bool QosPolicyCheck::CanQosBeUpdated(const DomainParticipantFactoryQos& curQos, const DomainParticipantFactoryQos& tagQos)
{
	/* yuguxu 所包含QoS无不可更改 */
	static_cast<void>(curQos);
	static_cast<void>(tagQos);
	return true;
}

bool QosPolicyCheck::CanQosBeUpdated(const DomainParticipantQos& curQos, const DomainParticipantQos& tagQos)
{
	if (curQos.network_interface.interface_whitelist.size() != tagQos.network_interface.interface_whitelist.size()) {
		return false;
	}
	for (size_t i = 0; i < curQos.network_interface.interface_whitelist.size(); ++i) {
		if (curQos.network_interface.interface_whitelist[i] != tagQos.network_interface.interface_whitelist[i]) {
			return false;
		}
	}
	return (curQos.discovery.kind == tagQos.discovery.kind) &&
		(curQos.transport.kinds == tagQos.transport.kinds);
}

bool QosPolicyCheck::CanQosBeUpdated(const PublisherQos& curQos, const PublisherQos& tagQos)
{
	return ConsistebcyCheckPresentationQos(curQos.presentation,tagQos.presentation);
}

bool QosPolicyCheck::CanQosBeUpdated(const SubscriberQos& curQos, const SubscriberQos& tagQos)
{
	return ConsistebcyCheckPresentationQos(curQos.presentation, tagQos.presentation);
}

bool QosPolicyCheck::CanQosBeUpdated(const TopicQos& curQos, const TopicQos& tagQos)
{
	return ConsistebcyCheckDurabilityQos(curQos.durability, tagQos.durability) &&
		   ConsistebcyCheckDurabilityServiceQos(curQos.durability_service, tagQos.durability_service) &&
		   ConsistebcyCheckOwnershipQos(curQos.ownership, tagQos.ownership) &&
		   ConsistebcyCheckLivelinessQos(curQos.liveliness, tagQos.liveliness) &&
		   ConsistebcyCheckReliabilityQos(curQos.reliability, tagQos.reliability) &&
		   ConsistebcyCheckDestinationOrderQos(curQos.destination_order, tagQos.destination_order) &&
		   ConsistebcyCheckHistoryQos(curQos.history, tagQos.history) &&
		   ConsistebcyCheckResourceLimitsQos(curQos.resource_limits, tagQos.resource_limits);
}

bool QosPolicyCheck::CanQosBeUpdated(const DataWriterQos& curQos, const DataWriterQos& tagQos)
{
	return curQos.data_sharing == tagQos.data_sharing &&
		   curQos.payload_pool == tagQos.payload_pool &&
		   ConsistebcyCheckDurabilityQos(curQos.durability, tagQos.durability) &&
		   ConsistebcyCheckDurabilityServiceQos(curQos.durability_service, tagQos.durability_service) &&
		   ConsistebcyCheckOwnershipQos(curQos.ownership, tagQos.ownership) &&
		   ConsistebcyCheckLivelinessQos(curQos.liveliness, tagQos.liveliness) &&
		   ConsistebcyCheckReliabilityQos(curQos.reliability, tagQos.reliability) &&
		   ConsistebcyCheckDestinationOrderQos(curQos.destination_order, tagQos.destination_order) &&
		   ConsistebcyCheckHistoryQos(curQos.history, tagQos.history) &&
		   ConsistebcyCheckResourceLimitsQos(curQos.resource_limits, tagQos.resource_limits);
}

bool QosPolicyCheck::CanQosBeUpdated(const DataReaderQos& curQos, const DataReaderQos& tagQos)
{
	return curQos.data_sharing == tagQos.data_sharing &&
		   ConsistebcyCheckDurabilityQos(curQos.durability, tagQos.durability) &&
		   ConsistebcyCheckOwnershipQos(curQos.ownership, tagQos.ownership) &&
		   ConsistebcyCheckLivelinessQos(curQos.liveliness, tagQos.liveliness) &&
		   ConsistebcyCheckReliabilityQos(curQos.reliability, tagQos.reliability) &&
		   ConsistebcyCheckDestinationOrderQos(curQos.destination_order, tagQos.destination_order) &&
		   ConsistebcyCheckHistoryQos(curQos.history, tagQos.history) &&
		   ConsistebcyCheckResourceLimitsQos(curQos.resource_limits, tagQos.resource_limits);
}

bool QosPolicyCheck::ConsistebcyCheckDurabilityQos(const DurabilityQosPolicy& qosA, const DurabilityQosPolicy& qosB)
{
	return qosA.kind == qosB.kind;
}

bool QosPolicyCheck::ConsistebcyCheckDurabilityServiceQos(const DurabilityServiceQosPolicy& qosA, const DurabilityServiceQosPolicy& qosB)
{
	return (qosA.service_cleanup_delay == qosB.service_cleanup_delay) &&
	       (qosA.history_kind == qosB.history_kind) &&
	       (qosA.history_depth == qosB.history_depth) &&
	       (qosA.max_samples == qosB.max_samples) &&
	       (qosA.max_instances == qosB.max_instances) &&
	       (qosA.max_samples_per_instance == qosB.max_samples_per_instance);
}

bool QosPolicyCheck::ConsistebcyCheckOwnershipQos(const OwnershipQosPolicy& qosA, const OwnershipQosPolicy& qosB)
{
	return qosA.kind == qosB.kind;
}

bool QosPolicyCheck::ConsistebcyCheckLivelinessQos(const LivelinessQosPolicy& qosA, const LivelinessQosPolicy& qosB)
{
	return (qosA.kind == qosB.kind) &&
		   (qosA.lease_duration == qosB.lease_duration);
}

bool QosPolicyCheck::ConsistebcyCheckReliabilityQos(const ReliabilityQosPolicy& qosA, const ReliabilityQosPolicy& qosB)
{
	return (qosA.kind == qosB.kind) &&
		   (qosA.max_blocking_time == qosB.max_blocking_time);
}

bool QosPolicyCheck::ConsistebcyCheckDestinationOrderQos(const DestinationOrderQosPolicy& qosA, const DestinationOrderQosPolicy& qosB)
{
	return qosA.kind == qosB.kind;
}

bool QosPolicyCheck::ConsistebcyCheckHistoryQos(const HistoryQosPolicy& qosA, const HistoryQosPolicy& qosB)
{
	return (qosA.kind == qosB.kind) && (qosA.depth == qosB.depth);
}

bool QosPolicyCheck::ConsistebcyCheckResourceLimitsQos(const ResourceLimitsQosPolicy& qosA, const ResourceLimitsQosPolicy& qosB)
{
	return (qosA.max_samples == qosB.max_samples) &&
		   (qosA.max_instances == qosB.max_instances) &&
		   (qosA.max_samples_per_instance == qosB.max_samples_per_instance);
}

bool QosPolicyCheck::ConsistebcyCheckPresentationQos(const PresentationQosPolicy& qosA, const PresentationQosPolicy& qosB)
{
	return (qosA.access_scope == qosB.access_scope) &&
		   (qosA.coherent_access == qosB.coherent_access) &&
		   (qosA.ordered_access == qosB.ordered_access);
}

bool QosPolicyCheck::ConsistebcyCheckDeadlineQos(const DeadlineQosPolicy& qosA, const DeadlineQosPolicy& qosB)
{
	return qosA.period == qosB.period;
}

bool QosPolicyCheck::ConsistebcyCheckTimeBasedFilterQos(const TimeBasedFilterQosPolicy& qosA, const TimeBasedFilterQosPolicy& qosB)
{
	return qosA.minimum_separation == qosB.minimum_separation;
}

