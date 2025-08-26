#include "dcps/domain/domainparticipantfactoryimpl.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "qos/qospolicycheck.h"
#include "common/log/logger.h"
#include "rtps/transport/asiocontext.h"
#include "common/sys/sysinfo.h"
#include <iostream>
USING_AGIDDS_NAMESPACE

DomainParticipantFactory* DomainParticipantFactory::get_instance()
{
    return DomainParticipantFactoryImpl::get_instance();
}

void DomainParticipantFactory::finalize()
{
	return DomainParticipantFactoryImpl::finalize();
}

DomainParticipantFactoryImpl::DomainParticipantFactoryImpl()
{

}

DomainParticipant* DomainParticipantFactoryImpl::create_participant(DomainId_t domain_id, const DomainParticipantQos& qos, DomainParticipantListener* listener, const StatusMask& mask)
{
    if (domain_id > DOMAIN_ID_MAX) {
		AGILOG(AGILOG_ERROR, "DomainId_t is Err, should 0 <= domain_id <= 232 ");
		return nullptr;
    }
	const DomainParticipantQos& qosT = (&qos == &PARTICIPANT_QOS_DEFAULT) ? defaultParticipantQos_ : qos;
	if (RETCODE_OK != QosPolicyCheck::CheckQos(qosT)) {
		AGILOG(AGILOG_ERROR, "CheckQos Err");
		return nullptr;
	}
	DomainParticipant* domainPart = nullptr;
	std::unique_ptr<DomainParticipantImpl> domainParticipant = std::make_unique<DomainParticipantImpl>(domain_id, qosT, listener, mask);
	if (!domainParticipant.get()) {
		AGILOG(AGILOG_ERROR, "Make DP Err");
		return nullptr;
	}
    domainPart = domainParticipant.get();

	if (qos_.entity_factory.autoenable_created_entities) {
		if (RETCODE_OK != domainPart->enable()) {
			delete_participant(domainPart);
			domainPart = nullptr;
			AGILOG(AGILOG_ERROR, "Enable Err");
			return nullptr;
		}
	}
    {
        std::lock_guard<std::mutex> lock(mtxParticipants_);
        participants_.insert({ domainParticipant->get_instance_handle(), std::move(domainParticipant) });
    }
	return domainPart;
}

DomainParticipantFactoryImpl::~DomainParticipantFactoryImpl()
{
	participants_.clear();
}

ReturnCode_t DomainParticipantFactoryImpl::delete_participant(DomainParticipant*& participant)
{
    if (!participant) {
		AGILOG(AGILOG_ERROR, "RETCODE_BAD_PARAMETER");
        return RETCODE_BAD_PARAMETER;
    }
    InstanceHandle_t handle = participant->get_instance_handle();
    std::lock_guard<std::mutex> lock(mtxParticipants_);
    auto it = participants_.find(handle);
    if (it == participants_.end()) {
		AGILOG(AGILOG_ERROR, "Handle is Err");
        return RETCODE_ERROR;
    }
    if (!it->second->Empty()) {
		AGILOG(AGILOG_ERROR, "DP is nullptr");
        return RETCODE_PRECONDITION_NOT_MET;
    }
    participants_.erase(it);
    participant = nullptr;
    return RETCODE_OK;
}

DomainParticipant* DomainParticipantFactoryImpl::lookup_participant(DomainId_t domain_id) const
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

ReturnCode_t DomainParticipantFactoryImpl::set_default_participant_qos(const DomainParticipantQos& qos)
{
    if (&qos == &PARTICIPANT_QOS_DEFAULT) {
        return RETCODE_OK;
    }
    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "CheckQos Err");
        return ret;
    }
    defaultParticipantQos_ = qos;

    return RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryImpl::get_default_participant_qos(DomainParticipantQos& qos) const
{
    qos = defaultParticipantQos_;
    return RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryImpl::set_qos(const DomainParticipantFactoryQos& qos)
{

    ReturnCode_t ret = QosPolicyCheck::CheckQos(qos);
    if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "CheckQos Err");
        return ret;
    }
    if (!QosPolicyCheck::CanQosBeUpdated(qos_, qos)) {
		AGILOG(AGILOG_ERROR, "CanQosBeUpdated Err");
        return RETCODE_IMMUTABLE_POLICY;
    }
	qos_ = qos;

    return RETCODE_OK;
}

ReturnCode_t DomainParticipantFactoryImpl::get_qos(DomainParticipantFactoryQos& qos) const
{
    qos = qos_;
    return RETCODE_OK;
}

DomainParticipantFactoryImpl* DomainParticipantFactoryImpl::get_instance()
{
	// 确保在获取实例时，获取程序的创建时间戳
	static std::string timestamp = SysInfo::GetExecutionTimestamp();
    /* 优先获取网络资源，决定网络资源最后释放 */
    static AsioContext& as = AsioContext::GetInstance();
    static std::shared_ptr<DomainParticipantFactoryImpl> Impl(new DomainParticipantFactoryImpl);
    return Impl.get();
}

void DomainParticipantFactoryImpl::finalize()
{
    get_instance()->set_qos(DomainParticipantFactoryQos());
    DomainParticipantQos qos;
    get_instance()->set_default_participant_qos(qos);
}
