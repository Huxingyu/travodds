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
#ifndef AGIDDS_RTPS_DISCOVERY_DISCOVERY_H
#define AGIDDS_RTPS_DISCOVERY_DISCOVERY_H

#include "agidds_dll.h"
#include "dcps/domain/domainparticipantimpl.h"
#include "rtps/discovery/spdp.h"

AGIDDS_NAMESPACE_BEGIN
class Timer;

class Discovery
{
public:
	Discovery(DomainParticipantImpl* domainParticipant, std::shared_ptr<ThreadPool> threadPool);

	~Discovery();
	
	void InitBuiltinEndpoint();

	void WriteSpdPdiscoveredParticipantData();

	void RemoveSpdPdiscoveredParticipantData();

	void WritePublicationData(DiscoveredWriterData& data);

	void WriteSubscriptionData(DiscoveredReaderData& data);

	void RemovePublicationData(InstanceHandle_t& handle);
	 
	void RemoveSubscriptionData(InstanceHandle_t& handle);

	// 为内置端点配置代理 
	void AddProxy(const SPDPdiscoveredParticipantData& data);

	void RemoveProxy(GUID_t guid);

	void GetDiscoveredParticipants(InstanceHandleSeq& participant_handles);

	int GetParticipantData(BuiltinTopicKey_t& key, SPDPdiscoveredParticipantData& data);

	std::shared_ptr<ThreadPool> GetThreadPool() const;

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

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_RTPS_DISCOVERY_DISCOVERY_H
