#include "rtps/discovery/discovery.h"
#include "dcps/domain/domainparticipant.h"
#include "common/time/ddstimer.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "common/log/logger.h"
#include "common/sys/sysinfo.h"
USING_TRAVODDS_NAMESPACE;

Discovery::Discovery(DomainParticipantImpl* domainParticipant, std::shared_ptr<ThreadPool> threadPool)
	: domainParticipant_(domainParticipant),
	  threadPool_(threadPool),
	  builtinEndpointSet_(0)
{
}

Discovery::~Discovery()
{

}


void Discovery::InitBuiltinEndpoint()
{
	builtinPublisher_ = std::make_shared<PublisherImpl>(PUBLISHER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));
	builtinSubScriber_ = std::make_shared<SubscriberImpl>(SUBSCRIBER_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));

	dcpsParticipant_ = std::make_shared<TopicImpl>("DCPSParticipant", "SPDPdiscoveredParticipantData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));
	dcpsPublication_ = std::make_shared<TopicImpl>("DCPSPublication", "DiscoveredWriterData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));
	dcpsSubscription_ = std::make_shared<TopicImpl>("DCPSSubscription", "DiscoveredReaderData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));
	dcpsParticipantMessage_ = std::make_shared<TopicImpl>("DCPSParticipantMessage", "SPDPdiscoveredParticipantDataMessage", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));
	
#ifdef DDSSECURITY
    /* 安全相关扩展主题 */
    dcpsSubscriptionSecure_ = std::make_shared<TopicImpl>("DCPSSubscriptionSecure", "DiscoveredReaderDataSecure", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));
    dcpsPublicationSecure_ = std::make_shared<TopicImpl>("DCPSPublicationSecure", "DiscoveredWriterDataSecure", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));
    dcpsParticipantMessageSecure_ = std::make_shared<TopicImpl>("DCPSParticipantMessageSecure", "ParticipantDataMessageSecure", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));
    dcpsParticipantStatelessMessage_ = std::make_shared<TopicImpl>("DCPSParticipantStatelessMessageSecure", "ParticipantStatelessMessageSecure", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));
    dcpsParticipantVolatileMessageSecure_ = std::make_shared<TopicImpl>("DCPSParticipantVolatileSecureMessageSecure", "ParticipantVolatileSecureMessageSecure", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));

	dcpsParticipantSecure_ = std::make_shared<TopicImpl>("DCPSParticipantsSecure", "SPDPdiscoveredParticipantData", TOPIC_QOS_DEFAULT, nullptr, STATUS_MASK_NONE, const_cast<DomainParticipantImpl*>(domainParticipant_));
#endif // DDSSECURITY

	//TODO 补充具体TypeSupport
	DataWriterQos writerQoS;
	DataReaderQos readerQos;
	writerQoS.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
	readerQos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
	readerQos.history.depth = 1;
	builtinParticipantDataWriter_ = std::make_shared<DataWriterImpl>(dcpsParticipant_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinParticipantDataReader_ = std::make_shared<DataReaderImpl>(dcpsParticipant_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);

	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER;
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR;

	writerQoS.reliability.kind = RELIABLE_RELIABILITY_QOS;
	writerQoS.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
	writerQoS.history.kind = KEEP_LAST_HISTORY_QOS;
	writerQoS.history.depth = 1;

	readerQos.reliability.kind = RELIABLE_RELIABILITY_QOS;
	readerQos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
	readerQos.history.kind = KEEP_LAST_HISTORY_QOS;
	readerQos.history.depth = 1;
	builtinPublicationsDataWriter_ = std::make_shared<DataWriterImpl>(dcpsPublication_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinPublicationsDataReader_ = std::make_shared<DataReaderImpl>(dcpsPublication_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER;
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR;

#ifdef DDSSECURITY
	/* 安全相关扩展实体创建，绑定对应主题 */
	builtinPublicationsSecureDataWriter_ = std::make_shared<DataWriterImpl>(dcpsPublicationSecure_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinPublicationsSecureDataReader_ = std::make_shared<DataReaderImpl>(dcpsPublicationSecure_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_ANNOUNCER;
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_PUBLICATION_SECURE_DETECTOR;
#endif // DDSSECURITY

	builtinSubscriptionsDataWriter_ = std::make_shared<DataWriterImpl>(dcpsSubscription_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinSubscriptionsDataReader_ = std::make_shared<DataReaderImpl>(dcpsSubscription_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER;
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;

#ifdef DDSSECURITY
	builtinSubscriptionsSecureDataWriter_ = std::make_shared<DataWriterImpl>(dcpsSubscriptionSecure_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinSubscriptionsSecureDataReader_ = std::make_shared<DataReaderImpl>(dcpsSubscriptionSecure_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_ANNOUNCER;
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_SECURE_DETECTOR;
#endif // DDSSECURITY

	/* wangyi 添加 Liveliness 内置节点初始化 */
	builtinParticipantMessageDataWriter_ = std::make_shared<DataWriterImpl>(dcpsParticipantMessage_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinParticipantMessageDataReader_ = std::make_shared<DataReaderImpl>(dcpsParticipantMessage_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);
	builtinEndpointSet_ |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER;
	builtinEndpointSet_ |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER;

#ifdef DDSSECURITY
	builtinParticipantMessageSecureDataWriter_ = std::make_shared<DataWriterImpl>(dcpsParticipantMessageSecure_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinParticipantMessageSecureDataReader_ = std::make_shared<DataReaderImpl>(dcpsParticipantMessageSecure_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);
	builtinEndpointSet_ |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_SECURE_DATA_WRITER;
	builtinEndpointSet_ |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_SECURE_DATA_READER;
	writerQoS.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
	readerQos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
	builtinParticipantStatelessMessageDataWriter_ = std::make_shared<DataWriterImpl>(dcpsParticipantStatelessMessage_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinParticipantStatelessMessageDataReader_ = std::make_shared<DataReaderImpl>(dcpsParticipantStatelessMessage_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);
	
	builtinParticipantStatelessMessageDataWriter_->secPlugin_ = &(GetParticipant()->secPlugin_);
	GUID_t guid = GetParticipant()->get_guid_value();
	guid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER;
	builtinParticipantStatelessMessageDataWriter_->SetInstanceHandle(Conversion::ToInstanceHandle(guid));
	builtinParticipantStatelessMessageDataWriter_->GetSecAttributes();
	
	builtinEndpointSet_ |= BUILTIN_ENDPOINT_PARTICIPANT_STATELESS_MESSAGE_DATA_WRITER;
	builtinEndpointSet_ |= BUILTIN_ENDPOINT_PARTICIPANT_STATELESS_MESSAGE_DATA_READER;
	writerQoS.reliability.kind = RELIABLE_RELIABILITY_QOS;
	readerQos.reliability.kind = RELIABLE_RELIABILITY_QOS;
	builtinParticipantVolatileMessageSecureDataWriter_ = std::make_shared<DataWriterImpl>(dcpsParticipantVolatileMessageSecure_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinParticipantVolatileMessageSecureDataReader_ = std::make_shared<DataReaderImpl>(dcpsParticipantVolatileMessageSecure_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);
	// 内置实体volatile需要获取下安全属性并register_local_datawriter
	builtinParticipantVolatileMessageSecureDataWriter_->secPlugin_ = &(GetParticipant()->secPlugin_);
	guid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER;
	builtinParticipantVolatileMessageSecureDataWriter_->SetInstanceHandle(Conversion::ToInstanceHandle(guid));
	builtinParticipantVolatileMessageSecureDataWriter_->GetSecAttributes(); //register_local_datawriter
	

	builtinEndpointSet_ |= BUILTIN_ENDPOINT_PARTICIPANT_VOLATILE_MESSAGE_SECURE_DATA_WRITER;
	builtinEndpointSet_ |= BUILTIN_ENDPOINT_PARTICIPANT_VOLATILE_MESSAGE_SECURE_DATA_READER;

	builtinParticipantSecureDataWriter_ = std::make_shared<DataWriterImpl>(dcpsParticipantSecure_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinParticipantSecureDataReader_ = std::make_shared<DataReaderImpl>(dcpsParticipantSecure_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_PARTICIPANT_SECURE_ANNOUNCER;
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_PARTICIPANT_SECURE_DETECTOR;
#endif // DDSSECURITY

	/* wangyi LivelinessManager 初始化内置数据读者和写者 */
	domainParticipant_->GetLivelinessManager()->SetParticipantMessageDataWriter(builtinParticipantMessageDataWriter_);
	domainParticipant_->GetLivelinessManager()->SetParticipantMessageDataReader(builtinParticipantMessageDataReader_);

#ifdef DDSSECURITY
	domainParticipant_->GetLivelinessManager()->SetParticipantMessageSecureDataWriter(builtinParticipantMessageSecureDataWriter_);
	domainParticipant_->GetLivelinessManager()->SetParticipantMessageSecureDataReader(builtinParticipantMessageSecureDataReader_);
#endif // DDSSECURITY

	//TODO SPDP SEDP初始化

	spdp_ = std::make_shared<Spdp>(const_cast<DomainParticipantImpl*>(domainParticipant_)->GetRelatedParticipant().get(),this);
	sedp_ = std::make_shared<Sedp>(const_cast<DomainParticipantImpl*>(domainParticipant_)->GetRelatedParticipant().get(), spdp_.get());

	spdp_->SetParticipantDataWriter(builtinParticipantDataWriter_);
	spdp_->SetParticipantDataReader(builtinParticipantDataReader_);

	sedp_->SetPublicationEndpoint(builtinPublicationsDataWriter_.get(), builtinPublicationsDataReader_.get());
	sedp_->SetSubscriptionEndpoint(builtinSubscriptionsDataWriter_.get(), builtinSubscriptionsDataReader_.get());
#ifdef DDSSECURITY
	//bool ret = builtinParticipantDataReader_->GetSecAttributes();
	//if (!ret) {
	//	// TODO 获取安全属性失败 应该释放实体，表示创建失败
	//	TRAVODDS_LOG(LOG_ERROR, "Create DataReader failed: GetSecAttributes failed.");
	//	// 释放资源
	//	return;
	//}
	// 创建RTPS层数据读写者，给读者绑定sedp监听器
	sedp_->SetPublicationSecureEndpoint(builtinPublicationsSecureDataWriter_.get(), builtinPublicationsSecureDataReader_.get());
	sedp_->SetSubscriptionSecureEndpoint(builtinSubscriptionsSecureDataWriter_.get(), builtinSubscriptionsSecureDataReader_.get());
	sedp_->SetParticipantMessageSecureEndpoint(builtinParticipantMessageSecureDataWriter_.get(), builtinParticipantMessageSecureDataReader_.get());
	sedp_->SetParticipantStatelessMessageEndpoint(builtinParticipantStatelessMessageDataWriter_.get(), builtinParticipantStatelessMessageDataReader_.get());
	sedp_->SetParticipantVolatileSecureMessageEndpoint(builtinParticipantVolatileMessageSecureDataWriter_.get(), builtinParticipantVolatileMessageSecureDataReader_.get());

	spdp_->SetParticipantSecureDataWriter(builtinParticipantSecureDataWriter_);
	spdp_->SetParticipantSecureDataReader(builtinParticipantSecureDataReader_);
#endif // DDSSECURITY
}

void Discovery::WriteSpdPdiscoveredParticipantData()
{
	SPDPdiscoveredParticipantData data;
	std::shared_ptr<Participant> participant = domainParticipant_->GetRelatedParticipant();
	DomainParticipantQos dpQos;
	domainParticipant_->get_qos(dpQos);
	data.ddsParticipantData.key = Conversion::ToBuiltinTopicKey(participant->GetGuid());
	data.ddsParticipantData.user_data = dpQos.user_data;
	data.ddsParticipantData.sys_info = SysInfo::GetSysInfoProperties();
	for (auto locator : participant->GetMetatrafficUnicastLocators()) {
		if (LOCATOR_KIND_UDPv4 == locator.kind) {

			std::string hostip = std::to_string(locator.address[12]) + "." + std::to_string(locator.address[13]) + "." +
				std::to_string(locator.address[14]) + "." + std::to_string(locator.address[15]);
			data.ddsParticipantData.sys_info.push_back({ "dds.sys_info.hostip", hostip });
			break;
		}
	}
	data.domainId = domainParticipant_->get_domain_id();
	data.protocolVersion = PROTOCOLVERSION_2_5;
	data.vendorId = VENDORID_PROSIMA;
	data.defaultUnicastLocatorList = participant->GetDefaultUnicastLocators();
	data.defaultMulticastLocatorList = participant->GetDefaultMulticastLocators();
	data.metatrafficUnicastLocatorList = participant->GetMetatrafficUnicastLocators();
	data.metatrafficMulticastLocatorList = participant->GetMetatrafficMulticastLocators();
	data.availableBuiltinEndpoints = builtinEndpointSet_;

#ifdef DDSSECURITY
	//构造自身Participant中安全扩展相关
	if (domainParticipant_->enableDDSSec_) {
		data.ddsParticipantData.identity_token = domainParticipant_->identityToken_;
		data.ddsParticipantData.permissions_token = domainParticipant_->permissionsToken_;
		data.ddsParticipantData.security_info.participant_security_atributes =
			ParticipantSecurityAttributesToMask(&domainParticipant_->secAttributes_); 
		// data.ddsParticipantData.contains_security_data = true;
		data.ddsParticipantData.contains_security_data = data.ddsParticipantData.security_info.participant_security_atributes != 0;
		data.ddsParticipantData.security_info.plugin_participant_security_attributes = domainParticipant_->secAttributes_.plugin_participant_attributes;
	}
	else
	{
		data.ddsParticipantData.contains_security_data = false;
		data.ddsParticipantData.security_info = PARTICIPANT_SECURITY_ATTRIBUTES_INFO_DEFAULT;
	}
	//缓存并序列化保存
	pData_ = data;
	//spdp_->InitParticipantWriter();
#endif // DDSSECURITY

	spdp_->WriteSpdPdiscoveredParticipantData(data);
}

void Discovery::RemoveSpdPdiscoveredParticipantData()
{
    std::shared_ptr<Participant> participant = domainParticipant_->GetRelatedParticipant();
	GUID_t guid = participant->GetGuid();
	spdp_->RemoveSpdPdiscoveredParticipantData(guid);
}

void Discovery::WritePublicationData(DiscoveredWriterData& data)
{
	sedp_->WritePublicationData(data);
}

#ifdef DDSSECURITY
void Discovery::WritePublicationSecureData(DiscoveredWriterSecureData& data)
{
	sedp_->WritePublicationSecureData(data);
}
#endif // DDSSECURITY

void Discovery::WriteSubscriptionData(DiscoveredReaderData& data)
{
	sedp_->WriteSubscriptionData(data);
}

#ifdef DDSSECURITY
void Discovery::WriteSubscriptionSecureData(DiscoveredReaderSecureData& data)
{
	sedp_->WriteSubscriptionSecureData(data);
}

void Discovery::WriteParticipantMessageSecureData(ParticipantMessageSecureData& data) {
	sedp_->WriteParticipantMessageSecureData(data);
}

ReturnCode_t Discovery::WriteParticipantStatelessMessageData(const GUID_t* remoteGuid, ParticipantStatelessMessage& data) {
	return sedp_->WriteParticipantStatelessMessageData(remoteGuid, data);
}

void Discovery::WriteParticipantVolatileSecureMessage(const GUID_t* remoteGuid, ParticipantVolatileSecureMessage& data) {
	sedp_->WriteParticipantVolatileSecureMessage(remoteGuid, data);
}
#endif // DDSSECURITY

void Discovery::RemovePublicationData(InstanceHandle_t& handle)
{
	GUID_t guid = Conversion::ToGuid(handle);
	sedp_->RemovePublicationData(guid);
}

void Discovery::RemoveSubscriptionData(InstanceHandle_t& handle)
{
	GUID_t guid = Conversion::ToGuid(handle);
	sedp_->RemoveSubscriptionData(guid);
}

void Discovery::AddProxy(const SPDPdiscoveredParticipantData& data)
{
	spdp_->AddSpdpProxy(data);

	domainParticipant_->GetLivelinessManager()->AddParticipantMessageProxy(data);

	sedp_->AddSedpProxy(data);
}
#ifdef DDSSECURITY
void Discovery::AddPartProxy(const SPDPdiscoveredParticipantData& data)
{
	spdp_->AddSpdpProxy(data);

	//domainParticipant_->GetLivelinessManager()->AddParticipantMessageProxy(data);
}


#endif
void Discovery::RemoveProxy(GUID_t guid)
{
	sedp_->RemoveSedpProxy(guid);

	domainParticipant_->GetLivelinessManager()->RemoveParticipantMessageProxy(guid);

	spdp_->RemoveSpdpProxy(guid);
}

void Discovery::GetDiscoveredParticipants(InstanceHandleSeq & participant_handles)
{
	spdp_->GetDiscoveredParticipants(participant_handles);
}

int Discovery::GetParticipantData(BuiltinTopicKey_t & key, SPDPdiscoveredParticipantData & data)
{
	return spdp_->GetParticipantData(key, data);
}


std::shared_ptr<ThreadPool> Discovery::GetThreadPool() const
{
	return threadPool_;
}

void Discovery::SetIgnoredParticipants(GUID_t guid)
{
	spdp_->SetIgnoredParticipants(guid);
}

bool Discovery::FindIgnoredParticipants(GUID_t guid)
{
	return spdp_->FindIgnoredParticipants(guid);
}

DomainParticipantImpl* Discovery::GetParticipant()
{
	return domainParticipant_;
}

std::shared_ptr<Spdp> Discovery::GetSpdp()
{
	return spdp_;
}
std::shared_ptr<Sedp> Discovery::GetSedp()
{
	return sedp_;
}

SPDPdiscoveredParticipantData Discovery::GetParticipantData()
{
	return pData_;
}