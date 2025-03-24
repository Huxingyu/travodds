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

#include "BasicType.h"
#include "AGIDDS_DLL.h"

#include <chrono>
#include <functional>
#include <iostream>


AGIDDS_NAMESPACE_BEGIN

using TimerCallBack = std::function<void()>;

class Timer;

enum TimerType{
	single = 0,
	multiple = 1
};

class TimerManager {
public:
	TimerManager();

	/* 创建一个定时器 */
	//Timer* CreateTimer(Time_t& time);
	Timer* CreateTimer(Time_t& time, TimerCallBack callback, TimerType type = multiple);

	int DeleteTimer(Timer* timer);

};

class Timer {
public:
	
	Timer(std::chrono::milliseconds period,TimerCallBack callback, TimerType type);

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
private:

	/* 下一次触发的时间 */
	std::chrono::steady_clock::time_point nextTime_;

	/* 定时器的时间间隔 */
	std::chrono::milliseconds period_;

	/* 定时器的执行的回调函数 */
	TimerCallBack callback_;

	bool run_;

	bool exit_;

	std::thread* thread_;

	/* 定时器的种类 */
	TimerType type_;
};

AGIDDS_NAMESPACE_END