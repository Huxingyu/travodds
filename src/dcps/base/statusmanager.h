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
#ifndef AGIDDS_DCPS_BASE_STATUSMANAGER_H
#define AGIDDS_DCPS_BASE_STATUSMANAGER_H

#include "type/basictype.h"
#include "dcps/base/retcode.h"
#include "dcps/base/status.h"
#include "dcps/listener/domainparticipantlistener.h"
#include "dcps/listener/datawriterlistener.h"

AGIDDS_NAMESPACE_BEGIN

class DataWriterImpl;
class PublisherImpl;
class DataReaderImpl;
class SubscriberImpl;
class DomainParticipantImpl;
class TopicImpl;

class TopicStatusManager{
public:

	TopicStatusManager(TopicImpl* topic);

	ReturnCode_t GetInconsistentTopicStatus(InconsistentTopicStatus& status);

	void UpdateInconsistentTopicStatus();

private:
	InconsistentTopicStatus status_;
	TopicImpl* topic_;
};

class DataWriterStatusManager {
public:
	DataWriterStatusManager(DataWriterImpl* writer);
	~DataWriterStatusManager();

	/* yuguxu    更新LIVELINESS_LOST通信状态 */
	ReturnCode_t UpdateLivelinessLostStatus();
	
	/* yuguxu    更新Publication_matched的通信状态 */
	ReturnCode_t UpdatePublicationMatchedStatus(const InstanceHandle_t& handle, const bool& add);

	/* yuguxu    更新OFFERED_DEADLINE_MISSED通信状态 */
	ReturnCode_t UpdateOfferedDeadlineMissedStatus();

	/* yuguxu    更新OFFERED_INCOMPATIBLE_QOS通信状态 */
	ReturnCode_t UpdateOfferedIncompatibleQosStatus(std::set<QosPolicyId_t>& policyId);

	/* yuguxu    访问LIVELINESS_LOST通信状态 */
	ReturnCode_t GetLivelinessLostStatus(LivelinessLostStatus& status);

	/* yuguxu    访问OFFERED_DEADLINE_MISSED通信状态 */
	ReturnCode_t GetOfferedDeadlineMissedStatus(OfferedDeadlineMissedStatus& status);

	/* yuguxu    访问OFFERED_INCOMPATIBLE_QOS通信状态 */
	ReturnCode_t GetOfferedIncompatibleQosStatus(OfferedIncompatibleQosStatus& status);

	/* yuguxu    访问Publication_matched的通信状态 */
	ReturnCode_t GetPublicationMatchedStatus(PublicationMatchedStatus& status);

	/* yuguxu    设置OFFERED_INCOMPATIBLE_QOS通信状态 */
	ReturnCode_t SetOfferedIncompatibleQosStatus(const OfferedIncompatibleQosStatus& status);

private:
	DataWriterImpl* writer_;
	PublisherImpl* pub_;
	DomainParticipantImpl* participant_;

	OfferedDeadlineMissedStatus offeredDeadlineMissedStatus_;
	LivelinessLostStatus livelinessLostStatus_;
	PublicationMatchedStatus publicationMatchedStatus_;
	OfferedIncompatibleQosStatus offeredIncompatibleQosStatus_ = {0};

	std::mutex mtx_;
};

class DataReaderStatusManager {
public:
	DataReaderStatusManager(DataReaderImpl* reader);
	~DataReaderStatusManager();

	/* yuguxu    更新SUBSCRIPTION_MATCHED通信状态 */
	ReturnCode_t UpdateSubscriptionMatchedStatus(const InstanceHandle_t& handle, const bool& add);

	/* yuguxu   更新REQUESTED_DEADLINE_MISSED通信状态 */
	ReturnCode_t UpdateRequestedDeadlineMissedStatus();

	/* yuguxu   更新LIVELINESS_CHANGED通信状态 
	 * bool ensure : 明确节点失活（通知了），或者先前明确失活了，现又成功活了为  true，否则 false
	 */
	ReturnCode_t UpdateLivelinessChangedStatus(const InstanceHandle_t& handle, const bool& alive, const bool& ensure = true);

	/* yuguxu    更新SAMPLE_LOST通信状态 */
	ReturnCode_t UpdateSampleLostStatus(int32_t ilost_count);

	/* yuguxu    更新REQUESTED_INCOMPATIBLE_QOS通信状态 */
	ReturnCode_t UpdateRequestedIncompatibleQosStatus(std::set<QosPolicyId_t>& policyId);

	/* yuguxu    访问SAMPLE_REJECTED通信状态 */
	ReturnCode_t UpdateSampleRejectedStatus(const InstanceHandle_t& handle,const SampleRejectedStatusKind& reason);

	/* yuguxu   访问LIVELINESS_CHANGED通信状态 */
	ReturnCode_t GetLivelinessChangedStatus(LivelinessChangedStatus& status);

	/* yuguxu    访问REQUESTED_DEADLINE_MISSED通信状态 */
	ReturnCode_t GetRequestedDeadlineMissedStatus(RequestedDeadlineMissedStatus& status);

	/* yuguxu    访问REQUESTED_INCOMPATIBLE_QOS通信状态 */
	ReturnCode_t GetRequestedIncompatibleQosStatus(RequestedIncompatibleQosStatus& status);

	/* yuguxu    访问SAMPLE_LOST通信状态 */
	ReturnCode_t GetSampleLostStatus(SampleLostStatus& status);

	/* yuguxu    访问SAMPLE_REJECTED通信状态 */
	ReturnCode_t GetSampleRejectedStatus(SampleRejectedStatus& status);

	/* yuguxu    访问SUBSCRIPTION_MATCHED通信状态 */
	ReturnCode_t GetSubscriptionMatchedStatus(SubscriptionMatchedStatus& status);

private:
	DataReaderImpl* reader_;
	SubscriberImpl* sub_;
	DomainParticipantImpl* participant_;

	RequestedDeadlineMissedStatus reqDeadlineMissedStatus_;
	RequestedIncompatibleQosStatus reqIncompatibleQosStatus_;
	SampleRejectedStatus sampleRejectedStatus_;
	LivelinessChangedStatus livelinessChangedStatus;
	SubscriptionMatchedStatus subscriptionMatchedStatus_;
	SampleLostStatus sampleLostStatus_;

	std::mutex mtx_;
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_BASE_STATUSMANAGER_H
