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

#include "type/basictype.h"
#include "agidds_dll.h"

#include <chrono>
#include <functional>
#include <condition_variable>
#include <iostream>
#include <atomic>

AGIDDS_NAMESPACE_BEGIN

using TimerCallBack = std::function<void()>;

class Timer;

enum TimerType{
	TIMER_SINGLE = 0,
	TIMER_MULTIPLE = 1
};

class TimerManager {
public:

	static std::shared_ptr<Timer> CreateTimer(const Time_t& time, TimerCallBack callback, const std::string& name = "", TimerType type = TIMER_MULTIPLE);

private:
	TimerManager();

	/* 创建一个定时器 */
	//Timer* CreateTimer(Time_t& time);

	std::shared_ptr<Timer> PrivateCreateTimer(const Time_t& time, TimerCallBack callback, const std::string& name = "", TimerType type = TIMER_SINGLE);

	static TimerManager* GetIntance();

};

class Timer {
public:
	
	Timer(std::chrono::milliseconds period,TimerCallBack callback, TimerType type, const std::string& name);

	~Timer();
	
	/* 获取下一次执行的时间  */
	std::chrono::steady_clock GetNextTime();

	/* 开始定时器 */
	void Start();

	/* 暂停定时器 */
	void Stop();

	/* 停止定时器 */
	void Exit();

	/* 更新下次跟新的时间  */
	void UpdateNextTime();

	/* 获取还需要等待的时间  */
	std::chrono::milliseconds GetTriggerTime();

	void Run();
	/* 设置线程名描述 长度16字节包括休止符 */
	void SetThreadName();

	/* 支持跳过当前定时执行，重新设置间隔时间 */
	// void restartTimer();
	//
	// void restartTimer(std::chrono::milliseconds period);

	/* 重启定时器 */
	void ReStart();

	/* 更新定时器时间间隔 */
	bool UpdatePeriod(const Duration_t & period);

	/* 更新定时器时间间隔,间隔可负，运行超时触发 */
	bool UpdatePeriod(int64_t period);
private:
	std::string name_;

	/* 下一次触发的时间 */
	std::chrono::steady_clock::time_point nextTime_;

	/* 定时器的时间间隔 */
	std::chrono::milliseconds period_;

	/* 定时器的执行的回调函数 */
	TimerCallBack callback_;

	std::atomic_bool run_;

	std::atomic_bool exit_;

	std::atomic_bool restart_;
	/* run计数 */
	std::atomic_size_t runs_;
	
	std::condition_variable_any cva_;

	std::shared_ptr<std::thread> thread_ = nullptr;

	/* 定时器的种类 */
	TimerType type_;

	std::mutex mutex_;
};

AGIDDS_NAMESPACE_END
