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

#include "RTPS/RTPSEntity.h"

AGIDDS_NAMESPACE_BEGIN

class Endpoint : public RTPSEntity {
protected:
	Locator_t* unicastLocatorList;
	Locator_t* multicastLocatorList;
	ReliabilityKind_t reliabilityLevel;
	TopicKind_t topicKind;
};

AGIDDS_NAMESPACE_END
