#include "DDS/DomainParticipantFactoryImpl.h"
#include "DDS/DomainParticipantImpl.h"
#include "QoS/QosPolicyCheck.h"
USING_AGIDDS_NAMESPACE

DomainParticipantFactory* DomainParticipantFactory::get_instance()
{
    return DomainParticipantFactoryImpl::get_instance();
}

DomainParticipantFactoryImpl::DomainParticipantFactoryImpl()
{

}

DomainParticipant *DomainParticipantFactoryImpl::create_participant(DomainId_t domain_id, const DomainParticipantQos& qos, DomainParticipantListener* a_listener, const StatusMask& mask)
{
	const DomainParticipantQos& qosT = (&qos == &PARTICIPANT_QOS_DEFAULT) ? defaultParticipantQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		return nullptr;
	}
	DomainParticipant* domainPart = nullptr;
	std::unique_ptr<DomainParticipantImpl> domainParticipant = std::make_unique<DomainParticipantImpl>(domain_id, qosT, a_listener, mask);
	if (!domainParticipant.get()) {
		return nullptr;
	}
    domainPart = domainParticipant.get();
	{
		std::lock_guard<std::mutex> lock(mtxParticipants_);
        participants_.insert({ domainParticipant->get_instance_handle(), std::move(domainParticipant) });
	}
	if (qos_.entity_factory.autoenable_created_entities) {
		if (RETCODE_OK != domainPart->enable()) {
			delete_participant(domainPart);
			domainPart = nullptr;
			return nullptr;
		}
	}
	return domainPart;
}

DomainParticipantFactoryImpl::~DomainParticipantFactoryImpl()
{
	participants_.clear();
}

ReturnCode_t DomainParticipantFactoryImpl::delete_participant(DomainParticipant* &a_participant)
{
    if (!a_participant) {
        return RETCODE_BAD_PARAMETER;
    }
    InstanceHandle_t handle = a_participant->get_instance_handle();
    std::lock_guard<std::mutex> lock(mtxParticipants_);
    auto it = participants_.find(handle);
    if (it == participants_.end()) {
        return RETCODE_ERROR;
    }
    if (!it->second->Empty()) {
        return RETCODE_PRECONDITION_NOT_MET;
    }
    participants_.erase(handle);
    a_participant = nullptr;
    return RETCODE_OK;
}

DomainParticipant *DomainParticipantFactoryImpl::lookup_participant(DomainId_t domain_id) const
{
    DomainParticipant* dp = nullptr;
    for (auto& itor : participants_) {
        if (itor.second->get_domain_id() == domain_id) {
            dp = itor.second.get();
            break;
        }
    }
    return dp;
}

ReturnCode_t DomainParticipantFactoryImpl::set_default_participant_qos(const DomainParticipantQos &qos)
{
    if (&qos == &PARTICIPANT_QOS_DEFAULT) {
        return RETCODE_OK;
    }
    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
        return ret;
    }
    defaultParticipantQos_ = qos;

    return RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryImpl::get_default_participant_qos(DomainParticipantQos &qos) const
{
    qos = defaultParticipantQos_;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryImpl::set_qos(const DomainParticipantFactoryQos &qos)
{

    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
        return ret;
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qos)) {
        return RETCODE_IMMUTABLE_POLICY;
    }
	qos_ = qos;

    return RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryImpl::get_qos(DomainParticipantFactoryQos &qos) const
{
    qos = qos_;
    return RETCODE_OK;
}

DomainParticipantFactoryImpl* DomainParticipantFactoryImpl::get_instance()
{
    static DomainParticipantFactoryImpl Impl;
    return &Impl;
}
