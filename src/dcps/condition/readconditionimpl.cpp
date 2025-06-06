#include "dcps/condition/readconditionimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
USING_AGIDDS_NAMESPACE;

ReadConditionImpl::ReadConditionImpl(SampleStateMask smapleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, DataReader* dataReader)
    : sampleStateMask_(smapleStates), viewStateMask_(viewStates), instanceStateMask_(instanceStates), dataReader_(dataReader)
{
}

ReadConditionImpl::~ReadConditionImpl()
{
}

DataReader* ReadConditionImpl::get_datareader() const
{
    return dataReader_;
}

const SampleStateMask& ReadConditionImpl::get_sample_state_mask() const
{  
    return sampleStateMask_;
}

const ViewStateMask& ReadConditionImpl::get_view_state_mask() const
{  
    return viewStateMask_;
}

const InstanceStateMask& ReadConditionImpl::get_instance_state_mask() const
{ 
    return instanceStateMask_;
}
