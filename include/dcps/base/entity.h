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
#ifndef AGIDDS_DCPS_BASE_ENTITY_H
#define AGIDDS_DCPS_BASE_ENTITY_H

#include "dcps/condition/statuscondition.h"

AGIDDS_NAMESPACE_BEGIN

class DDS_DLL Entity {
public:
    virtual ~Entity() = default;
	// virtual ReturnCode_t set_qos(QosPolicy& qos_list) = 0;
	//
	// virtual ReturnCode_t get_qos(QosPolicy& qos_list) = 0;

    // virtual ReturnCode_t set_listener(const Listener* listener, StatusMask mask) = 0;
    //
    // virtual Listener* get_listener() = 0;

    /**
     * @brief 允许访问与实体关联的StatusCondition。
     *
     * @return 引用StatusCondition对象。
     */
    virtual StatusCondition& get_statuscondition() = 0;

    /**
     * @brief 检索实体中触发的状态集
     *
     * 触发状态是指其值已更改的状态自上次应用程序读取状态以来。
     * 当实体首次创建时，或者如果实体未启用，
     * 所有通信状态都处于非触发状态，
     * 因此get_statuschanges函数返回的列表将为空。
     * get_status_changes函数返回的状态列表指实体本身触发的状态
     * 并且不包括适用于所包含实体的状态。
     *
     * @return 对触发状态设置为1的StatusMask的常量引用。
     */
    virtual const StatusMask& get_status_change() const = 0;

    /**
     * @brief 此函数用于启用实体
     *
     * @return RETCODE_OK。
     */
    virtual ReturnCode_t enable() = 0 ;

    /**
     * @brief 检索表示实体的实例处理程序。
     *
     * @return 引用的实例句柄。
     */
    virtual const InstanceHandle_t& get_instance_handle() const = 0;

    // add for language wrapper, store user-defined data in the entit)

    virtual void set_object_data(void* data) = 0;

    virtual void* get_object_data() = 0;
};

AGIDDS_NAMESPACE_END
#endif // ! AGIDDS_DCPS_BASE_ENTITY_H
