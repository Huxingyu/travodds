#include "dcps/condition/guardconditionimpl.h"

USING_AGIDDS_NAMESPACE;

GuardConditionImpl::GuardConditionImpl(GuardCondition* guard)
    : guard_(guard) {
}

ReturnCode_t GuardConditionImpl::set_trigger_value(bool value)
{
	triggerValue_ .store(value);
    return RETCODE_OK;
}

GuardCondition* GuardConditionImpl::GetGuardCondition() const
{
    return guard_;
}
