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
#ifndef AGIDDS_DCPS_LISTENER_DATAWRITERLISTENER_H
#define AGIDDS_DCPS_LISTENER_DATAWRITERLISTENER_H

#include "dcps/listener/listener.h"
#include "dcps/base/status.h"

AGIDDS_NAMESPACE_BEGIN

class DataWriter;

/**
 * 类DataWriterListener，允许最终用户实现由某些事件触发的回调。
 */
class DataWriterListener : public Listener {
public:
    /**
     * @brief 构造函数。
     */
    DataWriterListener() {}

    /**
     * @brief 析构函数。
     */
    virtual ~DataWriterListener() {}

    /**
     * @brief 当DataWriter的活动性丢失时调用的回调函数。
     *
     * @param [in] writer 指向关联的DataWriter的指针。
     * @param [in] status 活动性变更状态
     */
    virtual void on_liveliness_lost(DataWriter* writer, const LivelinessLostStatus& status)
    {

    }

    /**
     * 错过截止日期时调用的回调函数。 
     *
     * @param [in] writer 指向关联的DataWriter的指针。
     * @param [in] status 到达截止日期未命中的状态。
     */
    virtual void on_offered_deadline_missed(DataWriter* writer, const OfferedDeadlineMissedStatus& status)
    {

    }

    /**
     * 当提供不兼容的QoS时调用的回调函数。
     *
     * @param [in] writer 指向关联的DataWriter的指针。
     * @param [in] status 不兼容的QoS信息。
     */
    virtual void on_offered_incompatible_qos(DataWriter* writer, const OfferedIncompatibleQosStatus& status)
    {

    }

    /**
     * 当DataWriter与端点匹配（或不匹配）时，调用此方法。
     *
     * @param [in] writer 指向关联的DataWriter的指针。
     * @param [in] info   关于匹配的DataReader的信息。
     */
    virtual void on_publication_matched(DataWriter* writer, const PublicationMatchedStatus& status)
    {

    }

};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_LISTENER_DATAWRITERLISTENER_H
