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
#ifndef AGIDDS_RTPS_TRANSPORT_LOCATOR_H
#define AGIDDS_RTPS_TRANSPORT_LOCATOR_H

#include <asio.hpp>
#include "rtps/basictypertps.h"
AGIDDS_NAMESPACE_BEGIN
class UDPv4Locator : public Locator_t
{
public:
	UDPv4Locator(const Locator_t& loc);

	virtual ~UDPv4Locator() {};

	uint32_t GetUIAddressV4();

	asio::ip::address_v4 GetAddressV4();

	bool Valid();
private:
	bool valid_ = true;
};

AGIDDS_NAMESPACE_END

#endif
