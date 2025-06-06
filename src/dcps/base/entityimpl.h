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
#ifndef AGIDDS_DCPS_BASE_ENTITYIMPL_H
#define AGIDDS_DCPS_BASE_ENTITYIMPL_H

#include "dcps/base/entity.h"
#include "dcps/condition/statusconditionimpl.h"

AGIDDS_NAMESPACE_BEGIN
class EntityImpl : public virtual Entity {
public:
	EntityImpl(const StatusMask& mask);

    // ReturnCode_t set_qos(QosPolicy& qos) ;
    //
    // ReturnCode_t get_qos(QosPolicy& qos) ;

    virtual StatusCondition* get_statuscondition() const;

    virtual const StatusMask& get_status_change() const;

    virtual ReturnCode_t enable();

    virtual const InstanceHandle_t& get_instance_handle() const;

    // add for language wrapper, store user-defined data in the entit)
    virtual void set_object_data(void* data);

    virtual void* get_object_data();

public:
	bool IsEnable() const;

    const StatusMask& get_status_mask() const;

	void SetInstanceHandle(const InstanceHandle_t& handle);

protected:
	bool enable_;

    StatusMask statusMask_;

    StatusConditionImpl statusCondition_;

	InstanceHandle_t handle_;

	void* objectData_;
};
AGIDDS_NAMESPACE_END

#endif // !AGIDDS_DCPS_BASE_ENTITYIMPL_H
