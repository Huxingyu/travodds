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
#ifndef AGIDDS_QOS_QOSPOLICYCHECK_H
#define AGIDDS_QOS_QOSPOLICYCHECK_H

#include "dcps/base/retcode.h"
#include "qos/qos.h"

AGIDDS_NAMESPACE_BEGIN

class QosPolicyCheck {
public:
	/* yuguxu  检查 QoS规范 */
	static ReturnCode_t CheckQos(const DomainParticipantFactoryQos& qos);

	static ReturnCode_t CheckQos(const DomainParticipantQos& qos);

	static ReturnCode_t CheckQos(const PublisherQos& qos);

	static ReturnCode_t CheckQos(const SubscriberQos& qos);

	static ReturnCode_t CheckQos(const TopicQos& qos);

	static ReturnCode_t CheckQos(const DataWriterQos& qos);

	static ReturnCode_t CheckQos(const DataReaderQos& qos);

	/* yuguxu 检查具体QoS策略 */
	static ReturnCode_t CheckHistoryAndResourcueLimits(const HistoryQosPolicy& historyQos, const ResourceLimitsQosPolicy& resourceLimitsQos);

	static ReturnCode_t CheckDeadlineAndTimeBasedFilter(const DeadlineQosPolicy& deadline, const TimeBasedFilterQosPolicy& timeBasedFilter);

	/* yuguxu 检查 QoS 是否可改 */
	static bool CanQosBeUpdated(const DomainParticipantFactoryQos& curQos, const DomainParticipantFactoryQos& tagQos);

	static bool CanQosBeUpdated(const DomainParticipantQos& curQos, const DomainParticipantQos& tagQos);

	static bool CanQosBeUpdated(const PublisherQos& curQos, const PublisherQos& tagQos);

	static bool CanQosBeUpdated(const SubscriberQos& curQos, const SubscriberQos& tagQos);

	static bool CanQosBeUpdated(const TopicQos& curQos, const TopicQos& tagQos);

	static bool CanQosBeUpdated(const DataWriterQos& curQos, const DataWriterQos& tagQos);

	static bool CanQosBeUpdated(const DataReaderQos& curQos, const DataReaderQos& tagQos);


	/* yuguxu 具体QoS一致性检查 */
	static bool ConsistebcyCheckDurabilityQos(const DurabilityQosPolicy& qosA, const DurabilityQosPolicy& qosB);

	static bool ConsistebcyCheckDurabilityServiceQos(const DurabilityServiceQosPolicy& qosA, const DurabilityServiceQosPolicy& qosB);
	
	static bool ConsistebcyCheckOwnershipQos(const OwnershipQosPolicy& qosA, const OwnershipQosPolicy& qosB);

	static bool ConsistebcyCheckLivelinessQos(const LivelinessQosPolicy& qosA, const LivelinessQosPolicy& qosB);
	
	static bool ConsistebcyCheckReliabilityQos(const ReliabilityQosPolicy& qosA, const ReliabilityQosPolicy& qosB);

	static bool ConsistebcyCheckDestinationOrderQos(const DestinationOrderQosPolicy& qosA, const DestinationOrderQosPolicy& qosB);

	static bool ConsistebcyCheckHistoryQos(const HistoryQosPolicy& qosA, const HistoryQosPolicy& qosB);

	static bool ConsistebcyCheckResourceLimitsQos(const ResourceLimitsQosPolicy& qosA, const ResourceLimitsQosPolicy& qosB);

	static bool ConsistebcyCheckPresentationQos(const PresentationQosPolicy& qosA, const PresentationQosPolicy& qosB);

	static bool ConsistebcyCheckDeadlineQos(const DeadlineQosPolicy& qosA, const DeadlineQosPolicy& qosB);

	static bool ConsistebcyCheckTimeBasedFilterQos(const TimeBasedFilterQosPolicy& qosA, const TimeBasedFilterQosPolicy& qosB);

	/* yuguxu Datawriter与Datareader Qos匹配规则检查 */
	static void CheckQosMatchRule(const DataWriterQos& dwQos, const DataReaderQos& drQos, std::set<QosPolicyId_t>& qosPolicyId);

};
AGIDDS_NAMESPACE_END

#endif // !AGIDDS_QOS_QOSPOLICYCHECK_H
