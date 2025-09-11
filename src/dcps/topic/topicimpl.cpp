#include "dcps/topic/topicimpl.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "qos/qospolicycheck.h"
#include "rtps/endpoint/participant.h"
#include "common/log/logger.h"
USING_TRAVODDS_NAMESPACE;


TopicImpl::TopicImpl(const std::string& name, const std::string& type_name, const TopicQos& qos, TopicListener* listener, StatusMask mask, DomainParticipant* participant)
    : DomainEntityImpl(mask), 
      TopicDescriptionImpl(name, type_name, dynamic_cast<DomainParticipantImpl*>(participant)), 
      qos_(qos), 
      listener_(listener),
      statusMgr_(new TopicStatusManager(this))
{
    GUID_t guid = participant_->GetRelatedParticipant()->GenerateGuid(ENTITYID_UNKNOWN, false, ENTITY_TOPIC);
    handle_ = Conversion::ToInstanceHandle(guid);
}

TopicImpl::~TopicImpl()
{
    if (listener_) {
        //delete listener_;
        listener_ = nullptr;
    }

}

ReturnCode_t TopicImpl::set_qos(const TopicQos& qos)
{
	TopicQos defaultQos;
	participant_->get_default_topic_qos(defaultQos);
    const TopicQos& qosTemp = (&qos == &TOPIC_QOS_DEFAULT) ? defaultQos : qos;

    if (&qos != &TOPIC_QOS_DEFAULT) {
        ReturnCode_t ret = QosPolicyCheck::CheckQos(qosTemp);
        if (RETCODE_OK != ret) {
			TRAVODDS_LOG(LOG_ERROR, "CheckQos Err");
            return ret;
        }
    }

    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qosTemp)) {
		TRAVODDS_LOG(LOG_ERROR, "CanQosBeUpdated Err");
        return RETCODE_IMMUTABLE_POLICY;
    }

    qos_ = qosTemp;

    return RETCODE_OK;
}

ReturnCode_t TopicImpl::get_qos(TopicQos& qos) const
{
	qos = qos_;
    return RETCODE_OK;

}

ReturnCode_t TopicImpl::set_listener(TopicListener* listener, const StatusMask& mask)
{
	//TODO 线程安全 //
    if (listener_) {
		//delete listener_;
		listener_ = nullptr;
    }
	listener_ = listener;
	statusMask_ = mask;
	return RETCODE_OK;
}

const TopicListener* TopicImpl::get_listener() const
{
    return listener_;
}

ReturnCode_t TopicImpl::get_inconsistent_topic_status(InconsistentTopicStatus& status) const
{
	statusMgr_->GetInconsistentTopicStatus(status);
    return RETCODE_OK;
}

void TopicImpl::update_inconsistent_topic_status()
{
	statusMgr_->UpdateInconsistentTopicStatus();
}

ReturnCode_t TopicImpl::enable()
{
	if (enable_) {
		return RETCODE_OK;
	}
	if (!participant_->IsEnable()) {
		TRAVODDS_LOG(LOG_ERROR, "IsEnable Err");
		return RETCODE_PRECONDITION_NOT_MET;
	}
	enable_ = true;
	return RETCODE_OK;
}
