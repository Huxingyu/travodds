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
#ifndef AGIDDS_DCPS_TOPIC_TOPICIMPL_H
#define AGIDDS_DCPS_TOPIC_TOPICIMPL_H

#include "dcps/topic/topic.h"
#include "dcps/topic/topicdescriptionimpl.h"
#include "dcps/domain/domainentityimpl.h"
#include "dcps/base/statusmanager.h"

AGIDDS_NAMESPACE_BEGIN
class DomainParticipantImpl;
class TopicImpl : public virtual Topic, public DomainEntityImpl, public TopicDescriptionImpl {
public:
    TopicImpl(const std::string& name, const std::string& type_name, const TopicQos& qos, TopicListener* listener, StatusMask mask, DomainParticipant* participant);
    
    virtual ~TopicImpl();

    ReturnCode_t set_qos(const TopicQos& qos);

    ReturnCode_t get_qos(TopicQos& qos) const;

    ReturnCode_t set_listener(TopicListener* listener, const StatusMask& mask);

    const TopicListener* get_listener() const;

    ReturnCode_t get_inconsistent_topic_status(InconsistentTopicStatus& status) const;

	void update_inconsistent_topic_status();

#ifdef _XML_INTERFACE_CPP

    virtual ReturnCode_t set_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

#endif // _XML_INTERFACE_CPP
public:
	ReturnCode_t enable();

private:
    TopicQos qos_;
    TopicListener* listener_ = nullptr;
	std::unique_ptr<TopicStatusManager> statusMgr_;
};
AGIDDS_NAMESPACE_END    
#endif // !AGIDDS_DCPS_TOPIC_TOPICIMPL_H
