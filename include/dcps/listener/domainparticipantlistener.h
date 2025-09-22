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
#ifndef TRAVODDS_DCPS_LISTENER_DOMAINPARTICIPANTLISTENER_H
#define TRAVODDS_DCPS_LISTENER_DOMAINPARTICIPANTLISTENER_H

#include "dcps/listener/publisherlistener.h"
#include "dcps/listener/subscriberlistener.h"
#include "dcps/listener/topiclistener.h"

TRAVODDS_NAMESPACE_BEGIN
/**
 * @brief 类DomainParticipantListener重写对某些事件的行为。
 *
 */
class DomainParticipantListener : virtual public PublisherListener, virtual public SubscriberListener, virtual public TopicListener {

};

TRAVODDS_NAMESPACE_END
#endif
