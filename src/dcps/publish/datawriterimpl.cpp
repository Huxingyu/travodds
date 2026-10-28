#include "dcps/publish/datawriterimpl.h"
#include "dcps/publish/sampleloan_state.h"
#include "dcps/publish/publisherimpl.h"
#include "dcps/topic/topicimpl.h"
#include "dcps/topic/typesupport.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "qos/qospolicycheck.h"
#include "common/memory/memorypool.h"
#include "common/memory/topicpayloadpool.h"
#include <limits>
#include <new>
#include "common/time/ddstimer.h"
#include "dcps/base/statusmanager.h"
#include "rtps/endpoint/writer.h"
#include "rtps/endpoint/participant.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "rtps/endpoint/statefulwriter.h"
#include "rtps/discovery/discovery.h"
#include "rtps/discovery/datasharingendpoint.h"
#include "common/timeconversion.h"
#include "common/log/logger.h"
#ifdef DDSSECURITY
#include "security/cryptotransform.h"
#include "security/securitycommon.h"
#include "dcps/domain/domainparticipantimpl.h"
#endif // DDSSECURITY
#include "xtypes/dynamictypebuilderfactory.h"
#include "xtypes/dynamictypebuilder.h"
#include "xtypes/dynamicdatafactory.h"
#include "xtypes/dynamicdatatypesupport.h"
using namespace std::chrono;
USING_TRAVODDS_NAMESPACE

namespace {
std::shared_ptr<IPayloadPool> writerPayloadPool(Topic* topic, PublisherImpl* publisher,
        const PayloadPoolQosPolicy& qos) {
	PayloadPoolConfig config;
	config.policy = qos.policy;
	config.payload_size = qos.payload_size;
	config.max_payloads = qos.max_payloads;
	config.initial_payloads = qos.initial_payloads;
	config.alignment = qos.alignment;
	return TopicPayloadPoolRegistry::instance().getOrCreate(
		{publisher->get_participant()->get_domain_id(), topic->get_name(), topic->get_type_name()}, config);
}
}

DataWriterImpl::DataWriterImpl(Topic* topic, const DataWriterQos& qos, DataWriterListener* listener, StatusMask mask, PublisherImpl* pub, TypeSupport* typeSupport)
	: DomainEntityImpl(mask), topic_(dynamic_cast<TopicImpl*>(topic)),
	  qos_(qos), listener_(listener), pub_(pub),
	  statusMgr_( new DataWriterStatusManager(this)),
	  deadlineTimer_(nullptr),
	  lifespanTimer_(nullptr),
	  memoryPool_(MemoryPool::GetInstance()),
	  payloadPool_(writerPayloadPool(topic, pub, qos.payload_pool)),
	  typeSupport_(typeSupport)
{
	topic_->Reference();

	InstanceHandle_t handle = pub_->get_participant()->get_instance_handle();
	memcpy(builtinData_.participant_key.value, &handle.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	builtinData_.topic_name = topic->get_name();
	builtinData_.type_name = topic->get_type_name();
	builtinData_.durability = qos.durability;
	builtinData_.durability_service = qos.durability_service;
	builtinData_.deadline = qos.deadline;
	builtinData_.latency_budget = qos.latency_budget;
	builtinData_.liveliness = qos.liveliness;
	builtinData_.reliability = qos.reliability;
	builtinData_.lifespan = qos.lifespan;
	builtinData_.user_data = qos.user_data;
	builtinData_.ownership = qos.ownership;
	builtinData_.ownership_strength = qos.ownership_strength;
	builtinData_.destination_order = qos.destination_order;
	PublisherQos pubQos;
	pub_->get_qos(pubQos);
	builtinData_.presentation = pubQos.presentation;
	builtinData_.partition = pubQos.partition;
	builtinData_.group_data = pubQos.group_data;
	TopicQos topicQos;
	topic->get_qos(topicQos);
	builtinData_.topic_data = topicQos.topic_data;
	
#ifdef DDSSECURITY
	memcpy(builtinSecureData_.participant_key.value, &handle.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	builtinSecureData_.topic_name = topic->get_name();
	builtinSecureData_.type_name = topic->get_type_name();
	builtinSecureData_.durability = qos.durability;
	builtinSecureData_.durability_service = qos.durability_service;
	builtinSecureData_.deadline = qos.deadline;
	builtinSecureData_.latency_budget = qos.latency_budget;
	builtinSecureData_.liveliness = qos.liveliness;
	builtinSecureData_.reliability = qos.reliability;
	builtinSecureData_.lifespan = qos.lifespan;
	builtinSecureData_.user_data = qos.user_data;
	builtinSecureData_.ownership = qos.ownership;
	builtinSecureData_.ownership_strength = qos.ownership_strength;
	builtinSecureData_.destination_order = qos.destination_order;
	builtinSecureData_.presentation = pubQos.presentation;
	builtinSecureData_.partition = pubQos.partition;
	builtinSecureData_.group_data = pubQos.group_data;
	builtinSecureData_.topic_data = topicQos.topic_data;

	builtinSecureData_.security_info.endpoint_security_attributes = 0;
	builtinSecureData_.security_info.plugin_endpoint_security_attributes = 0;
	//builtinSecureData_.protected_discovery = false;
#endif // DDSSECURITY
}

DataWriterImpl::~DataWriterImpl()
{
    { std::lock_guard<std::mutex> schedule(lastInstanceMtx_); timersClosing_ = true; }
#if defined(__linux__)
	if (nativePool_) nativePool_->closeWriter();
#endif
	if (deadlineTimer_) {
		deadlineTimer_->Exit();
	}

	if (lifespanTimer_) {
		lifespanTimer_->Exit();
	}
	if (enable_) {
		writer_->SetRelatedDataWriter(nullptr);
	}
	topic_->Dereference();

    if (listener_) {
        //delete listener_;
        listener_ = nullptr;
    }
#ifdef DDSSECURITY
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(pub_->get_participant());
	if (dp->enableDDSSec_ && statistic_.enableSec)
	{
		GUID_t dwGuid = Conversion::ToGuid(get_instance_handle());
		//if ((dwGuid.IsSecureBuiltinEntity()
		//	&& dwGuid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER)
		//	|| statistic_.secAttributes.is_payload_protected
		//	|| statistic_.secAttributes.is_submessage_protected)
		if (dwGuid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER &&
				(statistic_.secAttributes.is_payload_protected || 
				statistic_.secAttributes.is_submessage_protected))
		{
			if (statistic_.enableSec && secPlugin_ != NULL)
			{
				SecurityException secExce;
				secPlugin_->crypto_key_factory->unregister_datawriter(
					cryptoHdl_, &secExce);
			}
		}
	}
#endif // DDSSECURITY
}


ReturnCode_t DataWriterImpl::PrepareDataSharing()
{
	if (qos_.data_sharing.kind == OFF_DATA_SHARING_QOS) return RETCODE_UNSUPPORTED;
#if defined(__linux__)
	try {
#ifdef DDSSECURITY
		auto* dp = dynamic_cast<DomainParticipantImpl*>(pub_->get_participant());
		if (dp && dp->enableDDSSec_) return RETCODE_UNSUPPORTED;
#endif
		const auto type = typeSupport_->get_native_type_info();
		if (!type.loanable()) return RETCODE_UNSUPPORTED;
		std::lock_guard<std::mutex> lock(loanMutex_);
		if (nativePool_) return RETCODE_OK;
		// Discovery metadata is immutable after enable. AUTO fallback cannot
		// silently begin advertising a new segment during a later Loan call.
		if (enable_) return RETCODE_UNSUPPORTED;
		auto capability = LocalDataSharingInfo(qos_.data_sharing, type);
		if (!capability.valid(false)) return RETCODE_UNSUPPORTED;
		if (!nativePool_) {
			SharedPoolConfig config;
			config.payload_size = type.size;
			config.alignment = std::max(uint32_t(64), type.alignment);
			config.slots = qos_.data_sharing.max_samples;
			config.max_readers = qos_.data_sharing.max_readers;
			config.max_segment_bytes = qos_.data_sharing.max_segment_bytes;
			nativePool_ = SharedPayloadPool::create(Conversion::ToGuid(handle_), config,
				qos_.data_sharing.shared_memory_directory);
		}
		const auto& descriptor = nativePool_->descriptor();
		capability.session_high = descriptor.session.high;
		capability.session_low = descriptor.session.low;
		capability.segment_bytes = descriptor.segment_bytes;
		capability.slots = descriptor.config.slots;
		capability.max_readers = descriptor.config.max_readers;
		builtinData_.data_sharing = std::move(capability);
#ifdef DDSSECURITY
		builtinSecureData_.data_sharing = builtinData_.data_sharing;
#endif
		return RETCODE_OK;
	} catch (const std::bad_alloc&) { return RETCODE_OUT_OF_RESOURCES; }
	catch (const std::invalid_argument&) { return RETCODE_INCONSISTENT_POLICY; }
	catch (const std::exception&) { return RETCODE_ERROR; }
#else
	return RETCODE_UNSUPPORTED;
#endif
}

ReturnCode_t DataWriterImpl::loan_sample(SampleLoan& loan)
{
	if (!enable_) return RETCODE_NOT_ENABLED;
	if (loan) return RETCODE_PRECONDITION_NOT_MET;
#if defined(__linux__)
	try {
		const auto prepared = PrepareDataSharing();
		if (prepared != RETCODE_OK) return prepared;
		const auto type = typeSupport_->get_native_type_info();
		std::lock_guard<std::mutex> lock(loanMutex_);
		auto state = std::make_shared<SampleLoan::State>();
		state->writer = loanIdentity_;
		state->pool = nativePool_;
		char* buffer = nullptr;
		const auto result = nativePool_->loan(type.size, state->token, buffer);
		if (result == SharedPoolResult::EXHAUSTED) return RETCODE_OUT_OF_RESOURCES;
		if (result != SharedPoolResult::OK) return RETCODE_ERROR;
		state->data = buffer;
		if (!typeSupport_->construct_native_sample(buffer)) return RETCODE_ERROR;
		loan.state_ = std::move(state);
		return RETCODE_OK;
	} catch (const std::bad_alloc&) { return RETCODE_OUT_OF_RESOURCES; }
	catch (const std::invalid_argument&) { return RETCODE_BAD_PARAMETER; }
	catch (const std::exception&) { return RETCODE_ERROR; }
#else
	return RETCODE_UNSUPPORTED;
#endif
}

ReturnCode_t DataWriterImpl::discard_loan(SampleLoan& loan)
{
	if (!loan.state_ || loan.state_->writer != loanIdentity_) return RETCODE_PRECONDITION_NOT_MET;
	loan.reset();
	return RETCODE_OK;
}

ReturnCode_t DataWriterImpl::write(SampleLoan& loan, const InstanceHandle_t& handle)
{
	if (!enable_) return RETCODE_NOT_ENABLED;
	if (!loan.state_ || loan.state_->writer != loanIdentity_) return RETCODE_PRECONDITION_NOT_MET;
    Time_t timestamp;
    DomainParticipant::get_current_time(timestamp);
    SampleIdentity identity;
    const auto result = WriteSample(loan.data(), handle, timestamp, identity, {}, loan.state_);
	if (result == RETCODE_OK) loan.reset();
	return result;
}

ReturnCode_t DataWriterImpl::set_qos(const DataWriterQos& qos)
{
    std::unique_lock<std::mutex> writes(writeMutex_);
    std::unique_lock<std::mutex> loans(loanMutex_);
    std::unique_lock<std::mutex> schedule(lastInstanceMtx_);

    DataWriterQos defaultQos;
    pub_->get_default_datawriter_qos(defaultQos);
    const DataWriterQos& qosTemp = (&qos == &DATAWRITER_QOS_DEFAULT) ? defaultQos : qos;

    if (&qos != &DATAWRITER_QOS_DEFAULT) {
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
					"dw.deadline"
                );
			}
		} else if(qosTemp.deadline.period == Duration_t(TIME_INFINITE)) {
			if (deadlineTimer_) deadlineTimer_->Stop();
            if (history_) history_->UpAllDeadline(std::chrono::steady_clock::time_point::max());
		}

		/* wangyi Lifespan 重置 */
		if (qos_.lifespan.duration != qosTemp.lifespan.duration && qosTemp.lifespan.duration != Duration_t(TIME_INFINITE))
		{
			if (lifespanTimer_) {
				lifespanTimer_->UpdatePeriod(qosTemp.lifespan.duration.ToNs() * 1e-6);
			} else {
				lifespanTimer_ = TimerManager::CreateTimer(
					qosTemp.lifespan.duration,
					[this] {
						LifespanTimeout();
					},
					"dw.lifespan"
				);
			}
		} else if(qosTemp.lifespan.duration == Duration_t(TIME_INFINITE)) {
			if (lifespanTimer_) lifespanTimer_->Stop();
		}
	}

    qos_.deadline = qosTemp.deadline;
    qos_.lifespan = qosTemp.lifespan;
    qos_.latency_budget = qosTemp.latency_budget;
    qos_.transport_priority = qosTemp.transport_priority;
    qos_.ownership_strength = qosTemp.ownership_strength;
    qos_.writer_data_lifecycle = qosTemp.writer_data_lifecycle;
    qos_.user_data = qosTemp.user_data;
    qos_.property = qosTemp.property;

	if (writer_) writer_->SetQos(qosTemp);

    builtinData_.deadline = qosTemp.deadline;
    builtinData_.latency_budget = qosTemp.latency_budget;
    builtinData_.user_data = qosTemp.user_data;
    builtinData_.lifespan = qosTemp.lifespan;
    builtinData_.ownership_strength = qosTemp.ownership_strength;
    const auto advertisement = builtinData_;
    const bool advertise = enable_;
    schedule.unlock();
    loans.unlock(); writes.unlock();
#ifndef DDSSECURITY
    if (advertise) {
        auto* domain = dynamic_cast<DomainParticipantImpl*>(const_cast<DomainParticipant*>(pub_->get_participant()));
        if (domain && domain->GetDiscovery()) {
            DiscoveredWriterData update;
            update.ddsPublicationData = advertisement;
            domain->GetDiscovery()->WritePublicationData(update);
        }
    }
#endif
    return RETCODE_OK;
}

ReturnCode_t DataWriterImpl::get_qos(DataWriterQos& qos) const
{
	std::lock_guard<std::mutex> schedule(lastInstanceMtx_);
	qos = qos_;
    return RETCODE_OK;
}

ReturnCode_t DataWriterImpl::set_listener(DataWriterListener* listener, const StatusMask& mask)
{
	if (listener_) {
		//delete listener_;
		listener_ = nullptr;
	}
	listener_ = listener;
	statusMask_ = mask;

    return  RETCODE_OK;
}

const DataWriterListener* DataWriterImpl::get_listener() const
{
    return listener_;
}

InstanceHandle_t DataWriterImpl::register_instance(const void* const instance_data)
{
	Time_t time;
	DomainParticipant::get_current_time(time);

	return register_instance_w_timestamp(instance_data, time);
}

InstanceHandle_t DataWriterImpl::register_instance_w_timestamp(const void* const instance_data, const Time_t& source_timestamp)
{
    std::lock_guard<std::mutex> lock(writeMutex_);
    if (!enable_) return HANDLE_NIL;
    InstanceHandle_t handle = HANDLE_NIL;
    return RegistInstance(instance_data, source_timestamp, handle) == RETCODE_OK ? handle : HANDLE_NIL;
}

ReturnCode_t DataWriterImpl::unregister_instance(const void* const instance_data, const InstanceHandle_t& handle)
{
	Time_t time;
	DomainParticipant::get_current_time(time);

	return unregister_instance_w_timestamp(instance_data, handle, time);
}

ReturnCode_t DataWriterImpl::unregister_instance_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enable");
		return RETCODE_NOT_ENABLED;
	}
	return UnRegisteOrDispose(instance_data, handle, source_timestamp);
}

ReturnCode_t DataWriterImpl::get_key_value(void* key_holder, const InstanceHandle_t& handle)
{
    std::lock_guard<std::mutex> lock(writeMutex_);
    if (!enable_) return RETCODE_NOT_ENABLED;
    if (!typeSupport_->has_key()) return RETCODE_ILLEGAL_OPERATION;
    if (!key_holder || handle == HANDLE_NIL) return RETCODE_BAD_PARAMETER;
    if (auto native = history_->GetNativeKey(handle)) {
        return typeSupport_->copy_data(key_holder, native.get()) == 0 ? RETCODE_OK : RETCODE_ERROR;
    }
    auto data = history_->GetSerializedKey(handle);
    if (!data) return RETCODE_BAD_PARAMETER;
    // Deserialization advances the descriptor. Keep the registered snapshot
    // reusable, and retain its owner until the local view has been consumed.
    SerializedBuffer view = *data;
    view.reader_position = 0;
    CdrDeserializer cdr(&view);
    return typeSupport_->deserialize_data(key_holder, &cdr, DEFAULT_ENDIAN) == 0 ? RETCODE_OK : RETCODE_ERROR;
}

InstanceHandle_t DataWriterImpl::lookup_instance(const void* const instance) const
{
    std::lock_guard<std::mutex> lock(writeMutex_);
    if (!enable_ || !instance || !typeSupport_->has_key()) return HANDLE_NIL;
    InstanceHandle_t handle = HANDLE_NIL;
    if (MakeKey(instance, handle, true) != RETCODE_OK || !history_->IsKeyRegistered(handle)) return HANDLE_NIL;
    return handle;
}

ReturnCode_t DataWriterImpl::write(const void* const instance_data, const InstanceHandle_t& handle)
{
	Time_t time;
	DomainParticipantImpl::get_current_time(time);
	SampleIdentity sample_identity;
    return write_w_timestamp_related_sample_identity(instance_data, handle, time, sample_identity);
}

ReturnCode_t DataWriterImpl::write_w_related_sample_identity(const void* const instance_data, const InstanceHandle_t& handle, SampleIdentity& sample_identity, const SampleIdentity& related_sample_identity)
{
	Time_t time;
	DomainParticipantImpl::get_current_time(time);
	return write_w_timestamp_related_sample_identity(instance_data, handle, time, sample_identity, related_sample_identity);
}

ReturnCode_t DataWriterImpl::write_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp)
{
	SampleIdentity sample_identity;
    return write_w_timestamp_related_sample_identity(instance_data, handle, source_timestamp, sample_identity);
}

ReturnCode_t DataWriterImpl::write_w_timestamp_related_sample_identity(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp, SampleIdentity& sample_identity, const SampleIdentity& related_sample_identity)
{
    return WriteSample(instance_data, handle, source_timestamp, sample_identity, related_sample_identity);
}

ReturnCode_t DataWriterImpl::WriteSample(const void* const instance_data, const InstanceHandle_t& handle,
        const Time_t& source_timestamp, SampleIdentity& sample_identity,
        const SampleIdentity& related_sample_identity, const std::shared_ptr<SampleLoan::State>& native)
{
    std::lock_guard<std::mutex> writeLock(writeMutex_);
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enabled");
		return RETCODE_NOT_ENABLED;
	}
	if (!instance_data) {
		return RETCODE_BAD_PARAMETER;
	}
	ReturnCode_t ret = RETCODE_ERROR;
	InstanceHandle_t instanceHandle = HANDLE_NIL;

	ParameterList_t lineQos;

	if (related_sample_identity.writer_handle != HANDLE_NIL) {
		Parameter_t param;
		param.parameterId = ParameterId::PID_CUSTOM_RELATED_SAMPLE_IDENTITY;
		param.length = 24u;
		param.value.resize(24u);
		memcpy(param.value.data(), related_sample_identity.writer_handle.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
		memcpy(param.value.data() + MIG_RTPS_KEY_HASH_MAX_LENGTH, &related_sample_identity.sequence_number, sizeof(SequenceNumber_t));
		lineQos.push_back(param);
	}

	if (typeSupport_->has_key())
	{
        ret = MakeKey(instance_data, instanceHandle, true);
        if (ret != RETCODE_OK) return ret;

		if (HANDLE_NIL != handle && !history_->IsKeyRegistered(handle)) {
			TRAVODDS_LOG(LOG_ERROR, "handle invalid");
			return RETCODE_BAD_PARAMETER;
		}

		if (HANDLE_NIL != handle && handle != instanceHandle) {
			TRAVODDS_LOG(LOG_ERROR, "handle not met");
			return RETCODE_PRECONDITION_NOT_MET;
		}

		if (HANDLE_NIL == handle) {

            ret = RegistInstance(instance_data, source_timestamp, instanceHandle);

            if (ret != RETCODE_OK) {
				/* 注册失败 可能是因为max_instance的值达到 */
				TRAVODDS_LOG(LOG_ERROR, "RegistInstance Err ret= %d", ret);
				return ret;
			}
		}
		Parameter_t param;
		param.parameterId = PID_KEY_HASH;
		param.length = MIG_RTPS_KEY_HASH_MAX_LENGTH;
		param.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + MIG_RTPS_KEY_HASH_MAX_LENGTH);
		lineQos.push_back(param);
	}

#if defined(__linux__)
    // SQL filters still require the ordinary CDR path.
    if (nativePool_ && readerFilters_.empty()) {
        const auto targets = writer_->SelectDataSharingReaders(nativePool_->registeredReaders());
        const bool retainNative = qos_.durability.kind == TRANSIENT_LOCAL_DURABILITY_QOS;
        if (!targets.empty() || retainNative) {
            auto reservation = native;
            SampleLoan copied;
            if (!reservation) {
                auto result = loan_sample(copied);
                if (result == RETCODE_OUT_OF_RESOURCES &&
                    (qos_.history.kind == KEEP_LAST_HISTORY_QOS || qos_.reliability.kind == BEST_EFFORT_RELIABILITY_QOS) &&
                    history_->IsFull(instanceHandle)) {
                    history_->RemoveOldestChangeByInstance(instanceHandle);
                    result = loan_sample(copied);
                }
                if (result != RETCODE_OK && qos_.data_sharing.kind == ON_DATA_SHARING_QOS) return result;
                if (result == RETCODE_OK) {
                    reservation = copied.state_;
                    std::memcpy(reservation->data, instance_data, typeSupport_->get_native_type_info().size);
                }
            }
            if (reservation) {
                auto cache = writer_->NewChange(ALIVE, nullptr, instanceHandle, source_timestamp, sample_identity, payloadPool_);
                cache->nativePayload = std::shared_ptr<void>(reservation, reservation->data);
                cache->nativeType = typeSupport_->get_native_type_info();
                cache->nativeReaders.insert(targets.begin(), targets.end());
                cache->nativeReaderActive = [pool = reservation->pool, targets, retainNative](const GUID_t& reader) {
                    const bool initial = std::find(targets.begin(), targets.end(), reader) != targets.end();
                    if (!initial && !retainNative) return false;
                    // A late registration can skip CDR only when it requested
                    // replay. VOLATILE registrations have no historical pin.
                    const auto registered = pool->registeredReaders(!initial);
                    return std::find(registered.begin(), registered.end(), reader) != registered.end();
                };
                cache->inlineQos = lineQos;
                cache->related_sample_identity = related_sample_identity;
                SharedSampleMetadata metadata;
                metadata.source_seconds = source_timestamp.sec;
                metadata.source_nanoseconds = source_timestamp.nanosec;
                std::memcpy(metadata.instance, instanceHandle.keyHash.value, 16);
                std::memcpy(metadata.related_writer, related_sample_identity.writer_handle.keyHash.value, 16);
                metadata.related_sequence = (uint64_t(uint32_t(related_sample_identity.sequence_number.high)) << 32) |
                    related_sample_identity.sequence_number.low;
                cache->nativeCommit = [reservation, targets, metadata, retainNative](CacheChange& change) {
                    const uint64_t sequence = (uint64_t(uint32_t(change.sequenceNumber.high)) << 32) | change.sequenceNumber.low;
                    const auto result = reservation->pool->publish(reservation->token, change.nativeType.size, sequence, metadata, retainNative ? nullptr : &targets);
                    return result == SharedPoolResult::OK ? RETCODE_OK : RETCODE_ERROR;
                };
                cache->serializeNative = [reservation, type = typeSupport_, pool = payloadPool_](CacheChange& change) {
                    const uint32_t size = type->get_serialized_data_size(reservation->data, 0);
                    SerializedBuffer* buffer = nullptr;
                    const auto result = CreateSerializedPayload(pool, size, buffer);
                    if (result != RETCODE_OK) return result;
                    PayloadBufferPtr owner(buffer, PayloadBufferDeleter{pool});
                    CdrSerializer cdr(buffer);
                    if (type->serialize_data(reservation->data, &cdr, DEFAULT_ENDIAN) != 0) return RETCODE_ERROR;
                    buffer->buffer -= CDR_ENDIANNESS_LEN;
                    buffer->buffer_size += CDR_ENDIANNESS_LEN;
                    change.dataSize = size;
                    change.fragmentTatolNum = size / DATAMESSAGE_LIMIT + (size % DATAMESSAGE_LIMIT != 0);
                    change.fraqmentSize = DATAMESSAGE_LIMIT;
                    change.serializedBuffer = owner.release();
                    return RETCODE_OK;
                };
                if (writer_->HasRtpsReaders(targets)) {
                    const auto serialized = cache->EnsureSerializedPayload();
                    if (serialized != RETCODE_OK) return serialized;
                }
                const auto accepted = writer_->AddNewCachce(cache);
                if (accepted == RETCODE_OK) {
                    try { AfterWrite(instanceHandle); }
                    catch (...) { TRAVODDS_LOG(LOG_WARNING, "DataSharing accepted sample; QoS timer update failed"); }
                }
                return accepted;
            }
        }
    }
#else
    (void)native;
#endif

	uint32_t size = typeSupport_->get_serialized_data_size((void*)instance_data, 0);

	SerializedBuffer* bufferHeader = nullptr;
	ret = CreateSerializedBuffer(bufferHeader, size);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "CreateSerializedBuffer Err ret= %d", ret);
		return ret;
	}

	PayloadBufferPtr payloadOwner(bufferHeader, PayloadBufferDeleter{payloadPool_});
	CdrSerializer cdr(bufferHeader);
	if (typeSupport_->serialize_data((void*)instance_data, &cdr, DEFAULT_ENDIAN) != 0) {
		return RETCODE_ERROR;
	}
	std::set<GUID_t> filterReaders;
	if (!readerFilters_.empty()) {

		// 使用DynamicDataFactory创建数据容器
		DynamicData* dyn_data = DynamicDataFactory::get_instance().create_data(*typeSupport_->get_dynamictype());
		DynamicDataTypeSupport typeSupport(typeSupport_->get_dynamictype());
		CdrDeserializer dycdr(bufferHeader);
		typeSupport.deserialize_data(dyn_data, &dycdr, DEFAULT_ENDIAN);
		for (auto& it : readerFilters_) {
			if (it.second->Validate(dyn_data)) {
				filterReaders.insert(it.first);
			}
		}
		DynamicDataFactory::get_instance().delete_data(*dyn_data);
	}

	bufferHeader->buffer -= CDR_ENDIANNESS_LEN;
	bufferHeader->buffer_size += CDR_ENDIANNESS_LEN;

#ifdef DDSSECURITY
	//SerializedBuffer* encodeBuffer = new SerializedBuffer;
	SerializedBuffer* encodeBuffer = nullptr;
	SerializedBuffer* encodeBufferHeader = nullptr;
	PayloadBufferPtr encodedOwner(nullptr, PayloadBufferDeleter{payloadPool_});
	PayloadBufferPtr encodedParts(nullptr, PayloadBufferDeleter{payloadPool_});
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(pub_->get_participant());
	if (!dp) {
		TRAVODDS_LOG(LOG_ERROR, "Writer can not get_participant from Publisher", ret);
		return RETCODE_ERROR;
	}
	SecurityException secExce;
	SerializedBuffer inlineQos;
	if (statistic_.secAttributes.is_payload_protected)
	{
		//计算需要加密的buffer链表长度
		uint32_t bufferNum = 1; //需要加密的buffer链表长度
		uint32_t totalLen = bufferHeader->buffer_size; //链表字节总数
		SerializedBuffer* curBuffer = bufferHeader;
		while (curBuffer->next) {
			curBuffer = curBuffer->next;
			bufferNum++;
			totalLen += curBuffer->buffer_size;
		}

		CryptoTransform* cryptoTransPtr = dp->secPlugin_.crypto_transform;
		//开辟加密后缓存空间
		//encodeBuffer->buffer = new char[encodeSize];
		//encodeBuffer->buffer_size = encodeSize;
		int encodeSize = totalLen * 10 + 500;
		ret = CreateSerializedBuffer(encodeBuffer, encodeSize);
		if (RETCODE_OK != ret) {
			TRAVODDS_LOG(LOG_ERROR, "CreateSerializedBuffer Err ret= %d", ret);
			return ret;
		}

		encodedOwner.reset(encodeBuffer);

		if (!cryptoTransPtr->encode_serialized_payload(
			encodeBuffer,
			NULL,
			//&inlineQos,
			bufferHeader,
			bufferNum,
			cryptoHdl_,
			&secExce))
		{
			TRAVODDS_LOG(LOG_ERROR,
				"Write failed: failed to encode payload in DataWriter %s of Topic."
				"\n  Code: %d"
				"\n  Minor code: %d"
				"\n  Message: %s",
				secExce.code,
				secExce.minor_code,
				secExce.message);
			return RETCODE_ERROR;
		}
		size = encodeBuffer->buffer_size - 4; //size为减去长为CDR_ENDIANNESS_LEN的端序信息
		//加密后的单个encodeBuffer长度超限变为链表
		int encodeSubBufferNum = encodeBuffer->buffer_size / DATAMESSAGE_LIMIT;//size / DATAMESSAGE_LIMIT;
		if (encodeBuffer->buffer_size % DATAMESSAGE_LIMIT != 0) {
			encodeSubBufferNum += 1;
		}

		SerializedBuffer* curEncodeSubBuffer = NULL;
		SerializedBuffer* preEncodeSubBuffer = NULL;
		uint32_t curEncodeSubBufferSize = 0;
		uint32_t encodeBufferShift = 0;
		uint32_t encodeBufferSize = encodeBuffer->buffer_size;
		//SerializedBuffer* encodeBufferHeader = new SerializedBuffer;
		//逐个构造链表的每一部分buffer
		for (int i = 0; i < encodeSubBufferNum; ++i) {
			//curEncodeSubBuffer = new SerializedBuffer;
			curEncodeSubBuffer = nullptr; //curEncodeSubBuffer指向当前部分buffer
			if (encodeBufferSize >= DATAMESSAGE_LIMIT * (i + 1)) {
				curEncodeSubBufferSize = DATAMESSAGE_LIMIT;
			}
			else {
				curEncodeSubBufferSize = encodeBufferSize % DATAMESSAGE_LIMIT;
			}
			//curEncodeSubBuffer->buffer = new char[curEncodeSubBufferSize];
			//curEncodeSubBuffer->buffer_size = curEncodeSubBufferSize;
			ret = CreateSerializedBuffer(curEncodeSubBuffer, curEncodeSubBufferSize); //创建当前部分buffer
			if (RETCODE_OK != ret) {
				TRAVODDS_LOG(LOG_ERROR, "CreateSerializedBuffer Err ret= %d", ret);
				return ret;
			}
			//从encodeBuffer指向的buffer取出对应部分（根据偏移encodeBufferShift和计算出的长度curEncodeSubBufferSize），赋值给当前buffer
			memcpy(curEncodeSubBuffer->buffer, encodeBuffer->buffer + encodeBufferShift, curEncodeSubBufferSize);
			encodeBufferShift += curEncodeSubBufferSize; //更新偏移值encodeBufferShift
			if (i == 0) { //如果当前部分buffer是第一个
				encodeBufferHeader = curEncodeSubBuffer;
				encodedParts.reset(encodeBufferHeader);
				preEncodeSubBuffer = curEncodeSubBuffer;
			}
			else {
				preEncodeSubBuffer->next = curEncodeSubBuffer;
				preEncodeSubBuffer = curEncodeSubBuffer;
			}
		}

		//delete[]encodeBuffer->buffer;
		encodedOwner.reset();
	}
	std::shared_ptr<CacheChange> cacheChange;
	if (statistic_.secAttributes.is_payload_protected) {
		cacheChange = writer_->NewChange(ALIVE, encodeBufferHeader, instanceHandle, source_timestamp, payloadPool_);
		encodedParts.release();
	}
	else {
		cacheChange = writer_->NewChange(ALIVE, bufferHeader, instanceHandle, source_timestamp, payloadPool_);
		payloadOwner.release();
	}
#else
	std::shared_ptr<CacheChange> cacheChange = writer_->NewChange(ALIVE, bufferHeader, instanceHandle, source_timestamp, sample_identity, payloadPool_);
	payloadOwner.release();
#endif // DDSSECURITY
	if (nullptr == cacheChange) {
		TRAVODDS_LOG(LOG_ERROR, "cacheChange ret= %d", ret);
		return RETCODE_ERROR;
	}

	cacheChange->filterReaders_ = std::move(filterReaders);

	cacheChange->dataSize = size;
	cacheChange->inlineQos = lineQos;
	if (size > 0) {
		cacheChange->fragmentTatolNum = size / DATAMESSAGE_LIMIT;
		if (0 != size % DATAMESSAGE_LIMIT) {
			cacheChange->fragmentTatolNum += 1;
		}
		cacheChange->fraqmentSize = DATAMESSAGE_LIMIT;
	}

	ret = writer_->AddNewCachce(cacheChange);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "AddNewCachce Err ret= %d", ret);
		return ret;
	}

    AfterWrite(instanceHandle);
    return ret;
}

void DataWriterImpl::AfterWrite(const InstanceHandle_t& handle)
{
    std::lock_guard<std::mutex> schedule(lastInstanceMtx_);
    if (timersClosing_) return;
    history_->MarkDeadlineActive(handle);
	if (qos_.deadline.period != Duration_t(TIME_INFINITE)) {
		std::chrono::steady_clock::time_point nextTime =
			std::chrono::steady_clock::now() + std::chrono::nanoseconds(qos_.deadline.period.ToNs());
		history_->SetNextDeadline(handle, nextTime);
        if (history_->GetNextDeadline(lastInstance_, nextTime)) {
            const auto delay = std::chrono::ceil<std::chrono::milliseconds>(nextTime - std::chrono::steady_clock::now()).count();
            deadlineTimer_->UpdatePeriod(std::max<int64_t>(1, delay));
        }
	}

	/* wangyi lifespan qos */
	if (qos_.lifespan.duration != Duration_t(TIME_INFINITE))
	{
		lifespanTimer_->UpdatePeriod(qos_.lifespan.duration.ToNs() * 1e-6);
		lifespanTimer_->ReStart();
	}
}

ReturnCode_t DataWriterImpl::dispose(const void* const instance_data, const InstanceHandle_t& handle)
{
	Time_t cur_time = { 1,1 };
	//TODO 
	DomainParticipant::get_current_time(cur_time);
	return dispose_w_timestamp(instance_data, handle, cur_time);
}

ReturnCode_t DataWriterImpl::dispose_w_timestamp(const void* const data, const InstanceHandle_t& handle, const Time_t& source_timestamp)
{
	if (!enable_) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enabled");
		return RETCODE_NOT_ENABLED;
	}
    return UnRegisteOrDispose(data, handle, source_timestamp, true);
}
ReturnCode_t DataWriterImpl::UnRegisteOrDispose(const void* const data, const InstanceHandle_t& handle, const Time_t& timespace, bool dispose)
{
    std::lock_guard<std::mutex> lock(writeMutex_);
	ReturnCode_t ret = RETCODE_ERROR;
	InstanceHandle_t instanceHandle = HANDLE_NIL;
	SerializedBuffer *buffer = nullptr;
	ChangeKind_t kind = ALIVE;
	StatusInfo_t status = { 0 };

	//InstanceHandle_t instance_handle;
	if (!typeSupport_->has_key()) {
		TRAVODDS_LOG(LOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return RETCODE_ILLEGAL_OPERATION;
	}
	
	if (timespace.sec < 0) {
		TRAVODDS_LOG(LOG_ERROR, "timespace.sec[%d] < 0", timespace.sec);
		return RETCODE_BAD_PARAMETER;
	}
    if (!data) {
        if (handle == HANDLE_NIL || !history_->IsKeyRegistered(handle)) return RETCODE_BAD_PARAMETER;
        instanceHandle = handle;
        ret = RETCODE_OK;
    } else {
        ret = MakeKey(data, instanceHandle, true);
    }
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "handle bad parameter")
		return RETCODE_BAD_PARAMETER;
	}

	/* (生成Key错误 && 不是注册的key && (传入的句柄不是HANDLE_NIL &&  传入的handle和实际不一致 ) */
	if (handle != HANDLE_NIL && (!history_->IsKeyRegistered(handle))) {
		TRAVODDS_LOG(LOG_ERROR, "handle bad parameter");
		return RETCODE_BAD_PARAMETER;
	}

	/*  传入的handle和实际不一致  */
	if (handle != instanceHandle && handle != HANDLE_NIL) {
		TRAVODDS_LOG(LOG_ERROR, "handle not met");
		return RETCODE_PRECONDITION_NOT_MET;
	}

    if (!history_->IsKeyRegistered(instanceHandle)) return RETCODE_PRECONDITION_NOT_MET;

	if (dispose) {
		kind = NOT_ALIVE_DISPOSED;
		status[3] = StatusInfoFlag::DISPOSED_FLAG;
	} else {
		kind = NOT_ALIVE_UNREGISTERED;
		status[3] = StatusInfoFlag::UNREGISTERED_FLAG;
		if (qos_.writer_data_lifecycle.autodispose_unregistered_instances) {
			kind = NOT_ALIVE_DISPOSED_UNREGISTERED;
			status[3] |= StatusInfoFlag::DISPOSED_FLAG;
		}
	}
	Parameter_t keyParam, statueParam;
	keyParam.parameterId = PID_KEY_HASH;
	keyParam.length = MIG_RTPS_KEY_HASH_MAX_LENGTH;
	keyParam.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + MIG_RTPS_KEY_HASH_MAX_LENGTH);

	statueParam.parameterId = PID_STATUS_INFO;
	statueParam.length = sizeof(StatusInfo_t);
	statueParam.value.assign(status, status + sizeof(StatusInfo_t));


	PayloadBufferPtr payloadOwner(new (std::nothrow) SerializedBuffer, PayloadBufferDeleter{payloadPool_});
	if (!payloadOwner || !payloadPool_->getBuffer(BUFF_SHIFT * 2, payloadOwner->origin_buffer)) {
		return RETCODE_APPLY_MEM_ERR;
	}
	buffer = payloadOwner.get();
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;
	std::shared_ptr<CacheChange> cacheChange = writer_->NewChange(kind, buffer, instanceHandle, timespace, payloadPool_);
	payloadOwner.release();
	if (nullptr == cacheChange) {
		TRAVODDS_LOG(LOG_ERROR, "NewChange Err");
		return RETCODE_ERROR;
	}
	cacheChange->inlineQos.push_back(keyParam);
	cacheChange->inlineQos.push_back(statueParam);
	ret = writer_->AddNewCachce(cacheChange);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "AddNewCachce Err ret = %d", ret);
		return ret;
	}
	if (!dispose) {
		/* 在发送 */
		writer_->RemoveCacheByHandle(instanceHandle);
        std::lock_guard<std::mutex> deadlines(lastInstanceMtx_);
        history_->DeleteKeyHistory(instanceHandle);
	} else {
        std::lock_guard<std::mutex> deadlines(lastInstanceMtx_);
        history_->SetNextDeadline(instanceHandle, std::chrono::steady_clock::time_point::max());
    }

	return ret;
}

ReturnCode_t DataWriterImpl::wait_for_acknowledgments(const Duration_t& max_wait)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enable");
		return RETCODE_NOT_ENABLED;
	}
	return writer_->WaitForAcknowledgments(max_wait);
}

ReturnCode_t DataWriterImpl::get_liveliness_lost_status(LivelinessLostStatus& status)
{
	return statusMgr_->GetLivelinessLostStatus(status);
}

ReturnCode_t DataWriterImpl::get_offered_deadline_missed_status(OfferedDeadlineMissedStatus& status)
{
	return statusMgr_->GetOfferedDeadlineMissedStatus(status);
}

ReturnCode_t DataWriterImpl::get_offered_incompatible_qos_status(OfferedIncompatibleQosStatus& status)
{
	return statusMgr_->GetOfferedIncompatibleQosStatus(status);
}

ReturnCode_t DataWriterImpl::get_publication_matched_status(PublicationMatchedStatus& status)
{
	return statusMgr_->GetPublicationMatchedStatus(status);
}

Topic* DataWriterImpl::get_topic() const
{
	return topic_;
}

const Publisher* DataWriterImpl::get_publisher() const
{
	return pub_;
}

ReturnCode_t DataWriterImpl::assert_liveliness()
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enable");
		return RETCODE_NOT_ENABLED;
	}

	if (!(dynamic_cast<DomainParticipantImpl*>(pub_->get_participant()))->GetLivelinessManager()->AssertLiveliness(writer_->GetGuid(),true, qos_.liveliness.kind)) {
		TRAVODDS_LOG(LOG_ERROR, "Could not assert liveliness of write");
		return RETCODE_ERROR;
	}

	if (qos_.liveliness.kind == MANUAL_BY_TOPIC_LIVELINESS_QOS) {
		std::shared_ptr<StatefulWriter> statefulWriter = std::dynamic_pointer_cast<StatefulWriter>(writer_);

		if (statefulWriter != nullptr) {
			/* wangyi 发送HB */
			statefulWriter->WriteHeartbeat(true);
		}
	}
	return RETCODE_OK;
}

ReturnCode_t DataWriterImpl::get_matched_subscription_data(SubscriptionBuiltinTopicData& subscription_data, const InstanceHandle_t& subscription_handle)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enable");
		return RETCODE_NOT_ENABLED;
	}
	if (writer_->GetMatchedSubscriptionData(subscription_data, subscription_handle)) {
		return RETCODE_OK;
	}
	return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DataWriterImpl::get_matched_subscriptions(InstanceHandleSeq& subscription_handles)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enable");
		return RETCODE_NOT_ENABLED;
	}
	writer_->GetMatchedSubscriptions(subscription_handles);
	return RETCODE_OK;
}

ReturnCode_t DataWriterImpl::enable()
{
	if (enable_) {
		return RETCODE_OK;
	}
	if (!pub_->IsEnable()) {
		TRAVODDS_LOG(LOG_ERROR, "releate publisher isnot enable");
		return RETCODE_PRECONDITION_NOT_MET;
	}


	DomainParticipantImpl* domainParticipant = dynamic_cast<DomainParticipantImpl*>(const_cast<DomainParticipant*>(pub_->get_participant()));
	if (nullptr == domainParticipant) {
		return RETCODE_ERROR;
	}

	const auto sharing = PrepareDataSharing();
	if (qos_.data_sharing.kind == ON_DATA_SHARING_QOS && sharing != RETCODE_OK) return sharing;
	if (qos_.data_sharing.kind == AUTO_DATA_SHARING_QOS && sharing != RETCODE_OK &&
		sharing != RETCODE_UNSUPPORTED)
		TRAVODDS_LOG(LOG_WARNING, "DataSharing unavailable (%d); retaining RTPS", sharing);

	FlowControllerConfig flowContropllerConfig;
	flowContropllerConfig.kind = (FlowControllerKind)qos_.flow_controller_config.kind;

    std::shared_ptr<Writer>  writerPtr = domainParticipant->GetRelatedParticipant()->CreateWriter(this, qos_, Conversion::ToGuid(handle_), typeSupport_->has_key(), flowContropllerConfig);
	if (!writerPtr.get()) {
		return RETCODE_ERROR;
	}

	if (qos_.deadline.period != Duration_t(TIME_INFINITE)) {
        deadlineTimer_ = TimerManager::CreateTimer(
			qos_.deadline.period,
			[this] {
				DeadlineTimeout();
			},
			"dw.deadline"
        );
	}

	/* wangyi lifespan Qos 定时器添加  */
	if (qos_.lifespan.duration != Duration_t(TIME_INFINITE)) {
		lifespanTimer_ = TimerManager::CreateTimer(
			qos_.lifespan.duration,
			[this] {
				LifespanTimeout();
		},
			"dw.lifespan"
		);
		// 如果它是从持久性数据库加载的，则使旧样本过期。
		if (LifespanTimeout())
		{
			lifespanTimer_->ReStart();
		}
	}

	enable_ = true;

	std::shared_ptr<Discovery> discovery = domainParticipant->GetDiscovery();
#ifdef DDSSECURITY
	if ((builtinSecureData_.security_info.endpoint_security_attributes & 
		ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED) != 0) {
		DiscoveredWriterSecureData writerSecureData;
		writerSecureData.ddsPublicationData = GetBuiltinTopicSecureData();
		discovery->WritePublicationSecureData(writerSecureData);
	}
	else {
		DiscoveredWriterData writerData;
		writerData.ddsPublicationData = GetBuiltinTopicSecureData();
		discovery->WritePublicationData(writerData);
	}
	
#else
	//本地匹配完成后再发送SEPD报文，防止重复匹配
	DiscoveredWriterData writerData;
	writerData.ddsPublicationData = GetBuiltinTopicData();
	discovery->WritePublicationData(writerData);
#endif // DDSSECURITY

	return RETCODE_OK;
}
void DataWriterImpl::OnOfferedIncompatibleQos(std::set<QosPolicyId_t>& qosSeq)
{
	statusMgr_->UpdateOfferedIncompatibleQosStatus(qosSeq);
}

void DataWriterImpl::OnPublicationMatched(InstanceHandle_t& handle, const bool& add)
{
	statusMgr_->UpdatePublicationMatchedStatus(handle, add);

}

PublicationBuiltinTopicData DataWriterImpl::GetBuiltinTopicData()
{
    std::lock_guard<std::mutex> schedule(lastInstanceMtx_);
	return builtinData_;
}
#ifdef DDSSECURITY
const PublicationBuiltinTopicSecureData& DataWriterImpl::GetBuiltinTopicSecureData()
{
    return builtinSecureData_;
}
#endif // DDSSECURITY
void DataWriterImpl::SetRelatedWriter(std::shared_ptr<Writer> writer) 
{
	history_ = writer->GetHistoryCache();


	writer_ = writer;
	writer->SetRelatedDataWriter(this);

	handle_ = Conversion::ToInstanceHandle(writer->GetGuid());
	builtinData_.key = Conversion::ToBuiltinTopicKey(handle_);
#ifdef DDSSECURITY
	builtinSecureData_.key = Conversion::ToBuiltinTopicKey(handle_);
#endif // DDSSECURITY
}


ReturnCode_t DataWriterImpl::MakeKey(const void* const data, InstanceHandle_t& handle, bool forceMd5) const
{
    handle = HANDLE_NIL;
    if (!data) return RETCODE_BAD_PARAMETER;
    try {
        return typeSupport_->MakeKey(data, handle, forceMd5) == 0 ? RETCODE_OK : RETCODE_BAD_PARAMETER;
    } catch (const std::bad_alloc&) { return RETCODE_OUT_OF_RESOURCES; }
    catch (const std::exception&) { return RETCODE_ERROR; }
}


void DataWriterImpl::DeadlineTimeout()
{
	InstanceHandle_t instance;
	{
		std::lock_guard<std::mutex> lock(lastInstanceMtx_);
        if (timersClosing_ || !deadlineTimer_ || qos_.deadline.period == Duration_t(TIME_INFINITE)) return;
        std::chrono::steady_clock::time_point due;
        if (!history_->GetNextDeadline(instance, due)) return;
        const auto now = std::chrono::steady_clock::now();
        const bool expired = due <= now;
        if (expired && !history_->SetNextDeadline(instance,
                now + std::chrono::nanoseconds(qos_.deadline.period.ToNs()))) return;
        if (history_->GetNextDeadline(lastInstance_, due)) {
            const auto delay = std::chrono::ceil<std::chrono::milliseconds>(due - std::chrono::steady_clock::now()).count();
            deadlineTimer_->UpdatePeriod(std::max<int64_t>(1, delay));
        }
        if (!expired) return; // A write can supersede an already queued wakeup.
	}
    // Listeners may write/unregister; do not hold the scheduling mutex here.
	statusMgr_->UpdateOfferedDeadlineMissedStatus(instance);
}

bool DataWriterImpl::LifespanTimeout()
{
    std::lock_guard<std::mutex> schedule(lastInstanceMtx_);
    if (timersClosing_ || !writer_ || !lifespanTimer_ || qos_.lifespan.duration == Duration_t(TIME_INFINITE)) return false;
	Duration_t currentTime;
	Duration_t::now(currentTime);
	std::shared_ptr<CacheChange> earliestChange = writer_->GetWriterCache()->GetMinSourceTimespace();
	while (earliestChange != nullptr) {
		
		Duration_t expirationTime = earliestChange->sourceTimespace + qos_.lifespan.duration;

		/* 检查最早的报文缓存是否已过期（启动计时器的报文缓存可能已从历史记录中删除）*/
		if (currentTime < expirationTime)
		{
			Duration_t interval = expirationTime - currentTime;
			lifespanTimer_->UpdatePeriod(interval.ToNs() * 1e-6);
			return true;
		}
		else {
			// 最早的报文缓存已过期，删除该报文。
			writer_->GetWriterCache()->RemoveChange(earliestChange->sequenceNumber);
			earliestChange = writer_->GetWriterCache()->GetMinSourceTimespace();
		}
	}
	return false;
}

ReturnCode_t DataWriterImpl::RegistInstance(const void* const instance_data, const Time_t& source_timestamp, InstanceHandle_t& handle)
{
    handle = HANDLE_NIL;
    if (!instance_data || source_timestamp.sec < 0) return RETCODE_BAD_PARAMETER;
    if (!typeSupport_->has_key()) return RETCODE_ILLEGAL_OPERATION;
    auto result = MakeKey(instance_data, handle, true);
    if (result != RETCODE_OK) return result;
    if (history_->IsKeyRegistered(handle)) return RETCODE_OK;
    try {
        std::shared_ptr<void> native;
        std::shared_ptr<SerializedBuffer> serialized;
#if defined(__linux__)
        if (nativePool_) {
            const auto info = typeSupport_->get_native_type_info();
            const auto alignment = std::align_val_t(std::max(info.alignment, uint32_t(alignof(std::max_align_t))));
            native = std::shared_ptr<void>(::operator new(info.size, alignment),
                [alignment](void* p) { ::operator delete(p, alignment); });
            if (!typeSupport_->construct_native_sample(native.get())) return RETCODE_ERROR;
            std::memcpy(native.get(), instance_data, info.size);
        } else
#endif
        {
            serialized = std::shared_ptr<SerializedBuffer>(new SerializedBuffer, PayloadBufferDeleter{memoryPool_});
            serialized->buffer_size = typeSupport_->get_serialized_data_size(const_cast<void*>(instance_data), 0);
            if (!memoryPool_->getBuffer(serialized->buffer_size, serialized->origin_buffer)) return RETCODE_OUT_OF_RESOURCES;
            serialized->buffer = serialized->origin_buffer;
            CdrSerializer cdr(serialized.get());
            if (typeSupport_->serialize_data(const_cast<void*>(instance_data), &cdr, DEFAULT_ENDIAN) != 0) return RETCODE_ERROR;
        }
        return history_->RegisterKey(handle, serialized, native);
    } catch (const std::bad_alloc&) { return RETCODE_OUT_OF_RESOURCES; }
    catch (const std::exception&) { return RETCODE_ERROR; }
}
ReturnCode_t DataWriterImpl::UpdateLivelinessLostStatus()
{
	return statusMgr_->UpdateLivelinessLostStatus();

}


ReturnCode_t DataWriterImpl::CreateSerializedBuffer(SerializedBuffer*& bufferHeader, uint32_t dataLen)
{
	return CreateSerializedPayload(payloadPool_, dataLen, bufferHeader);
}


bool DataWriterImpl::AddReaderFilter(const GUID_t& readerGuid, const ContentFilterProperty_t& filterInfo)
{
	using namespace DDSSQL;
	auto filter = readerFilters_.emplace(readerGuid, std::make_shared<DDSSQLFilter>(typeSupport_->get_dynamictype()));
	assert(filter.second);
	ReturnCode_t ret = RETCODE_ERROR;
	if (filter.second) { 
		ret = filter.first->second->InputSQL(filterInfo.filterExpression, filterInfo.expressionParameters);
	}
	return ret == RETCODE_OK ? true : false;
}

void DataWriterImpl::UpdateReaderFilter(const GUID_t& readerGuid, const ContentFilterProperty_t& filterInfo)
{
	using namespace DDSSQL;
	auto it = readerFilters_.find(readerGuid);
	if (it != readerFilters_.end()) {
		it->second->InputSQL(filterInfo.filterExpression, filterInfo.expressionParameters);
	}
}

void DataWriterImpl::RemoveReaderFilter(const GUID_t& readerGuid)
{
	readerFilters_.erase(readerGuid);
}

#ifdef DDSSECURITY
PublisherImpl* DataWriterImpl::GetParent()
{
	return pub_;
}

ReturnCode_t DataWriterImpl::GetSecAttributes()
{
	SecurityException secExce;
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(pub_->get_participant());
	if (dp->enableDDSSec_) {
		secPlugin_ = &dp->secPlugin_;
		GUID_t guid = Conversion::ToGuid(handle_);
		EndpointSecurityAttributes secAttr;
		if ((guid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER &&
			guid.IsSecureBuiltinEntity())
			|| (guid.IsUserDefinedEntity()))
		{
			if (dp->secPlugin_.crypto_key_factory == NULL) {
				return RETCODE_ERROR;
			}
			PublisherQos qos;
			pub_->get_qos(qos);
			if (!secPlugin_->access_control->get_datawriter_sec_attributes(
				dp->permissionHdl_,
				topic_->get_name(),
				&qos.partition,
				NULL,
				&secAttr,
				&secExce))
			{
				return RETCODE_ERROR;
			}
			statistic_.secAttributes.ac_endpoint_properties = secAttr.ac_endpoint_properties;
			statistic_.secAttributes.is_write_protected = secAttr.is_write_protected;
			statistic_.secAttributes.is_read_protected = secAttr.is_read_protected;
			statistic_.secAttributes.is_discovery_protected = secAttr.is_discovery_protected;
			statistic_.secAttributes.is_liveliness_protected = secAttr.is_liveliness_protected;
			statistic_.secAttributes.is_payload_protected = secAttr.is_payload_protected;
			statistic_.secAttributes.is_key_protected = secAttr.is_key_protected;
			statistic_.secAttributes.is_submessage_protected = secAttr.is_submessage_protected;
			statistic_.secAttributes.plugin_endpoint_attributes = secAttr.plugin_endpoint_attributes;

			builtinSecureData_.security_info.plugin_endpoint_security_attributes = secAttr.plugin_endpoint_attributes;

			secPlugin_->access_control->return_datawriter_sec_attributes(&secAttr, &secExce);
			if (guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER)
			{
				statistic_.rtpsMsgProtected = false;
			}
			else
			{
				statistic_.rtpsMsgProtected = dp->secAttributes_.is_rtps_protected;
			}
		}
		else
		{
			statistic_.secAttributes.is_write_protected = false;
			statistic_.secAttributes.is_read_protected = false;
			statistic_.secAttributes.is_discovery_protected = false;
			statistic_.secAttributes.is_liveliness_protected = false;
			statistic_.secAttributes.is_payload_protected = false;
			statistic_.secAttributes.is_key_protected = false;
			statistic_.secAttributes.is_submessage_protected = false;
			statistic_.rtpsMsgProtected = false;
		}
		statistic_.enableSec = false;
		//TODO
		if ((guid.IsSecureBuiltinEntity()
			&& guid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER)
			|| statistic_.secAttributes.is_write_protected
			|| statistic_.secAttributes.is_read_protected
			|| statistic_.secAttributes.is_discovery_protected
			//|| statistic_.secAttributes.is_liveliness_protected //PND 目前is_liveliness_protected初始化了，但是没做功能适配
			|| statistic_.secAttributes.is_submessage_protected
			|| statistic_.secAttributes.is_payload_protected
			//|| statistic_.secAttributes.is_key_protected //PND 目前is_liveliness_protected初始化了，但是没做功能适配
			) 
		{
			if (guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
				//unsigned int tmplen = statistic_.secAttributes.ac_endpoint_properties.size();
				std::string tmpName = "dds.sec.builtin_endpoint_name";
				std::string tmpValue = "BuiltinParticipantVolatileMessageSecureWriter";
				Property_t tmpProperty;
				tmpProperty.name = tmpName;
				tmpProperty.value = tmpValue;
				statistic_.secAttributes.ac_endpoint_properties.push_back(tmpProperty);
			}
			// register_local_datawriter
			cryptoHdl_ = secPlugin_->crypto_key_factory->register_local_datawriter(
				dp->cryptoHdl_,
				&statistic_.secAttributes.ac_endpoint_properties,
				&secExce);
			if (cryptoHdl_ == SEC_NIL_HANDLE)
			{
				return RETCODE_ERROR;
			}
			if (statistic_.secAttributes.is_payload_protected)
			{
				secPlugin_->crypto_transform->get_payload_encode_attributes(
					cryptoHdl_,
					&statistic_.payloadEncodeAttr);
			}
			if (statistic_.secAttributes.is_submessage_protected)
			{
				secPlugin_->crypto_transform->get_datawriter_submessage_encode_attributes(
					cryptoHdl_,
					&statistic_.subMsgEncodeAttr);
			}
			if (statistic_.rtpsMsgProtected)
			{
				secPlugin_->crypto_transform->get_rtps_message_encode_attributes(
					dp->cryptoHdl_,
					&statistic_.rtpsMsgEncodeAttr);
			}
			statistic_.enableSec = true;
			if (statistic_.secAttributes.is_payload_protected
				|| statistic_.secAttributes.is_submessage_protected
				|| statistic_.rtpsMsgProtected)
			{
				statistic_.encryptoMsg = true;
			}
		}
	}
	else
	{
		statistic_.enableSec = false;
		statistic_.secAttributes.is_write_protected = false;
		statistic_.secAttributes.is_read_protected = false;
		statistic_.secAttributes.is_discovery_protected = false;
		statistic_.secAttributes.is_liveliness_protected = false;
		statistic_.secAttributes.is_payload_protected = false;
		statistic_.secAttributes.is_key_protected = false;
		statistic_.secAttributes.is_submessage_protected = false;
		statistic_.rtpsMsgProtected = false;
		secPlugin_ = NULL;
	}
	return RETCODE_OK;
}

ReturnCode_t DataWriterImpl::FillSecureData()
{
	//端点保护下根据实体属性初始化builtinSecureData_的security_info 否则用的builtinData_ 不需要初始化
	if (statistic_.secAttributes.is_discovery_protected) {
		if (statistic_.enableSec) {
			builtinSecureData_.security_info.endpoint_security_attributes =
				EndpointSecurityAttributesToMask(&statistic_.secAttributes);
			builtinSecureData_.security_info.plugin_endpoint_security_attributes = statistic_.secAttributes.plugin_endpoint_attributes;
		}
		else
		{
			builtinSecureData_.security_info = ENDPOINT_SECURITY_ATTRIBUTES_INFO_DEFAULT;
		}
	}
	return RETCODE_OK;
}

uint32_t DataWriterImpl::SetRemoteReaderCryptoTokens(const GUID_t* readerId, const CryptoTokenSeq* tokens)
{
	if (readerId == NULL || tokens == NULL)
	{
		return 1;
	}
	GUID_t guid = Conversion::ToGuid(get_instance_handle());
	if (guid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER)
	{
		return 0;
	}
	auto rdIter = remoteReaders_.preparingReaders.find(*readerId);
	if (rdIter == remoteReaders_.preparingReaders.end())
	{
		DataHolderSeq& tokenRef = remoteReaders_.remoteReaderCryptoToken[*readerId];
		tokenRef = *tokens;
		return 0;
	}
	SecurityException secExce;
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(pub_->get_participant());
	if (!dp->secPlugin_.crypto_key_exchange->set_remote_datareader_crypto_tokens(
		cryptoHdl_,
		rdIter->second.cryptoHdl,
		tokens,
		&secExce))
	{
		return 1;
	}
	rdIter->second.readyForSecComm = true;
	secReaders_.push_back(rdIter->second.cryptoHdl);
	secReadersMap_[readerId->prefix] = rdIter->second.cryptoHdl;
	remoteReaders_.preparingReaders.erase(rdIter);
	return 0;
}
#endif // DDSSECURITY
