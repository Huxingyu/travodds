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

#include "DDS/Topic.h"
#include "Listener/DataWriterListener.h"
#include "BuiltinTopicData.h"

AGIDDS_NAMESPACE_BEGIN

class Publisher;

class DDS_DLL DataWriter : public virtual DomainEntity {
public:
    virtual ~DataWriter() = default;

	virtual ReturnCode_t set_qos(const DataWriterQos& qos) = 0;

	virtual ReturnCode_t get_qos(DataWriterQos& qos) const = 0;

	virtual ReturnCode_t set_listener(DataWriterListener* a_listener, const StatusMask& mask) = 0;

	virtual const DataWriterListener* get_listener() const = 0;

    virtual InstanceHandle_t register_instance(const void* const instance) = 0;

    virtual InstanceHandle_t register_instance_w_timestamp(const void* const instance, const Time_t& timestamp) = 0;

    virtual ReturnCode_t unregister_instance(const void* const instance, const InstanceHandle_t& handle) = 0;

    virtual ReturnCode_t unregister_instance_w_timestamp(const void* const instance, const InstanceHandle_t& handle, const Time_t& timestamp) = 0;

    virtual ReturnCode_t get_key_value(void* key_holder, const InstanceHandle_t& handle) = 0;

    virtual InstanceHandle_t lookup_instance(const void* const instance) const = 0;

    virtual ReturnCode_t write(const void* const data, const InstanceHandle_t& handle) = 0;

    virtual ReturnCode_t write_w_timestamp(const void* const data, const InstanceHandle_t& handle, const Time_t& timestamp) = 0;

    virtual ReturnCode_t dispose(const void* const data, const InstanceHandle_t& handle)= 0;

    virtual ReturnCode_t dispose_w_timestamp(const void* const data, const InstanceHandle_t& handle, const Time_t& timestamp) = 0;

    virtual ReturnCode_t wait_for_acknowledgments(const Duration_t& max_wait) = 0;

    virtual ReturnCode_t get_liveliness_lost_status(LivelinessLostStatus& status) = 0;

    virtual ReturnCode_t get_offered_deadline_missed_status(OfferedDeadlineMissedStatus& status) = 0;

    virtual ReturnCode_t get_offered_incompatible_qos_status(OfferedIncompatibleQosStatus& status) = 0;

    virtual ReturnCode_t get_publication_matched_status(PublicationMatchedStatus& status) = 0;

    virtual Topic* get_topic() const = 0;

    virtual const Publisher* get_publisher() const = 0;

    virtual ReturnCode_t assert_liveliness() = 0;

    virtual ReturnCode_t get_matched_subscription_data(SubscriptionBuiltinTopicData& subscription_data, const InstanceHandle_t& subscription_handle)= 0;

    virtual ReturnCode_t get_matched_subscriptions(InstanceHandleSeq& subscription_handles) = 0;
};

AGIDDS_NAMESPACE_END
