#include "rtps/discovery/discovery.h"
#include "dcps/domain/domainparticipant.h"
#include "common/time/ddstimer.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "common/log/logger.h"
USING_AGIDDS_NAMESPACE;

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

	builtinSubscriptionsDataWriter_ = std::make_shared<DataWriterImpl>(dcpsSubscription_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinSubscriptionsDataReader_ = std::make_shared<DataReaderImpl>(dcpsSubscription_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);

	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER;
	builtinEndpointSet_ |= DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;

	/* wangyi 添加 Liveliness 内置节点初始化 */
	builtinParticipantMessageDataWriter_ = std::make_shared<DataWriterImpl>(dcpsParticipantMessage_.get(), writerQoS, nullptr, STATUS_MASK_NONE, builtinPublisher_.get(), nullptr);
	builtinParticipantMessageDataReader_ = std::make_shared<DataReaderImpl>(dcpsParticipantMessage_.get(), readerQos, nullptr, STATUS_MASK_NONE, builtinSubScriber_.get(), nullptr);

	builtinEndpointSet_ |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER;
	builtinEndpointSet_ |= BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER;

	/* wangyi LivelinessManager 初始化内置数据读者和写者 */
	domainParticipant_->GetLivelinessManager()->SetParticipantMessageDataWriter(builtinParticipantMessageDataWriter_);
	domainParticipant_->GetLivelinessManager()->SetParticipantMessageDataReader(builtinParticipantMessageDataReader_);

	//TODO SPDP SEDP初始化

	spdp_ = std::make_shared<Spdp>(const_cast<DomainParticipantImpl*>(domainParticipant_)->GetRelatedParticipant().get(),this);
	sedp_ = std::make_shared<Sedp>(const_cast<DomainParticipantImpl*>(domainParticipant_)->GetRelatedParticipant().get(), spdp_.get());

	spdp_->SetParticipantDataWriter(builtinParticipantDataWriter_);
	spdp_->SetParticipantDataReader(builtinParticipantDataReader_);

	sedp_->SetPublicationEndpoint(builtinPublicationsDataWriter_.get(), builtinPublicationsDataReader_.get());
	sedp_->SetSubscriptionEndpoint(builtinSubscriptionsDataWriter_.get(), builtinSubscriptionsDataReader_.get());

}

void Discovery::WriteSpdPdiscoveredParticipantData()
{
	SPDPdiscoveredParticipantData data;
	std::shared_ptr<Participant> participant = domainParticipant_->GetRelatedParticipant();
	DomainParticipantQos dpQos;
	domainParticipant_->get_qos(dpQos);
	data.ddsParticipantData.key = Conversion::ToBuiltinTopicKey(participant->GetGuid());
	data.ddsParticipantData.user_data = dpQos.user_data;
	data.domainId = domainParticipant_->get_domain_id();
	data.protocolVersion = PROTOCOLVERSION_2_5;
	data.vendorId = VENDORID_UNKNOWN;
	data.defaultUnicastLocatorList = participant->GetDefaultUnicastLocators();
	data.defaultMulticastLocatorList = participant->GetDefaultMulticastLocators();
	data.metatrafficUnicastLocatorList = participant->GetMetatrafficUnicastLocators();
	data.metatrafficMulticastLocatorList = participant->GetMetatrafficMulticastLocators();
	data.availableBuiltinEndpoints = builtinEndpointSet_;

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

void Discovery::WriteSubscriptionData(DiscoveredReaderData& data)
{
	sedp_->WriteSubscriptionData(data);
}

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

