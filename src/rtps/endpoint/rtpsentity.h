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
#ifndef AGIDDS_RTPS_ENDPOINT_RTPSENTITY_H
#define AGIDDS_RTPS_ENDPOINT_RTPSENTITY_H

#include "rtps/basictypertps.h"

AGIDDS_NAMESPACE_BEGIN

/**
 * @brief RTPSEntity类是RTPS层支持QoS策略、监听器和状态条件的所有对象的抽象基类。
 */
class RTPSEntity {
public:
	/**
	 * @brief 获取当前对象的Guid信息。
	 *
	 * @return 返回当前对象的Guid信息。
	 */
	GUID_t GetGuid() const
	{
		return guid_;
	}
protected:
	GUID_t guid_;
};

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_ENDPOINT_RTPSENTITY_H
