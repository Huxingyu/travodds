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
#ifndef TRAVODDS_RTPS_TRANSPORT_IPCONTAINER_H
#define TRAVODDS_RTPS_TRANSPORT_IPCONTAINER_H

#include "rtps/basictypertps.h"

TRAVODDS_NAMESPACE_BEGIN

enum IPTYPE
{
    IPV4,
    IPV6,
    IPV4_LOCAL,  // 回环::1 , 127.
    IPV6_LOCAL
};

struct InfoIP_t {
    IPTYPE type;
    std::string name; // IP字符串
    std::string dev;
    octet address[16];
    uint8_t mask;   // 子网掩码长度
	uint32_t scope_id; // 作用域ID，仅IPv6有效

	InfoIP_t() : type(IPV4), address{0}, mask(0), scope_id(0) {
	}
};

struct InfoMAC_t {
    octet address[6];

    bool operator == (const InfoMAC_t& other) {
        return memcmp(address, other.address, 6) == 0;
    }

};

/**
 * IpContainer 类, 本机IP地址信息获取类。
 */
class IpContainer {

public:
    /**
     * @brief 获取本机所有IPv4地址信息。
     *
     * @param [out] ips 返回的存储IPv4地址容器。
     * @param [in] containLoopback 是否包含本地回环地址标志。
     *
     * @return 获取成功返回true，否则返回false。
     */
    static bool GetInfoIPs(std::vector<InfoIP_t>& ips, bool containLoopback = false);

    /**
      * @brief 获取本机所有IPv6地址信息。
      *
      * @param [in] name -。
      *
      * @return 字符串。
      * 
      * @warning 尚不支持。
      */
    static std::string GetIPv6Address(const std::string& name);

    /**
      * @brief 获取本机所有MAC地址信息。
      *
      * @param [out] macs 返回的存储MAC地址容器。
      *
      * @return 获取成功返回true，否则返回false。
      * 
      * @warning 尚不支持。
      */
    static bool GetAllMACAddress(std::vector<InfoMAC_t>* macs);


	static uint32_t GetAdapterScopeId(const octet* address);

private:
    static std::mutex mtx_;
    static std::vector<InfoIP_t> ips_;
};



TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_RTPS_TRANSPORT_IPCONTAINER_H
