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
#ifndef TRAVODDS_RTPS_BUILTIN_DPR_DYNAMICPERFORMANCETYPE_H
#define TRAVODDS_RTPS_BUILTIN_DPR_DYNAMICPERFORMANCETYPE_H

#include "travodds_dll.h"
#include "type/basictype.h"
#include "qos/qos.h"

TRAVODDS_NAMESPACE_BEGIN


struct NetworkStats {
	bool noInitStats;
    // 总带宽
    double linkSpeed;
	// 接收统计
	uint64_t rxBytes;               // 接口接收到的总字节数
	uint64_t rxPackets;           // 成功接收的数据包总数
	uint64_t rxErrs;                 // 接收错误的数据包数量
	uint64_t rxDrop;                // 被内核丢弃的包数
	uint64_t rxFifo;                  // FIFO缓冲区错误数
	uint64_t rxFrame;             // 帧对齐错误数
	uint64_t rxMulticast;       // 接收的多播数据包数

	// 发送统计
	uint64_t txBytes;              // 接口发送的总字节数
	uint64_t txPackets;          // 成功发送的数据包总数
	uint64_t txErrs;                // 发送错误的数据包数
	uint64_t txDrop;               // 发送时丢弃的包数
	uint64_t txFifo;                  // 发送FIFO缓冲区错误
	uint64_t txColls;                // 以太网冲突次数
	uint64_t txCarrier;            // 载波信号错误数

    NetworkStats() : noInitStats(false), linkSpeed(0), rxBytes(0), rxPackets(0), rxErrs(0), rxDrop(0),
        rxFifo(0), rxFrame(0), rxMulticast(0),
        txBytes(0), txPackets(0), txErrs(0), txDrop(0),
        txFifo(0), txColls(0), txCarrier(0) {
    }
};


struct EnhancedNetworkMetrics {
    // 总带宽
    double linkSpeed;
    // 基础速率指标
    double rxRatembps;
    double txRatembps;

    // 接收质量指标
    double rxPacketLossRate;     // 基于 rx_drop
    double rxErrorRate;           // 基于 rx_errs
    double rxFifoErrorRate;      // FIFO缓冲区错误率
    double rxFrameErrorRate;     // 帧对齐错误率

    // 发送质量指标  
    double txErrorRate;           // 发送错误率
    double txDropRate;            // 发送丢包率
    double txFifoErrorRate;      // FIFO缓冲区错误率
    double txCollisionRate;       // 冲突率
    double txCarrierErrorRate;   // 载波错误率
    // 网络特性
    double multicastRatio;         // 多播流量比例

    EnhancedNetworkMetrics()
        : rxRatembps(0), txRatembps(0),
        rxPacketLossRate(0), rxErrorRate(0),
        rxFifoErrorRate(0), rxFrameErrorRate(0),
        txErrorRate(0), txDropRate(0), txFifoErrorRate(0),
        txCollisionRate(0), txCarrierErrorRate(0),
        multicastRatio(0), linkSpeed(0) {
    }
};

/**
 * @brief 动态性能调整配置参数
 */
struct DynamicPerformanceConfig
{
	// 动态性能调整策略
    DynamicAdjustQosPolicy policy;
    
    // 流控调整参数
	int32_t bytesPerPeriod = 625000000;        // 每个周期最大字节数
	uint64_t flowcontrolIntervalMs = 1000;        // 基础流控间隔(毫秒)
	uint64_t flowcontrolMaxIntervalMs = 5000;       // 最大流控间隔(毫秒)
	uint64_t flowcontrolMinIntervalMs = 10;         // 最小流控间隔(毫秒)
	uint64_t flowcontrolMinSpeed = 50;         // 最低流控流量Mbps

    DynamicPerformanceConfig() = default;
    
    DynamicPerformanceConfig(const DynamicAdjustQosPolicy& qosPolicy)
        : policy(qosPolicy) {
    }
};

TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_RTPS_BUILTIN_DPR_DYNAMICPERFORMANCETYPE_H