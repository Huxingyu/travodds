#include "Condition/StatusConditionImpl.h"

USING_AGIDDS_NAMESPACE;
StatusConditionImpl::StatusConditionImpl(Entity* parent)
    : ConditionImpl(), entity_(parent)
{
}
ReturnCode_t StatusConditionImpl::set_enabled_statuses(const StatusMask& mask)
{
    return RETCODE_UNSUPPORTED;
}

const StatusMask& StatusConditionImpl::get_enabled_statuses() const
{
   return StatusMask(0);
}

const StatusMask& StatusConditionImpl::get_status() const
{
    return status_;
}

Entity * StatusConditionImpl::get_entity() const
{
    return entity_;
}
