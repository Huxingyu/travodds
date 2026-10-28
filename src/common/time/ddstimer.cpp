#include "common/time/ddstimer.h"
#include "common/sys/sysinfo.h"

USING_TRAVODDS_NAMESPACE

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
	if (time == Time_t(TIME_INFINITE)) {
		period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(time.sec));
	}

	std::shared_ptr<Timer> timer = std::make_shared<Timer>(period, callback, type, name);

	timer->Start();

	return timer;
}

// Worker state has independent lifetime: callbacks may release the last Timer
// reference. No worker accesses Timer after invoking application code.
struct Timer::Runtime {
    std::mutex mutex;
    std::condition_variable changed;
    std::chrono::milliseconds period;
    std::chrono::steady_clock::time_point next;
    TimerCallBack callback;
    TimerType type;
    std::string name;
    uint64_t revision = 0;
    bool running = false, exiting = false;
};

Timer::Timer(std::chrono::milliseconds period, TimerCallBack callback, TimerType type, const std::string& name)
    : runtime_(std::make_shared<Runtime>())
{
    runtime_->period = std::max(period, std::chrono::milliseconds(1));
    runtime_->next = std::chrono::steady_clock::now() + runtime_->period;
    runtime_->callback = std::move(callback);
    runtime_->type = type;
    runtime_->name = name;
}
Timer::~Timer() { Exit(); }
void Timer::Start() {
    auto state = runtime_;
    std::lock_guard<std::mutex> threadLock(threadMutex_);
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->exiting) return;
    state->running = true;
    state->next = std::chrono::steady_clock::now() + state->period;
    ++state->revision;
    if (!thread_.joinable()) thread_ = std::thread([state] { RunRuntime(state); });
    state->changed.notify_all();
}
void Timer::Stop() {
    auto state = runtime_;
    std::lock_guard<std::mutex> lock(state->mutex);
    state->running = false; ++state->revision;
    state->changed.notify_all();
}
void Timer::Exit() {
    auto state = runtime_;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->exiting = true; state->running = false; ++state->revision;
        state->changed.notify_all();
    }
    std::thread pending;
    {
        std::lock_guard<std::mutex> threadLock(threadMutex_);
        if (thread_.joinable()) {
            if (thread_.get_id() == std::this_thread::get_id()) thread_.detach();
            else pending = std::move(thread_);
        }
    }
    if (pending.joinable()) pending.join();
}
void Timer::RunRuntime(const std::shared_ptr<Runtime>& state) {
    ThreadFunction::SetThreadName(state->name);
    std::unique_lock<std::mutex> lock(state->mutex);
    while (!state->exiting) {
        state->changed.wait(lock, [&] { return state->exiting || state->running; });
        if (state->exiting) break;
        const auto revision = state->revision;
        const auto due = state->next;
        if (state->changed.wait_until(lock, due, [&] {
                return state->exiting || !state->running || state->revision != revision;
            })) continue;
        auto callback = state->callback;
        if (state->type == TIMER_SINGLE) state->running = false;
        lock.unlock();
        try { callback(); } catch (...) {
            // Keep the timer worker valid if an application callback throws.
        }
        lock.lock();
        if (state->revision == revision)
            state->next = std::chrono::steady_clock::now() + state->period;
    }
}
void Timer::Run() { RunRuntime(runtime_); }
void Timer::UpdateNextTime() {
    auto state = runtime_;
    std::lock_guard<std::mutex> lock(state->mutex);
    state->next = std::chrono::steady_clock::now() + state->period;
    ++state->revision; state->changed.notify_all();
}
std::chrono::steady_clock::time_point Timer::GetNextTime() {
    auto state = runtime_;
    std::lock_guard<std::mutex> lock(state->mutex);
    return state->next;
}
std::chrono::milliseconds Timer::GetTriggerTime() {
    auto state = runtime_;
    std::lock_guard<std::mutex> lock(state->mutex);
    return std::chrono::duration_cast<std::chrono::milliseconds>(state->next - std::chrono::steady_clock::now());
}
void Timer::ReStart() {
    auto state = runtime_;
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->exiting) return;
    state->running = true;
    state->next = std::chrono::steady_clock::now() + state->period;
    ++state->revision; state->changed.notify_all();
}
bool Timer::UpdatePeriod(const Duration_t& period) {
    return UpdatePeriod(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::seconds(period.sec) + std::chrono::nanoseconds(period.nanosec)).count());
}
bool Timer::UpdatePeriod(int64_t period) {
    auto state = runtime_;
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->exiting) return false;
    state->period = std::chrono::milliseconds(std::max<int64_t>(1, period));
    state->next = std::chrono::steady_clock::now() + state->period;
    state->running = true;
    ++state->revision; state->changed.notify_all();
    return true;
}
