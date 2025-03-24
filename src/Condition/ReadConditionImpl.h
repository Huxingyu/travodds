//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#pragma once
#include "Condition/ReadCondition.h"
#include "Condition/ConditionImpl.h"
#include  "States.h"

AGIDDS_NAMESPACE_BEGIN

class DataReader;
class ReadConditionImpl : public virtual ReadCondition, public ConditionImpl {
public:
	ReadConditionImpl();

	ReadConditionImpl(SampleStateMask smaple_states, ViewStateMask view_states, InstanceStateMask instance_states, DataReader* dataReader);

    ~ReadConditionImpl();

	DataReader* get_datareader() const;

	const SampleStateMask& get_sample_state_mask() const;

	const ViewStateMask& get_view_state_mask() const;

	const InstanceStateMask& get_instance_state_mask() const;
private:
    SampleStateMask sampleStateMask_;
    ViewStateMask viewStateMask_;
    InstanceStateMask instanceStateMask_;
    DataReader* dataReader_ = nullptr;


};

AGIDDS_NAMESPACE_END
