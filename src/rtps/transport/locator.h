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


class AsioLocator : public Locator_t
{
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] loc 地址信息。
	 */
	AsioLocator(const Locator_t& loc);
	/**
	 * @brief 析构函数。
	 */
	virtual ~AsioLocator() {};
	/**
	 * @brief 验证地址是否是多播地址。
	 *
	 * @return 如果是多播地址则返回true，否则返回false。
	 */
	bool IsMulticast() const;
	/**
	 * @brief 将octet数组格式ip地址转换为std::string格式。
	 *
	 * @return std::string格式ip地址。
	 */
	virtual std::string GetStringAddress() const;

	/**
	 * @brief 获取uint32_t格式IPv4或IPv6网络地址值。
	 *
	 * @return uint32_t格式IPv4或IPv6网络地址值。
	 */
	uint32_t GetUIAddress() const;

	/**
	 * @brief 模板函数，获取网络地址。
	 * 该函数具体实现根据当前需求返回IPv4地址或IPv6地址
	 * 
	 * @return 网络地址。
	 */
	template<typename T>
	T GetAddress() const {
		return T();
	}

	/**
	 * @brief 验证地址是否都为空。
	 *
	 * @return 空则返回true，否则返回false。
	 */
	bool Empty() const;
};

/**
 * @brief 模板函数特例，获取IPv4网络地址。
 *
 * @return IPv4网络地址。
 */
template<>
inline asio::ip::address_v4 AsioLocator::GetAddress() const
{
	return asio::ip::address_v4({ address[12], address[13], address[14], address[15] });
}
/**
 * @brief 模板函数特例，获取IPv6网络地址。
 *
 * @return IPv6网络地址。
 */
template<>
inline asio::ip::address_v6 AsioLocator::GetAddress() const
{
	asio::ip::address_v6::bytes_type bytes = { 0 };
	std::memcpy(bytes.data(), address, sizeof(address));
	return asio::ip::address_v6(bytes);
}

AGIDDS_NAMESPACE_END

#endif
