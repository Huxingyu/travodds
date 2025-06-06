//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef AGIDDS_RTPS_ENDPOINT_PARTICIPANT_H
#define AGIDDS_RTPS_ENDPOINT_PARTICIPANT_H

#include "rtps/endpoint/rtpsentity.h"
#include "rtps/endpoint/reader.h"

AGIDDS_NAMESPACE_BEGIN

class Writer;
class Reader;
class DomainParticipantImpl;
class DataWriterImpl;
class DataReaderImpl;
class MessageProcessor;
class LivelinessManager;
class ThreadPool;

enum EntityType : char{
	ENTITY_DOMAIN,
	ENTITY_SUB,
	ENTITY_PUB,
	ENTITY_TOPIC,
	ENTITY_WRITER,
	ENTITY_READER
};

class MessageTransmitter;

class Participant: public RTPSEntity {
public:
	// Participant(const DomainParticipantImpl* domainImpl);
	Participant(DomainId_t id, std::shared_ptr<ThreadPool> threadPool);

	// Participant(const Participant&) = delete ;

	virtual ~Participant();

	ReturnCode_t Init();

    std::shared_ptr<Writer> CreateWriter(DataWriterImpl* dw, const DataWriterQos& dwQos, const EntityId_t& EntityID, bool key);

    std::shared_ptr<Reader> CreateReader(DataReaderImpl* dr, const DataReaderQos& drQos, const EntityId_t& EntityID, bool key);

	ReturnCode_t DeleteWriter(const InstanceHandle_t& handle);

	ReturnCode_t DeleteReader(const InstanceHandle_t& handle);

	std::shared_ptr<Writer> GetWriter(const EntityId_t& EntityID);

	std::shared_ptr<Reader> GetReader(const EntityId_t& EntityID);

	ReturnCode_t AddRemoteWriter(const DiscoveredWriterData& data);

	ReturnCode_t AddRemoteReader(const DiscoveredReaderData& data);

	ReturnCode_t RemoveRemoteWriter(GUID_t guid);

	ReturnCode_t RemoveRemoteReader(GUID_t guid);

	void GetParticipantAllWriter(const GUID_t& remoteParticipantGuid,std::vector<GUID_t> & guids);

	ReturnCode_t RemoveRemoteParticipantAllWriter(GUID_t remoteParticipantGuid);

	void GetParticipantAllReader(const GUID_t& remoteParticipantGuid, std::vector<GUID_t> & guids);

	ReturnCode_t RemoveRemoteParticipantAllReader(GUID_t remoteParticipantGuid);

	GUID_t GenerateGuid(const EntityId_t& EntityID, bool key, EntityType type);

	GUID_t GenerateGuid(const InstanceHandle_t& handle);

	ReturnCode_t GenerateGuidPrefix(int32_t domainId, int32_t instanceId);

	GuidPrefix_t GetGuidPrefix();

	ReturnCode_t AddLocalWriter(std::shared_ptr<Writer> writer);

	ReturnCode_t AddLocalReader(std::shared_ptr<Reader> reader);

	bool Match(const PublicationBuiltinTopicData& pubData, const SubscriptionBuiltinTopicData& subData, std::set<QosPolicyId_t>& qosSeq);
	
	/* wangyi 获取livelinessManager */
	std::shared_ptr<LivelinessManager> GetLivelinessManager();

	void addWriterMatch(std::shared_ptr<Writer> writer, const DiscoveredReaderData& subData);

	void addReaderMatch(std::shared_ptr<Reader> reader, const DiscoveredWriterData& pubData);

	//TODO 获取网络传输模块对象
	std::shared_ptr<MessageTransmitter> GetMessageTransmitter();

	LocatorVec GetDefaultUnicastLocators();

	LocatorVec GetDefaultMulticastLocators();

	LocatorVec GetMetatrafficUnicastLocators();

	LocatorVec GetMetatrafficMulticastLocators();

	ReturnCode_t GetPublications(InstanceHandleSeq& handles, bool includeContainedEntities);

	ReturnCode_t GetPublicationData(PublicationBuiltinTopicData& pubData, const InstanceHandle_t& handle);

	ReturnCode_t GetSublications(InstanceHandleSeq& handles, bool includeContainedEntities);

	ReturnCode_t GetSublicationData(SubscriptionBuiltinTopicData& subData, const InstanceHandle_t& handle);

	/* 获取和远端匹配的端点列表 GUID:远端端点的GUID, endpointList:本地匹配的端点信息 */
	void GetMacthList(const GUID_t& writerGuid, std::list<EntityId_t>& endpointList);

	int32_t CreateReceiverResources(LocatorVec& locatorList, bool applyMutation);

	/* 停止接收线程 */
	void StopReceiveThread();

	std::shared_ptr<ThreadPool> GetThreadPool() const;

private:
	LocatorVec defaultUnicastLocatorList_;
	LocatorVec defaultMulticastLocatorList_;
	LocatorVec metatrafficUnicastLocatorList_;
	LocatorVec metatrafficMulticastLocatorList_;
	ProtocolVersion_t protocolVersion_;
	VendorId_t vendorId_;

	uint32_t readerId_;
	uint32_t writerId_;
	uint32_t nextId_;

	DomainId_t domainId_;

	std::map<std::string, std::vector<SubscriptionBuiltinTopicData> > localReaders_;
	std::map<std::string, std::vector<PublicationBuiltinTopicData> > localWriters_;

	std::map<GUID_t, DiscoveredReaderData> remoteReaders_;
	std::map<GUID_t, DiscoveredWriterData> remoteWriters_;


	// const DomainParticipantImpl* const domainImpl_ ;

	std::map<EntityId_t, std::shared_ptr<Writer>> writers_;

	std::map<EntityId_t, std::shared_ptr<Reader>> readers_;

	std::map<EntityId_t, std::shared_ptr<Endpoint>>  endpoints_;

	/* 远端GUID和本地匹配关系的列表  key: 远端GUID， value：本地的 */
	std::map<GUID_t, std::list<EntityId_t>> matchEntitys_;

	//TODO 网络传输模块对象
	std::shared_ptr<MessageTransmitter> messageTransmitter_ = nullptr;

	std::shared_ptr<MessageProcessor> messageProcessor_ = nullptr;

	/* wangyi Liveliness 管理 */
	std::shared_ptr<LivelinessManager> livelinessManager_;

	std::shared_ptr<ThreadPool> threadPool_;
};

AGIDDS_NAMESPACE_END

#endif
