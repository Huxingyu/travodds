#include "dcps/condition/readconditionimpl.h"
#include "dcps/subscribe/datareaderimpl.h"

USING_TRAVODDS_NAMESPACE;

ReadConditionImpl::ReadConditionImpl(SampleStateMask smapleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, DataReaderImpl* dataReader)
    : sampleStateMask_(smapleStates), viewStateMask_(viewStates), instanceStateMask_(instanceStates), dataReader_(dataReader)
{
}

ReadConditionImpl::~ReadConditionImpl()
{
}

bool ReadConditionImpl::get_trigger_value() const
{
    // Conditions are level-triggered. Inspecting one condition must neither
    // consume an event nor hide data from another condition/WaitSet.
    SampleCounters counters = dataReader_->GetSampleCounters(false);
    SampleStateMask smapleStates = (counters.sampleRead ? READ_SAMPLE_STATE : 0) | 
        (counters.sampleNotRead ? NOT_READ_SAMPLE_STATE : 0);
    ViewStateMask viewStates = (counters.viewNew ? NEW_VIEW_STATE : 0) | 
        (counters.viewNotNew ? NOT_NEW_VIEW_STATE : 0);
    InstanceStateMask instanceStates = (counters.instanceAlive ? ALIVE_INSTANCE_STATE : 0) | 
        (counters.instanceDisposed ? NOT_ALIVE_DISPOSED_INSTANCE_STATE : 0) | 
        (counters.instanceNoWriters ? NOT_ALIVE_NO_WRITERS_INSTANCE_STATE : 0);

    return sampleStateMask_ & smapleStates && 
        viewStateMask_ & viewStates &&
        instanceStateMask_ & instanceStates;
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
