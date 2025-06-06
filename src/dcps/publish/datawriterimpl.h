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
#ifndef AGIDDS_DCPS_PUBLISH_DATAWRITERIMPL_H
#define AGIDDS_DCPS_PUBLISH_DATAWRITERIMPL_H

#include "dcps/publish/datawriter.h"
#include "dcps/domain/domainentityimpl.h"
#include "dcps/topic/typesupport.h"
#include "rtps/historycache/historycache.h"

AGIDDS_NAMESPACE_BEGIN

class Writer;
class TypeSupport;
class PublisherImpl;
class TopicImpl;
class Timer;
class DataWriterStatusManager;
class DataWriterImpl : public virtual DataWriter, public DomainEntityImpl {
public:
	DataWriterImpl(Topic* topic, const DataWriterQos& qos, DataWriterListener* listener, StatusMask mask, PublisherImpl* pub, TypeSupport* typeSupport);

    ~DataWriterImpl();

    ReturnCode_t set_qos(const DataWriterQos& qos);

    ReturnCode_t get_qos(DataWriterQos& qos) const;

    ReturnCode_t set_listener(DataWriterListener* listener, const StatusMask& mask);

    const DataWriterListener* get_listener() const;

	InstanceHandle_t register_instance(const void* const instance_data);

	InstanceHandle_t register_instance_w_timestamp(const void* const instance_data, const Time_t& source_timestamp);

    ReturnCode_t unregister_instance(const void* const instance_data, const InstanceHandle_t& handle) ;

	ReturnCode_t unregister_instance_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp);

	ReturnCode_t get_key_value(void* key_holder, const InstanceHandle_t& handle);

	InstanceHandle_t lookup_instance(const void* const instance_data) const;

	ReturnCode_t write(const void* const instance_data, const InstanceHandle_t& handle);

	ReturnCode_t write_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp);

	ReturnCode_t dispose(const void* const instance_data, const InstanceHandle_t& handle);

	ReturnCode_t dispose_w_timestamp(const void* const instance_data, const InstanceHandle_t& handle, const Time_t& source_timestamp);

	ReturnCode_t wait_for_acknowledgments(const Duration_t& max_wait);

	ReturnCode_t get_liveliness_lost_status(LivelinessLostStatus& status);

	ReturnCode_t get_offered_deadline_missed_status(OfferedDeadlineMissedStatus& status);

	ReturnCode_t get_offered_incompatible_qos_status(OfferedIncompatibleQosStatus& status);

	ReturnCode_t get_publication_matched_status(PublicationMatchedStatus& status);

	Topic* get_topic() const;

	const Publisher* get_publisher() const;

	ReturnCode_t assert_liveliness();

	ReturnCode_t get_matched_subscription_data(SubscriptionBuiltinTopicData& subscription_data, const InstanceHandle_t& subscription_handle);

	ReturnCode_t get_matched_subscriptions(InstanceHandleSeq& subscription_handles);
#ifdef _XML_INTERFACE_CPP

	virtual ReturnCode_t set_qos_with_profile(
		const char* library_name,
		const char* profile_name,
		const char* qos_name);

#endif // _XML_INTERFACE_CPP
public:
	ReturnCode_t enable();

	void SetRelatedWriter(std::shared_ptr<Writer> writer);

	ReturnCode_t UpdateLivelinessLostStatus();

	ReturnCode_t UnRegisteOrDispose(const void* const data, const InstanceHandle_t& handle, const Time_t& timespace, bool dispose = false);

	void OnLivelinessLost(DataWriter* the_writer, const LivelinessLostStatus& status);

	void OnOfferedDeadlineMissed();

	void OnOfferedIncompatibleQos(std::set<QosPolicyId_t>& qosSeq);

	void OnPublicationMatched(InstanceHandle_t& handle, const bool& add = true);

    const PublicationBuiltinTopicData& GetBuiltinTopicData();

	bool MakeKey(const void* const data, InstanceHandle_t& handle, bool forceMd5) const;
	
	/* 最后写入的样本实例 */
	const InstanceHandle_t& GetLastinstanceHandle() const;

	ReturnCode_t RegistInstance(const void* const instance_data, const Time_t& source_timestamp, InstanceHandle_t& handle);

	/* 根据数据的长度，创建对应的SerializedBuffer */
	ReturnCode_t CreateSerializedBuffer(SerializedBuffer* &bufferHeader, uint32_t dataLen);
	
	/* 释放对应的空间 */
	ReturnCode_t DeleteSerializedBuffer(SerializedBuffer* buffer);

private:

    TopicImpl* topic_ = nullptr;
    DataWriterQos qos_;
    DataWriterListener* listener_ = nullptr;
    PublisherImpl* pub_ = nullptr;
	/* 相关联的RTPS对象 */
	std::shared_ptr<Writer>  writer_;
	/* 相关的数据类型支持的对象  */
	std::unique_ptr<DataWriterStatusManager> statusMgr_;
	std::shared_ptr<Timer> deadlineTimer_;
	InstanceHandle_t lastInstance_;
	mutable std::mutex lastInstanceMtx_;
    PublicationBuiltinTopicData builtinData_;

	std::shared_ptr<MemoryPool> memoryPool_;
protected:
	TypeSupport* typeSupport_ = nullptr;

	std::shared_ptr<HistoryCache> history_ = nullptr;
};
AGIDDS_NAMESPACE_END

#endif // !AGIDDS_DCPS_PUBLISH_DATAWRITERIMPL_H
