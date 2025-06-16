#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/statefulwriter.h"
#include "rtps/endpoint/statelesswriter.h"
#include "rtps/endpoint/statefulreader.h"
#include "rtps/endpoint/statelessreader.h"
#include "rtps/transport/messagetransmitterudp.h"
#include "rtps/messages/messageprocessor.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "dcps/publish/datawriterimpl.h"
#include "rtps/transport/ipcontainer.h"
#include "rtps/builtin/liveliness/livelinessmanager.h"
#include "common/log/logger.h"
#include <random>
#include <regex>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif // if defined(_WIN32)

USING_AGIDDS_NAMESPACE;

// Participant::Participant(const DomainParticipantImpl* domainImpl)
// 	:domainImpl_(domainImpl)
// {
// 	readerId_ = 0;
// 	writerId_ = 0;
// 	nextId_ = 0;
//
// 	//protocolVersion_ = 0
// }

Participant::Participant(DomainId_t id, std::shared_ptr<ThreadPool> threadPool)
	: domainId_(id), readerId_(0), writerId_(0), nextId_(0),
	  protocolVersion_(PROTOCOLVERSION),
	  vendorId_(VENDORID_PROSIMA),
	  threadPool_(threadPool)
{

}

Participant::~Participant()
{
	StopReceiveThread();

	//if (userDataRecvThread_.joinable()) {
	//	userDataRecvThread_.join();
	//}

	//if (multiCastRecvThread_.joinable()) {
	//	multiCastRecvThread_.join();
	//}

}

ReturnCode_t Participant::Init()
{
	GenerateGuidPrefix(domainId_, domainId_);
	guid_.entityId = ENTITYID_PARTICIPANT;
	/* wangyi */
	livelinessManager_ = std::make_shared<LivelinessManager>(this);

	messageProcessor_ = std::make_shared<MessageProcessor>(this);
	messageTransmitter_ = std::make_shared<MessageTransmitterUDP>(messageProcessor_);

	//网络传输模块对象初始化发送地址、接收地址
	Locator_t unicastAddress;

	unicastAddress.kind = LOCATOR_KIND_UDPv4;
	messageTransmitter_->SendAddrAdd(unicastAddress, unicastAddress);

	std::vector<InfoIP_t> localIps;
	IpContainer::GetInfoIPs(localIps);

	for (auto ip : localIps) {
		if (ip.type == IPV4) {
			memcpy(unicastAddress.address, ip.address, 16);
			break;
		}
	}

	// 1. 初始化随机数引擎
	std::random_device rd;  // 可能生成真随机数（系统依赖）
	std::mt19937 gen(rd()); // 用random_device的输出来种子化mt19937

	// 2. 定义分布器（例如：生成0到99的均匀整数）
	std::uniform_int_distribution<> dis(0,80);

	// 3. 生成随机数
	int32_t participantid = 0;

	unicastAddress.port = USER_DEFAULT_UNICAST_PORT(domainId_, participantid);
	// messageTransmitter_->ReceiveAddrAdd(unicastAddress);
	defaultUnicastLocatorList_.push_back(unicastAddress);
	CreateReceiverResources(defaultUnicastLocatorList_, true);

	unicastAddress.port = SPDP_WELL_KNOWN_UNICAST_PORT(domainId_, participantid);
	// messageTransmitter_->ReceiveAddrAdd(unicastAddress);
	metatrafficUnicastLocatorList_.push_back(unicastAddress);
	CreateReceiverResources(metatrafficUnicastLocatorList_, true);

	messageTransmitter_->SetUnicastLocatorList(defaultUnicastLocatorList_);

	Locator_t multicastAaddresses;
	multicastAaddresses.kind = LOCATOR_KIND_UDPv4;

	multicastAaddresses.address[12] = 239;
	multicastAaddresses.address[13] = 255;
	multicastAaddresses.address[14] = 0;
	multicastAaddresses.address[15] = 1;

	multicastAaddresses.port = SPDP_WELL_KNOWN_MULTICAST_PORT(domainId_);
	messageTransmitter_->ReceiveAddrAdd(multicastAaddresses);
	metatrafficMulticastLocatorList_.push_back(multicastAaddresses);

	//Locator_t localAddress;
	//unicastAddress.kind = LOCATOR_KIND_UDPv4;
	return RETCODE_OK;
}
std::shared_ptr<Writer> Participant::CreateWriter(DataWriterImpl* dw, const DataWriterQos & dwQos, const EntityId_t & EntityID, bool key)
{
	std::shared_ptr<Writer> writerPtr;
	std::shared_ptr<Endpoint> endpointPtr;
	GUID_t guid;
	TopicKind_t kind;

	kind.value = key ? WITH_KEY : NO_KEY;
	guid = GenerateGuid(EntityID, key, ENTITY_WRITER);

	if (dwQos.reliability.kind == BEST_EFFORT_RELIABILITY_QOS) {
		writerPtr = std::make_shared<StatelessWriter>(dwQos, guid, kind);
	} else {
		writerPtr = std::make_shared<StatefulWriter>(dwQos, guid, kind);
	}
	endpointPtr = writerPtr;
	endpointPtr->SetParticipant(this);

    dw->SetRelatedWriter(writerPtr);

	{
		std::unique_lock<std::mutex> lock(mtxEndpoints_);
		endpoints_[guid.entityId] = endpointPtr;
	}
  
	{
		std::unique_lock<std::mutex> lock(mtxWriters_);
		writers_[guid.entityId] = writerPtr;
	}
	

	/* wangyi 添加writer */
	if (writerPtr->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER &&
		writerPtr->GetGuid().entityId != ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER &&
		writerPtr->GetGuid().entityId != ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER &&
		writerPtr->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER) {
		livelinessManager_->AddLivelinessWriter(writerPtr, dwQos.liveliness);
	}

	return writerPtr;
}

std::shared_ptr<Reader> Participant::CreateReader(DataReaderImpl* dr, const DataReaderQos & drQos, const EntityId_t & EntityID, bool key)
{
	std::shared_ptr<Reader> readerPtr;
	std::shared_ptr<Endpoint> endpointPtr;
	GUID_t guid;
	TopicKind_t kind;

	kind.value = key ? WITH_KEY : NO_KEY;

	guid = GenerateGuid(EntityID, key, ENTITY_READER);

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
		readerPtr->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER) {
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
			itor->second->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER) {
			livelinessManager_->RemoveLivelinessWriter(itor->second);
		}
		writers_.erase(itor);
	} else {
		AGILOG(AGILOG_ERROR, "handle not exist");
		return RETCODE_ERROR;
	}
	{
		std::unique_lock<std::mutex> lock(mtxEndpoints_);
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
			itor->second->GetGuid().entityId != ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER) {
			livelinessManager_->RemoveLivelinessReader(itor->second);
		}
		readers_.erase(itor);
	} else {
		AGILOG(AGILOG_ERROR, "handle not exist");
		return RETCODE_ERROR;
	}
	{
		std::unique_lock<std::mutex> lock(mtxEndpoints_);
		endpoints_.erase(guid.entityId);
	}
	
	return RETCODE_OK;
}

std::shared_ptr<Writer> Participant::GetWriter(const EntityId_t & EntityID)
{
	std::unique_lock<std::mutex> lock(mtxWriters_);
	
	if (0 == writers_.count(EntityID)) {
		AGILOG(AGILOG_ERROR, "EntityID not exist");
		return nullptr;
	}
		
	return writers_[EntityID];
}

std::shared_ptr<Reader> Participant::GetReader(const EntityId_t & EntityID)
{
	std::unique_lock<std::mutex> lock(mtxReaders_);
	if (0 == readers_.count(EntityID)) {
		AGILOG(AGILOG_ERROR, "EntityID not exist");
		return nullptr;
	}

	return readers_[EntityID];
}

ReturnCode_t Participant::AddRemoteWriter(const DiscoveredWriterData& data)
{
	remoteWriters_.insert({ data.remoteWriterGuid, data });

	std::unique_lock<std::mutex> lock(mtxReaders_);
	for (auto& reader : readers_) {
		addReaderMatch(reader.second, data);
	}

	return RETCODE_OK;
}

ReturnCode_t Participant::AddRemoteReader(const DiscoveredReaderData& data)
{
	remoteReaders_.insert({ data.remoteReaderGuid, data });

	std::unique_lock<std::mutex> lock(mtxWriters_);

	for (auto& writer : writers_) {
		addWriterMatch(writer.second, data);
	}	
	
	return RETCODE_OK;
}

ReturnCode_t Participant::RemoveRemoteWriter(GUID_t guid)
{
	auto itor = remoteWriters_.find(guid);
	if (itor == remoteWriters_.end()) {
		return RETCODE_OK;
	}

	std::list<EntityId_t> readersEntityid;
	{
		std::unique_lock<std::mutex> lock(mtxMatchEntitys_);
		readersEntityid = matchEntitys_[guid];
		matchEntitys_.erase(guid);
	} 
	
	for(auto readerEntityid : readersEntityid) {
		std::unique_lock<std::mutex> lock(mtxReaders_);
		auto writerItor = readers_.find(readerEntityid);
		if (writerItor == readers_.end()) {
			continue;
		}
		auto reader = readers_[readerEntityid];
		if (!reader) {
			continue;
		}

		std::shared_ptr<StatefulReader> statefulReader = std::dynamic_pointer_cast<StatefulReader>(reader);

		if(statefulReader) {
			std::shared_ptr<WriterProxy> writerProxy = statefulReader->MatchedWriterLookup(guid);
			statefulReader->MatchedWriterRemove(writerProxy);

			//TODO 重新检测确认数据
		}

		std::shared_ptr<StatelessReader> statelessReader = std::dynamic_pointer_cast<StatelessReader>(reader);

		if (statelessReader) {

			//TODO 补充
			std::shared_ptr<WriterProxy> writerProxy = statelessReader->MatchedWriterLookup(guid);
			statelessReader->MatchedWriterRemove(writerProxy->GetGuid());
		}

		InstanceHandle_t handle = Conversion::ToInstanceHandle(itor->second.remoteWriterGuid);
		DataReaderImpl* dr = reader->GetRelatedDataReader();
		if (dr) {
			dr->OnSubscriptionMatched(handle, false);
		}
	}

	remoteWriters_.erase(itor);

	return RETCODE_OK;

}

ReturnCode_t Participant::RemoveRemoteReader(GUID_t guid)
{
	auto itor = remoteReaders_.find(guid);
	if (itor == remoteReaders_.end()) {
		return RETCODE_OK;
	}

	std::list<EntityId_t> writersEntityid;
	{
		std::unique_lock<std::mutex> lock(mtxMatchEntitys_);
		writersEntityid = matchEntitys_[guid];
		matchEntitys_.erase(guid);
	}
	
	for (auto writerEntityid : writersEntityid) {

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
			statefulWriter->MatchedReaderRemove(readerProxy);

			//TODO 重新检测确认数据
		}

		std::shared_ptr<StatelessWriter> statelessWriter = std::dynamic_pointer_cast<StatelessWriter>(writer);
		if (statelessWriter) {

			for (auto locator : itor->second.multicastLocatorList) {
				statelessWriter->ReaderLocatorRemove(locator);
			}

			for (auto locator : itor->second.unicastLocatorList) {
				statelessWriter->ReaderLocatorRemove(locator);
			}

		}

		InstanceHandle_t handle = Conversion::ToInstanceHandle(itor->second.remoteReaderGuid);
		writer->GetRelatedDataWriter()->OnPublicationMatched(handle,false);
	}

	remoteReaders_.erase(itor);

	return RETCODE_OK;
}


void Participant::GetParticipantAllWriter(const GUID_t& remoteParticipantGuid,std::vector<GUID_t>& guids)
{
	for (auto writer : remoteWriters_) {
		if (CompareGuidPrefix(remoteParticipantGuid.prefix, writer.first.prefix)) {
			guids.push_back(writer.first);
		}
	}
}

ReturnCode_t Participant::RemoveRemoteParticipantAllWriter(GUID_t remoteParticipantGuid)
{
	std::vector<GUID_t> guids;
	GetParticipantAllWriter(remoteParticipantGuid, guids);

	for (auto guid : guids) {
		RemoveRemoteWriter(guid);
	}

	return RETCODE_OK;
}

void Participant::GetParticipantAllReader(const GUID_t & remoteParticipantGuid, std::vector<GUID_t>& guids)
{
	for (auto reader : remoteReaders_) {
		if (CompareGuidPrefix(remoteParticipantGuid.prefix, reader.first.prefix)) {
			guids.push_back(reader.first);
		}
	}
}

ReturnCode_t Participant::RemoveRemoteParticipantAllReader(GUID_t remoteParticipantGuid)
{
	std::vector<GUID_t> guids;
	GetParticipantAllReader(remoteParticipantGuid, guids);

	for (auto guid : guids) {
		RemoveRemoteReader(guid);
	}

	return RETCODE_OK;
}

ReturnCode_t Participant::AddLocalWriter(std::shared_ptr<Writer> writer)
{
	for (auto& remoteReaderData : remoteReaders_) {
		addWriterMatch(writer, remoteReaderData.second);
	}
	return RETCODE_OK;
}

ReturnCode_t Participant::AddLocalReader(std::shared_ptr<Reader> reader)
{
	for (auto& remoteWriterData : remoteWriters_) {
		addReaderMatch(reader, remoteWriterData.second);
	}
	return RETCODE_OK;
}

GUID_t Participant::GenerateGuid(const EntityId_t& EntityID, bool key, EntityType type)
{
	GUID_t guid;
	char cKey = 0;

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
	GUID_t guid;
	memcpy(&guid, handle.keyHash.value, MIG_RTPS_KEY_HASH_MAX_LENGTH);
	return guid;
}

ReturnCode_t Participant::GenerateGuidPrefix(int32_t domainId, int32_t instanceId)
{
	int32_t processId = 0;
	
	//12 个字节
	GuidPrefix_t &prefix = guid_.prefix;

	//threadId = std::threa);


	//std::this_thread::get_id();


#if defined(_WIN32)
	processId = (int32_t)_getpid();
#else
	processId = (int32_t)getpid();
#endif // platform selection

	std::random_device generator;
	std::uniform_int_distribution<uint16_t> distribution(0, 65535);
	uint16_t rand_value = distribution(generator);

	// TODO host_id的处理方式
	uint16_t host_id = distribution(generator);
	prefix.value[0] = vendorId_[0];
	prefix.value[1] = vendorId_[1];

	char* temp = (char*)&host_id;
	prefix.value[2] = temp[0];
	prefix.value[3] = temp[1];

	temp = (char*)&processId;
	prefix.value[4] = temp[0];
	prefix.value[5] = temp[1];

	temp = (char*)&rand_value;
	prefix.value[6] = temp[0];
	prefix.value[7] = temp[1];

	temp = (char*)&instanceId;
	prefix.value[8] = temp[0];
	prefix.value[9] = temp[1];
	prefix.value[10] = temp[2];
	prefix.value[11] = temp[3];

	return RETCODE_OK;
}


GuidPrefix_t Participant::GetGuidPrefix()
{
	return guid_.prefix;
}
void Participant::addWriterMatch(std::shared_ptr<Writer> writer, const DiscoveredReaderData &subData)
{
    DataWriterImpl* dw = writer->GetRelatedDataWriter();
    if (nullptr == dw) {
		AGILOG(AGILOG_ERROR, "datawriter is nullptr");
        return;
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
		AGILOG(AGILOG_ERROR, "Loacl dw key = %x ,but remote dr key = %x", writer->GetGuid().entityId.entityKind, subData.remoteReaderGuid.entityId.entityKind);

		return;
	}

	std::set<QosPolicyId_t> qosSeq;
	bool ret = Match(pubData, subData.ddsSubscriptionData, qosSeq);
	if (!ret) {
		writer->GetRelatedDataWriter()->OnOfferedIncompatibleQos(qosSeq);
		return;
	}

	writer->MatchedReaderAdd(subData);

	std::unique_lock<std::mutex>  lock(mtxMatchEntitys_);
	auto writers = matchEntitys_.find(subData.remoteReaderGuid);
	if (matchEntitys_.end() == writers) {
		std::list<EntityId_t> writerList;
		writerList.push_back(writer->GetGuid().entityId);
		matchEntitys_[subData.remoteReaderGuid] = writerList;
	}
	else {
		writers->second.push_back(writer->GetGuid().entityId);
	}
	
	InstanceHandle_t handle = Conversion::ToInstanceHandle(subData.remoteReaderGuid);
	writer->GetRelatedDataWriter()->OnPublicationMatched(handle);
}

void Participant::addReaderMatch(std::shared_ptr<Reader> reader, const DiscoveredWriterData &pubData)
{
    DataReaderImpl* dr = reader->GetRelatedDataReader();
    if(nullptr == dr) {
        return;
    }
    const SubscriptionBuiltinTopicData& subData = dr->GetBuiltinTopicData();
	
	if (pubData.ddsPublicationData.topic_name != subData.topic_name) {
		return;
	}

	const TopicImpl* topicImpl = dynamic_cast<const TopicImpl*>(reader->GetRelatedDataReader()->get_topicdescription());
	if (nullptr == topicImpl)
		return;

	if(pubData.ddsPublicationData.type_name != subData.type_name)
	{
		const_cast<TopicImpl*>(topicImpl)->update_inconsistent_topic_status();
		return;
	}

	if ((0x02 == pubData.remoteWriterGuid.entityId.entityKind && reader->GetGuid().entityId.entityKind != 0x07) 
		|| (0x03 == pubData.remoteWriterGuid.entityId.entityKind && reader->GetGuid().entityId.entityKind != 0x04)) {
		const_cast<TopicImpl*>(topicImpl)->update_inconsistent_topic_status();
		AGILOG(AGILOG_ERROR, "remote dw key = %x ,but local dr key = %x", pubData.remoteWriterGuid.entityId.entityKind, reader->GetGuid().entityId.entityKind);
		return;
	}

	std::set<QosPolicyId_t> qosSeq;
	bool ret = Match(pubData.ddsPublicationData, subData, qosSeq);
	if (!ret) {
		reader->GetRelatedDataReader()->OnRequestedIncompatibleQos(qosSeq);
		return ;
	}


	if (reader->IsReliable()) {
		std::shared_ptr<WriterProxy> proxy = std::make_shared<WriterProxy>(pubData.remoteWriterGuid, pubData.remoteGroupGuid, pubData.unicastLocatorList, pubData.multicastLocatorList);
		StatefulReader* fulReader = dynamic_cast<StatefulReader*> (reader.get());
		if (fulReader) {
			fulReader->MatchedWriterAdd(proxy);
		} else {
			AGILOG(AGILOG_ERROR, "MatchedWriterAdd Err")
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
			AGILOG(AGILOG_ERROR, "MatchedWriterAdd Err")
		}
	}


	std::unique_lock<std::mutex>  lock(mtxMatchEntitys_);
	auto readers = matchEntitys_.find(pubData.remoteWriterGuid);
	if (matchEntitys_.end() == readers) {
		std::list<EntityId_t> readerList;
		readerList.push_back(reader->GetGuid().entityId);
		matchEntitys_[pubData.remoteWriterGuid] = readerList;
	}
	else {
		readers->second.push_back(reader->GetGuid().entityId);
	}

	InstanceHandle_t handle = Conversion::ToInstanceHandle(pubData.remoteWriterGuid);
	reader->GetRelatedDataReader()->OnSubscriptionMatched(handle);
}
std::shared_ptr<MessageTransmitter> Participant::GetMessageTransmitter()
{
	return messageTransmitter_;
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
	for (auto& writer : remoteWriters_) {

		if (false == includeContainedEntities && true == CompareGuidPrefix(guid_.prefix, writer.first.prefix)) {
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

int32_t Participant::CreateReceiverResources(LocatorVec& locatorList, bool applyMutation)
{
	LocatorVec inList = std::move(locatorList);
	locatorList.clear();
	ReturnCode_t ret;
	for (auto& itor : inList) {
		ret = messageTransmitter_->ReceiveAddrAdd(itor);
		if (RETCODE_OK != ret && applyMutation) {
			/* 限制重新尝试次数 */
			uint32_t tries = 100u;
			for (uint32_t i = 0; RETCODE_OK != ret && i < tries; i++) {
				itor.port += PARTICIPANTID_GAIN;
				ret = messageTransmitter_->ReceiveAddrAdd(itor);
			}
		}
		if (RETCODE_OK == ret) {
			locatorList.push_back(itor);
		} else {
			/* 异常处理 */
			AGILOG(AGILOG_ERROR, "CreateReceiverResources Err")
		}
	}
	return static_cast<int32_t>(locatorList.size());
}

void Participant::StopReceiveThread()
{
    messageTransmitter_->SetStop();
}

std::shared_ptr<ThreadPool> Participant::GetThreadPool() const
{
	return threadPool_;
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
		qosSeq.insert(DURABILITYSERVICE_QOS_POLICY_ID);
	}

	if (!((subData.presentation.access_scope <= pubData.presentation.access_scope) &&
		(!pubData.presentation.coherent_access || (subData.presentation.coherent_access == pubData.presentation.coherent_access)) &&
		(!pubData.presentation.ordered_access || (subData.presentation.ordered_access == pubData.presentation.ordered_access)))) {
		qosSeq.insert(PRESENTATION_QOS_POLICY_ID);
	}

	//TODO 分区匹配
	bool ret = true;
	if (0 != pubData.partition.name.size()  || 0 != subData.partition.name.size()) {
		ret = false;
	}
	for (auto pubName : pubData.partition.name) {
		if (ret) {
			/* 已经匹配成功 */
			break;
		}
		for (auto subName : subData.partition.name) {
			std::regex pubRegex(pubName);
			std::regex subRegex(subName);
			if (std::regex_match(subName, pubRegex)) {
				ret = true;
				break;
			}
			if (std::regex_match(pubName, subRegex)) {
				ret = true;
				break;
			}

		}
	}
	if (!ret) {
		qosSeq.insert(PARTITION_QOS_POLICY_ID);
	}

	return qosSeq.empty();
}

ReturnCode_t Participant::GetPublicationData(PublicationBuiltinTopicData &pubData, const InstanceHandle_t& handle)
{
	GUID_t guid = Conversion::ToGuid(handle);

	auto itor = remoteWriters_.find(guid);
	if (itor == remoteWriters_.end()) {
		AGILOG(AGILOG_ERROR, "handle not exist")
		return RETCODE_BAD_PARAMETER;
	}
	pubData = itor->second.ddsPublicationData;
	return RETCODE_OK;

}

ReturnCode_t Participant::GetSublications(InstanceHandleSeq & handles, bool includeContainedEntities)
{
	for (auto& reader : remoteReaders_) {

		if (false == includeContainedEntities && true == CompareGuidPrefix(guid_.prefix, reader.first.prefix)) {
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

	auto itor = remoteReaders_.find(guid);
	if (itor == remoteReaders_.end()) {
		AGILOG(AGILOG_ERROR, "handle not exist")
		return RETCODE_BAD_PARAMETER;
	}
	subData = itor->second.ddsSubscriptionData;
	return RETCODE_OK;
}

void Participant::GetMacthList(const GUID_t& writerGuid, std::list<EntityId_t>& endpointList)
{
	std::unique_lock<std::mutex>  lock(mtxMatchEntitys_);
	auto listItor = matchEntitys_.find(writerGuid);
	if (matchEntitys_.end() != listItor) {
		endpointList = listItor->second;
	}
	else {
		if (writerGuid.entityId == ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER) {
			endpointList.push_back(ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER);
		}
	}
	return;
}

/* wangyi 获取LivelinessManager */
std::shared_ptr<LivelinessManager> Participant::GetLivelinessManager()
{
	return livelinessManager_;
}
