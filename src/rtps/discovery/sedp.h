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
#ifndef TRAVODDS_RTPS_DISCOVERY_SEDP_H
#define TRAVODDS_RTPS_DISCOVERY_SEDP_H

#include "travodds_dll.h"
#include "rtps/discovery/discoverydata.h"
#include "rtps/endpoint/statefulwriter.h"
#include "rtps/endpoint/statefulreader.h"
#include "rtps/endpoint/participant.h"
#ifdef DDSSECURITY
#include "security/participantmessagedata.h"
#include "security/participantgenericmessage.h"
#endif // DDSSECURITY

TRAVODDS_NAMESPACE_BEGIN

class Spdp;
/**
 * @brief Sedp类，内置匹配控制读写者数据收发控制类。
 *	实现内置数据读写者匹配，用户节点匹配报文（DATA(w)和DATA(r)）收发，匹配节点管理功能。
 */
class Sedp : public ReaderListener {

public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] participant  Participant类指针。
	 * @param [in] spdp			Spdp类指针。
	 */
	Sedp(Participant* participant, Spdp* spdp);
	
	/**
	 * @brief 析构函数。
	 */
	~Sedp();
	
	/**
	 * @brief RTPS层发布者内置Writer和内置Reader创建。
	 *  用于收发DATA(w)报文 
	 * 
	 * @param [in] dataWriter  DataWriterImpl类指针。
	 * @param [in] dataReader  DataReaderImpl类指针。
	 */
	void SetPublicationEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader);
#ifdef DDSSECURITY
    void SetPublicationSecureEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader);
#endif // DDSSECURITY
	/**
	 * @brief RTPS层订阅者内置Writer和内置Reader创建。
	 *  用于收发DATA(r)报文
	 *
	 * @param [in] dataWriter  DataWriterImpl类指针。
	 * @param [in] dataReader  DataReaderImpl类指针。
	 */
	void SetSubscriptionEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader);

#ifdef DDSSECURITY
    void SetSubscriptionSecureEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader);
    void SetParticipantMessageSecureEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader);
    void SetParticipantStatelessMessageEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader);
    void SetParticipantVolatileSecureMessageEndpoint(DataWriterImpl* dataWriter, DataReaderImpl* dataReader);
#endif // DDSSECURITY
	/**
	 * @brief 发现新节点时，为内置主题添加代理。
	 *
	 * @param [in] data  新节点信息。
	 */
	void AddSedpProxy(const SPDPdiscoveredParticipantData& data);
#ifdef DDSSECURITY
	void AddPartSedpProxy(const SPDPdiscoveredParticipantData& data);

	void AddRemainSedpProxy(const SPDPdiscoveredParticipantData& data);


	// SedpAssociationSpdpEntities
	void AddBuiltinSpdpProxy(const SPDPdiscoveredParticipantData& data);

	// SedpAssociationSedpEntities
	void AddBuiltinSedpProxy(const SPDPdiscoveredParticipantData& data);

	// SedpAssociationBuiltinSecureSpdpEntities共享秘钥后完成后调用
	void AddVolatileMessageBuiltinSecureSpdpProxy(const SPDPdiscoveredParticipantData& data);

	// SedpAssociationBuiltinSecureSedpEntities握手完成或者秘钥交换完成后调用
	void AddBuiltinSecureSedpProxy(const SPDPdiscoveredParticipantData& data);
#endif // DDSSECURITY
	/**
	 * @brief 已发现节点退出时，为内置主题删除代理。
	 *
	 * @param [in] guid 退出节点guid。
	 */
	void RemoveSedpProxy(GUID_t guid);

	/**
	 * @brief 内置DATA(r)/DATA(w)报文数据读者回调函数。
	 *
	 * @param [in] change DATA(r)/DATA(w)报文数据。
	 */
	virtual void on_data_available(std::shared_ptr<CacheChange> change);

	/**
	 * @brief 构建并发送DATA(w)报文。
	 *
	 * @param [in] data DATA(w)报文参数。
	 */
	void WritePublicationData(DiscoveredWriterData& data);

#ifdef DDSSECURITY
	void WritePublicationSecureData(DiscoveredWriterSecureData& data);
#endif // DDSSECURITY

	/**
	 * @brief 构建并发送DATA(r)报文。
	 *
	 * @param [in] data DATA(r)报文参数。
	 */
	void WriteSubscriptionData(DiscoveredReaderData& data);

#ifdef DDSSECURITY
	enum StatelessMessageDataKind {
		REQUEST,
		REPLY,
		FINAL,
		OTHER
	};
	void WriteSubscriptionSecureData(DiscoveredReaderSecureData& data);
	void WriteParticipantMessageSecureData(ParticipantMessageSecureData& data);
	
	void WriteParticipantStatelessMessageData(const GUID_t* remoteGuid, ParticipantStatelessMessage& data, StatelessMessageDataKind kind);
	void WriteParticipantVolatileSecureMessage(const GUID_t* remoteGuid, ParticipantVolatileSecureMessage& data);
#endif // DDSSECURITY

	/**
	 * @brief 当调用delete_datawriter函数时触发，发送Writer下线报文。
	 *
	 * @param [in] guid 下线的Writer Guid。
	 */
	void RemovePublicationData(GUID_t& guid);

	/**
	 * @brief 当调用delete_datareader函数时触发，发送Reader下线报文。
	 *
	 * @param [in] guid 下线的Reader Guid。
	 */
	void RemoveSubscriptionData(GUID_t& guid);

private:
	/**
	 * @brief 解析DATA(w)报文参数并存储到DiscoveredWriterData结构中。
	 *
	 * @param [out] data  远端DataWriter配置参数。
	 * @param [in] buffer DATA(w)报文。
	 */
	void TakeDiscoveredWriterData(DiscoveredWriterData& data, SerializedBuffer* buffer);

#ifdef DDSSECURITY
	void TakeDiscoveredWriterSecureData(DiscoveredWriterSecureData& data, SerializedBuffer* buffer);
#endif // DDSSECURITY

	/**
	 * @brief 添加远端DataWriter信息到远端远端DataWriter管理列表中。
	 *
	 * @param [in] data 远端DataWriter信息。
	 */
	void ReceiveDiscoveredWriterData(DiscoveredWriterData& data);

#ifdef DDSSECURITY
	void ReceiveDiscoveredWriterSecureData(DiscoveredWriterSecureData& data);
#endif // DDSSECURITY

	/**
	 * @brief 解析DATA(r)报文参数并存储到DiscoveredReaderData结构中。
	 *
	 * @param [in] data 远端DataReader配置参数。
	 * @param [in] buffer DATA(r)报文。
	 */
	void TakeDiscoveredReaderData(DiscoveredReaderData& data, SerializedBuffer* buffer);

#ifdef DDSSECURITY
	void TakeDiscoveredReaderSecureData(DiscoveredReaderSecureData& data, SerializedBuffer* buffer);
	void TakeParticipantMessageSecureData(ParticipantMessageSecureData& data, SerializedBuffer* buffer);
	void TakeParticipantGenericMessageData(ParticipantGenericMessage& data, SerializedBuffer* buffer);
#endif // DDSSECURITY

	/**
	 * @brief 添加远端DataReader信息到远端远端DataReader管理列表中。
	 *
	 * @param [in] data 远端DataReader信息。
	 */
	void ReceiveDiscoveredReaderData(DiscoveredReaderData& data);

#ifdef DDSSECURITY
	void ReceiveDiscoveredReaderSecureData(DiscoveredReaderSecureData& data);

	void ReceiveParticipantGenericMessageData(ParticipantGenericMessage& data);

	uint32_t SetRemoteDomainEntityCryptoTokens(const ParticipantVolatileSecureMessage* sample, bool isReader);
#endif // DDSSECURITY
private:
	Spdp* spdp_;
	Participant* participant_;

	std::shared_ptr<StatefulWriter> publicationsWriter_  = nullptr;
	std::shared_ptr<StatefulWriter> subscriptionsWriter_ = nullptr;
	std::shared_ptr<StatefulReader> publicationsReader_  = nullptr;
	std::shared_ptr<StatefulReader> subscriptionsReader_ = nullptr;
#ifdef DDSSECURITY
	std::shared_ptr<StatefulWriter> publicationsSecureWriter_ = nullptr;
	std::shared_ptr<StatefulWriter> subscriptionsSecureWriter_ = nullptr;
	std::shared_ptr<StatefulWriter> participantMessageSecureWriter_ = nullptr;
	std::shared_ptr<StatefulWriter> participantStatelessMessageWriter_ = nullptr;
	std::shared_ptr<StatefulWriter> participantVolatileSecureMessageWriter_ = nullptr;
	std::shared_ptr<StatefulReader> publicationsSecureReader_ = nullptr;
	std::shared_ptr<StatefulReader> subscriptionsSecureReader_ = nullptr;
	std::shared_ptr<StatefulReader> participantMessageSecureReader_ = nullptr;
	std::shared_ptr<StatefulReader> participantStatelessMessageReader_ = nullptr;
	std::shared_ptr<StatefulReader> participantVolatileSecureMessageReader_ = nullptr;
	SequenceNumber_t statelessMsgSeqNum_;
#endif // DDSSECURITY
	std::shared_ptr<MemoryPool> memoryPool_;
};

TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_RTPS_DISCOVERY_SEDP_H
