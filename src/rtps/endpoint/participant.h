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
#ifndef TRAVODDS_RTPS_ENDPOINT_PARTICIPANT_H
#define TRAVODDS_RTPS_ENDPOINT_PARTICIPANT_H

#include "rtps/endpoint/rtpsentity.h"
#include "rtps/endpoint/reader.h"
#include "rtps/flowcontrol/flowcontroltype.h"
TRAVODDS_NAMESPACE_BEGIN

class Writer;
class Reader;
class DomainParticipantImpl;
class DataWriterImpl;
class DataReaderImpl;
class MessageProcessor;
class LivelinessManager;
class ThreadPool;
class Timer;
class FlowControlManager;
class FlowRateLimitedManager;

enum EntityType : char{
	ENTITY_DOMAIN,
	ENTITY_SUB,
	ENTITY_PUB,
	ENTITY_TOPIC,
	ENTITY_WRITER,
	ENTITY_READER
};

class MessageTransmitter;

/**
 * @brief Participant类，包含RTPS层Partipant的公共API。
 */
class Participant: public RTPSEntity {
public:
	// Participant(const DomainParticipantImpl* domainImpl);
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] id 域ID。
	 * @param [in] threadPool 线程池指针。
	 */
	Participant(DomainId_t id, const GUID_t& guid, std::shared_ptr<ThreadPool> threadPool);

	// Participant(const Participant&) = delete ;
	/**
	 * @brief 析构函数。
	 */
	virtual ~Participant();

	/**
	 * @brief Participant类初始化。
	 *
	 * @return 初始化成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t Init(const DomainParticipantQos & qos);

	/**
	 * @brief 初始化网络。
	 *
	 * @return 初始化成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t InitNetwork();

	/**
	 * @brief 创建发布端在rtps层映射Writer。
	 *  将创建完成的映射存储到容器中。
	 *
	 * @param [in] dw DataWriterImpl类指针。
	 * @param [in] dwQos DataWriter Qos。
	 * @param [in] GUID_t 实体id。
	 * @param [in] key 键值标志。
	 *
	 * @return 创建出的的发布端映射Writer指针。
	 */
    std::shared_ptr<Writer> CreateWriter(DataWriterImpl* dw, const DataWriterQos& dwQos, const GUID_t& guid, bool key, FlowControllerConfig flowConfig = FlowControllerConfig());

	/**
	 * @brief 创建发布端在rtps层映射Reader。
	 *	将创建完成的映射存储到容器中。
	 *
	 * @param [in] dw DataReaderImpl类指针。
	 * @param [in] dwQos DataReader Qos。
	 * @param [in] GUID_t 实体id。
	 * @param [in] key 键值标志。
	 *
	 * @return 创建出的的发布端映射Reader指针。
	 */
    std::shared_ptr<Reader> CreateReader(DataReaderImpl* dr, const DataReaderQos& drQos, const GUID_t& guid, bool key);

	/**
	 * @brief 从容器中删除发布端在rtps层映射Writer。
	 *
	 * @param [in] handle Writer对应的句柄。
	 *
	 * @return 删除成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t DeleteWriter(const InstanceHandle_t& handle);

	/**
	 * @brief 从容器中删除发布端在rtps层映射Reader。
	 *
	 * @param [in] handle Reader对应的句柄。
	 *
	 * @return 删除成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t DeleteReader(const InstanceHandle_t& handle);

	/**
	 * @brief 从容器中获取发布端在rtps层映射Writer。
	 *
	 * @param [in] EntityID Writer实体id。
	 *
	 * @return 获取成功返回对应的Writer指针，否则返回nullptr。
	 */
	std::shared_ptr<Writer> GetWriter(const EntityId_t& EntityID);

	/**
	 * @brief 从容器中获取发布端在rtps层映射Reader。
	 *
	 * @param [in] EntityID Reader实体id。
	 *
	 * @return 获取成功返回对应的Reader指针，否则返回nullptr。
	 */
	std::shared_ptr<Reader> GetReader(const EntityId_t& EntityID);

	/**
	 * @brief 添加远端Writer的信息到远端Writer存储容器中。
	 *  在添加前需与本地Reader进行匹配判断（数据类型、主题名、Qos配置等），判断结果一致方可添加。
	 *
	 * @param [in] data 远端Writer配置信息。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t AddRemoteWriter(const DiscoveredWriterData& data);

#ifdef DDSSECURITY
	ReturnCode_t AddRemoteSecureWriter(const DiscoveredWriterSecureData& data);
#endif // DDSSECURITY

	/**
	 * @brief 添加远端Reader的信息到远端Reader存储容器中。
	 *  在添加前需与本地Writer进行匹配判断（数据类型、主题名、Qos配置等），判断结果一致方可添加。
	 *
	 * @param [in] data 远端Reader配置信息。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t AddRemoteReader(const DiscoveredReaderData& data);

#ifdef DDSSECURITY
	ReturnCode_t AddRemoteSecureReader(const DiscoveredReaderSecureData& data);
#endif // DDSSECURITY

	/**
	 * @brief 从远端Writer存储容器中删除远端Writer。
	 *
	 * @param [in] data 远端Writer配置信息。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t RemoveRemoteWriter(GUID_t guid);

	/**
	 * @brief 从远端Reader存储容器中删除远端Reader。
	 *
	 * @param [in] data 远端Reader配置信息。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t RemoveRemoteReader(GUID_t guid);

	/**
	 * @brief 从远端Writer存储容器中获取同一个远端节点所有远端Writer的GUID。
	 *
	 * @param [in] remoteParticipantGuid 远端节点GUID。
	 * @param [out] guids 远端Writer的GUID集合。
	 */
	void GetParticipantAllWriter(const GUID_t& remoteParticipantGuid,std::vector<GUID_t> & guids);

	/**
	 * @brief 从远端Writer存储容器中删除同一个远端节点所有远端Writer。
	 *
	 * @param [in] remoteParticipantGuid 远端节点GUID。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t RemoveRemoteParticipantAllWriter(GUID_t remoteParticipantGuid);

	/**
	 * @brief 从远端Reader存储容器中获取同一个远端节点所有远端Reader的GUID。
	 *
	 * @param [in] remoteParticipantGuid 远端节点GUID。
	 * @param [out] guids 远端Reader的GUID集合。
	 */
	void GetParticipantAllReader(const GUID_t& remoteParticipantGuid, std::vector<GUID_t> & guids);

	/**
	 * @brief 从远端Reader存储容器中删除同一个远端节点所有远端Reader。
	 *
	 * @param [in] remoteParticipantGuid 远端节点GUID。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t RemoveRemoteParticipantAllReader(GUID_t remoteParticipantGuid);

	/**
	 * @brief GUID生成。
	 *	根据EntityId_t、key和EntityType生成对应的GUID
	 *
	 * @param [in] EntityID 实体ID。
	 * @param [in] key 是否存在键值标志。
	 * @param [in] type 实体类型。
	 *
	 * @return guid。
	 */
	GUID_t GenerateGuid(const EntityId_t& EntityID, bool key, EntityType type);

	/**
	 * @brief GUID生成。
	 *	根据InstanceHandle_t生成对应的GUID
	 *
	 * @param [in] handle 实体句柄。
	 *
	 * @return RETCODE_OK。
	 */
	GUID_t GenerateGuid(const InstanceHandle_t& handle);

	/**
	 * @brief 获取GUID前缀。
	 *
	 * @return GUID前缀值。
	 */
	GuidPrefix_t GetGuidPrefix();

	/**
	 * @brief 添加本地Writer。
	 *	与存储在远端Reader容器中的Reader一一判断是否匹配，匹配则加入Writer匹配容器中。
	 *
	 * @param [in] writer 本地Writer指针。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t AddLocalWriter(std::shared_ptr<Writer> writer);

	/**
	 * @brief 添加本地Reader。
	 *	与存储在远端Writer容器中的Writer一一判断是否匹配，匹配则加入Reader匹配容器中。
	 *
	 * @param [in] reader 本地Reader指针。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t AddLocalReader(std::shared_ptr<Reader> reader);

	/**
	 * @brief Reader与Writer的Qos匹配判断。
	 *	根据DDS协议判断Writer与Reader设置的各Qos是否符合协议要求的匹配关系。
	 *
	 * @param [in] pubData Writer Qos配置。
	 * @param [in] subData Reader Qos配置。
	 * @param [out] qosSeq Qos匹配结果容器。
	 *
	 * @return 匹配成功返回true，否则返回false。
	 */
	bool Match(const PublicationBuiltinTopicData& pubData, const SubscriptionBuiltinTopicData& subData, std::set<QosPolicyId_t>& qosSeq);
	
	/**
	 * @brief 获取livelinessManager类指针。
	 * 
	 * @return livelinessManager类指针。
	 */
	std::shared_ptr<LivelinessManager> GetLivelinessManager();

	static bool IsRegexPattern(const std::string& str);

	bool RegexPartitionMatch(const std::string& pubName, const std::string& subName);

	bool RegexPartitionsMatch(const StringSeq& pubPartitions, const StringSeq& subPatterns);


	/**
	 * @brief 判断本地Writer的信息是否与远端Reader匹配，匹配则存储到远端Reader存储容器中。
	 *
	 * @param [in] writer 本地Writer指针。
	 * @param [in] subData 远端Reader配置信息。
	 */
	void addWriterMatch(std::shared_ptr<Writer> writer, const DiscoveredReaderData& subData);
#ifdef DDSSECURITY
	void addWriterMatch(std::shared_ptr<Writer> writer, const DiscoveredReaderSecureData& subData);
#endif // DDSSECURITY
	/**
	 * @brief 判断本地Reader的信息是否与远端Writer匹配，匹配则存储到远端Writer存储容器中。
	 *
	 * @param [in] reader 本地Reader指针。
	 * @param [in] pubData 远端Writer配置信息。
	 */
	void addReaderMatch(std::shared_ptr<Reader> reader, const DiscoveredWriterData& pubData);
#ifdef DDSSECURITY
	void addReaderMatch(std::shared_ptr<Reader> reader, const DiscoveredWriterSecureData& pubData);
#endif // DDSSECURITY
	/**
	 * @brief 获取网络传输模块对象。
	 *
	 * @return 网络传输模块对象。
	 */
	std::vector<std::shared_ptr<MessageTransmitter>> GetMessageTransmitters();

	/* 向同一目标的地址发送 */
	void TransmittersSendMessage(LocatorVec destinationList, char* message, uint32_t length);
	/* std::vector<LocatorVec>&  destinationList 采用二级存储, 第一级存储目标，第二级存储同一目标的地址 */
	void TransmittersSendMessage(std::vector<LocatorVec>& destinationList, char* message, uint32_t length);

	void FlowRateLimitedMessageSend(std::vector<LocatorVec>& destinationList, char* message, uint32_t length);

	void TransmittersConnect(const LocatorVec& remoteLocator);

	void TransmittersDisconnect(const LocatorVec& remoteLocator);

	/**
	 * @brief 获取默认单播定位器。
	 *
	 * @return 默认单播定位器。
	 */
	LocatorVec GetDefaultUnicastLocators();

	/**
	 * @brief 获取默认组播定位器。
	 *
	 * @return 默认组播定位器。
	 */
	LocatorVec GetDefaultMulticastLocators();

	/**
	 * @brief 获取默认组播定位器。
	 *
	 * @return 默认组播定位器。
	 */
	LocatorVec GetMetatrafficUnicastLocators();

	/**
	 * @brief 获取Metatraffic单播定位器。
	 *
	 * @return Metatraffic单播定位器。
	 */
	LocatorVec GetMetatrafficMulticastLocators();

	/**
	 * @brief 获取已发现的DataWriter列表。
	 *
	 * @param [out] handles Writer InstanceHandle 列表。
	 * @param [in] includeContainedEntities 包括包含的实体标志。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetPublications(InstanceHandleSeq& handles, bool includeContainedEntities);

	/**
	 * @brief 获取指定句柄的DataWriter信息。
	 *
	 * @param [out] pubData Writer Qos信息。
	 * @param [in] handle 实体句柄。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetPublicationData(PublicationBuiltinTopicData& pubData, const InstanceHandle_t& handle);

	/**
	 * @brief 获取已发现的DataReader列表。
	 *
	 * @param [out] handles Reader InstanceHandle 列表。
	 * @param [in] includeContainedEntities 包括包含的实体标志。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetSublications(InstanceHandleSeq& handles, bool includeContainedEntities);

	/**
	 * @brief 获取指定句柄的DataReader信息。
	 *
	 * @param [out] subData Reader Qos信息。
	 * @param [in] handle 实体句柄。
	 *
	 * @return RETCODE_OK。
	 */
	ReturnCode_t GetSublicationData(SubscriptionBuiltinTopicData& subData, const InstanceHandle_t& handle);

	/* 获取和远端匹配的端点列表 GUID:远端端点的GUID, endpointList:本地匹配的端点信息 */
	/**
	 * @brief 获取和远端匹配的端点列表。
	 *
	 * @param [in] writerGuid 远端端点的GUID。
	 * @param [out] endpointList 本地匹配的端点信息。
	 */
	void GetMacthList(const GUID_t& writerGuid, std::list<EntityId_t>& endpointList);

	/**
	 * @brief 创建接收器资源。
	 *
	 * @param [out] locatorList 接收器资源列表。
	 * @param [in] applyMutation 是否应用变化。
	 *
	 * @return 接收器资源列表大小。
	 */
	int32_t CreateReceiverResources(std::shared_ptr<MessageTransmitter> messageTransmitter, LocatorVec& locatorList, bool applyMutation);

	/**
	 * @brief 停止接收线程 
	 */
	void StopReceiveThread();

	/**
	 * @brief 获取线程池。
	 *
	 * @return 线程池指针。
	 */
	std::shared_ptr<ThreadPool> GetThreadPool() const;

	void GetMatchedEntitys(const EntityId_t& entity, InstanceHandleSeq& handles) const;
	bool RemoteEntityIsMatched(const EntityId_t& entity, const InstanceHandle_t& remotehandle) const;

#ifdef DDSSECURITY
	std::map<EntityId_t, std::shared_ptr<Writer>>* GetWriterMap()
	{
		return &writers_;
	}
	std::map<EntityId_t, std::shared_ptr<Reader>>* GetReaderMap()
	{
		return &readers_;
	}
#endif // DDSSECURITY
private:
	DomainParticipantQos qos_;

	LocatorVec IPv4LocatorList_;
	LocatorVec IPv6LocatorList_;
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

	// 本地缓存发现的远端DiscoveredReaderData和DiscoveredWriterData
	std::map<GUID_t, DiscoveredReaderData> remoteReaders_;
	std::map<GUID_t, DiscoveredWriterData> remoteWriters_;
#ifdef DDSSECURITY
	std::map<GUID_t, DiscoveredReaderSecureData> remoteSecureReaders_;
	std::map<GUID_t, DiscoveredWriterSecureData> remoteSecureWriters_;
#endif // DDSSECURITY
	// const DomainParticipantImpl* const domainImpl_ ;
    std::unique_ptr<FlowRateLimitedManager> flowRateLimitedManager_;

    std::unique_ptr<FlowControlManager> flowControlManager_;

	std::shared_ptr<Timer> hbTimer_;

	std::map<EntityId_t, std::shared_ptr<Writer>> writers_;
	
	std::map<EntityId_t, std::shared_ptr<Reader>> readers_;

	std::map<EntityId_t, std::shared_ptr<Endpoint>>  endpoints_;

	/* 远端GUID和本地匹配关系的列表  key: 远端GUID， value：本地的 */
	std::map<GUID_t, std::list<EntityId_t>> matchLocalEntitys_;
	/* 本地EntityId_t和远端匹配关系的列表  key: 本地的， value：远端GUID */
	std::map<EntityId_t, std::set<GUID_t>> matchRemoteEntitys_;

	//TODO 网络传输模块对象
	std::vector<std::shared_ptr<MessageTransmitter>> messageTransmitters_;

	std::shared_ptr<MessageProcessor> messageProcessor_ = nullptr;

	/* wangyi Liveliness 管理 */
	std::shared_ptr<LivelinessManager> livelinessManager_;

	std::shared_ptr<ThreadPool> threadPool_;

	/* 互斥访问writers_  */
	mutable std::mutex mtxWriters_;

	/* 互斥访问readers_  */
	mutable std::mutex mtxReaders_;

	/* 互斥访问endpoints_ */
	mutable std::mutex mtxEndpoints_;

	/* 互斥访问matchEntitys_ */
	mutable std::mutex mtxMatchEntitys_;

#ifdef DDSSECURITY
public:
	//新增所属域参与者
	DomainParticipantImpl* participantImpl_;
#endif // DDSSECURITY
};

TRAVODDS_NAMESPACE_END

#endif
