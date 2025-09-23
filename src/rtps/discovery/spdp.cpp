#include <algorithm>
#include "rtps/discovery/spdp.h"
#include "rtps/discovery/discovery.h"
#include "rtps/messages/parameter.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/publish/publisher.h"
#include "common/timeconversion.h"
#include "common/time/ddstimer.h"
#include "common/log/logger.h"
#ifdef DDSSECURITY
#include "security/authentication.h"
#include "security/securitycommon.h"
#include "rtps/messages/parameter.h"
#include <chrono>
#include <thread>
#include <iomanip>
#include <algorithm>
#endif // DDSSECURITY

USING_TRAVODDS_NAMESPACE;

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
	if (timer_) {
		timer_->Exit();
	}

	if (writer_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(writer_->GetGuid());
		participant_->DeleteWriter(handle);
	}
	if (reader_) {
		InstanceHandle_t handle = Conversion::ToInstanceHandle(reader_->GetGuid());
		participant_->DeleteReader(handle);
	}
//#ifdef DDSSECURITY
//	if (bigEndianMsgData_) {
//		if (bigEndianMsgData_->buffer) {
//			delete[] bigEndianMsgData_->buffer;
//		}
//		delete bigEndianMsgData_;
//	}
//#endif // DDSSECURITY
}

void Spdp::SetParticipantDataWriter(std::shared_ptr<DataWriterImpl> dataWriter)
{
	DataWriterQos qos;
	dataWriter->get_qos(qos);

	//TODO 补充resendPeriod参数
	GUID_t guid = participant_->GenerateGuid(ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER, true, ENTITY_WRITER);
    std::shared_ptr < Writer > writer = participant_->CreateWriter(dataWriter.get(), qos, guid, true);
	if (nullptr == writer) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "writer is nullptr");
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

	Locator_t locatorv6;
	locatorv6.kind = LOCATOR_KIND_UDPv6;
	locatorv6.port = SPDP_WELL_KNOWN_MULTICAST_PORT(dataWriter->get_publisher()->get_participant()->get_domain_id());
	locatorv6.address[0] = 0xff;
	locatorv6.address[1] = 0x1e;
	//3-9 : 0
	locatorv6.address[10] = 0xff;
	locatorv6.address[11] = 0xff;
	locatorv6.address[12] = 0xef;
	locatorv6.address[13] = 0xff;
	locatorv6.address[15] = 0x01;

	writer_->ReaderLocatorAdd(locatorv6);

	// DiscoveredReaderData data;
	// participant_->AddRemoteReader()

	//TODO DataWriter与writer关联
}

void Spdp::SetParticipantDataReader(std::shared_ptr<DataReaderImpl> dataReader)
{
	DataReaderQos qos;
	dataReader->get_qos(qos);
	qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
	GUID_t guid = participant_->GenerateGuid(ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER, true, ENTITY_READER);
    std::shared_ptr<Reader> reader = participant_->CreateReader(dataReader.get(), qos, guid, true);
	if (nullptr == reader) {
		//TODO 日志记录
		TRAVODDS_LOG(LOG_ERROR, "reader is nullptr");
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
		if(flag | NOT_ALIVE_DISPOSED){
			
		}
		// U 未注册
		if (flag | NOT_ALIVE_UNREGISTERED) {

			discovery_->RemoveProxy(guid);
		}

		// F 已过滤
		if (flag | ALIVE_FILTERED) {

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
		TRAVODDS_LOG(LOG_ERROR, "key not exist");
		return RETCODE_ERROR;
	}

	data = remoteParticipantData_[key];
	return RETCODE_OK;
}

void Spdp::SetIgnoredParticipants(GUID_t guid)
{
	ignoredParticipantsMutex_.lock();
	ignoredParticipants_.push_back(guid);
	ignoredParticipantsMutex_.unlock();
}

bool Spdp::FindIgnoredParticipants(GUID_t guid)
{
	auto it = std::find(ignoredParticipants_.begin(), ignoredParticipants_.end(), guid);
	if (it == ignoredParticipants_.end())
	{
		return false;
	}
	return true;
}

#ifdef DDSSECURITY
bool Spdp::ValidateParticipantGenericMessage(GUID_t destGuid)
{
	if (Conversion::ToGuid(discovery_->GetParticipantData().ddsParticipantData.key) == GUID_UNKNOWN) {
		return true;
	}
	if (Conversion::ToGuid(discovery_->GetParticipantData().ddsParticipantData.key).prefix == destGuid.prefix) {
		return true;
	}
	return false;
}

int Spdp::ValidateRemoteParticipant(DiscoveryDPInfo* dpInfo)
{
	AuthenticatedPeerCredentialToken authToken;
	SecurityException secExce;
	if (!discovery_->GetParticipant()->secPlugin_.authentication->get_authenticated_peer_credential_token(
		&authToken, dpInfo->handshakeHdl_, &secExce))
	{
		TRAVODDS_LOG(LOG_ERROR, "Get authenticated peer credential token for remote participant failed.");
		return -91;
	}
	dpInfo->permissionsHdl_ = discovery_->GetParticipant()->secPlugin_.access_control->validate_remote_permissions(
		discovery_->GetParticipant()->secPlugin_.authentication,
		discovery_->GetParticipant()->identityHdl_,
		dpInfo->identityHdl_,
		&dpInfo->pData_.ddsParticipantData.permissions_token,
		&authToken,
		&secExce);
	discovery_->GetParticipant()->secPlugin_.authentication->return_authenticated_peer_credential_token(&authToken, &secExce);
	if (dpInfo->permissionsHdl_ == SEC_NIL_HANDLE) {
		TRAVODDS_LOG(LOG_ERROR, "Validate remote participant permissions failed.");
		return -92;
	}
	if (!discovery_->GetParticipant()->secPlugin_.access_control->check_remote_participant(
		dpInfo->permissionsHdl_,
		discovery_->GetParticipant()->GetDomainId(),
		&dpInfo->pData_.ddsParticipantData,
		&secExce))
	{
		TRAVODDS_LOG(LOG_ERROR, "Check remote participant failed.");
		return -93;
	}
	return 0;
}

Discovery* Spdp::GetDiscovery() {
	return discovery_;
}

int Spdp::SpdpCleanupRemoteSecRsc(DiscoveryDPInfo* dpInfo) {
	if (discovery_->GetParticipant()->enableDDSSec_) {
		DomainParticipantImpl* relatedDp = discovery_->GetParticipant();
		SecurityException secExce;
		relatedDp->secPlugin_.access_control->return_permissions_handle(
			dpInfo->permissionsHdl_, &secExce);
		dpInfo->permissionsHdl_ = SEC_NIL_HANDLE;
		relatedDp->secPlugin_.authentication->return_identity_handle(
			dpInfo->identityHdl_, &secExce);
		dpInfo->identityHdl_ = SEC_NIL_HANDLE;
		relatedDp->secPlugin_.authentication->return_sharedsecret_handle(
			dpInfo->sharedSecretHdl_, &secExce);
		dpInfo->sharedSecretHdl_ = SEC_NIL_HANDLE;
		relatedDp->secPlugin_.authentication->return_handshake_handle(
			dpInfo->handshakeHdl_, &secExce);
		dpInfo->handshakeHdl_ = SEC_NIL_HANDLE;
		for (DomainParticipantImpl::SecParticipantCryptoHandleList::iterator iter = relatedDp->secParticipants_.begin();
			iter != relatedDp->secParticipants_.end();
			++iter)
		{
			if (*iter == dpInfo->cryptoHdl_)
			{
				relatedDp->secParticipants_.erase(iter);
				break;
			}
		}
		relatedDp->secPlugin_.crypto_key_factory->unregister_participant(
			dpInfo->cryptoHdl_, &secExce);
		dpInfo->cryptoHdl_ = SEC_NIL_HANDLE;
	}
	// dpInfo->m_pData.ddsParticipantData的identity_token permissions_token回收
	if (dpInfo->handshakeMsg_ != NULL) {
		delete dpInfo->handshakeMsg_;
		dpInfo->handshakeMsg_ = NULL;
	}
	return 0;
}

void Spdp::RemoveDiscoveryDPInfoFromDiscParts(const GuidPrefix_t& prefix)
{
	if (!discParts_.count(prefix)) {
		return;
	}
	delete discParts_[prefix];
	discParts_.erase(prefix);
}

ParticipantCryptoHandle Spdp::GetRemoteParticipantCryptoHandle(const GuidPrefix_t* remotePrefix)
{
	auto dpInfoIter = discParts_.find(*remotePrefix);
	if (dpInfoIter == discParts_.end())
	{
		//TRAVODDS_LOG(LOG_ERROR, "GetRemoteParticipantCryptoHandle is nullptr");
		return SEC_NIL_HANDLE;
	}
	if (dpInfoIter->second->enableDDSSec_)
	{
		return dpInfoIter->second->cryptoHdl_;
	}
	return SEC_NIL_HANDLE;
}

SharedSecretHandle Spdp::GetRemoteParticipantSharedSecret(const GuidPrefix_t* remotePrefix)
{
	auto dpInfoIter = discParts_.find(*remotePrefix);
	if (dpInfoIter == discParts_.end())
	{
		TRAVODDS_LOG(LOG_ERROR, "GetRemoteParticipantSharedSecret is nullptr");
		return SEC_NIL_HANDLE;
	}
	if (dpInfoIter->second->enableDDSSec_)
	{
		return dpInfoIter->second->sharedSecretHdl_;
	}
	return SEC_NIL_HANDLE;
}
#endif // DDSSECURITY

void Spdp::RemoveParticipant(GUID_t guid)
{
	discovery_->RemoveProxy(guid);
}

void Spdp::WriteSpdPdiscoveredParticipantData(SPDPdiscoveredParticipantData& data)
{
	data.leaseDuration = leaseDuration_;

	SerializedBuffer *buffer = new SerializedBuffer;
#ifdef DDSSECURITY
	bigEndianMsgData_ = buffer;
#endif // DDSSECURITY
	ParameterList list(data);
	uint16_t size = list.GetMessageSize();
	Time_t time;
	DomainParticipantImpl::get_current_time(time);

	buffer->buffer_size = size ;

	memoryPool_->getBuffer(buffer->buffer_size + BUFF_SHIFT, buffer->origin_buffer);

	buffer->buffer = buffer->origin_buffer + BUFF_SHIFT;

	list.WriterToMessage((octet*)buffer->buffer, size);
	InstanceHandle_t handle;
	memcpy(handle.keyHash.value, data.ddsParticipantData.key.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);

	std::shared_ptr<CacheChange> cacheChange = writer_->NewChange(ALIVE, buffer, handle, time);
	if (nullptr == cacheChange) {
		TRAVODDS_LOG(LOG_ERROR, "create newChange Err");
		return ;
	}

	ReturnCode_t ret = writer_->AddBuiltinCache(cacheChange);

	if (RETCODE_OK != ret) {
		//TODO 记录日志
		TRAVODDS_LOG(LOG_ERROR, "writer_ AddBuiltinCache ret= %d", ret);
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
	keyParam.length = MIG_RTPS_KEY_HASH_MAX_LENGTH;
	keyParam.value.assign(instanceHandle.keyHash.value, instanceHandle.keyHash.value + MIG_RTPS_KEY_HASH_MAX_LENGTH);

	StatusInfo_t status = { 0 };
	status[3] = StatusInfoFlag::DISPOSED_FLAG | StatusInfoFlag::UNREGISTERED_FLAG;
	statueParam.parameterId = PID_STATUS_INFO;
	statueParam.length = sizeof(StatusInfo_t);
	statueParam.value.assign(status, status + sizeof(StatusInfo_t));

	change->inlineQos.push_back(keyParam);
	change->inlineQos.push_back(statueParam);

	writer_->AddBuiltinCache(change);
}

void Spdp::TakeSPDPdiscoveredParticipantData(SPDPdiscoveredParticipantData& data, SerializedBuffer* buffer)
{
	ParameterList list((octet*)(buffer->buffer), buffer->buffer_size);
	list.GetDiscoveredData(data);
}

void Spdp::ReceiveSPDPdiscoveredParticipantData(SPDPdiscoveredParticipantData& data)
{
	//TODO 新增节点 添加代理
	if (0 == remoteParticipantData_.count(data.ddsParticipantData.key)) {
#ifdef DDSSECURITY
		// 安全下先不进行Subscription、Publication匹配
		discovery_->AddPartProxy(data);
#else
		// 没有安全下直接实体匹配
		discovery_->AddProxy(data);
#endif // DDSSECURITY
	}
	else
	{
		Time_t currentTime;
		DomainParticipantImpl::get_current_time(currentTime);
		remoteParticipantStatus_[data.ddsParticipantData.key].recvTime = Conversion::ToSecondsDouble(currentTime);
	}
#ifdef DDSSECURITY
	// 判断是否是自己
	GUID_t selfPartGuid = Conversion::ToGuid(discovery_->GetParticipant()->get_instance_handle());
	if (Conversion::ToGuid(data.ddsParticipantData.key) == selfPartGuid) {
		// 忽略来自自身的，测试阶段后续去掉TODO
		return;
	}
	//判断是否为已知节点，如果是发现新的DP 则进入验证远端，发起握手流程
	GuidPrefix_t remote_prefix = Conversion::ToGuid(data.ddsParticipantData.key).prefix;
	if (0 != discParts_.count(remote_prefix)) {
		// TRAVODDS_LOG(LOG_INFO, "the ParticipantData is not first received.");
		return;
	}
	// 发现新的DP 判断是否在忽略列表中
	auto it = find(ignoredParticipants_.begin(), ignoredParticipants_.end(), Conversion::ToGuid(data.ddsParticipantData.key));
	if (it != ignoredParticipants_.end()) {
		// 忽略这个DP
		TRAVODDS_LOG(LOG_DEBUG, "ignore associate with participant.");
		return;
	}

	// DDSSEC TODO: validate_remote_identity
	// 存储该DP到本地信息 TODO 析构以及移除等适配
	DiscoveryDPInfo* newDPInfo = new DiscoveryDPInfo;
	newDPInfo->owner_ = this;
	newDPInfo->pData_ = data;
	//try to authenticate the discovered participant
	SecurityException secExce;
	SecurityPluginInstanceImpl* secPlugin = &discovery_->GetParticipant()->secPlugin_;
	if (discovery_->GetParticipant()->enableDDSSec_ && secPlugin != NULL) {
		if (!data.ddsParticipantData.contains_security_data) {
			if (!discovery_->GetParticipant()->secAttributes_.allow_unauthenticated_participants) {
				TRAVODDS_LOG(LOG_ERROR, "Discovered a Participant without supporting DDS Security.");
				return;
			}
			newDPInfo->enableDDSSec_ = false;
		}
		else {
			TRAVODDS_LOG(LOG_DEBUG, "validate_remote_identity.");
			// validate_remote_identity 生成为远端参与者创建的handle, 缓存到newDPInfo, 用于本地验证.
			ValidationResult_t validationRst =
				secPlugin->authentication->validate_remote_identity(
					&newDPInfo->identityHdl_, //生成为远端参与者创建的handle,用于本地验证 缓存到newDPInfo
					discovery_->GetParticipant()->identityHdl_, //本地验证获取的handle
					data.ddsParticipantData.identity_token, //接收的token,来自远端调用的函数get_identity_token,来源远端的handle
					data.ddsParticipantData.key, // 从远端获取的pariticipant key
					&secExce);
			switch (validationRst)
			{
			case VALIDATION_OK: {
				newDPInfo->enableDDSSec_ = true;
				newDPInfo->endpointState_ = DiscoveryDPInfo::DDSSEC_OK;
				discParts_[remote_prefix] = newDPInfo;
				//TOCHECK
				discovery_->GetSedp()->AddBuiltinSpdpProxy(data);

				//根据SPDPdiscoveredParticipantData报文的内容生成远端guid和DiscoveredWriterData或其他内置主题数据结构
				HandshakeHandle localHsHdl = SEC_NIL_HANDLE; //begin_handshake_request的出参
				HandshakeMessageToken handshakeTokenRequest;
				HandshakeMessageToken handshakeTokenReply;
				HandshakeMessageToken handshakeTokenFinal;

				validationRst = secPlugin->authentication->begin_handshake_request(
					&localHsHdl, //生成握手信息
					&handshakeTokenRequest,
					discovery_->GetParticipant()->identityHdl_, //本地身份信息IdentityHandle
					newDPInfo->identityHdl_, //远端身份信息IdentityHandle
					(char*)bigEndianMsgData_->buffer, //SPDPdiscoveredParticipantData m_selfPartData的序列化内容
					bigEndianMsgData_->buffer_size, //长度
					&secExce);
				if (validationRst == VALIDATION_FAILED)
				{
					TRAVODDS_LOG(LOG_ERROR, "begin_handshake_request on self failed.");
					//discParts_.erase(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					SpdpCleanupRemoteSecRsc(newDPInfo);
					RemoveDiscoveryDPInfoFromDiscParts(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					return;
				}
				validationRst = secPlugin->authentication->begin_handshake_reply(
					&newDPInfo->handshakeHdl_,
					&handshakeTokenReply,
					&handshakeTokenRequest,
					newDPInfo->identityHdl_,
					discovery_->GetParticipant()->identityHdl_,
					(char*)bigEndianMsgData_->buffer,
					bigEndianMsgData_->buffer_size,
					&secExce);
				if (validationRst == VALIDATION_FAILED)
				{
					//discParts_.erase(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					SpdpCleanupRemoteSecRsc(newDPInfo);
					RemoveDiscoveryDPInfoFromDiscParts(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					return;
				}
				validationRst = secPlugin->authentication->process_handshake(
					&handshakeTokenFinal,
					&handshakeTokenReply,
					localHsHdl,
					&secExce);
				if (validationRst == VALIDATION_FAILED)
				{
					//discParts_.erase(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					SpdpCleanupRemoteSecRsc(newDPInfo);
					RemoveDiscoveryDPInfoFromDiscParts(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					return;
				}
				validationRst = secPlugin->authentication->process_handshake(
					NULL,
					&handshakeTokenFinal,
					newDPInfo->handshakeHdl_,
					&secExce);
				if (validationRst == VALIDATION_FAILED)
				{
					//discParts_.erase(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					SpdpCleanupRemoteSecRsc(newDPInfo);
					RemoveDiscoveryDPInfoFromDiscParts(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					return;
				}
				newDPInfo->sharedSecretHdl_ = secPlugin->authentication->get_shared_secret(
					localHsHdl, &secExce);
				//discovery_->AddRemainProxy(data);
				if (newDPInfo->sharedSecretHdl_ == SEC_NIL_HANDLE)
				{
					//discParts_.erase(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					SpdpCleanupRemoteSecRsc(newDPInfo);
					RemoveDiscoveryDPInfoFromDiscParts(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					return;
				}
				TRAVODDS_LOG(LOG_INFO, "Authenticating get_shared_secret.");
				int ret = ValidateRemoteParticipant(newDPInfo);
				if (ret != 0)
				{
					TRAVODDS_LOG(LOG_ERROR, "Authenticating local Participant failed: failed to get_authenticated_peer_credential_token.");
					return;
				}

				newDPInfo->cryptoHdl_ = secPlugin->crypto_key_factory->register_matched_remote_participant(
					newDPInfo->identityHdl_,
					discovery_->GetParticipant()->cryptoHdl_,
					newDPInfo->sharedSecretHdl_,
					newDPInfo->permissionsHdl_,
					&secExce);
				if (newDPInfo->cryptoHdl_ == SEC_NIL_HANDLE)
				{
					TRAVODDS_LOG(LOG_ERROR, "Register remote participant to CryptographicFactory failed.");
					//discParts_.erase(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					SpdpCleanupRemoteSecRsc(newDPInfo);
					RemoveDiscoveryDPInfoFromDiscParts(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					return;
				}

				//TOCHECK
				discovery_->GetSedp()->AddVolatileMessageBuiltinSecureSpdpProxy(data);

				if (discovery_->GetParticipant()->secAttributes_.is_rtps_protected)
				{
					Sequence<ParticipantCryptoToken> cryptoTokens;
					if (!secPlugin->crypto_key_exchange->create_local_participant_crypto_tokens(
						&cryptoTokens,
						discovery_->GetParticipant()->cryptoHdl_,
						newDPInfo->cryptoHdl_,
						&secExce))
					{
						//discParts_.erase(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
						SpdpCleanupRemoteSecRsc(newDPInfo);
						RemoveDiscoveryDPInfoFromDiscParts(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
						return;
					}
					if (!secPlugin->crypto_key_exchange->set_remote_participant_crypto_tokens(
						discovery_->GetParticipant()->cryptoHdl_,
						newDPInfo->cryptoHdl_,
						&cryptoTokens,
						&secExce))
					{
						//discParts_.erase(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
						SpdpCleanupRemoteSecRsc(newDPInfo);
						RemoveDiscoveryDPInfoFromDiscParts(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
						return;
					}
					secPlugin->crypto_key_exchange->return_crypto_tokens(&cryptoTokens, &secExce);
				}

				discovery_->GetParticipant()->secParticipants_.push_back(newDPInfo->cryptoHdl_);
				//TOCHECK
				discovery_->GetSedp()->AddBuiltinSecureSedpProxy(data);
				discovery_->GetSedp()->AddSedpProxy(data);

				secPlugin->authentication->return_handshake_handle(localHsHdl, &secExce);
				break;
			}
			case VALIDATION_FAILED: {
				TRAVODDS_LOG(LOG_ERROR, "validate_remote_identity failed.");
				if (!discovery_->GetParticipant()->secAttributes_.allow_unauthenticated_participants) {
				}
				newDPInfo->enableDDSSec_ = false;
				break;
			}
			case VALIDATION_PENDING_RETRY:
			{
				TRAVODDS_LOG(LOG_ERROR, "validate_remote_identity retry.");
				// @TODO: add retry process
				break;
			}
			case VALIDATION_PENDING_HANDSHAKE_REQUEST: //HandshakeInit begin_handshake_request
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(800)); //考虑时序性 确保对端进入waiting request
				newDPInfo->enableDDSSec_ = true;
				newDPInfo->endpointState_ = DiscoveryDPInfo::DDSSEC_HANDSHAKE_REQUESTING;
				discParts_[remote_prefix] = newDPInfo;

				// TOCHECK
				discovery_->GetSedp()->AddBuiltinSpdpProxy(data);

				// TODO
				newDPInfo->handshakeMsg_ = new ParticipantGenericMessage;
				HandshakeMessageToken handshakeToken;
				TRAVODDS_LOG(LOG_DEBUG, "begin_handshake_request.");
				validationRst = secPlugin->authentication->begin_handshake_request(
					&newDPInfo->handshakeHdl_,
					&handshakeToken,
					discovery_->GetParticipant()->identityHdl_,
					newDPInfo->identityHdl_,
					(char*)bigEndianMsgData_->buffer,
					bigEndianMsgData_->buffer_size,
					&secExce);

				//TODO 测试
				discovery_->GetSedp()->AddBuiltinSedpProxy(data);

				if (validationRst == VALIDATION_PENDING_HANDSHAKE_MESSAGE)
				{
					newDPInfo->handshakeMsg_->messageData.push_back(handshakeToken);
					secPlugin->authentication->return_handshake_token(&handshakeToken, &secExce);
					GUID_t remoteGuid;
					remoteGuid.prefix = Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix;
					remoteGuid.entityId = ENTITYID_PARTICIPANT;
					newDPInfo->handshakeMsg_->messageClassId = GMCLASSID_SECURITY_AUTH_HANDSHAKE;

					//auto now = std::chrono::system_clock::now();
					//std::time_t now_ms = std::chrono::system_clock::to_time_t(now);
					//std::cout << "now time after begin_handshake_request: " << std::put_time(std::localtime(&now_ms), "%F %T") << std::endl;

					//执行相互认证和密钥交换之前，发送ParticipantStatelessMessageData解决序列化攻击
					TRAVODDS_LOG(LOG_INFO, "TESTINFO send request TOKEN.");
					discovery_->WriteParticipantStatelessMessageData(&remoteGuid, *newDPInfo->handshakeMsg_, Sedp::StatelessMessageDataKind::REQUEST);
					newDPInfo->endpointState_ = DiscoveryDPInfo::DDSSEC_HANDSHAKE_WAITING_REPLY;

					//now = std::chrono::system_clock::now();
					//now_ms = std::chrono::system_clock::to_time_t(now);
					//std::cout << "now time after into WAITING_REPLY: " << std::put_time(std::localtime(&now_ms), "%F %T") << std::endl;
				}
				else if (validationRst == VALIDATION_OK)
				{
					secPlugin->authentication->return_handshake_token(&handshakeToken, &secExce);
					return;
				}
				else {
					//discParts_.erase(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					SpdpCleanupRemoteSecRsc(newDPInfo);
					RemoveDiscoveryDPInfoFromDiscParts(Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix);
					return;
				}
				return;
			}
			case VALIDATION_PENDING_HANDSHAKE_MESSAGE: {
				TRAVODDS_LOG(LOG_DEBUG, "validate_remote_identity into wait request.");
				newDPInfo->enableDDSSec_ = true;
				newDPInfo->endpointState_ = DiscoveryDPInfo::DDSSEC_HANDSHAKE_WAITING_REQUEST;

				//auto now = std::chrono::system_clock::now();
				//std::time_t now_ms = std::chrono::system_clock::to_time_t(now);
				//std::cout << "now time afer into WAITING_REQUEST: " << std::put_time(std::localtime(&now_ms), "%F %T") << std::endl;

				discParts_[remote_prefix] = newDPInfo;

				//TOCKECK
				discovery_->GetSedp()->AddBuiltinSpdpProxy(data);

				if (Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix !=
					Conversion::ToGuid(discovery_->GetParticipantData().ddsParticipantData.key).prefix) {
				}
				return;
			}
			case VALIDATION_OK_FINAL_MESSAGE: {
				TRAVODDS_LOG(LOG_DEBUG, "validate_remote_identity okfinal.");
			}
			default:
			{
				TRAVODDS_LOG(LOG_DEBUG, "validate_remote_identity default.");
				return;
			}
			} //Switch End
		}
	}
	
	discParts_[Conversion::ToGuid(newDPInfo->pData_.ddsParticipantData.key).prefix] = newDPInfo;
	// SedpAssociationSpdpEntities
	discovery_->GetSedp()->AddBuiltinSpdpProxy(data);
	// SedpAssociationSedpEntities
	discovery_->GetSedp()->AddBuiltinSedpProxy(data);

#endif // DDSSECURITY
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


	//printf("SPDP: add remote participant %d ", remoteParticipantData_.size());
	//GUID_t guid = Conversion::ToGuid(data.ddsParticipantData.key);
	//for (int i = 0; i < MIG_RTPS_KEY_HASH_MAX_LENGTH; i++) {
	//	printf("%02x ", ((unsigned char*)&guid)[i]);
	//}
	//printf("\n");

	discovery_->GetThreadPool()->submit([=]() {
		writer_->ResendDataPeriod();
	});

}

void Spdp::RemoveSpdpProxy(const GUID_t& guid)
{
	std::lock_guard<std::mutex> lock(mutex_);

	BuiltinTopicKey_t key = Conversion::ToBuiltinTopicKey(guid);

	if (0 == remoteParticipantStatus_.count(key)) {
		TRAVODDS_LOG(LOG_ERROR, "guid not exist");
		return;
	}
		

	remoteParticipantStatus_.erase(key);
	remoteParticipantData_.erase(key);

	//printf("SPDP: after remove remote participant %zu ", remoteParticipantData_.size());
	//for (int i = 0; i < 16; i++) {

	//	printf("%02x ", ((unsigned char *)&guid)[i]);
	//}
	//printf("\n");

	participant_->RemoveRemoteParticipantAllWriter(guid);
	participant_->RemoveRemoteParticipantAllReader(guid);

#ifdef DDSSECURITY
	RemoveDiscoveryDPInfoFromDiscParts(guid.prefix);
#endif // DDSSECURITY


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
