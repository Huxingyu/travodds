#include "rtps/discovery/sedp.h"
#include "rtps/discovery/spdp.h"
#include "rtps/messages/parameter.h"
#include "dcps/domain/domainparticipant.h"
#include "common/log/logger.h"
USING_AGIDDS_NAMESPACE;

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
}

void Sedp::SetPublicationEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader)
{
	DataWriterQos writerQos;
	dataWriter->get_qos(writerQos);
    std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter, writerQos, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER, true);
	if (nullptr == writer) {
		//TODO 日志记录
		AGILOG(AGILOG_ERROR, "writer is nullptr");
		return;
	}
	publicationsWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);

	DataReaderQos readerQos;
	dataReader->get_qos(readerQos);
    std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader, readerQos, ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER,true);
	if (nullptr == reader) {	
		//TODO 日志记录
		AGILOG(AGILOG_ERROR, "reader is nullptr");
		return;
	}
	publicationsReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);

	reader->SetReaderListener(this);
}

void Sedp::SetSubscriptionEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader)
{
	DataWriterQos writerQos;
	dataWriter->get_qos(writerQos);
    std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter, writerQos, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER, true);
	if (nullptr == writer) {
		//TODO 日志记录
		AGILOG(AGILOG_ERROR, "writer is nullptr");
		return;
	}
	subscriptionsWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);

	DataReaderQos readerQos;
	dataReader->get_qos(readerQos);
    std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader, readerQos, ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER, true);
	if (nullptr == reader) {
		//TODO 日志记录
		AGILOG(AGILOG_ERROR, "reader is nullptr");
		return;
	}
	subscriptionsReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);

	reader->SetReaderListener(this);
}

void Sedp::AddSedpProxy(const SPDPdiscoveredParticipantData& data)
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
}

void Sedp::on_data_available(std::shared_ptr<CacheChange> change)
{
	if (change->kind >= ALIVE_FILTERED) {
		GUID_t guid = Conversion::ToGuid(change->instanceHandle);

		octet flag = change->kind;

		// D 已处理
		if (flag | 0x01) {

		}
		// U 未注册
		if (flag | 0x02) {
			if(change->writerGuid.entityId == ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER)
				participant_->RemoveRemoteWriter(guid);
			if (change->writerGuid.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER)
				participant_->RemoveRemoteReader(guid);
		}

		// F 已过滤
		if (flag | 0x03) {

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

		if (change->writerGuid.entityId == ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER) {
			DiscoveredReaderData  data;

			TakeDiscoveredReaderData(data, change->serializedBuffer);

			ReceiveDiscoveredReaderData(data);
		}

	}
}

void Sedp::WritePublicationData(DiscoveredWriterData& data)
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
	memcpy(handle.keyHash.value, data.ddsPublicationData.key.value, 16);

	std::shared_ptr<CacheChange> change = publicationsWriter_->NewChange(ALIVE, buffer, handle, cur_time);

	ReturnCode_t ret = publicationsWriter_->AddBuiltinCache(change);

	if(RETCODE_OK != ret) {
		//TODO 记录日志
		AGILOG(AGILOG_ERROR, "publicationsWriter AddBuiltinCache Err ret = %d", ret);
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
	memcpy(handle.keyHash.value, data.ddsSubscriptionData.key.value, 16);
	std::shared_ptr<CacheChange> change = subscriptionsWriter_->NewChange(ALIVE, buffer, handle, cur_time);

	ReturnCode_t ret = subscriptionsWriter_->AddBuiltinCache(change);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
		AGILOG(AGILOG_ERROR, "subscriptionsWriter_ AddBuiltinCache Err ret = %d", ret);
	}
}

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
	keyParam.length = 16;
	keyParam.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + 16);

	char status[4] = { 0 };
	status[3] = 3;
	statueParam.parameterId = PID_STATUS_INFO;
	statueParam.length = 4;
	statueParam.value.assign(status, status + 4);

	change->inlineQos.push_back(keyParam);
	change->inlineQos.push_back(statueParam);

	ReturnCode_t ret = publicationsWriter_->AddBuiltinCache(change);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
		AGILOG(AGILOG_ERROR, "publicationsWriter AddBuiltinCache Err ret = %d", ret);
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
	keyParam.length = 16;
	keyParam.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + 16);

	char status[4] = { 0 };
	status[3] = 3;
	statueParam.parameterId = PID_STATUS_INFO;
	statueParam.length = 4;
	statueParam.value.assign(status, status + 4);

	change->inlineQos.push_back(keyParam);
	change->inlineQos.push_back(statueParam);

	ReturnCode_t ret = subscriptionsWriter_->AddBuiltinCache(change);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
		AGILOG(AGILOG_ERROR, "subscriptionsWriter_ AddBuiltinCache Err ret = %d", ret);
	}
	
	subscriptionsWriter_->RemoveCacheByHandle(instanceHandle);
}

void Sedp::TakeDiscoveredWriterData(DiscoveredWriterData& data, SerializedBuffer* buffer)
{
	ParameterList list((octet*)(buffer->buffer), buffer->buffer_size);
	list.GetDiscoveredData(data);

	data.remoteWriterGuid = Conversion::ToGuid(data.ddsPublicationData.key);
}

void Sedp::ReceiveDiscoveredWriterData(DiscoveredWriterData& data)
{

	SPDPdiscoveredParticipantData participantData;
	if (RETCODE_OK == spdp_->GetParticipantData(data.ddsPublicationData.participant_key, participantData)) {
		data.unicastLocatorList = participantData.defaultUnicastLocatorList;
	}

	participant_->AddRemoteWriter(data);
}

void Sedp::TakeDiscoveredReaderData(DiscoveredReaderData& data, SerializedBuffer* buffer)
{
	ParameterList list((octet*)(buffer->buffer), buffer->buffer_size);
	list.GetDiscoveredData(data);

	data.remoteReaderGuid = Conversion::ToGuid(data.ddsSubscriptionData.key);
}

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
