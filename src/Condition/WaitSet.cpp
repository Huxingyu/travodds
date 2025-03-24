#include "Condition/WaitSet.h"
#include "Condition/WaitSetImpl.h"

USING_AGIDDS_NAMESPACE

WaitSet::WaitSet() : impl_(new WaitSetImpl)
{
}
WaitSet::~WaitSet()
{
	if (impl_) {
		delete impl_;
		impl_ = nullptr;
	}
}
ReturnCode_t WaitSet::attach_condition(const Condition& a_condition)
{
	return impl_->attach_condition(a_condition);
}

ReturnCode_t WaitSet::detach_condition(const Condition& a_condition)
{
	return impl_->detach_condition(a_condition);
}

ReturnCode_t WaitSet::wait(ConditionSeq& active_conditions, const Duration_t& timeout)
{
	return impl_->wait(active_conditions,timeout);
}

ReturnCode_t WaitSet::get_conditions(ConditionSeq& attached_conditions) const
{
	return impl_->get_conditions(attached_conditions);
}
