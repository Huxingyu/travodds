#include "dcps/publish/datawriterimpl.h"
#include "dcps/publish/publisherimpl.h"
#include "dcps/topic/topicimpl.h"
#include "dcps/topic/typesupport.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "qos/qospolicycheck.h"
#include "common/memory/memorypool.h"
#include "common/time/ddstimer.h"
#include "dcps/base/statusmanager.h"
#include "rtps/endpoint/writer.h"
#include "rtps/endpoint/participant.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "rtps/endpoint/statefulwriter.h"
#include <chrono>
#include <iostream>
#include <mutex>
#include  "common/timeconversion.h"
#include "common/log/logger.h"

using namespace std::chrono;

USING_AGIDDS_NAMESPACE;

DataWriterImpl::DataWriterImpl(Topic* topic, const DataWriterQos& qos, DataWriterListener* listener, StatusMask mask, PublisherImpl* pub, TypeSupport* typeSupport)
	: DomainEntityImpl(mask), topic_(dynamic_cast<TopicImpl*>(topic)),
	  qos_(qos), listener_(listener), pub_(pub), typeSupport_(typeSupport),
	  statusMgr_( new DataWriterStatusManager(this)),
	  deadlineTimer_(nullptr),
	  memoryPool_(MemoryPool::GetInstance())
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

}

DataWriterImpl::~DataWriterImpl()
{
	writer_->SetRelatedDataWriter(nullptr);
	topic_->Dereference();

    if (listener_) {
        //delete listener_;
        listener_ = nullptr;
    }
}


ReturnCode_t DataWriterImpl::set_qos(const DataWriterQos& qos)
{
    DataWriterQos defaultQos;
    pub_->get_default_datawriter_qos(defaultQos);
    const DataWriterQos& qosTemp = (&qos == &DATAWRITER_QOS_DEFAULT) ? defaultQos : qos;

    if (&qos != &DATAWRITER_QOS_DEFAULT) {
        ReturnCode_t ret = QosPolicyCheck::CheckQos(qosTemp);
        if (RETCODE_OK != ret) {
			AGILOG(AGILOG_ERROR, "CheckQos Err");
            return ret;
        }
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qosTemp)) {
		AGILOG(AGILOG_ERROR, "CanQosBeUpdated Err");
        return RETCODE_IMMUTABLE_POLICY;
    }

	if (enable_) {
		if (qos_.deadline.period != qos.deadline.period && qos_.deadline.period != Duration_t(TIME_INFINITE)) {
			std::chrono::steady_clock::time_point nextTime =
				std::chrono::steady_clock::now() + std::chrono::milliseconds(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
			history_->UpAllDeadline(nextTime);
			if (deadlineTimer_) {
				deadlineTimer_->UpdatePeriod(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
			} else {
                deadlineTimer_ = TimerManager::CreateTimer(
					reinterpret_cast<const Time_t&>(qosTemp.deadline.period),
					[this] {
						DeadlineTimeout();
					},
					"dw.deadline"
                );
			}
		} else if(qos_.deadline.period == Duration_t(TIME_INFINITE)) {
			deadlineTimer_.reset();
		}
	}

	qos_ = qosTemp;


    return RETCODE_OK;
}

ReturnCode_t DataWriterImpl::get_qos(DataWriterQos& qos) const
{
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

    return  RETCODE_BAD_PARAMETER;
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
    InstanceHandle_t handle;
	if (source_timestamp.sec < 0 ) {
		return HANDLE_NIL;
	}

	//InstanceHandle_t instance_handle;
	if (!typeSupport_->has_key()) {
		AGILOG(AGILOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return HANDLE_NIL;
	}
	// void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5
	SerializedBuffer *buffer = new SerializedBuffer;
	buffer->buffer_size = typeSupport_->get_serialized_data_size((void*)instance_data, 0);
	memoryPool_->getBuffer(buffer->buffer_size, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer;
	
	AGIDDSCdrSerializer cdr(buffer);
	typeSupport_->serialize_data((void*)instance_data, &cdr, DEFAULT_ENDIAN);

	/* 生成key */
	MakeKey(instance_data, handle, true);

	if (!history_->IsKeyRegistered(handle)) {
		if (RETCODE_OK != history_->AddKeyHistory(handle, buffer)) {
			memoryPool_->releaseBuffer(buffer->origin_buffer);
			/* 注册失败，释放空间 */
			delete buffer;
			buffer = nullptr;
			handle = HANDLE_NIL;
			AGILOG(AGILOG_ERROR, "AddKeyHistory Error");
		}
		
	} else {
		memoryPool_->releaseBuffer(buffer->origin_buffer);
		delete buffer;
		buffer = nullptr;
	}

	return handle;
}

ReturnCode_t DataWriterImpl::unregister_instance(const void* const instance_data, const InstanceHandle_t& handle)
{
	Time_t time;
	DomainParticipant::get_current_time(time);

	return unregister_instance_w_timestamp(instance_data, handle, time);
}

ReturnCode_t DataWriterImpl::unregister_instance_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp)
{
	return UnRegisteOrDispose(instance_data, handle, source_timestamp);
}

ReturnCode_t DataWriterImpl::get_key_value(void* key_holder, const InstanceHandle_t& handle)
{
	if (!typeSupport_->has_key()) {
		AGILOG(AGILOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return RETCODE_ILLEGAL_OPERATION;
	}

	SerializedBuffer* cacheData = history_->get_key_value(handle);
	if (nullptr == cacheData) {
		AGILOG(AGILOG_ERROR, "cacheData is nullptr");
		return RETCODE_BAD_PARAMETER;
	}
	AGIDDSCdrDeserializer cdr(cacheData);
	int32_t ret = typeSupport_->deserialize_data(key_holder, &cdr, DEFAULT_ENDIAN);
	if (0 != ret) {
		AGILOG(AGILOG_ERROR, "deserialize_data Error[%d]", ret);
		return RETCODE_ERROR; 
	}
	return RETCODE_OK;
}

InstanceHandle_t DataWriterImpl::lookup_instance(const void* const instance) const
{
	InstanceHandle_t handle = HANDLE_NIL;

	if (typeSupport_->has_key()) {
		if (MakeKey(instance, handle, true)) {
			if (!history_->IsInstancePresent(handle)) {
				handle = HANDLE_NIL;
			} 
		}
	}
	return handle;
}

ReturnCode_t DataWriterImpl::write(const void* const instance_data, const InstanceHandle_t& handle)
{
	Time_t time;
	DomainParticipantImpl::get_current_time(time);

    return write_w_timestamp(instance_data, handle, time);
}

ReturnCode_t DataWriterImpl::write_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp)
{
	ReturnCode_t ret = RETCODE_ERROR;
	InstanceHandle_t instanceHandle = HANDLE_NIL;

	ParameterList_t lineQos;
	bool fragFlag = false;

	if(typeSupport_->has_key())
	{
		MakeKey(instance_data, instanceHandle, true);

		if (HANDLE_NIL != handle && !history_->IsKeyRegistered(handle)){
			AGILOG(AGILOG_ERROR, "handle invalid");
			return RETCODE_BAD_PARAMETER;
		}

		if (HANDLE_NIL != handle && handle != instanceHandle) {
			AGILOG(AGILOG_ERROR, "handle not met");
			return RETCODE_PRECONDITION_NOT_MET;
		}

		if (HANDLE_NIL == handle) {
	
			instanceHandle = register_instance_w_timestamp(instance_data, source_timestamp);

			if (HANDLE_NIL == instanceHandle) {
				/* 注册失败 可能是因为max_instance的值达到 */
				AGILOG(AGILOG_ERROR, "RegistInstance Err ret= %d", ret);
				return ret;
			}
		}
		Parameter_t param;
		param.parameterId = PID_KEY_HASH;
		param.length = MIG_RTPS_KEY_HASH_MAX_LENGTH;
		param.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + MIG_RTPS_KEY_HASH_MAX_LENGTH);
		lineQos.push_back(param);
	}

	uint32_t size = typeSupport_->get_serialized_data_size((void*)instance_data, 0);

	SerializedBuffer* bufferHeader = nullptr;
	ret = CreateSerializedBuffer(bufferHeader, size);
	if (RETCODE_OK != ret) {
		DeleteSerializedBuffer(bufferHeader);
		AGILOG(AGILOG_ERROR, "CreateSerializedBuffer Err ret= %d", ret);
		return ret;
	}

	AGIDDSCdrSerializer cdr(bufferHeader);
	typeSupport_->serialize_data((void*)instance_data, &cdr, DEFAULT_ENDIAN);

	bufferHeader->buffer -= CDR_ENDIANNESS_LEN;
	bufferHeader->buffer_size += CDR_ENDIANNESS_LEN;

	std::shared_ptr<CacheChange> cacheChange = writer_->NewChange(ALIVE, bufferHeader, instanceHandle, source_timestamp);
	if (nullptr == cacheChange) {
		AGILOG(AGILOG_ERROR, "cacheChange ret= %d", ret);
		return RETCODE_ERROR;
	}
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
		AGILOG(AGILOG_ERROR, "AddNewCachce Err ret= %d", ret);
		return ret;
	}
	{
		std::lock_guard<std::mutex> lock(lastInstanceMtx_);
		lastInstance_ = handle;
	}
	if (qos_.deadline.period != Duration_t(TIME_INFINITE)) {
		deadlineTimer_->ReStart();
	}
    return ret;
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
    return UnRegisteOrDispose(data, handle, source_timestamp, true);
}
ReturnCode_t DataWriterImpl::UnRegisteOrDispose(const void* const data, const InstanceHandle_t& handle, const Time_t& timespace, bool dispose)
{
	ReturnCode_t ret = RETCODE_ERROR;
	InstanceHandle_t instanceHandle = HANDLE_NIL;
	SerializedBuffer *buffer = nullptr;
	ChangeKind_t kind = ALIVE;
	StatusInfo_t status = { 0 };

	//InstanceHandle_t instance_handle;
	if (!typeSupport_->has_key()) {
		AGILOG(AGILOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return RETCODE_ILLEGAL_OPERATION;
	}
	
	if (timespace.sec < 0) {
		AGILOG(AGILOG_ERROR, "timespace.sec[%d] < 0", timespace.sec);
		return RETCODE_BAD_PARAMETER;
	}
	ret = MakeKey(data, instanceHandle, true);
	if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "handle bad parameter")
		return RETCODE_BAD_PARAMETER;
	}

	/* (生成Key错误 && 不是注册的key && (传入的句柄不是HANDLE_NIL &&  传入的handle和实际不一致 ) */
	if (handle != HANDLE_NIL && (!history_->IsKeyRegistered(handle))) {
		AGILOG(AGILOG_ERROR, "handle bad parameter");
		return RETCODE_BAD_PARAMETER;
	}

	/*  传入的handle和实际不一致  */
	if (handle != instanceHandle && handle != HANDLE_NIL) {
		AGILOG(AGILOG_ERROR, "handle not met");
		return RETCODE_PRECONDITION_NOT_MET;
	}

	if (dispose) {
		kind = NOT_ALIVE_DISPOSED;
		status[3] = StatusInfoFlag::DISPOSED_FLAG;
	} else {
		kind = NOT_ALIVE_UNREGISTERED;
		status[3] = StatusInfoFlag::DISPOSED_FLAG | StatusInfoFlag::UNREGISTERED_FLAG;
	}
	Parameter_t keyParam, statueParam;
	keyParam.parameterId = PID_KEY_HASH;
	keyParam.length = MIG_RTPS_KEY_HASH_MAX_LENGTH;
	keyParam.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + MIG_RTPS_KEY_HASH_MAX_LENGTH);

	statueParam.parameterId = PID_STATUS_INFO;
	statueParam.length = sizeof(StatusInfo_t);
	statueParam.value.assign(status, status + sizeof(StatusInfo_t));


	buffer = new SerializedBuffer;
	buffer->buffer_size = 0;
	if (!memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT * 2, buffer->origin_buffer)) {
		AGILOG(AGILOG_ERROR, "getBuffer error size[%d]", buffer->buffer_size);
		return ret;
	}
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;
	std::shared_ptr<CacheChange> cacheChange = writer_->NewChange(kind, buffer, instanceHandle, timespace);
	if (nullptr == cacheChange) {
		AGILOG(AGILOG_ERROR, "NewChange Err");
		return RETCODE_ERROR;
	}
	cacheChange->inlineQos.push_back(keyParam);
	cacheChange->inlineQos.push_back(statueParam);
	ret = writer_->AddNewCachce(cacheChange);
	if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "AddNewCachce Err ret = %d", ret);
		return ret;
	}
	if (!dispose) {
		/* 在发送 */
		writer_->RemoveCacheByHandle(instanceHandle);
	}

	return ret;
}

ReturnCode_t DataWriterImpl::wait_for_acknowledgments(const Duration_t& max_wait)
{
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
	if (writer_ == nullptr) {
		AGILOG(AGILOG_ERROR, "writer_ is nullptr");
		return RETCODE_NOT_ENABLED;
	}

	if (!(dynamic_cast<DomainParticipantImpl*>(pub_->get_participant()))->GetLivelinessManager()->AssertLiveliness(writer_->GetGuid(),true, qos_.liveliness.kind)) {
		AGILOG(AGILOG_ERROR, "Could not assert liveliness of write");
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
	AGILOG(AGILOG_ERROR, "datawriter donot support interface");
	return RETCODE_UNSUPPORTED;
}

ReturnCode_t DataWriterImpl::get_matched_subscriptions(InstanceHandleSeq& subscription_handles)
{
	if(nullptr == writer_ ) {
		AGILOG(AGILOG_ERROR, "writer_ is nullptr");
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
		AGILOG(AGILOG_ERROR, "releate publisher isnot enable");
		return RETCODE_PRECONDITION_NOT_MET;
	}

	if (qos_.deadline.period != Duration_t(TIME_INFINITE)) {
        deadlineTimer_ = TimerManager::CreateTimer(
			reinterpret_cast<Time_t&>(qos_.deadline.period),
			[this] {
				DeadlineTimeout();
			},
			"dw.deadline"
        );
	}

	enable_ = true;
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

const PublicationBuiltinTopicData& DataWriterImpl::GetBuiltinTopicData()
{
	return builtinData_;
}

void DataWriterImpl::SetRelatedWriter(std::shared_ptr<Writer> writer) 
{
	history_ = writer->GetHistoryCache();


	writer_ = writer;
	writer->SetRelatedDataWriter(this);

	handle_ = Conversion::ToInstanceHandle(writer->GetGuid());
	builtinData_.key = Conversion::ToBuiltinTopicKey(handle_);
}


bool DataWriterImpl::MakeKey(const void* const data, InstanceHandle_t& handle, bool forceMd5) const
{
	SerializedBuffer buffer;
	buffer.buffer_size = typeSupport_->get_serialized_data_size((void*)data, 0);
	memoryPool_->getBuffer(buffer.buffer_size, buffer.origin_buffer);
	buffer.buffer = buffer.origin_buffer;

	AGIDDSCdrSerializer cdr(&buffer);
	bool ret = typeSupport_->get_instancehandle((void*)data, &cdr, handle, true);

	memoryPool_->releaseBuffer(buffer.origin_buffer);

	return ret;
}


void DataWriterImpl::DeadlineTimeout()
{
	InstanceHandle_t instance;
	{
		std::lock_guard<std::mutex> lock(lastInstanceMtx_);
		instance = lastInstance_;
		std::chrono::steady_clock::time_point nextTime =
			std::chrono::steady_clock::now() + std::chrono::milliseconds(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
		history_->SetNextDeadline(lastInstance_, nextTime);
		history_->GetNextDeadline(lastInstance_, nextTime);
		auto period = nextTime - std::chrono::steady_clock::now();
		deadlineTimer_->UpdatePeriod(std::chrono::duration_cast<std::chrono::milliseconds>(period).count());
	}
	statusMgr_->UpdateOfferedDeadlineMissedStatus(instance);
}

ReturnCode_t DataWriterImpl::RegistInstance(const void* const instance_data, const Time_t& source_timestamp, InstanceHandle_t& handle)
{
	
	if (source_timestamp.sec < 0) {
		AGILOG(AGILOG_ERROR, "source_timestamp.sec[%d] < 0", source_timestamp.sec);
		return RETCODE_BAD_PARAMETER;
	}

	//InstanceHandle_t instance_handle;
	if (!typeSupport_->has_key()) {
		AGILOG(AGILOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return RETCODE_BAD_PARAMETER;
	}
	// void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5
	SerializedBuffer *buffer = new SerializedBuffer;
	buffer->buffer_size = typeSupport_->get_serialized_data_size((void*)instance_data, 0);
	memoryPool_->getBuffer(buffer->buffer_size, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer;

	AGIDDSCdrSerializer cdr(buffer);
	typeSupport_->serialize_data((void*)instance_data, &cdr, DEFAULT_ENDIAN);

	/* 生成key */
	MakeKey(instance_data, handle, true);

	if (!history_->IsKeyRegistered(handle)) {
		ReturnCode_t ret = history_->AddKeyHistory(handle, buffer);
		if (RETCODE_OK != ret) {
			memoryPool_->releaseBuffer(buffer->origin_buffer);
			/* 注册失败，释放空间 */
			delete buffer;
			buffer = nullptr;
			handle = HANDLE_NIL;
			AGILOG(AGILOG_ERROR, "AddKeyHistory Err ret = %d",ret);
			return ret;
		}

	}
	else {
		memoryPool_->releaseBuffer(buffer->origin_buffer);
		delete buffer;
		buffer = nullptr;
	}

	return RETCODE_OK;
}
ReturnCode_t DataWriterImpl::UpdateLivelinessLostStatus()
{
	return statusMgr_->UpdateLivelinessLostStatus();

}


ReturnCode_t DataWriterImpl::CreateSerializedBuffer(SerializedBuffer* &bufferHeader, uint32_t dataLen)
{
	SerializedBuffer* buffer = nullptr;
	/* 长度 + CDR_ENDIANNESS_LEN 为 CDR 标识的长度 */
	dataLen += CDR_ENDIANNESS_LEN;
	bool first = true;
	do {
		SerializedBuffer* nextBuffer = new SerializedBuffer();
			
		if (dataLen > DATAMESSAGE_LIMIT) {
			nextBuffer->buffer_size = DATAMESSAGE_LIMIT;
			dataLen -= DATAMESSAGE_LIMIT;
		}
		else {
			nextBuffer->buffer_size = dataLen;
			dataLen -= dataLen;
		}
		
		if (!memoryPool_->getBuffer(nextBuffer->buffer_size + 2 * BUFF_SHIFT, nextBuffer->origin_buffer)) {
			return RETCODE_APPLY_MEM_ERR;
		}
		nextBuffer->buffer = nextBuffer->origin_buffer + BUFF_SHIFT;

		if (first) {
			buffer = nextBuffer;
			bufferHeader = buffer;

			memset(buffer->buffer, 0, CDR_ENDIANNESS_LEN);
			buffer->buffer[1] = DEFAULT_ENDIAN;

			buffer->buffer += CDR_ENDIANNESS_LEN;
			buffer->buffer_size -= CDR_ENDIANNESS_LEN;
			first = false;
		}
		else {
			buffer->next = nextBuffer;
			buffer = nextBuffer;
		}

	} while (dataLen != 0);

	return RETCODE_OK;
}


ReturnCode_t DataWriterImpl::DeleteSerializedBuffer(SerializedBuffer* buffer)
{
	if (!buffer) {
		AGILOG(AGILOG_ERROR, "buffer is nullptr");
		return RETCODE_ERROR;
	}

	DeleteSerializedBuffer(buffer->next);

	memoryPool_->releaseBuffer(buffer->origin_buffer);

	return RETCODE_OK;
}