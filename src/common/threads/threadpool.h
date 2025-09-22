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
#ifndef TRAVODDS_COMMON_THREADS_THREADPOOL_H
#define TRAVODDS_COMMON_THREADS_THREADPOOL_H

#include "travodds_dll.h"
#include <vector>       
#include <queue>        
#include <functional>   
#include <utility>      
#include <thread>       
#include <mutex>        
#include <future>
#include <atomic>

TRAVODDS_NAMESPACE_BEGIN
/* 使用std::queue的队列的线程安全实现 */ 
using ThreadPoolTask = std::function<void()>;

/**
 * @brief SafeQueue类,线程池执行函数安全队列，即任务队列。
 */
class SafeQueue {

public:
	/**
	 * @brief 构造函数。
	 */
	SafeQueue();

	/**
	 * @brief 析构函数。
	 */
	~SafeQueue();

	/**
	 * @brief 判断队列是否为空函数。
	 * 
	 * @return 队列为空返回true，否则返回false。
	 */
	bool Empty();

	/**
	 * @brief 获取队列大小。
	 *
	 * @return 队列大小。
	 */
	int32_t Size();

	/**
	 * @brief 向队列中添加元素。
	 *
	 * @param [in] t  队列元素。
	 */
	void enqueue(ThreadPoolTask& t);

	/**
	 * @brief 从队列中取出元素。
	 *
	 * @param [out] t 队列元素。
	 * 
	 * @return 获取成功返回true，否则返回false。
	 */
	bool dequeue(ThreadPoolTask& t);

private:
	/* 利用模板函数构造队列 */
	std::queue<ThreadPoolTask> queue_;

	/* 访问互斥信号量 */
	std::mutex mutex_;
};

class ThreadPool {

public:
	/**
	 * @brief 线程池构造函数。
	 *
	 * @param [in] threadNum 工作队列构造值（默认值：4）。
	 */
	ThreadPool(const int32_t threadNum = 4);
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool &operator=(const ThreadPool&) = delete;
	ThreadPool &operator=(ThreadPool&&) = delete;

	/**
	 * @brief 析构函数。
	 */
	~ThreadPool();

	/**
	 * @brief 关闭线程池。
	 */
	void shutdown();

	/**
	 * @brief 线程入口函数。
	 */
	void ThreadWorker();

	/**
	 * @brief 提交线程池中异步执行的函数。
	 */
	template <typename F, typename... Args>
	auto submit(F&& f, Args&& ...args) -> std::future<decltype(f(args...))>
	{
		// Create a function with bounded parameter ready to execute
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...); // 连接函数和参数定义，特殊函数类型，避免左右值错误
		// Encapsulate it into a shared pointer in order to be able to copy construct
		auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
		// Warp packaged task into void function
		std::function<void()> warpper_func = [task]() {
			(*task)();
		};
		// 队列通用安全封包函数，并压入安全队列
		queue_.enqueue(warpper_func);

		// 唤醒一个等待中的线程
		lock_.notify_one();

		// 返回先前注册的任务指针
		return task->get_future();
	}
private:
	/* 线程池是否关闭 */
	std::atomic_bool shutdown_;

	/* 执行函数安全队列，即任务队列 */
	SafeQueue queue_;

	/* 工作线程队列 */
	std::vector<std::shared_ptr<std::thread>> threads_;

	/* 线程休眠锁互斥变量 */
	std::mutex mutex_;

	/* 线程环境锁，可以让线程处于休眠或者唤醒状态 */
	std::condition_variable lock_;

};

TRAVODDS_NAMESPACE_END

#endif
