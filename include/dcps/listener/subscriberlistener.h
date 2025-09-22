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
#ifndef TRAVODDS_DCPS_LISTENER_SUBSCRIBERLISTENER_H
#define TRAVODDS_DCPS_LISTENER_SUBSCRIBERLISTENER_H

#include "dcps/listener/datareaderlistener.h"

TRAVODDS_NAMESPACE_BEGIN

class Subscriber;
/**
 * @brief 类SubscriberListener，最终用户应该使用它来实现对某些操作的特定回调。
 * 它还继承所有DataReaderListener回调。
 *
 */
class SubscriberListener : public DataReaderListener {
public:
    /**
     * @brief 由用户实现的虚拟功能，包含当新数据消息在任何读取器上可用时要执行的操作。
     *
     * @param subscriber Subscriber
     */
    virtual void on_data_on_readers(Subscriber* subscriber)
    {
	    
    }
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_LISTENER_SUBSCRIBERLISTENER_H
