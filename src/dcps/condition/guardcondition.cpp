#include "dcps/condition/guardcondition.h"
#include "dcps/condition/guardconditionimpl.h"
AGIDDS_NAMESPACE_BEGIN;

GuardCondition::GuardCondition()
	: impl_(new GuardConditionImpl(this))
{
}

GuardCondition::~GuardCondition()
{
	if (impl_) {
		delete impl_;
		impl_ = nullptr;
	}
}

bool GuardCondition::get_trigger_value() const
{
	return impl_->get_trigger_value();
}

ReturnCode_t GuardCondition::set_trigger_value(bool value)
{
	return impl_->set_trigger_value(value);
}
AGIDDS_NAMESPACE_END
