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
#ifndef TRAVODDS_DCPS_TOPIC_TOPICDESCRIPTION_H
#define TRAVODDS_DCPS_TOPIC_TOPICDESCRIPTION_H

#include "agidds_dll.h"

TRAVODDS_NAMESPACE_BEGIN

class DomainParticipant;
/**
 * @brief TopicDescription类，表示发布和订阅都绑定到单个数据类型的真实情况。
 */
class DDS_DLL TopicDescription {
public:
    /**
     * @brief 析构函数。
     */
    virtual ~TopicDescription() = default;

    /**
     * @brief 获取TopicDescription所属的DomainParticipant。
     *
     * @return TopicDescription所属的DomainParticipant。
     */
    virtual DomainParticipant* get_participant() const = 0;

    /**
     * @brief 获取关联的数据类型名称。
     *
     * @return 数据类型名.
     */
    virtual const char* get_type_name() const = 0;

    /**
     * @brief 获取用于创建此TopicDescription的名称。
     *
     * @return 用于创建此TopicDescription的名称。
     */
    virtual const char* get_name() const = 0;

};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_TOPIC_TOPICDESCRIPTION_H