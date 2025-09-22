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
#ifndef TRAVODDS_DCPS_LISTENER_TOPICLISTENER_H
#define TRAVODDS_DCPS_LISTENER_TOPICLISTENER_H

#include "dcps/listener/listener.h"
#include "dcps/base/status.h"

TRAVODDS_NAMESPACE_BEGIN

class Topic;
/**
 * @brief 类TopicListener，最终用户应该使用它来实现对某些操作的特定回调。
 *
 */
class TopicListener : public Listener {
public:
    /**
     * @brief 由用户实现的虚拟功能，包含当存在具有相同名称但不同配置的另一个主题时要执行的操作。
     *
     * @param topic Topic
     * @param status 主题配置不一致信息。
     */
    virtual void on_inconsistent_topic(Topic* topic, const InconsistentTopicStatus& status)
    {
	    
    }
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_LISTENER_TOPICLISTENER_H