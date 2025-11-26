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
#ifndef TRAVODDS_RTPS_DISCOVERY_SPDP_H
#define TRAVODDS_RTPS_DISCOVERY_SPDP_H

#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/statelesswriter.h"
#include "rtps/endpoint/statelessreader.h"
#include "rtps/discovery/discoverydata.h"
#include "rtps/discovery/sedp.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
#ifdef DDSSECURITY
#include "security/crycommonsource.h"
#include "security/authentication.h"
#endif // DDSSECURITY

TRAVODDS_NAMESPACE_BEGIN

class Discovery;

struct ParticipantStatus_t
{
	BuiltinTopicKey_t key;
	double leaseDuration;
	double recvTime;
};

#ifdef DDSSECURITY
/**
 * @typedef struct DiscoveryDPInfo
 * @brief   用于表示发现发现的DomainParticipant。
 */
typedef struct DiscoveryDPInfo
{
	/** @brief   用于表示对端DomainParticipant信息。 */
	SPDPdiscoveredParticipantData pData_;
	/** @brief   上一个SequenceNumber。 */
	SequenceNumber_t lastSeq_;
	/** @brief   用于获取定时器回调函数中相应的SPDP。 */
	Spdp* owner_;

	enum DDSSECURITY_ENDPOINT_STATE
	{
		DDSSEC_HANDSHAKE_EMPTY,
		DDSSEC_HANDSHAKE_REQUESTING,
		DDSSEC_HANDSHAKE_WAITING_REQUEST,
		DDSSEC_HANDSHAKE_WAITING_REPLY,
		DDSSEC_HANDSHAKE_WAITING_FINAL,
		DDSSEC_HANDSHAKE_OK,
		DDSSEC_WAITING_CRYPTO_TOKEN,
		DDSSEC_OK,
	};
	/** @brief 指明对端DomainParticipant是否启用了安全 */
	bool enableDDSSec_;
	/** @brief 指明对端DomainParticipant与本地DomainParticipant关联的状态 */
	DDSSECURITY_ENDPOINT_STATE endpointState_;
	/** @brief 远端DomainParticipant身份验证句柄 */
	IdentityHandle identityHdl_;
	/** @brief 远端DomainParticipant握手句柄 */
	HandshakeHandle handshakeHdl_;
	/** @brief 远端DomainParticipant握手消息 */
	bool handshakMsgInit_ = false;
	ParticipantStatelessMessage* handshakeMsg_ = NULL;
	/** @brief 远端DomainParticipant共用密钥 */
	SharedSecretHandle sharedSecretHdl_;
	/** @brief 远端DomainParticipant访问控制句柄 */
	PermissionsHandle permissionsHdl_;
	/** @brief 远端DomainParticipant加密句柄 */
	ParticipantCryptoHandle cryptoHdl_ = NULL;
}DiscoveryDPInfo;
#endif // DDSSECURITY

/**
 * @brief Spdp类，内置发现控制读写者数据收发控制类。
 *	实现内置发现功能，用户节点发现报文（DATA(p)）收发，节点发现管理功能。
 */
class Spdp: public ReaderListener
{
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] participant  Participant类指针。
	 * @param [in] discovery	Discovery类指针。
	 */
	Spdp(Participant* participant,Discovery* discovery);
	
	/**
	 * @brief 析构函数。
	 */
	~Spdp();

	/**
	 * @brief 内置DATA(p)报文数据读者回调函数。
	 *
	 * @param [in] change DATA(p)报文数据。
	 */
	virtual void on_data_available(std::shared_ptr<CacheChange> change);

	/**
	 * @brief DATA(p)报文RTPS层发布者内置Writer创建。
	 *
	 * @param [in] dataWriter  DataWriterImpl类指针。
	 */
	void SetParticipantDataWriter(std::shared_ptr<DataWriterImpl> dataWriter);

	/**
	 * @brief DATA(p)报文RTPS层订阅者内置Writer创建。
	 *
	 * @param [in] dataReader  DataReaderImpl类指针。
	 */
	void SetParticipantDataReader(std::shared_ptr<DataReaderImpl> dataReader);

	/**
	 * @brief 删除DDS通信参与者节点信息。
	 *
	 * @param [in] guid  通信参与者Guid。
	 */
	void RemoveParticipant(GUID_t guid);

	/**
	 * @brief 构建并发送DDS通信参与者节点发现报文。
	 *
	 * @param [in] data  当前节点发现信息。
	 */
	void WriteSpdPdiscoveredParticipantData(SPDPdiscoveredParticipantData& data);

	/**
	 * @brief 构建并发送DDS通信参与者节点退出报文。
	 *
	 * @param [in] guid  当前节点guid。
	 */
	void RemoveSpdPdiscoveredParticipantData(GUID_t& guid);

	/**
	 * @brief 创建新节点时，为内置发现报文主题添加代理。
	 *
	 * @param [in] data 新节点信息。
	 */
	void AddSpdpProxy(const SPDPdiscoveredParticipantData& data);

	/**
	 * @brief 删除节点时，为内置发现报文主题删除代理。
	 *
	 * @param [in] guid 节点Guid。
	 */
	void RemoveSpdpProxy(const GUID_t& guid);

	/**
	 * @brief 删除节点时，为内置发现报文主题删除代理。
	 *
	 * @param [in] guid 节点Guid。
	 */
	void CheckRemoteParticipantDuration();

	/**
	 * @brief 获取已过期参与者。
	 *
	 * @param [out] keys 已过期参与者键值集合。
	 */
	void GetOverdueParticipant(std::vector<BuiltinTopicKey_t>& keys);

	/**
	 * @brief 获取已发现的参与者。
	 *
	 * @param [out] keys 已发现参与者句柄集合。
	 */
	void GetDiscoveredParticipants(InstanceHandleSeq& participant_handles);

	/**
	 * @brief 根据参与者BuiltinTopicKey_t键值获取指定参与者配置信息。
	 *
	 * @param [in]  key  参与者键值。
	 * @param [out] data 键值对应的参与者配置信息。
	 * 
	 * @return 获取成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	int GetParticipantData(BuiltinTopicKey_t& key, SPDPdiscoveredParticipantData& data);

	/**
	 * @brief 保存要忽略的参与者Guid到忽略的参与者集合中。
	 *
	 * @param [in]  guid  参与者Guid。
	 */
	void SetIgnoredParticipants(GUID_t guid);

	/**
	 * @brief 根据参与者Guid判断指定参与者是否已被忽略。
	 *
	 * @param [in]  guid  参与者Guid。
	 *
	 * @return 已被忽略返回true，否则返回false。
	 */
	bool FindIgnoredParticipants(GUID_t guid);

#ifdef DDSSECURITY
	Discovery* GetDiscovery();

	bool ValidateParticipantGenericMessage(GUID_t destGuid);

	int ValidateRemoteParticipant(DiscoveryDPInfo* dpInfo);

	/**
     * @fn ZR_INT32 SpdpCleanupRemoteSecRsc(DiscoveryDPInfo* dpInfo);
     *
     * @brief 清除远端安全相关资源
     *
     * @param dpInfo        远端关联的资源
     *
     * @return 成功返回0，否则返回非0值。
     */
    int SpdpCleanupRemoteSecRsc(DiscoveryDPInfo* dpInfo);

	void RemoveDiscoveryDPInfoFromDiscParts(const GuidPrefix_t& prefix);

	ParticipantCryptoHandle GetRemoteParticipantCryptoHandle(const GuidPrefix_t* remotePrefix);

	SharedSecretHandle GetRemoteParticipantSharedSecret(const GuidPrefix_t* remotePrefix);

	std::map<GuidPrefix_t, DiscoveryDPInfo*> discParts_;

	SerializedBuffer* bigEndianMsgData_;
#endif // DDSSECURITY

private:

	/**
	 * @brief 解析DATA(p)报文获取远端参与者配置信息。
	 *
	 * @param [out]  data  解析获取的远端参与者配置信息。
	 * @param [in]   buffer  DATA(p)报文指针。
	 */
	void TakeSPDPdiscoveredParticipantData(SPDPdiscoveredParticipantData& data, SerializedBuffer* buffer);

	/**
	 * @brief 远端参与者管理函数。
	 *  判断当前远端参与者是否已添加到远端参与者管理容器中，未添加则执行添加操作，
	 *  已添加则更新参与者发现报文接收时间。
	 *
	 * @param [in] data  远端参与者配置信息。
	 */
	void ReceiveSPDPdiscoveredParticipantData(SPDPdiscoveredParticipantData& data);

private:
	Participant* participant_;
	std::shared_ptr<StatelessWriter> writer_ = nullptr;
	std::shared_ptr<StatelessReader> reader_ = nullptr;

	SpdpDataMap remoteParticipantData_;
	std::map<BuiltinTopicKey_t, ParticipantStatus_t> remoteParticipantStatus_;
	std::mutex mutex_;

	/* 被忽略的参与者 */
	std::vector<GUID_t> ignoredParticipants_;
	std::mutex ignoredParticipantsMutex_;

	Discovery* discovery_;
	
	std::shared_ptr<MemoryPool> memoryPool_;

	
	Duration_t leaseDuration_;
	std::shared_ptr<Timer> timer_;
};

TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_RTPS_DISCOVERY_SPDP_H
