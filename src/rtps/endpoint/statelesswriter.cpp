#include "rtps/endpoint/statelesswriter.h"
#include "rtps/endpoint/participant.h"
#include "rtps/messages/messageprocessor.h"
#include "rtps/transport/messagetransmitter.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "rtps/flowcontrol/flowcontroller.h"
#include "rtps/transport/locator.h"
#include "common/time/ddstimer.h"
#include "common/log/logger.h"
#ifdef DDSSECURITY
#include "rtps/discovery/discovery.h"
#include "security/securitycommon.h"
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/publish/publisherimpl.h"
#endif // DDSSECURITY

USING_TRAVODDS_NAMESPACE;


StatelessWriter::StatelessWriter(const DataWriterQos& qos, const GUID_t& guid, const TopicKind_t& kind)
	: Writer(qos, kind),  resendDataPeriod_(TIME_INFINITE), timer_(nullptr)
{
	guid_ = guid;
}

StatelessWriter::~StatelessWriter()
{

}
void StatelessWriter::MatchedReaderAdd(const DiscoveredReaderData &subData)
{
	std::lock_guard<std::mutex> lock(readerLocatorsMutex_);
	if (0 != readerLocators_.count(subData.remoteReaderGuid)) {
		return;
	}
	for (const auto& locator : subData.multicastLocatorList) {
		readerMulticastLocators_[locator].insert(subData.remoteReaderGuid);
	}
	std::shared_ptr<ReaderLocator> readerLocator = std::make_shared<ReaderLocator>(subData.remoteReaderGuid, subData.unicastLocatorList);
	readerLocator->SetHistoryCache(writerCache_);
	readerLocators_[subData.remoteReaderGuid] = readerLocator;
	transportGroup_.AddLocator(subData.remoteReaderGuid.prefix, subData.unicastLocatorList);
    if (qos_.durability.kind == TRANSIENT_LOCAL_DURABILITY_QOS &&
        subData.ddsSubscriptionData.durability.kind == TRANSIENT_LOCAL_DURABILITY_QOS) {
        auto sequence = writerCache_->GetSeqNumMin();
        const auto last = writerCache_->GetSeqNumMax();
        for (; sequence != SEQUENCENUMBER_UNKNOWN && sequence <= last; sequence++) {
            auto change = writerCache_->GetChange(sequence);
            if (!change || (change->nativePayload && change->UsesDataSharing(subData.remoteReaderGuid))) continue;
            if (change->nativePayload && change->EnsureSerializedPayload() != RETCODE_OK) continue;
            DeliveryDataToReader(change, qos_.reliability.max_blocking_time, &subData.remoteReaderGuid);
        }
    }
}

#ifdef DDSSECURITY
void StatelessWriter::MatchedReaderAddWithSecurity(const DiscoveredReaderData& subData, bool isSecure)
{
	DataWriterImpl* dw = datawriter_;
	if (!isSecure && dw->statistic_.secAttributes.is_discovery_protected && guid_.IsUserDefinedEntity()) { //开启保护但收到的是普通的发现数据
		return;
	}
	GUID_t dwGuid = Conversion::ToGuid(dw->get_instance_handle());
	GUID_t remoteDrGuid = Conversion::ToGuid(subData.ddsSubscriptionData.key);
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(dw->GetParent()->get_participant());
	if (dp->enableDDSSec_ &&
		!dwGuid.IsSecureBuiltinEntity() &&
		dwGuid.IsUserDefinedEntity()) {
		if (dw->statistic_.secAttributes.is_read_protected) {
			SecurityException secExce;
			PermissionsHandle remotePermissionsHandle = SEC_NIL_HANDLE;

			BuiltinTopicKey_t participant_key = subData.ddsSubscriptionData.participant_key;
			GuidPrefix_t remote_prefix = Conversion::ToGuid(participant_key).prefix;
			std::map<GuidPrefix_t, DiscoveryDPInfo*>& discParts = dp->GetDiscovery()->GetSpdp()->discParts_;
			auto dpInfoIter = discParts.find(remote_prefix);
			if (dpInfoIter == discParts.end())
			{
				remotePermissionsHandle = SEC_NIL_HANDLE;
			}
			if (dpInfoIter->second->enableDDSSec_)
			{
				remotePermissionsHandle = dpInfoIter->second->permissionsHdl_;
			}
			if (!dp->secPlugin_.access_control->check_remote_datareader(
				remotePermissionsHandle,
				dp->GetDomainId(),
				&subData.ddsSubscriptionData,
				false, // @todo: distinguish different type of reader
				&secExce))
			{
				TRAVODDS_LOG(LOG_ERROR, "check_remote_datareader failed.");
				return;
			}
		}
	}

	if (dp->enableDDSSec_ && dwGuid.IsUserDefinedEntity() && dw->statistic_.secAttributes.is_discovery_protected)
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

	// 安全扩展相关 crypto_key_factory
	ReaderInfo rdInfo;
	rdInfo.cryptoHdl = SEC_NIL_HANDLE;
	rdInfo.readyForSecComm = false;
	if (dw->statistic_.enableSec) {
		if (dw->statistic_.secAttributes.is_payload_protected
			|| dw->statistic_.secAttributes.is_submessage_protected) {
			SecurityException secExce;
			rdInfo.cryptoHdl = dw->secPlugin_->crypto_key_factory->register_matched_remote_datareader(
				dw->cryptoHdl_,
				dp->GetDiscovery()->GetSpdp()->GetRemoteParticipantCryptoHandle(&remoteDrGuid.prefix),
				dp->GetDiscovery()->GetSpdp()->GetRemoteParticipantSharedSecret(&remoteDrGuid.prefix),
				false, // @todo add more reader types
				&secExce);
			if (rdInfo.cryptoHdl == SEC_NIL_HANDLE) {
				TRAVODDS_LOG(LOG_ERROR, "Match Reader failed:  failed to register remote reader to key factory ");
				return;
			}
			//TOCHECK dw.statistic.writerType != DWT_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE
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
				dp->GetDiscovery()->GetSedp()->WriteParticipantVolatileSecureMessage(&msgReceiver, cryptoTokensSample);
				dw->secPlugin_->crypto_key_exchange->return_crypto_tokens(
					&cryptoTokensSample.messageData, &secExce);
				cryptoTokensSample.messageClassId = "";
			}
		}
		else
		{
			dw->secReaders_.push_back(rdInfo.cryptoHdl);
			dw->secReadersMap_[remoteDrGuid.prefix] = rdInfo.cryptoHdl;
			rdInfo.readyForSecComm = true;
		}
	}
	if (dw->statistic_.enableSec && !rdInfo.readyForSecComm)
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

void StatelessWriter::ReaderLocatorAdd(Locator_t& locator, const GUID_t& guid)
{
	AsioLocator asioLocator(locator);
	std::lock_guard<std::mutex> lock(readerLocatorsMutex_);
	if (asioLocator.IsMulticast()) {
		readerMulticastLocators_[locator].insert(guid);
	} else {
		auto itor = readerLocators_.find(guid);
		if (itor == readerLocators_.end()) {
			LocatorSeq locatorList{ locator };
			std::shared_ptr<ReaderLocator> readerLocator = std::make_shared<ReaderLocator>(guid, locatorList);
			readerLocators_[guid] = readerLocator;
			readerLocator->SetHistoryCache(writerCache_);
			return;
		}
		//itor->second->LocatorAdd(locator);
	}
}

void StatelessWriter::ReaderLocatorRemove(Locator_t& locator, const GUID_t& guid)
{
	AsioLocator asioLocator(locator);
	std::lock_guard<std::mutex> lock(readerLocatorsMutex_);
	if (asioLocator.IsMulticast()) {
		auto itor = readerMulticastLocators_.find(locator);
		if (itor != readerMulticastLocators_.end()) {
			itor->second.erase(guid);
			if (itor->second.empty()) {
				readerMulticastLocators_.erase(itor);
			}
		}
	} else {
		auto itor = readerLocators_.find(guid);
		if (itor == readerLocators_.end()) {
			return;
		}
		itor->second->LocatorRemove(locator);
	}
}

void StatelessWriter::ReaderLocatorRemove(const GUID_t& guid)
{
	std::lock_guard<std::mutex> lock(readerLocatorsMutex_);
	for (auto it = readerMulticastLocators_.begin(); it != readerMulticastLocators_.end(); ) {
		it->second.erase(guid);
		if (it->second.empty()) {
			it = readerMulticastLocators_.erase(it);
		} else {
			++it;
		}
	}

	auto itor = readerLocators_.find(guid);
	if (itor == readerLocators_.end()) {
		return;
	}
	readerLocators_.erase(itor);
    RemoveDataSharingReader(guid);
	transportGroup_.DecreaseLocator(guid.prefix);

}

void StatelessWriter::UnsentChangesReset()
{
	for (auto& itor : readerLocators_) {
		itor.second->ResetHighestSentChange();
	}
}

ReturnCode_t StatelessWriter::DeliveryData(std::shared_ptr<CacheChange> change, const Duration_t& timeOut)
{
    if (!change) return RETCODE_BAD_PARAMETER;
    std::lock_guard<std::mutex> lock(readerLocatorsMutex_);
    // Packet headers are encoded in the cache's headroom. Initial delivery,
    // periodic SPDP resend and discovery replay must not mutate/send it at once.
    if (!change->nativePayload) return DeliveryDataToReader(change, timeOut, nullptr);
    ReturnCode_t result = RETCODE_OK;
    for (const auto& reader : readerLocators_) {
        if (change->UsesDataSharing(reader.first)) continue;
        const auto serialized = change->EnsureSerializedPayload();
        if (serialized != RETCODE_OK) return serialized;
        const auto sent = DeliveryDataToReader(change, timeOut, &reader.first);
        if (sent != RETCODE_OK) result = sent;
    }
    return result;
}

ReturnCode_t StatelessWriter::DeliveryDataToReader(std::shared_ptr<CacheChange> change,
        const Duration_t& timeOut, const GUID_t* target)
{
	/* wnagyi  */
	Duration_t duration = TIME_INFINITE;
	if (qos_.liveliness.lease_duration < duration) {
		auto livelinessManager = participant_->GetLivelinessManager();
		if (livelinessManager != nullptr) {
			livelinessManager->GetLocalWriterLivelinessManager()->AssertLiveliness(GetGuid(),qos_.liveliness.kind);
		}
		else {
			/* TODO 日志 */
			TRAVODDS_LOG(LOG_ERROR, "livelinessManager is nullptr")
		}
	}

	if (readerLocators_.empty() && readerMulticastLocators_.empty()) {
		return RETCODE_OK;
	}

	//TODO 发送报文
	//srcLocators = participant_->GetDefaultUnicastLocators();
	//TODO 封装报文


	SerializedBuffer* buffer = change->serializedBuffer;

#ifdef DDSSECURITY
    bool encodeSubMsg = datawriter_->statistic_.enableSec
        && datawriter_->statistic_.secAttributes.is_submessage_protected
        && guid_.IsUserDefinedEntity();
    bool encodeRtpsMsg = datawriter_->statistic_.enableSec
        && datawriter_->statistic_.rtpsMsgProtected
        && guid_.IsUserDefinedEntity();
#endif // DDSSECURITY

	std::vector<LocatorVec> dstLocators;
	std::set<GuidPrefix_t> prefixs;
	for (auto& it : readerMulticastLocators_) {
		if (!target || it.second.count(*target)) dstLocators.push_back(LocatorSeq{ it.first });
	}
	for (auto& readerlocator : readerLocators_) {
        if (target && readerlocator.first != *target) continue;
		if (!change->filterReaders_.empty() && change->filterReaders_.find(readerlocator.first) != change->filterReaders_.end()) {
			continue;
		}
		prefixs.insert(readerlocator.first.prefix);
	}

	transportGroup_.GetLocator(prefixs, dstLocators);

	if (prefixs.empty() && dstLocators.empty()) {
		return RETCODE_OK;
	} else if (dstLocators.empty()) {
		return RETCODE_ERROR;
	}

	if (buffer->next) {
		DataFragSubMessage_t dataFragSubMessage;

		if (0 != change->inlineQos.size())
		{
			dataFragSubMessage.header.flags |= SubMessageFlag::INLINE_QOS_FLAG;
			dataFragSubMessage.inlineQos = change->inlineQos;
		}

		dataFragSubMessage.header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

		/* wangyi fastdds 适配测试默认16 */
		dataFragSubMessage.octetsToInlineQos = DATA_FRAG_OCTETS_TO_INLINEQOS_LEN;
		dataFragSubMessage.extraFlags[0] = SubMessageFlag::NONE_FLAG;
		dataFragSubMessage.extraFlags[1] = SubMessageFlag::NONE_FLAG;
		dataFragSubMessage.readerId = target ? target->entityId : ENTITYID_UNKNOWN;
		dataFragSubMessage.writerId = guid_.entityId;
		dataFragSubMessage.writerSN = change->sequenceNumber;
		dataFragSubMessage.dataFrag.fragmentStartingNum = 0;
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
			uint32_t encodeBufLen = buffer->buffer_size * 10; //TODO 长度公式
			if (encodeSubMsg || encodeRtpsMsg) {
				encodeBuf = new char[encodeBufLen];
			}
			encodeRtpsBuffer.buffer_size = encodeBufLen; //TODO 长度公式
			encodeRtpsBuffer.buffer = encodeBuf;

			//封装子消息
			uint32_t encodeShift = 0;
#endif // DDSSECURITY

			MessageProcessor::EncapsulateDataFragMessage(*buffer, shift, dataFragSubMessage);

			InfoTimestamp_t ints;
			ints.header.flags = IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;
			ints.timestamp = change->sourceTimespace;
			MessageProcessor::EncapsulateINFOTSMessageNow(*buffer, shift, ints);
			if (target != nullptr) {
			    InfoDestinationSubMessage_t destination;
			    destination.guidPrefix = target->prefix;
			    destination.header.flags = DEFAULT_ENDIAN;
			    MessageProcessor::EncapsulateINFODSTMessage(*buffer, shift, destination);
			}
	
#ifdef DDSSECURITY
			//加密子消息
			SerializedBuffer srcBuffer;
			SerializedBuffer encodeBuffer;
			srcBuffer.buffer = buffer->buffer - shift;
			srcBuffer.buffer_size = buffer->buffer_size + shift;
			int dataRet = 0;
			// 子消息加密后长度 包含DataFrag头部，DataFrag其他信息，DataFrag负载的长度
			uint32_t encodeDataSize = 0;
			if (encodeSubMsg) {
				encodeBuffer.buffer_size = srcBuffer.buffer_size * 10; //TODO 分配子消息加密后空间
				encodeBuffer.buffer = new char[encodeBuffer.buffer_size];
				//encode_datawriter_message
				dataRet = EncodeSubMsgBuffer(srcBuffer, encodeBuffer);
				if (dataRet != 0) {
					TRAVODDS_LOG(LOG_ERROR, "Failed to encode submessage.");
					delete[] encodeBuffer.buffer;
					return RETCODE_ERROR;
				}
				//加密后的子消息放到encodeRtpsBuffer（encodeBuf）的缓冲区尾部
				memcpy(encodeRtpsBuffer.buffer + encodeRtpsBuffer.buffer_size - encodeBuffer.buffer_size,
					encodeBuffer.buffer,
					encodeBuffer.buffer_size);
				encodeDataSize = encodeBuffer.buffer_size;
				delete[] encodeBuffer.buffer;
			}

			//封装Header
			if (encodeSubMsg) {
				encodeBuffer.buffer = encodeRtpsBuffer.buffer
					+ encodeRtpsBuffer.buffer_size
					- encodeDataSize;
				encodeBuffer.buffer_size = encodeDataSize;
				MessageProcessor::EncapsulateMessageHeader(encodeBuffer, encodeShift, guid_.prefix);
			}
			else {
				MessageProcessor::EncapsulateMessageHeader(*buffer, shift, guid_.prefix);
			}
#else
			MessageProcessor::EncapsulateMessageHeader(*buffer, shift, guid_.prefix);	
#endif // DDSSECURITY		
#ifdef DDSSECURITY
			if (encodeRtpsMsg) {
				ParticipantCryptoHandle* partCryptoHdls = NULL;
				uint32_t partCryptoHdlsCount = 0;
				DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(datawriter_->GetParent()->get_participant());
				partCryptoHdls = dp->secParticipants_.data();
				partCryptoHdlsCount = static_cast<uint32_t>(dp->secParticipants_.size());
				SecurityException secExce;

				//SerializedBuffer encodeBuffer;
				SerializedBuffer plainBuffer;
				//被加密的缓冲 
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
				//encodeBuffer存储被rtps加密报文的缓冲区
				encodeBuffer.buffer = new char[buffer->buffer_size * 10];
				encodeBuffer.buffer_size = buffer->buffer_size * 10;
				if (datawriter_->secPlugin_ && !datawriter_->secPlugin_->crypto_transform->encode_rtps_message(
					&encodeBuffer,
					&plainBuffer,
					1, //TODO
					dp->cryptoHdl_,
					partCryptoHdls,
					partCryptoHdlsCount,
					&secExce))
				{
					delete[]encodeBuffer.buffer;
					return RETCODE_ERROR;
				}
				//将encodeBuffer拷贝到encodeRtpsBuffer
				encodeRtpsBuffer.buffer = encodeRtpsBuffer.buffer + encodeRtpsBuffer.buffer_size - encodeBuffer.buffer_size;
				encodeRtpsBuffer.buffer_size = encodeBuffer.buffer_size;
				memcpy(encodeRtpsBuffer.buffer, encodeBuffer.buffer, encodeBuffer.buffer_size);
				delete[]encodeBuffer.buffer;
			}
#endif // DDSSECURITY

			ReturnCode_t ret = RETCODE_OK;
#ifdef DDSSECURITY
			if (encodeSubMsg && !encodeRtpsMsg) {
				participant_->FlowRateLimitedMessageSend(dstLocators, encodeBuffer.buffer - encodeShift, encodeDataSize + encodeShift, timeOut);
			}
			else if (encodeRtpsMsg) {
				participant_->FlowRateLimitedMessageSend(dstLocators, encodeRtpsBuffer.buffer, encodeRtpsBuffer.buffer_size, timeOut);
			}
			else {
				participant_->FlowRateLimitedMessageSend(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift, timeOut);
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
#endif // DDSSECURITY

			buffer = buffer->next;

		} while (buffer);
	} 
	else {
		DataSubMessage_t dataSubMessage;
		if (0 != change->serializedBuffer->buffer_size) {
			dataSubMessage.header.flags |= SubMessageFlag::DATA_FLAG;
		}

		if (0 != change->inlineQos.size())
		{
			dataSubMessage.header.flags |= SubMessageFlag::INLINE_QOS_FLAG;
			dataSubMessage.inlineQos = change->inlineQos;
		}

		dataSubMessage.header.flags |= IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;

		/* wangyi fastdds 适配测试默认16 */
		dataSubMessage.octetsToInlineQos = OCTETS_TO_INLINEQOS_LEN;
		dataSubMessage.extraFlags[0] = SubMessageFlag::NONE_FLAG;
		dataSubMessage.extraFlags[1] = SubMessageFlag::NONE_FLAG;
		dataSubMessage.readerId = target ? target->entityId : ENTITYID_UNKNOWN;
		dataSubMessage.writerId = guid_.entityId;
		dataSubMessage.writerSN = change->sequenceNumber;

		uint32_t shift = 0;
		
#ifdef DDSSECURITY
        SerializedBuffer encodeRtpsBuffer; //存储加密后缓存
        char* encodeBuf = NULL; 
        if (encodeSubMsg || encodeRtpsMsg) {
            encodeBuf = new char[buffer->buffer_size * 10];
        }
        encodeRtpsBuffer.buffer_size = buffer->buffer_size * 10; //TODO 长度公式
        encodeRtpsBuffer.buffer = encodeBuf;
		// 对于临时开辟存储加密后的缓冲Buffer的偏移，这里主要记录子消息加密情况下，封装header的偏移
        uint32_t encodeShift = 0;
#endif

        MessageProcessor::EncapsulateDATAMessage(*change->serializedBuffer, shift, dataSubMessage);

		InfoTimestamp_t ints;
		ints.header.flags = IS_LITTLE_ENDIAN ? SubMessageFlag::ENDIANNESS_FLAG : SubMessageFlag::NONE_FLAG;
		ints.timestamp = change->sourceTimespace;
		MessageProcessor::EncapsulateINFOTSMessageNow(*change->serializedBuffer, shift, ints);
		if (target != nullptr) {
		    InfoDestinationSubMessage_t destination;
		    destination.guidPrefix = target->prefix;
		    destination.header.flags = DEFAULT_ENDIAN;
		    MessageProcessor::EncapsulateINFODSTMessage(*change->serializedBuffer, shift, destination);
		}
#ifdef DDSSECURITY
        // srcBuffer指向被加密子消息
        SerializedBuffer srcBuffer;
        // encodeBuffer为临时开辟缓冲区，存储加密后子消息
        SerializedBuffer encodeBuffer;
        srcBuffer.buffer = change->serializedBuffer->buffer - shift;
        srcBuffer.buffer_size = change->serializedBuffer->buffer_size + shift;;
        int dataRet = 0;
        // 子消息加密后长度 包含Data头部，Data其他信息，Data负载的长度
        uint32_t encodeDataSize = 0;
        //子消息加密 encodeRtpsBuffer包含DATA头和DATA其他信息
        if (encodeSubMsg) {
            encodeBuffer.buffer_size = srcBuffer.buffer_size * 10; //TODO 分配子消息加密后空间
            encodeBuffer.buffer = new char[encodeBuffer.buffer_size];
            //encode_datawriter_message
            dataRet = EncodeSubMsgBuffer(srcBuffer, encodeBuffer);
			if (dataRet != 0) {
				TRAVODDS_LOG(LOG_ERROR, "Failed to encode submessage.");
				delete[] encodeBuffer.buffer;
				return RETCODE_ERROR;
			}
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
            MessageProcessor::EncapsulateMessageHeader(*change->serializedBuffer, shift, guid_.prefix);
        }
#else
		MessageProcessor::EncapsulateMessageHeader(*change->serializedBuffer, shift, guid_.prefix);
#endif // DDSSECURITY

		
#ifdef DDSSECURITY
		if (encodeRtpsMsg) {
			ParticipantCryptoHandle* partCryptoHdls = NULL;
			uint32_t partCryptoHdlsCount = 0;
			DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(datawriter_->GetParent()->get_participant());
			partCryptoHdls = dp->secParticipants_.data();
			partCryptoHdlsCount = static_cast<uint32_t>(dp->secParticipants_.size());
			SecurityException secExce;

			//SerializedBuffer encodeBuffer;
			SerializedBuffer plainBuffer;
			//被加密的缓冲 
			//int msgHeaderSize = sizeof(MessageHeader_t);
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
					dp->cryptoHdl_,
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
			participant_->FlowRateLimitedMessageSend(dstLocators, encodeBuffer.buffer - encodeShift, encodeDataSize + encodeShift, timeOut);
		}
		else if (encodeRtpsMsg) {
			participant_->FlowRateLimitedMessageSend(dstLocators, encodeRtpsBuffer.buffer, encodeRtpsBuffer.buffer_size, timeOut);
		}
		else {
			participant_->FlowRateLimitedMessageSend(dstLocators, buffer->buffer - shift, buffer->buffer_size + shift, timeOut);
		}

		if (encodeBuf) {
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
#endif // DDSSECURITY
	}
	return RETCODE_OK;
}

void StatelessWriter::SetResendDataPeriod(Duration_t period)
{
	resendDataPeriod_ = period;

	Time_t time;
	time.sec = period.sec;
	time.nanosec = period.nanosec;
	if (time != Time_t(TIME_INFINITE)) {
		std::string n = "slw" + std::to_string(guid_.entityId.entityKind);

		timer_ = TimerManager::CreateTimer(time, [&]() { this->ResendDataPeriod(); }, n);
	}
}

void StatelessWriter::ResendDataPeriod()
{
	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();
	SequenceNumber_t maxSeq = writerCache_->GetSeqNumMax();

	for (; minSeq <= maxSeq; minSeq++) {
		std::shared_ptr<CacheChange> change = writerCache_->GetChange(minSeq);
		if (!change) {
			continue;
		}
		DeliveryData(change, TIME_INFINITE);
	}
}


ReturnCode_t StatelessWriter::ReceiveAckNackMessage(const Receive_t&  headerMsg, const AcknackSubMessage_t& ackMsg)
{
	static_cast<void>(headerMsg);
	static_cast<void>(ackMsg);

	return RETCODE_OK;
}

ReturnCode_t StatelessWriter::ReceiveNackFragMessage(const Receive_t&  headerMsg, const NackFragSubMessage_t& nackMsg)
{
	static_cast<void>(headerMsg);
	static_cast<void>(nackMsg);

	return RETCODE_OK;
}

#ifdef DDSSECURITY
int StatelessWriter::EncodeSubMsgBuffer(SerializedBuffer& buffer, SerializedBuffer& encodeBuffer)
{
	bool isEncode = datawriter_->statistic_.enableSec
		&& datawriter_->statistic_.secAttributes.is_submessage_protected
		&& guid_.IsUserDefinedEntity();
	if (!isEncode) {
		return -1;
	}
	SecurityException secExce;
	DatareaderCryptoHandle* rdCryptoHdls = NULL;
	uint32_t rdCryptoHdlsCount = 0;

	rdCryptoHdls = datawriter_->secReaders_.data();
	rdCryptoHdlsCount = static_cast<uint32_t>(datawriter_->secReaders_.size());

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
