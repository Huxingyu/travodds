#include "Condition/GuardConditionImpl.h"
USING_AGIDDS_NAMESPACE

ReturnCode_t GuardConditionImpl::set_trigger_value(bool value)
{
	triggerValue_ = value;
    return RETCODE_OK;
}
