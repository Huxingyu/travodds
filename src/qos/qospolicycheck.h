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
#ifndef TRAVODDS_QOS_QOSPOLICYCHECK_H
#define TRAVODDS_QOS_QOSPOLICYCHECK_H

#include "dcps/base/retcode.h"
#include "qos/qos.h"

TRAVODDS_NAMESPACE_BEGIN

class QosPolicyCheck {
public:
	/**
	 * @brief 检查DomainParticipantFactoryQoS是否符合规范。
	 *
	 * @param [in] qos DomainParticipantFactory QoS。
	 *
	 * @return RETCODE_OK。
	 */
	static ReturnCode_t CheckQos(const DomainParticipantFactoryQos& qos);

	/**
	 * @brief 检查DomainParticipantQos是否符合规范。
	 *
	 * @param [in] qos DomainParticipant QoS。
	 *
	 * @return RETCODE_OK。
	 */
	static ReturnCode_t CheckQos(const DomainParticipantQos& qos);

	/**
	 * @brief 检查PublisherQos是否符合规范。
	 *
	 * @param [in] qos Publisher QoS。
	 *
	 * @return RETCODE_OK。
	 */
	static ReturnCode_t CheckQos(const PublisherQos& qos);

	/**
	 * @brief 检查SubscriberQos是否符合规范。
	 *
	 * @param [in] qos Subscriber QoS。
	 *
	 * @return RETCODE_OK。
	 */
	static ReturnCode_t CheckQos(const SubscriberQos& qos);

	/**
	 * @brief 检查TopicQos是否符合规范。
	 *
	 * @param [in] qos Topic QoS。
	 *
	 * @return 符合规范则返回RETCODE_OK，不符合则返回RETCODE_INCONSISTENT_POLICY。
	 */
	static ReturnCode_t CheckQos(const TopicQos& qos);

	/**
	 * @brief 检查DataWriterQos是否符合规范。
	 *
	 * @param [in] qos DataWriter QoS。
	 *
	 * @return 符合规范则返回RETCODE_OK，不符合则返回RETCODE_INCONSISTENT_POLICY。
	 */
	static ReturnCode_t CheckQos(const DataWriterQos& qos);

	/**
	 * @brief 检查DataReaderQos是否符合规范。
	 *
	 * @param [in] qos DataReader QoS。
	 *
	 * @return 符合规范则返回RETCODE_OK，不符合则返回RETCODE_INCONSISTENT_POLICY。
	 */
	static ReturnCode_t CheckQos(const DataReaderQos& qos);

	/**
	 * @brief 检查History Qos和ResourceLimits Qos是否符合规范。
	 *
	 * @param [in] historyQos        History Qos。
	 * @param [in] resourceLimitsQos ResourceLimits Qos。
	 *
	 * @return 符合规范则返回RETCODE_OK，不符合则返回RETCODE_INCONSISTENT_POLICY。
	 */
	static ReturnCode_t CheckHistoryAndResourcueLimits(const HistoryQosPolicy& historyQos, const ResourceLimitsQosPolicy& resourceLimitsQos);

	/**
	 * @brief 检查Deadline Qos和TimeBasedFilter Qos是否符合规范。
	 *
	 * @param [in] deadline        Deadline Qos。
	 * @param [in] timeBasedFilter TimeBasedFilter Qos。
	 *
	 * @return 符合规范则返回RETCODE_OK，不符合则返回RETCODE_INCONSISTENT_POLICY。
	 */
	static ReturnCode_t CheckDeadlineAndTimeBasedFilter(const DeadlineQosPolicy& deadline, const TimeBasedFilterQosPolicy& timeBasedFilter);

	/**
	 * @brief 检查DomainParticipantFactoryQos是否可更改。
	 *
	 * @param [in] curQos 当前使用的Qos。
	 * @param [in] tagQos 要更改的到的目标Qos。
	 *
	 * @return 可更改返回true，不可更改返回false。
	 */
	static bool CanQosBeUpdated(const DomainParticipantFactoryQos& curQos, const DomainParticipantFactoryQos& tagQos);

	/**
	 * @brief 检查DomainParticipantFactoryQos是否可更改。
	 *
	 * @param [in] curQos 当前使用的Qos。
	 * @param [in] tagQos 要更改的到的目标Qos。
	 *
	 * @return 可更改返回true，不可更改返回false。
	 */
	static bool CanQosBeUpdated(const DomainParticipantQos& curQos, const DomainParticipantQos& tagQos);

	/**
	 * @brief 检查PublisherQos是否可更改。
	 *
	 * @param [in] curQos 当前使用的Qos。
	 * @param [in] tagQos 要更改的到的目标Qos。
	 *
	 * @return 可更改返回true，不可更改返回false。
	 */
	static bool CanQosBeUpdated(const PublisherQos& curQos, const PublisherQos& tagQos);

	/**
	 * @brief 检查SubscriberQos是否可更改。
	 *
	 * @param [in] curQos 当前使用的Qos。
	 * @param [in] tagQos 要更改的到的目标Qos。
	 *
	 * @return 可更改返回true，不可更改返回false。
	 */
	static bool CanQosBeUpdated(const SubscriberQos& curQos, const SubscriberQos& tagQos);

	/**
	 * @brief 检查TopicQos是否可更改。
	 *
	 * @param [in] curQos 当前使用的Qos。
	 * @param [in] tagQos 要更改的到的目标Qos。
	 *
	 * @return 可更改返回true，不可更改返回false。
	 */
	static bool CanQosBeUpdated(const TopicQos& curQos, const TopicQos& tagQos);

	/**
	 * @brief 检查DataWriterQos是否可更改。
	 *
	 * @param [in] curQos 当前使用的Qos。
	 * @param [in] tagQos 要更改的到的目标Qos。
	 *
	 * @return 可更改返回true，不可更改返回false。
	 */
	static bool CanQosBeUpdated(const DataWriterQos& curQos, const DataWriterQos& tagQos);

	/**
	 * @brief 检查DataReaderQos是否可更改。
	 *
	 * @param [in] curQos 当前使用的Qos。
	 * @param [in] tagQos 要更改的到的目标Qos。
	 *
	 * @return 可更改返回true，不可更改返回false。
	 */
	static bool CanQosBeUpdated(const DataReaderQos& curQos, const DataReaderQos& tagQos);

	/**
	 * @brief 检查DurabilityQos配置一致性检查。
	 *
	 * @param [in] qosA DurabilityQos配置信息A。
	 * @param [in] qosB DurabilityQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckDurabilityQos(const DurabilityQosPolicy& qosA, const DurabilityQosPolicy& qosB);

	/**
	 * @brief 检查DurabilityServiceQos配置一致性检查。
	 *
	 * @param [in] qosA DurabilityServiceQos配置信息A。
	 * @param [in] qosB DurabilityServiceQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckDurabilityServiceQos(const DurabilityServiceQosPolicy& qosA, const DurabilityServiceQosPolicy& qosB);

	/**
	 * @brief 检查OwnershipQos配置一致性检查。
	 *
	 * @param [in] qosA OwnershipQos配置信息A。
	 * @param [in] qosB OwnershipQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckOwnershipQos(const OwnershipQosPolicy& qosA, const OwnershipQosPolicy& qosB);

	/**
	 * @brief 检查LivelinessQos配置一致性检查。
	 *
	 * @param [in] qosA LivelinessQos配置信息A。
	 * @param [in] qosB LivelinessQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckLivelinessQos(const LivelinessQosPolicy& qosA, const LivelinessQosPolicy& qosB);

	/**
	 * @brief 检查ReliabilityQos配置一致性检查。
	 *
	 * @param [in] qosA ReliabilityQos配置信息A。
	 * @param [in] qosB ReliabilityQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckReliabilityQos(const ReliabilityQosPolicy& qosA, const ReliabilityQosPolicy& qosB);

	/**
	 * @brief 检查DestinationOrderQos配置一致性检查。
	 *
	 * @param [in] qosA DestinationOrderQos配置信息A。
	 * @param [in] qosB DestinationOrderQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckDestinationOrderQos(const DestinationOrderQosPolicy& qosA, const DestinationOrderQosPolicy& qosB);

	/**
	 * @brief 检查HistoryQos配置一致性检查。
	 *
	 * @param [in] qosA HistoryQos配置信息A。
	 * @param [in] qosB HistoryQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckHistoryQos(const HistoryQosPolicy& qosA, const HistoryQosPolicy& qosB);

	/**
	 * @brief 检查ResourceLimitsQos配置一致性检查。
	 *
	 * @param [in] qosA ResourceLimitsQos配置信息A。
	 * @param [in] qosB ResourceLimitsQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckResourceLimitsQos(const ResourceLimitsQosPolicy& qosA, const ResourceLimitsQosPolicy& qosB);

	/**
	 * @brief 检查PresentationQos配置一致性检查。
	 *
	 * @param [in] qosA PresentationQos配置信息A。
	 * @param [in] qosB PresentationQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckPresentationQos(const PresentationQosPolicy& qosA, const PresentationQosPolicy& qosB);

	/**
	 * @brief 检查DeadlineQos配置一致性检查。
	 *
	 * @param [in] qosA DeadlineQos配置信息A。
	 * @param [in] qosB DeadlineQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckDeadlineQos(const DeadlineQosPolicy& qosA, const DeadlineQosPolicy& qosB);

	/**
	 * @brief 检查TimeBasedFilterQos配置一致性检查。
	 *
	 * @param [in] qosA TimeBasedFilterQos配置信息A。
	 * @param [in] qosB TimeBasedFilterQos配置信息B。
	 *
	 * @return 一致则返回true，不一致则返回false。
	 */
	static bool ConsistebcyCheckTimeBasedFilterQos(const TimeBasedFilterQosPolicy& qosA, const TimeBasedFilterQosPolicy& qosB);

	static bool IsDangerousPattern(const std::string& pattern);
};
TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_QOS_QOSPOLICYCHECK_H