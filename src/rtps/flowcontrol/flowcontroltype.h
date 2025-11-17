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

#ifndef TRAVODDS_RTPS_FLOWCONTROL_FLOWCONTROLTYPE_H
#define TRAVODDS_RTPS_FLOWCONTROL_FLOWCONTROLTYPE_H

#include "travodds_dll.h"
#include "type/basictype.h"
TRAVODDS_NAMESPACE_BEGIN

enum class FlowControllerKind : uint8_t {
	// 完全同步,阻塞发送线程，直到接收线程处理完数据，策略调度无效
	SYNC,
	// 异步
	ASYNC,
	// 半同步，新数据同步，旧重传数据异步
	SEMI_SYNC
};

enum class SchedulerPolicy : uint8_t {
	// 先进先出
	FIFO
};

struct FlowParameters
{
    int32_t maxBytesPerPeriod = 0; // 每个周期内最大发送字节数，0表示不限制

	uint64_t periodMs = 1000; // 周期长度，单位毫秒
	FlowParameters() = default;
	FlowParameters(int32_t maxBytes, uint64_t period)
		: maxBytesPerPeriod(maxBytes), periodMs(period) {
	}
};

struct FlowControllerConfig {
	FlowControllerKind kind = FlowControllerKind::SEMI_SYNC;

	SchedulerPolicy policy = SchedulerPolicy::FIFO;
};



TRAVODDS_NAMESPACE_END

#endif
