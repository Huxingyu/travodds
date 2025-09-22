#pragma once

#include "../../../include/travodds_dll.h"
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

// DDS相关包含
#include "dcps/domain/domainparticipant.h"
#include "dcps/subscribe/datareader.h"
#include "dcps/publish/datawriter.h"
#include "type/basictype.h"
#include "type/builtintopicdata.h"
#include "../../../src/rtps/basictypertps.h"  // 为了使用GUID_t

TRAVODDS_NAMESPACE_BEGIN

// 存储的数据项结构
struct StoredData {
    std::string topic_name;     //主题名称
    std::string type_name;      // 类型名称
    std::string instance_id;      // 实例标识
    std::vector<uint8_t> data;    
    SampleInfo sample_info;
    Time_t storage_time;
    uint64_t sequence_number;     // 序列号，便于排序
    bool is_disposed = false;     // 是否被dispose
    GUID_t writer_guid;          // 数据来源的Writer GUID，用于区分不同的数据源
    
    // 完整的QoS信息，用于兼容性检查
    DurabilityQosPolicyKind durability_kind;
    ReliabilityQosPolicyKind reliability_kind;
    PresentationQosPolicy presentation;
    DeadlineQosPolicy deadline;
    LatencyBudgetQosPolicy latency_budget;
    OwnershipQosPolicy ownership;
    OwnershipStrengthQosPolicy ownership_strength;
    LivelinessQosPolicy liveliness;
    DestinationOrderQosPolicy destination_order;
    LifespanQosPolicy lifespan;
    
    // 为Qt显示提供的便利方法
    [[nodiscard]] auto to_string() const -> std::string {
        return "Topic: " + topic_name + ", Type: " + type_name + 
               ", Size: " + std::to_string(data.size()) + " bytes";
    }
};

// 内存缓存项
struct MemoryCacheItem {
    StoredData data;
    std::chrono::steady_clock::time_point timestamp;
    
    MemoryCacheItem(StoredData stored_data) 
        : data(std::move(stored_data)), timestamp(std::chrono::steady_clock::now()) {}
};

TRAVODDS_NAMESPACE_END