#include "rtps/flowcontrol/flowratelimitedmanager.h"
#include "rtps/endpoint/participant.h"
#include "common/time/ddstimer.h"
#include "common/timeconversion.h"

#include <algorithm>

USING_AGIDDS_NAMESPACE;

FlowRateLimitedManager::FlowRateLimitedManager(Participant* participant, const FlowParameters& parameters)
    : participant_(participant), 
      maxBytesPerPeriod_(parameters.maxBytesPerPeriod),
      bytesAvailableThisPeriod_(parameters.maxBytesPerPeriod),
      periodMs_(std::chrono::milliseconds(parameters.periodMs)),
      currentTicket_(0),
      nextTicket_(0),
      timer_(nullptr)
{
    if (periodMs_.count() > 0 && maxBytesPerPeriod_ > 0) {
        timer_ = CreateTimer();
    }
}

FlowRateLimitedManager::~FlowRateLimitedManager()
{
    if (timer_) {
        timer_->Exit();
    }
}

void FlowRateLimitedManager::ResetPeriod(uint64_t periodMs)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (periodMs <= 0) {
        if (timer_) {
            timer_->Exit();
            timer_.reset();
        }
        periodMs_ = std::chrono::milliseconds(0);
        bytesAvailableThisPeriod_ = 0;
		return; // 停止限流
    }
    if (periodMs_ == std::chrono::milliseconds(periodMs)) {
		return; // periodMs 没有变化，不需要更新
	}
    periodMs_ = std::chrono::milliseconds(periodMs);
    if(timer_) {
        timer_->UpdatePeriod(periodMs);
    } else if (maxBytesPerPeriod_ > 0) {
        timer_ = CreateTimer();
    }
}

void FlowRateLimitedManager::ResetMaxBytesPerPeriod(int32_t maxBytesPerPeriod)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (maxBytesPerPeriod <= 0) {
        if (timer_) {
            timer_->Exit();
            timer_.reset();
		}
        maxBytesPerPeriod_ = 0;
        bytesAvailableThisPeriod_ = 0;
		return; // 停止限流
    }
    if (maxBytesPerPeriod_ == maxBytesPerPeriod) {
        return; // maxBytesPerPeriod 没有变化，不需要更新
	}
    maxBytesPerPeriod_ = maxBytesPerPeriod;
    if (!timer_ && periodMs_.count() > 0) {
        timer_ = CreateTimer();
	}

}

void FlowRateLimitedManager::MessageSend(LocatorVec& destinationList, char* message, uint32_t length)
{
    if (timer_) {
        int32_t thisTicket = nextTicket_.fetch_add(1);
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待直到轮到自己
        cv_.wait(lock, [this, thisTicket, length] {
            return currentTicket_ == thisTicket && PrepareSend(length);
            });

        bytesAvailableThisPeriod_ -= length;
        currentTicket_++;
        cv_.notify_all();
    }
    participant_->TransmittersSendMessage(destinationList, message, length);
}

bool FlowRateLimitedManager::PrepareSend(int32_t bytesToSend)
{
    if( bytesToSend <= 0 ) {
        return false; // 无效的发送字节数
	}
    if (!timer_) {
		return true; // 如果没有设置限流，则直接允许发送
    }
    // 处理超大消息
    if (bytesToSend > maxBytesPerPeriod_) {
        return bytesAvailableThisPeriod_ == maxBytesPerPeriod_;
    }

    return bytesAvailableThisPeriod_ >= bytesToSend;
}
std::shared_ptr<Timer> FlowRateLimitedManager::CreateTimer()
{
    return TimerManager::CreateTimer(
        Conversion::MilliSecondsToTime(periodMs_.count()),
        [this]() {
            bytesAvailableThisPeriod_ = maxBytesPerPeriod_;
            cv_.notify_all();
        },
        "reset.par.flow");
}
std::chrono::milliseconds FlowRateLimitedManager::GetRemainingTime()
{
    if (timer_) {
        return timer_->GetTriggerTime();
	}
    return std::chrono::milliseconds(0);
}
