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
#include "DDS/DataWriter.h"
#include "DDS/DomainEntityImpl.h"

#include "TypeSupport.h"
#include "RTPS/HistoryCache.h"
AGIDDS_NAMESPACE_BEGIN

class Writer;
class TypeSupport;
class PublisherImpl;
class TopicImpl;

class DataWriterStatusManager;
class DataWriterImpl : public virtual DataWriter, public DomainEntityImpl {
public:
	DataWriterImpl(Topic* a_topic, const DataWriterQos& qos, DataWriterListener* a_listener, StatusMask mask, PublisherImpl* a_pub, TypeSupport* a_typeSupport);

    ~DataWriterImpl();

    ReturnCode_t set_qos(const DataWriterQos& qos);

    ReturnCode_t get_qos(DataWriterQos& qos) const;

    ReturnCode_t set_listener(DataWriterListener* a_listener, const StatusMask& mask);

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
public:
	ReturnCode_t enable();

private:
    TopicImpl* topic_ = nullptr;
    DataWriterQos qos_;
    DataWriterListener* listener_ = nullptr;
    PublisherImpl* pub_ = nullptr;
	/* 相关联的RTPS对象 */
	Writer* writer_ = nullptr;
	/* 相关的数据类型支持的对象  */

	DataWriterStatusManager* statusMgr_ = nullptr;
protected:
	TypeSupport* typeSupport_ = nullptr;

	HistoryCache* history_ = nullptr;
};
AGIDDS_NAMESPACE_END

