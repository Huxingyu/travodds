#include "dcps/condition/conditionimpl.h"
#include "dcps/condition/waitsetimpl.h"
USING_TRAVODDS_NAMESPACE;

ConditionImpl::ConditionImpl()
	: triggerValue_(false), notify_(false)
{
}

ConditionImpl::~ConditionImpl()
{
	for (const auto& it : waitSets_) {
		it->detach_condition(*this);
	}
}

bool ConditionImpl::get_trigger_value() const
{
	return triggerValue_.load();
}

void ConditionImpl::Notify()
{
	std::lock_guard<std::mutex> lock(mtx_);
	if (waitSets_.empty()) {
		return;
	}
	for (const auto& it : waitSets_) {
		it->Notify();
	}
	notify_.store(true);
}

bool ConditionImpl::IsNotify()
{
	return notify_.exchange(false);
}

void ConditionImpl::JoinWaitSet(WaitSetImpl* waitImpl)
{
	std::lock_guard<std::mutex> lock(mtx_);
	for (const auto& it : waitSets_) {
		if (waitImpl == it) {
			return;
		}
	}
	waitSets_.emplace_back(waitImpl);
}

void ConditionImpl::LeaveWaitSet(WaitSetImpl* waitImpl)
{
	std::lock_guard<std::mutex> lock(mtx_);
	auto itor = waitSets_.begin();
	for ( ; itor != waitSets_.end(); itor++) {
		if (waitImpl == *itor) {
			waitSets_.erase(itor);
			return;
		}
	}
}
