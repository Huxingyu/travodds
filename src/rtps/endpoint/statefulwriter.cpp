#include "rtps/endpoint/statefulwriter.h"
#include "rtps/endpoint/participant.h"
#include "rtps/messages/messageprocessor.h"
#include "rtps/transport/messagetransmitter.h"
#include "dcps/publish/datawriterimpl.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "rtps/flowcontrol/flowcontroller.h"
#include "common/log/logger.h"
#include <chrono>
#ifdef DDSSECURITY
#include "dcps/domain/domainparticipantimpl.h"
#include "rtps/discovery/discovery.h"
#include "security/securitycommon.h"
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/publish/publisherimpl.h"
#include <iomanip>
#endif // DDSSECURITY
USING_TRAVODDS_NAMESPACE;

StatefulWriter::StatefulWriter(const DataWriterQos& qos, const GUID_t& guid, const TopicKind_t& kind)
	: Writer(qos, kind)
{
	guid_ = guid;
}

StatefulWriter::~StatefulWriter() {

}
void StatefulWriter::MatchedReaderAdd(const DiscoveredReaderData &subData)
{
	bool active = subData.ddsSubscriptionData.reliability.kind == BEST_EFFORT_RELIABILITY_QOS ? false : true;
	std::shared_ptr<ReaderProxy> proxy = std::make_shared<ReaderProxy>(subData.remoteReaderGuid, subData.remoteGroupGuid, subData.unicastLocatorList, subData.multicastLocatorList, active);
	
	SequenceNumber_t maxSeq = writerCache_->GetSeqNumMax();
	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();

	if (maxSeq != SEQUENCENUMBER_UNKNOWN) {
		if (TRANSIENT_LOCAL_DURABILITY_QOS == subData.ddsSubscriptionData.durability.kind && TRANSIENT_LOCAL_DURABILITY_QOS == qos_.durability.kind) {
			proxy->InitChangeForReader(minSeq);
		}
		else {
			proxy->InitChangeForReader(maxSeq + 1);
		}
	}

	MatchedReaderAdd(proxy);
	transportGroup_.AddLocator(subData.remoteReaderGuid.prefix, subData.unicastLocatorList);
}

#ifdef DDSSECURITY
void StatefulWriter::MatchedReaderAddWithSecurity(const DiscoveredReaderData& subData, bool isSecure)
{
	DataWriterImpl* dw = datawriter_;
	if (!isSecure && dw->statistic_.secAttributes.is_discovery_protected && guid_.IsUserDefinedEntity()) { //开启保护但收到的是普通的发现数据
		return;
	}
	GUID_t dwGuid = Conversion::ToGuid(dw->get_instance_handle());
	GUID_t remoteDrGuid = Conversion::ToGuid(subData.ddsSubscriptionData.key);
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(dw->GetParent()->get_participant());
	if (dp->enableDDSSec_ && dwGuid.IsUserDefinedEntity()) {
		if (dw->statistic_.secAttributes.is_read_protected) {
			// 检查这个匹配的远端读者的安全属性check_remote_datareader
			SecurityException secExce;
			PermissionsHandle remotePermissionsHandle = SEC_NIL_HANDLE;
			DomainParticipantImpl* participant = dynamic_cast<DomainParticipantImpl*>(dw->GetParent()->get_participant());
			std::map<GuidPrefix_t, DiscoveryDPInfo*>& discParts = participant->GetDiscovery().get()->GetSpdp()->discParts_;
			BuiltinTopicKey_t participant_key = subData.ddsSubscriptionData.participant_key;
			GuidPrefix_t remote_prefix = Conversion::ToGuid(participant_key).prefix;
			auto dpInfoIter = discParts.find(remote_prefix);
			if (dpInfoIter == discParts.end())
			{
				remotePermissionsHandle = SEC_NIL_HANDLE;
			}
			else if (dpInfoIter->second->enableDDSSec_)
			{
				remotePermissionsHandle = dpInfoIter->second->permissionsHdl_;
			}
			if (!participant->secPlugin_.access_control->check_remote_datareader(
				remotePermissionsHandle,
				participant->GetDomainId(),
				&subData.ddsSubscriptionData,
				false, // @todo: distinguish different type of reader
				&secExce))
			{
				TRAVODDS_LOG(LOG_ERROR,
					"Match Reader failed: rejected by access control plugin in DataWriter."
					"\n  Code: %d"
					"\n  Minor code: %d"
					"\n  Message: %s",
					secExce.code,
					secExce.minor_code,
					secExce.message);
				return;
			}
		}
	}

	if (dp->enableDDSSec_ && dwGuid.IsUserDefinedEntity())
	{
	    EndpointSecurityAttributesMask dwSecAttrMask =
	        EndpointSecurityAttributesToMask(&dw->statistic_.secAttributes);
	    EndpointSecurityAttributesMask drSecAttrMask =
	        subData.ddsSubscriptionData.security_info.endpoint_security_attributes;
	    if ((drSecAttrMask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID) == 0)
	    {
	        drSecAttrMask = 0;
	    }
	    if (dwSecAttrMask != drSecAttrMask
	        || dw->statistic_.secAttributes.plugin_endpoint_attributes
	        != subData.ddsSubscriptionData.security_info.plugin_endpoint_security_attributes)
	    {
			TRAVODDS_LOG(LOG_ERROR, "Match Reader failed: endpoint security_info different from local writer.");
	        return;
	    }
	}

	ReaderInfo rdInfo; // rdInfo用来存储register_matched_remote_datareader生成的远端读者加密句柄
	rdInfo.cryptoHdl = SEC_NIL_HANDLE;
	rdInfo.readyForSecComm = false;
	if (dw->statistic_.enableSec && dw->secPlugin_ != NULL && 
		(dwGuid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) ||
		dwGuid.IsUserDefinedEntity()) { //用户自定义实体或者是volatile内置实体
		if (dw->statistic_.secAttributes.is_payload_protected
			|| dw->statistic_.secAttributes.is_submessage_protected)
		{
			// 为此匹配的远端读者创建加密句柄
			SecurityException secExce;
			DomainParticipantImpl* participant = dynamic_cast<DomainParticipantImpl*>(dw->GetParent()->get_participant());
			rdInfo.cryptoHdl = dw->secPlugin_->crypto_key_factory->register_matched_remote_datareader(
				dw->cryptoHdl_,
				participant->GetDiscovery()->GetSpdp()->GetRemoteParticipantCryptoHandle(&remoteDrGuid.prefix),
				participant->GetDiscovery()->GetSpdp()->GetRemoteParticipantSharedSecret(&remoteDrGuid.prefix),
				false, // @todo add more reader types
				&secExce);
			if (rdInfo.cryptoHdl == SEC_NIL_HANDLE) {
				TRAVODDS_LOG(LOG_ERROR, "Match Reader failed:  failed to register remote reader to key factory "
					"in DataWriter %s of Topic."
					"\n  Code: %d"
					"\n  Minor code: %d"
					"\n  Message: %s",
					secExce.code,
					secExce.minor_code,
					secExce.message);
				return;
			}
			if (dwGuid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER)
			{
				ParticipantGenericMessage cryptoTokensSample;
				if (!dw->secPlugin_->crypto_key_exchange->create_local_datawriter_crypto_tokens(
					&cryptoTokensSample.messageData,
					dw->cryptoHdl_,
					rdInfo.cryptoHdl,
					&secExce))
				{
					return;
				}
				cryptoTokensSample.messageClassId = GMCLASSID_SECURITY_DATAWRITER_CRYPTO_TOKENS;
				memcpy(cryptoTokensSample.destinationEndpointKey.value, &remoteDrGuid, 16);
				memcpy(cryptoTokensSample.sourceEndpointKey.value, &dwGuid, 16);
				GUID_t msgReceiver = remoteDrGuid;
				msgReceiver.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER;
				participant->GetDiscovery()->GetSedp()->WriteParticipantVolatileSecureMessage(&msgReceiver, cryptoTokensSample);
				dw->secPlugin_->crypto_key_exchange->return_crypto_tokens(
					&cryptoTokensSample.messageData, &secExce);
				cryptoTokensSample.messageClassId = "";
			}
			else
			{
				dw->secReaders_.push_back(rdInfo.cryptoHdl); //内置实体volatile使用participant的密钥材料作为writer端的密钥材料
				dw->secReadersMap_[remoteDrGuid.prefix] = rdInfo.cryptoHdl;
				rdInfo.readyForSecComm = true;
			}
		}
		else if (dw->statistic_.rtpsMsgProtected)
		{
			rdInfo.readyForSecComm = true;
		}
	}
	if (dw->statistic_.enableSec
		//&& dwGuid.IsSecureBuiltinEntity() //TODEL
		&& dwGuid.IsUserDefinedEntity() //TODEL
		&& dw->secPlugin_ != NULL
		&& !rdInfo.readyForSecComm)
	{
		if (dw->statistic_.secAttributes.is_payload_protected
			|| dw->statistic_.secAttributes.is_submessage_protected)
		{
			dw->remoteReaders_.preparingReaders[remoteDrGuid] = rdInfo;
			auto rdTokenIter = dw->remoteReaders_.remoteReaderCryptoToken.find(remoteDrGuid);
			if (rdTokenIter != dw->remoteReaders_.remoteReaderCryptoToken.end())
			{
				dw->SetRemoteReaderCryptoTokens(&remoteDrGuid, &rdTokenIter->second);
				dw->remoteReaders_.remoteReaderCryptoToken.erase(rdTokenIter);
			}
			// return;
		}
	}
	// 安全的匹配逻辑通过，调用通用的匹配逻辑
	return MatchedReaderAdd(subData);
}
#endif // DDSSECURITY

void StatefulWriter::MatchedReaderAdd(std::shared_ptr<ReaderProxy> readerProxy)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	readerProxy->SetHistoryCache(writerCache_);
	matchedReaders_[readerProxy->GetGuid()] = readerProxy;

	//participant_->GetThreadPool()->submit(
	//	[=]() {
	//		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//		WriteHeartbeat();
	//	});
}

void StatefulWriter::MatchedReaderRemove(std::shared_ptr<ReaderProxy> readerProxy)
{

	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	matchedReaders_.erase(readerProxy->GetGuid());

	transportGroup_.DecreaseLocator(readerProxy->GetGuid().prefix);

}

std::shared_ptr<ReaderProxy> StatefulWriter::MatchedReaderLookup(GUID_t readerGuid)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	auto itor = matchedReaders_.find(readerGuid);
	if (itor == matchedReaders_.end()) {
		TRAVODDS_LOG(LOG_DEBUG, "no match reader");
		return nullptr;
	}
	return itor->second;
}

ReturnCode_t StatefulWriter::WaitForAcknowledgments(const Duration_t & max_wait)
{
	{
		std::lock_guard<std::mutex> lock(readerPorxyMutex_);
		if (CheckAllAcked()) {
			return RETCODE_OK;
		}
	}
	std::chrono::nanoseconds maxW(max_wait.ToNs());
	std::unique_lock<std::mutex> ackLock(allAckedMtx_);
	auto result = condAllAcked_.wait_for(ackLock, maxW);
	return result == std::cv_status::no_timeout ? RETCODE_OK : RETCODE_TIMEOUT;
}

bool StatefulWriter::CheckAllAcked()
{
	SequenceNumber_t sequenceNumber = writerCache_->GetSeqNumMax();
	for (auto& reader : matchedReaders_) {
		if (false == reader.second->IsAllAcked(sequenceNumber)) {
			return false;
		}
	}
	return true;
}

bool StatefulWriter::IsAckedByAll(SequenceNumber_t changeSeqNum)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	for (const auto& readerProxy : matchedReaders_) {
		if (false == readerProxy.second->IsAcked(changeSeqNum)) {
			return false;
		}
	}
	return true;
}

ReturnCode_t StatefulWriter::DeliveryData(std::shared_ptr<CacheChange> change, const Duration_t& timeOut)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	/* wnagyi  */
	Duration_t duration = TIME_INFINITE;
	if (qos_.liveliness.lease_duration < duration) {
		auto livelinessManager = participant_->GetLivelinessManager();
		if (livelinessManager != nullptr) {
			livelinessManager->GetLocalWriterLivelinessManager()->AssertLiveliness(GetGuid(),qos_.liveliness.kind);
		}
		else {
			/* TODO 日志 */
			TRAVODDS_LOG(LOG_ERROR, "messageTransmitter is nullptr");
		}
	}

	std::vector<std::shared_ptr<ReaderProxy>> readerProxys;
	for (auto& readerProxy : matchedReaders_) {
		readerProxys.push_back(readerProxy.second);
#ifdef DDSSECURITY
		//如果是volatile实体，需要单独加密发送
		if (guid_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
			DeliveryDataToReaderForVolatile(change, readerProxy.second, timeOut);
		}
#endif // DDSSECURITY
	}

#ifdef DDSSECURITY
	if (guid_.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
		return DeliveryDataToReader(change, readerProxys, timeOut);
	}
#else
	return DeliveryDataToReader(change, readerProxys, timeOut);
#endif // DDSSECURITY
	

	return RETCODE_OK;
}

ChangeForReader StatefulWriter::CreateChangeForReader(std::shared_ptr<CacheChange> change)
{
	ChangeForReader changeForReader;
	changeForReader.sequenceNumber = change->sequenceNumber;

	if (true == IsPushMode()) {
		changeForReader.status = UNSENT;
	} else {
		changeForReader.status = UNACKNOWLEDGED;
	}

	if (true == DataFilter(change)) {
		changeForReader.isRelevant = false;
	} else {
		changeForReader.isRelevant = true;
	}

	return changeForReader;
}


ReturnCode_t StatefulWriter::ReceiveAckNackMessage(const Receive_t& headerMsg, const AcknackSubMessage_t& ackMsg)
{
	std::unique_lock<std::mutex> lock(readerPorxyMutex_);

	std::shared_ptr<ReaderProxy> proxy = nullptr;
	GUID_t guid(headerMsg.sourceGuidPrefix, ackMsg.readerId);

	auto itor = matchedReaders_.find(guid);
	if (matchedReaders_.end() == itor) {
		TRAVODDS_LOG(LOG_DEBUG, "no match reader");
		return RETCODE_ERROR;
	}
	proxy = itor->second;
	if (!proxy) {
		TRAVODDS_LOG(LOG_ERROR, "match readerproxy is nullptr");
		return RETCODE_ERROR;
	}

	ReturnCode_t ret = proxy->RecvAcknack(ackMsg);
	if (RETCODE_RECEIVE_DUPLICATE == ret) {
		// 返回RETCODE_RECEIVE_DUPLICATE，表示Ack报文接收重复，不处理
		return RETCODE_OK;
	} else if (RETCODE_OK == ret) {
		if(CheckAllAcked()) {
			condAllAcked_.notify_all();
		}
	}

	if (qos_.durability.kind != TRANSIENT_LOCAL_DURABILITY_QOS) {
		lock.unlock();
		RemoveAllAckedData();
		lock.lock();
	}

	//返回值为1
	if (RETCODE_REPEAT_REQUEXT == ret) {
		// TODO 为RETCODE_REPEAT_REQUEXT，需要重传数据
		flowController_->ReSendData(this,proxy);
	}
	/* 需要强制回复一个HB报文 */
	if (ackMsg.header.flags & !SubMessageFlag::FINAL_FLAG) {
		//TODO 
		WriteHeartbeat(proxy, true);
	}

	return RETCODE_OK;
}

ReturnCode_t StatefulWriter::ReceiveNackFragMessage(const Receive_t& headerMsg, const NackFragSubMessage_t& nackMsg)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	std::shared_ptr<ReaderProxy> proxy = nullptr;
	GUID_t guid(headerMsg.sourceGuidPrefix, nackMsg.readerId);

	auto itor = matchedReaders_.find(guid);
	if (matchedReaders_.end() == itor) {
		TRAVODDS_LOG(LOG_DEBUG, "no match reader");
		return RETCODE_ERROR;
	}
	proxy = itor->second;
	if (!proxy) {
		TRAVODDS_LOG(LOG_ERROR, "match readerproxy is nullptr");
		return RETCODE_ERROR;
	}

	ReturnCode_t ret = proxy->RecvNackFrag(nackMsg);
	if (RETCODE_RECEIVE_DUPLICATE == ret) {
		// 返回RETCODE_RECEIVE_DUPLICATE，表示nack报文接收重复，不处理
		return RETCODE_OK;
	}

	flowController_->ReSendDataFrag(this, proxy, nackMsg);

	//需要强制回复一个HBFrag报文
	if (nackMsg.header.flags & SubMessageFlag::FINAL_FLAG) {
		//TODO 
	}
	return RETCODE_OK;
}

ReturnCode_t StatefulWriter::DeliveryDataToReader(std::shared_ptr<CacheChange> change, const std::vector<std::shared_ptr<ReaderProxy>>& readerProxys, const Duration_t& timeOut)
{
	if (nullptr == change || 0 == readerProxys.size()) {
		return RETCODE_OK;
	}
	DataSubMessage_t dataSubMessage;
	SerializedBuffer* buffer = change->serializedBuffer;

#ifdef DDSSECURITY
	//是否子消息加密（内置实体volatile为子消息加密，或者是用户自定义实体配置子消息加密）
	bool encodeSubMsg = datawriter_->statistic_.enableSec
        && datawriter_->statistic_.secAttributes.is_submessage_protected
        && (guid_.IsUserDefinedEntity() || guid_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER);
    bool encodeRtpsMsg = datawriter_->statistic_.enableSec
        && datawriter_->statistic_.rtpsMsgProtected
        && guid_.IsUserDefinedEntity();
#endif // DDSSECURITY


	std::vector<LocatorVec> dstLocators;
	std::set<GuidPrefix_t> prefixs;
	for (auto& reader : readerProxys) {
		if (!change->filterReaders_.empty() && change->filterReaders_.find(reader->GetGuid()) != change->filterReaders_.end()) {
			SendGap({ change->sequenceNumber }, reader);
			continue;
		}
		prefixs.insert(reader->GetGuid().prefix);
	}

	transportGroup_.GetLocator(prefixs, dstLocators);

	if (buffer->next) {
		/* 分片数据处理 */
		DataFragSubMessage_t dataFragSubMessage;
		if (0 != change->inlineQos.size())
		{
			dataFragSubMessage.header.flags |= SubMessageFlag::INLINE_QOS_FLAG;
			dataFragSubMessage.inlineQos = change->inlineQos;
		}

		dataFragSubMessage.header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

		/* wangyi fastdds 适配测试默认16 */
		dataFragSubMessage.octetsToInlineQos = OCTETS_TO_INLINEQOS_LEN;
		dataFragSubMessage.extraFlags[0] = 0;
		dataFragSubMessage.extraFlags[1] = 0;
		dataFragSubMessage.readerId = ENTITYID_UNKNOWN;
		dataFragSubMessage.writerId = guid_.entityId;
		dataFragSubMessage.writerSN = change->sequenceNumber;
		dataFragSubMessage.dataFrag.fragmentStartingNum = 0;
		uint32_t sendFragCount = 0;
		do {
			/* 分片报文处理 */
			uint32_t shift = 0;
			dataFragSubMessage.dataFrag.fragmentsInSubmessage = 1;
			dataFragSubMessage.dataFrag.fragmentSize = DATAMESSAGE_LIMIT;
			dataFragSubMessage.dataFrag.fragmentStartingNum += 1;
			dataFragSubMessage.dataFrag.sampleSize = change->dataSize;

#ifdef DDSSECURITY
			SerializedBuffer encodeRtpsBuffer;
			char* encodeBuf = NULL; //开辟的存储加密后缓存
			if (encodeSubMsg || encodeRtpsMsg) {
				encodeBuf = new char[buffer->buffer_size * 10];
			}
			encodeRtpsBuffer.buffer_size = buffer->buffer_size * 10; //TODO 长度公式
			encodeRtpsBuffer.buffer = encodeBuf;
			// 对于临时开辟存储加密后的缓冲Buffer的偏移，这里主要记录子消息加密情况下，封装header的偏移
			uint32_t encodeShift = 0;
#endif // DDSSECURITY

			MessageProcessor::EncapsulateDataFragMessage(*buffer, shift, dataFragSubMessage);

#ifdef DDSSECURITY
			// encodeBuffer为子消息缓冲区
			SerializedBuffer srcBuffer;
			// encodeBuffer为临时开辟缓冲区，存储加密后子消息
			SerializedBuffer encodeBuffer;
			srcBuffer.buffer = buffer->buffer - shift;
			srcBuffer.buffer_size = buffer->buffer_size + shift;;

			int dataRet;
			// 子消息加密后长度 包含DataFrag头部，DataFrag其他信息，DataFrag负载的长度
			uint32_t encodeDataSize = 0;
			if (encodeSubMsg) {
				encodeBuffer.buffer_size = srcBuffer.buffer_size * 10; //TODO 分配子消息加密后空间
				encodeBuffer.buffer = new char[encodeBuffer.buffer_size];
				//encode_datawriter_message
				dataRet = EncodeSubMsgBuffer(NULL, srcBuffer, encodeBuffer);
				//加密后的子消息放到encodeRtpsBuffer的缓冲区尾部
				memcpy(encodeRtpsBuffer.buffer + encodeRtpsBuffer.buffer_size - encodeBuffer.buffer_size,
					encodeBuffer.buffer,
					encodeBuffer.buffer_size);
				encodeDataSize = encodeBuffer.buffer_size;
				delete[] encodeBuffer.buffer;
			}

			//封装Header
			if (encodeSubMsg) {
				//encodeBuffer指向encodeRtpsBuffer的DATA的头部位置
				encodeBuffer.buffer = encodeRtpsBuffer.buffer
					+ encodeRtpsBuffer.buffer_size
					- encodeBuffer.buffer_size;
				encodeBuffer.buffer_size = encodeDataSize;
				MessageProcessor::EncapsulateMessageHeader(encodeBuffer, encodeShift, guid_.prefix);
			}
			else {
				//change->serializedBuffer指向负载，shift出入参，代表存储DATA的向前偏移以及加上Header后的向前偏移
				MessageProcessor::EncapsulateMessageHeader(*buffer, shift, guid_.prefix);
			}
#else
			MessageProcessor::EncapsulateMessageHeader(*buffer, shift, guid_.prefix);
#endif // DDSSECURITY

#ifdef DDSSECURITY
			if (encodeRtpsMsg) {
				ParticipantCryptoHandle* partCryptoHdls = NULL;
				uint32_t partCryptoHdlsCount = 0;
				DomainParticipantImpl* participant = dynamic_cast<DomainParticipantImpl*>(datawriter_->GetParent()->get_participant());
				partCryptoHdls = participant->secParticipants_.data();
				partCryptoHdlsCount = participant->secParticipants_.size();
				SecurityException secExce;

				SerializedBuffer encodeBuffer;
				SerializedBuffer plainBuffer;
				//被加密的缓冲 
				int msgHeaderSize = sizeof(MessageHeader_t);
				if (!encodeSubMsg) { //从buffer取
					plainBuffer.buffer = buffer->buffer - shift; //负载位置buffer->buffer往前shift，shift是DATA子报文一些内容
					plainBuffer.buffer_size = shift + buffer->buffer_size;
				}
				else { //从已经子消息加密的encodeRtpsBuffer取
					plainBuffer.buffer = encodeRtpsBuffer.buffer
						+ encodeRtpsBuffer.buffer_size
						- encodeDataSize //DATA
						- encodeShift; //Header
					plainBuffer.buffer_size = encodeDataSize + encodeShift;
				}

				if (encodeRtpsMsg) {
					//encodeBuffer存储被rtps加密报文的缓冲区
					encodeBuffer.buffer = new char[buffer->buffer_size * 10];
					encodeBuffer.buffer_size = buffer->buffer_size * 10;
					if (datawriter_->secPlugin_ && !datawriter_->secPlugin_->crypto_transform->encode_rtps_message(
						&encodeBuffer,
						&plainBuffer,
						1, //TODO
						participant->cryptoHdl_,
						partCryptoHdls,
						partCryptoHdlsCount,
						&secExce))
					{
						delete[]encodeBuffer.buffer;
						return -1;
					}
					//将encodeBuffer拷贝到encodeRtpsBuffer
					encodeRtpsBuffer.buffer = encodeRtpsBuffer.buffer + encodeRtpsBuffer.buffer_size - encodeBuffer.buffer_size;
					encodeRtpsBuffer.buffer_size = encodeBuffer.buffer_size;
					memcpy(encodeRtpsBuffer.buffer, encodeBuffer.buffer, encodeBuffer.buffer_size);
					delete[]encodeBuffer.buffer;
				}
			}
#endif // DDSSECURITY
			ReturnCode_t ret = RETCODE_OK;
#ifdef DDSSECURITY
			if (encodeSubMsg && !encodeRtpsMsg) {
				ret = participant_->FlowRateLimitedMessageSend(dstLocators, encodeBuffer.buffer - encodeShift, encodeDataSize + encodeShift, timeOut);
			}
			else if (encodeRtpsMsg) {
				ret = participant_->FlowRateLimitedMessageSend(dstLocators, encodeRtpsBuffer.buffer, encodeRtpsBuffer.buffer_size, timeOut);
			}
			else {
				ret = participant_->FlowRateLimitedMessageSend(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift, timeOut);
			}
			if (encodeSubMsg || encodeRtpsMsg) {
				delete[] encodeBuf;
			}
			if (ret != RETCODE_OK) {
				TRAVODDS_LOG(LOG_ERROR, "write data error.");
				return ret;
			}	
#else
			ret =participant_->FlowRateLimitedMessageSend(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift, timeOut);
			if (ret != RETCODE_OK) {
				TRAVODDS_LOG(LOG_ERROR, "write data error.");
				return ret;
			}	
#endif
			/* TODO 添加延时 */
			sendFragCount++;
			if (0 == sendFragCount % DATAFRAG_DELAY_NUM) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			buffer = buffer->next;
		} while (buffer);

	}
	else {
		if (0 != change->serializedBuffer->buffer_size) {
			dataSubMessage.header.flags |= SubMessageFlag::DATA_FLAG;
		}

		if (0 != change->inlineQos.size()) {
			dataSubMessage.header.flags |= SubMessageFlag::INLINE_QOS_FLAG;
			dataSubMessage.inlineQos = change->inlineQos;
		}

		dataSubMessage.header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

		/* wangyi fastdds 适配测试默认16 */
		dataSubMessage.octetsToInlineQos = OCTETS_TO_INLINEQOS_LEN;
		dataSubMessage.extraFlags[0] = SubMessageFlag::NONE_FLAG;
		dataSubMessage.extraFlags[1] = SubMessageFlag::NONE_FLAG;
		dataSubMessage.readerId = ENTITYID_UNKNOWN;
		dataSubMessage.writerId = guid_.entityId;
		dataSubMessage.writerSN = change->sequenceNumber;

		uint32_t shift = 0;

#ifdef DDSSECURITY
		SerializedBuffer encodeRtpsBuffer;
		char* encodeBuf = NULL; //开辟的存储加密后缓存
		if (encodeSubMsg || encodeRtpsMsg) {
			encodeBuf = new char[buffer->buffer_size * 10];
		}
		encodeRtpsBuffer.buffer_size = buffer->buffer_size * 10; //TODO 长度公式
		encodeRtpsBuffer.buffer = encodeBuf;
		// 对于临时开辟存储加密后的缓冲Buffer的偏移，这里主要记录子消息加密情况下，封装header的偏移
		uint32_t encodeShift = 0;
#endif // DDSSECURITY

		MessageProcessor::EncapsulateDATAMessage(*change->serializedBuffer, shift, dataSubMessage);

#ifdef DDSSECURITY
		SerializedBuffer srcBuffer;
		// encodeBuffer为临时开辟缓冲区，存储加密后子消息
		SerializedBuffer encodeBuffer;
		srcBuffer.buffer = change->serializedBuffer->buffer - shift;
		srcBuffer.buffer_size = change->serializedBuffer->buffer_size + shift;;

		int dataRet;
		// 子消息加密后长度 包含Data头部，Data其他信息，Data负载的长度
		uint32_t encodeDataSize = 0;
		// 子消息加密 encodeRtpsBuffer包含DATA头和DATA其他信息
		if (encodeSubMsg) {
			encodeBuffer.buffer_size = srcBuffer.buffer_size * 10; //TODO 分配子消息加密后空间
			encodeBuffer.buffer = new char[encodeBuffer.buffer_size];
			//encode_datawriter_message
			dataRet = EncodeSubMsgBuffer(NULL, srcBuffer, encodeBuffer);
			//加密后的子消息放到encodeRtpsBuffer的缓冲区尾部
			memcpy(encodeRtpsBuffer.buffer + encodeRtpsBuffer.buffer_size - encodeBuffer.buffer_size,
				encodeBuffer.buffer,
				encodeBuffer.buffer_size);
			encodeDataSize = encodeBuffer.buffer_size;
			delete[] encodeBuffer.buffer;
		}
#endif // DDSSECURITY

#ifdef DDSSECURITY
		if (encodeSubMsg) {
			//encodeBuffer指向encodeRtpsBuffer的DATA的头部位置
			encodeBuffer.buffer = encodeRtpsBuffer.buffer
				+ encodeRtpsBuffer.buffer_size
				- encodeBuffer.buffer_size;
			encodeBuffer.buffer_size = encodeDataSize;
			MessageProcessor::EncapsulateMessageHeader(encodeBuffer, encodeShift, guid_.prefix);
		}
		else {
			MessageProcessor::EncapsulateMessageHeader(*change->serializedBuffer, shift, guid_.prefix);
		}

		if (encodeRtpsMsg) {
			ParticipantCryptoHandle* partCryptoHdls = NULL;
			uint32_t partCryptoHdlsCount = 0;
			DomainParticipantImpl* participant = dynamic_cast<DomainParticipantImpl*>(datawriter_->GetParent()->get_participant());
			partCryptoHdls = participant->secParticipants_.data();
			partCryptoHdlsCount = participant->secParticipants_.size();
			SecurityException secExce;

			SerializedBuffer encodeBuffer;
			SerializedBuffer plainBuffer;
			//被加密的缓冲 
			int msgHeaderSize = sizeof(MessageHeader_t);
			if (!encodeSubMsg) { //从buffer取
				plainBuffer.buffer = buffer->buffer - shift; //负载位置buffer->buffer往前shift，shift是DATA子报文一些内容
				plainBuffer.buffer_size = shift + buffer->buffer_size;
			}
			else { //从已经子消息加密的encodeRtpsBuffer取
				plainBuffer.buffer = encodeRtpsBuffer.buffer
					+ encodeRtpsBuffer.buffer_size
					- encodeDataSize //DATA
					- encodeShift; //Header
				plainBuffer.buffer_size = encodeDataSize + encodeShift;
			}

			if (encodeRtpsMsg) {
				//encodeBuffer存储被rtps加密报文的缓冲区
				encodeBuffer.buffer = new char[buffer->buffer_size * 10];
				encodeBuffer.buffer_size = buffer->buffer_size * 10;
				if (datawriter_->secPlugin_ && !datawriter_->secPlugin_->crypto_transform->encode_rtps_message(
					&encodeBuffer,
					&plainBuffer,
					1, //TODO
					participant->cryptoHdl_,
					partCryptoHdls,
					partCryptoHdlsCount,
					&secExce)) {
					delete[]encodeBuffer.buffer;
					return -1;
				}
				//将encodeBuffer拷贝到encodeRtpsBuffer
				encodeRtpsBuffer.buffer = encodeRtpsBuffer.buffer + encodeRtpsBuffer.buffer_size - encodeBuffer.buffer_size;
				encodeRtpsBuffer.buffer_size = encodeBuffer.buffer_size;
				memcpy(encodeRtpsBuffer.buffer, encodeBuffer.buffer, encodeBuffer.buffer_size);
				delete[]encodeBuffer.buffer;
			}
		}
#else
		MessageProcessor::EncapsulateMessageHeader(*change->serializedBuffer, shift, guid_.prefix);
#endif // DDSSECURITY

		//for (auto reader : readerProxys) {

		//	LocatorVec dstLocators = reader->GetUnicastLocatorList();
		//	messageTransmitter->MessageSend(srcLocators, dstLocators, change->serializedBuffer->buffer - shift, change->serializedBuffer->buffer_size + shift);
		//}

		ReturnCode_t ret = RETCODE_OK;
#ifdef DDSSECURITY
		if (encodeSubMsg && !encodeRtpsMsg) {
			participant_->FlowRateLimitedMessageSend(dstLocators, encodeBuffer.buffer - encodeShift, encodeDataSize + encodeShift, timeOut);
		}
		else if (encodeRtpsMsg) {
			participant_->FlowRateLimitedMessageSend(dstLocators, encodeRtpsBuffer.buffer, encodeRtpsBuffer.buffer_size, timeOut);
		}
		else {
			participant_->FlowRateLimitedMessageSend(dstLocators, change->serializedBuffer->buffer - shift, change->serializedBuffer->buffer_size + shift, timeOut);
		}
		if (encodeSubMsg || encodeRtpsMsg) {
			delete[] encodeBuf;
		}
		if (ret != RETCODE_OK) {
			TRAVODDS_LOG(LOG_ERROR, "write data error.");
			return ret;
		}
#else
		participant_->FlowRateLimitedMessageSend(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift, timeOut);
		if (ret != RETCODE_OK) {
			TRAVODDS_LOG(LOG_ERROR, "write data error.");
			return ret;
		}
#endif
	}
	return RETCODE_OK;
}

#ifdef DDSSECURITY
ReturnCode_t StatefulWriter::DeliveryDataToReaderForVolatile(std::shared_ptr<CacheChange> change, std::shared_ptr<ReaderProxy> readerProxy, const Duration_t& timeOut)
{
	if (nullptr == change || readerProxy == nullptr) {
		return RETCODE_OK;
	}
	DataSubMessage_t dataSubMessage;
	SerializedBuffer* buffer = change->serializedBuffer;

	bool encodeSubMsg = datawriter_->statistic_.enableSec
		&& datawriter_->statistic_.secAttributes.is_submessage_protected; //volatile这个值一般为true
	bool encodeRtpsMsg = false; //volatile这个值为false

	std::vector<LocatorVec> dstLocators;
	std::set<GuidPrefix_t> prefixs;
	if (!change->filterReaders_.empty() && change->filterReaders_.find(readerProxy->GetGuid()) != change->filterReaders_.end()) {
		SendGap({ change->sequenceNumber }, readerProxy);
	}
	prefixs.insert(readerProxy->GetGuid().prefix);

	transportGroup_.GetLocator(prefixs, dstLocators);

	if (0 != change->serializedBuffer->buffer_size) {
		dataSubMessage.header.flags |= SubMessageFlag::DATA_FLAG;
	}

	if (0 != change->inlineQos.size()) {
		dataSubMessage.header.flags |= SubMessageFlag::INLINE_QOS_FLAG;
		dataSubMessage.inlineQos = change->inlineQos;
	}

	dataSubMessage.header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

	/* wangyi fastdds 适配测试默认16 */
	dataSubMessage.octetsToInlineQos = OCTETS_TO_INLINEQOS_LEN;
	dataSubMessage.extraFlags[0] = SubMessageFlag::NONE_FLAG;
	dataSubMessage.extraFlags[1] = SubMessageFlag::NONE_FLAG;
	dataSubMessage.readerId = ENTITYID_UNKNOWN;
	dataSubMessage.writerId = guid_.entityId;
	dataSubMessage.writerSN = change->sequenceNumber;

	uint32_t shift = 0;
	SerializedBuffer encodeRtpsBuffer;
	char* encodeBuf = NULL; //开辟的存储加密后缓存
	if (encodeSubMsg) {
		encodeBuf = new char[buffer->buffer_size * 10];
	}
	encodeRtpsBuffer.buffer_size = buffer->buffer_size * 10; //TODO 长度公式
	encodeRtpsBuffer.buffer = encodeBuf;
	// 对于临时开辟存储加密后的缓冲Buffer的偏移，这里主要记录子消息加密情况下，封装header的偏移
	uint32_t encodeShift = 0;

	MessageProcessor::EncapsulateDATAMessage(*change->serializedBuffer, shift, dataSubMessage);

	SerializedBuffer srcBuffer;
	SerializedBuffer encodeBuffer; // encodeBuffer为临时开辟缓冲区，存储加密后子消息
	srcBuffer.buffer = change->serializedBuffer->buffer - shift;
	srcBuffer.buffer_size = change->serializedBuffer->buffer_size + shift;;

	int dataRet;
	// 子消息加密后长度 包含Data头部，Data其他信息，Data负载的长度
	uint32_t encodeDataSize = 0;
	// 子消息加密 encodeRtpsBuffer包含DATA头和DATA其他信息
	if (encodeSubMsg) {
		encodeBuffer.buffer_size = srcBuffer.buffer_size * 10; //TODO 分配子消息加密后空间
		encodeBuffer.buffer = new char[encodeBuffer.buffer_size];
		//encode_datawriter_message
		dataRet = EncodeSubMsgBuffer(readerProxy, srcBuffer, encodeBuffer);
		//加密后的子消息放到encodeRtpsBuffer的缓冲区尾部
		memcpy(encodeRtpsBuffer.buffer + encodeRtpsBuffer.buffer_size - encodeBuffer.buffer_size,
			encodeBuffer.buffer,
			encodeBuffer.buffer_size);
		encodeDataSize = encodeBuffer.buffer_size;
		delete[] encodeBuffer.buffer;
	}

	if (encodeSubMsg) {
		//encodeBuffer指向encodeRtpsBuffer的DATA的头部位置
		encodeBuffer.buffer = encodeRtpsBuffer.buffer
			+ encodeRtpsBuffer.buffer_size
			- encodeBuffer.buffer_size;
		encodeBuffer.buffer_size = encodeDataSize;
		MessageProcessor::EncapsulateMessageHeader(encodeBuffer, encodeShift, guid_.prefix);
	}
	else {
		MessageProcessor::EncapsulateMessageHeader(*change->serializedBuffer, shift, guid_.prefix);
	}

	ReturnCode_t ret = RETCODE_OK;

	if (encodeSubMsg) {
		participant_->FlowRateLimitedMessageSend(dstLocators, encodeBuffer.buffer - encodeShift, encodeDataSize + encodeShift, timeOut);
	}
	else {
		participant_->FlowRateLimitedMessageSend(dstLocators, change->serializedBuffer->buffer - shift, change->serializedBuffer->buffer_size + shift, timeOut);
	}
	if (encodeSubMsg) {
		delete[] encodeBuf;
	}
	if (ret != RETCODE_OK) {
		TRAVODDS_LOG(LOG_ERROR, "write data error.");
		return ret;
	}

	return RETCODE_OK;
}
#endif

ReturnCode_t StatefulWriter::WriteHeartbeat()
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);
	HeartBeatSubMessage_t hbMsg;

	for (auto& readerProxy : matchedReaders_) {
		WriteHeartbeat(readerProxy.second, false);
	}
	return RETCODE_OK;
}

/* wangyi 发送 liveliness 心跳 */
ReturnCode_t StatefulWriter::WriteHeartbeat(bool liveliness)
{
	std::lock_guard<std::mutex> lock(readerPorxyMutex_);

	HeartBeatSubMessage_t hbMsg;

	for (auto& writerProxy : matchedReaders_) {
		WriteHeartbeat(writerProxy.second, true,liveliness);
	}
	return RETCODE_OK;
}

ReturnCode_t StatefulWriter::WriteHeartbeat(std::shared_ptr<ReaderProxy> readerProxy, bool force, bool liveliness)
{
	//TODO 此函数可以发送HeartBeat报文,当时后续需要优化改进

	if (!readerProxy) {
		TRAVODDS_LOG(LOG_ERROR, "readerProxy is nullptr");
		return RETCODE_ERROR;
	}

	// statelessreader 不发送heartbeat
	if (false == readerProxy->isActive()) {
		return RETCODE_OK;
	}
		

	HeartBeatSubMessage_t hbMsg;
	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();
	SequenceNumber_t maxSeq = writerCache_->GetSeqNumMax();
	SequenceNumber_t relateSeq = readerProxy->GetRelatedSequenceNumber();

	/* 在非强制回复HB的情况下，如果所有数据被确认，没有数据需要被确认不发送心跳  */
	if (!force && (SEQUENCENUMBER_ZERO >= maxSeq || !readerProxy->UnackedChanges())) {
		return RETCODE_OK;
	}

	LocatorSeq unicastList = readerProxy->GetUnicastLocatorList();
	LocatorSeq multicastList = readerProxy->GetMulticastLocatorList();

	char data[1224] = { 0 }; //开辟消息空间
	SerializedBuffer bufffer;
	uint32_t shift = 0;
	bufffer.buffer_size = sizeof(HeartBeatSubMessage_t);
	bufffer.buffer = data;
	bufffer.buffer = bufffer.buffer + BUFF_SHIFT; //先指向data的1024偏移

	//加密前的子消息长度
	uint32_t lengthHeartBeatMsgBefore = 0;
	uint32_t lengthGapMsgBefore = 0;

#ifdef DDSSECURITY
	bool encodeSubMsg = datawriter_->statistic_.enableSec
		&& datawriter_->statistic_.secAttributes.is_submessage_protected
		&& guid_.IsUserDefinedEntity();
	bool encodeRtpsMsg = datawriter_->statistic_.enableSec
		&& datawriter_->statistic_.rtpsMsgProtected
		&& guid_.IsUserDefinedEntity();
	//encodeSubMsg = false;
	//encodeRtpsMsg = false;
	SerializedBuffer encodeBufffer; //加密情况发送的buffer
	SerializedBuffer tmpPlainBuffer;
	SerializedBuffer tmpEncodeBuffer; //临时存储加密后缓冲，拷贝到encodeData后释放
	char encodeData[1224 * 2] = { 0 }; //开辟加密消息空间
#endif // DDSSECURITY

/*	HeartbeatMsgFlag_t flag;
	flag.groupInfoFlag = 1;
	flag.groupInfoFlag = 1; */
	hbMsg.header.flags = (IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG);
	/* wangyi liveliness  */
	if (liveliness) {
		hbMsg.header.flags |= SubMessageFlag::LIVELINESS_FLAG;
	}
	hbMsg.readerId = readerProxy->GetGuid().entityId;
	hbMsg.writerId = guid_.entityId;
	hbMsg.firstSN = minSeq;
	hbMsg.lastSN = maxSeq;
	hbMsg.count = readerProxy->HeaertSendCountAdd();

	// 序列化Heartbeat, bufffer指向Heartbeat头部位置
	MessageProcessor::EncapsulateHeartBeatMessage(bufffer, shift, hbMsg);
	lengthHeartBeatMsgBefore = shift;
	bufffer.buffer_size = shift;
	shift = 0;

#ifdef DDSSECURITY
	int dataRet;
	//加密后的子消息长度
	uint32_t lengthHeartBeatMsg = 0;
	uint32_t lengthGapMsg = 0;
	uint32_t lengthHeaderMsg = 0;
	//加密heartbeat子消息
	if (encodeSubMsg) {
		tmpEncodeBuffer.buffer_size = bufffer.buffer_size * 10; //TODO 长度计算
		tmpEncodeBuffer.buffer = new char[tmpEncodeBuffer.buffer_size];
		//encode_datawriter_message
		dataRet = EncodeSubMsgBuffer(readerProxy, bufffer, tmpEncodeBuffer);
		lengthHeartBeatMsg = tmpEncodeBuffer.buffer_size;
		//拷贝到encodeData末尾
		memcpy(encodeData + 1224 * 2 - lengthHeartBeatMsg, tmpEncodeBuffer.buffer, lengthHeartBeatMsg);
		delete[] tmpEncodeBuffer.buffer;
	}
#endif // DDSSECURITY

	/* 封装一个Gap报文 */
	if (relateSeq > minSeq) {
		/*  minseq 和 relateSeq 之间的序号与对应的代理无关，发送Gap报文 */
		GapSubMessage_t gapMsg;
		gapMsg.header.subMessageId = GAP;
		gapMsg.header.subMessageLength = SUB_MESSAGE_LEN_GAP; //TODO
		gapMsg.header.flags = IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;
		gapMsg.writerId = guid_.entityId;
		gapMsg.readerId = readerProxy->GetGuid().entityId;
		gapMsg.gapStart = minSeq;
		gapMsg.gapList.bitmapBase = relateSeq;
		gapMsg.gapList.bitmap.size = 0;
		int size = 0;
		
		size = sizeof(gapMsg.header) +  sizeof(gapMsg.writerId) * 2 +  sizeof(gapMsg.gapStart) + sizeof(gapMsg.gapList.bitmapBase)
			+ sizeof(gapMsg.gapList.bitmap.size);
		bufffer.buffer = bufffer.buffer - size;  //bufffer指向gapMsg起始位置
		
		MessageProcessor::EncapsulateGapMessage(bufffer, shift, gapMsg); // 出参shift为gapMsg长度
		lengthGapMsgBefore = shift;
		bufffer.buffer_size += shift; //bufffer长度gap+heartbeat
		shift = 0;

#ifdef DDSSECURITY
		//被加密的Gap消息明文
		tmpPlainBuffer.buffer_size = lengthGapMsgBefore;
		tmpPlainBuffer.buffer = bufffer.buffer;
		//加密子消息encode
		if (encodeSubMsg) {
			tmpEncodeBuffer.buffer_size = tmpPlainBuffer.buffer_size * 10;
			tmpEncodeBuffer.buffer = new char[tmpEncodeBuffer.buffer_size];
			//encode_datawriter_message
			dataRet = EncodeSubMsgBuffer(readerProxy, tmpPlainBuffer, tmpEncodeBuffer);
			lengthGapMsg = tmpEncodeBuffer.buffer_size;
			//拷贝到encodeData
			memcpy(encodeData + 1224 * 2 - lengthHeartBeatMsg - lengthGapMsg,
				tmpEncodeBuffer.buffer,
				lengthGapMsg);
			delete[] tmpEncodeBuffer.buffer;
		}
#endif // DDSSECURITY
	}

#ifdef DDSSECURITY
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(datawriter_->GetParent()->get_participant());
	if (encodeSubMsg) { //如果之前加密了子消息
		// tmpPlainBuffer指向加密后的消息(包含Heartbeat和Gap)
		tmpPlainBuffer.buffer_size = lengthHeartBeatMsg + lengthGapMsg;
		tmpPlainBuffer.buffer = encodeData + 1224 * 2 - lengthHeartBeatMsg - lengthGapMsg;
		MessageProcessor::EncapsulateMessageHeader(tmpPlainBuffer, shift, guid_.prefix); //这里出参shift为rtpsHeader长度
	}
	else {
		MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix); //这里出参shift为rtpsHeader长度
	}
	if (encodeRtpsMsg) {
		ParticipantCryptoHandle* partCryptoHdls = NULL;
		uint32_t partCryptoHdlsCount = 0;
		DomainParticipantImpl* participant = dynamic_cast<DomainParticipantImpl*>(datawriter_->GetParent()->get_participant());
		partCryptoHdls = participant->secParticipants_.data();
		partCryptoHdlsCount = participant->secParticipants_.size();
		SecurityException secExce;
		if (encodeSubMsg) { // 加密子消息情况下使用encodeData
			tmpPlainBuffer.buffer = encodeData + 1224 * 2 - lengthHeartBeatMsg - lengthGapMsg - shift;
			tmpPlainBuffer.buffer_size = lengthHeartBeatMsg + lengthGapMsg + shift;
		}
		else { //不加密子消息情况下使用bufffer,bufffer指向gap,需要往前偏移shift(rtpsHeader)
			tmpPlainBuffer.buffer_size = bufffer.buffer_size + shift;
			tmpPlainBuffer.buffer = bufffer.buffer - shift;
		}
		// tmpEncodeBuffer开辟对整个消息加密后空间
		tmpEncodeBuffer.buffer_size = tmpPlainBuffer.buffer_size * 10;
		tmpEncodeBuffer.buffer = new char[tmpEncodeBuffer.buffer_size];
		if (datawriter_->secPlugin_ && !datawriter_->secPlugin_->crypto_transform->encode_rtps_message(
			&tmpEncodeBuffer,
			&tmpPlainBuffer,
			1, //TODO
			participant->cryptoHdl_,
			partCryptoHdls,
			partCryptoHdlsCount,
			&secExce))
		{
			delete[]tmpEncodeBuffer.buffer;
			return -1;
		}
		// 存到encodeData
		memcpy(encodeData + 1224 * 2 - tmpEncodeBuffer.buffer_size, tmpEncodeBuffer.buffer, tmpEncodeBuffer.buffer_size);
		// encodeBuffer指向对应的数据
		encodeBufffer.buffer = encodeData + 1224 * 2 - tmpEncodeBuffer.buffer_size;
		encodeBufffer.buffer_size = tmpEncodeBuffer.buffer_size;
		delete[]tmpEncodeBuffer.buffer;
	}
	else { // 不Rtps加密
		if (encodeSubMsg) { // 加密子消息情况下使用encodeData
			encodeBufffer.buffer = encodeData + 1224 * 2 - lengthHeartBeatMsg - lengthGapMsg - sizeof(MessageHeader_t);;
			encodeBufffer.buffer_size = lengthHeartBeatMsg + lengthGapMsg + sizeof(MessageHeader_t);
		}
	}
#else
	MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix); //这里出参shift为rtpsHeader长度
#endif


#ifdef DDSSECURITY
	if (encodeSubMsg || encodeRtpsMsg) {
		participant_->TransmittersSendMessage(unicastList, encodeBufffer.buffer, encodeBufffer.buffer_size);
	}
	else {
		participant_->TransmittersSendMessage(unicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
	}
#else
	participant_->TransmittersSendMessage(unicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
#endif // DDSSECURITY
	//TODO 暂时屏蔽组播
	// messageTransmitter->MessageSend(srcLocators, multicastList, bufffer.buffer - shift, bufffer.buffer_size + shift);
	
	return RETCODE_OK;
}


ReturnCode_t StatefulWriter::WriteHeartFragbeat(std::shared_ptr<ReaderProxy> readerProxy)
{
	//TODO 
	TRAVODDS_LOG(LOG_ERROR, " not support interface");
	return RETCODE_UNSUPPORTED;
}

ReturnCode_t StatefulWriter::ReSendData(std::shared_ptr<ReaderProxy> readerProxy)
{
	SeqNumberVec gapVec;
	SequenceNumber_t lastGapseq;
	SeqNumberVec seqVec = readerProxy->RequestedChanges();
	std::vector<std::shared_ptr<ReaderProxy>> readerProxys;
	readerProxys.push_back(readerProxy);

	for(auto& seqNumber : seqVec)
	{
		std::shared_ptr<CacheChange> change = writerCache_->GetChange(seqNumber);
		
		if (!change.get()) {
			gapVec.push_back(seqNumber);
			continue;
		}
		/* 如果是分片报文，并且已经通过NackFrag报文请求数据，不在重复发送报文 */
		if (0 != change->fragmentTatolNum && readerProxy->IsNackFrag(seqNumber)) {
			continue;
		}
#ifdef DDSSECURITY
		if (guid_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
			for (auto& readerProxy : readerProxys) {
				DeliveryDataToReaderForVolatile(change, readerProxy, TIME_INFINITE);
			}
		}
		else {
			DeliveryDataToReader(change, readerProxys, TIME_INFINITE);
		}
#else
		DeliveryDataToReader(change, readerProxys, TIME_INFINITE);
#endif //DDSSECURITY
	}

	SendGap(gapVec, readerProxy);

	WriteHeartbeat(readerProxy, false);

	return RETCODE_OK;
}

void StatefulWriter::RemoveAllAckedData()
{
	auto seq = writerCache_->GetSeqNumMin();
	bool remove = true;
	while (IsAckedByAll(seq)) {
		writerCache_->RemoveChange(seq);
		seq = writerCache_->GetSeqNumMin();
		if (seq == SequenceNumber_t{ 0, 1 }) {
			break;
		}
	}
}

void StatefulWriter::SendGap(const SeqNumberVec& seqVec, std::shared_ptr<ReaderProxy> proxy)
{
	if (!proxy.get() || 0 == seqVec.size()) {
		return;
	}
	//TODO 
	GapSubMessage_t gapMsg;
	gapMsg.header.subMessageId = GAP;
	gapMsg.header.subMessageLength = SUB_MESSAGE_LEN_GAP; //TODO
	gapMsg.header.flags = IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;
	gapMsg.writerId = guid_.entityId;
	gapMsg.readerId = proxy->GetGuid().entityId;
	gapMsg.gapStart = seqVec[0];
	gapMsg.gapList.bitmapBase = gapMsg.gapStart + 1;
	int32_t i = 0;
	for (i = 1; i < seqVec.size(); i++) {
		if (seqVec[i] - gapMsg.gapList.bitmapBase > 0) {
			break;
		 }
		gapMsg.gapList.bitmapBase = seqVec[i] + 1;
	}
	for ( ; i < seqVec.size(); i++) {
		int32_t s = seqVec[i] - gapMsg.gapList.bitmapBase;
		if (s >= 255) {
			break;
		}
		gapMsg.gapList.bitmap.size = s + 1;
		gapMsg.gapList.bitmap.SetBit(s); 
	}

	LocatorVec dstLocators = proxy->GetUnicastLocatorList();

	uint32_t shift = 0;	
	SerializedBuffer bufffer;
	char data[1224] = { 0 }; //开辟消息空间
	bufffer.buffer_size = sizeof(HeartBeatSubMessage_t);
	bufffer.buffer = data;
	bufffer.buffer = bufffer.buffer + BUFF_SHIFT; //先指向data的1024偏移
	uint32_t lengthGapMsgBefore = 0;

#ifdef DDSSECURITY
	bool encodeSubMsg = datawriter_->statistic_.enableSec
		&& datawriter_->statistic_.secAttributes.is_submessage_protected
		&& guid_.IsUserDefinedEntity();
	bool encodeRtpsMsg = datawriter_->statistic_.enableSec
		&& datawriter_->statistic_.rtpsMsgProtected
		&& guid_.IsUserDefinedEntity();
	SerializedBuffer encodeBufffer; //加密情况发送的buffer
	SerializedBuffer tmpPlainBuffer;
	SerializedBuffer tmpEncodeBuffer; //临时存储加密后缓冲，拷贝到encodeData后释放
	char encodeData[1224 * 2] = { 0 }; //开辟加密消息空间
#endif // DDSSECURITY
	// 序列化Gap, bufffer指向Gap头部位置
	MessageProcessor::EncapsulateGapMessage(bufffer, shift, gapMsg);
	lengthGapMsgBefore = shift;
	bufffer.buffer_size = shift;
	shift = 0;

#ifdef DDSSECURITY
	int dataRet;
	//加密后的子消息长度
	uint32_t lengthGapMsg = 0;
	//加密heartbeat子消息
	if (encodeSubMsg) {
		tmpEncodeBuffer.buffer_size = bufffer.buffer_size * 10; //TODO 长度计算
		tmpEncodeBuffer.buffer = new char[tmpEncodeBuffer.buffer_size];
		//encode_datawriter_message
		dataRet = EncodeSubMsgBuffer(proxy, bufffer, tmpEncodeBuffer);
		lengthGapMsg = tmpEncodeBuffer.buffer_size;
		//拷贝到encodeData末尾
		memcpy(encodeData + 1224 * 2 - lengthGapMsg, tmpEncodeBuffer.buffer, lengthGapMsg);
		delete[] tmpEncodeBuffer.buffer;
	}
#endif // DDSSECURITY

#ifdef DDSSECURITY
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(datawriter_->GetParent()->get_participant());
	if (encodeSubMsg) { //如果之前加密了子消息
		// tmpPlainBuffer指向加密后的消息(包含Gap)
		tmpPlainBuffer.buffer_size = lengthGapMsg;
		tmpPlainBuffer.buffer = encodeData + 1224 * 2 - lengthGapMsg;
		MessageProcessor::EncapsulateMessageHeader(tmpPlainBuffer, shift, guid_.prefix); //这里出参shift为rtpsHeader长度
}
	else {
		MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix); //这里出参shift为rtpsHeader长度
	}
	if (encodeRtpsMsg) {
		ParticipantCryptoHandle* partCryptoHdls = NULL;
		uint32_t partCryptoHdlsCount = 0;
		DomainParticipantImpl* participant = dynamic_cast<DomainParticipantImpl*>(datawriter_->GetParent()->get_participant());
		partCryptoHdls = participant->secParticipants_.data();
		partCryptoHdlsCount = participant->secParticipants_.size();
		SecurityException secExce;
		if (encodeSubMsg) { // 加密子消息情况下使用encodeData
			tmpPlainBuffer.buffer = encodeData + 1224 * 2  - lengthGapMsg - shift;
			tmpPlainBuffer.buffer_size = lengthGapMsg + shift;
		}
		else { //不加密子消息情况下使用bufffer,bufffer指向gap,需要往前偏移shift(rtpsHeader)
			tmpPlainBuffer.buffer_size = bufffer.buffer_size + shift;
			tmpPlainBuffer.buffer = bufffer.buffer - shift;
		}
		// tmpEncodeBuffer开辟对整个消息加密后空间
		tmpEncodeBuffer.buffer_size = tmpPlainBuffer.buffer_size * 10;
		tmpEncodeBuffer.buffer = new char[tmpEncodeBuffer.buffer_size];
		if (datawriter_->secPlugin_ && !datawriter_->secPlugin_->crypto_transform->encode_rtps_message(
			&tmpEncodeBuffer,
			&tmpPlainBuffer,
			1, //TODO
			participant->cryptoHdl_,
			partCryptoHdls,
			partCryptoHdlsCount,
			&secExce))
		{
			delete[]tmpEncodeBuffer.buffer;
			return;
		}
		// 存到encodeData
		memcpy(encodeData + 1224 * 2 - tmpEncodeBuffer.buffer_size, tmpEncodeBuffer.buffer, tmpEncodeBuffer.buffer_size);
		// encodeBuffer指向对应的数据
		encodeBufffer.buffer = encodeData + 1224 * 2 - tmpEncodeBuffer.buffer_size;
		encodeBufffer.buffer_size = tmpEncodeBuffer.buffer_size;
		delete[]tmpEncodeBuffer.buffer;
	}
	else { // 不Rtps加密
		if (encodeSubMsg) { // 加密子消息情况下使用encodeData
			encodeBufffer.buffer = encodeData + 1224 * 2 - lengthGapMsg - sizeof(MessageHeader_t);;
			encodeBufffer.buffer_size = lengthGapMsg + sizeof(MessageHeader_t);
		}
	}
#else
	MessageProcessor::EncapsulateMessageHeader(bufffer, shift, guid_.prefix); //这里出参shift为rtpsHeader长度
#endif

#ifdef DDSSECURITY
	if (encodeSubMsg || encodeRtpsMsg) {
		participant_->TransmittersSendMessage(dstLocators, encodeBufffer.buffer, encodeBufffer.buffer_size);
	}
	else {
		participant_->TransmittersSendMessage(dstLocators, bufffer.buffer - shift, bufffer.buffer_size + shift);
	}
#else
	participant_->TransmittersSendMessage(dstLocators, bufffer.buffer - shift, bufffer.buffer_size + shift);
#endif // DDSSECURITY
	//participant_->TransmittersSendMessage(dstLocators, bufffer.buffer - shift, bufffer.buffer_size + shift);
}

/* 重传分片数据 */
ReturnCode_t StatefulWriter::ReSendDataFrag(std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& datafrag)
{
	ReturnCode_t ret = RETCODE_OK;

	/* 找到对应的序号的数据 */
	std::shared_ptr<CacheChange> cache =  writerCache_->GetChange(datafrag.writerSN);
	if (cache) {

		DataFragSubMessage_t dataFragSubMessage;

		if (0 != cache->inlineQos.size()) {
			dataFragSubMessage.header.flags |= SubMessageFlag::INLINE_QOS_FLAG;
			dataFragSubMessage.inlineQos = cache->inlineQos;
		}

		dataFragSubMessage.header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

		/* wangyi fastdds 适配测试默认16 */
		dataFragSubMessage.octetsToInlineQos = OCTETS_TO_INLINEQOS_LEN;
		dataFragSubMessage.extraFlags[0] = SubMessageFlag::NONE_FLAG;
		dataFragSubMessage.extraFlags[1] = SubMessageFlag::NONE_FLAG;
		dataFragSubMessage.readerId = ENTITYID_UNKNOWN;
		dataFragSubMessage.writerId = guid_.entityId;
		dataFragSubMessage.writerSN = cache->sequenceNumber;
		dataFragSubMessage.dataFrag.fragmentStartingNum = 0;

		SerializedBuffer* buffer = cache->serializedBuffer;
		std::vector<LocatorVec> dstLocators{readerProxy->GetUnicastLocatorList()};

        for(uint32_t i = 1; i < datafrag.fragmentNumberState.bitmapBase; i++)
        {
            if(nullptr != buffer){
                buffer = buffer->next;
            }
        }

        if(nullptr == buffer){
            TRAVODDS_LOG(LOG_ERROR, "buffer is nullptr");
			return RETCODE_ERROR;
        }

		/* TODO  */
		int32_t fragSendCount = 0;
		for (int32_t num = 0; (num + datafrag.fragmentNumberState.bitmapBase) <= cache->fragmentTatolNum && num < datafrag.fragmentNumberState.bitmap.size; num++) {
			if (datafrag.fragmentNumberState.bitmap.IsBitSet(num)) {

		        if(nullptr == buffer){
		            TRAVODDS_LOG(LOG_ERROR, "buffer is nullptr");
					return RETCODE_ERROR;
		        }
			
				uint32_t shift = 0;
				dataFragSubMessage.dataFrag.fragmentsInSubmessage = 1;
				dataFragSubMessage.dataFrag.fragmentSize = DATAMESSAGE_LIMIT;
				dataFragSubMessage.dataFrag.fragmentStartingNum = num + datafrag.fragmentNumberState.bitmapBase;
				dataFragSubMessage.dataFrag.sampleSize = cache->dataSize;
				MessageProcessor::EncapsulateDataFragMessage(*buffer, shift, dataFragSubMessage);
				
				MessageProcessor::EncapsulateMessageHeader(*buffer, shift, guid_.prefix);
				participant_->FlowRateLimitedMessageSend(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift, TIME_INFINITE);

				fragSendCount++;

				if (0 == fragSendCount % DATAFRAG_DELAY_NUM) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
			buffer = buffer->next;
		}
	}
	else {
		/*  数据因为某种原因被删除，需要发送Gap报文 */
		SeqNumberVec seqVec;
		seqVec.push_back(datafrag.writerSN);
		SendGap(seqVec, readerProxy);
	}

	return ret;
}

#ifdef DDSSECURITY
int StatefulWriter::EncodeSubMsgBuffer(std::shared_ptr<ReaderProxy> readerProxy, SerializedBuffer& buffer, SerializedBuffer& encodeBuffer)
{
	bool isEncode = datawriter_->statistic_.enableSec
		&& datawriter_->statistic_.secAttributes.is_submessage_protected
		&& (guid_.IsUserDefinedEntity() || guid_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER);
	if (!isEncode) {
		return -1;
	}
	SecurityException secExce;
	DatareaderCryptoHandle* rdCryptoHdls = NULL;
	uint32_t rdCryptoHdlsCount = 0;
	//if (epPtr != NULL)
	//{
	//    rdCryptoHdls = epPtr->rdCryptoHdl.data();
	//    rdCryptoHdlsCount = epPtr->rdCryptoHdl.size();
	//}
	//else
	{
		if (guid_.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
			if (readerProxy && datawriter_->secReadersMap_.count(readerProxy->GetGuid().prefix)) {
				rdCryptoHdls = &datawriter_->secReadersMap_[readerProxy->GetGuid().prefix];
				rdCryptoHdlsCount = 1;
			}
		}
		else {
			rdCryptoHdls = datawriter_->secReaders_.data();
			rdCryptoHdlsCount = datawriter_->secReaders_.size();
		}
	}
	if (datawriter_->secPlugin_ && !datawriter_->secPlugin_->crypto_transform->encode_datawriter_submessage(
		&encodeBuffer,
		&buffer,
		1,
		datawriter_->cryptoHdl_,
		rdCryptoHdls,
		rdCryptoHdlsCount,
		&secExce))
	{
		TRAVODDS_LOG(LOG_ERROR, "Failed to encode submessage."
			"\n  Code: %d"
			"\n  Minor code: %d"
			"\n  Message: %s",
			secExce.code,
			secExce.minor_code,
			secExce.message);
		return -1;
	}
	return 0;
}
#endif // DDSSECURITY
