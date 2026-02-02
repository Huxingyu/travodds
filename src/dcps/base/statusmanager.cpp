#include "dcps/base/statusmanager.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/publish/publisherimpl.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/condition/statusconditionimpl.h"

USING_TRAVODDS_NAMESPACE;

DataWriterStatusManager::DataWriterStatusManager(DataWriterImpl* writer)
	: writer_(writer),
	  pub_(dynamic_cast<PublisherImpl*>(const_cast<Publisher*>(writer_->get_publisher()))),
	  participant_(dynamic_cast<DomainParticipantImpl*>(const_cast<DomainParticipant*>(pub_->get_participant()))),
	  writercondition_(dynamic_cast<StatusConditionImpl*>(&writer->get_statuscondition())),
	  pubcondition_(dynamic_cast<StatusConditionImpl*>(&pub_->get_statuscondition())),
	  participantcondition_(dynamic_cast<StatusConditionImpl*>(&participant_->get_statuscondition()))
{
}

DataWriterStatusManager::~DataWriterStatusManager()
{
}

ReturnCode_t DataWriterStatusManager::UpdateLivelinessLostStatus()
{
	/* 非用户writer 不触发回调 */
	GUID_t guid = Conversion::ToGuid(writer_->get_instance_handle());
	if (ENTITYKIND_USER_WRITER_NO_KEY != guid.entityId.entityKind  && ENTITYKIND_USER_WRITER_WITH_KEY != guid.entityId.entityKind) {
		return  RETCODE_OK;
	}
	
	std::lock_guard<std::mutex> lock(mtx_);

	livelinessLostStatus_.total_count++;
	livelinessLostStatus_.total_count_change++;
	bool flag = false;
	if (LIVELINESS_LOST_STATUS & writer_->get_status_mask()) {
		if (nullptr != writer_->get_listener()) {
			const_cast<DataWriterListener*>(writer_->get_listener())->on_liveliness_lost(writer_, livelinessLostStatus_);
			flag = true;
		}
	}
	if (LIVELINESS_LOST_STATUS & pub_->get_status_mask()) {
		if (nullptr != pub_->get_listener()) {
			const_cast<PublisherListener*>(pub_->get_listener())->on_liveliness_lost(writer_, livelinessLostStatus_);
			flag = true;
		}
	}
	if (LIVELINESS_LOST_STATUS & participant_->get_status_mask()) {
		if (nullptr != participant_->get_listener()) {
			const_cast<DomainParticipantListener*>(participant_->get_listener())->on_liveliness_lost(writer_, livelinessLostStatus_);
			flag = true;
		}
	}
	if (flag) {
		livelinessLostStatus_.total_count_change = 0;
	}
	SetStatus(LIVELINESS_LOST_STATUS, true);
	return RETCODE_OK;
}

ReturnCode_t DataWriterStatusManager::UpdatePublicationMatchedStatus(const InstanceHandle_t& handle, const bool& add)
{
	/* 非用户writer 不触发回调 */
	GUID_t guid = Conversion::ToGuid(writer_->get_instance_handle());
	if (ENTITYKIND_USER_WRITER_NO_KEY != guid.entityId.entityKind  && ENTITYKIND_USER_WRITER_WITH_KEY != guid.entityId.entityKind)
		return  RETCODE_OK;

	std::lock_guard<std::mutex> lock(mtx_);
	if (add) {
		publicationMatchedStatus_.total_count++;
		publicationMatchedStatus_.total_count_change++;
		publicationMatchedStatus_.current_count++;
		publicationMatchedStatus_.current_count_change++;
	} else {
		publicationMatchedStatus_.current_count--;
		publicationMatchedStatus_.current_count_change++;
	}
	//if (publicationMatchedStatus_.current_count > publicationMatchedStatus_.current_count_peak) {
	//	publicationMatchedStatus_.current_count_peak = publicationMatchedStatus_.current_count;
	//}
	publicationMatchedStatus_.last_subscription_handle = handle;

	bool flag = false;
	if (PUBLICATION_MATCHED_STATUS & writer_->get_status_mask()) {
		if (nullptr != writer_->get_listener()) {
			const_cast<DataWriterListener*>(writer_->get_listener())->on_publication_matched(writer_, publicationMatchedStatus_);
			flag = true;
		}
	}
	if (PUBLICATION_MATCHED_STATUS & pub_->get_status_mask()) {
		if (nullptr != pub_->get_listener()) {
			const_cast<PublisherListener*>(pub_->get_listener())->on_publication_matched(writer_, publicationMatchedStatus_);
			flag = true;
		}
	}
	if (PUBLICATION_MATCHED_STATUS & participant_->get_status_mask()) {
		if (nullptr != participant_->get_listener()) {
			const_cast<DomainParticipantListener*>(participant_->get_listener())->on_publication_matched(writer_, publicationMatchedStatus_);
			flag = true;
		}
	}
	if (flag) {
		publicationMatchedStatus_.total_count_change = 0;
		publicationMatchedStatus_.current_count_change = 0;
	}
	SetStatus(PUBLICATION_MATCHED_STATUS, true);
	return RETCODE_OK;
}

ReturnCode_t DataWriterStatusManager::UpdateOfferedDeadlineMissedStatus(const InstanceHandle_t& handle)
{
	/* 非用户writer 不触发回调 */
	GUID_t guid = Conversion::ToGuid(writer_->get_instance_handle());
	if (ENTITYKIND_USER_WRITER_NO_KEY != guid.entityId.entityKind  && ENTITYKIND_USER_WRITER_WITH_KEY != guid.entityId.entityKind)
		return  RETCODE_OK;

	std::lock_guard<std::mutex> lock(mtx_);

	offeredDeadlineMissedStatus_.total_count++;
	offeredDeadlineMissedStatus_.total_count_change++;
	offeredDeadlineMissedStatus_.last_instance_handle = handle;

	bool flag = false;
	if (OFFERED_DEADLINE_MISSED_STATUS & writer_->get_status_mask()) {
		if (nullptr != writer_->get_listener()) {
			const_cast<DataWriterListener*>(writer_->get_listener())->on_offered_deadline_missed(writer_, offeredDeadlineMissedStatus_);
			flag = true;
		}
	}
	if (OFFERED_DEADLINE_MISSED_STATUS & pub_->get_status_mask()) {
		if (nullptr != pub_->get_listener()) {
			const_cast<PublisherListener*>(pub_->get_listener())->on_offered_deadline_missed(writer_, offeredDeadlineMissedStatus_);
			flag = true;
		}
	}
	if (OFFERED_DEADLINE_MISSED_STATUS & participant_->get_status_mask()) {
		if (nullptr != participant_->get_listener()) {
			const_cast<DomainParticipantListener*>(participant_->get_listener())->on_offered_deadline_missed(writer_, offeredDeadlineMissedStatus_);
			flag = true;
		}
	}
	if (flag) {
		offeredDeadlineMissedStatus_.total_count_change = 0;
	}
	SetStatus(OFFERED_DEADLINE_MISSED_STATUS, true);
	return RETCODE_OK;
}

ReturnCode_t DataWriterStatusManager::UpdateOfferedIncompatibleQosStatus(std::set<QosPolicyId_t>& policyId)
{
	/* 非用户writer 不触发回调 */
	GUID_t guid = Conversion::ToGuid(writer_->get_instance_handle());
	if (ENTITYKIND_USER_WRITER_NO_KEY != guid.entityId.entityKind  && ENTITYKIND_USER_WRITER_WITH_KEY != guid.entityId.entityKind)
		return  RETCODE_OK;

	std::lock_guard<std::mutex> lock(mtx_);
	offeredIncompatibleQosStatus_.last_policy_id = *policyId.begin();
	int32_t length = offeredIncompatibleQosStatus_.policies.size();
	for (int32_t i = 0; i < length; i++) {
		auto it = policyId.find(offeredIncompatibleQosStatus_.policies[i].policy_id);
		if (it != policyId.end()) {
			offeredIncompatibleQosStatus_.policies[i].count++;
			policyId.erase(it);
		}
	}
	for (auto it : policyId) {
		QosPolicyCount count;
		count.policy_id = it;
		count.count = 1;
		offeredIncompatibleQosStatus_.policies.push_back(count);
	}
	offeredIncompatibleQosStatus_.total_count++;
	offeredIncompatibleQosStatus_.total_count_change++;
	bool flag = false;
	if (OFFERED_INCOMPATIBLE_QOS_STATUS & writer_->get_status_mask()) {
		if (nullptr != writer_->get_listener()) {
			const_cast<DataWriterListener*>(writer_->get_listener())->on_offered_incompatible_qos(writer_, offeredIncompatibleQosStatus_);
			flag = true;
		}
	}
	if (OFFERED_INCOMPATIBLE_QOS_STATUS & pub_->get_status_mask()) {
		if (nullptr != pub_->get_listener()) {
			const_cast<PublisherListener*>(pub_->get_listener())->on_offered_incompatible_qos(writer_, offeredIncompatibleQosStatus_);
			flag = true;
		}
	}
	if (OFFERED_INCOMPATIBLE_QOS_STATUS & participant_->get_status_mask()) {
		if (nullptr != participant_->get_listener()) {
			const_cast<DomainParticipantListener*>(participant_->get_listener())->on_offered_incompatible_qos(writer_, offeredIncompatibleQosStatus_);
			flag = true;
		}
	}
	if (flag) {
		offeredIncompatibleQosStatus_.total_count_change = 0;
	}
	SetStatus(OFFERED_INCOMPATIBLE_QOS_STATUS, true);
	return RETCODE_OK;
}


ReturnCode_t DataWriterStatusManager::GetLivelinessLostStatus(LivelinessLostStatus& status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = livelinessLostStatus_;
	livelinessLostStatus_.total_count_change = 0;
	SetStatus(LIVELINESS_LOST_STATUS, false);
	return RETCODE_OK;
}

ReturnCode_t DataWriterStatusManager::GetOfferedDeadlineMissedStatus(OfferedDeadlineMissedStatus& status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = offeredDeadlineMissedStatus_;
	offeredDeadlineMissedStatus_.total_count_change = 0;
	SetStatus(OFFERED_DEADLINE_MISSED_STATUS, false);
	return RETCODE_OK;
}

ReturnCode_t DataWriterStatusManager::GetOfferedIncompatibleQosStatus(OfferedIncompatibleQosStatus& status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = offeredIncompatibleQosStatus_;
	offeredIncompatibleQosStatus_.total_count_change = 0;
	SetStatus(OFFERED_INCOMPATIBLE_QOS_STATUS, false);
	return RETCODE_OK;
}

ReturnCode_t DataWriterStatusManager::GetPublicationMatchedStatus(PublicationMatchedStatus& status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = publicationMatchedStatus_;
	publicationMatchedStatus_.total_count_change = 0;
	publicationMatchedStatus_.current_count_change = 0;
	SetStatus(PUBLICATION_MATCHED_STATUS, false);
	return RETCODE_OK;
}

void DataWriterStatusManager::SetStatus(const StatusMask& status, bool addFlag)
{
	writercondition_->set_status(status, addFlag);
	pubcondition_->set_status(status, addFlag);
	participantcondition_->set_status(status, addFlag);
}

DataReaderStatusManager::DataReaderStatusManager(DataReaderImpl* reader)
	: reader_(reader),
	  sub_(dynamic_cast<SubscriberImpl*>(const_cast<Subscriber*>(reader_->get_subscriber()))),
	  participant_(dynamic_cast<DomainParticipantImpl*>(const_cast<DomainParticipant*>(sub_->get_participant()))),
	  readercondition_(dynamic_cast<StatusConditionImpl*>(&reader_->get_statuscondition())),
	  subcondition_(dynamic_cast<StatusConditionImpl*>(&sub_->get_statuscondition())),
      participantcondition_(dynamic_cast<StatusConditionImpl*>(&participant_->get_statuscondition()))
{
}

DataReaderStatusManager::~DataReaderStatusManager()
{
}

ReturnCode_t DataReaderStatusManager::UpdateSubscriptionMatchedStatus(const InstanceHandle_t& handle, const bool& add)
{
	/* 非用户reader 不触发回调 */
	GUID_t guid = Conversion::ToGuid(reader_->get_instance_handle());
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind  && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind) {
		return  RETCODE_OK;
	}
		

	std::lock_guard<std::mutex> lock(mtx_);

	if (add) {
		subscriptionMatchedStatus_.total_count++;
		subscriptionMatchedStatus_.total_count_change++;
		subscriptionMatchedStatus_.current_count++;
		subscriptionMatchedStatus_.current_count_change++;
	} else {
		subscriptionMatchedStatus_.current_count--;
		subscriptionMatchedStatus_.current_count_change++;
	}

	subscriptionMatchedStatus_.last_publication_handle = handle;

	bool flag = false;
	if (SUBSCRIPTION_MATCHED_STATUS & reader_->get_status_mask()) {
		if (nullptr != reader_->get_listener()) {
			const_cast<DataReaderListener*>(reader_->get_listener())->on_subscription_matched(reader_, subscriptionMatchedStatus_);
			flag = true;
		}
	}
	if (SUBSCRIPTION_MATCHED_STATUS & sub_->get_status_mask()) {
		if (nullptr != sub_->get_listener()) {
			const_cast<SubscriberListener*>(sub_->get_listener())->on_subscription_matched(reader_, subscriptionMatchedStatus_);
			flag = true;
		}
	}
	if (SUBSCRIPTION_MATCHED_STATUS & participant_->get_status_mask()) {
		if (nullptr != participant_->get_listener()) {
			const_cast<DomainParticipantListener*>(participant_->get_listener())->on_subscription_matched(reader_, subscriptionMatchedStatus_);
			flag = true;
		}
	}
	if (flag) {
		subscriptionMatchedStatus_.total_count_change = 0;
		subscriptionMatchedStatus_.current_count_change = 0;
	}
	SetStatus(SUBSCRIPTION_MATCHED_STATUS, true);
	return RETCODE_OK;
}

ReturnCode_t DataReaderStatusManager::UpdateLivelinessChangedStatus(const InstanceHandle_t& handle, const bool& alive, const bool& ensure)
{
	/* 非用户reader 不触发回调 */
	GUID_t guid = Conversion::ToGuid(reader_->get_instance_handle());
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind  && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind)
		return  RETCODE_OK;

	std::lock_guard<std::mutex> lock(mtx_);
	if (ensure) {
		if (alive) {
			livelinessChangedStatus.alive_count++;
			livelinessChangedStatus.alive_count_change++;
		} else {
			livelinessChangedStatus.alive_count--;
			livelinessChangedStatus.alive_count_change--;
		}
	} else {
		if (alive) {
			livelinessChangedStatus.alive_count++;
			livelinessChangedStatus.not_alive_count--;
			livelinessChangedStatus.alive_count_change++;
			livelinessChangedStatus.not_alive_count_change--;
		} else {
			livelinessChangedStatus.alive_count--;
			livelinessChangedStatus.not_alive_count++;
			livelinessChangedStatus.alive_count_change--;
			livelinessChangedStatus.not_alive_count_change++;
		}
	}

	livelinessChangedStatus.last_publication_handle = handle;
	bool flag = false;
	if (LIVELINESS_CHANGED_STATUS & reader_->get_status_mask()) {
		if (nullptr != reader_->get_listener()) {
			const_cast<DataReaderListener*>(reader_->get_listener())->on_liveliness_changed(reader_, livelinessChangedStatus);
			flag = true;
		}
	}
	if (LIVELINESS_CHANGED_STATUS & sub_->get_status_mask()) {
		if (nullptr != sub_->get_listener()) {
			const_cast<SubscriberListener*>(sub_->get_listener())->on_liveliness_changed(reader_, livelinessChangedStatus);
			flag = true;
		}
	}
	if (LIVELINESS_CHANGED_STATUS & participant_->get_status_mask()) {
		if (nullptr != participant_->get_listener()) {
			const_cast<DomainParticipantListener*>(participant_->get_listener())->on_liveliness_changed(reader_, livelinessChangedStatus);
			flag = true;
		}
	}
	if (flag) {
		livelinessChangedStatus.alive_count_change = 0;
		livelinessChangedStatus.not_alive_count_change = 0;
	}
	SetStatus(LIVELINESS_CHANGED_STATUS, true);
	return RETCODE_OK;
}


ReturnCode_t DataReaderStatusManager::UpdateSampleLostStatus(int32_t ilost_count)
{
	/* 非用户reader 不触发回调 */
	GUID_t guid = Conversion::ToGuid(reader_->get_instance_handle());
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind  && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind)
		return  RETCODE_OK;

	std::lock_guard<std::mutex> lock(mtx_);

	sampleLostStatus_.total_count += ilost_count;
	sampleLostStatus_.total_count_change = ilost_count;
	bool flag = false;
	if (SAMPLE_LOST_STATUS & reader_->get_status_mask()) {
		if (nullptr != reader_->get_listener()) {
			const_cast<DataReaderListener*>(reader_->get_listener())->on_sample_lost(reader_, sampleLostStatus_);
			flag = true;
		}
	}
	if (SAMPLE_LOST_STATUS & sub_->get_status_mask()) {
		if (nullptr != sub_->get_listener()) {
			const_cast<SubscriberListener*>(sub_->get_listener())->on_sample_lost(reader_, sampleLostStatus_);
			flag = true;
		}
	}
	if (SAMPLE_LOST_STATUS & participant_->get_status_mask()) {
		if (nullptr != participant_->get_listener()) {
			const_cast<DomainParticipantListener*>(participant_->get_listener())->on_sample_lost(reader_, sampleLostStatus_);
			flag = true;
		}
	}
	if (flag) {
		sampleLostStatus_.total_count_change = 0;
	}
	SetStatus(SAMPLE_LOST_STATUS, true);
	return RETCODE_OK;
}

ReturnCode_t DataReaderStatusManager::UpdateRequestedIncompatibleQosStatus(std::set<QosPolicyId_t>& policyId)
{
	/* 非用户reader 不触发回调 */
	GUID_t guid = Conversion::ToGuid(reader_->get_instance_handle());
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind  && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind)
		return  RETCODE_OK;

	std::lock_guard<std::mutex> lock(mtx_);
	reqIncompatibleQosStatus_.last_policy_id = *policyId.begin();
	int32_t length = reqIncompatibleQosStatus_.policies.size();
	for (int32_t i = 0; i < length; i++) {
		auto it = policyId.find(reqIncompatibleQosStatus_.policies[i].policy_id);
		if (it != policyId.end()) {
			reqIncompatibleQosStatus_.policies[i].count++;
			policyId.erase(it);
		}
	}
	for (auto it : policyId) {
		QosPolicyCount count;
		count.policy_id = it;
		count.count = 1;
		reqIncompatibleQosStatus_.policies.push_back(count);
	}
	reqIncompatibleQosStatus_.total_count++;
	reqIncompatibleQosStatus_.total_count_change++;

	bool flag = false;
	if (REQUESTED_INCOMPATIBLE_QOS_STATUS & reader_->get_status_mask()) {
		if (nullptr != reader_->get_listener()) {
			const_cast<DataReaderListener*>(reader_->get_listener())->on_requested_incompatible_qos(reader_, reqIncompatibleQosStatus_);
			flag = true;
		}
	}
	if (REQUESTED_INCOMPATIBLE_QOS_STATUS & sub_->get_status_mask()) {
		if (nullptr != sub_->get_listener()) {
			const_cast<SubscriberListener*>(sub_->get_listener())->on_requested_incompatible_qos(reader_, reqIncompatibleQosStatus_);
			flag = true;
		}
	}
	if (REQUESTED_INCOMPATIBLE_QOS_STATUS & participant_->get_status_mask()) {
		if (nullptr != participant_->get_listener()) {
			const_cast<DomainParticipantListener*>(participant_->get_listener())->on_requested_incompatible_qos(reader_, reqIncompatibleQosStatus_);
			flag = true;
		}
	}
	if (flag) {
		reqIncompatibleQosStatus_.total_count_change = 0;
	}
	SetStatus(REQUESTED_INCOMPATIBLE_QOS_STATUS, true);
	return RETCODE_OK;
}


ReturnCode_t DataReaderStatusManager::UpdateSampleRejectedStatus(const InstanceHandle_t& handle, const SampleRejectedStatusKind& reason)
{
	/* 非用户reader 不触发回调 */
	GUID_t guid = Conversion::ToGuid(reader_->get_instance_handle());
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind  && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind)
		return  RETCODE_OK;

	std::lock_guard<std::mutex> lock(mtx_);

	sampleRejectedStatus_.total_count++;
	sampleRejectedStatus_.total_count_change++;
	sampleRejectedStatus_.last_reason = reason;
	sampleRejectedStatus_.last_instance_handle = handle;

	bool flag = false;
	if (SAMPLE_REJECTED_STATUS & reader_->get_status_mask()) {
		if (nullptr != reader_->get_listener()) {
			const_cast<DataReaderListener*>(reader_->get_listener())->on_sample_rejected(reader_, sampleRejectedStatus_);
			flag = true;
		}
	}
	if (SAMPLE_REJECTED_STATUS & sub_->get_status_mask()) {
		if (nullptr != sub_->get_listener()) {
			const_cast<SubscriberListener*>(sub_->get_listener())->on_sample_rejected(reader_, sampleRejectedStatus_);
			flag = true;
		}
	}
	if (SAMPLE_REJECTED_STATUS & participant_->get_status_mask()) {
		if (nullptr != participant_->get_listener()) {
			const_cast<DomainParticipantListener*>(participant_->get_listener())->on_sample_rejected(reader_, sampleRejectedStatus_);
			flag = true;
		}
	}
	if (flag) {
		sampleRejectedStatus_.total_count_change = 0;
	}
	SetStatus(SAMPLE_REJECTED_STATUS, true);
	return RETCODE_OK;
}

ReturnCode_t DataReaderStatusManager::UpdateRequestedDeadlineMissedStatus(const InstanceHandle_t& handle)
{
	/* 非用户reader 不触发回调 */
	GUID_t guid = Conversion::ToGuid(reader_->get_instance_handle());
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind  && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind)
		return  RETCODE_OK;

	std::lock_guard<std::mutex> lock(mtx_);

	reqDeadlineMissedStatus_.total_count++;
	reqDeadlineMissedStatus_.total_count_change++;
	reqDeadlineMissedStatus_.last_instance_handle = handle;
	bool flag = false;
	if (REQUESTED_DEADLINE_MISSED_STATUS & reader_->get_status_mask()) {
		if (nullptr != reader_->get_listener()) {
			const_cast<DataReaderListener*>(reader_->get_listener())->on_requested_deadline_missed(reader_, reqDeadlineMissedStatus_);
			flag = true;
		}
	}
	if (REQUESTED_DEADLINE_MISSED_STATUS & sub_->get_status_mask()) {
		if (nullptr != sub_->get_listener()) {
			const_cast<SubscriberListener*>(sub_->get_listener())->on_requested_deadline_missed(reader_, reqDeadlineMissedStatus_);
			flag = true;
		}
	}
	if (REQUESTED_DEADLINE_MISSED_STATUS & participant_->get_status_mask()) {
		if (nullptr != participant_->get_listener()) {
			const_cast<DomainParticipantListener*>(participant_->get_listener())->on_requested_deadline_missed(reader_, reqDeadlineMissedStatus_);
			flag = true;
		}
	}
	if (flag) {
		reqDeadlineMissedStatus_.total_count_change = 0;
	}
	SetStatus(REQUESTED_DEADLINE_MISSED_STATUS, true);
	return RETCODE_OK;
}

ReturnCode_t DataReaderStatusManager::GetLivelinessChangedStatus(LivelinessChangedStatus& status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = livelinessChangedStatus;
	livelinessChangedStatus.alive_count_change = 0;
	livelinessChangedStatus.not_alive_count_change = 0;
	SetStatus(LIVELINESS_CHANGED_STATUS, false);
	return RETCODE_OK;
}

ReturnCode_t DataReaderStatusManager::GetRequestedDeadlineMissedStatus(RequestedDeadlineMissedStatus& status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = reqDeadlineMissedStatus_;
	reqDeadlineMissedStatus_.total_count_change = 0;
	SetStatus(REQUESTED_DEADLINE_MISSED_STATUS, false);
	return RETCODE_OK;
}

ReturnCode_t DataReaderStatusManager::GetRequestedIncompatibleQosStatus(RequestedIncompatibleQosStatus& status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = reqIncompatibleQosStatus_;
	reqIncompatibleQosStatus_.total_count_change = 0;
	SetStatus(REQUESTED_INCOMPATIBLE_QOS_STATUS, false);
	return RETCODE_OK;
}

ReturnCode_t DataReaderStatusManager::GetSampleLostStatus(SampleLostStatus& status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = sampleLostStatus_;
	sampleLostStatus_.total_count_change = 0;
	SetStatus(SAMPLE_LOST_STATUS, false);
	return RETCODE_OK;
}

ReturnCode_t DataReaderStatusManager::GetSampleRejectedStatus(SampleRejectedStatus& status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = sampleRejectedStatus_;
	sampleRejectedStatus_.total_count_change = 0;
	SetStatus(SAMPLE_REJECTED_STATUS, false);
	return RETCODE_OK;
}

ReturnCode_t DataReaderStatusManager::GetSubscriptionMatchedStatus(SubscriptionMatchedStatus& status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = subscriptionMatchedStatus_;
	subscriptionMatchedStatus_.current_count_change = 0;
	subscriptionMatchedStatus_.total_count_change = 0;
	SetStatus(SUBSCRIPTION_MATCHED_STATUS, false);
	return RETCODE_OK;
}

void DataReaderStatusManager::SetStatus(const StatusMask& status, bool addFlag)
{
	readercondition_->set_status(status, addFlag);
	subcondition_->set_status(status, addFlag);
	participantcondition_->set_status(status, addFlag);
}

TopicStatusManager::TopicStatusManager(TopicImpl * topic)
	: topic_(topic)
{
}

ReturnCode_t TopicStatusManager::GetInconsistentTopicStatus(InconsistentTopicStatus & status)
{
	std::lock_guard<std::mutex> lock(mtx_);

	status = status_;
	status_.total_count_change = 0;
	dynamic_cast<StatusConditionImpl*>(&topic_->get_statuscondition())->set_status(INCONSISTENT_TOPIC_STATUS,false);
	dynamic_cast<StatusConditionImpl*>(&topic_->get_participant()->get_statuscondition())->set_status(INCONSISTENT_TOPIC_STATUS, false);
	return RETCODE_OK;
}

void TopicStatusManager::UpdateInconsistentTopicStatus()
{
	std::lock_guard<std::mutex> lock(mtx_);

	status_.total_count++;
	status_.total_count_change++;

	const DomainParticipantImpl* participant = dynamic_cast<const DomainParticipantImpl*>(topic_->get_participant());

	InconsistentTopicStatus status = status_;
	if (participant->get_listener() && (INCONSISTENT_TOPIC_STATUS & participant->get_status_mask())) {
		DomainParticipantListener * listener = const_cast<DomainParticipantListener*>(participant->get_listener());
		listener->on_inconsistent_topic(topic_, status);
		status_.total_count_change = 0;
	}

	if (topic_->get_listener() && (INCONSISTENT_TOPIC_STATUS & topic_->get_status_mask())) {
		const_cast<TopicListener*>(topic_->get_listener())->on_inconsistent_topic(topic_, status);
		status_.total_count_change = 0;
	}
	dynamic_cast<StatusConditionImpl*>(&topic_->get_statuscondition())->set_status(INCONSISTENT_TOPIC_STATUS, true);
	dynamic_cast<StatusConditionImpl*>(&topic_->get_participant()->get_statuscondition())->set_status(INCONSISTENT_TOPIC_STATUS, true);
}
