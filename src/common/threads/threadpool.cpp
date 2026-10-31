#include "common/threads/threadpool.h"

USING_TRAVODDS_NAMESPACE;

SafeQueue::SafeQueue()
{

}
SafeQueue::~SafeQueue()
{

}
bool SafeQueue::Empty()
{
	std::unique_lock<std::mutex> lock(mutex_); // 互斥信号变量加锁，防止m_queue被改变
	return queue_.empty();
}

size_t SafeQueue::Size()
{
	std::unique_lock<std::mutex> lock(mutex_); // 互斥信号变量加锁，防止m_queue被改变
	return queue_.size();
}

void SafeQueue::enqueue(ThreadPoolTask &t)
{
	std::unique_lock<std::mutex> lock(mutex_);
	queue_.emplace(t);
}

bool SafeQueue::dequeue(ThreadPoolTask &t)
{
	std::unique_lock<std::mutex> lock(mutex_); // 队列加锁
	if (queue_.empty()) {
		return false;
	}
	t = std::move(queue_.front()); // 取出队首元素，返回队首元素值，并进行右值引用
	queue_.pop(); // 弹出入队的第一个元素
	return true;
}


ThreadPool::ThreadPool(const int32_t threadNum)
	:shutdown_(false)
{
	threads_.resize(threadNum);

	for (uint32_t i = 0; i < threads_.size(); ++i) {
		threads_.at(i) = std::make_shared<std::thread>(&ThreadPool::ThreadWorker, this); // 分配工作线程
	}
}

ThreadPool::~ThreadPool()
{
	shutdown();
}

void ThreadPool::shutdown()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		shutdown_ = true;
	}
	lock_.notify_all(); // 通知，唤醒所有工作线程
	for (uint32_t i = 0; i < threads_.size(); ++i) {
		// 判断线程是否在等待
		if (threads_.at(i)->joinable()) {
			threads_.at(i)->join(); // 将线程加入到等待队列
		}
	}
}
void ThreadPool::ThreadWorker()
{
	for (;;) {
		ThreadPoolTask task;
		{
			std::unique_lock<std::mutex> lock(mutex_);
			lock_.wait(lock, [this] { return shutdown_ || !queue_.Empty(); });
			// Shutdown finishes active tasks but does not drain queued work.
			if (shutdown_) return;
			if (!queue_.dequeue(task)) continue;
		}
		// Tasks may submit or wait for another worker; never hold its wait lock.
		task();
	}
}
