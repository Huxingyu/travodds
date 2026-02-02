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

#ifndef TRAVODDS_RTPS_FLOWCONTROL_FLOWRATELIMITEDMANAGER_H
#define TRAVODDS_RTPS_FLOWCONTROL_FLOWRATELIMITEDMANAGER_H

#include <condition_variable>
#include <atomic>
#include "rtps/flowcontrol/flowcontroltype.h"
#include "rtps/basictypertps.h"
#include "dcps/base/retcode.h"

TRAVODDS_NAMESPACE_BEGIN

class Timer;
class Participant;
/* yuguxu 对带宽加以周期限制，控制发送速率，唤醒等待线程 */
class FlowRateLimitedManager
{
public:
	FlowRateLimitedManager(Participant* participant, const FlowParameters& parameters);

	~FlowRateLimitedManager();
	//  periodMs <= 0 停止限流 
	// 设置每个周期内最大发送字节数，maxBytesPerPeriod<= 0 停止限流
	void ResetPeriod(const FlowParameters& parameters);
    
	ReturnCode_t MessageSend(std::vector<LocatorVec>& destinationList, char* message, uint32_t length, const Duration_t& timeOut);
	
protected:
	// 对发送请求进行过滤
	bool PrepareSend(int32_t bytesToSend);
	// 创建一个周期性定时器
	std::shared_ptr<Timer> CreateTimer();
	// 获取当前周期剩余时间
	std::chrono::milliseconds GetRemainingTime();

private:
	Participant* participant_;
	int32_t maxBytesPerPeriod_; // 每个周期内最大发送字节数，0表示不限制
	int32_t bytesAvailableThisPeriod_; // 当前周期内可发送的字节数
	int64_t periodMs_; // 周期长度，单位毫秒
	int32_t currentTicket_;
	std::atomic<int32_t> nextTicket_;
	std::set<int32_t> timeoutTicket_; // 记录超时的ticket
	std::condition_variable cv_;
	std::mutex mtx_;
	std::shared_ptr<Timer> timer_;
};
TRAVODDS_NAMESPACE_END
#endif