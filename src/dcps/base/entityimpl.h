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
#ifndef TRAVODDS_DCPS_BASE_ENTITYIMPL_H
#define TRAVODDS_DCPS_BASE_ENTITYIMPL_H

#include "dcps/base/entity.h"
#include "dcps/condition/statusconditionimpl.h"

TRAVODDS_NAMESPACE_BEGIN
/**
 * @brief EntityImpl类是Entity子类，并具体实现Entity类功能。
 * 支持QoS策略、监听器和状态条件的所有对象的抽象基类。
 */
class EntityImpl : public virtual Entity {
public:

    /**
     * @brief 构造函数。
     */
	EntityImpl(const StatusMask& mask);

    // ReturnCode_t set_qos(QosPolicy& qos) ;
    //
    // ReturnCode_t get_qos(QosPolicy& qos) ;

    /**
     * @brief 允许访问与实体关联的StatusCondition。
     *
     * @return 引用StatusCondition对象。
     */
    virtual StatusCondition& get_statuscondition();

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
    virtual const StatusMask& get_status_change() const;

    /**
     * @brief 此函数用于启用实体
     *
     * @return RETCODE_OK。
     */
    virtual ReturnCode_t enable();

    /**
     * @brief 检索表示实体的实例处理程序。
     *
     * @return 引用的实例句柄。
     */
    virtual const InstanceHandle_t& get_instance_handle() const;

    virtual const GUID_t& get_guid() const;

    // add for language wrapper, store user-defined data in the entit)
    /**
     * @brief 设置objectData_变量值为data。
     *
     * @param [in] data  新的language wrapper相关用户数据。
     */
    virtual void set_object_data(void* data);

    /**
     * @brief 获取objectData_变量值。
     *
     * @return 新的language wrapper相关用户数据。
     */
    virtual void* get_object_data();

public:
    /**
     * @brief 获取实体是否已启动。
     *
     * @return 返回实体启动标志enable_值。
     */
	bool IsEnable() const;

    /**
     * @brief 获取状态掩码。
     *
     * @return 状态掩码信息。
     */
    const StatusMask& get_status_mask() const;

    /**
     * @brief 设置实例句柄。
     *
     * @param [in] handle  新的实例句柄。
     */
	void SetInstanceHandle(const InstanceHandle_t& handle);

protected:
	bool enable_;

    StatusMask statusMask_;

    StatusConditionImpl statusCondition_;

	InstanceHandle_t handle_;

	void* objectData_;
};
TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_DCPS_BASE_ENTITYIMPL_H
