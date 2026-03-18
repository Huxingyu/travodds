#include <rtps/builtin/liveliness/livelinessmanager.h>
#include <rtps/historycache/historycache.h>
#include "rtps/discovery/discovery.h"
#include "common/timeconversion.h"
#include "dcps/base/statusmanager.h"
#include "common/time/ddstimer.h"
#include "common/log/logger.h"
#include <algorithm>
USING_TRAVODDS_NAMESPACE

LivelinessManager::LivelinessManager(Participant* participant):participant_(participant), automaticReaders_(false)
{
	GUID_t tmpGuid = participant_->GetGuid();
	memset(&tmpGuid.entityId.entityKey, 0, sizeof(OctetArray3));
	tmpGuid.entityId.entityKind = 0;
	autoHandle_ = Conversion::ToInstanceHandle(tmpGuid);
	manualHandle_ = autoHandle_;
	autoHandle_.keyHash.value[15] = AUTOMATIC_LIVELINESS_QOS + octet(1);
	manualHandle_.keyHash.value[15] = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS + octet(1);

	/* 发布者Liveliness管理 */
	localWriterLivelinessManager_ = std::make_shared<LivelinessAssert>(this, true);
	/* 订阅者Liveliness管理 */
	remoteWriterLivelinessManager_ = std::make_shared<LivelinessAssert>(this,false);
}

LivelinessManager::~LivelinessManager()
{
	if (autoTimer_) {
		autoTimer_->Exit();
	}
	if (manualTimer_) {
		manualTimer_->Exit();
	}

	if (ParticipantMessageDataWriter_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(ParticipantMessageDataWriter_->GetGuid());
		participant_->DeleteWriter(handle);
	}
	if (ParticipantMessageDataReader_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(ParticipantMessageDataReader_->GetGuid());
		participant_->DeleteReader(handle);
	}
#ifdef DDSSECURITY
    if (ParticipantMessageSecureDataWriter_) {
        InstanceHandle_t handle = Conversion::ToInstanceHandle(ParticipantMessageSecureDataWriter_->GetGuid());
        participant_->DeleteWriter(handle);
    }
    if (ParticipantMessageSecureDataReader_) {
        InstanceHandle_t handle = Conversion::ToInstanceHandle(ParticipantMessageSecureDataReader_->GetGuid());
        participant_->DeleteReader(handle);
    }
#endif // DDSSECURITY
}

void LivelinessManager::AddParticipantMessageProxy(const SPDPdiscoveredParticipantData & data)
{
	DiscoveredWriterData writerData;
	writerData.remoteWriterGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	writerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	writerData.multicastLocatorList = data.metatrafficMulticastLocatorList;

	writerData.remoteWriterGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
#ifdef DDSSECURITY
	writerData.ddsPublicationData = ParticipantMessageDataWriter_->GetRelatedDataWriter()->GetBuiltinTopicSecureData();
#else
	writerData.ddsPublicationData = ParticipantMessageDataWriter_->GetRelatedDataWriter()->GetBuiltinTopicData();
#endif //DDSSECURITY
	participant_->AddRemoteWriter(writerData);

	DiscoveredReaderData readerData;
	readerData.remoteReaderGuid = Conversion::ToGuid(data.ddsParticipantData.key);
	readerData.unicastLocatorList = data.metatrafficUnicastLocatorList;
	readerData.multicastLocatorList = data.metatrafficMulticastLocatorList;

	readerData.remoteReaderGuid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
#ifdef DDSSECURITY
	readerData.ddsSubscriptionData = ParticipantMessageDataReader_->GetRelatedDataReader()->GetBuiltinTopicSecureData();
#else
	readerData.ddsSubscriptionData = ParticipantMessageDataReader_->GetRelatedDataReader()->GetBuiltinTopicData();
#endif //DDSSECURITY
	participant_->AddRemoteReader(readerData);
}

void LivelinessManager::RemoveParticipantMessageProxy(GUID_t guid)
{
	guid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER;
	participant_->RemoveRemoteReader(guid);

	guid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER;
	participant_->RemoveRemoteWriter(guid);
#ifdef DDSSECURITY
	//ParticipantMessageSecure扩展
	guid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER;
	participant_->RemoveRemoteReader(guid);

	guid.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER;
	participant_->RemoveRemoteWriter(guid);
#endif // DDSSECURITY	
}


/* wnagyi LivelinessQos */
bool LivelinessManager::AddLivelinessWriter(std::shared_ptr<Writer> writerPtr, const LivelinessQosPolicy & qos)
{
	std::unique_lock<std::mutex> lock(writerMutex_);

	if (qos.kind == AUTOMATIC_LIVELINESS_QOS) {

		automaticWriters_.emplace(writerPtr->GetGuid(), writerPtr);

		SetTimer(qos);

	}
	else if (qos.kind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS) {

		manualByParticipantWriters_.emplace(writerPtr->GetGuid(),writerPtr);

		SetTimer(qos);

		if (!localWriterLivelinessManager_->AddWriter(LivelinesStatus(writerPtr->GetGuid(),qos))) {
			/* TODO 日志 */
			TRAVODDS_LOG(LOG_ERROR, "MANUAL_BY_PARTICIPANT_LIVELINESS_QOS AddWriter Err");
		}
	}
	else if (qos.kind == MANUAL_BY_TOPIC_LIVELINESS_QOS) {

		manualByTopicWriters_.emplace(writerPtr->GetGuid(),writerPtr);

		if (!localWriterLivelinessManager_->AddWriter(LivelinesStatus(writerPtr->GetGuid(),qos))) {
			/* TODO 日志 */
			TRAVODDS_LOG(LOG_ERROR, "MANUAL_BY_TOPIC_LIVELINESS_QOS AddWriter Err");
		}
	}

	return true;
}

bool LivelinessManager::RemoveLivelinessWriter(std::shared_ptr<Writer>  writerPtr)
{
	std::unique_lock<std::mutex> lock(writerMutex_);
	/* TODO 日志 */
	//AGILOG(AGILOG_ERROR, "RemoveLivelinessWriter begin");

	if (writerPtr->GetQos().liveliness.kind == AUTOMATIC_LIVELINESS_QOS) {

		if (!RemoveWriterMap(writerPtr->GetGuid(), automaticWriters_)) {
			/* TODO 日志 */
			TRAVODDS_LOG(LOG_ERROR, "AUTOMATIC RemoveWriterMap Err");
			return false;
		}

		return UpdateTimer(AUTOMATIC_LIVELINESS_QOS);
	}
	else if (writerPtr->GetQos().liveliness.kind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS) {

		if (!RemoveWriterMap(writerPtr->GetGuid(), manualByParticipantWriters_)) {
			/* TODO 日志 */
			TRAVODDS_LOG(LOG_ERROR, "MANUAL_BY_PARTICIPANT_LIVELINESS_QOS RemoveWriterMap Err");
			return false;
		}

		if (!localWriterLivelinessManager_->RemoveWriter(writerPtr->GetGuid())) {
			/* TODO 日志 */
			TRAVODDS_LOG(LOG_ERROR, "MANUAL_BY_PARTICIPANT_LIVELINESS_QOS RemoveWriter Err");
		}

		return UpdateTimer(MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
	}
	else if (writerPtr->GetQos().liveliness.kind == MANUAL_BY_TOPIC_LIVELINESS_QOS) {

		if (!RemoveWriterMap(writerPtr->GetGuid(), manualByTopicWriters_)){
			/* TODO 日志 */
			TRAVODDS_LOG(LOG_ERROR, "MANUAL_BY_TOPIC_LIVELINESS_QOS RemoveWriterMap Err");
			return false;
		}

		if (!localWriterLivelinessManager_->RemoveWriter(writerPtr->GetGuid())) {
			/* TODO 日志 */
			TRAVODDS_LOG(LOG_ERROR, "MANUAL_BY_TOPIC_LIVELINESS_QOS RemoveWriter Err");
		}
		return true;
	}

	/* TODO 日志 */
	//AGILOG(AGILOG_ERROR, "RemoveLivelinessWriter Err");
	return false;
}

void LivelinessManager::SetTimer(const LivelinessQosPolicy & qos)
{
	/* 自动发送和检测时间获取 */
	double milliSec(Conversion::ToMilliSecondsDouble(qos.lease_duration) * 0.5);
	Time_t periodTime = Conversion::MilliSecondsToTime(milliSec);

	std::shared_ptr<Timer> timer;
	std::map<GUID_t, std::shared_ptr<Writer>> writers;

	if (AUTOMATIC_LIVELINESS_QOS == qos.kind && nullptr == autoTimer_) {
		autoTimer_ = TimerManager::CreateTimer(periodTime, [&]() -> bool {AutoLivelinessChecker(); return true; }, "autoLiveliness");
	}
	else if (MANUAL_BY_PARTICIPANT_LIVELINESS_QOS == qos.kind && nullptr == manualTimer_) {

		manualTimer_ = TimerManager::CreateTimer(periodTime, [&]() -> bool {ParticipantLivelinessChecker(); return true; }, "manuLiveliness");
	}
	else
	{
		UpdateTimer(qos.kind);
	}

}

bool LivelinessManager::RemoveWriterMap(GUID_t guid, std::map<GUID_t, std::shared_ptr<Writer>>& writers)
{
	auto it = writers.find(guid);

	if (it == writers.end()) {
		TRAVODDS_LOG(LOG_ERROR, "guid is not exist");
		return false;
	}

	writers.erase(it);
	return true;
}

bool LivelinessManager::UpdateTimer(LivelinessQosPolicyKind kind)
{
	std::shared_ptr<Timer> timer;
	std::map<GUID_t, std::shared_ptr<Writer>> writers;

	if (AUTOMATIC_LIVELINESS_QOS == kind) {
		timer = autoTimer_;
		writers = automaticWriters_;
	}
	else if (MANUAL_BY_PARTICIPANT_LIVELINESS_QOS == kind)
	{
		timer = manualTimer_;
		writers = manualByParticipantWriters_;
	}

	Duration_t duration = TIME_INFINITE;

	if (writers.empty()) {
		timer->Stop();
		return true;
	}
	// 重新获取最小发送报文时间间隔
	for (auto it = writers.begin(); it != writers.end(); it++) {
		auto period = it->second->GetQos().liveliness.lease_duration;
		if (duration > period) {
			duration = period;
		}
	}

	timer->UpdatePeriod(Conversion::ToMilliSecondsDouble(duration) * 0.5);

	return true;
}

bool LivelinessManager::AddLivelinessReader(std::shared_ptr<Reader> readerPtr, const LivelinessQosPolicy & qos)
{
	std::unique_lock<std::mutex> lock(writerMutex_);

	if (qos.kind == AUTOMATIC_LIVELINESS_QOS) {
		automaticReaders_ = true;
	}

	readers_.emplace(readerPtr->GetGuid(),(readerPtr));

	return true;
}

bool LivelinessManager::RemoveLivelinessReader(std::shared_ptr<Reader> readerPtr)
{
	auto it = readers_.find(readerPtr->GetGuid());
	if (it != readers_.end()) {
		readers_.erase(it);
		return true;
	}

	/* TODO 日志 */
	TRAVODDS_LOG(LOG_ERROR, "guid is not exist");
	return false;
}


bool LivelinessManager::AutoLivelinessChecker()
{
	std::unique_lock<std::mutex> lock(writerMutex_);

	if (0 < automaticWriters_.size()) {

		return SendLivelinessMessage(autoHandle_);
	}

	return true;
}


bool LivelinessManager::ParticipantLivelinessChecker()
{
	std::unique_lock<std::mutex> lock(writerMutex_);

	if (0 < manualByParticipantWriters_.size()) {

		if (localWriterLivelinessManager_->HasAliveWriters(MANUAL_BY_PARTICIPANT_LIVELINESS_QOS)) {

 			return SendLivelinessMessage(manualHandle_);
		}
	}
	//AGILOG(AGILOG_ERROR, "manualByParticipantWriters_.size() <= 0", manualByParticipantWriters_.size());
	return false;
}

bool LivelinessManager::SendLivelinessMessage(const InstanceHandle_t& instance)
{
	SerializedBuffer *buffer = new SerializedBuffer;
	octet data[28] = { 0 };
	uint16_t size = 0;
	Time_t time;
	DomainParticipantImpl::get_current_time(time);

	/* 序列化 */
	data[1] = PL_DEFAULT_ENDIAN;
	size += CDR_ENDIANNESS_LEN;
	memcpy(data + size, instance.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	size += sizeof(InstanceHandle_t);
	buffer->buffer_size = size;
	
	if (!MemoryPool::GetInstance()->getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->buffer)) {
		return false;
	}
	buffer->buffer = buffer->buffer + BUFF_SHIFT;
	memcpy(buffer->buffer, data, sizeof(data));
	
	std::shared_ptr<CacheChange> cacheChange = ParticipantMessageDataWriter_->NewChange(ChangeKind_t::ALIVE, buffer, instance, time);
	if (nullptr == cacheChange) {
		TRAVODDS_LOG(LOG_ERROR, "create newChange Err");
		return false;
	}

	ParticipantMessageDataWriter_->AddNewCachce(cacheChange);
	return true;
}

void LivelinessManager::SetParticipantMessageDataWriter(std::shared_ptr<DataWriterImpl> dataWriter)
{
	DataWriterQos writerQos;
	dataWriter->get_qos(writerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER,true, ENTITY_WRITER);
    std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter.get(), writerQos, guid, true);
	ParticipantMessageDataWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);
}

void LivelinessManager::SetParticipantMessageDataReader(std::shared_ptr<DataReaderImpl> dataReader)
{
	DataReaderQos readerQos;
	dataReader->get_qos(readerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER, true, ENTITY_READER);
	std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader.get(), readerQos, guid, true);
	ParticipantMessageDataReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);

	reader->SetReaderListener(this);
}

#ifdef DDSSECURITY
void LivelinessManager::SetParticipantMessageSecureDataWriter(std::shared_ptr<DataWriterImpl> dataWriter)
{
    DataWriterQos writerQos;
    dataWriter->get_qos(writerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER, true, ENTITY_WRITER);
    std::shared_ptr<Writer> writer = participant_->CreateWriter(dataWriter.get(), writerQos, guid, true);
    ParticipantMessageSecureDataWriter_ = std::dynamic_pointer_cast<StatefulWriter>(writer);
}

void LivelinessManager::SetParticipantMessageSecureDataReader(std::shared_ptr<DataReaderImpl> dataReader)
{
    DataReaderQos readerQos;
    dataReader->get_qos(readerQos);
	GUID_t guid = participant_->GenerateGuid(ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER, true, ENTITY_READER);
    std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader.get(), readerQos, guid, true);
    ParticipantMessageSecureDataReader_ = std::dynamic_pointer_cast<StatefulReader>(reader);

    reader->SetReaderListener(this);
}
#endif // DDSSECURITY
bool LivelinessManager::AssertLiveliness(GUID_t guid, bool localWriter, LivelinessQosPolicyKind kind)
{
	if (true == localWriter) {
		return localWriterLivelinessManager_->AssertLiveliness(guid, kind);
	}
	else {
		return remoteWriterLivelinessManager_->AssertLiveliness(guid, kind);
	}	
}

//bool LivelinessManager::AssertLivelinessManualByParticipant()
//{
//	if (manualByParticipantWriters_.size() > 0) {
//		return localWriterLivelinessManager_->AssertLiveliness(MANUAL_BY_PARTICIPANT_LIVELINESS_QOS, participant_->GetGuid());
//	}
//	return false;
//}

void LivelinessManager::LiveLinessStatusChanged(const GUID_t& writerGuid, bool localWriter,const LivelinessQosPolicy& livelinessQos, WriterStatus status)
{
	if (localWriter) {
		LocalStatusChanged(writerGuid, livelinessQos, status);
	}
	else
	{
		RemoteStatusChanged(writerGuid, livelinessQos, status);
	}

}

void LivelinessManager::LocalStatusChanged(const GUID_t & writer, const LivelinessQosPolicy & livelinessQos, WriterStatus status)
{
	if (WriterStatus::LIVELINESSALIVE != status) {
		return;
	}
		
	if (livelinessQos.kind == AUTOMATIC_LIVELINESS_QOS) {
		UpdateLivelinessLostStatus(writer, automaticWriters_);
	}
	else if (livelinessQos.kind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS) {
		UpdateLivelinessLostStatus(writer, manualByParticipantWriters_);
	}
	else if (livelinessQos.kind == MANUAL_BY_TOPIC_LIVELINESS_QOS) {
		UpdateLivelinessLostStatus(writer, manualByTopicWriters_);
	}
}

void LivelinessManager::UpdateLivelinessLostStatus(const GUID_t& writerGuid, std::map<GUID_t, std::shared_ptr<Writer>> writers)
{
	auto writer = writers.find(writerGuid);
	if (writer != writers.end()) {
		writer->second->GetRelatedDataWriter()->UpdateLivelinessLostStatus();
	}
}

void LivelinessManager::RemoteStatusChanged(const GUID_t& writerGuid, const LivelinessQosPolicy& livelinessQos, WriterStatus status)
{
	static_cast<void>(livelinessQos);
	bool alive, ensure;

	if (status == WriterStatus::LIVELINESSALIVE){
		alive = false;
		ensure = false;
	}
	else if (status == WriterStatus::NOTALIVE) {
		alive = true;
		ensure = false;
	}
	else if (status == WriterStatus::NOTASSERTED) {
		alive = true;
		ensure = true;
	}

	for (auto it = readers_.begin(); it != readers_.end(); it++) {
		if (it->second->MatchedWriterIsMatched(writerGuid)) {

			it->second->GetRelatedDataReader()->UpdateLivelinessChangedStatus(Conversion::ToInstanceHandle(writerGuid), alive, ensure);
		}
	}
}

std::shared_ptr<LivelinessAssert> LivelinessManager::GetLocalWriterLivelinessManager()
{
	return localWriterLivelinessManager_;
}

std::shared_ptr<LivelinessAssert> LivelinessManager::GetRemoteWriterLivelinessManager()
{
	return remoteWriterLivelinessManager_;
}

std::shared_ptr<StatefulReader> LivelinessManager::GetParticipantDataReader()
{
	return ParticipantMessageDataReader_;
}

void LivelinessManager::on_data_available(std::shared_ptr<CacheChange> change)
{
	if (change->serializedBuffer) {

		GUID_t guid;
		LivelinessQosPolicyKind livelinessKind;

		memcpy(change->instanceHandle.keyHash.value, change->serializedBuffer->buffer + 4, MIG_RTPS_KEY_HASH_MAX_LENGTH);

		if (change->serializedBuffer->buffer_size > 0)
		{
			for (size_t i = 0; i < 12; ++i)
			{
				// 解析出Participant的标识guid.prefix
				guid.prefix.value[i] = change->serializedBuffer->buffer[i + 4];
			}
			livelinessKind = (LivelinessQosPolicyKind)(change->serializedBuffer->buffer[19] - octet(1));
		}
		else
		{
			for (uint8_t i = 0; i < 12; ++i)
			{
				guid.prefix.value[i] = change->instanceHandle.keyHash.value[i];
			}
			livelinessKind = (LivelinessQosPolicyKind)change->instanceHandle.keyHash.value[15];
		}

		if (guid.prefix == ParticipantMessageDataReader_->GetGuid().prefix) {
			/* TODO 日志 */
			//AGILOG(AGILOG_ERROR, "CompareGuidPrefix Err");
			return;
		}

		if (automaticReaders_)
		{
			AssertLiveliness(guid,false,AUTOMATIC_LIVELINESS_QOS);
		}

		if (livelinessKind == MANUAL_BY_PARTICIPANT_LIVELINESS_QOS)
		{
			AssertLiveliness(guid, false,MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
		}
	}
	return;
}



