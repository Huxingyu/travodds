#include "DataWriterImpl.h"
#include "DDS/TopicImpl.h"
#include "DDS/DomainParticipantImpl.h"
#include "RTPS/Writer.h"
#include "DDS/TypeSupport.h"
#include "PublisherImpl.h"
#include "QoS/QosPolicyCheck.h"
#include "System/MemoryPool/MemoryPool.h"
#include "Status/StatusManager.h"
#include <chrono>
#include <iostream>
#include <mutex>

using namespace std::chrono;

USING_AGIDDS_NAMESPACE;

ReturnCode_t DataWriterImpl::wait_for_acknowledgments(const Duration_t &max_wait)
{
	return writer_->WaitForAcknowledgments(max_wait);
}

ReturnCode_t DataWriterImpl::get_liveliness_lost_status(LivelinessLostStatus &status)
{
    return statusMgr_->get_liveliness_lost_status(status);
}

ReturnCode_t DataWriterImpl::get_offered_deadline_missed_status(OfferedDeadlineMissedStatus &status)
{
	return statusMgr_->get_offered_deadline_missed_status(status);
}

ReturnCode_t DataWriterImpl::get_offered_incompatible_qos_status(OfferedIncompatibleQosStatus &status)
{
	return statusMgr_->get_offered_incompatible_qos_status(status);
}

ReturnCode_t DataWriterImpl::get_publication_matched_status(PublicationMatchedStatus &status)
{
	return statusMgr_->get_publication_matched_status(status);
}

Topic *DataWriterImpl::get_topic() const
{
    return topic_;
}

const Publisher *DataWriterImpl::get_publisher() const
{
    return pub_;
}

ReturnCode_t DataWriterImpl::assert_liveliness()
{
    ReturnCode_t temp = 1;
    return temp;
}

ReturnCode_t DataWriterImpl::get_matched_subscription_data(SubscriptionBuiltinTopicData &subscription_data, const InstanceHandle_t &subscription_handle)
{
    ReturnCode_t temp = 1;
    return temp;
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
	/* yuguxu 预留 DataWriter 资源申请
	 *
	 * 待实现
	 */

	enable_ = true;
	return RETCODE_OK;
}


DataWriterImpl::DataWriterImpl(Topic* a_topic, const DataWriterQos& qos, DataWriterListener* a_listener, StatusMask mask, PublisherImpl* a_pub, TypeSupport* a_typeSupport)
    : DomainEntityImpl(mask), topic_(dynamic_cast<TopicImpl*>(a_topic)), 
	  qos_(qos), listener_(a_listener), pub_(a_pub), typeSupport_(a_typeSupport),
	  statusMgr_( new DataWriterStatusManager(this))
{
	topic_->Reference();
}

DataWriterImpl::~DataWriterImpl()
{
	topic_->Dereference();

	if (statusMgr_) {
		delete statusMgr_;
		statusMgr_ = nullptr;
	}
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
	qos_ = qosTemp;
    return RETCODE_OK;
}

ReturnCode_t DataWriterImpl::get_qos(DataWriterQos& qos) const
{
	qos = qos_;
    return RETCODE_OK;
}

ReturnCode_t DataWriterImpl::set_listener(DataWriterListener* a_listener, const StatusMask& mask)
{
    if (a_listener) {
		if (listener_) {
			delete listener_;
			listener_ = nullptr;
		}

        //std::shared_ptr<DataWriterListener> Ptr(a_listener);
        listener_ = a_listener;
        statusMask_ = mask;
        return RETCODE_OK;
    }
    return  RETCODE_BAD_PARAMETER;
}

const DataWriterListener * DataWriterImpl::get_listener() const
{
    return listener_;
}

InstanceHandle_t DataWriterImpl::register_instance(const void* const instance_data)
{
	InstanceHandle_t handle;
	if (!typeSupport_->IsKey()) {
		std::cout << "Topic is NO_KEY, operation not permitted" << std::endl;
		return HANDLE_NIL;
	}

	/* 生成key */
	typeSupport_->MakeKey(instance_data, handle);
	/* 记录key */
	SerializedBuffer *pInstace = nullptr;
	history_->AddKeyHistory(handle, pInstace);

	typeSupport_->serialize_data((void*)&instance_data, pInstace, TypeSupport::IsLittleEndian());
	return handle;
}

InstanceHandle_t DataWriterImpl::register_instance_w_timestamp(const void* const instance_data, const Time_t& source_timestamp)
{
    InstanceHandle_t handle;
	if (source_timestamp.sec < 0 ) {
		return HANDLE_NIL;
	}

	//InstanceHandle_t instance_handle;
	if (!typeSupport_->IsKey()) {
		std::cout << "Topic is NO_KEY, operation not permitted" << std::endl;
		return HANDLE_NIL;
	}

	/* 生成key */
	typeSupport_->MakeKey(instance_data, handle);
	/* 记录key */
	SerializedBuffer *pInstace = nullptr;
	history_->AddKeyHistory(handle, pInstace);
	
	typeSupport_->serialize_data((void*)&instance_data, pInstace, TypeSupport::IsLittleEndian());
	return handle;
}

ReturnCode_t DataWriterImpl::unregister_instance(const void* const instance_data, const InstanceHandle_t& handle)
{
	InstanceHandle_t key;
	ReturnCode_t ret = typeSupport_->MakeKey(instance_data, key);
	if (RETCODE_OK == ret && !history_->IsKeyRegistered(handle) && (handle != key)) {
		ret = RETCODE_PRECONDITION_NOT_MET;
	}
	/* 删除key记录 */
	history_->DeleteKeyHistory(handle);
	return ret;
}

ReturnCode_t DataWriterImpl::unregister_instance_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp)
{
   	InstanceHandle_t keyHandle;
	ReturnCode_t retCode = RETCODE_OK;
	if (source_timestamp.sec < 0) {
		retCode = RETCODE_BAD_PARAMETER;
	}
	ReturnCode_t ret = typeSupport_->MakeKey(instance_data, keyHandle);
	if (RETCODE_OK == ret && !history_->IsKeyRegistered(keyHandle) && (handle != keyHandle)) {
		ret = RETCODE_PRECONDITION_NOT_MET;
	}
	/* 删除key记录 */
	history_->DeleteKeyHistory(keyHandle);
	return retCode;

}

ReturnCode_t DataWriterImpl::get_key_value(void* key_holder, const InstanceHandle_t& handle)
{
	if (!typeSupport_->IsKey()) {
		std::cout<<"Topic is NO_KEY, operation not permitted"<<std::endl;
		return RETCODE_ILLEGAL_OPERATION;
	}

	SerializedBuffer* cacheData = history_->get_key_value(handle);
	if (nullptr == cacheData) {
		return RETCODE_BAD_PARAMETER;
	}

	int ret = typeSupport_->deserialize_data(cacheData, key_holder, TypeSupport::IsLittleEndian());
	if (0 != ret) {
		return RETCODE_ERROR;
	}
	return RETCODE_OK;
}

InstanceHandle_t DataWriterImpl::lookup_instance(const void* const instance) const
{
	InstanceHandle_t handle = HANDLE_NIL;

	if (typeSupport_->IsKey()) {
		if (typeSupport_->MakeKey(instance, handle, false)) {
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

	unsigned int size = typeSupport_->get_max_serialized_data_size();

	SerializedBuffer *buffer = new SerializedBuffer;
	SerializedBuffer *bufferHeader = buffer;
	while (size >= DATAMESSAGE_LIMIT) {
		buffer->buffer_size = DATAMESSAGE_LIMIT;

		if (!MemoryPool::GetInstance().getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->buffer)) {
			return ret;
		}

		buffer->buffer = buffer->buffer + BUFF_SHIFT;
		size -= DATAMESSAGE_LIMIT;

		buffer->next = new SerializedBuffer;
		buffer = buffer->next;
	}

	buffer->buffer_size = size;
	if (!MemoryPool::GetInstance().getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->buffer)) {
		return ret;
	}
	buffer->buffer = buffer->buffer + BUFF_SHIFT;

	typeSupport_->serialize_data(instance_data, bufferHeader, TypeSupport::IsLittleEndian());

	CacheChange* cacheChange = writer_->NewChange(ALIVE, buffer, handle);
	if (nullptr == cacheChange) {
		return RETCODE_ERROR;
	}
		
	ret = writer_->AddNewCachce(cacheChange);
	if (RETCODE_OK != ret) {
		return ret;
	}

    return ret;
}

ReturnCode_t DataWriterImpl::dispose(const void* const instance_data, const InstanceHandle_t& handle)
{
	Time_t cur_time = { 1,1 };
	//TODO 
	return dispose_w_timestamp(instance_data, handle, cur_time);
}

ReturnCode_t DataWriterImpl::dispose_w_timestamp(const void* const data, const InstanceHandle_t& handle, const Time_t& source_timestamp)
{
	
	ReturnCode_t ret = RETCODE_ERROR;

	unsigned int size = typeSupport_->get_max_serialized_data_size();

	SerializedBuffer *buffer = new SerializedBuffer;
	SerializedBuffer *bufferHeader = buffer;
	while(size >= DATAMESSAGE_LIMIT) {
		buffer->buffer_size = DATAMESSAGE_LIMIT;

		if (!MemoryPool::GetInstance().getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->buffer)) {
			return ret;
		}

		buffer->buffer = buffer->buffer + BUFF_SHIFT;
		size -= DATAMESSAGE_LIMIT;

		buffer->next = new SerializedBuffer;
		buffer = buffer->next;
	}

	buffer->buffer_size = size;
	if (!MemoryPool::GetInstance().getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->buffer)) {
		return ret;
	}
	buffer->buffer = buffer->buffer + BUFF_SHIFT;

	typeSupport_->serialize_data(data, bufferHeader, TypeSupport::IsLittleEndian());

	CacheChange* cacheChange = writer_->NewChange(NOT_ALIVE_DISPOSED, buffer, handle);
	if (nullptr == cacheChange) {
		return RETCODE_ERROR;
	}

	ret = writer_->AddNewCachce(cacheChange);
	if (RETCODE_OK != ret) {
		return ret;
	}

    return RETCODE_OK;
}
