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
#ifndef TRAVODDS_RTPS_BUILTIN_DPR_DYNAMICPERFORMANCEREGULATOR_H
#define TRAVODDS_RTPS_BUILTIN_DPR_DYNAMICPERFORMANCEREGULATOR_H

#include "rtps/builtin/dpr/dynamicperformancetype.h"
#include "rtps/flowcontrol/flowcontroltype.h"
#include "dcps/base/status.h"
#include <deque>
TRAVODDS_NAMESPACE_BEGIN

class Timer;
class FlowRateLimitedManager;

class DynamicPerformanceRegulator {
public:
	DynamicPerformanceRegulator(const DynamicAdjustQosPolicy& policy, FlowRateLimitedManager* flowRateLimitedManager);

	virtual ~DynamicPerformanceRegulator();

	void RegisterInterface(std::string face);

	void UnregisterInterface(std::string face);

private:
	struct Score {
		double value; // 性能分数
		double txmbps; // 发送速率 Mbps
		double txrateratio; // 发送速率与链路速率比值
		Score() : value(0), txmbps(0), txrateratio(0) {
		}
	};
private:
	void UpdateNetDevStats();

	Score CalculateMetricsScore(const NetworkStats& currentStats, const NetworkStats& lastStats) const;

	void Adjustment(const Score& score);

	bool IsWirelessInterface(const std::string& interface) const;

	bool PushBack(FlowParameters& para);
	void PopBack(FlowParameters& para);

private:
	FlowRateLimitedManager* flowRateLimitedManager_;
	std::map<std::string, NetworkStats> facestats_;
	mutable std::mutex statsMtx_;

	std::shared_ptr<Timer> timer_;

	DynamicPerformanceConfig config_;
	// 当前已应用的流控参数
	std::deque<FlowParameters> flowparameters_;
	uint32_t count_;


};

TRAVODDS_NAMESPACE_END

#endif