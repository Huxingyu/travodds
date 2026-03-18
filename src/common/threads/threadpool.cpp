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
	shutdown_ = true;
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
	ThreadPoolTask task; // 定义基础函数类func
	bool dequeued = false; // 是否正在取出队列中元素
	while (!shutdown_) {
		// 为线程环境加锁，互访问工作线程的休眠和唤醒
		std::unique_lock<std::mutex> lock(mutex_);
		// 如果任务队列为空，阻塞当前线程
		if (queue_.Empty()) {
			lock_.wait(lock, [this] {return shutdown_ || !queue_.Empty(); }); // 等待条件变量通知，开启线程
		}
		// 取出任务队列中的元素
		dequeued = queue_.dequeue(task);
		// 如果成功取出，执行工作函数

		if (dequeued) {
			task();
		}
	}

}
