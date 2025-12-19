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
#ifndef TRAVODDS_DCPS_BASE_STATUSMANAGER_H
#define TRAVODDS_DCPS_BASE_STATUSMANAGER_H

#include "type/basictype.h"
#include "dcps/base/retcode.h"
#include "dcps/base/status.h"
#include "dcps/listener/domainparticipantlistener.h"
#include "dcps/listener/datawriterlistener.h"

TRAVODDS_NAMESPACE_BEGIN

class DataWriterImpl;
class PublisherImpl;
class DataReaderImpl;
class SubscriberImpl;
class DomainParticipantImpl;
class TopicImpl;
class StatusConditionImpl;
/**
 * @brief TopicStatusManager类，主题状态管理器。
 *
 */
class TopicStatusManager{
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] topic  TopicImpl指针。
	 */
	TopicStatusManager(TopicImpl* topic);

	/**
	 * @brief 获取通信主题不一致状态信息。
	 *
	 * @param [out] status 通信主题信息。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetInconsistentTopicStatus(InconsistentTopicStatus& status);

	/**
	 * @brief 更新不一致的主题状态信息。
	 */
	void UpdateInconsistentTopicStatus();

private:
	InconsistentTopicStatus status_;
	TopicImpl* topic_;
	std::mutex mtx_;
};
/**
 * @brief DataWriterStatusManager类，数据写者状态管理器。
 *
 */
class DataWriterStatusManager {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] writer  DataWriterImpl指针。
	 */
	DataWriterStatusManager(DataWriterImpl* writer);

	/**
	 * @brief 析构函数。
	 */
	~DataWriterStatusManager();

	/**
	 * @brief 更新LIVELINESS_LOST通信状态。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t UpdateLivelinessLostStatus();
	
	/**
	 * @brief 更新Publication_matched的通信状态。
	 *
	 * @param [in] handle 匹配状态变更的远端订阅者实例句柄。
	 * @param [in] add	  新增远端订阅者标志。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t UpdatePublicationMatchedStatus(const InstanceHandle_t& handle, const bool& add);

	/**
	 * @brief 更新OFFERED_DEADLINE_MISSED通信状态。
	 *
	 * @param [in] handle 状态更改的实例句柄。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t UpdateOfferedDeadlineMissedStatus(const InstanceHandle_t& handle);

	/**
	 * @brief 更新OFFERED_INCOMPATIBLE_QOS通信状态。
	 *
	 * @param [in] policyId 不匹配的Qos ID列表。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t UpdateOfferedIncompatibleQosStatus(std::set<QosPolicyId_t>& policyId);

	/**
	 * @brief 访问LIVELINESS_LOST通信状态。
	 *
	 * @param [out] status LIVELINESS_LOST通信状态信息。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetLivelinessLostStatus(LivelinessLostStatus& status);

	/**
	 * @brief  访问OFFERED_DEADLINE_MISSED通信状态。
	 *
	 * @param [out] status OFFERED_DEADLINE_MISSED通信状态信息。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetOfferedDeadlineMissedStatus(OfferedDeadlineMissedStatus& status);

	/**
	 * @brief 访问OFFERED_INCOMPATIBLE_QOS通信状态。
	 *
	 * @param [out] status OFFERED_INCOMPATIBLE_QOS通信状态信息。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetOfferedIncompatibleQosStatus(OfferedIncompatibleQosStatus& status);

	/**
	 * @brief 访问Publication_matched的通信状态。
	 *
	 * @param [out] status Publication_matched的通信状态。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetPublicationMatchedStatus(PublicationMatchedStatus& status);

private:
	void SetStatus(const StatusMask& status, bool addFlag);

	DataWriterImpl* writer_;
	PublisherImpl* pub_;
	DomainParticipantImpl* participant_;
	StatusConditionImpl* writercondition_;
	StatusConditionImpl* pubcondition_;
	StatusConditionImpl* participantcondition_;

	OfferedDeadlineMissedStatus offeredDeadlineMissedStatus_;
	LivelinessLostStatus livelinessLostStatus_;
	PublicationMatchedStatus publicationMatchedStatus_;
	OfferedIncompatibleQosStatus offeredIncompatibleQosStatus_;

	std::mutex mtx_;
};
/**
 * @brief DataReaderStatusManager类，数据读者状态管理器。
 *
 */
class DataReaderStatusManager {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] reader  DataReaderImpl指针。
	 */
	DataReaderStatusManager(DataReaderImpl* reader);
	/**
	 * @brief 析构函数。
	 */
	~DataReaderStatusManager();

	/**
	 * @brief 更新SUBSCRIPTION_MATCHED通信状态。
	 *
	 * @param [in] handle 匹配状态变更的远端发布者实例句柄。
	 * @param [in] add    新增远端发布者匹配标志。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t UpdateSubscriptionMatchedStatus(const InstanceHandle_t& handle, const bool& add);

	/**
	 * @brief  更新REQUESTED_DEADLINE_MISSED通信状态。
	 *
	 * @param [in] handle 通信状态更改的实例句柄。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t UpdateRequestedDeadlineMissedStatus(const InstanceHandle_t& handle);

	/**
	 * @brief  更新LIVELINESS_CHANGED通信状态 。
	 *
	 * ensure : 明确节点失活（通知了），或者先前明确失活了，现又成功活了为  true，否则 false
	 * 
	 * @param [in] handle 通信状态更新发布者句柄。
	 * @param [in] alive  失活标志。
	 * @param [in] ensure 明确失活标志。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t UpdateLivelinessChangedStatus(const InstanceHandle_t& handle, const bool& alive, const bool& ensure = true);

	/* yuguxu    更新SAMPLE_LOST通信状态 */	
	/**
	 * @brief 更新SAMPLE_LOST通信状态。
	 *
	 * @param [in] ilost_count 丢失样本数。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t UpdateSampleLostStatus(int32_t ilost_count);

	/**
	 * @brief 更新REQUESTED_INCOMPATIBLE_QOS通信状态。
	 *
	 * @param [in] policyId 不匹配的Qos ID列表。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t UpdateRequestedIncompatibleQosStatus(std::set<QosPolicyId_t>& policyId);

	/* yuguxu    访问SAMPLE_REJECTED通信状态 */	
	/**
	 * @brief 访问SAMPLE_REJECTED通信状态。
	 *
	 * @param [in] handle 被拒绝样本实例句柄。
	 * @param [in] reason 被拒绝原因。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t UpdateSampleRejectedStatus(const InstanceHandle_t& handle,const SampleRejectedStatusKind& reason);

	/**
	 * @brief 访问LIVELINESS_CHANGED通信状态。
	 *
	 * @param [in] status LIVELINESS_CHANGED通信状态。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetLivelinessChangedStatus(LivelinessChangedStatus& status);

	/**
	 * @brief 访问REQUESTED_DEADLINE_MISSED通信状态。
	 *
	 * @param [out] status REQUESTED_DEADLINE_MISSED通信状态。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetRequestedDeadlineMissedStatus(RequestedDeadlineMissedStatus& status);

	/**
	 * @brief 访问REQUESTED_INCOMPATIBLE_QOS通信状态。
	 *
	 * @param [out] status REQUESTED_INCOMPATIBLE_QOS通信状态。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetRequestedIncompatibleQosStatus(RequestedIncompatibleQosStatus& status);

	/**
	 * @brief 访问SAMPLE_LOST通信状态。
	 *
	 * @param [out] status SAMPLE_LOST通信状态。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetSampleLostStatus(SampleLostStatus& status);

	/**
	 * @brief 访问SAMPLE_REJECTED通信状态。
	 *
	 * @param [out] status SAMPLE_REJECTED通信状态。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetSampleRejectedStatus(SampleRejectedStatus& status);

	/**
	 * @brief 访问SUBSCRIPTION_MATCHED通信状态 。
	 *
	 * @param [out] status SUBSCRIPTION_MATCHED通信状态 。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetSubscriptionMatchedStatus(SubscriptionMatchedStatus& status);

private:
	void SetStatus(const StatusMask& status, bool addFlag);

	DataReaderImpl* reader_;
	SubscriberImpl* sub_;
	DomainParticipantImpl* participant_;
	StatusConditionImpl* readercondition_;
	StatusConditionImpl* subcondition_;
	StatusConditionImpl* participantcondition_;

	RequestedDeadlineMissedStatus reqDeadlineMissedStatus_;
	RequestedIncompatibleQosStatus reqIncompatibleQosStatus_;
	SampleRejectedStatus sampleRejectedStatus_;
	LivelinessChangedStatus livelinessChangedStatus;
	SubscriptionMatchedStatus subscriptionMatchedStatus_;
	SampleLostStatus sampleLostStatus_;

	std::mutex mtx_;
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_BASE_STATUSMANAGER_H
