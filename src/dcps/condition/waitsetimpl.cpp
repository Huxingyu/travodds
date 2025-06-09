#include "dcps/condition/waitsetimpl.h"
#include "dcps/condition/conditionimpl.h"
#include "common/log/logger.h"

USING_AGIDDS_NAMESPACE;

WaitSetImpl::~WaitSetImpl()
{
	for (const auto& it : conditions_) {
		it->LeaveWaitSet(this);
	}
}



ReturnCode_t WaitSetImpl::attach_condition(const Condition& condition)
{
	{
		std::lock_guard<std::mutex> lock(mtx_);
		for (const auto& it : conditions_) {
			if (&condition == it) {
				return RETCODE_OK;
			}
		}
		conditions_.emplace_back(dynamic_cast<ConditionImpl*>(const_cast<Condition*>(&condition)));
	}
	dynamic_cast<ConditionImpl*>(const_cast<Condition*>(&condition))->JoinWaitSet(this);
	if (condition.get_trigger_value()) {
		Notify();
	}
	return RETCODE_OK;
}

ReturnCode_t WaitSetImpl::detach_condition(const Condition& condition)
{
	bool ret = false;
	{
		std::lock_guard<std::mutex> lock(mtx_);
		auto itor = conditions_.begin();
		for (; itor != conditions_.end(); itor++) {
			if (&condition == *itor) {
				conditions_.erase(itor);
				ret = true;
				break;
			}
		}
	}
	if (ret) {
		dynamic_cast<ConditionImpl*>(const_cast<Condition*>(&condition))->LeaveWaitSet(this);
	}
	return ret ? RETCODE_OK : RETCODE_PRECONDITION_NOT_MET;
}

ReturnCode_t WaitSetImpl::wait(ConditionSeq& active_conditions, const Duration_t& timeout)
{
	active_conditions.clear();
	active_conditions.reserve(conditions_.size());
	if (FindTrigger(active_conditions) != 0) {
		return RETCODE_OK;
	}
	std::unique_lock<std::mutex> lockCv(mtxCv_);
	if (timeout == Duration_t(TIME_INFINITE)) {
		cv_.wait(lockCv);
	} else {
		std::cv_status ret = cv_.wait_for(lockCv, std::chrono::nanoseconds(timeout.ToNs()));
		if (ret == std::cv_status::timeout) {
			return RETCODE_TIMEOUT;
		}
	}
	lockCv.unlock();
	if (FindTrigger(active_conditions) != 0) {
		return RETCODE_OK;
	}
	return RETCODE_NO_DATA;
}

ReturnCode_t WaitSetImpl::get_conditions(ConditionSeq& attached_conditions) const
{
	attached_conditions.clear();
	std::lock_guard<std::mutex> lock(mtx_);
	attached_conditions.reserve(conditions_.size());
	for (auto& it : conditions_) {
		attached_conditions.emplace_back(it);
	}
	return RETCODE_OK;
}

uint32_t WaitSetImpl::FindTrigger(ConditionSeq& active_conditions)
{
	std::lock_guard<std::mutex> lock(mtx_);
	for (auto& it : conditions_) {
		if (it->get_trigger_value() || it->IsNotify()) {
			active_conditions.emplace_back(it);
		}
	}
	return active_conditions.size();
}

void WaitSetImpl::Notify() 
{
	std::lock_guard<std::mutex> guard(mtxCv_);
	cv_.notify_one();
}