#include "WaitSetImpl.h"
USING_AGIDDS_NAMESPACE
ReturnCode_t WaitSetImpl::attach_condition(const Condition& a_condition)
{
	return RETCODE_UNSUPPORTED;
}

ReturnCode_t WaitSetImpl::detach_condition(const Condition& a_condition)
{
	return RETCODE_UNSUPPORTED;
}

ReturnCode_t WaitSetImpl::wait(ConditionSeq& active_conditions, const Duration_t& timeout)
{
	return RETCODE_UNSUPPORTED;
}

ReturnCode_t WaitSetImpl::get_conditions(ConditionSeq& attached_conditions) const
{
	return RETCODE_UNSUPPORTED;
}
