#include "dcps/subscribe/datareaderimpl.h"
#include "rtps/discovery/datasharingendpoint.h"
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/topic/topicdescriptionimpl.h"
#include "dcps/topic/typesupport.h"
#include "dcps/condition/readconditionimpl.h"
#include "dcps/condition/queryconditionimpl.h"
#include "dcps/base/statusmanager.h"
#include "dcps/listener/subscriberlistener.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "rtps/historycache/historycache.h"
#include "rtps/endpoint/statefulreader.h"
#include "rtps/endpoint/participant.h"
#include "rtps/discovery/discovery.h"
#include "qos/qospolicycheck.h"
#include "common/memory/memorypool.h"
#include "common/time/ddstimer.h"
#include "common/timeconversion.h"
#include "common/log/logger.h"
#ifdef DDSSECURITY
#include "security/securitycommon.h"
#endif // DDSSECURITY
#include "xtypes/dynamictypebuilderfactory.h"
#include "xtypes/dynamictypebuilder.h"
#include "xtypes/dynamicdatafactory.h"
#include "xtypes/dynamicdatatypesupport.h"
USING_TRAVODDS_NAMESPACE;

namespace {
struct NativeReaderBatch {
	std::vector<std::shared_ptr<CacheChange>> changes;
	SampleInfoSeq infos;
};
}

ReturnCode_t DataReaderImpl::CopySample(void* destination, const std::shared_ptr<CacheChange>& change)
{
	if (!destination || !change) return RETCODE_BAD_PARAMETER;
	if (change->nativePayload) {
		const auto type = typeSupport_->get_native_type_info();
		if (!type.loanable() || !(type == change->nativeType)) return RETCODE_BAD_PARAMETER;
		return typeSupport_->copy_data(destination, change->nativePayload.get()) == 0 ? RETCODE_OK : RETCODE_ERROR;
	}
	if (!change->serializedBuffer) return RETCODE_ERROR;
	SerializedBuffer buffer = *change->serializedBuffer;
	const bool littleEndian = GetEndianess(&buffer);
	CdrDeserializer cdr(&buffer, littleEndian);
	return typeSupport_->deserialize_data(destination, &cdr, littleEndian) == 0 ? RETCODE_OK : RETCODE_ERROR;
}

ReturnCode_t DataReaderImpl::MaterializeSamples(DataSeq& values, SampleInfoSeq& infos,
	const std::vector<std::shared_ptr<CacheChange>>& changes, bool requestLoan)
{
	if (changes.size() != infos.length()) return RETCODE_ERROR;
	try {
		const auto type = typeSupport_->get_native_type_info();
		bool allNative = !changes.empty();
		for (size_t i = 0; i < changes.size(); ++i) {
			if (!changes[i]) return RETCODE_ERROR;
			if (infos[uint32_t(i)].valid_data && changes[i]->nativePayload) {
				if (!type.loanable() || !(type == changes[i]->nativeType) ||
					values.element_size() != type.size || values.element_alignment() != type.alignment)
					return RETCODE_BAD_PARAMETER;
			} else allNative = false;
		}
		if (requestLoan && allNative) {
			auto batch = std::make_shared<NativeReaderBatch>();
			batch->changes = changes;
			std::vector<void*> dataPointers, infoPointers;
			dataPointers.reserve(changes.size());
			infoPointers.reserve(changes.size());
			for (size_t i = 0; i < changes.size(); ++i) dataPointers.push_back(changes[i]->nativePayload.get());
			batch->infos = std::move(infos);
			for (uint32_t i = 0; i < batch->infos.length(); ++i) infoPointers.push_back(&batch->infos[i]);
			if (!values.borrow(std::move(dataPointers), batch, loanIdentity_, 1) ||
				!infos.borrow(std::move(infoPointers), batch, loanIdentity_, 2)) {
				values.unloan();
				infos.unloan();
				return RETCODE_ERROR;
			}
			return RETCODE_OK;
		}
		if (!values.length(uint32_t(changes.size()))) return RETCODE_PRECONDITION_NOT_MET;
		for (uint32_t i = 0; i < changes.size(); ++i) {
			if (!infos[i].valid_data) continue;
			const auto result = CopySample(values[i], changes[i]);
			if (result != RETCODE_OK) return result;
		}
		return RETCODE_OK;
	} catch (const std::bad_alloc&) { return RETCODE_OUT_OF_RESOURCES; }
}

DataReaderImpl::DataReaderImpl(TopicDescription* topic, const DataReaderQos& qos, DataReaderListener* listener, StatusMask mask, SubscriberImpl* sub, TypeSupport* typeSupport)
    : DomainEntityImpl(mask), topic_(dynamic_cast<TopicDescriptionImpl*>(topic)), 
	  qos_(qos), listener_(listener), sub_(sub),
      statusMgr_(new DataReaderStatusManager(this)),
	  deadlineTimer_(nullptr),
	  lastInstance_(HANDLE_NIL),
	  filter_(nullptr),
	  typeSupport_(typeSupport),
	  history_(nullptr)
{
	topic_->Reference();
	if(topic)
	{
		memcpy(builtinData_.key.value, &handle_.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		InstanceHandle_t handle = sub_->get_participant()->get_instance_handle();
		memcpy(builtinData_.participant_key.value, &handle.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		builtinData_.type_name = topic->get_type_name();
		builtinData_.durability = qos.durability;
		builtinData_.deadline = qos.deadline;
		builtinData_.latency_budget = qos.latency_budget;
		builtinData_.liveliness = qos.liveliness;
		builtinData_.reliability = qos.reliability;
		builtinData_.user_data = qos.user_data;
		builtinData_.ownership = qos.ownership;
		builtinData_.destination_order = qos.destination_order;
		builtinData_.time_based_filter = qos.time_based_filter;
		SubscriberQos subQos;
		sub_->get_qos(subQos);
		builtinData_.presentation = subQos.presentation;
		builtinData_.partition = subQos.partition;
		builtinData_.group_data = subQos.group_data;
		Topic* realTopic = dynamic_cast<Topic*>(topic);
		if (realTopic) {
			TopicQos topicQos;
			realTopic->get_qos(topicQos);
			builtinData_.topic_name = topic->get_name();
			builtinData_.topic_data = topicQos.topic_data;
		}
		ContentFilteredTopicImpl* filteredTopic = dynamic_cast<ContentFilteredTopicImpl*>(topic);
		if (filteredTopic) {
			TopicQos topicQos;
			filteredTopic->get_related_topic()->get_qos(topicQos);
			builtinData_.topic_name = filteredTopic->get_related_topic()->get_name();
			builtinData_.topic_data = topicQos.topic_data;
			contentFilter_.contentFilteredTopicName = filteredTopic->get_name();
			contentFilter_.relatedTopicName = filteredTopic->get_related_topic()->get_name();
			/* sql 语法 */
			contentFilter_.filterClassName = SQL_FILTER_NAME;
			contentFilter_.filterExpression = filteredTopic->get_filter_expression();
			filteredTopic->get_expression_parameters(contentFilter_.expressionParameters);
			filteredTopic->AddReader(this);

			filter_ = std::make_shared<DDSSQL::DDSSQLFilter>(typeSupport_->get_dynamictype());
			filter_->InputSQL(contentFilter_.filterExpression, contentFilter_.expressionParameters);
		}
#ifdef DDSSECURITY
		memcpy(builtinSecureData_.key.value, &handle_.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		memcpy(builtinSecureData_.participant_key.value, &handle.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		builtinSecureData_.type_name = topic->get_type_name();
		builtinSecureData_.durability = qos.durability;
		builtinSecureData_.deadline = qos.deadline;
		builtinSecureData_.latency_budget = qos.latency_budget;
		builtinSecureData_.liveliness = qos.liveliness;
		builtinSecureData_.reliability = qos.reliability;
		builtinSecureData_.user_data = qos.user_data;
		builtinSecureData_.ownership = qos.ownership;
		builtinSecureData_.destination_order = qos.destination_order;
		builtinSecureData_.time_based_filter = qos.time_based_filter;
		builtinSecureData_.presentation = subQos.presentation;
		builtinSecureData_.partition = subQos.partition;
		builtinSecureData_.group_data = subQos.group_data;
		if (realTopic) {
			TopicQos topicQos;
			realTopic->get_qos(topicQos);
			builtinSecureData_.topic_name = topic->get_name();
			builtinSecureData_.topic_data = topicQos.topic_data;
		}
		if (filteredTopic) {
			TopicQos topicQos;
			filteredTopic->get_related_topic()->get_qos(topicQos);
			builtinSecureData_.topic_name = filteredTopic->get_related_topic()->get_name();
			builtinSecureData_.topic_data = topicQos.topic_data;
			contentFilter_.contentFilteredTopicName = filteredTopic->get_name();
			contentFilter_.relatedTopicName = filteredTopic->get_related_topic()->get_name();
			/* sql 语法 */
			contentFilter_.filterClassName = SQL_FILTER_NAME;
			contentFilter_.filterExpression = filteredTopic->get_filter_expression();
			filteredTopic->get_expression_parameters(contentFilter_.expressionParameters);
		}
#endif // DDSSECURITY
	}
}

DataReaderImpl::~DataReaderImpl()
{
    { std::lock_guard<std::mutex> schedule(lastInstanceMtx_); timersClosing_ = true; }
    if (reader_) {
        std::shared_ptr<Timer> filterTimer;
        { std::lock_guard<std::recursive_mutex> ingress(reader_->dataMutex_); filterTimer = reader_->lastNewCacheTimer_; }
        // The filter callback releases ingress before calling the application.
        // Drain it before disconnecting/destroying the associated DCPS state.
        if (filterTimer) filterTimer->Exit();
        // Join native listeners without holding ingress, then drain discovery
        // callbacks before destroying the status manager or other DCPS state.
        reader_->StopDataSharing();
        reader_->SetRelatedDataReader(nullptr);
    }
	if (deadlineTimer_) {
		deadlineTimer_->Exit();
	}

	//if (lifespanTimer_) {
	//	lifespanTimer_->Exit();
	//}
	ContentFilteredTopicImpl* filteredTopic = dynamic_cast<ContentFilteredTopicImpl*>(topic_);
	if (filteredTopic){
		filteredTopic->RemoveReader(this);
	}
	topic_->Dereference();
	readcdts_.clear();
	querycdts_.clear();
    if (listener_) {
        //delete listener_;
		listener_ = nullptr;
    }
#ifdef DDSSECURITY
	for (const auto& it : preparingWriters_) {
		if (it.second != nullptr) {
			delete it.second;
			preparingWriters_[it.first] = nullptr;
		}
	}
	//安全扩展 释放crypto_key_factory相关
	if (enableSec_)
	{
		GUID_t drGuid = Conversion::ToGuid(get_instance_handle());
		//if ((drGuid.IsSecureBuiltinEntity()
		//	&& drGuid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER)
		//	|| secAttributes_.is_payload_protected
		//	|| secAttributes_.is_submessage_protected)
		if (drGuid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER &&
			(secAttributes_.is_payload_protected ||
			secAttributes_.is_submessage_protected))
		{
			if (secPlugin_ != NULL && secPlugin_->crypto_key_factory != NULL)
			{
				SecurityException secExce;
				secPlugin_->crypto_key_factory->unregister_datareader(
					cryptoHdl_, &secExce);
			}
		}
	}
#endif // DDSSECURITY
}

ReturnCode_t DataReaderImpl::set_qos(const DataReaderQos& qos)
{
    std::unique_lock<std::recursive_mutex> ingress;
    if (reader_) ingress = std::unique_lock<std::recursive_mutex>(reader_->dataMutex_);
    std::unique_lock<std::mutex> schedule(lastInstanceMtx_);
    if (timersClosing_) return RETCODE_ALREADY_DELETED;

    DataReaderQos defaultQos;
    sub_->get_default_datareader_qos(defaultQos);
    const DataReaderQos& qosTemp = (&qos == &DATAREADER_QOS_DEFAULT) ? defaultQos : qos;
    if (&qos != &DATAREADER_QOS_DEFAULT) {
        ReturnCode_t ret = QosPolicyCheck::CheckQos(qosTemp);
        if (RETCODE_OK != ret) {
			TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
            return ret;
        }
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qosTemp)) {
		TRAVODDS_LOG(LOG_ERROR, "CanQosBeUpdated Err");
        return RETCODE_IMMUTABLE_POLICY;
    }
	if (enable_) {
		if (qos_.deadline.period != qosTemp.deadline.period && qosTemp.deadline.period != Duration_t(TIME_INFINITE)) {
			std::chrono::steady_clock::time_point nextTime =
				std::chrono::steady_clock::now() + std::chrono::milliseconds(Conversion::ToMilliSecondsInt64(qosTemp.deadline.period));
			history_->UpAllDeadline(nextTime);
			if (deadlineTimer_) {
				deadlineTimer_->UpdatePeriod(Conversion::ToMilliSecondsInt64(qosTemp.deadline.period));
			} else {
                deadlineTimer_ = TimerManager::CreateTimer(
					qosTemp.deadline.period,
					[this] {
						DeadlineTimeout();
					},
					"dr.deadline"
                );
			}
		}
		else if (qosTemp.deadline.period == Duration_t(TIME_INFINITE)) {
			if (deadlineTimer_) deadlineTimer_->Stop();
            if (history_) history_->UpAllDeadline(std::chrono::steady_clock::time_point::max());
		}
		reader_->SetQos(qosTemp);
	}
	qos_ = qosTemp;
    builtinData_.deadline = qosTemp.deadline;
    builtinData_.latency_budget = qosTemp.latency_budget;
    builtinData_.user_data = qosTemp.user_data;
    builtinData_.time_based_filter = qosTemp.time_based_filter;
    const auto advertisement = builtinData_;
    const bool advertise = enable_;
    schedule.unlock();
    if (ingress.owns_lock()) ingress.unlock();
#ifndef DDSSECURITY
    if (advertise) {
        auto* domain = dynamic_cast<DomainParticipantImpl*>(const_cast<DomainParticipant*>(sub_->get_participant()));
        if (domain && domain->GetDiscovery()) {
            DiscoveredReaderData update;
            update.ddsSubscriptionData = advertisement;
            domain->GetDiscovery()->WriteSubscriptionData(update);
        }
    }
#endif
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::get_qos(DataReaderQos& qos) const
{
    std::lock_guard<std::mutex> schedule(lastInstanceMtx_);
    qos = qos_;
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::set_listener(DataReaderListener* listener, const StatusMask& mask)
{
	//TODO JD 需要添加互斥锁  //
	if (listener_) {
		//delete listener_;
		listener_ = nullptr;
	}

	listener_ = listener;
	statusMask_ = mask;

    return RETCODE_OK;
}

const DataReaderListener * DataReaderImpl::get_listener() const
{
    return listener_;
}

ReturnCode_t DataReaderImpl::read(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, SampleStateMask sample_states,
	ViewStateMask view_states, InstanceStateMask instance_states)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	const auto outputType = typeSupport_->get_native_type_info();
	if (outputType.loanable() && (data_values.element_size() != outputType.size ||
		data_values.element_alignment() != outputType.alignment)) return RETCODE_BAD_PARAMETER;
	if (&data_values == static_cast<SequenceBase*>(&sample_infos)) return RETCODE_BAD_PARAMETER;
	if (data_values.has_loan() || sample_infos.has_loan()) return RETCODE_PRECONDITION_NOT_MET;
	if (max_samples <= 0 && max_samples != LENGTH_UNLIMITED) return RETCODE_BAD_PARAMETER;
	const bool requestLoan = data_values.maximum() == 0;
	data_values.unloan();
	sample_infos.unloan();
	std::vector<std::shared_ptr<CacheChange>> datas;
	ReturnCode_t ret = RETCODE_OK;

	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, sample_states, view_states, instance_states, false, true);
	if (RETCODE_OK != ret) {
		if(RETCODE_ERROR == ret) {
			TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
		}
		return ret;
	}
	const auto result = MaterializeSamples(data_values, sample_infos, datas, requestLoan);
    if (result != RETCODE_OK) { data_values.unloan(); sample_infos.unloan(); return result; }
    history_->CommitSamples(datas, false);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::take(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	const auto outputType = typeSupport_->get_native_type_info();
	if (outputType.loanable() && (data_values.element_size() != outputType.size ||
		data_values.element_alignment() != outputType.alignment)) return RETCODE_BAD_PARAMETER;
	if (&data_values == static_cast<SequenceBase*>(&sample_infos)) return RETCODE_BAD_PARAMETER;
	if (data_values.has_loan() || sample_infos.has_loan()) return RETCODE_PRECONDITION_NOT_MET;
	if (max_samples <= 0 && max_samples != LENGTH_UNLIMITED) return RETCODE_BAD_PARAMETER;
	const bool requestLoan = data_values.maximum() == 0;
	data_values.unloan();
	sample_infos.unloan();
	std::vector<std::shared_ptr<CacheChange>> datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, sample_states, view_states, instance_states, true, true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
		return ret;
	}

	const auto result = MaterializeSamples(data_values, sample_infos, datas, requestLoan);
    if (result != RETCODE_OK) { data_values.unloan(); sample_infos.unloan(); return result; }
    history_->CommitSamples(datas, true);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::read_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, ReadCondition* condition)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	const auto outputType = typeSupport_->get_native_type_info();
	if (outputType.loanable() && (data_values.element_size() != outputType.size ||
		data_values.element_alignment() != outputType.alignment)) return RETCODE_BAD_PARAMETER;
	if (&data_values == static_cast<SequenceBase*>(&sample_infos)) return RETCODE_BAD_PARAMETER;
	if (data_values.has_loan() || sample_infos.has_loan()) return RETCODE_PRECONDITION_NOT_MET;
	if (max_samples <= 0 && max_samples != LENGTH_UNLIMITED) return RETCODE_BAD_PARAMETER;
	if (!condition || condition->get_datareader() != this) return RETCODE_BAD_PARAMETER;
	const bool requestLoan = data_values.maximum() == 0;
	data_values.unloan();
	sample_infos.unloan();
	std::vector<std::shared_ptr<CacheChange>> datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, condition->get_sample_state_mask(),
		condition->get_view_state_mask(), condition->get_instance_state_mask(), false, true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
		return ret;
	}

	const auto result = MaterializeSamples(data_values, sample_infos, datas, requestLoan);
    if (result != RETCODE_OK) { data_values.unloan(); sample_infos.unloan(); return result; }
    history_->CommitSamples(datas, false);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::take_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, ReadCondition* condition)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	const auto outputType = typeSupport_->get_native_type_info();
	if (outputType.loanable() && (data_values.element_size() != outputType.size ||
		data_values.element_alignment() != outputType.alignment)) return RETCODE_BAD_PARAMETER;
	if (&data_values == static_cast<SequenceBase*>(&sample_infos)) return RETCODE_BAD_PARAMETER;
	if (data_values.has_loan() || sample_infos.has_loan()) return RETCODE_PRECONDITION_NOT_MET;
	if (max_samples <= 0 && max_samples != LENGTH_UNLIMITED) return RETCODE_BAD_PARAMETER;
	if (!condition || condition->get_datareader() != this) return RETCODE_BAD_PARAMETER;
	const bool requestLoan = data_values.maximum() == 0;
	data_values.unloan();
	sample_infos.unloan();
	std::vector<std::shared_ptr<CacheChange>> datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTake(datas, sample_infos, max_samples, condition->get_sample_state_mask(),
		condition->get_view_state_mask(), condition->get_instance_state_mask(), true, true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
		return ret;
	}

	const auto result = MaterializeSamples(data_values, sample_infos, datas, requestLoan);
    if (result != RETCODE_OK) { data_values.unloan(); sample_infos.unloan(); return result; }
    history_->CommitSamples(datas, true);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::read_next_sample(void* data_value, SampleInfo& sample_info)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
    if (!enable_) return RETCODE_NOT_ENABLED;
    if (!data_value) return RETCODE_BAD_PARAMETER;
    std::vector<std::shared_ptr<CacheChange>> changes;
    SampleInfoSeq infos;
    const auto result = reader_->ReadOrTake(changes, infos, 1, NOT_READ_SAMPLE_STATE,
        ANY_VIEW_STATE, ANY_INSTANCE_STATE, false, true);
    if (result != RETCODE_OK) return result;
    sample_info = infos[0];
    const auto copied = sample_info.valid_data ? CopySample(data_value, changes[0]) : RETCODE_OK;
    if (copied != RETCODE_OK) return copied;
    history_->CommitSamples(changes, false);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::take_next_sample(void* data_value, SampleInfo& sample_info)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
    if (!enable_) return RETCODE_NOT_ENABLED;
    if (!data_value) return RETCODE_BAD_PARAMETER;
    std::vector<std::shared_ptr<CacheChange>> changes;
    SampleInfoSeq infos;
    const auto result = reader_->ReadOrTake(changes, infos, 1, NOT_READ_SAMPLE_STATE,
        ANY_VIEW_STATE, ANY_INSTANCE_STATE, true, true);
    if (result != RETCODE_OK) return result;
    sample_info = infos[0];
    const auto copied = sample_info.valid_data ? CopySample(data_value, changes[0]) : RETCODE_OK;
    if (copied != RETCODE_OK) return copied;
    history_->CommitSamples(changes, true);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::read_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& a_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	const auto outputType = typeSupport_->get_native_type_info();
	if (outputType.loanable() && (data_values.element_size() != outputType.size ||
		data_values.element_alignment() != outputType.alignment)) return RETCODE_BAD_PARAMETER;
	if (&data_values == static_cast<SequenceBase*>(&sample_infos)) return RETCODE_BAD_PARAMETER;
	if (data_values.has_loan() || sample_infos.has_loan()) return RETCODE_PRECONDITION_NOT_MET;
	if (max_samples <= 0 && max_samples != LENGTH_UNLIMITED) return RETCODE_BAD_PARAMETER;
	const bool requestLoan = data_values.maximum() == 0;
	data_values.unloan();
	sample_infos.unloan();
	std::vector<std::shared_ptr<CacheChange>>  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeInstance(datas, sample_infos, max_samples, a_handle, sample_states, view_states, instance_states, false, true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTake ret = %d", ret);
		return ret;
	}

	const auto result = MaterializeSamples(data_values, sample_infos, datas, requestLoan);
    if (result != RETCODE_OK) { data_values.unloan(); sample_infos.unloan(); return result; }
    history_->CommitSamples(datas, false);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::take_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	const auto outputType = typeSupport_->get_native_type_info();
	if (outputType.loanable() && (data_values.element_size() != outputType.size ||
		data_values.element_alignment() != outputType.alignment)) return RETCODE_BAD_PARAMETER;
	if (&data_values == static_cast<SequenceBase*>(&sample_infos)) return RETCODE_BAD_PARAMETER;
	if (data_values.has_loan() || sample_infos.has_loan()) return RETCODE_PRECONDITION_NOT_MET;
	if (max_samples <= 0 && max_samples != LENGTH_UNLIMITED) return RETCODE_BAD_PARAMETER;
	const bool requestLoan = data_values.maximum() == 0;
	data_values.unloan();
	sample_infos.unloan();
	std::vector<std::shared_ptr<CacheChange>>  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeInstance(datas, sample_infos, max_samples, handle, sample_states, view_states, instance_states,true, true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTakeInstance ret = %d", ret);
		return ret;
	}

	const auto result = MaterializeSamples(data_values, sample_infos, datas, requestLoan);
    if (result != RETCODE_OK) { data_values.unloan(); sample_infos.unloan(); return result; }
    history_->CommitSamples(datas, true);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::read_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	const auto outputType = typeSupport_->get_native_type_info();
	if (outputType.loanable() && (data_values.element_size() != outputType.size ||
		data_values.element_alignment() != outputType.alignment)) return RETCODE_BAD_PARAMETER;
	if (&data_values == static_cast<SequenceBase*>(&sample_infos)) return RETCODE_BAD_PARAMETER;
	if (data_values.has_loan() || sample_infos.has_loan()) return RETCODE_PRECONDITION_NOT_MET;
	if (max_samples <= 0 && max_samples != LENGTH_UNLIMITED) return RETCODE_BAD_PARAMETER;
	const bool requestLoan = data_values.maximum() == 0;
	data_values.unloan();
	sample_infos.unloan();
	std::vector<std::shared_ptr<CacheChange>>  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle, sample_states, view_states, instance_states, false, true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
		return ret;
	}

	const auto result = MaterializeSamples(data_values, sample_infos, datas, requestLoan);
    if (result != RETCODE_OK) { data_values.unloan(); sample_infos.unloan(); return result; }
    history_->CommitSamples(datas, false);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::take_next_instance(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, SampleStateMask sample_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	const auto outputType = typeSupport_->get_native_type_info();
	if (outputType.loanable() && (data_values.element_size() != outputType.size ||
		data_values.element_alignment() != outputType.alignment)) return RETCODE_BAD_PARAMETER;
	if (&data_values == static_cast<SequenceBase*>(&sample_infos)) return RETCODE_BAD_PARAMETER;
	if (data_values.has_loan() || sample_infos.has_loan()) return RETCODE_PRECONDITION_NOT_MET;
	if (max_samples <= 0 && max_samples != LENGTH_UNLIMITED) return RETCODE_BAD_PARAMETER;
	const bool requestLoan = data_values.maximum() == 0;
	data_values.unloan();
	sample_infos.unloan();
	std::vector<std::shared_ptr<CacheChange>>  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle, sample_states, view_states, instance_states,true, true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
		return ret;
	}

	const auto result = MaterializeSamples(data_values, sample_infos, datas, requestLoan);
    if (result != RETCODE_OK) { data_values.unloan(); sample_infos.unloan(); return result; }
    history_->CommitSamples(datas, true);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::read_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, ReadCondition* condition)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	const auto outputType = typeSupport_->get_native_type_info();
	if (outputType.loanable() && (data_values.element_size() != outputType.size ||
		data_values.element_alignment() != outputType.alignment)) return RETCODE_BAD_PARAMETER;
	if (&data_values == static_cast<SequenceBase*>(&sample_infos)) return RETCODE_BAD_PARAMETER;
	if (data_values.has_loan() || sample_infos.has_loan()) return RETCODE_PRECONDITION_NOT_MET;
	if (max_samples <= 0 && max_samples != LENGTH_UNLIMITED) return RETCODE_BAD_PARAMETER;
	if (!condition || condition->get_datareader() != this) return RETCODE_BAD_PARAMETER;
	const bool requestLoan = data_values.maximum() == 0;
	data_values.unloan();
	sample_infos.unloan();
	std::vector<std::shared_ptr<CacheChange>>  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle,
		condition->get_sample_state_mask(), condition->get_view_state_mask(), condition->get_instance_state_mask(), false, true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
		return ret;
	}

	const auto result = MaterializeSamples(data_values, sample_infos, datas, requestLoan);
    if (result != RETCODE_OK) { data_values.unloan(); sample_infos.unloan(); return result; }
    history_->CommitSamples(datas, false);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::take_next_instance_w_condition(DataSeq& data_values, SampleInfoSeq& sample_infos, int32_t max_samples, const InstanceHandle_t& previous_handle, ReadCondition* condition)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    std::lock_guard<std::recursive_mutex> access(reader_->sampleAccessMutex);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	const auto outputType = typeSupport_->get_native_type_info();
	if (outputType.loanable() && (data_values.element_size() != outputType.size ||
		data_values.element_alignment() != outputType.alignment)) return RETCODE_BAD_PARAMETER;
	if (&data_values == static_cast<SequenceBase*>(&sample_infos)) return RETCODE_BAD_PARAMETER;
	if (data_values.has_loan() || sample_infos.has_loan()) return RETCODE_PRECONDITION_NOT_MET;
	if (max_samples <= 0 && max_samples != LENGTH_UNLIMITED) return RETCODE_BAD_PARAMETER;
	if (!condition || condition->get_datareader() != this) return RETCODE_BAD_PARAMETER;
	const bool requestLoan = data_values.maximum() == 0;
	data_values.unloan();
	sample_infos.unloan();
	std::vector<std::shared_ptr<CacheChange>>  datas;
	ReturnCode_t ret = RETCODE_OK;
	ret = reader_->ReadOrTakeNextInstance(datas, sample_infos, max_samples, previous_handle,
		condition->get_sample_state_mask(), condition->get_view_state_mask(), condition->get_instance_state_mask(), true, true);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "ReadOrTakeNextInstance ret = %d", ret);
		return ret;
	}

	const auto result = MaterializeSamples(data_values, sample_infos, datas, requestLoan);
    if (result != RETCODE_OK) { data_values.unloan(); sample_infos.unloan(); return result; }
    history_->CommitSamples(datas, true);
    SetStatus(false);
    NotifyReadConditions();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::return_loan(DataSeq& data_values, SampleInfoSeq& sample_infos)
{
    if (!data_values.has_loan() && !sample_infos.has_loan()) return RETCODE_OK;
    if (!data_values.matches_loan(sample_infos, loanIdentity_, 1, 2))
        return RETCODE_PRECONDITION_NOT_MET;
    data_values.unloan();
    sample_infos.unloan();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::CreateInstanceKey(const std::shared_ptr<CacheChange>& change,
        std::shared_ptr<SerializedBuffer>& serialized, std::shared_ptr<void>& native)
{
    if (!change) return RETCODE_BAD_PARAMETER;
    // Builtin discovery Readers have listeners but no application TypeSupport.
    if (!typeSupport_ || !typeSupport_->has_key() || change->kind != ALIVE) return RETCODE_OK;
    try {
        if (change->nativePayload) {
            const auto& info = change->nativeType;
            if (!info.loanable() || !(info == typeSupport_->get_native_type_info())) return RETCODE_BAD_PARAMETER;
            const auto alignment = std::align_val_t(std::max(info.alignment, uint32_t(alignof(std::max_align_t))));
            auto snapshot = std::shared_ptr<void>(::operator new(info.size, alignment),
                [alignment](void* p) { ::operator delete(p, alignment); });
            if (!typeSupport_->construct_native_sample(snapshot.get())) return RETCODE_ERROR;
            std::memcpy(snapshot.get(), change->nativePayload.get(), info.size);
            native = std::move(snapshot);
        } else {
            const auto pool = MemoryPool::GetInstance();
            PayloadBufferPtr owner(nullptr, PayloadBufferDeleter{pool});
            SerializedBuffer* tail = nullptr;
            for (auto* source = change->serializedBuffer; source; source = source->next) {
                PayloadBufferPtr part(new SerializedBuffer, PayloadBufferDeleter{pool});
                part->buffer_size = source->buffer_size;
                if (!source->buffer || !pool->getBuffer(part->buffer_size, part->origin_buffer)) return RETCODE_OUT_OF_RESOURCES;
                part->buffer = part->origin_buffer;
                part->writer_position = part->buffer_size;
                std::memcpy(part->buffer, source->buffer, part->buffer_size);
                auto* current = part.get();
                if (tail) tail->next = part.release();
                else owner = std::move(part);
                tail = current;
            }
            if (!owner) return RETCODE_BAD_PARAMETER;
            serialized = std::shared_ptr<SerializedBuffer>(std::move(owner));
        }
        return RETCODE_OK;
    } catch (const std::bad_alloc&) { return RETCODE_OUT_OF_RESOURCES; }
    catch (const std::exception&) { return RETCODE_ERROR; }
}

ReturnCode_t DataReaderImpl::get_key_value(void* key_holder, const InstanceHandle_t& handle)
{
    if (!enable_) return RETCODE_NOT_ENABLED;
    if (!typeSupport_->has_key()) return RETCODE_ILLEGAL_OPERATION;
    if (!key_holder || handle == HANDLE_NIL) return RETCODE_BAD_PARAMETER;
    if (auto native = history_->GetNativeKey(handle)) {
        return typeSupport_->copy_data(key_holder, native.get()) == 0 ? RETCODE_OK : RETCODE_ERROR;
    }
    auto data = history_->GetSerializedKey(handle);
    if (!data) return RETCODE_BAD_PARAMETER;
    // Each query owns its cursors, including all fragments after the first.
    std::vector<SerializedBuffer> views;
    for (auto* part = data.get(); part; part = part->next) views.push_back(*part);
    for (size_t i = 0; i < views.size(); ++i) {
        views[i].reader_position = 0;
        views[i].next = i + 1 < views.size() ? &views[i + 1] : nullptr;
    }
    if (views.empty() || views[0].buffer_size < CDR_ENDIANNESS_LEN) return RETCODE_ERROR;
    const bool littleEndian = GetEndianess(&views[0]);
    CdrDeserializer cdr(&views[0], littleEndian);
    return typeSupport_->deserialize_data(key_holder, &cdr, littleEndian) == 0 ? RETCODE_OK : RETCODE_ERROR;
}

InstanceHandle_t DataReaderImpl::lookup_instance(const void* instance_data) const
{
    if (!enable_ || !instance_data || !typeSupport_->has_key()) return HANDLE_NIL;
    InstanceHandle_t handle = HANDLE_NIL;
    if (!MakeKey(instance_data, handle, true) || !history_->IsInstancePresent(handle)) return HANDLE_NIL;
    return handle;
}

ReadCondition* DataReaderImpl::create_readcondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states)
{
    std::lock_guard<std::mutex> conditions(conditionsMutex_);
	ReadCondition* readcdt = nullptr;
    std::shared_ptr<ReadConditionImpl> readcdtPtr = std::make_shared<ReadConditionImpl>(smaple_states, view_states, instance_states,this);
    if (readcdtPtr) {
        readcdts_.push_back(readcdtPtr);
		readcdt = readcdtPtr.get();
    }
    return readcdt;
}

QueryCondition* DataReaderImpl::create_querycondition(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states, const std::string & query_expression, const StringSeq& query_parameters)
{
    std::lock_guard<std::mutex> conditions(conditionsMutex_);
	QueryCondition* querycdt = nullptr;
	std::shared_ptr<QueryConditionImpl> querycdtPtr = std::make_shared<QueryConditionImpl>(smaple_states, view_states, instance_states, query_expression, query_parameters, this);
	if (querycdtPtr) {
		querycdts_.push_back(querycdtPtr);
		querycdt = querycdtPtr.get();
	}

	return querycdt;
}

ReturnCode_t DataReaderImpl::delete_readcondition(ReadCondition* condition)
{
    std::lock_guard<std::mutex> conditions(conditionsMutex_);
	if (condition == nullptr) {
		TRAVODDS_LOG(LOG_ERROR, "condition is nullptr,RETCODE_BAD_PARAMETER");
		return RETCODE_BAD_PARAMETER;
	}
	for (auto it = readcdts_.begin(); it != readcdts_.end(); it++) {
		if (it->get() == condition) {
			readcdts_.erase(it);
			return RETCODE_OK;
		}
	}
	for (auto it = querycdts_.begin(); it != querycdts_.end(); it++) {
		if (it->get() == condition) {
			querycdts_.erase(it);
			return RETCODE_OK;
		}
	}
	TRAVODDS_LOG(LOG_ERROR, "datareder donot contain this condition");
	return RETCODE_PRECONDITION_NOT_MET;
}

ReturnCode_t DataReaderImpl::get_liveliness_changed_status(LivelinessChangedStatus &status) const
{
    return statusMgr_->GetLivelinessChangedStatus(status);
}

ReturnCode_t DataReaderImpl::get_requested_deadline_missed_status(RequestedDeadlineMissedStatus &status)
{
	return statusMgr_->GetRequestedDeadlineMissedStatus(status);
}

ReturnCode_t DataReaderImpl::get_requested_incompatible_qos_status(RequestedIncompatibleQosStatus &status)
{
	return statusMgr_->GetRequestedIncompatibleQosStatus(status);
}

ReturnCode_t DataReaderImpl::get_sample_lost_status(SampleLostStatus &status) const
{
	return statusMgr_->GetSampleLostStatus(status);
}

ReturnCode_t DataReaderImpl::get_sample_rejected_status(SampleRejectedStatus &status) const
{
	return statusMgr_->GetSampleRejectedStatus(status);
}

ReturnCode_t DataReaderImpl::get_subscription_matched_status(SubscriptionMatchedStatus &status) const
{
    return statusMgr_->GetSubscriptionMatchedStatus(status);;
}

const TopicDescription *DataReaderImpl::get_topicdescription() const
{
    return topic_;
}

const Subscriber *DataReaderImpl::get_subscriber() const
{
    return sub_;
}

ReturnCode_t DataReaderImpl::delete_contained_entities()
{
    std::lock_guard<std::mutex> conditions(conditionsMutex_);
	readcdts_.clear();
	querycdts_.clear();
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::wait_for_historical_data(const Duration_t &max_wait) const
{
	static_cast<void>(max_wait);
	TRAVODDS_LOG(LOG_ERROR, "datareader donot support interface");
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t DataReaderImpl::get_matched_publication_data(PublicationBuiltinTopicData &publication_data, const InstanceHandle_t &publication_handle) const
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
	if (reader_->GetMatchedPublicationData(publication_data, publication_handle)) {
		return RETCODE_OK;
	}
	return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DataReaderImpl::get_matched_publications(InstanceHandleSeq &publication_handles) const
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataReader not enable");
		return RETCODE_NOT_ENABLED;
	}
    reader_->GetMatchedPublications(publication_handles);
    return RETCODE_OK;
}

ReturnCode_t DataReaderImpl::enable()
{
	if (enable_) {
		return RETCODE_OK;
	}
	if (!sub_->IsEnable()) {
		return RETCODE_PRECONDITION_NOT_MET;
	}
	if (qos_.data_sharing.kind == ON_DATA_SHARING_QOS) {
#if defined(__linux__)
		if (!typeSupport_->get_native_type_info().loanable()) return RETCODE_UNSUPPORTED;
#else
		return RETCODE_UNSUPPORTED;
#endif
	}
	/* yuguxu 预留 DataReader 资源申请
	 *
	 * 待实现
	 */
	DomainParticipantImpl* domainParticipant = dynamic_cast<DomainParticipantImpl*>(const_cast<DomainParticipant*>(sub_->get_participant()));
	if (nullptr == domainParticipant) {
		return RETCODE_ERROR;
	}
	bool sharingAllowed = qos_.data_sharing.kind != OFF_DATA_SHARING_QOS &&
        !dynamic_cast<ContentFilteredTopicImpl*>(topic_);
    if (!sharingAllowed && qos_.data_sharing.kind == ON_DATA_SHARING_QOS) return RETCODE_UNSUPPORTED;
#ifdef DDSSECURITY
	sharingAllowed = sharingAllowed && !domainParticipant->enableDDSSec_;
#endif
	if (sharingAllowed)
		builtinData_.data_sharing = LocalDataSharingInfo(qos_.data_sharing, typeSupport_->get_native_type_info());
	if (qos_.data_sharing.kind == ON_DATA_SHARING_QOS && !builtinData_.data_sharing.valid(false))
		return RETCODE_UNSUPPORTED;
#ifdef DDSSECURITY
	builtinSecureData_.data_sharing = builtinData_.data_sharing;
#endif
	std::shared_ptr<Reader> readerPtr = domainParticipant->GetRelatedParticipant()->CreateReader(this, qos_, Conversion::ToGuid(handle_), typeSupport_->has_key());
	if (!readerPtr.get()) {
		return RETCODE_ERROR;
	}
	history_ = readerPtr->readerCache_;

	if (qos_.deadline.period != Duration_t(TIME_INFINITE)) {
        deadlineTimer_ = TimerManager::CreateTimer(
			qos_.deadline.period,
			[this] {
				DeadlineTimeout();
			},
			"dr.deadline"
        );
	}

	/* wangyi lifespan Qos 定时器添加  */
	//lifespanTimer_ = TimerManager::CreateTimer(
	//	TIME_INFINITE,
	//	[this] {
	//	LifespanTimeout();
	//	},
	//	"dr.lifespan"
	//);

    SetRelatedReader(readerPtr);
    readerPtr->StartMaintenance(readerPtr);
	domainParticipant->GetRelatedParticipant()->AddLocalReader(readerPtr);
	enable_ = true;

	DiscoveredReaderData readerData;
	std::shared_ptr<Discovery> discovery = domainParticipant->GetDiscovery();
#ifdef DDSSECURITY
	//本地匹配完成后再发送SEPD报文，防止重复匹配
	readerData.ddsSubscriptionData = GetBuiltinTopicSecureData();
	DiscoveredReaderSecureData readerSecureData;
	readerSecureData.ddsSubscriptionData = GetBuiltinTopicSecureData();
	if ((builtinSecureData_.security_info.endpoint_security_attributes &
		ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED) != 0) {
		discovery->WriteSubscriptionSecureData(readerSecureData);
	}
	else {
		discovery->WriteSubscriptionData(readerSecureData);
	}
#else
	//readerData.remoteReaderGuid = reader_->GetGuid();
	readerData.ddsSubscriptionData = GetBuiltinTopicData();
	readerData.contentFilter = contentFilter_;
	/* jdxcz TODO */
	readerData.unicastLocatorList = domainParticipant->GetRelatedParticipant()->GetDefaultUnicastLocators();
	readerData.multicastLocatorList = domainParticipant->GetRelatedParticipant()->GetDefaultMulticastLocators();


	discovery->WriteSubscriptionData(readerData);
#endif // DDSSECURITY

	return RETCODE_OK;
}

uint64_t DataReaderImpl::get_unread_count()
{
	if (history_) {
		SampleCounters counter = history_->GetSampleCounters(false);
		return counter.sampleNotRead;
	}
	return 0;
}

void DataReaderImpl::SetRelatedReader(std::shared_ptr<Reader> reader)
{
	reader_ = reader;
	reader->SetRelatedDataReader(this);
	handle_ = Conversion::ToInstanceHandle(reader->GetGuid());
	builtinData_.key = Conversion::ToBuiltinTopicKey(handle_);
#ifdef DDSSECURITY
	builtinSecureData_.key = Conversion::ToBuiltinTopicKey(handle_);
#endif // DDSSECURITY
}

bool DataReaderImpl::Empty() const
{
    std::lock_guard<std::mutex> conditions(conditionsMutex_);
	return readcdts_.empty() && querycdts_.empty();
}

ReturnCode_t DataReaderImpl::OnDataAvaible()
{
	/* 非用户reader 不触发回调 */
	GUID_t guid = reader_->GetGuid();
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind) {
		return  RETCODE_OK;
	}

	NotifyReadConditions();
    SetStatus(true);

	auto subListener = sub_->GetListener();
	const DomainParticipantImpl* participant = dynamic_cast<const DomainParticipantImpl*>(sub_->get_participant());
	 
	if(participant->get_listener() && (DATA_ON_READERS_STATUS & participant->get_status_mask())) {
		DomainParticipantListener * listener = const_cast<DomainParticipantListener*>(participant->get_listener());
		listener->on_data_on_readers(sub_);

	} else if (subListener && (DATA_ON_READERS_STATUS & sub_->get_status_mask())) {
		subListener->on_data_on_readers(sub_);
	} else if (listener_ && (DATA_AVAILABLE_STATUS & statusMask_)) {
		listener_->on_data_available(this);
	}

	return RETCODE_OK;
}
ReturnCode_t DataReaderImpl::OnSampleLost(int32_t lostCount)
{
	return statusMgr_->UpdateSampleLostStatus(lostCount);
}
int32_t DataReaderImpl::OnSampleRejected(const InstanceHandle_t& handle, const SampleRejectedStatusKind& reason)
{
	return statusMgr_->UpdateSampleRejectedStatus(handle, reason);
}

void DataReaderImpl::OnRequestedIncompatibleQos(std::set<QosPolicyId_t>& qosSeq)
{
	statusMgr_->UpdateRequestedIncompatibleQosStatus(qosSeq);
}
void DataReaderImpl::OnSubscriptionMatched(InstanceHandle_t& handle, const bool& add) {
	statusMgr_->UpdateSubscriptionMatchedStatus(handle, add);
}

SubscriptionBuiltinTopicData DataReaderImpl::GetBuiltinTopicData()
{
    std::lock_guard<std::mutex> schedule(lastInstanceMtx_);
	return builtinData_;
}

#ifdef DDSSECURITY
const SubscriptionBuiltinTopicSecureData& DataReaderImpl::GetBuiltinTopicSecureData()
{
	return builtinSecureData_;
}

bool DataReaderImpl::GetSecAttributes()
{
	enableSec_ = false;
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(sub_->get_participant());
	if (dp->enableDDSSec_) {
		SecurityException secExce;
		GUID_t guid = Conversion::ToGuid(handle_);
		//participantstatestatelessmessageredaer的安全实体的reader 或者是用户自定义读者实体
		if ((guid.IsSecureBuiltinEntity() && guid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER)
			|| (guid.IsUserDefinedEntity() && guid.entityId != ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER))
		{
			// Access_Control 创建的数据读者若为内置安全相关实体或者非内置实体，需要获取安全属性
			EndpointSecurityAttributes secAttr;
			SubscriberQos qos;
			sub_->get_qos(qos);
			if (!dp->secPlugin_.access_control->get_datareader_sec_attributes(
				dp->permissionHdl_,
				builtinSecureData_.topic_name.c_str(),
				&qos.partition,
				NULL,
				&secAttr,
				&secExce)) {
				//取消初始化释放资源
				createSuccess_ = false;
				return false;;
			}
			secAttributes_.ac_endpoint_properties = secAttr.ac_endpoint_properties;
			secAttributes_.is_write_protected = secAttr.is_write_protected;
			secAttributes_.is_read_protected = secAttr.is_read_protected;
			secAttributes_.is_discovery_protected = secAttr.is_discovery_protected;
			secAttributes_.is_liveliness_protected = secAttr.is_liveliness_protected;
			secAttributes_.is_payload_protected = secAttr.is_payload_protected;
			secAttributes_.is_key_protected = secAttr.is_key_protected;
			secAttributes_.is_submessage_protected = secAttr.is_submessage_protected;
			secAttributes_.plugin_endpoint_attributes = secAttr.plugin_endpoint_attributes;

			builtinSecureData_.security_info.plugin_endpoint_security_attributes = secAttr.plugin_endpoint_attributes;

			dp->secPlugin_.access_control->return_datawriter_sec_attributes(&secAttr, &secExce);
			if (guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) {
				rtpsMsgProtected_ = false;
			}
			else
			{
				rtpsMsgProtected_ = dp->secAttributes_.is_rtps_protected;
			}
		}
		else
		{
			// 若为内置实体，则不启用安全
			secAttributes_.is_write_protected = false;
			secAttributes_.is_read_protected = false;
			secAttributes_.is_discovery_protected = false;
			secAttributes_.is_liveliness_protected = false;
			secAttributes_.is_submessage_protected = false;
			secAttributes_.is_payload_protected = false;
			secAttributes_.is_key_protected = false;
			rtpsMsgProtected_ = false;
		}
		if (secAttributes_.is_write_protected
			|| secAttributes_.is_read_protected
			|| secAttributes_.is_discovery_protected
			//|| secAttributes_.is_liveliness_protected //PND 目前is_liveliness_protected初始化了，但是没做功能适配
			|| secAttributes_.is_submessage_protected
			|| secAttributes_.is_payload_protected
			//|| secAttributes_.is_key_protected //PND 目前is_liveliness_protected初始化了，但是没做功能适配
			)
		{
			if (dp->secPlugin_.crypto_key_factory == NULL)
			{
				TRAVODDS_LOG(LOG_ERROR, "Create DataReader failed: key factory not found.");
				createSuccess_ = false;
				return false;
			}
			if (guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER) {
				// unsigned int tmplen = secAttributes_.ac_endpoint_properties.size();
				Property_t tmpProperty;
				tmpProperty.name = "dds.sec.builtin_endpoint_name";
				tmpProperty.value = "BuiltinParticipantVolatileMessageSecureReader";
				secAttributes_.ac_endpoint_properties.push_back(tmpProperty);
			}
			cryptoHdl_ = dp->secPlugin_.crypto_key_factory->register_local_datareader(
				dp->cryptoHdl_,
				&secAttributes_.ac_endpoint_properties,
				&secExce);
			if (cryptoHdl_ == SEC_NIL_HANDLE)
			{
				createSuccess_ = false;
				return false;
			}
			if (!dp->secPlugin_.crypto_transform->get_datareader_submessage_encode_attributes(
				cryptoHdl_,
				&submsgEncodeAttr_))
			{
				createSuccess_ = false;
				return false;
			}
			enableSec_ = true;
			secPlugin_ = &(dp->secPlugin_);
		}
		else if (rtpsMsgProtected_)
		{
			enableSec_ = true;
			secPlugin_ = &dp->secPlugin_;
		}
		else
		{
			enableSec_ = false;
		}
	}
	createSuccess_ = false;
	return true;
}

int DataReaderImpl::SetRemoteWriterCryptoTokens(const GUID_t* writerId, const Sequence<CryptoToken>* tokens)
{
	GUID_t guid = reader_->GetGuid(); //ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER
	if (guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER)
	{
		return 0;
	}
	auto wrIter = preparingWriters_.find(*writerId);
	if (wrIter == preparingWriters_.end())
	{
		Sequence<CryptoToken>& tokenRef = remoteWriterCryptoToken_[*writerId];
		tokenRef = *tokens;
		return 0;
	}
	WriterSampleList* writer = wrIter->second;
	SecurityException secExce;
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(sub_->get_participant());
	if (!dp->secPlugin_.crypto_key_exchange->set_remote_datawriter_crypto_tokens(
		cryptoHdl_,
		writer->cryptoHdl,
		tokens,
		&secExce))
	{
		return -2;
	}
	writer->partCryptoHdl =
		GetRemoteParticipantCryptoHandle(dp->GetDiscovery()->GetSpdp().get(), &writerId->prefix);
	secWriters_.push_back(writer->cryptoHdl);
	writer->readyForSecComm = true;

	preparingWriters_.erase(wrIter);
	//DataReaderImpl存储远端写者的加密相关信息
	writerSampleList_[*writerId] = writer;
	StatefulReader* statefulReader = dynamic_cast<StatefulReader*>(reader_.get());
	if (statefulReader) {
		WriterProxyMap* writerMap = statefulReader->GetMatchedWriters();
		if (!writerMap->count(*writerId)) {
			return -1;
		}
		(*writerMap)[*writerId]->partCryptoHdl_ = writer->partCryptoHdl;
		(*writerMap)[*writerId]->cryptoHdl_ = writer->cryptoHdl;
		(*writerMap)[*writerId]->readyForSecComm_ = writer->readyForSecComm;
	}
	StatelessReader* statelessReader = dynamic_cast<StatelessReader*>(reader_.get());
	if (statelessReader) {
		std::map<GUID_t, std::shared_ptr<WriterProxy>>* writerMap = statelessReader->GetMatchWriters();
		if (!writerMap->count(*writerId)) {
			//return -1;
		} else {
            (*writerMap)[*writerId]->partCryptoHdl_ = writer->partCryptoHdl;
            (*writerMap)[*writerId]->cryptoHdl_ = writer->cryptoHdl;
            (*writerMap)[*writerId]->readyForSecComm_ = writer->readyForSecComm;
        }
	}
	return 0;
}

ParticipantCryptoHandle DataReaderImpl::GetRemoteParticipantCryptoHandle(Spdp* spdp, const GuidPrefix_t* remotePrefix)
{
	auto dpInfoIter = spdp->discParts_.find(*remotePrefix);
	if (dpInfoIter == spdp->discParts_.end())
	{
		return SEC_NIL_HANDLE;
	}
	if (dpInfoIter->second->enableDDSSec_)
	{
		return dpInfoIter->second->cryptoHdl_;
	}
	return SEC_NIL_HANDLE;
}

ReturnCode_t DataReaderImpl::FillSecureData()
{
	if (secAttributes_.is_discovery_protected) {
		if (enableSec_) {
			builtinSecureData_.security_info.endpoint_security_attributes =
				EndpointSecurityAttributesToMask(&secAttributes_);
			builtinSecureData_.security_info.plugin_endpoint_security_attributes = secAttributes_.plugin_endpoint_attributes;
		}
		else
		{
			builtinSecureData_.security_info = ENDPOINT_SECURITY_ATTRIBUTES_INFO_DEFAULT;
		}
	}
	return RETCODE_OK;
}


#endif // DDSSECURITY

bool DataReaderImpl::GetEndianess(SerializedBuffer* buffer)
{
	bool ret = false;
	ret = buffer->buffer[1] == uint32_t(1);

	buffer->buffer += CDR_ENDIANNESS_LEN;
	buffer->writer_position -= CDR_ENDIANNESS_LEN;
	buffer->buffer_size -= CDR_ENDIANNESS_LEN;

	return ret;
}

bool DataReaderImpl::MakeKey(const void* data, InstanceHandle_t& handle, bool forceMd5) const
{
    handle = HANDLE_NIL;
    if (!data) return false;
    try { return typeSupport_->MakeKey(data, handle, forceMd5) == 0; }
    catch (const std::exception&) { handle = HANDLE_NIL; return false; }
}

void DataReaderImpl::SetLastinstanceHandle(const std::shared_ptr<CacheChange>& cache)
{
	/*  */
	// cache->kind;
	

	if (!history_) return; // Builtin discovery readers have no application history binding.
	history_->MarkDeadlineActive(cache->instanceHandle, cache->kind == ALIVE);
	/* deadline */
	{
		std::lock_guard<std::mutex> lock(lastInstanceMtx_);
        if (timersClosing_ || !deadlineTimer_ || qos_.deadline.period == Duration_t(TIME_INFINITE)) return;
		std::chrono::steady_clock::time_point nextTime = 
			cache->kind == ALIVE ? std::chrono::steady_clock::now() + std::chrono::nanoseconds(qos_.deadline.period.ToNs()) :
            std::chrono::steady_clock::time_point::max();
		history_->UpdateReaderDeadline(cache->instanceHandle, cache->kind, nextTime);
        if (history_->GetNextDeadline(lastInstance_, nextTime)) {
            const auto delay = std::chrono::ceil<std::chrono::milliseconds>(nextTime - std::chrono::steady_clock::now()).count();
            deadlineTimer_->UpdatePeriod(std::max<int64_t>(1, delay));
        }
	}

}

void DataReaderImpl::DeadlineTimeout()
{
	InstanceHandle_t instance;
	{
		std::lock_guard<std::mutex> lock(lastInstanceMtx_);
        if (timersClosing_ || !deadlineTimer_ || qos_.deadline.period == Duration_t(TIME_INFINITE)) return;
        std::chrono::steady_clock::time_point due;
        if (!history_->GetNextDeadline(instance, due)) return;
        const auto now = std::chrono::steady_clock::now();
        bool expired = due <= now;
        if (expired && !history_->SetNextDeadline(instance,
                now + std::chrono::nanoseconds(qos_.deadline.period.ToNs()), true)) expired = false;
        if (history_->GetNextDeadline(lastInstance_, due)) {
            const auto delay = std::chrono::ceil<std::chrono::milliseconds>(due - std::chrono::steady_clock::now()).count();
            deadlineTimer_->UpdatePeriod(std::max<int64_t>(1, delay));
        }
        if (!expired) return;
	}
	
	statusMgr_->UpdateRequestedDeadlineMissedStatus(instance);
}

bool DataReaderImpl::CheckLifespan(const std::shared_ptr<CacheChange>& cache)
{
    if (cache->writerGuid == GUID_UNKNOWN) return true; // Internal history-only insertion has no remote QoS.
	/* 非用户reader不触发判断 */
	GUID_t guid = reader_->GetGuid();
	if (ENTITYKIND_USER_READER_NO_KEY != guid.entityId.entityKind && ENTITYKIND_USER_READER_WITH_KEY != guid.entityId.entityKind) {
		return  true;
	}

	PublicationBuiltinTopicData publicationData;
	if (!reader_->GetMatchedPublicationData(publicationData, Conversion::ToInstanceHandle(cache->writerGuid))) {
		return false;
	}
	 
	if (publicationData.lifespan.duration == Duration_t(TIME_INFINITE))
	{
		return true;
	}

    cache->expiresAt = std::chrono::system_clock::time_point(
        std::chrono::seconds(cache->sourceTimespace.sec) + std::chrono::nanoseconds(cache->sourceTimespace.nanosec) +
        std::chrono::nanoseconds(publicationData.lifespan.duration.ToNs()));
	Duration_t expirationTime = cache->sourceTimespace + publicationData.lifespan.duration;
	Duration_t currentTime;
	Duration_t::now(currentTime);

	//如果新报文到达得太晚，则它可能已过期。
	//如果已过期，将其从历史记录中删除，并返回false，以避免通知监听器。
	if (expirationTime < currentTime)
	{
		// Admission is checked before insertion; never remove by sequence alone.
		return false;
	}

	/* wangyi TODO 定时器停止后不再启动，待查 */
	//if (reader_->GetReaderCache()->GetMinSourceTimespace().get() == cache.get()) {
	//	lifespanTimer_->Stop();
	//}

	//Duration_t interval = expirationTime - currentTime;
	//lifespanTimer_->UpdatePeriod(interval.ToNs() * 1e-6);
	//lifespanTimer_->ReStart();
	return true;
}

//bool DataReaderImpl::LifespanTimeout()
//{
//	Duration_t currentTime;
//	Duration_t::now(currentTime);
//	std::shared_ptr<CacheChange> earliestChange = reader_->GetReaderCache()->GetMinSourceTimespace();
//	while (earliestChange != nullptr) {
//
//		PublicationBuiltinTopicData publicationData;
//		reader_->GetMatchedPublicationData(publicationData, Conversion::ToInstanceHandle(earliestChange->writerGuid));
//
//		if (publicationData.lifespan.duration == Duration_t(TIME_INFINITE))
//		{
//			break;
//		}
//
//		Duration_t expirationTime = Conversion::TimeToDuration(earliestChange->sourceTimespace) + publicationData.lifespan.duration;
//
//		/* 检查最早的报文缓存是否已过期（启动计时器的报文缓存可能已从历史记录中删除）*/
//		if (currentTime < expirationTime)
//		{
//			Duration_t interval = expirationTime - currentTime;
//			lifespanTimer_->UpdatePeriod(interval.ToNs() * 1e-6);
//			return true;
//		}
//		else {
//			// 最早的报文缓存已过期，删除该报文。
//			//reader_->GetReaderCache()->RemoveChange(earliestChange->sequenceNumber);
//			//earliestChange = reader_->GetReaderCache()->GetMinSourceTimespace();
//			break;
//		}
//	}
//	return false;
//}

ReturnCode_t DataReaderImpl::UpdateLivelinessChangedStatus(const InstanceHandle_t & handle, const bool & alive, const bool & ensure)
{
	if (!alive) {
		history_->RemoveRemoteWriter(Conversion::ToGuid(handle));
	}
	return statusMgr_->UpdateLivelinessChangedStatus(handle, alive, ensure);
}

void DataReaderImpl::SetStatus(bool add)
{
	dynamic_cast<StatusConditionImpl*>(&sub_->get_statuscondition())->set_status(DATA_ON_READERS_STATUS, add);
	dynamic_cast<StatusConditionImpl*>(&get_statuscondition())->set_status(DATA_AVAILABLE_STATUS, add);
}

SampleCounters DataReaderImpl::GetSampleCounters(bool trigger)
{
	return history_->GetSampleCounters(trigger);
}

void DataReaderImpl::NotifyReadConditions()
{
    std::vector<std::shared_ptr<ReadConditionImpl>> conditions;
    { std::lock_guard<std::mutex> lock(conditionsMutex_); conditions = readcdts_; }
	for (auto& it : conditions) {
		if (it->get_trigger_value()) {
			it->Notify();
		}
	}
}

SubscriberImpl* DataReaderImpl::getParent()
{
	return sub_;
}

void DataReaderImpl::ContentFilterUpdate()
{
	ContentFilteredTopicImpl* filteredTopic = static_cast<ContentFilteredTopicImpl*>(topic_);
	filteredTopic->get_expression_parameters(contentFilter_.expressionParameters);
	filter_->InputSQL(contentFilter_.filterExpression, contentFilter_.expressionParameters);
	if(enable_) {
		DiscoveredReaderData readerData;
#ifdef DDSSECURITY
		readerData.ddsSubscriptionData = builtinSecureData_;
#else
		readerData.ddsSubscriptionData = builtinData_;
#endif
		readerData.contentFilter = contentFilter_;
		DomainParticipantImpl* domainParticipant = dynamic_cast<DomainParticipantImpl*>(const_cast<DomainParticipant*>(sub_->get_participant()));
		std::shared_ptr<Discovery> discovery = domainParticipant->GetDiscovery();
		discovery->WriteSubscriptionData(readerData);
	}
}

bool DataReaderImpl::ValidateFilterData(SerializedBuffer* buffer)
{
    bool ret = false;
	if (filter_ != nullptr) {
		SerializedBuffer buf = *buffer;
		bool littleEndian = GetEndianess(&buf);

		// 使用DynamicDataFactory创建数据容器
		DynamicData* dyn_data = DynamicDataFactory::get_instance().create_data(*typeSupport_->get_dynamictype());
		DynamicDataTypeSupport typeSupport(typeSupport_->get_dynamictype());
		CdrDeserializer cdr(&buf, littleEndian);
		typeSupport.deserialize_data(dyn_data, &cdr, littleEndian);
		ret = filter_->Validate(dyn_data);
		DynamicDataFactory::get_instance().delete_data(*dyn_data);

	}
	return ret;
}
