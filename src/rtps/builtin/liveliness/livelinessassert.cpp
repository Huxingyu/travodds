#include "rtps/builtin/liveliness/livelinessassert.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include <algorithm>
#include <functional>
#include "common/timeconversion.h"
#include "common/log/logger.h"
#include <iomanip>

USING_TRAVODDS_NAMESPACE;
using namespace std::chrono;

// 线程安全的时间间隔计算器
class IntervalCalculator {
public:
    IntervalCalculator() : first_call(true) {}

    // 获取与上一次调用的时间差（单位：毫秒）
    double get_interval() {
        std::lock_guard<std::mutex> lock(mtx);
        auto now = std::chrono::steady_clock::now();

        if (first_call) {
            first_call = false;
            last_time = now;
            return 0.0;
        }

        auto duration = now - last_time;
        last_time = now;
        return std::chrono::duration<double, std::milli>(duration).count();
    }

    // 重置计时器
    void reset() {
        std::lock_guard<std::mutex> lock(mtx);
        first_call = true;
    }

private:
    std::chrono::steady_clock::time_point last_time;
    bool first_call;
    std::mutex mtx;
};

IntervalCalculator intervalCalculator_;

LivelinessAssert::LivelinessAssert(LivelinessManager* livelinessManager, bool localWriter)
:livelinessManager_(livelinessManager), writers_(), mutex_(), checkedWrtier_(nullptr), localWriter_(localWriter)
{
    Time_t time = TIME_INFINITE;
    timer_ = TimerManager::CreateTimer(time, [&]() {this->TimerExpired(); }, "LivelinessAssert");
}

LivelinessAssert::~LivelinessAssert()
{
	if (timer_) {
		timer_->Exit();
	}
}

bool LivelinessAssert::AddWriter(LivelinesStatus writer)
{
    std::unique_lock<std::mutex> lock(mutex_);

	writer.time_ = steady_clock::now() + nanoseconds(writer.livelinessQos_.lease_duration.ToNs());

    auto result = writers_.emplace(writer.guid_, writer);
    if (!result.second) {
        /* 已存在,更新设置 */
        auto it = writers_.find(writer.guid_);
        it->second.count_++;
        it->second.livelinessQos_ = writer.livelinessQos_;
        return true;
    }

	UpDateCheckedWriter();

    return true;
}

bool LivelinessAssert::RemoveWriter(GUID_t guid)
{
    std::unique_lock<std::mutex> lock(mutex_);

    auto writer = writers_.find(guid);
    if (writer == writers_.end()) {
		TRAVODDS_LOG(LOG_ERROR, "guid is not exit");
        return false;
    }

	writer->second.count_--;
	if (writer->second.count_ != 0) {

		return true;
	}

    if (livelinessManager_ != nullptr && writer->second.status_ != WriterStatus::NOTASSERTED) {

       //livelinessManager_->LiveLinessStatusChanged(guid, localWriter_, writer->second.livelinessQos_, writer->second.status_);
    }

    writers_.erase(writer);

	UpDateCheckedWriter();

    return true;

}


bool LivelinessAssert::AssertLiveliness(GUID_t guid, LivelinessQosPolicyKind kind)
{
	std::unique_lock<std::mutex> lock(mutex_);

	if (writers_.empty()) {
		return true;
	}

	//更新指定Guid writer的存活状态
	if(MANUAL_BY_TOPIC_LIVELINESS_QOS == kind) {
		auto itStatus = writers_.find(guid);

		if (itStatus == writers_.end()) {
			TRAVODDS_LOG(LOG_ERROR, "guid is not exit");
			return false;
		}

		AssertWriterLiveliness(itStatus->second);
	}
    else {
        //更新相同guidprefix writer的存活状态
        for (auto writerIt = writers_.begin(); writerIt != writers_.end(); writerIt++)
        {
            if (writerIt->second.livelinessQos_.kind == kind && CompareGuidPrefix(guid.prefix, writerIt->first.prefix)) {
                AssertWriterLiveliness(writerIt->second);
            }
        }
    }

	UpDateCheckedWriter();

	return true;
}

bool LivelinessAssert::GetWriterLiveLinessKind(GUID_t guid, LivelinessQosPolicyKind & kind)
{
	std::unique_lock<std::mutex> lock(mutex_);

	if (0 == writers_.count(guid)) {
		TRAVODDS_LOG(LOG_ERROR, "guid is not exit");
		return false;
	}

	kind = writers_[guid].livelinessQos_.kind;

	return true;
}

bool LivelinessAssert::UpDateCheckedWriter()
{
    const auto now = steady_clock::now();
    Time_t timeInfinite = TIME_INFINITE;
    auto minTime = now + nanoseconds(timeInfinite.ToNs()); // 初始化为最大时间

    bool ret = false;

	for (auto writerIt = writers_.begin(); writerIt != writers_.end(); writerIt++)
	{
		if(writerIt->second.status_ != WriterStatus::LIVELINESSALIVE)
			continue;

		if (writerIt->second.time_ < minTime) {
			minTime = writerIt->second.time_;
			checkedWrtier_ = &writerIt->second;
			ret = true;
		}
	}

	if(ret && checkedWrtier_ != nullptr) {
		auto period = checkedWrtier_->time_ - now;
        
		timer_->UpdatePeriod(duration_cast<milliseconds>(period).count());
	}
    else         {
        checkedWrtier_ = nullptr;
        Time_t time = TIME_INFINITE;
        auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(time.sec));
        timer_->UpdatePeriod(duration_cast<milliseconds>(period).count());
    }

    return ret;
}

bool LivelinessAssert::TimerExpired()
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (checkedWrtier_ == nullptr) {
		/* TODO: 日志 */
		//AGILOG(AGILOG_ERROR, "checkedWrtier_ is nullptr");
        return false;
    }

    if (livelinessManager_ != nullptr && checkedWrtier_->status_ == WriterStatus::LIVELINESSALIVE) {

    	livelinessManager_->LiveLinessStatusChanged(checkedWrtier_->guid_, localWriter_, checkedWrtier_->livelinessQos_, checkedWrtier_->status_);

    }

	checkedWrtier_->status_ = WriterStatus::NOTALIVE;


	UpDateCheckedWriter();

    return true;
}


bool LivelinessAssert::HasAliveWriters(LivelinessQosPolicyKind kind)
{
    std::unique_lock<std::mutex> lock(mutex_);

    for (auto writer : writers_) {
        if (writer.second.status_ == WriterStatus::LIVELINESSALIVE && writer.second.livelinessQos_.kind == kind) {
            return true;
        }
    }
	//AGILOG(AGILOG_ERROR, "not has alive writers");
    return false;
}

void LivelinessAssert::AssertWriterLiveliness(LivelinesStatus& writer)
{
    if (livelinessManager_ != nullptr && writer.status_ != WriterStatus::LIVELINESSALIVE) {

		livelinessManager_->LiveLinessStatusChanged(writer.guid_, localWriter_, writer.livelinessQos_, writer.status_);
    }

    writer.status_ = WriterStatus::LIVELINESSALIVE;
    writer.time_ = steady_clock::now() + nanoseconds(writer.livelinessQos_.lease_duration.ToNs());
}



