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
#include "rtps/discovery/discovery.h"
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

DataWriterImpl::DataWriterImpl(Topic* topic, const DataWriterQos& qos, DataWriterListener* listener, StatusMask mask, PublisherImpl* pub, TypeSupport* typeSupport)
	: DomainEntityImpl(mask), topic_(dynamic_cast<TopicImpl*>(topic)),
	  qos_(qos), listener_(listener), pub_(pub),
	  statusMgr_( new DataWriterStatusManager(this)),
	  deadlineTimer_(nullptr),
	  lifespanTimer_(nullptr),
	  memoryPool_(MemoryPool::GetInstance()),
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


ReturnCode_t DataWriterImpl::set_qos(const DataWriterQos& qos)
{
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
			deadlineTimer_.reset();
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
			lifespanTimer_.reset();
		}
	}

	qos_ = qosTemp;

	writer_->SetQos(qosTemp);

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
	if (enable_ == false){
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enabled");
		return HANDLE_NIL;
	}

    InstanceHandle_t handle;
	if (source_timestamp.sec < 0 ) {
		return HANDLE_NIL;
	}

	//InstanceHandle_t instance_handle;
	if (!typeSupport_->has_key()) {
		TRAVODDS_LOG(LOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return HANDLE_NIL;
	}
	// void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5
	SerializedBuffer *buffer = new SerializedBuffer;
	buffer->buffer_size = typeSupport_->get_serialized_data_size((void*)instance_data, 0);
	memoryPool_->getBuffer(buffer->buffer_size, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer;
	
	CdrSerializer cdr(buffer);
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
			TRAVODDS_LOG(LOG_ERROR, "AddKeyHistory Error");
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
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enable");
		return RETCODE_NOT_ENABLED;
	}
	return UnRegisteOrDispose(instance_data, handle, source_timestamp);
}

ReturnCode_t DataWriterImpl::get_key_value(void* key_holder, const InstanceHandle_t& handle)
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enable");
		return RETCODE_NOT_ENABLED;
	}
	if (!typeSupport_->has_key()) {
		TRAVODDS_LOG(LOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return RETCODE_ILLEGAL_OPERATION;
	}

	SerializedBuffer* cacheData = history_->get_key_value(handle);
	if (nullptr == cacheData) {
		TRAVODDS_LOG(LOG_ERROR, "cacheData is nullptr");
		return RETCODE_BAD_PARAMETER;
	}
	CdrDeserializer cdr(cacheData);
	int32_t ret = typeSupport_->deserialize_data(key_holder, &cdr, DEFAULT_ENDIAN);
	if (0 != ret) {
		TRAVODDS_LOG(LOG_ERROR, "deserialize_data Error[%d]", ret);
		return RETCODE_ERROR; 
	}
	return RETCODE_OK;
}

InstanceHandle_t DataWriterImpl::lookup_instance(const void* const instance) const
{
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enable");
		return HANDLE_NIL;
	}
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
	if (enable_ == false) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enabled");
		return RETCODE_NOT_ENABLED;
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
		MakeKey(instance_data, instanceHandle, true);

		if (HANDLE_NIL != handle && !history_->IsKeyRegistered(handle)) {
			TRAVODDS_LOG(LOG_ERROR, "handle invalid");
			return RETCODE_BAD_PARAMETER;
		}

		if (HANDLE_NIL != handle && handle != instanceHandle) {
			TRAVODDS_LOG(LOG_ERROR, "handle not met");
			return RETCODE_PRECONDITION_NOT_MET;
		}

		if (HANDLE_NIL == handle) {

			instanceHandle = register_instance_w_timestamp(instance_data, source_timestamp);

			if (HANDLE_NIL == instanceHandle) {
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

	uint32_t size = typeSupport_->get_serialized_data_size((void*)instance_data, 0);

	SerializedBuffer* bufferHeader = nullptr;
	ret = CreateSerializedBuffer(bufferHeader, size);
	if (RETCODE_OK != ret) {
		DeleteSerializedBuffer(bufferHeader);
		TRAVODDS_LOG(LOG_ERROR, "CreateSerializedBuffer Err ret= %d", ret);
		return ret;
	}

	CdrSerializer cdr(bufferHeader);
	typeSupport_->serialize_data((void*)instance_data, &cdr, DEFAULT_ENDIAN);
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
			DeleteSerializedBuffer(encodeBuffer);
			TRAVODDS_LOG(LOG_ERROR, "CreateSerializedBuffer Err ret= %d", ret);
			return ret;
		}

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
			DeleteSerializedBuffer(encodeBuffer);
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
				DeleteSerializedBuffer(curEncodeSubBuffer);
				TRAVODDS_LOG(LOG_ERROR, "CreateSerializedBuffer Err ret= %d", ret);
				return ret;
			}
			//从encodeBuffer指向的buffer取出对应部分（根据偏移encodeBufferShift和计算出的长度curEncodeSubBufferSize），赋值给当前buffer
			memcpy(curEncodeSubBuffer->buffer, encodeBuffer->buffer + encodeBufferShift, curEncodeSubBufferSize);
			encodeBufferShift += curEncodeSubBufferSize; //更新偏移值encodeBufferShift
			if (i == 0) { //如果当前部分buffer是第一个
				encodeBufferHeader = curEncodeSubBuffer;
				preEncodeSubBuffer = curEncodeSubBuffer;
			}
			else {
				preEncodeSubBuffer->next = curEncodeSubBuffer;
				preEncodeSubBuffer = curEncodeSubBuffer;
			}
		}

		//delete[]encodeBuffer->buffer;
		DeleteSerializedBuffer(encodeBuffer);
	}
	std::shared_ptr<CacheChange> cacheChange;
	if (statistic_.secAttributes.is_payload_protected) {
		cacheChange = writer_->NewChange(ALIVE, encodeBufferHeader, instanceHandle, source_timestamp);
	}
	else {
		cacheChange = writer_->NewChange(ALIVE, bufferHeader, instanceHandle, source_timestamp);
	}
#else
	std::shared_ptr<CacheChange> cacheChange = writer_->NewChange(ALIVE, bufferHeader, instanceHandle, source_timestamp, sample_identity);
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

	if (qos_.deadline.period != Duration_t(TIME_INFINITE)) {
		std::lock_guard<std::mutex> lock(lastInstanceMtx_);
		std::chrono::steady_clock::time_point nextTime =
			std::chrono::steady_clock::now() + std::chrono::milliseconds(Conversion::ToMilliSecondsInt64(qos_.deadline.period));
		history_->SetNextDeadline(handle, nextTime);
		history_->GetNextDeadline(lastInstance_, nextTime);
		auto period = nextTime - std::chrono::steady_clock::now();
		deadlineTimer_->UpdatePeriod(std::chrono::duration_cast<std::chrono::milliseconds>(period).count());
	}

	/* wangyi lifespan qos */
	if (qos_.lifespan.duration != Duration_t(TIME_INFINITE))
	{
		lifespanTimer_->UpdatePeriod(qos_.lifespan.duration.ToNs() * 1e-6);
		lifespanTimer_->ReStart();
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
	if (!enable_) {
		TRAVODDS_LOG(LOG_WARNING, "DataWriter not enabled");
		return RETCODE_NOT_ENABLED;
	}
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
		TRAVODDS_LOG(LOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return RETCODE_ILLEGAL_OPERATION;
	}
	
	if (timespace.sec < 0) {
		TRAVODDS_LOG(LOG_ERROR, "timespace.sec[%d] < 0", timespace.sec);
		return RETCODE_BAD_PARAMETER;
	}
	ret = MakeKey(data, instanceHandle, true);
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


	buffer = new SerializedBuffer;
	buffer->buffer_size = 0;
	if (!memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT * 2, buffer->origin_buffer)) {
		TRAVODDS_LOG(LOG_ERROR, "getBuffer error size[%d]", buffer->buffer_size);
		return ret;
	}
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;
	std::shared_ptr<CacheChange> cacheChange = writer_->NewChange(kind, buffer, instanceHandle, timespace);
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

const PublicationBuiltinTopicData& DataWriterImpl::GetBuiltinTopicData()
{
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


bool DataWriterImpl::MakeKey(const void* const data, InstanceHandle_t& handle, bool forceMd5) const
{
	static_cast<void>(forceMd5);
	SerializedBuffer buffer;
	buffer.buffer_size = typeSupport_->get_serialized_data_size((void*)data, 0);
	memoryPool_->getBuffer(buffer.buffer_size, buffer.origin_buffer);
	buffer.buffer = buffer.origin_buffer;

	CdrSerializer cdr(&buffer);
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

bool DataWriterImpl::LifespanTimeout()
{
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
	
	if (source_timestamp.sec < 0) {
		TRAVODDS_LOG(LOG_ERROR, "source_timestamp.sec[%d] < 0", source_timestamp.sec);
		return RETCODE_BAD_PARAMETER;
	}

	//InstanceHandle_t instance_handle;
	if (!typeSupport_->has_key()) {
		TRAVODDS_LOG(LOG_ERROR, "Topic is NO_KEY, operation not permitted");
		return RETCODE_BAD_PARAMETER;
	}
	// void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5
	SerializedBuffer *buffer = new SerializedBuffer;
	buffer->buffer_size = typeSupport_->get_serialized_data_size((void*)instance_data, 0);
	memoryPool_->getBuffer(buffer->buffer_size, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer;

	CdrSerializer cdr(buffer);
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
			TRAVODDS_LOG(LOG_ERROR, "AddKeyHistory Err ret = %d",ret);
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
		// TRAVODDS_LOG(LOG_ERROR, "buffer is nullptr");
		return RETCODE_ERROR;
	}

	DeleteSerializedBuffer(buffer->next);

	memoryPool_->releaseBuffer(buffer->origin_buffer);

	return RETCODE_OK;
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
