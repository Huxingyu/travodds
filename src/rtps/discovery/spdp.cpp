#include "rtps/discovery/spdp.h"
#include "rtps/discovery/discovery.h"
#include "rtps/messages/parameter.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/publish/publisher.h"
#include "common/timeconversion.h"
#include "common/time/ddstimer.h"

USING_AGIDDS_NAMESPACE;

Spdp::Spdp(Participant* participant, Discovery* discovery)
	: participant_(participant), discovery_(discovery),
	  memoryPool_(MemoryPool::GetInstance()),
	leaseDuration_({ 100,0 })
{
	Time_t time;
	time.sec = leaseDuration_.sec;
	time.nanosec = leaseDuration_.nanosec;
	timer_ = TimerManager::CreateTimer(time, [&]() { this->CheckRemoteParticipantDuration(); }, "CheckRemoteParticipantDuration");
}

Spdp::~Spdp()
{
	if (writer_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(writer_->GetGuid());
		participant_->DeleteWriter(handle);
	}
	if (reader_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(reader_->GetGuid());
		participant_->DeleteReader(handle);
	}
}

void Spdp::SetParticipantDataWriter(std::shared_ptr<DataWriterImpl> dataWriter)
{
	DataWriterQos qos;
	dataWriter->get_qos(qos);

	//TODO 补充resendPeriod参数
    std::shared_ptr < Writer > writer = participant_->CreateWriter(dataWriter.get(), qos, ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER, true);
	if (nullptr == writer) {
		//TODO 日志记录
		return;
	}
	writer_ = std::dynamic_pointer_cast<StatelessWriter>(writer);

	writer_->SetResendDataPeriod(leaseDuration_);

	//TODO 补充locator与ip的装换函数
	Locator_t locator;
	locator.kind = LOCATOR_KIND_UDPv4;
	locator.port = SPDP_WELL_KNOWN_MULTICAST_PORT(dataWriter->get_publisher()->get_participant()->get_domain_id());
	locator.address[12] = 0xEF;
	locator.address[13] = 0xFF;
	locator.address[14] = 0x00;
	locator.address[15] = 0x01;

	writer_->ReaderLocatorAdd(locator);

	// DiscoveredReaderData data;
	// participant_->AddRemoteReader()

	//TODO DataWriter与writer关联
}

void Spdp::SetParticipantDataReader(std::shared_ptr<DataReaderImpl> dataReader)
{
	DataReaderQos qos;
	dataReader->get_qos(qos);
	qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
    std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader.get(), qos, ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER, true);
	if (nullptr == reader) {
		//TODO 日志记录
		return;
	}
	reader_ = std::dynamic_pointer_cast<StatelessReader>(reader);
	reader->SetReaderListener(this);

	//TODO DataReader与reader关联
}

void Spdp::on_data_available(std::shared_ptr<CacheChange> change)
{
	if (change->kind >= ALIVE_FILTERED)
	{
		GUID_t guid = Conversion::ToGuid(change->instanceHandle);

		octet flag = change->kind;

		// D 已处理
		if(flag | 0x01){
			
		}
		// U 未注册
		if (flag | 0x02) {

			discovery_->RemoveProxy(guid);
		}

		// F 已过滤
		if (flag | 0x03) {

		}
		return;
	}

	if (change->serializedBuffer) {

		SPDPdiscoveredParticipantData data;

		TakeSPDPdiscoveredParticipantData(data, change->serializedBuffer);

		ReceiveSPDPdiscoveredParticipantData(data);
	}
}

int Spdp::GetParticipantData(BuiltinTopicKey_t& key, SPDPdiscoveredParticipantData& data)
{
	if (0 == remoteParticipantData_.count(key)) {
		return RETCODE_ERROR;
	}

	data = remoteParticipantData_[key];
	return RETCODE_OK;
}

void Spdp::RemoveParticipant(GUID_t guid)
{
	discovery_->RemoveProxy(guid);
}

void Spdp::WriteSpdPdiscoveredParticipantData(SPDPdiscoveredParticipantData& data)
{
	data.leaseDuration = leaseDuration_;

	SerializedBuffer *buffer = new SerializedBuffer;
	ParameterList list(data);
	uint16_t size = list.GetMessageSize();
	Time_t time;
	DomainParticipantImpl::get_current_time(time);

	buffer->buffer_size = size ;

	memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->origin_buffer);

	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;

	list.WriterToMessage((octet*)buffer->buffer, size);
	InstanceHandle_t handle;
	memcpy(handle.keyHash.value, data.ddsParticipantData.key.value, 16);

	std::shared_ptr<CacheChange> cacheChange = writer_->NewChange(ALIVE, buffer, handle, time);
	if (nullptr == cacheChange) {
		return ;
	}

	ReturnCode_t ret = writer_->AddBuiltinCache(cacheChange);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
	}

}

void Spdp::RemoveSpdPdiscoveredParticipantData(GUID_t & guid)
{
	SerializedBuffer *buffer = new SerializedBuffer;
	buffer->buffer_size = 0;
	if (!memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT * 2, buffer->origin_buffer)) {
	}
	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;

	Time_t cur_time;
	DomainParticipant::get_current_time(cur_time);

	std::shared_ptr<CacheChange> change = writer_->NewChange(NOT_ALIVE_DISPOSED_UNREGISTERED, buffer, HANDLE_NIL, cur_time);

	InstanceHandle_t instanceHandle = Conversion::ToInstanceHandle(guid);
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

	writer_->AddBuiltinCache(change);
}

void Spdp::TakeSPDPdiscoveredParticipantData(SPDPdiscoveredParticipantData& data, SerializedBuffer* buffer)
{
	ParameterList list((octet*)(buffer->buffer), buffer->buffer_size);
	list.GetDiscoveredData(data);
}

void Spdp::ReceiveSPDPdiscoveredParticipantData(const SPDPdiscoveredParticipantData& data)
{

	//TODO 新增节点 添加代理
	if (0 == remoteParticipantData_.count(data.ddsParticipantData.key)) {

		discovery_->AddProxy(data);
	}
	else
	{
		Time_t currentTime;
		DomainParticipantImpl::get_current_time(currentTime);
		remoteParticipantStatus_[data.ddsParticipantData.key].recvTime = Conversion::ToSecondsDouble(currentTime);
		// printf("SPDPD : time %f \n", remoteParticipantStatus_[data.ddsParticipantData.key].recvTime);
	}

	//TODO 已存在节点 在线检测
}


void Spdp::AddSpdpProxy(const SPDPdiscoveredParticipantData& data)
{
	std::lock_guard<std::mutex> lock(mutex_);

	Time_t currentTime;
	DomainParticipantImpl::get_current_time(currentTime);

	remoteParticipantData_[data.ddsParticipantData.key] = data;

	remoteParticipantStatus_[data.ddsParticipantData.key].key = data.ddsParticipantData.key;
	remoteParticipantStatus_[data.ddsParticipantData.key].leaseDuration = Conversion::ToSecondsDouble(data.leaseDuration);
	remoteParticipantStatus_[data.ddsParticipantData.key].recvTime = Conversion::ToSecondsDouble(currentTime);


	printf("SPDP: add remote participant %d ", remoteParticipantData_.size());
	GUID_t guid = Conversion::ToGuid(data.ddsParticipantData.key);
	for (int i = 0; i < 16; i++) {
		printf("%02x ", ((unsigned char*)&guid)[i]);
	}
	printf("\n");

	discovery_->GetThreadPool()->submit([=]() {
		writer_->ResendDataPeriod();
	});

}

void Spdp::RemoveSpdpProxy(const GUID_t& guid)
{
	std::lock_guard<std::mutex> lock(mutex_);

	BuiltinTopicKey_t key = Conversion::ToBuiltinTopicKey(guid);

	if (0 == remoteParticipantStatus_.count(key))
		return;

	remoteParticipantStatus_.erase(key);
	remoteParticipantData_.erase(key);

	printf("SPDP: after remove remote participant %zu ", remoteParticipantData_.size());
	for (int i = 0; i < 16; i++) {

		printf("%02x ", ((unsigned char *)&guid)[i]);
	}
	printf("\n");

	participant_->RemoveRemoteParticipantAllWriter(guid);
	participant_->RemoveRemoteParticipantAllReader(guid);

}

void Spdp::CheckRemoteParticipantDuration()
{
	std::vector<BuiltinTopicKey_t> keys;

	GetOverdueParticipant(keys);

	for(auto key : keys)
	{
		GUID_t guid = Conversion::ToGuid(key);
		discovery_->RemoveProxy(guid);
	}
}

void Spdp::GetOverdueParticipant(std::vector<BuiltinTopicKey_t>& keys)
{
	std::lock_guard<std::mutex> lock(mutex_);

	Time_t currentTime;
	DomainParticipantImpl::get_current_time(currentTime);
	double time = Conversion::ToSecondsDouble(currentTime);
	//遍历检查是否超期，三次检测未收到报文，判断为超期进行移除操作 
	for (auto itor : remoteParticipantStatus_) {

		if (itor.second.recvTime + itor.second.leaseDuration * 3 < time) {
			keys.push_back(itor.first);
			// printf("recvTime %f leaseDuration %f time %f \n", itor.second.recvTime, itor.second.leaseDuration, time);
		}
	}
}

void Spdp::GetDiscoveredParticipants(InstanceHandleSeq & participant_handles)
{
	std::lock_guard<std::mutex> lock(mutex_);

	for (auto & key : remoteParticipantData_) {
		participant_handles.push_back(Conversion::ToInstanceHandle(key.first));
	}
}
