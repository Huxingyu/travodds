#include "TopicImpl.h"
#include "DDS/DomainParticipantImpl.h"
#include "QoS/QosPolicyCheck.h"
USING_AGIDDS_NAMESPACE;
TopicImpl::TopicImpl()
    : DomainEntityImpl(0)
{

}

TopicImpl::~TopicImpl()
{
    if (listener_) {
        delete listener_;
        listener_ = nullptr;
    }
    
}

TopicImpl::TopicImpl(const string& name, const string& type_name, const TopicQos& qos, TopicListener* a_listener, StatusMask mask, DomainParticipant* a_participant)
    : DomainEntityImpl(mask), qos_(qos), listener_(a_listener)
{
    name_ = name;
    typeName_ = type_name;
    participant_ = dynamic_cast<DomainParticipantImpl*>(a_participant);
}


ReturnCode_t TopicImpl::set_qos(const TopicQos &qos)
{
	TopicQos defaultQos;
	participant_->get_default_topic_qos(defaultQos);
    const TopicQos& qosTemp = (&qos == &TOPIC_QOS_DEFAULT) ? defaultQos : qos;

    if (&qos != &TOPIC_QOS_DEFAULT) {
        ReturnCode_t ret = QosPolicyCheck::CheckQos(qosTemp);
        if (RETCODE_OK != ret) {
            return ret;
        }
    }

    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qosTemp)) {
        return RETCODE_IMMUTABLE_POLICY;
    }

    qos_ = qosTemp;

    return RETCODE_OK;
}

ReturnCode_t TopicImpl::get_qos(TopicQos &qos) const
{
	qos = qos_;
    return RETCODE_OK;

}

ReturnCode_t TopicImpl::set_listener(TopicListener* a_listener, const StatusMask& mask)
{
    if (a_listener) {
        listener_ = a_listener;
        statusMask_ = mask;
        return RETCODE_OK;
    }
    return RETCODE_BAD_PARAMETER;
}

const TopicListener* TopicImpl::get_listener() const
{
    return listener_;
}

ReturnCode_t TopicImpl::get_inconsistent_topic_status(InconsistentTopicStatus& a_status) const
{
    return RETCODE_UNSUPPORTED;
}

ReturnCode_t TopicImpl::enable()
{
	if (enable_) {
		return RETCODE_OK;
	}
	if (!participant_->IsEnable()) {
		return RETCODE_PRECONDITION_NOT_MET;
	}
	enable_ = true;
	return RETCODE_OK;
}
