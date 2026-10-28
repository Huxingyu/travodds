#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/statefulwriter.h"
#include "rtps/endpoint/statelesswriter.h"
#include "rtps/endpoint/statefulreader.h"
#include "rtps/endpoint/statelessreader.h"
#include "rtps/transport/messagetransmitterudp.h"
#include "rtps/transport/messagetransmittertcp.h"
#include "rtps/transport/messagetransmittershm.h"
#include "rtps/messages/messageprocessor.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "rtps/transport/ipcontainer.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "rtps/builtin/dpr/dynamicperformanceregulator.h"
#include "rtps/flowcontrol/flowcontrolmanager.h"
#include "rtps/flowcontrol/flowratelimitedmanager.h"
#include "common/log/logger.h"
#include <regex>
#ifdef DDSSECURITY
#include "security/securitycommon.h"
#include "rtps/discovery/discovery.h"
#include "dcps/subscribe/subscriberimpl.h"
#include "dcps/publish/publisherimpl.h"
#endif // DDSSECURITY


USING_TRAVODDS_NAMESPACE;

// Participant::Participant(const DomainParticipantImpl* domainImpl)
// 	:domainImpl_(domainImpl)
// {
// 	readerId_ = 0;
// 	writerId_ = 0;
// 	nextId_ = 0;
//
// 	//protocolVersion_ = 0
// }

Participant::Participant(DomainId_t id, const GUID_t& guid,std::shared_ptr<ThreadPool> threadPool)
	: protocolVersion_(PROTOCOLVERSION),
	  vendorId_(VENDORID_PROSIMA),
      readerId_(0), writerId_(0), nextId_(0), domainId_(id),
	  flowRateLimitedManager_(nullptr),
	  flowControlManager_(nullptr),
	  threadPool_(threadPool)
{
	guid_ = guid;

	Time_t heartBeatPeriod = { 0, 200 * 1000 * 1000 };
	hbTimer_ = TimerManager::CreateTimer(heartBeatPeriod, 
	    [&, this]() {
			std::vector<std::shared_ptr<Writer>> reliable_writers;
			{
				std::lock_guard<std::mutex> lock(mtxWriters_);
				for (auto& it : writers_) {
					if (it.second->GetQos().reliability.kind == RELIABLE_RELIABILITY_QOS) {
						reliable_writers.push_back(it.second);
					}
				}
			}
			for (auto& writer : reliable_writers) {
				std::static_pointer_cast<StatefulWriter>(writer)->WriteHeartbeat();
			}
	    }, 
	    "hb");
}

Participant::~Participant()
{
	if (hbTimer_) {
		hbTimer_->Exit();
	}
	StopReceiveThread();

	//if (userDataRecvThread_.joinable()) {
	//	userDataRecvThread_.join();
	//}

	//if (multiCastRecvThread_.joinable()) {
	//	multiCastRecvThread_.join();
	//}

}

ReturnCode_t Participant::Init(const DomainParticipantQos& qos)
{
	qos_ = qos;

	FlowParameters flowParameters;
	//20260713 
	flowParameters.maxBytesPerPeriod  = qos.flow_parameters.maxBytesPerPeriod;
	flowParameters.periodMs = qos.flow_parameters.periodMs;

	flowRateLimitedManager_ = std::make_unique<FlowRateLimitedManager>(this, flowParameters);
	flowControlManager_ = std::make_unique<FlowControlManager>(this);
	regulator_ = std::make_unique<DynamicPerformanceRegulator>(qos.dynamic_adjust, flowRateLimitedManager_.get());

	int32_t participantid = 0;

	uint32_t userPort = USER_DEFAULT_UNICAST_PORT(domainId_, participantid);
	uint32_t spdpPort = SPDP_WELL_KNOWN_UNICAST_PORT(domainId_, participantid);

	/* wangyi */
	livelinessManager_ = std::make_shared<LivelinessManager>(this);

	messageProcessor_ = std::make_shared<MessageProcessor>(this);
	/* MessageTransmitter添加顺序即为传输器发送优先级 */
	if (qos_.transport.hasTransport(TRANSPORT_SHM_QOS)) {
		messageTransmitters_.push_back(std::make_shared<MessageTransmitterSHM>(messageProcessor_));
		Locator_t loc;
		loc.kind = LOCATOR_KIND_SHM;
		loc.port = userPort;
		memcpy(loc.address, MessageTransmitterSHM::GetAddress().data(), sizeof(Locator_t::address));
		defaultUnicastLocatorList_.push_back(loc);
	}
	if (qos_.transport.hasTransport(TRANSPORT_UDPv4_QOS) || qos_.discovery.kind == DISCOVER_UDPv4_QOS || qos_.discovery.kind == DISCOVER_UDPv4_AND_UDPv6_QOS) {
		messageTransmitters_.push_back(std::make_shared<MessageTransmitterUDPv4>(messageProcessor_));
	}
	if (qos_.transport.hasTransport(TRANSPORT_UDPv6_QOS) || qos_.discovery.kind == DISCOVER_UDPv6_QOS || qos_.discovery.kind == DISCOVER_UDPv4_AND_UDPv6_QOS) {
		messageTransmitters_.push_back(std::make_shared<MessageTransmitterUDPv6>(messageProcessor_));
	}
	if (qos_.transport.hasTransport(TRANSPORT_TCPv4_QOS)) {
		messageTransmitters_.push_back(std::make_shared<MessageTransmitterTCPv4>(messageProcessor_));
	}
	if (qos_.transport.hasTransport(TRANSPORT_TCPv6_QOS)) {
		messageTransmitters_.push_back(std::make_shared<MessageTransmitterTCPv6>(messageProcessor_));
	}

	std::vector<InfoIP_t> localIps;
	IpContainer::GetInfoIPs(localIps);

	/* 若是指定网卡，过滤未选中的网卡 */
	if (!qos_.network_interface.interface_whitelist.empty()) {
		std::vector<InfoIP_t> filteredIps;
		for (const auto& pattern : qos_.network_interface.interface_whitelist) {
			for (const auto& ip : localIps) {
				if (pattern == ip.dev) {
					filteredIps.push_back(ip);
				}
			}
		}
		if (!filteredIps.empty()) {
			localIps = filteredIps;
		} else {
			TRAVODDS_LOG(LOG_WARNING, "No matching network interfaces found for the specified whitelist.");
		}
	}

	for (auto& ip : localIps) {
		Locator_t ipLoc;
		memcpy(ipLoc.address, ip.address, sizeof(Locator_t::address));
		if (ip.type == IPV4) {
			IPv4LocatorList_.push_back(ipLoc);
			ipLoc.kind = LOCATOR_KIND_UDPv4;
			if (qos_.discovery.kind == DISCOVER_UDPv4_QOS || qos_.discovery.kind == DISCOVER_UDPv4_AND_UDPv6_QOS) {
				ipLoc.port = spdpPort;
				metatrafficUnicastLocatorList_.push_back(ipLoc);
			}
			ipLoc.port = userPort;
			if (qos_.transport.hasTransport(TRANSPORT_UDPv4_QOS)) {

				defaultUnicastLocatorList_.push_back(ipLoc);
			}
			if (qos_.transport.hasTransport(TRANSPORT_TCPv4_QOS)) {
				ipLoc.kind = LOCATOR_KIND_TCPv4;
				defaultUnicastLocatorList_.push_back(ipLoc);
			}
		} else if (ip.type == IPV6){
			IPv6LocatorList_.push_back(ipLoc);
			ipLoc.kind = LOCATOR_KIND_UDPv6;
			if (qos_.discovery.kind == DISCOVER_UDPv6_QOS || qos_.discovery.kind == DISCOVER_UDPv4_AND_UDPv6_QOS) {
				ipLoc.port = spdpPort;
				metatrafficUnicastLocatorList_.push_back(ipLoc);
			}
			ipLoc.port = userPort;
			if (qos_.transport.hasTransport(TRANSPORT_UDPv6_QOS)) {
				defaultUnicastLocatorList_.push_back(ipLoc);
			}
			if (qos_.transport.hasTransport(TRANSPORT_TCPv6_QOS)) {
				ipLoc.kind = LOCATOR_KIND_TCPv6;
				defaultUnicastLocatorList_.push_back(ipLoc);
			}
		} else {
			continue;
		}
		if (qos_.transport.hasTransport(TransportQosPolicyKind(TRANSPORT_UDPv4_QOS | TRANSPORT_TCPv4_QOS | TRANSPORT_TCPv6_QOS | TRANSPORT_UDPv6_QOS))) {
			regulator_->RegisterInterface(ip.dev);
		}
	}
	return RETCODE_OK;
}

ReturnCode_t Participant::InitNetwork()
{
	uint32_t multicPort = SPDP_WELL_KNOWN_MULTICAST_PORT(domainId_);
	for (auto& messageTransmitter : messageTransmitters_) {
		Locator_t unicastAddress;
		//网络传输模块对象初始化发送地址、接收地址

		unicastAddress.kind = messageTransmitter->GetTransmitterKind();

		if (unicastAddress.kind == LOCATOR_KIND_UDPv6 || unicastAddress.kind == LOCATOR_KIND_TCPv6) {
			messageTransmitter->SetUnicastLocatorList(IPv6LocatorList_);
		} else {
			messageTransmitter->SetUnicastLocatorList(IPv4LocatorList_);
		}

		CreateReceiverResources(messageTransmitter, defaultUnicastLocatorList_, true);

		if (unicastAddress.kind != LOCATOR_KIND_UDPv4 && unicastAddress.kind != LOCATOR_KIND_UDPv6) {
			continue;
		}

		messageTransmitter->SendAddrAdd(unicastAddress, unicastAddress);
		CreateReceiverResources(messageTransmitter, metatrafficUnicastLocatorList_, true);


		/* UDPv4、UDPv6 组播设置 */
		Locator_t multicastAaddresses;
		multicastAaddresses.port = multicPort;
		if (LOCATOR_KIND_UDPv4 == messageTransmitter->GetTransmitterKind() && (qos_.discovery.kind == DISCOVER_UDPv4_QOS || qos_.discovery.kind == DISCOVER_UDPv4_AND_UDPv6_QOS)) {
			multicastAaddresses.kind = LOCATOR_KIND_UDPv4;
			multicastAaddresses.address[12] = 239;
			multicastAaddresses.address[13] = 255;
			multicastAaddresses.address[14] = 0;
			multicastAaddresses.address[15] = 1;
			messageTransmitter->ReceiveAddrAdd(multicastAaddresses);
			metatrafficMulticastLocatorList_.push_back(multicastAaddresses);
		} else if(LOCATOR_KIND_UDPv6 == messageTransmitter->GetTransmitterKind() && (qos_.discovery.kind == DISCOVER_UDPv6_QOS || qos_.discovery.kind == DISCOVER_UDPv4_AND_UDPv6_QOS)) {

			multicastAaddresses.kind = LOCATOR_KIND_UDPv6;
			multicastAaddresses.address[0] = 0xff;
			multicastAaddresses.address[1] = 0x1e;
			//3-9 : 0
			multicastAaddresses.address[10] = 0xff;
			multicastAaddresses.address[11] = 0xff;
			multicastAaddresses.address[12] = 0xef;
			multicastAaddresses.address[13] = 0xff;
			multicastAaddresses.address[15] = 0x01;
			messageTransmitter->ReceiveAddrAdd(multicastAaddresses);
			metatrafficMulticastLocatorList_.push_back(multicastAaddresses);
		}
	}

	return RETCODE_OK;
}
std::shared_ptr<Writer> Participant::CreateWriter(DataWriterImpl* dw, const DataWriterQos & dwQos, const GUID_t& guid, bool key, FlowControllerConfig flowConfig)
{
	std::shared_ptr<Writer> writerPtr;
	std::shared_ptr<Endpoint> endpointPtr;
	TopicKind_t kind;

	kind.value = key ? WITH_KEY : NO_KEY;
	//guid = GenerateGuid(EntityID, key, ENTITY_WRITER);

	if (dwQos.reliability.kind == BEST_EFFORT_RELIABILITY_QOS) {
		writerPtr = std::make_shared<StatelessWriter>(dwQos, guid, kind);
	} else {
		writerPtr = std::make_shared<StatefulWriter>(dwQos, guid, kind);
	}
	endpointPtr = writerPtr;
	endpointPtr->SetParticipant(this);
	
	FlowController* controller = flowControlManager_->GetFlowController(flowConfig);
	writerPtr->SetFlowController(controller);
	controller->RegisterWriter(writerPtr);

    dw->SetRelatedWriter(writerPtr);

	{
		std::unique_lock<std::mutex> lock(mtxEndpoints_);
		endpoints_[guid.entityId] = endpointPtr;
	}
  
	{
		std::unique_lock<std::mutex> lock(mtxWriters_);
		writers_[guid.entityId] = writerPtr;
		AddLocalWriter(writerPtr);
	}
	

	/* wangyi 添加writer */
	if (writerPtr->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER &&
		writerPtr->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER &&
		writerPtr->GetGuid().entityId != ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER &&
		writerPtr->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER 
#ifdef DDSSECURITY
		&& writerPtr->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER
        && writerPtr->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER
        && writerPtr->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER
        && writerPtr->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER
        && writerPtr->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER
#endif // DDSSECURITY
	){
		livelinessManager_->AddLivelinessWriter(writerPtr, dwQos.liveliness);
	}

	return writerPtr;
}

std::shared_ptr<Reader> Participant::CreateReader(DataReaderImpl* dr, const DataReaderQos & drQos, const GUID_t& guid, bool key)
{
	std::shared_ptr<Reader> readerPtr;
	std::shared_ptr<Endpoint> endpointPtr;
	TopicKind_t kind;

	kind.value = key ? WITH_KEY : NO_KEY;

	//guid = GenerateGuid(EntityID, key, ENTITY_READER);

	if (drQos.reliability.kind == BEST_EFFORT_RELIABILITY_QOS) {
		readerPtr = std::make_shared<StatelessReader>(drQos, guid, kind);
	} else {
		readerPtr = std::make_shared<StatefulReader>(drQos, guid, kind);
	}
	endpointPtr = readerPtr;
	endpointPtr->SetParticipant(this);

    dr->SetRelatedReader(readerPtr);
	{
		std::unique_lock<std::mutex> lock(mtxEndpoints_);
		endpoints_[guid.entityId] = endpointPtr;
	}
	{
		std::unique_lock<std::mutex> lock(mtxReaders_);
		readers_[guid.entityId] = readerPtr;
	}
	

	/* wangyi 添加reader */
	if (readerPtr->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER &&
		readerPtr->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER &&
		readerPtr->GetGuid().entityId != ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER &&
		readerPtr->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER
#ifdef DDSSECURITY
		&& readerPtr->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER
        && readerPtr->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER
        && readerPtr->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER
#endif // DDSSECURITY
	){
		livelinessManager_->AddLivelinessReader(readerPtr, drQos.liveliness);
	}

	return readerPtr;
}


ReturnCode_t Participant::DeleteWriter(const InstanceHandle_t& handle)
{

	GUID_t guid = Conversion::ToGuid(handle);

	std::unique_lock<std::mutex> lock(mtxWriters_);
	auto itor = writers_.find(guid.entityId);
	if (itor != writers_.end()) {
		/* wangyi 删除writer */
		if (itor->second->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER &&
			itor->second->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER &&
			itor->second->GetGuid().entityId != ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER &&
			itor->second->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER
#ifdef DDSSECURITY
			&& itor->second->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER
			&& itor->second->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER
			&& itor->second->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER
			&& itor->second->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER
			&& itor->second->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER
#endif // DDSSECURITY
			) {
			livelinessManager_->RemoveLivelinessWriter(itor->second);
		}
		itor->second->GetFlowController()->UnregisterWriter(itor->second);
		writers_.erase(itor);
	} else {
		//已经通过sedp通知节点退出，dr和dw之间还有数据缓存处理，正常流程，不应该算做错误
		//TRAVODDS_LOG(LOG_DEBUG, "handle not exist");
		//return RETCODE_ERROR;
	}
	{
		std::unique_lock<std::mutex> plock(mtxEndpoints_);
		endpoints_.erase(guid.entityId);
	}
	

	return RETCODE_OK;
}
ReturnCode_t Participant::DeleteReader(const InstanceHandle_t& handle)
{
	GUID_t guid = Conversion::ToGuid(handle);

	std::unique_lock<std::mutex> lock(mtxReaders_);
	auto itor = readers_.find(guid.entityId);
	if (itor != readers_.end()) {
		/* wangyi 删除reader */
		if (itor->second->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER &&
			itor->second->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER &&
			itor->second->GetGuid().entityId != ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER &&
			itor->second->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER
#ifdef DDSSECURITY
			&& itor->second->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_PUBLICATIONS_SECURE_READER
			&& itor->second->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER
			&& itor->second->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER
			&& itor->second->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER
			&& itor->second->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER
#endif // DDSSECURITY
			) {
			livelinessManager_->RemoveLivelinessReader(itor->second);
		}
		readers_.erase(itor);
	} else {
		TRAVODDS_LOG(LOG_DEBUG, "handle not exist");
		return RETCODE_ERROR;
	}
	{
		std::unique_lock<std::mutex> plock(mtxEndpoints_);
		endpoints_.erase(guid.entityId);
	}
	
	return RETCODE_OK;
}

std::shared_ptr<Writer> Participant::GetWriter(const EntityId_t & EntityID)
{
	std::unique_lock<std::mutex> lock(mtxWriters_);
	
	if (0 == writers_.count(EntityID)) {
		TRAVODDS_LOG(LOG_DEBUG, "EntityID not exist");
		return nullptr;
	}
		
	return writers_[EntityID];
}

std::shared_ptr<Reader> Participant::GetReader(const EntityId_t & EntityID)
{
	std::unique_lock<std::mutex> lock(mtxReaders_);
	if (0 == readers_.count(EntityID)) {
		//AGILOG(AGILOG_ERROR, "EntityID not exist");
		return nullptr;
	}

	return readers_[EntityID];
}

#ifdef DDSSECURITY
ReturnCode_t Participant::AddRemoteWriter(const DiscoveredWriterData& data, bool isSecure)
#else
ReturnCode_t Participant::AddRemoteWriter(const DiscoveredWriterData& data)
#endif // DDSSECURITY
{
	std::unique_lock<std::mutex> lock(mtxReaders_);
	// Publish queryable metadata before exposing a match or invoking its
	// callback. The short matching lock also protects concurrent lookups;
	// callbacks must never need to reacquire the Reader table lock.
	{
		std::lock_guard<std::mutex> metadata(mtxMatchEntitys_);
		remoteWriters_.insert_or_assign(data.remoteWriterGuid, data);
	}
	for (auto& reader : readers_) {
#ifdef DDSSECURITY
		addReaderMatch(reader.second, data, isSecure);
#else
		addReaderMatch(reader.second, data);
#endif // DDSSECURITY
	}
	return RETCODE_OK;
}

#ifdef DDSSECURITY
ReturnCode_t Participant::AddRemoteReader(const DiscoveredReaderData& data, bool isSecure)
#else
ReturnCode_t Participant::AddRemoteReader(const DiscoveredReaderData& data)
#endif // DDSSECURITY
{
	std::unique_lock<std::mutex> lock(mtxWriters_);
	{
		std::lock_guard<std::mutex> metadata(mtxMatchEntitys_);
		remoteReaders_.insert_or_assign(data.remoteReaderGuid, data);
	}

	for (auto& writer : writers_) {
#ifdef DDSSECURITY
		addWriterMatch(writer.second, data, isSecure);
#else
		addWriterMatch(writer.second, data);
#endif // DDSSECURITY
	}	

	return RETCODE_OK;
}

ReturnCode_t Participant::RemoveRemoteWriter(GUID_t guid)
{
	DiscoveredWriterData dwData;
	{
		std::unique_lock<std::mutex> lock(mtxReaders_);
		std::lock_guard<std::mutex> metadata(mtxMatchEntitys_);
		auto itor = remoteWriters_.find(guid);
		if (itor == remoteWriters_.end()) {
			return RETCODE_OK;
		}
		dwData = std::move(itor->second);
		remoteWriters_.erase(itor);
	}

	// yuguxu 传输断开链接
	TransmittersDisconnect(dwData.unicastLocatorList);

	std::list<EntityId_t> readersEntityid;
	{
		std::unique_lock<std::mutex> lock(mtxMatchEntitys_);
		readersEntityid = matchLocalEntitys_[guid];
		matchLocalEntitys_.erase(guid);
		for(auto& readerEntityid : readersEntityid) {
			auto itor = matchRemoteEntitys_.find(readerEntityid);
			if (itor != matchRemoteEntitys_.end()) {
				itor->second.erase(guid);
				if (itor->second.empty()) {
					matchRemoteEntitys_.erase(itor);
				}
			}
		}
	} 
	
	for(auto& readerEntityid : readersEntityid) {
		std::unique_lock<std::mutex> lock(mtxReaders_);
		auto writerItor = readers_.find(readerEntityid);
		if (writerItor == readers_.end()) {
			continue;
		}
		auto reader = readers_[readerEntityid];
        lock.unlock();
		if (!reader) {
			continue;
		}
		std::shared_ptr<StatefulReader> statefulReader = std::dynamic_pointer_cast<StatefulReader>(reader);

		if(statefulReader) {
#ifdef DDSSECURITY
			DataReaderImpl *dataReader = statefulReader->GetRelatedDataReader();
			std::map<GUID_t, WriterSampleList*> writerSampleMap = dataReader->writerSampleList_;
			auto writerIdx = writerSampleMap.find(guid);
			if (writerIdx != writerSampleMap.end()) {
				WriterSampleList* writer = writerIdx->second;
				// 安全扩展相关
				if (dataReader->enableSec_ && writer->readyForSecComm)
				{
					for (auto secWrIter = dataReader->secWriters_.begin();
						secWrIter != dataReader->secWriters_.end();
						++secWrIter)
					{
						if (*secWrIter == writer->cryptoHdl)
						{
							dataReader->secWriters_.erase(secWrIter);
							break;
						}
					}
					if (dataReader->secAttributes_.is_payload_protected
						|| dataReader->secAttributes_.is_submessage_protected)
					{
						SecurityException secExce;
						dataReader->secPlugin_->crypto_key_factory->unregister_datawriter(
							writer->cryptoHdl, &secExce);
					}
				}
			}
#endif // DDSSECURITY
			std::shared_ptr<WriterProxy> writerProxy = statefulReader->MatchedWriterLookup(guid);
			if (writerProxy) {
				statefulReader->MatchedWriterRemove(writerProxy);
			}
			//TODO 重新检测确认数据
		}

		std::shared_ptr<StatelessReader> statelessReader = std::dynamic_pointer_cast<StatelessReader>(reader);

		if (statelessReader) {
#ifdef DDSSECURITY
			DataReaderImpl* dataReader = statelessReader->GetRelatedDataReader();
			std::map<GUID_t, WriterSampleList*> writerSampleMap = dataReader->writerSampleList_;
			auto writerIdx = writerSampleMap.find(guid);
			if (writerIdx != writerSampleMap.end()) {
				WriterSampleList* writer = writerIdx->second;
				// 安全扩展相关
				if (dataReader->enableSec_ && writer->readyForSecComm)
				{
					for (auto secWrIter = dataReader->secWriters_.begin();
						secWrIter != dataReader->secWriters_.end();
						++secWrIter)
					{
						if (*secWrIter == writer->cryptoHdl)
						{
							dataReader->secWriters_.erase(secWrIter);
							break;
						}
					}
					if (dataReader->secAttributes_.is_payload_protected
	
						|| dataReader->secAttributes_.is_submessage_protected)
					{
						SecurityException secExce;
						dataReader->secPlugin_->crypto_key_factory->unregister_datawriter(
							writer->cryptoHdl, &secExce);
					}
				}
			}
#endif // DDSSECURITY
			//TODO 补充
			std::shared_ptr<WriterProxy> writerProxy = statelessReader->MatchedWriterLookup(guid);
			if (writerProxy) {
				statelessReader->MatchedWriterRemove(writerProxy->GetGuid());
			}
		}

		InstanceHandle_t handle = Conversion::ToInstanceHandle(dwData.remoteWriterGuid);
        // MatchedWriterRemove may join the native listener; perform that join
        // before acquiring the ingress barrier for the DCPS callback.
        reader->NotifySubscriptionMatched(handle, false);
	}


	return RETCODE_OK;
}

ReturnCode_t Participant::RemoveRemoteReader(GUID_t guid)
{
	DiscoveredReaderData drData;
	{
		std::unique_lock<std::mutex> lock(mtxWriters_);
		std::lock_guard<std::mutex> metadata(mtxMatchEntitys_);
		auto itor = remoteReaders_.find(guid);
		if (itor == remoteReaders_.end()) {
			return RETCODE_OK;
		}
		drData = std::move(itor->second);
		remoteReaders_.erase(itor);
	}


	// yuguxu 传输断开链接
	TransmittersDisconnect(drData.unicastLocatorList);

	std::list<EntityId_t> writersEntityid;
	{
		std::unique_lock<std::mutex> lock(mtxMatchEntitys_);
		writersEntityid = matchLocalEntitys_[guid];
		matchLocalEntitys_.erase(guid);
		for (auto& writerEntityid : writersEntityid) {
			auto itor = matchRemoteEntitys_.find(writerEntityid);
			if (itor != matchRemoteEntitys_.end()) {
				itor->second.erase(guid);
				if (itor->second.empty()) {
					matchRemoteEntitys_.erase(itor);
				}
			}
		}
	}
	
	for (auto& writerEntityid : writersEntityid) {

		std::unique_lock<std::mutex> lock(mtxWriters_);
		auto writerItor = writers_.find(writerEntityid);
		if (writerItor == writers_.end()) {
			continue;
		}
		auto writer = writerItor->second;
		if (!writer) {
			continue;
		}

		std::shared_ptr<StatefulWriter> statefulWriter = std::dynamic_pointer_cast<StatefulWriter>(writer);
		if (statefulWriter) {
			std::shared_ptr<ReaderProxy> readerProxy = statefulWriter->MatchedReaderLookup(guid);
			if(readerProxy) {
				statefulWriter->MatchedReaderRemove(readerProxy);
			}

			//TODO 重新检测确认数据
		}

		std::shared_ptr<StatelessWriter> statelessWriter = std::dynamic_pointer_cast<StatelessWriter>(writer);
		if (statelessWriter) {
			statelessWriter->ReaderLocatorRemove(drData.remoteReaderGuid);
		}

		InstanceHandle_t handle = Conversion::ToInstanceHandle(drData.remoteReaderGuid);
		writer->GetRelatedDataWriter()->OnPublicationMatched(handle,false);

		writer->GetRelatedDataWriter()->RemoveReaderFilter(guid);
	}


	return RETCODE_OK;
}


void Participant::GetParticipantAllWriter(const GUID_t& remoteParticipantGuid,std::vector<GUID_t>& guids)
{
	std::unique_lock<std::mutex> lock(mtxReaders_);
	for (auto& writer : remoteWriters_) {
		if (remoteParticipantGuid.prefix == writer.first.prefix) {
			guids.push_back(writer.first);
		}
	}
}

ReturnCode_t Participant::RemoveRemoteParticipantAllWriter(GUID_t remoteParticipantGuid)
{
	std::vector<GUID_t> guids;
	GetParticipantAllWriter(remoteParticipantGuid, guids);

	for (auto& guid : guids) {
		RemoveRemoteWriter(guid);
	}

	return RETCODE_OK;
}

void Participant::GetParticipantAllReader(const GUID_t & remoteParticipantGuid, std::vector<GUID_t>& guids)
{
	std::unique_lock<std::mutex> lock(mtxWriters_);
	for (auto& reader : remoteReaders_) {
		if (remoteParticipantGuid.prefix == reader.first.prefix) {
			guids.push_back(reader.first);
		}
	}
}

ReturnCode_t Participant::RemoveRemoteParticipantAllReader(GUID_t remoteParticipantGuid)
{
	std::vector<GUID_t> guids;
	GetParticipantAllReader(remoteParticipantGuid, guids);

	for (auto& guid : guids) {
		RemoveRemoteReader(guid);
	}

	return RETCODE_OK;
}

ReturnCode_t Participant::AddLocalWriter(std::shared_ptr<Writer> writer)
{
	for (auto& remoteReaderData : remoteReaders_) {
#ifdef DDSSECURITY
		//remoteReaderData是否来自安全相关实体
		bool isSecure = remoteReaderData.second.ddsSubscriptionData.security_info.endpoint_security_attributes &
			ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED;
		addWriterMatch(writer, remoteReaderData.second, isSecure);
#else
		addWriterMatch(writer, remoteReaderData.second);
#endif
	}
	return RETCODE_OK;
}

ReturnCode_t Participant::AddLocalReader(std::shared_ptr<Reader> reader)
{
	// SEDP may remove a remote Writer while this application thread enables
	// a Reader. Use the same lock as Add/RemoveRemoteWriter for iteration.
	std::unique_lock<std::mutex> lock(mtxReaders_);
	for (auto& remoteWriterData : remoteWriters_) {
#ifdef DDSSECURITY
		bool isSecure = remoteWriterData.second.ddsPublicationData.security_info.endpoint_security_attributes & 
			ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED;
		addReaderMatch(reader, remoteWriterData.second, isSecure);
#else
		addReaderMatch(reader, remoteWriterData.second);
#endif
	}
	return RETCODE_OK;
}

GUID_t Participant::GenerateGuid(const EntityId_t& EntityID, bool key, EntityType type)
{
	GUID_t guid;
	guid.prefix = guid_.prefix;

	if (ENTITYID_UNKNOWN != EntityID) {
		guid.entityId = EntityID;
		return guid;
	}

	switch (type) {
		case ENTITY_WRITER:
			guid.entityId.entityKind = key ? 0x02 : 0x03;
			writerId_++;
			guid.entityId.entityKey[0] = writerId_ % 255;
			guid.entityId.entityKey[1] = writerId_ / 255 % 255;
			guid.entityId.entityKey[2] = writerId_ / 255 / 255 % 255;
			break;
		case ENTITY_READER:
			guid.entityId.entityKind = key ? 0x07 : 0x04;
			readerId_++;
			guid.entityId.entityKey[0] = readerId_ % 255;
			guid.entityId.entityKey[1] = readerId_ / 255 % 255;
			guid.entityId.entityKey[2] = readerId_ / 255 / 255 % 255;
			break;
		case ENTITY_DOMAIN:
			guid.entityId = ENTITYID_PARTICIPANT;
			break;
		default:
			guid.entityId.entityKind = type;
			nextId_++;
			guid.entityId.entityKey[0] = nextId_ % 255;
			guid.entityId.entityKey[1] = nextId_ / 255 % 255;
			guid.entityId.entityKey[2] = nextId_ / 255 / 255 % 255;
			break;

	}
	return guid;
}

GUID_t Participant::GenerateGuid(const InstanceHandle_t& handle)
{
	return Conversion::ToGuid(handle);
}

GuidPrefix_t Participant::GetGuidPrefix() const
{
	return guid_.prefix;
}

#ifdef DDSSECURITY
void Participant::addWriterMatch(std::shared_ptr<Writer> writer, const DiscoveredReaderData& subData, bool isSecure)
#else
void Participant::addWriterMatch(std::shared_ptr<Writer> writer, const DiscoveredReaderData& subData)
#endif // DDSSECURITY
{
	DataWriterImpl* dw = writer->GetRelatedDataWriter();
	if (nullptr == dw) {
		TRAVODDS_LOG(LOG_ERROR, "datawriter is nullptr");
		return;
	}
	{
		std::unique_lock<std::mutex>  lock(mtxMatchEntitys_);
		auto readers = matchRemoteEntitys_.find(writer->GetGuid().entityId);
		if (readers != matchRemoteEntitys_.end() && readers->second.find(subData.remoteReaderGuid) != readers->second.end()) {
		    // 已经匹配过了
			dw->UpdateReaderFilter(subData.remoteReaderGuid, subData.contentFilter);
			return;
		}
	}

    const PublicationBuiltinTopicData& pubData = dw->GetBuiltinTopicData();

    if (pubData.topic_name != subData.ddsSubscriptionData.topic_name) {
		return;
	}

	const TopicImpl* topicImpl = dynamic_cast<const TopicImpl*>(writer->GetRelatedDataWriter()->get_topic());
	if (nullptr == topicImpl)
		return;

	if (pubData.type_name != subData.ddsSubscriptionData.type_name) {

		const_cast<TopicImpl*>(topicImpl)->update_inconsistent_topic_status();
		return;
	}

	if ((0x02 == writer->GetGuid().entityId.entityKind  &&  subData.remoteReaderGuid.entityId.entityKind  != 0x07)
		|| (0x03 == writer->GetGuid().entityId.entityKind  && subData.remoteReaderGuid.entityId.entityKind != 0x04)) {
		const_cast<TopicImpl*>(topicImpl)->update_inconsistent_topic_status();
		TRAVODDS_LOG(LOG_ERROR, "Loacl dw key = %x ,but remote dr key = %x", writer->GetGuid().entityId.entityKind, subData.remoteReaderGuid.entityId.entityKind);

		return;
	}

	std::set<QosPolicyId_t> qosSeq;
	bool ret = Match(pubData, subData.ddsSubscriptionData, qosSeq);
	if (!ret) {
		writer->GetRelatedDataWriter()->OnOfferedIncompatibleQos(qosSeq);
		return;
	}

	// yuguxu 传输链接
	TransmittersConnect(subData.unicastLocatorList);

	// yuguxu 主题数据过滤器
	if (subData.contentFilter.contentFilteredTopicName.size() > 0 &&
		subData.contentFilter.relatedTopicName == pubData.topic_name &&
		subData.contentFilter.filterClassName == SQL_FILTER_NAME &&
		subData.contentFilter.filterExpression.size() > 0) {
		dw->AddReaderFilter(subData.remoteReaderGuid, subData.contentFilter);
	}
#ifdef DDSSECURITY
	writer->MatchedReaderAddWithSecurity(subData, isSecure);
#else
	writer->MatchedReaderAdd(subData);
#endif

    writer->SetDataSharingReader(subData.remoteReaderGuid,
        pubData.data_sharing.compatible_with(subData.ddsSubscriptionData.data_sharing) &&
        subData.contentFilter.contentFilteredTopicName.empty());
	//printf("SEPD: topic %s addd remote reader \n", pubData.topic_name.c_str());

	{
		std::unique_lock<std::mutex>  lock(mtxMatchEntitys_);
		auto writers = matchLocalEntitys_.find(subData.remoteReaderGuid);
		if (matchLocalEntitys_.end() == writers) {
			std::list<EntityId_t> writerList;
			writerList.push_back(writer->GetGuid().entityId);
			matchLocalEntitys_[subData.remoteReaderGuid] = writerList;
		} else {
			writers->second.push_back(writer->GetGuid().entityId);
		}
		auto readers = matchRemoteEntitys_.find(writer->GetGuid().entityId);
		if (readers == matchRemoteEntitys_.end()) {
			std::set<GUID_t> remoteWriters;
			remoteWriters.insert(subData.remoteReaderGuid);
			matchRemoteEntitys_[writer->GetGuid().entityId] = remoteWriters;
		} else {
			readers->second.insert(subData.remoteReaderGuid);
		}
	}
	
	InstanceHandle_t handle = Conversion::ToInstanceHandle(subData.remoteReaderGuid);
	writer->GetRelatedDataWriter()->OnPublicationMatched(handle);
}



#ifdef DDSSECURITY
void Participant::addReaderMatch(std::shared_ptr<Reader> reader, const DiscoveredWriterData& pubData, bool isSecure)
#else
void Participant::addReaderMatch(std::shared_ptr<Reader> reader, const DiscoveredWriterData& pubData)
#endif // DDSSECURITY
{
    DataReaderImpl* dr = reader->GetRelatedDataReader();
    if(nullptr == dr) {
        return;
    }
    const SubscriptionBuiltinTopicData& subData = dr->GetBuiltinTopicData();
	
	if (pubData.ddsPublicationData.topic_name != subData.topic_name) {
		return;
	}
	const ContentFilteredTopicImpl* cfTopic = dynamic_cast<const ContentFilteredTopicImpl*>(reader->GetRelatedDataReader()->get_topicdescription());
	const TopicImpl* topicImpl = dynamic_cast<const TopicImpl*>(reader->GetRelatedDataReader()->get_topicdescription());
	if(nullptr != cfTopic) {
		topicImpl = dynamic_cast<const TopicImpl*>(cfTopic->get_related_topic());
	}

	if (nullptr == topicImpl) {
		return;
	}


	if(pubData.ddsPublicationData.type_name != subData.type_name)
	{
		const_cast<TopicImpl*>(topicImpl)->update_inconsistent_topic_status();
		return;
	}

	if ((0x02 == pubData.remoteWriterGuid.entityId.entityKind && reader->GetGuid().entityId.entityKind != 0x07) 
		|| (0x03 == pubData.remoteWriterGuid.entityId.entityKind && reader->GetGuid().entityId.entityKind != 0x04)) {
		const_cast<TopicImpl*>(topicImpl)->update_inconsistent_topic_status();
		TRAVODDS_LOG(LOG_ERROR, "remote dw key = %x ,but local dr key = %x", pubData.remoteWriterGuid.entityId.entityKind, reader->GetGuid().entityId.entityKind);
		return;
	}

	std::set<QosPolicyId_t> qosSeq;
	bool ret = Match(pubData.ddsPublicationData, subData, qosSeq);
	if (!ret) {
		reader->GetRelatedDataReader()->OnRequestedIncompatibleQos(qosSeq);
		return ;
	}

    if (reader->MatchedWriterIsMatched(pubData.remoteWriterGuid) &&
        !reader->DataSharingSessionChanged(pubData)) {
        reader->StartDataSharing(reader, pubData);
        return;
    }
	// yuguxu 传输链接
	TransmittersConnect(pubData.unicastLocatorList);

#ifdef DDSSECURITY
	WriterSampleList* newWriter = nullptr;
	
	//if ((isSecure && dr->secAttributes_.is_discovery_protected) || 
	//	!dr->secAttributes_.is_discovery_protected) {
	//	newWriter = addReaderMatchWithSecurity(reader, pubData);
	//}
	// 用户自定义实体涉及的安全相关的发现匹配流程
	GUID_t drGuid = Conversion::ToGuid(dr->get_instance_handle());
	if (dr->enableSec_ && !drGuid.IsSecureBuiltinEntity() && drGuid.IsUserDefinedEntity()) {
		newWriter = addReaderMatchWithSecurity(reader, pubData, isSecure);
		if (!newWriter) {
			return;
		}
	}
#endif // DDSSECURITY

	if (reader->IsReliable()) {
		std::shared_ptr<WriterProxy> proxy = std::make_shared<WriterProxy>(pubData.remoteWriterGuid, pubData.remoteGroupGuid, pubData.unicastLocatorList, pubData.multicastLocatorList);
		StatefulReader* fulReader = dynamic_cast<StatefulReader*> (reader.get());

#ifdef DDSSECURITY
		if (newWriter) {
			proxy->cryptoHdl_ = newWriter->cryptoHdl;
			proxy->partCryptoHdl_ = newWriter->partCryptoHdl;
			proxy->readyForSecComm_ = newWriter->readyForSecComm;
		}
#endif // DDSSECURITY

		if (fulReader) {
			fulReader->MatchedWriterAdd(proxy);
		} else {
			TRAVODDS_LOG(LOG_ERROR, "MatchedWriterAdd Err")
		}
	}
	else {
		StatelessReader* lessReader = dynamic_cast<StatelessReader*> (reader.get());
		////WriterData;
		//std::shared_ptr<RemoteWriterData> proxy = nullptr;
		/* wangyi  */
		std::shared_ptr<WriterProxy> proxy = std::make_shared<WriterProxy>(pubData.remoteWriterGuid, pubData.remoteGroupGuid, pubData.unicastLocatorList, pubData.multicastLocatorList);
		if (lessReader) {
			lessReader->MatchedWriterAdd(proxy);
		} else {
			TRAVODDS_LOG(LOG_ERROR, "MatchedWriterAdd Err")
		}
	}

	//printf("SEPD: topic %s addd remote writer \n", pubData.ddsPublicationData.topic_name.c_str());

	std::unique_lock<std::mutex>  lock(mtxMatchEntitys_);
	auto readers = matchLocalEntitys_.find(pubData.remoteWriterGuid);
	if (matchLocalEntitys_.end() == readers) {
		std::list<EntityId_t> readerList;
		readerList.push_back(reader->GetGuid().entityId);
		matchLocalEntitys_[pubData.remoteWriterGuid] = readerList;
	} else {
		readers->second.push_back(reader->GetGuid().entityId);
	}
	auto writers = matchRemoteEntitys_.find(reader->GetGuid().entityId);
	if (writers == matchRemoteEntitys_.end()) {
		std::set<GUID_t> remoteReaders;
		remoteReaders.insert(pubData.remoteWriterGuid);
		matchRemoteEntitys_[reader->GetGuid().entityId] = remoteReaders;
	} else {
		writers->second.insert(pubData.remoteWriterGuid);
	}

	lock.unlock();
    reader->StartDataSharing(reader, pubData);

	InstanceHandle_t handle = Conversion::ToInstanceHandle(pubData.remoteWriterGuid);
	reader->NotifySubscriptionMatched(handle);
}



std::vector<std::shared_ptr<MessageTransmitter>> Participant::GetMessageTransmitters()
{
	return messageTransmitters_;
}
void Participant::TransmittersSendMessage(LocatorVec destinationList, char* message, uint32_t length)
{
	ReturnCode_t ret = RETCODE_OK;
	Locator_t srcLocator;
	for (auto& dest : destinationList) {
		for (auto& transmitter : messageTransmitters_) {
			srcLocator.kind = transmitter->GetTransmitterKind();
			ret = transmitter->MessageSend(srcLocator, dest, message, length);
			if (RETCODE_OK == ret) {
				return;
			}
		}
	}
}
void Participant::TransmittersSendMessage(std::vector<LocatorVec>& destinationList, char* message, uint32_t length)
{
	for (auto& destList : destinationList) {
		TransmittersSendMessage(destList, message, length);
	}	
}
ReturnCode_t Participant::FlowRateLimitedMessageSend(std::vector<LocatorVec>& destinationList, char* message, uint32_t length, const Duration_t& timeOut)
{
	if (flowRateLimitedManager_) {
		return flowRateLimitedManager_->MessageSend(destinationList, message, length, timeOut);
	}
	TransmittersSendMessage(destinationList, message, length);
	return RETCODE_OK;
}
void Participant::TransmittersConnect(const LocatorVec& remoteLocator)
{
	std::shared_ptr<MessageTransmitter> tcpv4Transmitter = nullptr;
	std::shared_ptr<MessageTransmitter> tcpv6Transmitter = nullptr;
	std::shared_ptr<MessageTransmitter> shmTransmitter = nullptr;
	for (auto& transmitter : messageTransmitters_) {

		if(transmitter->GetTransmitterKind() == LOCATOR_KIND_TCPv4 ) {
			tcpv4Transmitter = transmitter;
			continue;
		} else if (transmitter->GetTransmitterKind() == LOCATOR_KIND_TCPv6) {
			tcpv6Transmitter = transmitter;
			continue;
		} else if (transmitter->GetTransmitterKind() == LOCATOR_KIND_SHM) {
			shmTransmitter = transmitter;
			continue;
		} else {
			continue;
		}
	}
	for (const auto& dest : remoteLocator) {
		Locator_t src;
		if(dest.kind == LOCATOR_KIND_TCPv4 && tcpv4Transmitter) {
			src.kind = LOCATOR_KIND_TCPv4;
			tcpv4Transmitter->SendAddrAdd(src, dest);
		} else if (dest.kind == LOCATOR_KIND_TCPv6 && tcpv6Transmitter) {
			src.kind = LOCATOR_KIND_TCPv6;
			tcpv6Transmitter->SendAddrAdd(src, dest);
		} else if (dest.kind == LOCATOR_KIND_SHM && shmTransmitter) {
			src.kind = LOCATOR_KIND_SHM;
			shmTransmitter->SendAddrAdd(src, dest);
		} else {
			continue;
		}
	}
}
void Participant::TransmittersDisconnect(const LocatorVec& remoteLocator)
{
	std::shared_ptr<MessageTransmitter> tcpv4Transmitter = nullptr;
	std::shared_ptr<MessageTransmitter> tcpv6Transmitter = nullptr;
	std::shared_ptr<MessageTransmitter> shmTransmitter = nullptr;

	for (auto& transmitter : messageTransmitters_) {

		if (transmitter->GetTransmitterKind() == LOCATOR_KIND_TCPv4) {
			tcpv4Transmitter = transmitter;
			continue;
		} else if (transmitter->GetTransmitterKind() == LOCATOR_KIND_TCPv6) {
			tcpv6Transmitter = transmitter;
			continue;
		} else if (transmitter->GetTransmitterKind() == LOCATOR_KIND_SHM) {
			shmTransmitter = transmitter;
			continue;
		} else {
			continue;
		}
	}
	for (const auto& dest : remoteLocator) {
		if (dest.kind == LOCATOR_KIND_TCPv4 && tcpv4Transmitter) {
			tcpv4Transmitter->SendAddrRemove(dest);
		} else if (dest.kind == LOCATOR_KIND_TCPv6 && tcpv6Transmitter) {
			tcpv6Transmitter->SendAddrRemove(dest);
		} else if (dest.kind == LOCATOR_KIND_SHM && shmTransmitter) {
			shmTransmitter->SendAddrRemove(dest);
		} else {
			continue;
		}
	}
}
LocatorVec Participant::GetDefaultUnicastLocators()
{
	return defaultUnicastLocatorList_;
}
LocatorVec Participant::GetDefaultMulticastLocators()
{
	return defaultMulticastLocatorList_;
}
LocatorVec Participant::GetMetatrafficUnicastLocators()
{
	return metatrafficUnicastLocatorList_;
}
LocatorVec Participant::GetMetatrafficMulticastLocators()
{
	return metatrafficMulticastLocatorList_;
}

ReturnCode_t Participant::GetPublications(InstanceHandleSeq& handles, bool includeContainedEntities)
{
	std::lock_guard<std::mutex> metadata(mtxMatchEntitys_);
	for (auto& writer : remoteWriters_) {

		if (false == includeContainedEntities && guid_.prefix == writer.first.prefix) {
			continue;
		}

		if (ENTITYKIND_BUILTIN_WRITER_NO_KEY == writer.first.entityId.entityKind || ENTITYKIND_BUILTIN_WRITER_WITH_KEY == writer.first.entityId.entityKind) {
			continue;
		}

		InstanceHandle_t handle = Conversion::ToInstanceHandle(writer.first);
		handles.push_back(handle);
	}
	return RETCODE_OK;
}

int32_t Participant::CreateReceiverResources(std::shared_ptr<MessageTransmitter> messageTransmitter, LocatorVec& locatorList, bool applyMutation)
{
	LocatorVec inList = std::move(locatorList);
	locatorList.clear();
	ReturnCode_t ret;
	for (auto& itor : inList) {
		if (itor.kind != messageTransmitter->GetTransmitterKind()) {
			locatorList.push_back(itor);
			continue;
		}
		ret = messageTransmitter->ReceiveAddrAdd(itor);
		if (RETCODE_PORT_IN_USE == ret && applyMutation) {
			/* 限制重新尝试次数 */
			uint32_t tries = 100u;
			for (uint32_t i = 0; RETCODE_PORT_IN_USE == ret && i < tries; i++) {
				itor.port += PARTICIPANTID_GAIN;
				ret = messageTransmitter->ReceiveAddrAdd(itor);
			}
		}
		if (RETCODE_OK == ret) {
			locatorList.push_back(itor);
		} else {
			/* 异常处理 */
			TRAVODDS_LOG(LOG_ERROR, "CreateReceiverResources Err")
		}
	}
	return static_cast<int32_t>(locatorList.size());
}

void Participant::StopReceiveThread()
{
	for (auto& messageTransmitter : messageTransmitters_) {
		messageTransmitter->SetStop();
	}
}

std::shared_ptr<ThreadPool> Participant::GetThreadPool() const
{
	return threadPool_;
}

void Participant::GetMatchedEntitys(const EntityId_t& entity, InstanceHandleSeq& handles) const
{
	handles.clear();
	std::unique_lock<std::mutex> lock(mtxMatchEntitys_);

	auto itor = matchRemoteEntitys_.find(entity);
	if (itor == matchRemoteEntitys_.end()) {
		return;
	}
	handles.reserve(itor->second.size());
	for (auto& guid : itor->second) {
		handles.push_back(Conversion::ToInstanceHandle(guid));
	}
}

bool Participant::RemoteEntityIsMatched(const EntityId_t& entity, const InstanceHandle_t& remotehandle) const
{
	GUID_t remoteGuid = Conversion::ToGuid(remotehandle);
	std::unique_lock<std::mutex> lock(mtxMatchEntitys_);
	auto itor = matchRemoteEntitys_.find(entity);
	if (itor == matchRemoteEntitys_.end()) {
		return false;
	}
	auto guidItor = itor->second.find(remoteGuid);
	if (guidItor == itor->second.end()) {
		return false;
	}
	return true;
}

bool Participant::Match(const PublicationBuiltinTopicData& pubData, const SubscriptionBuiltinTopicData& subData, std::set<QosPolicyId_t>& qosSeq)
{
	if (subData.reliability.kind > pubData.reliability.kind) {
		qosSeq.insert(RELIABILITY_QOS_POLICY_ID);
	}

	if (subData.deadline.period < pubData.deadline.period) {
		qosSeq.insert(DEADLINE_QOS_POLICY_ID);
	}

	if (subData.latency_budget.duration < pubData.latency_budget.duration) {
		qosSeq.insert(LATENCYBUDGET_QOS_POLICY_ID);
	}

	if ((subData.liveliness.kind > pubData.liveliness.kind) ||
		((subData.liveliness.lease_duration < pubData.liveliness.lease_duration))) {
		qosSeq.insert(LIVELINESS_QOS_POLICY_ID);
	}

	if (subData.ownership.kind != pubData.ownership.kind) {
		qosSeq.insert(OWNERSHIPSTRENGTH_QOS_POLICY_ID);
	}

	if (subData.destination_order.kind > pubData.destination_order.kind) {
		qosSeq.insert(DESTINATIONORDER_QOS_POLICY_ID);
	}

	if (subData.durability.kind > pubData.durability.kind) {
		qosSeq.insert(DURABILITY_QOS_POLICY_ID);
	}

	if (!((subData.presentation.access_scope <= pubData.presentation.access_scope) &&
		(!pubData.presentation.coherent_access || (subData.presentation.coherent_access == pubData.presentation.coherent_access)) &&
		(!pubData.presentation.ordered_access || (subData.presentation.ordered_access == pubData.presentation.ordered_access)))) {
		qosSeq.insert(PRESENTATION_QOS_POLICY_ID);
	}

	//TODO 分区匹配
	bool ret = true;
	if (0 != pubData.partition.name.size()  || 0 != subData.partition.name.size()) {
		ret = RegexPartitionsMatch(pubData.partition.name, subData.partition.name);
	}

	//for (auto pubName : pubData.partition.name) {
	//	if (ret) {
	//		/* 已经匹配成功 */
	//		break;
	//	}
	//	for (auto subName : subData.partition.name) {
	//		std::regex pubRegex(pubName);
	//		std::regex subRegex(subName);
	//		if (std::regex_match(subName, pubRegex)) {
	//			ret = true;
	//			break;
	//		}
	//		if (std::regex_match(pubName, subRegex)) {
	//			ret = true;
	//			break;
	//		}
	//	}
	//}
	if (!ret) {
		qosSeq.insert(PARTITION_QOS_POLICY_ID);
	}

	return qosSeq.empty();
}

ReturnCode_t Participant::GetPublicationData(PublicationBuiltinTopicData &pubData, const InstanceHandle_t& handle)
{
	GUID_t guid = Conversion::ToGuid(handle);
	std::lock_guard<std::mutex> metadata(mtxMatchEntitys_);

	auto itor = remoteWriters_.find(guid);
	if (itor == remoteWriters_.end()) {
		TRAVODDS_LOG(LOG_DEBUG, "handle not exist")
		return RETCODE_BAD_PARAMETER;
	}
	pubData = itor->second.ddsPublicationData;
	return RETCODE_OK;

}

ReturnCode_t Participant::GetSublications(InstanceHandleSeq & handles, bool includeContainedEntities)
{
	std::lock_guard<std::mutex> metadata(mtxMatchEntitys_);
	for (auto& reader : remoteReaders_) {

		if (false == includeContainedEntities && guid_.prefix == reader.first.prefix) {
			continue;
		}

		if (ENTITYKIND_BUILTIN_READER_NO_KEY == reader.first.entityId.entityKind || ENTITYKIND_BUILTIN_READER_WITH_KEY == reader.first.entityId.entityKind) {
			continue;
		}

		InstanceHandle_t handle = Conversion::ToInstanceHandle(reader.first);
		handles.push_back(handle);
	}
	return RETCODE_OK;
}

ReturnCode_t Participant::GetSublicationData(SubscriptionBuiltinTopicData& subData, const InstanceHandle_t& handle)
{
	GUID_t guid = Conversion::ToGuid(handle);
	std::lock_guard<std::mutex> metadata(mtxMatchEntitys_);

	auto itor = remoteReaders_.find(guid);
	if (itor == remoteReaders_.end()) {
		TRAVODDS_LOG(LOG_DEBUG, "handle not exist")
		return RETCODE_BAD_PARAMETER;
	}
	subData = itor->second.ddsSubscriptionData;
	return RETCODE_OK;
}

void Participant::GetMacthList(const GUID_t& writerGuid, std::list<EntityId_t>& endpointList)
{
	std::unique_lock<std::mutex>  lock(mtxMatchEntitys_);
	auto listItor = matchLocalEntitys_.find(writerGuid);
	if (matchLocalEntitys_.end() != listItor) {
		endpointList = listItor->second;
	}
	else {
		if (writerGuid.entityId == ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER) {
			endpointList.push_back(ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER);
		}
#ifdef DDSSECURITY
		if (writerGuid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER) {
            endpointList.push_back(ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER);
        }
        if (writerGuid.entityId == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER) {
            endpointList.push_back(ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER);
        }
#endif // DDSSECURITY
	}
	return;
}

/* wangyi 获取LivelinessManager */
std::shared_ptr<LivelinessManager> Participant::GetLivelinessManager()
{
	return livelinessManager_;
}

/* 检查字符串是否包含正则元字符 */ 
bool Participant::IsRegexPattern(const std::string& str)
{
	/* 检测正则元字符 */
	static const std::vector<std::string> regexIndicators = {
		".*", ".+", ".?", "\\d", "\\w", "\\s",
		"^", "$", "|", "(", ")", "[", "]", "{", "}"
	};

	for (const auto& indicator : regexIndicators) {
		if (str.find(indicator) != std::string::npos) {
			return true;
		}
	}
	return false;
}

/* 检查字符串是否包含通配符 */
bool Participant::IsWildcardPattern(const std::string& str)
{
	return str.find('*') != std::string::npos ||
		str.find('?') != std::string::npos ||
		str.find('[') != std::string::npos ||
		str.find(']') != std::string::npos;
}

/* 将通配符转换为等价的正则表达式 */
std::string Participant::ConvertWildcardToRegex(const std::string& wildcard)
{
	std::string regexStr;
	for (char c : wildcard) {
		switch (c) {
		case '*': regexStr += ".*"; break;
		case '?': regexStr += "."; break;
			// 转义正则表达式的特殊字符，防止语义冲突
		case '.': regexStr += "\\."; break;
		case '+': regexStr += "\\+"; break;
		case '^': regexStr += "\\^"; break;
		case '$': regexStr += "\\$"; break;
		case '|': regexStr += "\\|"; break;
		case '(': regexStr += "\\("; break;
		case ')': regexStr += "\\)"; break;
		case '{': regexStr += "\\{"; break;
		case '}': regexStr += "\\}"; break;
		default: regexStr += c; break; // [ 和 ] 保留，正则和 fnmatch 都支持字符集
		}
	}
	return regexStr;
}

/* 分区匹配检测函数 */
bool Participant::RegexPartitionMatch(const std::string& pubName, const std::string& subName)
{
	bool pubIsRegex = IsRegexPattern(pubName);
	bool subIsRegex = IsRegexPattern(subName);
	bool pubIsWildcard = IsWildcardPattern(pubName);
	bool subIsWildcard = IsWildcardPattern(subName);

	// 规则1: 双方都是正则模式，或双方都是通配符模式 -> 不匹配（符合 DDS 规范）
	if ((pubIsRegex && subIsRegex) || (pubIsWildcard && subIsWildcard)) {
		return false;
	}

	try {
		std::regex pubPattern;
		std::regex subPattern;
		bool pubHasPattern = false;
		bool subHasPattern = false;

		// 处理发布者模式（正则或通配符）
		if (pubIsRegex) {
			pubPattern = std::regex(pubName);
			pubHasPattern = true;
		}
		else if (pubIsWildcard) {
			pubPattern = std::regex(ConvertWildcardToRegex(pubName));
			pubHasPattern = true;
		}

		// 处理订阅者模式（正则或通配符）
		if (subIsRegex) {
			subPattern = std::regex(subName);
			subHasPattern = true;
		}
		else if (subIsWildcard) {
			subPattern = std::regex(ConvertWildcardToRegex(subName));
			subHasPattern = true;
		}

		// 规则2: 发布者是模式，订阅者是具体名称
		if (pubHasPattern && !subHasPattern) {
			return std::regex_match(subName, pubPattern);
		}

		// 规则3: 订阅者是模式，发布者是具体名称
		if (!pubHasPattern && subHasPattern) {
			return std::regex_match(pubName, subPattern);
		}

		// 规则4: 双方都是具体名称
		return pubName == subName;
	}
	catch (const std::regex_error& e) {
		// 【修改】使用标准日志宏，避免污染标准错误输出
		TRAVODDS_LOG(LOG_ERROR, "Regex error: %s, Pattern: %s", e.what(),
			(pubIsRegex || pubIsWildcard) ? pubName.c_str() : subName.c_str());
		return false;
	}
}

/* 分区列表匹配 */
bool Participant::RegexPartitionsMatch(const StringSeq& pubPartitions, const StringSeq& subPatterns)
{
	// 【修改】处理默认分区（空字符串 ""），依据规范：空字符串被视为有效分区
	StringSeq effectivePub = pubPartitions;
	StringSeq effectiveSub = subPatterns;

	if (effectivePub.empty()) {
		effectivePub.push_back("");
	}
	if (effectiveSub.empty()) {
		effectiveSub.push_back("");
	}

	for (const auto& subPattern : effectiveSub) {
		for (const auto& pubPartition : effectivePub) {
			if (RegexPartitionMatch(pubPartition, subPattern)) {
				return true;
			}
		}
	}
	return false;
}

#ifdef DDSSECURITY
WriterSampleList* Participant::addReaderMatchWithSecurity(std::shared_ptr<Reader> reader, const DiscoveredWriterData& pubData, bool isSecure)
{
	std::cout << "addReaderMatchWithSecurity" << std::endl;
	DataReaderImpl* dr = reader->GetRelatedDataReader();
	if (!isSecure && dr->secAttributes_.is_discovery_protected) {
		return nullptr; //开启端点保护情况会过滤掉来自未开启安全的数据写者的发现报文
	}

	// 访问控制check_remote_datawriter
	GUID_t drGuid = Conversion::ToGuid(dr->get_instance_handle());
	DomainParticipantImpl* dp = dynamic_cast<DomainParticipantImpl*>(dr->getParent()->get_participant());
	if (dr->secAttributes_.is_write_protected)
	{
		SecurityException secExce;
		BuiltinTopicKey_t participant_key = pubData.ddsPublicationData.participant_key;
		GuidPrefix_t remote_prefix = Conversion::ToGuid(participant_key).prefix;
		PermissionsHandle remotePermissionsHandle = SEC_NIL_HANDLE;
		std::map<GuidPrefix_t, DiscoveryDPInfo*>& discParts = dp->GetDiscovery()->GetSpdp()->discParts_;
		auto dpInfoIter = discParts.find(remote_prefix);
		if (dpInfoIter == discParts.end())
		{
			remotePermissionsHandle = SEC_NIL_HANDLE;
		}
		else if (dpInfoIter->second->enableDDSSec_)
		{
			remotePermissionsHandle = dpInfoIter->second->permissionsHdl_;
		}
		if (!dp->secPlugin_.access_control->check_remote_datawriter(
			remotePermissionsHandle,
			dp->GetDomainId(),
			&pubData.ddsPublicationData,
			&secExce))
		{
			TRAVODDS_LOG(LOG_ERROR, "Match Writer failed: rejected by access control plugin."
				"\n  Code: %d"
				"\n  Minor code: %d"
				"\n  Message: %s",
				secExce.code,
				secExce.minor_code,
				secExce.message);
			return nullptr;
		}
	}

	// 端点发现保护下校验掩码
	if(isSecure && dr->secAttributes_.is_discovery_protected)
	{
		EndpointSecurityAttributesMask drSecAttrMask =
			EndpointSecurityAttributesToMask(&dr->secAttributes_);
		EndpointSecurityAttributesMask dwSecAttrMask =
			pubData.ddsPublicationData.security_info.endpoint_security_attributes;
		if ((dwSecAttrMask & ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID) == 0)
		{
			dwSecAttrMask = 0;
		}
		if (drSecAttrMask != dwSecAttrMask
			|| dr->secAttributes_.plugin_endpoint_attributes
			!= pubData.ddsPublicationData.security_info.plugin_endpoint_security_attributes)
		{
			TRAVODDS_LOG(LOG_ERROR, "Match Writer failed: participant security_info different from local reader.");
			return nullptr;
		}
	}

	//端点密钥交换流程
	WriterSampleList* newWriter = new WriterSampleList;
	GUID_t writerId = Conversion::ToGuid(pubData.ddsPublicationData.key);
	newWriter->writerId = writerId;
	newWriter->readyForSecComm = false;
	if (dr->enableSec_ && dr->secPlugin_)
	{
		if (dr->secAttributes_.is_payload_protected
			|| dr->secAttributes_.is_submessage_protected)
		{
			SecurityException secExce;
			newWriter->cryptoHdl = dr->secPlugin_->crypto_key_factory->register_matched_remote_datawriter(
				dr->cryptoHdl_,
				dp->GetDiscovery()->GetSpdp()->GetRemoteParticipantCryptoHandle(&writerId.prefix),
				dp->GetDiscovery()->GetSpdp()->GetRemoteParticipantSharedSecret(&writerId.prefix),
				&secExce);

			if (newWriter->cryptoHdl == SEC_NIL_HANDLE)
			{
				TRAVODDS_LOG(LOG_ERROR, "Failed to register remote writer to key factory."
					"\n  Code: %d"
					"\n  Minor code: %d"
					"\n  Message: %s",
					secExce.code,
					secExce.minor_code,
					secExce.message);
				delete newWriter;
				return nullptr;
			}

			if (drGuid.entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER)
			{
				ParticipantGenericMessage cryptoTokensSample;
				std::cout << "create_local_datareader_crypto_tokens" << std::endl;
				if (!dr->secPlugin_->crypto_key_exchange->create_local_datareader_crypto_tokens(
					&cryptoTokensSample.messageData,
					dr->cryptoHdl_,
					newWriter->cryptoHdl,
					&secExce))
				{
					TRAVODDS_LOG(LOG_ERROR, "Match Writer failed:  failed to create crypto tokens for remote writer."
						"\n  Code: %d"
						"\n  Minor code: %d"
						"\n  Message: %s",
						secExce.code,
						secExce.minor_code,
						secExce.message);
					delete newWriter;
					return nullptr;
				}
				cryptoTokensSample.messageClassId = GMCLASSID_SECURITY_DATAREADER_CRYPTO_TOKENS;
				cryptoTokensSample.destinationEndpointKey = Conversion::ToBuiltinTopicKey(writerId);
				cryptoTokensSample.sourceEndpointKey = Conversion::ToBuiltinTopicKey(drGuid);
				GUID_t msgReceiver = writerId;
				msgReceiver.entityId = ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER;
				dp->GetDiscovery()->GetSedp()->WriteParticipantVolatileSecureMessage(&msgReceiver, cryptoTokensSample);
				dr->secPlugin_->crypto_key_exchange->return_crypto_tokens(
					&cryptoTokensSample.messageData, &secExce);
				cryptoTokensSample.messageClassId = "";
			}
			else
			{
				newWriter->partCryptoHdl = dp->GetDiscovery()->GetSpdp()->GetRemoteParticipantCryptoHandle(&writerId.prefix);
				dr->secWriters_.push_back(newWriter->cryptoHdl);
				newWriter->readyForSecComm = true;
			}
		}
		else if (dr->rtpsMsgProtected_)
		{
			newWriter->partCryptoHdl = dp->GetDiscovery()->GetSpdp()->GetRemoteParticipantCryptoHandle(&writerId.prefix);
			newWriter->readyForSecComm = true;
		}
		dr->writerSampleList_[writerId] = newWriter;
	}
	if (dr->enableSec_ && !newWriter->readyForSecComm) {
		if (newWriter->writerId.IsSecureBuiltinEntity()
			|| dr->secAttributes_.is_payload_protected
			|| dr->secAttributes_.is_submessage_protected) {
			// need crypto key exchange
			dr->preparingWriters_[writerId] = newWriter;
			auto wrTokenIter = dr->remoteWriterCryptoToken_.find(writerId);
			if (wrTokenIter != dr->remoteWriterCryptoToken_.end())
			{
				dr->SetRemoteWriterCryptoTokens(&writerId, &wrTokenIter->second);
				dr->remoteWriterCryptoToken_.erase(wrTokenIter);
			}
			return newWriter;
		}
	}
	return newWriter;
}
#endif
