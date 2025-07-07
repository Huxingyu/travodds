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
#ifndef AGIDDS_DCPS_LISTENER_DATAREADERLISTENER_H
#define AGIDDS_DCPS_LISTENER_DATAREADERLISTENER_H

#include "dcps/listener/listener.h"
#include "dcps/base/status.h"

AGIDDS_NAMESPACE_BEGIN

class DataReader;
class DataWriter;

/**
 * @brief DataReaderListener类, 最终用户应该使用它来实现对某些函数的特定回调。
 */
class DDS_DLL DataReaderListener : public Listener {
public:

    /**
     * 由用户实现的虚拟功能，包含接收新数据消息时要执行的函数。
     *
     * @param reader 数据读者。
     */
    virtual void on_data_available(DataReader* reader)
    {
    }


    /**
     * @brief 拒绝样本时调用的方法。
     *
     * @param reader 数据读者。
     * @param status 被拒绝的状态。
     */
    virtual void on_sample_rejected(DataReader* reader, const SampleRejectedStatus& status)
    {

    }


    /**
     * @brief 当与订户关联的活动状态更改时调用的方法。
     *
     * @param reader 数据读者。
     * @param status liveliness 状态更改。
     */
    virtual void on_liveliness_changed(DataReader* reader, const LivelinessChangedStatus& status)
    {

    }

    /**
     * 主题错过截止期限时要调用的虚拟方法。
     *
     * @param reader 数据读者。
     * @param status 请求的截止日期丢失状态。
     */
    virtual void on_requested_deadline_missed(DataReader* reader, const RequestedDeadlineMissedStatus& status)
    {

    }

    /**
     * @brief 请求不兼容QoS的方法。
     *
     * @param reader 数据读者。
     * @param status 请求的不兼容QoS状态。
     */
    virtual void on_requested_incompatible_qos(DataReader* reader, const RequestedIncompatibleQosStatus& status)
    {

    }

    /**
     *当订户与新的Writer匹配（或不匹配）时要调用的虚拟方法；即，当发现在相同主题中发布的作者时。
     *
     * @param reader 数据读者。
     * @param status 订阅匹配状态。
     */
    virtual void on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& status)
    {

    }

    /**
     * @brief 样本丢失时调用的方法。
     *
     * @param reader 数据读者。
     * @param status 样本丢失状态。
     */
    virtual void on_sample_lost(DataReader* reader, const SampleLostStatus& status)
    {

    }
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_LISTENER_DATAREADERLISTENER_H