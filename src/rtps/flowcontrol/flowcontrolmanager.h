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

#ifndef AGIDDS_RTPS_FLOWCONTROL_FLOWCONTROLMANAGER_H
#define AGIDDS_RTPS_FLOWCONTROL_FLOWCONTROLMANAGER_H

#include "rtps/flowcontrol/flowcontroller.h"	
#include "rtps/flowcontrol/flowcontrolscheduler.h"	
AGIDDS_NAMESPACE_BEGIN

class Participant;

class FlowControlManager {
	using PolicyMap = std::map<SchedulerPolicy, std::unique_ptr<FlowController>>;
	using FlowControllerMap = std::map<FlowControllerKind, PolicyMap>;
public:
	FlowControlManager(Participant* participant);

	// 内部管理外部勿释放
	FlowController* GetFlowController(const FlowControllerConfig& config);
private:
	Participant* participant_;
	FlowControllerMap flowControllers_;
	std::mutex mtx_;

};

AGIDDS_NAMESPACE_END

#endif