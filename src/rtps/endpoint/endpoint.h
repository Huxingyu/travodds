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
#ifndef AGIDDS_RTPS_ENDPOINT_ENDPOINT_H
#define AGIDDS_RTPS_ENDPOINT_ENDPOINT_H

#include "rtps/endpoint/rtpsentity.h"

AGIDDS_NAMESPACE_BEGIN

class Participant;

/**
 * @brief Endpoint类, RTPS网络的所有实体都从该类派生。
 * 尽管Participant在RTPS规范中也被定义为端点，但在此实现中，Partipant类不会从端点类继承。
 * 每个端点对象都拥有一个指向其所属Participant的指针。
 */
class Endpoint : public RTPSEntity {
public:
	/**
	 * @brief 构造函数。
	 */
	Endpoint() 
		: participant_(nullptr), 
		  topicKind_({ WITH_KEY }),
		  reliabilityLevel_(0),
		  endpointId_(ENTITYID_UNKNOWN)
	{
	}

	/**
	 * @brief 设置Participant指针。
	 *
	 * @param participant Participant指针。
	 */
	void SetParticipant(Participant* participant) {
		participant_ = participant;
	};
protected:
	LocatorSeq unicastLocatorList_;
	LocatorSeq multicastLocatorList_;
	ReliabilityKind_t reliabilityLevel_;
	TopicKind_t topicKind_;
	EntityId_t endpointId_;
	Participant* participant_;
};

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_ENDPOINT_ENDPOINT_H
