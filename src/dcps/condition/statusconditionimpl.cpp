#include "dcps/condition/statusconditionimpl.h"
#include "dcps/base/entityimpl.h"

USING_TRAVODDS_NAMESPACE;
StatusConditionImpl::StatusConditionImpl(Entity* parent)
    : ConditionImpl(), 
      entity_(parent), 
      status_(STATUS_MASK_NONE),
      enabledMask_(dynamic_cast<EntityImpl*>(parent)->get_status_mask())
{
}

bool StatusConditionImpl::get_trigger_value() const
{
    std::lock_guard<std::mutex> guard(mtx_);
    return (enabledMask_ & status_) != 0;
}

ReturnCode_t StatusConditionImpl::set_enabled_statuses(const StatusMask& mask)
{
    std::lock_guard<std::mutex> guard(mtx_);
    bool oldTrigger = (enabledMask_ & status_) != 0;
    enabledMask_ = mask;
    bool trigger = (enabledMask_ & status_) != 0;
    if (!oldTrigger && trigger) {
        Notify();
    }
    return RETCODE_OK;
}

const StatusMask& StatusConditionImpl::get_enabled_statuses() const
{
    std::lock_guard<std::mutex> guard(mtx_);
    return enabledMask_;
}

const StatusMask& StatusConditionImpl::get_status() const
{
    std::lock_guard<std::mutex> guard(mtx_);
    return status_;
}

void StatusConditionImpl::set_status(const StatusMask& status, bool addFlag)
{
    if (addFlag) {
        std::lock_guard<std::mutex> guard(mtx_);
        bool oldTrigger = (enabledMask_ & status_) != 0;
        status_ |= status;
        bool trigger = (enabledMask_ & status_) != 0;
        if (!oldTrigger && trigger) {
            Notify();
        }
    } else {
        std::lock_guard<std::mutex> guard(mtx_);
        status_ &= ~status;
    }
}

Entity* StatusConditionImpl::get_entity() const
{
    return entity_;
}
