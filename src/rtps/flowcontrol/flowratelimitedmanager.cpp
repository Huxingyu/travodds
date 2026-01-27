#include "rtps/flowcontrol/flowratelimitedmanager.h"
#include "rtps/endpoint/participant.h"
#include "common/time/ddstimer.h"
#include "common/timeconversion.h"

#include <algorithm>

USING_TRAVODDS_NAMESPACE;

FlowRateLimitedManager::FlowRateLimitedManager(Participant* participant, const FlowParameters& parameters)
    : participant_(participant), 
      maxBytesPerPeriod_(parameters.maxBytesPerPeriod),
      bytesAvailableThisPeriod_(parameters.maxBytesPerPeriod),
      periodMs_(parameters.periodMs),
      currentTicket_(0),
      nextTicket_(0),
      timer_(nullptr)
{
    if (periodMs_ > 0 && maxBytesPerPeriod_ > 0) {
        timer_ = CreateTimer();
    }
}

FlowRateLimitedManager::~FlowRateLimitedManager()
{
    if (timer_) {
        timer_->Exit();
    }
}

void FlowRateLimitedManager::ResetPeriod(const FlowParameters& parameters)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (parameters.periodMs <= 0 || parameters.maxBytesPerPeriod <= 0) {
        periodMs_ = 0;
        maxBytesPerPeriod_ = 0;
        bytesAvailableThisPeriod_ = 0;
        if (timer_) {
            timer_->Exit();
            timer_.reset();
            cv_.notify_all();
        }
        return; // 停止限流
    }
    if( periodMs_ == parameters.periodMs && maxBytesPerPeriod_ == parameters.maxBytesPerPeriod) {
        return; // 参数没有变化，不需要更新
    } else if (timer_ &&periodMs_ != parameters.periodMs && maxBytesPerPeriod_ == parameters.maxBytesPerPeriod) {
        periodMs_ = parameters.periodMs;
        timer_->UpdatePeriod(periodMs_);
    } else {
        periodMs_ = parameters.periodMs;
        maxBytesPerPeriod_ = parameters.maxBytesPerPeriod;
        bytesAvailableThisPeriod_ = maxBytesPerPeriod_;
        timer_ = CreateTimer();
    }
}

ReturnCode_t FlowRateLimitedManager::MessageSend(std::vector<LocatorVec>& destinationList, char* message, uint32_t length, const Duration_t& timeOut)
{
    if (timer_) {
        uint32_t totallength = destinationList.size() * length;
        std::chrono::nanoseconds timeout =  std::chrono::nanoseconds(timeOut.ToNs());
        int32_t thisTicket = nextTicket_.fetch_add(1);
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待直到轮到自己
        bool waitret = cv_.wait_for(lock,
            timeout,
            [this, thisTicket, totallength] {
                return currentTicket_ == thisTicket && PrepareSend(totallength);
            }
        );

        if (!waitret) {
            if (currentTicket_ == thisTicket) {
                currentTicket_++;
                cv_.notify_all();
            }  else {
                timeoutTicket_.insert(thisTicket);
            }
            return RETCODE_TIMEOUT;
        }

        bytesAvailableThisPeriod_ -= totallength;
        currentTicket_++;
        // 跳过所有连续的超时票号
        while (!timeoutTicket_.empty()) {
            auto it = timeoutTicket_.find(currentTicket_);
            if (it != timeoutTicket_.end()) {
                // 当前票号已超时，跳过并清理
                timeoutTicket_.erase(it);
                currentTicket_++;
            } else {
                break; // 没有连续的超时票号了
            }
        }
        cv_.notify_all();
    }
    participant_->TransmittersSendMessage(destinationList, message, length);
    return RETCODE_OK;
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
        Conversion::MilliSecondsToTime(periodMs_),
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
