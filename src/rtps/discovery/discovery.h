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
#ifndef TRAVODDS_RTPS_DISCOVERY_DISCOVERY_H
#define TRAVODDS_RTPS_DISCOVERY_DISCOVERY_H

#include "travodds_dll.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "rtps/discovery/spdp.h"

TRAVODDS_NAMESPACE_BEGIN
class Timer;

/**
 * Discovery类，实现RTPS规范中定义的简单端点发现协议。
 */
class Discovery
{
public:
	/**
	 * @brief 构造函数。
	 */
	Discovery(DomainParticipantImpl* domainParticipant, std::shared_ptr<ThreadPool> threadPool);

	/**
	 * @brief 析构函数。
	 */
	~Discovery();
	
	/**
	 * @brief 初始化本地内置参与者信息。
	 */
	void InitBuiltinEndpoint();

	/**
	 * @brief 生成并发送本地参与者Data(p)报文。
	 */
	void WriteSpdPdiscoveredParticipantData();
	
	/**
	 * @brief 生成并发送本地参与者释放Data(p)报文。
	 */
	void RemoveSpdPdiscoveredParticipantData();
	
	/**
	 * @brief 生成并发送本地发布者匹配报文(Data(w))。
	 * 
	 * @param data 发布者配置信息。
	 */
	void WritePublicationData(DiscoveredWriterData& data);
	
	/**
	 * @brief 生成并发送本地订阅者匹配报文(Data(r))。
	 *
	 * @param data 订阅者配置信息。
	 */
	void WriteSubscriptionData(DiscoveredReaderData& data);

	/**
	 * @brief 生成并发送本地发布者释放报文。
	 *
	 * @param handle 发布者句柄信息。
	 */
	void RemovePublicationData(InstanceHandle_t& handle);
	 
	/**
	 * @brief 生成并发送本地订阅者释放报文。
	 *
	 * @param handle 订阅者句柄信息。
	 */
	void RemoveSubscriptionData(InstanceHandle_t& handle);

	/**
	 * @brief 为内置端点配置代理 。
	 *
	 * @param data 内置端点代理信息。
	 */
	void AddProxy(const SPDPdiscoveredParticipantData& data);

	/**
	 * @brief 释放内置端点对应远端内置端点的代理信息 。
	 *
	 * @param data 远端内置端点guid。
	 */
	void RemoveProxy(GUID_t guid);

	/**
	 * @brief 释放内置端点对应远端内置端点的代理信息 。
	 *
	 * @param data 远端内置端点guid。
	 */
	void GetDiscoveredParticipants(InstanceHandleSeq& participant_handles);

	/**
	 * @brief 根据远端节点键值获取对应的远端节点配置信息 。
	 *
	 * @param [in] key 远端节点键值。
	 * @param [out] data 返回的键值对应的节点配置信息。
	 *
	 * @return 获取成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	int GetParticipantData(BuiltinTopicKey_t& key, SPDPdiscoveredParticipantData& data);

	/**
	 * @brief 获取线程池指针。
	 *
	 * @return 线程池指针。
	 */
	std::shared_ptr<ThreadPool> GetThreadPool() const;

	/**
	 * @brief 存储被忽略的远端节点信息。
	 *
	 * @param guid 被忽略的远端节点guid。
	 */
	void SetIgnoredParticipants(GUID_t guid);

	/**
	 * @brief 查询当前节点是否被忽略。
	 *
	 * @param guid 远端节点guid。
	 */
	bool FindIgnoredParticipants(GUID_t guid);

	/**
	 * @brief 获取内置订阅者，用于自动发现功能。
	 *
	 * @return 内置订阅者指针，如果未初始化则返回nullptr。
	 */
	std::shared_ptr<SubscriberImpl> GetBuiltinSubscriber() const;

private:
	DomainParticipantImpl* domainParticipant_;
	// 内置发布者、订阅者
	std::shared_ptr<PublisherImpl>  builtinPublisher_;
	std::shared_ptr<SubscriberImpl> builtinSubScriber_;

	//内置主题
	std::shared_ptr<TopicImpl> dcpsParticipant_;
	std::shared_ptr<TopicImpl> dcpsSubscription_;
	std::shared_ptr<TopicImpl> dcpsPublication_;
	std::shared_ptr<TopicImpl> dcpsParticipantMessage_;
	std::shared_ptr<TopicImpl> dcpsTopic_;

	// 内置端点
	std::shared_ptr<DataWriterImpl> builtinParticipantDataWriter_;
	std::shared_ptr<DataWriterImpl> builtinPublicationsDataWriter_;
	std::shared_ptr<DataWriterImpl> builtinSubscriptionsDataWriter_;
	std::shared_ptr<DataWriterImpl> builtinParticipantMessageDataWriter_;

	std::shared_ptr<DataReaderImpl> builtinParticipantDataReader_;
	std::shared_ptr<DataReaderImpl> builtinPublicationsDataReader_;
	std::shared_ptr<DataReaderImpl> builtinSubscriptionsDataReader_;
	std::shared_ptr<DataReaderImpl> builtinParticipantMessageDataReader_;

	//内置数据类型

	BuiltinEndpointSet_t builtinEndpointSet_;
	
	std::shared_ptr<Spdp> spdp_;
	std::shared_ptr<Sedp> sedp_;
	std::shared_ptr<ThreadPool> threadPool_;
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_RTPS_DISCOVERY_DISCOVERY_H
