#include "System/Timer/DDSTimer.h"

#include <condition_variable>

USING_AGIDDS_NAMESPACE

TimerManager::TimerManager()
{

}

Timer* TimerManager::CreateTimer(Time_t& time, TimerCallBack callback, TimerType type)
{
	std::chrono::milliseconds period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(time.nanosec) + std::chrono::seconds(time.sec));

	Timer* timer = new Timer(period, callback, type);

	timer->Start();

	return timer;
}

int TimerManager::DeleteTimer(Timer* timer)
{
	if (nullptr == timer) {
		return -1;
	}

	timer->Exit();

	return 0;
}


Timer::Timer(std::chrono::milliseconds period, TimerCallBack callback, TimerType type)
	: run_(false), exit_(false),callback_(callback), period_(period)
{
	nextTime_ = std::chrono::steady_clock::now() + period_;
}

void Timer::Start()
{
	run_ = true;

	if (multiple == type_) {
		thread_ = new std::thread([this]() {
			while (!this->exit_) {
				this->Run();
			}
		}
		);
	} else {
		std::thread thread([this]() {this->Run(); });
		thread.detach();
	}
}

void Timer::Stop()
{
	run_ = false;
}

/* Í£Ö¹¶¨Ê±Æ÷ */
void  Timer::Exit()
{
	exit_ = true;
}

void Timer::Run()
{
	std::condition_variable_any cva;
	std::mutex mutex;
	std::unique_lock<std::mutex> lock(mutex);

	cva.wait_until(lock, nextTime_);

	if (run_) {
		callback_();
	}

	UpdateNextTime();
}

void Timer::UpdateNextTime()
{
	nextTime_ = nextTime_ + period_;
}

std::chrono::milliseconds Timer::GetTriggerTime()
{
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(nextTime_ - std::chrono::steady_clock::now());
	return ms;
}