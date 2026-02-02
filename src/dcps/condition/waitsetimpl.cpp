#include "dcps/condition/waitsetimpl.h"
#include "dcps/condition/conditionimpl.h"
#include "dcps/condition/guardcondition.h"
#include "dcps/condition/guardconditionimpl.h"
#include "common/log/logger.h"

USING_TRAVODDS_NAMESPACE;

WaitSetImpl::~WaitSetImpl()
{
	for (const auto& it : conditions_) {
		it->LeaveWaitSet(this);
	}
}



ReturnCode_t WaitSetImpl::attach_condition(const Condition& condition)
{
	ConditionImpl* condImpl = nullptr;
	{
		std::lock_guard<std::mutex> lock(mtx_);

		Condition* cond = const_cast<Condition*>(&condition);

		GuardConditionImpl* guard  = IsGuardCondition(cond);
		if (nullptr != guard) {
			condImpl = guard;
		} else if (dynamic_cast<ConditionImpl*>(cond) != nullptr) {
			condImpl = dynamic_cast<ConditionImpl*>(cond);
		} else {
			TRAVODDS_LOG(LOG_ERROR, "Condition type is not supported in WaitSetImpl.");
			return RETCODE_BAD_PARAMETER;
		}
		for (const auto& it : conditions_) {
			if (condImpl == it) {
				return RETCODE_OK;
			}
		}
		conditions_.emplace_back(condImpl);
	}
	condImpl->JoinWaitSet(this);
	if (condition.get_trigger_value()) {
		Notify();
	}
	return RETCODE_OK;
}

ReturnCode_t WaitSetImpl::detach_condition(const Condition& condition)
{
	Condition* cond = const_cast<Condition*>(&condition);
	GuardConditionImpl* guard = IsGuardCondition(cond);
	ConditionImpl* condImpl = nullptr;
	{
		std::lock_guard<std::mutex> lock(mtx_);
		auto itor = conditions_.begin();
		for (; itor != conditions_.end(); itor++) {
			if (&condition == *itor || guard == *itor) {
				condImpl = *itor;
				conditions_.erase(itor);
				break;
			}
		}
	}
	if (nullptr != condImpl) {
		condImpl->LeaveWaitSet(this);
		return RETCODE_OK;
	}
	return RETCODE_PRECONDITION_NOT_MET;
}

ReturnCode_t WaitSetImpl::wait(ConditionSeq& active_conditions, const Duration_t& timeout)
{
	active_conditions.clear();
	active_conditions.reserve(conditions_.size());

	std::unique_lock<std::mutex> lockCv(mtxCv_);
	if (timeout == Duration_t(TIME_INFINITE)) {
		cv_.wait(lockCv, [this, &active_conditions](){return FindTrigger(active_conditions);});
	} else {
		if(!cv_.wait_for(lockCv, std::chrono::nanoseconds(timeout.ToNs()), [this, &active_conditions]() {return FindTrigger(active_conditions); })) {
			return RETCODE_TIMEOUT;
		}
	}
	return RETCODE_OK;
}

ReturnCode_t WaitSetImpl::get_conditions(ConditionSeq& attached_conditions) const
{
	attached_conditions.clear();
	std::lock_guard<std::mutex> lock(mtx_);
	attached_conditions.reserve(conditions_.size());
	for (auto& it : conditions_) {
		Condition* guard = IsGuardConditionImpl(it);
		if (nullptr != guard) {
			attached_conditions.emplace_back(guard);
			continue;
		}
		attached_conditions.emplace_back(it);
	}
	return RETCODE_OK;
}

bool WaitSetImpl::FindTrigger(ConditionSeq& active_conditions)
{
	for (auto& it : conditions_) {
		// 优先确认是否是Notify唤醒
		if (it->IsNotify() || it->get_trigger_value()) {
			Condition* guard = IsGuardConditionImpl(it);
			if (nullptr != guard) {
				active_conditions.emplace_back(guard);
				continue;
			}
			active_conditions.emplace_back(it);
		}
	}
	return active_conditions.size() != 0;
}

void WaitSetImpl::Notify() 
{
	std::lock_guard<std::mutex> guard(mtxCv_);
	cv_.notify_one();
}

GuardConditionImpl* WaitSetImpl::IsGuardCondition(Condition* cond)
{
	if (dynamic_cast<GuardCondition*>(cond) != nullptr) {
		return dynamic_cast<GuardCondition*>(cond)->impl_;
	}
	return nullptr;
}

Condition* WaitSetImpl::IsGuardConditionImpl(ConditionImpl* cond) const
{
	if (nullptr != dynamic_cast<GuardConditionImpl*>(cond)) {
		return  dynamic_cast<GuardConditionImpl*>(cond)->GetGuardCondition();
	}
	return nullptr;
}
