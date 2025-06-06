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
	memcpy(builtinData_.participant_key.value, &handle.keyHash.value, 16);
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
        delete listener_;
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
            return ret;
        }
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qosTemp)) {
        return RETCODE_IMMUTABLE_POLICY;
    }

	if (enable_) {
		if (qos_.deadline.period != qos.deadline.period && qos_.deadline.period != Duration_t(TIME_INFINITE)) {
			if (deadlineTimer_) {
				deadlineTimer_->UpdatePeriod(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
			} else {
                deadlineTimer_.reset();
                deadlineTimer_ = TimerManager::CreateTimer(
					reinterpret_cast<const Time_t&>(qosTemp.deadline.period),
					[this] {
						statusMgr_->UpdateOfferedDeadlineMissedStatus();
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
    if (listener) {
		if (listener_) {
			delete listener_;
			listener_ = nullptr;
		}
        listener_ = listener;
        statusMask_ = mask;
        return RETCODE_OK;
    }
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
		std::cout << "Topic is NO_KEY, operation not permitted" << std::endl;
		return HANDLE_NIL;
	}
	// void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5
	SerializedBuffer *buffer = new SerializedBuffer;
	buffer->buffer_size = typeSupport_->get_serialized_data_size((void*)instance_data, 0);
	memoryPool_->getBuffer(buffer->buffer_size, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer;
	
	AGIDDSCdrSerializer cdr(buffer);

	/* 生成key */
	typeSupport_->get_instancehandle((void*)instance_data,&cdr, handle,true);

	if (!history_->IsKeyRegistered(handle)) {
		if (RETCODE_OK != history_->AddKeyHistory(handle, buffer)) {
			memoryPool_->releaseBuffer(buffer->origin_buffer);
			/* 注册失败，释放空间 */
			delete buffer;
			buffer = nullptr;
			handle = HANDLE_NIL;
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
		std::cout<<"Topic is NO_KEY, operation not permitted"<<std::endl;
		return RETCODE_ILLEGAL_OPERATION;
	}

	SerializedBuffer* cacheData = history_->get_key_value(handle);
	if (nullptr == cacheData) {
		return RETCODE_BAD_PARAMETER;
	}
	AGIDDSCdrDeserializer cdr(cacheData);
	int32_t ret = typeSupport_->deserialize_data(key_holder, &cdr, TypeSupport::IsLittleEndian());
	if (0 != ret) {
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

		if (HANDLE_NIL != handle && handle != instanceHandle) {
			return RETCODE_PRECONDITION_NOT_MET;
		}
		if (!history_->IsKeyRegistered(instanceHandle)) {
			//TODO 待优化
			ret = RegistInstance(instance_data, source_timestamp, instanceHandle);
			if (RETCODE_OK != ret) {
				/* 注册失败 可能是因为max_instance的值达到 */
				return ret;
			}
		}
		Parameter_t param;
		param.parameterId = PID_KEY_HASH;
		param.length = 16;
		param.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + 16);
		lineQos.push_back(param);
	}

	uint32_t size = typeSupport_->get_serialized_data_size((void*)instance_data, 0);

	SerializedBuffer* bufferHeader = nullptr;
	ret = CreateSerializedBuffer(bufferHeader, size);
	if (RETCODE_OK != ret) {
		DeleteSerializedBuffer(bufferHeader);
		return ret;
	}

	AGIDDSCdrSerializer cdr(bufferHeader);
	typeSupport_->serialize_data((void*)instance_data, &cdr, TypeSupport::IsLittleEndian()); 

	bufferHeader->buffer -= 4;
	bufferHeader->buffer_size += 4;

	std::shared_ptr<CacheChange> cacheChange = writer_->NewChange(ALIVE, bufferHeader, instanceHandle, source_timestamp);
	if (nullptr == cacheChange) {
		return RETCODE_ERROR;
	}
	cacheChange->dataSize = size;
	cacheChange->inlineQos = lineQos;
	if (size > 0) {
		cacheChange->fragmentTatolNum = size / DATAMESSAGE_LIMIT;
		if (0 == size % DATAMESSAGE_LIMIT) {
			cacheChange->fragmentTatolNum += 1;
		}
		cacheChange->fraqmentSize = DATAMESSAGE_LIMIT;
	}

	ret = writer_->AddNewCachce(cacheChange);
	if (RETCODE_OK != ret) {

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
	char status[4] = { 0 };

	if (timespace.sec < 0) {
		return RETCODE_BAD_PARAMETER;
	}
	ret = MakeKey(data, instanceHandle, true);
	/*  传入的handle和实际不一致  */
	if (handle != instanceHandle && handle != HANDLE_NIL) {
		ret = RETCODE_PRECONDITION_NOT_MET;
	}

	/* (生成Key错误 && 不是注册的key && (传入的句柄不是HANDLE_NIL &&  传入的handle和实际不一致 ) */
	if (RETCODE_OK != ret && (!history_->IsKeyRegistered(instanceHandle))) {
		return RETCODE_BAD_PARAMETER;
	}

	if (dispose) {
		kind = NOT_ALIVE_DISPOSED;
		status[3] = 1;
	} else {
		kind = NOT_ALIVE_UNREGISTERED;
		status[3] = 3;
	}
	Parameter_t keyParam, statueParam;
	keyParam.parameterId = PID_KEY_HASH;
	keyParam.length = 16;
	keyParam.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + 16);

	statueParam.parameterId = PID_STATUS_INFO;
	statueParam.length = 4;
	statueParam.value.assign(status, status + 4);


	buffer = new SerializedBuffer;
	buffer->buffer_size = 0;
	if (!memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT * 2, buffer->origin_buffer)) {
		return ret;
	}
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;
	std::shared_ptr<CacheChange> cacheChange = writer_->NewChange(kind, buffer, instanceHandle, timespace);
	if (nullptr == cacheChange) {
		return RETCODE_ERROR;
	}
	cacheChange->inlineQos.push_back(keyParam);
	cacheChange->inlineQos.push_back(statueParam);
	ret = writer_->AddNewCachce(cacheChange);
	if (RETCODE_OK != ret) {
		//delete cacheChange;
		//cacheChange = nullptr;
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
		return RETCODE_NOT_ENABLED;
	}

	if (!(dynamic_cast<DomainParticipantImpl*>(pub_->get_participant()))->GetLivelinessManager()->AssertLiveliness(writer_->GetGuid(),true, qos_.liveliness.kind))
	{
		/* TODO 日志 logError(DATAWRITER, "Could not assert liveliness of writer " << writer_->getGuid()); */
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
	return RETCODE_UNSUPPORTED;
}

ReturnCode_t DataWriterImpl::get_matched_subscriptions(InstanceHandleSeq& subscription_handles)
{
	return RETCODE_UNSUPPORTED;
}

ReturnCode_t DataWriterImpl::enable()
{
	if (enable_) {
		return RETCODE_OK;
	}
	if (!pub_->IsEnable()) {
		return RETCODE_PRECONDITION_NOT_MET;
	}

	if (qos_.deadline.period != Duration_t(TIME_INFINITE)) {
        deadlineTimer_.reset();
        deadlineTimer_ = TimerManager::CreateTimer(
			reinterpret_cast<Time_t&>(qos_.deadline.period),
			[this] {
				statusMgr_->UpdateOfferedDeadlineMissedStatus();
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


const InstanceHandle_t& DataWriterImpl::GetLastinstanceHandle() const
{
	std::lock_guard<std::mutex> lock(lastInstanceMtx_);
	return lastInstance_;
}

ReturnCode_t DataWriterImpl::RegistInstance(const void* const instance_data, const Time_t& source_timestamp, InstanceHandle_t& handle)
{
	
	if (source_timestamp.sec < 0) {
		return RETCODE_BAD_PARAMETER;
	}

	//InstanceHandle_t instance_handle;
	if (!typeSupport_->has_key()) {
		std::cout << "Topic is NO_KEY, operation not permitted" << std::endl;
		return RETCODE_BAD_PARAMETER;
	}
	// void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5
	SerializedBuffer *buffer = new SerializedBuffer;
	buffer->buffer_size = typeSupport_->get_serialized_data_size((void*)instance_data, 0);
	memoryPool_->getBuffer(buffer->buffer_size, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer;

	AGIDDSCdrSerializer cdr(buffer);

	/* 生成key */
	typeSupport_->get_instancehandle((void*)instance_data, &cdr, handle, true);

	if (!history_->IsKeyRegistered(handle)) {
		ReturnCode_t ret = history_->AddKeyHistory(handle, buffer);
		if (RETCODE_OK != ret) {
			memoryPool_->releaseBuffer(buffer->origin_buffer);
			/* 注册失败，释放空间 */
			delete buffer;
			buffer = nullptr;
			handle = HANDLE_NIL;

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
	/* 长度 + 4 为 CDR 标识的长度 */
	dataLen += 4;
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
		nextBuffer->buffer = nextBuffer->origin_buffer + 1024;

		if (first) {
			buffer = nextBuffer;
			bufferHeader = buffer;

			memset(buffer->buffer, 0, 4);
			if (TypeSupport::IsLittleEndian())
				memcpy(buffer->buffer, &CDR_LE, sizeof(Endianness_t));
			else
				memcpy(buffer->buffer, &CDR_BE, sizeof(Endianness_t));

			buffer->buffer += 4;
			buffer->buffer_size -= 4;
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
		return RETCODE_ERROR;
	}

	DeleteSerializedBuffer(buffer->next);

	memoryPool_->releaseBuffer(buffer->origin_buffer);

	return RETCODE_OK;
}