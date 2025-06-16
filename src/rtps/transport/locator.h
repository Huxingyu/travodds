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

	/**
	 * @brief 构造函数。
	 *
	 * @param [in] loc 地址信息。
	 */
	UDPv4Locator(const Locator_t& loc);

	/**
	 * @brief 析构函数。
	 */
	virtual ~UDPv4Locator() {};

	/**
	 * @brief 将octet数组格式ip地址转换为uint32_t格式。
	 * 
	 * @return uint32_t格式ip地址。
	 */	
	uint32_t GetUIAddressV4();

	/**
	 * @brief 将octet数组格式ip地址转换为asio::ip::address_v4格式。
	 *
	 * @return asio::ip::address_v4格式ip地址。
	 */
	asio::ip::address_v4 GetAddressV4();

	/**
	 * @brief 获取有效性。
	 *
	 * @return 有效则返回true，否则返回false。
	 */
	bool Valid();
private:
	bool valid_ = true;
};

AGIDDS_NAMESPACE_END

#endif
