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
#include <set>
#include "BasicType.h"
#include "RetCode.h"
#include "Status.h"
#include "Listener/DomainParticipantListener.h"
#include "Listener/DataWriterListener.h"

//#include "DDS/DataWriter.h"
//#include "DDS/DataReader.h"
//#include "DDS/Publisher.h"
//#include "DDS/Subscriber.h"
//#include "DDS/DomainParticipant.h"
//#include "DDS/Topic.h"

AGIDDS_NAMESPACE_BEGIN



class DataWriterImpl;
class PublisherImpl;
class DataReaderImpl;
class SubscriberImpl;
class DomainParticipantImpl;


class DataWriterStatusManager {
public:
	DataWriterStatusManager(DataWriterImpl* the_writer);
	~DataWriterStatusManager();

	/* yuguxu    更新LIVELINESS_LOST通信状态*/
	ReturnCode_t update_liveliness_lost_status();
	
	/* yuguxu    更新Publication_matched的通信状态 */
	ReturnCode_t update_publication_matched_status(const InstanceHandle_t& handle, const bool& add);

	/* yuguxu    更新OFFERED_DEADLINE_MISSED通信状态 */
	ReturnCode_t update_offered_deadline_missed_status(const InstanceHandle_t& handle);

	/* yuguxu    更新OFFERED_INCOMPATIBLE_QOS通信状态 */
	ReturnCode_t update_offered_incompatible_qos_status(std::set<QosPolicyId_t>& policyId);

	/* yuguxu    访问LIVELINESS_LOST通信状态*/
	ReturnCode_t get_liveliness_lost_status(LivelinessLostStatus& status);

	/* yuguxu    访问OFFERED_DEADLINE_MISSED通信状态 */
	ReturnCode_t get_offered_deadline_missed_status(OfferedDeadlineMissedStatus& status);

	/* yuguxu    访问OFFERED_INCOMPATIBLE_QOS通信状态 */
	ReturnCode_t get_offered_incompatible_qos_status(OfferedIncompatibleQosStatus& status);

	/* yuguxu    访问Publication_matched的通信状态 */
	ReturnCode_t get_publication_matched_status(PublicationMatchedStatus& status);

	/* yuguxu    设置OFFERED_INCOMPATIBLE_QOS通信状态 */
	ReturnCode_t set_offered_incompatible_qos_status(const OfferedIncompatibleQosStatus& status);

private:
	DataWriterImpl* writer_;
	PublisherImpl* pub_;
	DomainParticipantImpl* participant_;

	OfferedDeadlineMissedStatus offeredDeadlineMissedStatus_ = {0};
	LivelinessLostStatus livelinessLostStatus_ = {0};
	PublicationMatchedStatus publicationMatchedStatus_ = {0};
	OfferedIncompatibleQosStatus offeredIncompatibleQosStatus_ = {0};

	std::mutex mtx_;
};


class DataReaderStatusManager {
public:
	DataReaderStatusManager(DataReaderImpl* the_reader);
	~DataReaderStatusManager();

	/* yuguxu    更新SUBSCRIPTION_MATCHED通信状态 */
	ReturnCode_t update_subscription_matched_status(const InstanceHandle_t& handle, const bool& add);

	/* yuguxu   更新REQUESTED_DEADLINE_MISSED通信状态 */
	ReturnCode_t update_requested_deadline_missed_status(const InstanceHandle_t& handle);

	/* yuguxu   更新LIVELINESS_CHANGED通信状态 
	 * bool ensure : 明确节点失活（通知了），或者先前明确失活了，现又成功活了为  true，否则 false
	 */
	ReturnCode_t update_liveliness_changed_status(const InstanceHandle_t& stHandle, const bool& alive,const bool& ensure = true);

	/* yuguxu    更新SAMPLE_LOST通信状态 */
	ReturnCode_t update_sample_lost_status(int ilost_count);

	/* yuguxu    更新REQUESTED_INCOMPATIBLE_QOS通信状态 */
	ReturnCode_t updata_requested_incompatible_qos_status(std::set<QosPolicyId_t>& policyId);

	/* yuguxu    访问SAMPLE_REJECTED通信状态 */
	ReturnCode_t update_sample_rejected_status(const InstanceHandle_t& stHandle,const SampleRejectedStatusKind& reason);

	/* yuguxu   访问LIVELINESS_CHANGED通信状态 */
	ReturnCode_t get_liveliness_changed_status(LivelinessChangedStatus& status);

	/* yuguxu    访问REQUESTED_DEADLINE_MISSED通信状态 */
	ReturnCode_t get_requested_deadline_missed_status(RequestedDeadlineMissedStatus& status);

	/* yuguxu    访问REQUESTED_INCOMPATIBLE_QOS通信状态 */
	ReturnCode_t get_requested_incompatible_qos_status(RequestedIncompatibleQosStatus& status);

	/* yuguxu    访问SAMPLE_LOST通信状态 */
	ReturnCode_t get_sample_lost_status(SampleLostStatus& status);

	/* yuguxu    访问SAMPLE_REJECTED通信状态 */
	ReturnCode_t get_sample_rejected_status(SampleRejectedStatus& status);

	/* yuguxu    访问SUBSCRIPTION_MATCHED通信状态 */
	ReturnCode_t get_subscription_matched_status(SubscriptionMatchedStatus& status);

private:
	DataReaderImpl* reader_;
	SubscriberImpl* sub_;
	DomainParticipantImpl* participant_;

	RequestedDeadlineMissedStatus reqDeadlineMissedStatus_ = { 0 };
	RequestedIncompatibleQosStatus reqIncompatibleQosStatus_ = { 0 };
	SampleRejectedStatus sampleRejectedStatus_ = { 0 };
	LivelinessChangedStatus livelinessChangedStatus = { 0 };
	SubscriptionMatchedStatus subscriptionMatchedStatus_ = { 0 };
	SampleLostStatus sampleLostStatus_ = { 0 };

	std::mutex mtx_;
};


AGIDDS_NAMESPACE_END