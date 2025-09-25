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

/**
 * @brief 存储的数据项结构体
 * 
 * 包含DDS数据样本的完整信息，包括序列化数据、元信息、QoS策略等。
 * 用于在内存缓存和数据库中统一表示持久化的数据。
 */
struct StoredData {
    std::string topic_name;                      ///< Topic名称
    std::string type_name;                       ///< 数据类型名称
    std::string instance_id;                     ///< 实例标识符
    std::vector<uint8_t> data;                   ///< 序列化的数据内容
    SampleInfo sample_info;                      ///< DDS样本信息，包含时间戳等
    Time_t storage_time;                         ///< 数据存储时间
    uint64_t sequence_number;                    ///< 序列号，用于数据排序
    bool is_disposed = false;                    ///< 是否被dispose操作标记
    GUID_t writer_guid;                          ///< 数据源DataWriter的GUID，用于区分不同数据源
    
    // 完整的QoS信息，用于兼容性检查
    DurabilityQosPolicyKind durability_kind;     ///< 持久化策略类型
    ReliabilityQosPolicyKind reliability_kind;   ///< 可靠性策略类型
    PresentationQosPolicy presentation;          ///< 展现策略
    DeadlineQosPolicy deadline;                  ///< 截止时间策略
    LatencyBudgetQosPolicy latency_budget;       ///< 延迟预算策略
    OwnershipQosPolicy ownership;                ///< 所有权策略
    OwnershipStrengthQosPolicy ownership_strength; ///< 所有权强度策略
    LivelinessQosPolicy liveliness;              ///< 活跃性策略
    DestinationOrderQosPolicy destination_order; ///< 目标顺序策略
    LifespanQosPolicy lifespan;                  ///< 生命周期策略
    
    /**
     * @brief 转换为字符串表示
     * 
     * 为Qt界面显示提供的便利方法，返回数据的简要描述。
     * 
     * @return 数据的字符串描述
     */
    [[nodiscard]] auto to_string() const -> std::string {
        return "Topic: " + topic_name + ", Type: " + type_name + 
               ", Size: " + std::to_string(data.size()) + " bytes";
    }
};

/**
 * @brief 内存缓存项结构体
 * 
 * 在内存缓存中存储数据时使用的包装结构，添加了缓存时间戳信息。
 */
struct MemoryCacheItem {
    StoredData data;                                          ///< 存储的数据
    std::chrono::steady_clock::time_point timestamp;         ///< 缓存时间戳
    
    /**
     * @brief 构造函数
     * 
     * @param stored_data 要缓存的存储数据
     */
    MemoryCacheItem(StoredData stored_data) 
        : data(std::move(stored_data)), timestamp(std::chrono::steady_clock::now()) {}
};

TRAVODDS_NAMESPACE_END