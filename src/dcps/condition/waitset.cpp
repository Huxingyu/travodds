#include "dcps/condition/waitset.h"
#include "dcps/condition/waitsetimpl.h"

USING_TRAVODDS_NAMESPACE;

WaitSet::WaitSet()
	: impl_(new WaitSetImpl)
{
}
WaitSet::~WaitSet()
{
}
ReturnCode_t WaitSet::attach_condition(const Condition& condition)
{
	return impl_->attach_condition(condition);
}

ReturnCode_t WaitSet::detach_condition(const Condition& condition)
{
	return impl_->detach_condition(condition);
}

ReturnCode_t WaitSet::wait(ConditionSeq& active_conditions, const Duration_t& timeout)
{
	return impl_->wait(active_conditions, timeout);
}

ReturnCode_t WaitSet::get_conditions(ConditionSeq& attached_conditions) const
{
	return impl_->get_conditions(attached_conditions);
}
