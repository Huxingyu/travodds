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
#ifndef AGIDDS_DCPS_TOPIC_TOPIC_H
#define AGIDDS_DCPS_TOPIC_TOPIC_H

#include "dcps/topic/topicdescription.h"
#include "dcps/domain/domainentity.h"
#include "dcps/listener/topiclistener.h"
#include "qos/qos.h"

AGIDDS_NAMESPACE_BEGIN
/**
 * Topic类，表示发布和订阅都绑定到单个数据类型的实际情况。
 */
    class DDS_DLL Topic : public virtual DomainEntity, public virtual TopicDescription {
    public:
    /**
    * @brief 析构函数。
    */
    virtual ~Topic() = default;

    /**
    * 允许修改主题Qos。
    * 给定的Qos必须由主题支持。
    *
    * @param qos 要为主题设置的新TopicQos值。
    * @return RETCODE_IMMUTABLE_POLICY（如果不允许更改）。
    * RETCODE_INCONSISTENT_POLICY，如果新的qos具有不一致的值。
    * RETCODE_OK（如果更新了qos）。
    */
    virtual ReturnCode_t set_qos(const TopicQos& qos) = 0;

    /**
    * 获取主题Qos。
    *
    * @param qos 返回qos的TopicQos。
    * @return RETCODE_OK
    */
    virtual ReturnCode_t get_qos(TopicQos& qos) const = 0;

    /**
    * 设置TopicListener。
    *
    * @param 监听器TopicListener的新值
    * @param mask 保存监听器响应的状态StatusMask对象。
    * @return RETCODE_OK
    */
    virtual ReturnCode_t set_listener(TopicListener* listener, const StatusMask& mask) = 0;

    /**
    * 获取附加的TopicListener。
    *
    * @return 指向TopicListener的指针。
    */
    virtual const TopicListener* get_listener() const = 0;

    /**
    * 允许应用程序获取的主题的INCONSISTENT_TOPIC_STATUS状态。
    *
    * @param status [out] 要获取的状态的数据结构。
    * @return RETCODE_OK
    */
    virtual ReturnCode_t get_inconsistent_topic_status(InconsistentTopicStatus& status) const = 0;
#ifdef _XML_INTERFACE_CPP

    /**
     * @brief 指定QoS仓库的Qos配置，并设置主题的QoS。
     * 
     * @param library_name QoS库名称，允许为NULL，将转换为空字符串。
     * @param profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param qos_name QoS名称，允许为空，将转换为空字符串。
     * 
     * @return 设置成功返回DDS_RETCODE_OK，否则返回DDS_RETCODE_ERROR。
     */
    virtual ReturnCode_t set_qos_with_profile(
        const char* library_name, 
        const char* profile_name, 
        const char* qos_name) = 0;

#endif // _XML_INTERFACE_CPP
};

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_DCPS_TOPIC_TOPIC_H
