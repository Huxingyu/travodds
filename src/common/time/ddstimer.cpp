#include "common/time/ddstimer.h"
#ifdef _WIN32
#include <windows.h>
#include <processthreadsapi.h>
#endif // _WIN32



#include <sstream>
#include <string>

USING_AGIDDS_NAMESPACE

TimerManager::TimerManager()
{

}
TimerManager* TimerManager::GetIntance()
{
	static TimerManager timerManager;
	return &timerManager;
}


std::shared_ptr<Timer> TimerManager::CreateTimer(const Time_t& time, TimerCallBack callback, const std::string& name, TimerType type)
{
	return GetIntance()->PrivateCreateTimer(time, callback, name, type);
}

std::shared_ptr<Timer> TimerManager::PrivateCreateTimer(const Time_t& time, TimerCallBack callback, const std::string& name, TimerType type)
{
	std::chrono::milliseconds period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(time.nanosec) + std::chrono::seconds(time.sec));

	std::shared_ptr<Timer> timer = std::make_shared<Timer>(period, callback, type, name);

	timer->Start();

	return timer;
}

Timer::Timer(std::chrono::milliseconds period, TimerCallBack callback, TimerType type, const std::string& name)
	: run_(false), exit_(false), restart_(false), 
	  callback_(callback), period_(period), type_(type), runs_(0), name_(name)
{
	nextTime_ = std::chrono::steady_clock::now() + period_;
}

Timer::~Timer()
{
	Exit();
}

void Timer::Start()
{

	run_.store(true);

	if (TIMER_MULTIPLE == type_) {
		if (!thread_.get()) {
			thread_.reset(new std::thread([this]() {
				SetThreadName();
				while (!this->exit_.load()) {
					this->Run();
				}
			}
			));
		}
	}
	else {
		std::thread thread([this]() {
			SetThreadName();
			this->Run(); 
			}
		);
		thread.detach();
	}
}

void Timer::Stop()
{
	run_.store(false);
	cva_.notify_one();
}

/* 停止定时器 */
void  Timer::Exit()
{
	exit_.store(true);
	run_.store(false);
	cva_.notify_one();
	if (TIMER_MULTIPLE == type_ && thread_->joinable()) {
		thread_->join();
	}
	while (runs_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void Timer::Run()
{
	runs_++;
	std::unique_lock<std::mutex> lock(mutex_);

	cva_.wait_until(lock, nextTime_);

	if (restart_.load()) {
		restart_.store(false);
		nextTime_ = std::chrono::steady_clock::now() + period_;
		runs_--;
		return;
	}

	if (run_.load()) {
		callback_();
	}

	UpdateNextTime();

	runs_--;
}

void Timer::SetThreadName()
{
	if (name_.empty()) {
		return;
	}
	std::array<char, 16> buffer;
	snprintf(buffer.data(), buffer.size(), "%s", name_.data());
	buffer.back() = '\0';  // 确保以null结尾


#ifdef _WIN32
	std::wstringstream stream;
	stream << buffer.data();
	std::wstring wThreadName = stream.str();
	SetThreadDescription(GetCurrentThread(), wThreadName.data());
#else
	// Linux/macOS实现
	pthread_setname_np(pthread_self(), buffer.data());
#endif
}

// void Timer::restartTimer()
// {
// 	restartTimer(period_);
// }
//
// void Timer::restartTimer(std::chrono::milliseconds period)
// {
// 	restart_.store(true);
// 	period_ = period;
// 	cva_.notify_one();
// }

void Timer::UpdateNextTime()
{
	nextTime_ = nextTime_ + period_;
}

std::chrono::milliseconds Timer::GetTriggerTime()
{
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(nextTime_ - std::chrono::steady_clock::now());
	return ms;
}

void Timer::ReStart()
{
	restart_.store(true);

	cva_.notify_one();
}

bool Timer::UpdatePeriod(
	const Duration_t& period)
{
	period_ = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(period.nanosec) + std::chrono::seconds(period.sec));

	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(nextTime_ - std::chrono::steady_clock::now());

	//if (ms > period_)
		ReStart();

	return true;
}

bool Timer::UpdatePeriod(int64_t period)
{
	//std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(nextTime_ - std::chrono::steady_clock::now());

	period_ = std::chrono::milliseconds(period);
	//if (ms > period_) {
		ReStart();
	//}

	return true;
}

