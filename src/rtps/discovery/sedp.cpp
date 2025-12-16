#include "rtps/discovery/sedp.h"
#include "rtps/discovery/spdp.h"
#include "rtps/messages/parameter.h"
#include "dcps/domain/domainparticipant.h"
#include "common/log/logger.h"
#ifdef DDSSECURITY
#include "security/participantgenericmessagetypesupport.h"
#include "security/participantmessagesecuredatatypesupport.h"
#include "security/securitycommon.h"
#include "rtps/discovery/discovery.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#endif // DDSSECURITY
USING_TRAVODDS_NAMESPACE

Sedp::Sedp(Participant* participant,Spdp* spdp)
	: participant_(participant),
	  spdp_(spdp),
	  memoryPool_(MemoryPool::GetInstance())
{
}

Sedp::~Sedp()
{
	if (publicationsWriter_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(publicationsWriter_->GetGuid());
		participant_->DeleteWriter(handle);
	}
	if (subscriptionsWriter_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(subscriptionsWriter_->GetGuid());
		participant_->DeleteWriter(handle);
	}
	if (publicationsReader_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(publicationsReader_->GetGuid());
		participant_->DeleteReader(handle);
	}
	if (subscriptionsReader_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(subscriptionsReader_->GetGuid());
		participant_->DeleteReader(handle);
	}
#ifdef DDSSECURITY
	if (publicationsSecureWriter_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(publicationsSecureWriter_->GetGuid());
		participant_->DeleteWriter(handle);
	}
	if (subscriptionsSecureWriter_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(subscriptionsSecureWriter_->GetGuid());
		participant_->DeleteWriter(handle);
	}
	if (publicationsSecureReader_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(publicationsSecureReader_->GetGuid());
		participant_->DeleteReader(handle);
	}
	if (subscriptionsSecureReader_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(subscriptionsSecureReader_->GetGuid());
		participant_->DeleteReader(handle);
	}
	if (participantMessageSecureWriter_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(participantMessageSecureWriter_->GetGuid());
		participant_->DeleteWriter(handle);
	}
	if (participantMessageSecureReader_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(participantMessageSecureReader_->GetGuid());
		participant_->DeleteReader(handle);
	}
	if (participantStatelessMessageWriter_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(participantStatelessMessageWriter_->GetGuid());
		participant_->DeleteWriter(handle);
	}
	if (participantStatelessMessageReader_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(participantStatelessMessageReader_->GetGuid());
		participant_->DeleteReader(handle);
	}
	if (participantVolatileSecureMessageWriter_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(participantVolatileSecureMessageWriter_->GetGuid());
		participant_->DeleteWriter(handle);
	}
	if (participantVolatileSecureMessageReader_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(participantVolatileSecureMessageReader_->GetGuid());
		participant_->DeleteReader(handle);
	}
#endif // DDSSECURITY
}

void Sedp::SetPublicationEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader)
{
	DataWriterQos writerQos;
	dataWriter->get_qos(writerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER, true, ENTITY_WRITER);
    std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter, writerQos, guid, true);
	if (nullptr == writer) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "writer is nullptr");
		return;
	}
	publicationsWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);

	DataReaderQos readerQos;
	dataReader->get_qos(readerQos);
	guid = participant_->GenerateGuid(ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER, true, ENTITY_READER);
    std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader, readerQos, guid,true);
	if (nullptr == reader) {	
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "reader is nullptr");
		return;
	}
	publicationsReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);

	reader->SetReaderListener(this);
}

void Sedp::SetSubscriptionEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader)
{
	DataWriterQos writerQos;
	dataWriter->get_qos(writerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER, true, ENTITY_WRITER);
    std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter, writerQos, guid, true);
	if (nullptr == writer) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "writer is nullptr");
		return;
	}
	subscriptionsWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);

	DataReaderQos readerQos;
	dataReader->get_qos(readerQos);
	guid = participant_->GenerateGuid(ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER, true, ENTITY_READER);
    std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader, readerQos, guid, true);
	if (nullptr == reader) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "reader is nullptr");
		return;
	}
	subscriptionsReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);

	reader->SetReaderListener(this);
}

#ifdef DDSSECURITY
void Sedp::SetPublicationSecureEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader)
{
	DataWriterQos writerQos;
	dataWriter->get_qos(writerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER, true, ENTITY_WRITER);
	std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter, writerQos, guid, true);
	if (nullptr == writer) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "writer is nullptr");
		return;
	}
	publicationsSecureWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);

	DataReaderQos readerQos;
	dataReader->get_qos(readerQos);
	guid = participant_->GenerateGuid(ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER, true, ENTITY_READER);
	std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader, readerQos, guid, true);
	if (nullptr == reader) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "reader is nullptr");
		return;
	}
	publicationsSecureReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);
	reader->SetReaderListener(this);
}

void Sedp::SetSubscriptionSecureEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader)
{
	DataWriterQos writerQos;
	dataWriter->get_qos(writerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER, true, ENTITY_WRITER);
	std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter, writerQos, guid, true);
	if (nullptr == writer) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "writer is nullptr");
		return;
	}
	subscriptionsSecureWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);

	DataReaderQos readerQos;
	dataReader->get_qos(readerQos);
	guid = participant_->GenerateGuid(ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER, true, ENTITY_READER);
	std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader, readerQos, guid, true);

	if (nullptr == reader) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "reader is nullptr");
		return;
	}
	subscriptionsSecureReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);
	reader->SetReaderListener(this);
}

void Sedp::SetParticipantMessageSecureEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader)
{
	DataWriterQos writerQos;
	dataWriter->get_qos(writerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER, true, ENTITY_WRITER);
	std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter, writerQos, guid, true);
	if (nullptr == writer) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "writer is nullptr");
		return;
	}
	participantMessageSecureWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);

	DataReaderQos readerQos;
	dataReader->get_qos(readerQos);
	guid = participant_->GenerateGuid(ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER, true, ENTITY_READER);
	std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader, readerQos, guid, true);
	if (nullptr == reader) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "reader is nullptr");
		return;
	}
	participantMessageSecureReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);
	reader->SetReaderListener(this);
}

void Sedp::SetParticipantStatelessMessageEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader) {
	DataWriterQos writerQos;
	dataWriter->get_qos(writerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER, true, ENTITY_WRITER);
	std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter, writerQos, guid, true);
	if (nullptr == writer) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "writer is nullptr");
		return;
	}
	participantStatelessMessageWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);

	DataReaderQos readerQos;
	dataReader->get_qos(readerQos);
	guid = participant_->GenerateGuid(ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER, true, ENTITY_READER);
	std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader, readerQos, guid, true);
	if (nullptr == reader) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "reader is nullptr");
		return;
	}
	participantStatelessMessageReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);
	reader->SetReaderListener(this);
}
void Sedp::SetParticipantVolatileSecureMessageEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader) {
	DataWriterQos writerQos;
	dataWriter->get_qos(writerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER, true, ENTITY_WRITER);
	std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter, writerQos, guid, true);
	if (nullptr == writer) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "writer is nullptr");
		return;
	}
	participantVolatileSecureMessageWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);

	DataReaderQos readerQos;
	dataReader->get_qos(readerQos);
	guid = participant_->GenerateGuid(ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER, true, ENTITY_READER);
	std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader, readerQos, guid, true);
	if (nullptr == reader) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "reader is nullptr");
		return;
	}
	participantVolatileSecureMessageReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);
	reader->SetReaderListener(this);
}
#endif // DDSSECURITY

void Sedp::AddSedpProxy(const SPDPdiscoveredParticipantData& data)
{
	// publications和subscriptions
	DiscoveredWriterData writerData;
	writerData.remoteWriterGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	writerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	writerData.multicastLocatorList = data.metatrafficMulticastLocatorList;

	writerData.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
	writerData.ddsPublicationData = publicationsWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);

	writerData.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
	writerData.ddsPublicationData = subscriptionsWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);

	DiscoveredReaderData readerData;
	readerData.remoteReaderGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	readerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	readerData.multicastLocatorList = data.metatrafficMulticastLocatorList;

	readerData.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
	readerData.ddsSubscriptionData = publicationsReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);

	readerData.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
	readerData.ddsSubscriptionData = subscriptionsReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);

	//GUID_t guid;
	//guid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
	//ReaderProxy* readerProxy = new ReaderProxy(guid, GUID_UNKNOWN, data.metatrafficUnicastLocatorList, data.metatrafficMulticastLocatorList);
	//publicationsWriter_->MatchedReaderAdd(readerProxy);

	//guid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
	//readerProxy = new ReaderProxy(guid, GUID_UNKNOWN, data.metatrafficUnicastLocatorList, data.metatrafficMulticastLocatorList);
	//subscriptionsWriter_->MatchedReaderAdd(readerProxy);

	//guid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
	//WriterProxy* writerProxy = new WriterProxy(guid, GUID_UNKNOWN, data.metatrafficUnicastLocatorList, data.metatrafficMulticastLocatorList);
	//publicationsReader_->MatchedWriterAdd(writerProxy);

	//guid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
	//writerProxy = new WriterProxy(guid, GUID_UNKNOWN, data.metatrafficUnicastLocatorList, data.metatrafficMulticastLocatorList);
	//subscriptionsReader_->MatchedWriterAdd(writerProxy);
}

#ifdef DDSSECURITY
void Sedp::AddPartSedpProxy(const SPDPdiscoveredParticipantData& data)
{
	DiscoveredWriterData writerData;
	writerData.remoteWriterGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	writerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	writerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	writerData.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
	writerData.ddsPublicationData = publicationsSecureWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);

	writerData.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
	writerData.ddsPublicationData = subscriptionsSecureWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);

	writerData.remoteWriterGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
	writerData.ddsPublicationData = participantMessageSecureWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);

	writerData.remoteWriterGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER;
	writerData.ddsPublicationData = participantStatelessMessageWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);

	writerData.remoteWriterGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER;
	writerData.ddsPublicationData = participantVolatileSecureMessageWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);

	DiscoveredReaderData readerData;
	readerData.remoteReaderGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	readerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	readerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	readerData.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
	readerData.ddsSubscriptionData = publicationsSecureReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);

	readerData.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
	readerData.ddsSubscriptionData = subscriptionsSecureReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);

	readerData.remoteReaderGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
	readerData.ddsSubscriptionData = participantMessageSecureReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);

	readerData.remoteReaderGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER;
	readerData.ddsSubscriptionData = participantStatelessMessageReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);

	readerData.remoteReaderGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER;
	readerData.ddsSubscriptionData = participantVolatileSecureMessageReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);
}

void Sedp::AddRemainSedpProxy(const SPDPdiscoveredParticipantData& data) {
	//DiscoveredWriterData writerData;
	//writerData.remoteWriterGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	//writerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	//writerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	//writerData.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
	//writerData.ddsPublicationData = publicationsSecureWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	//participant_->AddRemoteWriter(writerData);

	//writerData.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
	//writerData.ddsPublicationData = subscriptionsSecureWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	//participant_->AddRemoteWriter(writerData);

	//DiscoveredReaderData readerData;
	//readerData.remoteReaderGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	//readerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	//readerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	//readerData.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
	//readerData.ddsSubscriptionData = publicationsSecureReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	//participant_->AddRemoteReader(readerData);

	//readerData.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
	//readerData.ddsSubscriptionData = subscriptionsSecureReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	//participant_->AddRemoteReader(readerData);

	AddSedpProxy(data);
}

void Sedp::AddBuiltinSpdpProxy(const SPDPdiscoveredParticipantData& data)
{
	DomainParticipantImpl* dp = spdp_->GetDiscovery()->GetParticipant();
	dp->GetLivelinessManager()->AddParticipantMessageProxy(data);

	DiscoveredWriterData writerData;
	writerData.remoteWriterGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	writerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	writerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	writerData.remoteWriterGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER;
	writerData.ddsPublicationData = participantStatelessMessageWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);

	DiscoveredReaderData readerData;
	readerData.remoteReaderGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	readerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	readerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	readerData.remoteReaderGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER;
	readerData.ddsSubscriptionData = participantStatelessMessageReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);
}

void Sedp::AddBuiltinSedpProxy(const SPDPdiscoveredParticipantData& data)
{
	DiscoveredWriterData writerData;
	writerData.remoteWriterGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	writerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	writerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	writerData.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
	writerData.ddsPublicationData = publicationsWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);
	writerData.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
	writerData.ddsPublicationData = subscriptionsWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);

	DiscoveredReaderData readerData;
	readerData.remoteReaderGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	readerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	readerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	readerData.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
	readerData.ddsSubscriptionData = publicationsReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);
	readerData.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
	readerData.ddsSubscriptionData = subscriptionsReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);
}

void Sedp::AddVolatileMessageBuiltinSecureSpdpProxy(const SPDPdiscoveredParticipantData& data) {
	DiscoveredWriterData writerData;
	writerData.remoteWriterGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	writerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	writerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	writerData.remoteWriterGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER;
	writerData.ddsPublicationData = participantVolatileSecureMessageWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);
	
	DiscoveredReaderData readerData;
	readerData.remoteReaderGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	readerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	readerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	readerData.remoteReaderGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER;
	readerData.ddsSubscriptionData = participantVolatileSecureMessageReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);
}

void Sedp::AddBuiltinSecureSedpProxy(const SPDPdiscoveredParticipantData& data) {
	DiscoveredWriterData writerData;
	writerData.remoteWriterGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	writerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	writerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	writerData.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
	writerData.ddsPublicationData = publicationsSecureWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);
	writerData.remoteWriterGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
	writerData.ddsPublicationData = subscriptionsSecureWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);
	writerData.remoteWriterGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
	writerData.ddsPublicationData = participantMessageSecureWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
	participant_->AddRemoteWriter(writerData);

	DiscoveredReaderData readerData;
	readerData.remoteReaderGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	readerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	readerData.multicastLocatorList = data.metatrafficMulticastLocatorList;
	readerData.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
	readerData.ddsSubscriptionData = publicationsSecureReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);
	readerData.remoteReaderGuid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
	readerData.ddsSubscriptionData = subscriptionsSecureReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);
	readerData.remoteReaderGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
	readerData.ddsSubscriptionData = participantMessageSecureReader_->GetRelatedDataReader()->GetBuiltinTopicData();
	participant_->AddRemoteReader(readerData);
}
#endif // DDSSECURITY


void Sedp::RemoveSedpProxy(GUID_t guid)
{
	guid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
	participant_->RemoveRemoteReader(guid);

	guid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
	participant_->RemoveRemoteReader(guid);

	guid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER;
	participant_->RemoveRemoteWriter(guid);

	guid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER;
	participant_->RemoveRemoteWriter(guid);

#ifdef DDSSECURITY
	guid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER;
	participant_->RemoveRemoteReader(guid);

	guid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER;
	participant_->RemoveRemoteReader(guid);

	guid.entityId = ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER;
	participant_->RemoveRemoteWriter(guid);

	guid.entityId = ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER;
	participant_->RemoveRemoteWriter(guid);

	guid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER;
	participant_->RemoveRemoteReader(guid);

	guid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER;
	participant_->RemoveRemoteWriter(guid);

	guid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER;
	participant_->RemoveRemoteReader(guid);

	guid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER;
	participant_->RemoveRemoteWriter(guid);
#endif // DDSSECURITY
}

void Sedp::on_data_available(std::shared_ptr<CacheChange> change)
{
	if (change->kind >= ALIVE_FILTERED) {
		GUID_t guid = Conversion::ToGuid(change->instanceHandle);

		octet flag = change->kind;

		// D 已处理
		if (flag | NOT_ALIVE_DISPOSED) {

		}
		// U 未注册
		if (flag | NOT_ALIVE_UNREGISTERED) {
			if(change->writerGuid.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER)
				participant_->RemoveRemoteWriter(guid);
			if (change->writerGuid.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER)
				participant_->RemoveRemoteReader(guid);
		}

		// F 已过滤
		if (flag | ALIVE_FILTERED) {

		}
		return;
	}

	if (change->serializedBuffer) {

		if (change->writerGuid.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER)
		{
			DiscoveredWriterData  data;

			TakeDiscoveredWriterData(data, change->serializedBuffer);

			ReceiveDiscoveredWriterData(data);
		}
#ifdef DDSSECURITY
		if (change->writerGuid.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER)
		{
			DiscoveredWriterSecureData  data;

			TakeDiscoveredWriterSecureData(data, change->serializedBuffer);

			ReceiveDiscoveredWriterSecureData(data);
		}
#endif // DDSSECURITY
		if (change->writerGuid.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER) {
			DiscoveredReaderData  data;

			TakeDiscoveredReaderData(data, change->serializedBuffer);

			ReceiveDiscoveredReaderData(data);
		}
#ifdef DDSSECURITY
		if (change->writerGuid.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER) {
			DiscoveredReaderSecureData  data;

			TakeDiscoveredReaderSecureData(data, change->serializedBuffer);

			ReceiveDiscoveredReaderSecureData(data);
		}

		if (change->writerGuid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER) {
			ParticipantMessageSecureData  data;

			TakeParticipantMessageSecureData(data, change->serializedBuffer);

			//ReceiveParticipantMessageSecureData(data); TODO
		}
		//接收到ParticipantGenericMessage数据（握手或者加密相关Token）
		if (change->writerGuid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER)
		{
			ParticipantGenericMessage data;
			TakeParticipantGenericMessageData(data, change->serializedBuffer);
			ReceiveParticipantGenericMessageData(data);
		}
		if (change->writerGuid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER)
		{
			ParticipantGenericMessage data;
			TakeParticipantGenericMessageData(data, change->serializedBuffer);
			ReceiveParticipantGenericMessageData(data);
		}
#endif // DDSSECURITY
	}
}

void Sedp::WritePublicationData(DiscoveredWriterData& data)
{
	std::cout << __FUNCTION__ << std::endl;
	SerializedBuffer *buffer = new SerializedBuffer;
	ParameterList list(data);
	unsigned short size = list.GetMessageSize();

	buffer->buffer_size = size;
	memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;

	list.WriterToMessage((octet*)buffer->buffer, size);

	Time_t cur_time;
	DomainParticipant::get_current_time(cur_time);

	InstanceHandle_t handle;
	memcpy(handle.keyHash.value, data.ddsPublicationData.key.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);

	std::shared_ptr<CacheChange> change = publicationsWriter_->NewChange(ALIVE, buffer, handle, cur_time);

	ReturnCode_t ret = publicationsWriter_->AddBuiltinCache(change);

	if(RETCODE_OK != ret) {
		//TODO 记录日志
		TRAVODDS_LOG(LOG_ERROR, "publicationsWriter AddBuiltinCache Err ret = %d", ret);
	}
}

void Sedp::WriteSubscriptionData(DiscoveredReaderData& data)
{
	SerializedBuffer *buffer = new SerializedBuffer;
	ParameterList list(data);
	unsigned short size = list.GetMessageSize();

	buffer->buffer_size = size;
	memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;

	list.WriterToMessage((octet*)buffer->buffer, size);

	Time_t cur_time;
	DomainParticipant::get_current_time(cur_time);
	InstanceHandle_t handle;
	memcpy(handle.keyHash.value, data.ddsSubscriptionData.key.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	std::shared_ptr<CacheChange> change = subscriptionsWriter_->NewChange(ALIVE, buffer, handle, cur_time);

	ReturnCode_t ret = subscriptionsWriter_->AddBuiltinCache(change);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
		TRAVODDS_LOG(LOG_ERROR, "subscriptionsWriter_ AddBuiltinCache Err ret = %d", ret);
	}
}

#ifdef DDSSECURITY
void Sedp::WritePublicationSecureData(DiscoveredWriterSecureData& data)
{
	std::cout << __FUNCTION__ << std::endl;
    SerializedBuffer *buffer = new SerializedBuffer;
    ParameterList list(data);
    unsigned short size = list.GetMessageSize();

    buffer->buffer_size = size;
    memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->origin_buffer);
    buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;

    list.WriterToMessage((octet*)buffer->buffer, size);

    Time_t cur_time;
    DomainParticipant::get_current_time(cur_time);

    InstanceHandle_t handle;
    memcpy(handle.keyHash.value, data.ddsPublicationData.key.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);

    std::shared_ptr<CacheChange> change = publicationsSecureWriter_->NewChange(ALIVE, buffer, handle, cur_time);

    ReturnCode_t ret = publicationsSecureWriter_->AddBuiltinCache(change);

    if (RETCODE_OK != ret) {
        //TODO 记录日志
		TRAVODDS_LOG(LOG_ERROR, "publicationsWriter AddBuiltinCache Err ret = %d", ret);
    }
}

void Sedp::WriteSubscriptionSecureData(DiscoveredReaderSecureData& data)
{
    SerializedBuffer *buffer = new SerializedBuffer;
    ParameterList list(data);
    unsigned short size = list.GetMessageSize();

    buffer->buffer_size = size;
    memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->origin_buffer);
    buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;

    list.WriterToMessage((octet*)buffer->buffer, size);

    Time_t cur_time;
    DomainParticipant::get_current_time(cur_time);
    InstanceHandle_t handle;
    memcpy(handle.keyHash.value, data.ddsSubscriptionData.key.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
    std::shared_ptr<CacheChange> change = subscriptionsSecureWriter_->NewChange(ALIVE, buffer, handle, cur_time);

    ReturnCode_t ret = subscriptionsSecureWriter_->AddBuiltinCache(change);

    if (RETCODE_OK != ret) {
        //TODO 记录日志
		TRAVODDS_LOG(LOG_ERROR, "subscriptionsWriter_ AddBuiltinCache Err ret = %d", ret);
    }
}

void Sedp::WriteParticipantMessageSecureData(ParticipantMessageSecureData& data) {
	SerializedBuffer* buffer = new SerializedBuffer;

	buffer->buffer_size = ParticipantMessageSecureDataTypeSupport::get_instance()->get_serialized_data_size(&data, 0);
	memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;
	CdrSerializer cdr(buffer);
	uint32_t res = ParticipantMessageSecureDataTypeSupport::get_instance()->serialize_data(&data, &cdr, true);
	if (res != 0) {
		TRAVODDS_LOG(LOG_ERROR, "ParticipantMessageSecureData serialize_data Err res = %d", res);
		return;
	}
	Time_t cur_time;
	DomainParticipant::get_current_time(cur_time);
	std::shared_ptr<CacheChange> change = participantMessageSecureWriter_->NewChange(ALIVE, buffer, HANDLE_NIL, cur_time);

	ReturnCode_t ret = participantMessageSecureWriter_->AddBuiltinCache(change);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
		TRAVODDS_LOG(LOG_ERROR, "subscriptionsWriter_ AddBuiltinCache Err ret = %d", ret);
	}
}

ReturnCode_t Sedp::WriteParticipantStatelessMessageData(const GUID_t* remoteGuid, ParticipantStatelessMessage& data)
{
	if (remoteGuid != NULL)
	{
		memcpy(data.destinationParticipantKey.value, remoteGuid, 12);
		memcpy(data.destinationParticipantKey.value + 3, &ENTITYID_PARTICIPANT, 4);
	}
	else
	{
		memset(data.destinationParticipantKey.value, 0, 16);
	}
	memset(data.destinationEndpointKey.value, 0, 16);
	//获取16字节唯一标识实体赋给sample。
	//memcpy(&data.messageIdentity.sourceGuid, spdp_->GetDiscovery()->domainParticipant_->get_instance_handle().keyHash.value, 16);
	data.messageIdentity.sourceGuid = Conversion::ToBuiltinTopicKey(spdp_->GetDiscovery()->GetParticipant()->GetRelatedParticipant()->GetGuid());
	memset(data.destinationEndpointKey.value, 0, 16);
	memset(data.sourceEndpointKey.value, 0, 16);
	++statelessMsgSeqNum_.low;
	if (statelessMsgSeqNum_.low == 0)
	{
		++statelessMsgSeqNum_.high;
	}
	data.messageIdentity.sequenceNumber = statelessMsgSeqNum_;
	SerializedBuffer* buffer = new SerializedBuffer;
	// buffer->buffer_size = 4; //端序CDR编码的数据封装 已经放在了serialized的size计算中了
	// buffer->buffer_size += ParticipantGenericMessageTypeSupport::get_instance()->get_serialized_data_size(&data, 0); //ParticipantGenericMessageGetSerializedSampleSize(&data);
	buffer->buffer_size = ParticipantGenericMessageTypeSupport::get_instance()->get_serialized_data_size(&data, 0); //ParticipantGenericMessageGetSerializedSampleSize(&data);
	memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;
	CdrSerializer cdr(buffer);
	uint32_t res = ParticipantGenericMessageTypeSupport::get_instance()->serialize_data(&data, &cdr, true);//ParticipantGenericMessageSerialize(&data, buffer);

	Time_t cur_time;
	DomainParticipant::get_current_time(cur_time);
	InstanceHandle_t handle;
	BUILTIN_TOPIC_KEY_TYPE_NATIVE value[4];
	//data.messageIdentity.sourceGuid.value;
	GUID_t guid = participant_->GetGuid();
	BuiltinTopicKey_t key = Conversion::ToBuiltinTopicKey(participant_->GetGuid());
	for (int i = 0; i < 4; ++i) {
		//value[i] = data.messageIdentity.sourceGuid.value[i];
		value[i] = key.value[i];
	}
	memcpy(handle.keyHash.value, value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	std::shared_ptr<CacheChange> change = participantStatelessMessageWriter_->NewChange(ALIVE, buffer, handle, cur_time);

	ReturnCode_t ret = participantStatelessMessageWriter_->AddBuiltinCache(change);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
		TRAVODDS_LOG(LOG_ERROR, "subscriptionsWriter_ AddBuiltinCache Err ret = %d", ret);
	}
	return ret;
}

void Sedp::WriteParticipantVolatileSecureMessage(const GUID_t* remoteGuid, ParticipantVolatileSecureMessage& data) {
	if (remoteGuid == NULL)
	{
		memset(data.destinationParticipantKey.value, 0, 16);
	}
	else
	{
		memcpy(data.destinationParticipantKey.value, remoteGuid, 12);
		memcpy(data.destinationParticipantKey.value + 3, &ENTITYID_PARTICIPANT, 4);
	}
	memcpy(data.messageIdentity.sourceGuid.value, participantVolatileSecureMessageWriter_->GetGuidPtr(), 16);
	++statelessMsgSeqNum_.low;
	if (statelessMsgSeqNum_.low == 0)
	{
		++statelessMsgSeqNum_.high;
	}
	data.messageIdentity.sequenceNumber = statelessMsgSeqNum_;
	GUID_t remoteReceiverGuid = *remoteGuid;
	remoteReceiverGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER;

	SerializedBuffer* buffer = new SerializedBuffer;
	buffer->buffer_size = ParticipantGenericMessageTypeSupport::get_instance()->get_serialized_data_size(&data, 0); //ParticipantGenericMessageGetSerializedSampleSize(&data);
	memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->origin_buffer);
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;
	CdrSerializer cdr(buffer);
	uint32_t res = ParticipantGenericMessageTypeSupport::get_instance()->serialize_data(&data, &cdr, true);//ParticipantGenericMessageSerialize(&data, buffer);

	Time_t cur_time;
	DomainParticipant::get_current_time(cur_time);
	InstanceHandle_t handle;
	BUILTIN_TOPIC_KEY_TYPE_NATIVE value[4];
	data.messageIdentity.sourceGuid.value;
	for (int i = 0; i < 4; ++i) {
		value[i] = data.messageIdentity.sourceGuid.value[i];
	}
	memcpy(handle.keyHash.value, value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	std::shared_ptr<CacheChange> change = participantVolatileSecureMessageWriter_->NewChange(ALIVE, buffer, handle, cur_time);

	ReturnCode_t ret = participantVolatileSecureMessageWriter_->AddBuiltinCache(change);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
		TRAVODDS_LOG(LOG_ERROR, "subscriptionsWriter_ AddBuiltinCache Err ret = %d", ret);
	}
}
#endif // DDSSECURITY

void Sedp::RemovePublicationData(GUID_t & guid)
{
	SerializedBuffer *buffer = new SerializedBuffer;
	buffer->buffer_size = 0;
	if (!memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT * 2, buffer->origin_buffer)) {
	}
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;

	Time_t cur_time;
	DomainParticipant::get_current_time(cur_time);

	InstanceHandle_t instanceHandle = Conversion::ToInstanceHandle(guid);
	std::shared_ptr<CacheChange> change = publicationsWriter_->NewChange(NOT_ALIVE_DISPOSED_UNREGISTERED , buffer, instanceHandle, cur_time);


	Parameter_t keyParam, statueParam;
	keyParam.parameterId = PID_KEY_HASH;
	keyParam.length = MIG_RTPS_KEY_HASH_MAX_LENGTH;
	keyParam.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + MIG_RTPS_KEY_HASH_MAX_LENGTH);

	StatusInfo_t status = { 0 };
	status[3] = StatusInfoFlag::DISPOSED_FLAG | StatusInfoFlag::UNREGISTERED_FLAG;
	statueParam.parameterId = PID_STATUS_INFO;
	statueParam.length = sizeof(StatusInfo_t);
	statueParam.value.assign(status, status + sizeof(StatusInfo_t));

	change->inlineQos.push_back(keyParam);
	change->inlineQos.push_back(statueParam);

	ReturnCode_t ret = publicationsWriter_->AddBuiltinCache(change);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
		TRAVODDS_LOG(LOG_ERROR, "publicationsWriter AddBuiltinCache Err ret = %d", ret);
	}
	
	publicationsWriter_->RemoveCacheByHandle(instanceHandle);
}

void Sedp::RemoveSubscriptionData(GUID_t & guid)
{
	SerializedBuffer *buffer = new SerializedBuffer;
	buffer->buffer_size = 0;
	if (!memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT * 2, buffer->origin_buffer)) {
	}
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;

	Time_t cur_time;
	DomainParticipant::get_current_time(cur_time);

    InstanceHandle_t instanceHandle = Conversion::ToInstanceHandle(guid);

	std::shared_ptr<CacheChange> change = subscriptionsWriter_->NewChange(NOT_ALIVE_DISPOSED_UNREGISTERED, buffer, instanceHandle, cur_time);

	
	Parameter_t keyParam, statueParam;
	keyParam.parameterId = PID_KEY_HASH;
	keyParam.length = MIG_RTPS_KEY_HASH_MAX_LENGTH;
	keyParam.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + MIG_RTPS_KEY_HASH_MAX_LENGTH);

	StatusInfo_t status = { 0 };
	status[3] = StatusInfoFlag::DISPOSED_FLAG | StatusInfoFlag::UNREGISTERED_FLAG;
	statueParam.parameterId = PID_STATUS_INFO;
	statueParam.length = sizeof(StatusInfo_t);
	statueParam.value.assign(status, status + sizeof(StatusInfo_t));

	change->inlineQos.push_back(keyParam);
	change->inlineQos.push_back(statueParam);

	ReturnCode_t ret = subscriptionsWriter_->AddBuiltinCache(change);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
		TRAVODDS_LOG(LOG_ERROR, "subscriptionsWriter_ AddBuiltinCache Err ret = %d", ret);
	}
	
	subscriptionsWriter_->RemoveCacheByHandle(instanceHandle);
}

void Sedp::TakeDiscoveredWriterData(DiscoveredWriterData& data, SerializedBuffer* buffer)
{
	ParameterList list((octet*)(buffer->buffer), buffer->buffer_size);
	list.GetDiscoveredData(data);

	data.remoteWriterGuid = Conversion::ToGuid(data.ddsPublicationData.key);

	/* wangyi Cyclone DDS适配 participant_key 值为{0,0,0,0}时使用Guid转换  */
	BuiltinTopicKey_t keyNull;
	if (data.ddsPublicationData.participant_key == keyNull)
	{
		GUID_t participantGuid(data.remoteWriterGuid.prefix, ENTITYID_PARTICIPANT);
		data.ddsPublicationData.participant_key = Conversion::ToBuiltinTopicKey(participantGuid);
	}
}
#ifdef DDSSECURITY
void Sedp::TakeDiscoveredWriterSecureData(DiscoveredWriterSecureData& data, SerializedBuffer* buffer)
{
    ParameterList list((octet*)(buffer->buffer), buffer->buffer_size);
    list.GetDiscoveredSecureData(data);

    data.remoteWriterGuid = Conversion::ToGuid(data.ddsPublicationData.key);

    /* wangyi Cyclone DDS适配 participant_key 值为{0,0,0,0}时使用Guid转换  */
    BuiltinTopicKey_t keyNull;
    if (data.ddsPublicationData.participant_key == keyNull)
    {
        GUID_t participantGuid(data.remoteWriterGuid.prefix, ENTITYID_PARTICIPANT);
        data.ddsPublicationData.participant_key = Conversion::ToBuiltinTopicKey(participantGuid);
    }
}
#endif // DDSSECURITY
void Sedp::ReceiveDiscoveredWriterData(DiscoveredWriterData& data)
{
	SPDPdiscoveredParticipantData participantData;
	if (RETCODE_OK == spdp_->GetParticipantData(data.ddsPublicationData.participant_key, participantData)) {
		data.unicastLocatorList = participantData.defaultUnicastLocatorList;
	}

	participant_->AddRemoteWriter(data);
}
#ifdef DDSSECURITY
void Sedp::ReceiveDiscoveredWriterSecureData(DiscoveredWriterSecureData& data) {

    SPDPdiscoveredParticipantData participantData;
    if (RETCODE_OK == spdp_->GetParticipantData(data.ddsPublicationData.participant_key, participantData)) {
        data.unicastLocatorList = participantData.defaultUnicastLocatorList;
    }

    participant_->AddRemoteSecureWriter(data);
}
#endif // DDSSECURITY
void Sedp::TakeDiscoveredReaderData(DiscoveredReaderData& data, SerializedBuffer* buffer)
{
	ParameterList list((octet*)(buffer->buffer), buffer->buffer_size);
	list.GetDiscoveredData(data);

	data.remoteReaderGuid = Conversion::ToGuid(data.ddsSubscriptionData.key);

	/* wangyi Cyclone DDS适配 participant_key 值为{0,0,0,0}时使用Guid转换  */
	BuiltinTopicKey_t keyNull;
	if (data.ddsSubscriptionData.participant_key == keyNull)
	{
		GUID_t participantGuid(data.remoteReaderGuid.prefix, ENTITYID_PARTICIPANT);
		data.ddsSubscriptionData.participant_key = Conversion::ToBuiltinTopicKey(participantGuid);
	}

}
#ifdef DDSSECURITY
void Sedp::TakeDiscoveredReaderSecureData(DiscoveredReaderSecureData& data, SerializedBuffer* buffer)
{
    ParameterList list((octet*)(buffer->buffer), buffer->buffer_size);
    list.GetDiscoveredSecureData(data);

    data.remoteReaderGuid = Conversion::ToGuid(data.ddsSubscriptionData.key);

    /* wangyi Cyclone DDS适配 participant_key 值为{0,0,0,0}时使用Guid转换  */
    BuiltinTopicKey_t keyNull;
    if (data.ddsSubscriptionData.participant_key == keyNull)
    {
        GUID_t participantGuid(data.remoteReaderGuid.prefix, ENTITYID_PARTICIPANT);
        data.ddsSubscriptionData.participant_key = Conversion::ToBuiltinTopicKey(participantGuid);
    }
}

void Sedp::TakeParticipantMessageSecureData(ParticipantMessageSecureData& data, SerializedBuffer* buffer) {
    CdrDeserializer cdr(buffer);
    ParticipantMessageSecureDataTypeSupport::get_instance()->deserialize_data(&data, &cdr, true);
}

void Sedp::TakeParticipantGenericMessageData(ParticipantGenericMessage& data, SerializedBuffer* buffer)
{
    CdrDeserializer cdr(buffer);
    ParticipantGenericMessageTypeSupport::get_instance()->deserialize_data(&data, &cdr, true);
}
#endif // DDSSECURITY

void Sedp::ReceiveDiscoveredReaderData(DiscoveredReaderData& data)
{
	SPDPdiscoveredParticipantData participantData;
	if( RETCODE_OK  == spdp_->GetParticipantData(data.ddsSubscriptionData.participant_key, participantData))
	{
		data.unicastLocatorList = participantData.defaultUnicastLocatorList;
		data.multicastLocatorList = participantData.defaultMulticastLocatorList;
	}

	participant_->AddRemoteReader(data);
}

#ifdef DDSSECURITY
void Sedp::ReceiveDiscoveredReaderSecureData(DiscoveredReaderSecureData& data)
{
	//TEST
	if (data.remoteReaderGuid.IsUserDefinedEntity()) {
		int tmp = 0;
	}

	SPDPdiscoveredParticipantData participantData;
	if (RETCODE_OK == spdp_->GetParticipantData(data.ddsSubscriptionData.participant_key, participantData))
	{
		data.unicastLocatorList = participantData.defaultUnicastLocatorList;
		data.multicastLocatorList = participantData.defaultMulticastLocatorList;
	}
	participant_->AddRemoteSecureReader(data);
}

void Sedp::ReceiveParticipantGenericMessageData(ParticipantGenericMessage& data) {
	GUID_t destGuid;
	memcpy(&destGuid, data.destinationParticipantKey.value, 16);
	DomainParticipantImpl* dp = spdp_->GetDiscovery()->GetParticipant();
	GUID_t localParticipantGuid = dp->GetRelatedParticipant()->GetGuid();
	// 判断目标是否是自己
	if (localParticipantGuid != destGuid) {
		return;
	}
	GUID_t sourceParticipantGuid;
	memcpy(&sourceParticipantGuid, data.messageIdentity.sourceGuid.value, 12);
	sourceParticipantGuid.entityId = ENTITYID_PARTICIPANT;
	
	// 此ParticipantGenericMessage对应的远端域参与者未在本地维护
	//if (!spdp_->remoteDPInfo_.count(sourceParticipantGuid))
	//{
	//	return;
	//}
	
	// dpInfo是对应的远端域参与者信息
	DiscoveryDPInfo* dpInfo = spdp_->discParts_[sourceParticipantGuid.prefix];
	if (!dpInfo || !dpInfo->enableDDSSec_)
	{
		TRAVODDS_LOG(LOG_ERROR, "match dpInfo is null or match dpinfo disableddssec.");
		return;
	}
	SecurityException secExce;
	switch (dpInfo->endpointState_)
	{
	case DiscoveryDPInfo::DDSSEC_HANDSHAKE_EMPTY:
	{
		TRAVODDS_LOG(LOG_ERROR, "Invalid Participant handshake state: DDSSEC_HANDSHAKE_EMPTY.");
		return;
	}
	case DiscoveryDPInfo::DDSSEC_HANDSHAKE_REQUESTING:
	{
		TRAVODDS_LOG(LOG_ERROR, "Invalid Participant handshake state: DDSSEC_HANDSHAKE_REQUESTING.");
		return;
	}
	case DiscoveryDPInfo::DDSSEC_HANDSHAKE_WAITING_REQUEST: //挂起，等待握手回复
	{
		if (dpInfo->handshakeMsg_ == NULL || !dpInfo->handshakMsgInit_)
		{
			dpInfo->handshakMsgInit_ = true;
			dpInfo->handshakeMsg_ = new ParticipantGenericMessage();
			if (dpInfo->handshakeMsg_ == NULL)
			{
				TRAVODDS_LOG(LOG_ERROR, "Create handshake failed.");
				return;
			}
		}
		if (data.messageClassId != GMCLASSID_SECURITY_AUTH_HANDSHAKE)
		{
			TRAVODDS_LOG(LOG_ERROR, "Invalid MessageClassId");
			return;
		}
		if (data.messageData.size() != 1) {
			TRAVODDS_LOG(LOG_ERROR, "Invalid ParticipantStatelessMessage.");
			return;
		}
		HandshakeMessageToken handshakeToken;
		ValidationResult_t validationRst =
			dp->secPlugin_.authentication->begin_handshake_reply(
				&dpInfo->handshakeHdl_, //与远端的握手标识，域发现时生成
				&handshakeToken,
				&data.messageData[0], //解析出来的远端的handshake_message_in
				dpInfo->identityHdl_, //远端身份句柄
				dp->identityHdl_, //本地身份句柄
				(char*)spdp_->bigEndianMsgData_->buffer,
				spdp_->bigEndianMsgData_->buffer_size,
				&secExce);
		auto now = std::chrono::system_clock::now();
		std::time_t now_ms = std::chrono::system_clock::to_time_t(now);
		if (validationRst == VALIDATION_PENDING_HANDSHAKE_MESSAGE) { //HandshakeMsgSend
			ParticipantStatelessMessage* handshakeMsg = dpInfo->handshakeMsg_;
			if (handshakeMsg == NULL) {
				return;
			}
			Sequence<DataHolder>& messageData = handshakeMsg->messageData;
			messageData.push_back(handshakeToken);
			dp->secPlugin_.authentication->return_handshake_token(&handshakeToken, &secExce);
			dpInfo->handshakeMsg_->messageClassId = GMCLASSID_SECURITY_AUTH_HANDSHAKE;
			//printf("begin_handshake_reply local %s remote %s.\n",
			//	this->participant_->GetGuid().prefix.toString().c_str(),
			//	sourceParticipantGuid.prefix.toString().c_str());
			WriteParticipantStatelessMessageData(&sourceParticipantGuid, *dpInfo->handshakeMsg_); //reply
			Time_t time = { 1, 0 };
			//this->spdp_->AddHandShakeRetryTimer(time, sourceParticipantGuid);
			this->spdp_->AddHandShakeMsg(sourceParticipantGuid); // 定时器重发reply
			//reply响应完对端request之后置为DDSSEC_HANDSHAKE_WAITING_FINAL，等待processhandshake
			dpInfo->endpointState_ = DiscoveryDPInfo::DDSSEC_HANDSHAKE_WAITING_FINAL;
		}
		else if (validationRst == VALIDATION_OK) {
			dp->secPlugin_.authentication->return_handshake_token(&handshakeToken, &secExce);
			return;
		}
		else
		{
			if (dp->secAttributes_.allow_unauthenticated_participants)
			{
				dpInfo->enableDDSSec_ = false;
				dp->secPlugin_.authentication->return_handshake_token(&handshakeToken, &secExce);
				dpInfo->handshakeMsg_->messageClassId = "";
				//TOCHECK
				SPDPdiscoveredParticipantData partData;
				BuiltinTopicKey_t partKet = Conversion::ToBuiltinTopicKey(sourceParticipantGuid);
				int ret = spdp_->GetParticipantData(partKet, partData);
				if (ret != 0)
				{
					TRAVODDS_LOG(LOG_ERROR, "failed to get match partData.");
					return;
				}
				AddBuiltinSedpProxy(partData);
				return;
			}
			else
			{
				return;
			}
		}
		return;
	}
	case DiscoveryDPInfo::DDSSEC_HANDSHAKE_WAITING_REPLY: {
		if (data.messageData.size() != 1) {
			TRAVODDS_LOG(LOG_ERROR, "Invalid ParticipantStatelessMessage.");
			return;
		}
		if (data.messageClassId == "" || data.messageClassId != GMCLASSID_SECURITY_AUTH_HANDSHAKE) {
			TRAVODDS_LOG(LOG_ERROR, "Invalid MessageClassId.");
			return;
		}
		HandshakeMessageToken handshakeToken;
		ValidationResult_t validationRst =
			dp->secPlugin_.authentication->process_handshake(
				&handshakeToken,
				&data.messageData[0],
				dpInfo->handshakeHdl_,
				&secExce);
		std::this_thread::sleep_for(std::chrono::milliseconds(200)); //考虑时序性
		if (validationRst == VALIDATION_OK_FINAL_MESSAGE) {
			dpInfo->handshakeMsg_->messageData[0] = handshakeToken;
			dp->secPlugin_.authentication->return_handshake_token(&handshakeToken, &secExce);
			//请求发发送processhandshake之后的msg，通知对端processhandshake
			printf("begin_handshake_final local %s remote %s.\n",
				this->participant_->GetGuid().prefix.toString().c_str(),
				sourceParticipantGuid.prefix.toString().c_str());
			WriteParticipantStatelessMessageData(&sourceParticipantGuid, *dpInfo->handshakeMsg_); //final
		}
		else {
			if (dp->secAttributes_.allow_unauthenticated_participants) {
				dpInfo->enableDDSSec_ = false;
				dp->secPlugin_.authentication->return_handshake_token(&handshakeToken, &secExce);
				dpInfo->handshakeMsg_->messageClassId = "";
				//TOCHECK
				SPDPdiscoveredParticipantData partData;
				BuiltinTopicKey_t partKet = Conversion::ToBuiltinTopicKey(sourceParticipantGuid);
				int ret = spdp_->GetParticipantData(partKet, partData);
				if (ret != 0)
				{
					TRAVODDS_LOG(LOG_ERROR, "failed to get match partData.");
					return;
				}
				AddBuiltinSedpProxy(partData);
				return;
			}
			else {
				dp->secPlugin_.authentication->return_handshake_token(&handshakeToken, &secExce);
				return;
			}
		}
		// handshake finished
		dpInfo->endpointState_ = DiscoveryDPInfo::DDSSEC_HANDSHAKE_OK;
		break;
	}
	case DiscoveryDPInfo::DDSSEC_HANDSHAKE_WAITING_FINAL: {
		if (data.messageData.size() != 1) {
			return;
		}
		if (data.messageClassId != GMCLASSID_SECURITY_AUTH_HANDSHAKE) {
			TRAVODDS_LOG(LOG_ERROR, "Invalid MessagageClassId.");
			return;
		}
		ValidationResult_t validationRst =
			dp->secPlugin_.authentication->process_handshake(
				&dpInfo->handshakeMsg_->messageData[0],
				&data.messageData[0],
				dpInfo->handshakeHdl_,
				&secExce);
		if (validationRst == VALIDATION_PENDING_RETRY) {
			TRAVODDS_LOG(LOG_INFO, "waiting final and receive final, process_handshake into retry.");
			return;
		}
		else if (validationRst != VALIDATION_OK) {
			TRAVODDS_LOG(LOG_INFO, "waiting final and receive final, process_handshake failed.");
			if (dp->secAttributes_.allow_unauthenticated_participants) {
				dpInfo->enableDDSSec_ = false;
				dpInfo->handshakeMsg_->messageClassId = "";
				//TOCHECK
				SPDPdiscoveredParticipantData partData;
				BuiltinTopicKey_t partKet = Conversion::ToBuiltinTopicKey(sourceParticipantGuid);
				int ret = spdp_->GetParticipantData(partKet, partData);
				if (ret != 0)
				{
					TRAVODDS_LOG(LOG_ERROR, "failed to get match partData.");
					return;
				}
				AddBuiltinSedpProxy(partData);
			}
			else {
				return;
			}
		}
		// handshake finished
		dpInfo->endpointState_ = DiscoveryDPInfo::DDSSEC_HANDSHAKE_OK;
		//printf("process_handshake local %s remote %s.\n",
		//	this->participant_->GetGuid().prefix.toString().c_str(),
		//	sourceParticipantGuid.prefix.toString().c_str());
		break;
	}
	case DiscoveryDPInfo::DDSSEC_WAITING_CRYPTO_TOKEN: {
		if (dp->secPlugin_.crypto_key_exchange == NULL) {
			return;
		}
		if (data.messageClassId != GMCLASSID_SECURITY_PARTICIPANT_CRYPTO_TOKENS) {
			if (data.messageClassId == GMCLASSID_SECURITY_AUTH_HANDSHAKE) {
				WriteParticipantStatelessMessageData(&sourceParticipantGuid, *dpInfo->handshakeMsg_);
				return;
			}
			TRAVODDS_LOG(LOG_ERROR, "Invalid MessageClassId.");
			return;
		}
		if (!dp->secPlugin_.crypto_key_exchange->set_remote_participant_crypto_tokens(
			dp->cryptoHdl_,
			dpInfo->cryptoHdl_,
			&data.messageData,
			&secExce))
		{
			TRAVODDS_LOG(LOG_ERROR, "set_remote_participant_crypto_tokens on Participant failed.");
			return;
		}
		dpInfo->endpointState_ = DiscoveryDPInfo::DDSSEC_OK;
		break;
	}
	case DiscoveryDPInfo::DDSSEC_HANDSHAKE_OK: {
		if (data.messageClassId == GMCLASSID_SECURITY_AUTH_HANDSHAKE)
		{
			WriteParticipantStatelessMessageData(&sourceParticipantGuid, *dpInfo->handshakeMsg_);
			return;
		}
		return;
	}
	case DiscoveryDPInfo::DDSSEC_OK: {
		if (data.messageClassId == GMCLASSID_SECURITY_DATAWRITER_CRYPTO_TOKENS) {
			//TODO
			SetRemoteDomainEntityCryptoTokens(&data, false);
		}
		else if (data.messageClassId == GMCLASSID_SECURITY_DATAREADER_CRYPTO_TOKENS) {
			//TODO
			SetRemoteDomainEntityCryptoTokens(&data, true);
		}
		else if (data.messageClassId == GMCLASSID_SECURITY_AUTH_HANDSHAKE) {
			//TODO
			WriteParticipantStatelessMessageData(&sourceParticipantGuid, *dpInfo->handshakeMsg_);
		}
		return;
	}
	default:
	{
		TRAVODDS_LOG(LOG_ERROR, "Unknown Participant handshake state");
		return;
	}
	} //switch end
	int ret = 0;
	// only when handshaking or crypto tokens exchanging finished will be here
	if (dpInfo->endpointState_ == DiscoveryDPInfo::DDSSEC_HANDSHAKE_OK)
	{
		//this->spdp_->RemoveHandShakeRetryTimer(sourceParticipantGuid);
		this->spdp_->RemoveHandShakeMsg(sourceParticipantGuid);
		ret = spdp_->ValidateRemoteParticipant(dpInfo);
		if (ret != 0)
		{
			TRAVODDS_LOG(LOG_ERROR, "failed to get_authenticated_peer_credential_token.");
			return;
		}
		//TODO get_shared_secret创建了shared_secret需要释放
		dpInfo->sharedSecretHdl_ = dp->secPlugin_.authentication->get_shared_secret(dpInfo->handshakeHdl_, &secExce);
		
		if (dpInfo->sharedSecretHdl_ == SEC_NIL_HANDLE)
		{
			TRAVODDS_LOG(LOG_ERROR, "Get shared secret of Participant failed.");
			GuidPrefix_t guidPrefix = Conversion::ToGuid(dpInfo->pData_.ddsParticipantData.key).prefix;
			//spdp_->discParts_.erase(guidPrefix);
			spdp_->RemoveDiscoveryDPInfoFromDiscParts(guidPrefix);
			return;
		}
		//std::cout << __FUNCTION__  << " get_shared_secret " << std::endl;
		// try to get and exchange crypto tokens
		dpInfo->cryptoHdl_ = dp->secPlugin_.crypto_key_factory->register_matched_remote_participant(
			dpInfo->identityHdl_,
			dp->cryptoHdl_,
			dpInfo->sharedSecretHdl_,
			dpInfo->permissionsHdl_,
			&secExce);
		if (dpInfo->cryptoHdl_ == SEC_NIL_HANDLE)
		{
			GuidPrefix_t guidPrefix = Conversion::ToGuid(dpInfo->pData_.ddsParticipantData.key).prefix;
			//spdp_->discParts_.erase(guidPrefix);
			spdp_->RemoveDiscoveryDPInfoFromDiscParts(guidPrefix);
		}

		//TOCHECK 绑定两端PARTICIPANT_VOLATILE_MESSAGE_SECURE相关实体
		SPDPdiscoveredParticipantData partData;
		BuiltinTopicKey_t partKet = Conversion::ToBuiltinTopicKey(sourceParticipantGuid);
		ret = spdp_->GetParticipantData(partKet, partData);
		if (ret != 0)
		{
			TRAVODDS_LOG(LOG_ERROR, "failed to get match partData.");
			return;
		}
		AddVolatileMessageBuiltinSecureSpdpProxy(partData);

		AddBuiltinSedpProxy(partData);

		if (dp->secAttributes_.is_rtps_protected)
		{
			ParticipantVolatileSecureMessage cryptoTokensSample;
			if (!dp->secPlugin_.crypto_key_exchange->create_local_participant_crypto_tokens(
				&cryptoTokensSample.messageData,
				dp->cryptoHdl_,
				dpInfo->cryptoHdl_,
				&secExce)) {
			}
			cryptoTokensSample.messageClassId = GMCLASSID_SECURITY_PARTICIPANT_CRYPTO_TOKENS;
			// send tokens
			memset(cryptoTokensSample.destinationEndpointKey.value, 0, 16);
			memset(cryptoTokensSample.sourceEndpointKey.value, 0, 16);

			std::this_thread::sleep_for(std::chrono::milliseconds(200)); //考虑时序性

			WriteParticipantVolatileSecureMessage(&sourceParticipantGuid, cryptoTokensSample);
			dp->secPlugin_.crypto_key_exchange->return_crypto_tokens(&cryptoTokensSample.messageData, &secExce);
			cryptoTokensSample.messageClassId = "";
			dpInfo->endpointState_ = DiscoveryDPInfo::DDSSEC_WAITING_CRYPTO_TOKEN;
			return;
		}
	}
	dpInfo->endpointState_ = DiscoveryDPInfo::DDSSEC_OK;

	//TOCHECK SedpAssociationBuiltinSecureSedpEntities
	SPDPdiscoveredParticipantData partData;
	BuiltinTopicKey_t partKet = Conversion::ToBuiltinTopicKey(sourceParticipantGuid);
	ret = spdp_->GetParticipantData(partKet, partData);
	if (ret != 0)
	{
		TRAVODDS_LOG(LOG_ERROR, "failed to get match partData.");
		return;
	}
	AddBuiltinSecureSedpProxy(partData);

	dp->secParticipants_.push_back(dpInfo->cryptoHdl_);
	return;
}

uint32_t Sedp::SetRemoteDomainEntityCryptoTokens(const ParticipantVolatileSecureMessage* sample, bool isReader)
{
	GUID_t srcGuid;
	GUID_t destGuid;
	memcpy(&srcGuid, sample->sourceEndpointKey.value, 16);
	memcpy(&destGuid, sample->destinationEndpointKey.value, 16);
	if (isReader) {
		if (destGuid == GUID_UNKNOWN)
		{
			std::map<EntityId_t, std::shared_ptr<Writer>>* writers = participant_->GetWriterMap();
			for (auto wrIter = writers->begin();
				wrIter != writers->end();
				++wrIter)
			{
				DataWriterImpl* dw = wrIter->second->GetRelatedDataWriter();
				dw->SetRemoteReaderCryptoTokens(&srcGuid, &sample->messageData);
			}
		}
		else
		{
			std::map<EntityId_t, std::shared_ptr<Writer>>* writers = participant_->GetWriterMap();
			auto wrIter = writers->find(destGuid.entityId);
			if (wrIter == writers->end())
			{
				return 0;
			}
			DataWriterImpl* dw = wrIter->second->GetRelatedDataWriter();
			dw->SetRemoteReaderCryptoTokens(&srcGuid, &sample->messageData);
		}
	}
	else {
		if (destGuid == GUID_UNKNOWN) {
			std::map<EntityId_t, std::shared_ptr<Reader>>* readers = participant_->GetReaderMap();
			for (auto reIter = readers->begin();
				reIter != readers->end();
				++reIter)
			{
				DataReaderImpl* dr = reIter->second->GetRelatedDataReader();
				dr->SetRemoteWriterCryptoTokens(&srcGuid, &sample->messageData);
			}
		}
		else {
			std::map<EntityId_t, std::shared_ptr<Reader>>* readers = participant_->GetReaderMap();
			auto reIter = readers->find(destGuid.entityId);
			if (reIter == readers->end())
			{
				return 0;
			}
			DataReaderImpl* dr = reIter->second->GetRelatedDataReader();
			dr->SetRemoteWriterCryptoTokens(&srcGuid, &sample->messageData);
		}
	}
	return 0;
}
#endif // DDSSECURITY
