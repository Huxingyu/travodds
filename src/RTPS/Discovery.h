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
#pragma once
#include "AGIDDS_DLL.h"
#include "DDS/DomainParticipantImpl.h"
#include "RTPS/Spdp.h"
AGIDDS_NAMESPACE_BEGIN


class Discovery
{
	Discovery();

	void InitBuiltinEndpoint();

private:
	DomainParticipantImpl* domainParticipant_;
	// 内置发布者、订阅者
	PublisherImpl*  builtinPublisher_;
	SubscriberImpl* builtinSubScriber_;

	// 内置端点
	DataWriterImpl* builtinParticipantDataWriter_;
	DataWriterImpl* builtinPublicationsDataWriter_;
	DataWriterImpl* builtinSubscriptionsDataWriter_;
	DataWriterImpl* builtinParticipantMessageDataWriter_;

	DataReaderImpl* builtinParticipantDataReader_;
	DataReaderImpl* builtinPublicationsDataReader_;
	DataReaderImpl* builtinSubscriptionsDataReader_;
	DataReaderImpl* builtinParticipantMessageDataReader_;

	//内置主题
	TopicImpl* DCPSParticipant_;
	TopicImpl* DCPSSubscription_;
	TopicImpl* DCPSPublication_;
	TopicImpl* DCPSTopic_;

	//内置数据类型

	BuiltinEndpointSet_t builtinEndpointSet_;
	
	Spdp* spdp_;
	Sedp* sedp_;
};

AGIDDS_NAMESPACE_END
