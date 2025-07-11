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

TRAVODDS_NAMESPACE_BEGIN

using TimerCallBack = std::function<void()>;

class Timer;

/**
 * @brief 定时器类型枚举.
 */
enum TimerType{
	/* 单次执行定时器 */
	TIMER_SINGLE = 0,
	/* 多次执行定时器 */
	TIMER_MULTIPLE = 1
};

/**
 * @brief 定时器管理类.
 */
class TimerManager {
public:

	/**
	 * @brief 定时器创建控制函数。
	 *
	 * @param [in] time     定时器执行时间间隔。
	 * @param [in] callback	定时执行函数句柄。
	 * @param [in] name	    定时器名。
	 * @param [in] type	    定时器类型（默认值：TIMER_MULTIPLE）。
	 *
	 * @return 定时器指针。
	 */
	static std::shared_ptr<Timer> CreateTimer(const Time_t& time, TimerCallBack callback, const std::string& name = "", TimerType type = TIMER_MULTIPLE);

private:
	/**
	 * @brief 构造函数。
	 */
	TimerManager();

	/**
	 * @brief 内置定时器创建控制函数。
	 *
	 * @param [in] time     定时器执行时间间隔。
	 * @param [in] callback	定时执行函数句柄。
	 * @param [in] name	    定时器名。
	 * @param [in] type	    定时器类型（默认值：TIMER_MULTIPLE）。
	 *
	 * @return 定时器指针。
	 */
	std::shared_ptr<Timer> PrivateCreateTimer(const Time_t& time, TimerCallBack callback, const std::string& name = "", TimerType type = TIMER_SINGLE);

	/**
	 * @brief 定时器管理类单例模式指针获取函数。
	 * 
	 * @return 定时器管理类指针。
	 */
	static TimerManager* GetIntance();

};

class Timer {
public:
	
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] period     定时器执行时间间隔。
	 * @param [in] callback	定时执行函数句柄。
	 * @param [in] type	    定时器类型。
	 * @param [in] name	    定时器名。
	 */
	Timer(std::chrono::milliseconds period,TimerCallBack callback, TimerType type, const std::string& name);

	/**
	 * @brief 析构函数。
	 */
	~Timer();
	
	/**
	 * @brief 获取下一次定时函数执行的时间。
	 *
	 * @return 下一次定时函数执行的时间。
	 */
	std::chrono::steady_clock GetNextTime();

	/**
	 * @brief 开始定时器。
	 */
	void Start();

	/**
	 * @brief 暂停定时器。
	 */
	void Stop();

	/**
	 * @brief 停止定时器
	 */
	void Exit();

	/**
	 * @brief 更新下次执行的时间。
	 */
	void UpdateNextTime();

	/**
	 * @brief 获取还需要等待的时间。
	 */
	std::chrono::milliseconds GetTriggerTime();

	/**
	 * @brief 定时函数执行控制。
	 */
	void Run();

	/**
	 * @brief 设置线程名描述。
	 *  长度16字节包括休止符。
	 */
	void SetThreadName();

	/* 支持跳过当前定时执行，重新设置间隔时间 */
	// void restartTimer();
	//
	// void restartTimer(std::chrono::milliseconds period);

	/**
	 * @brief 重启定时器。
	 */
	void ReStart();

	/**
	 * @brief 更新定时器时间间隔。
	 */
	bool UpdatePeriod(const Duration_t & period);

	/**
	 * @brief 更新定时器时间间隔,间隔可负，运行超时触发。
	 */
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

TRAVODDS_NAMESPACE_END
