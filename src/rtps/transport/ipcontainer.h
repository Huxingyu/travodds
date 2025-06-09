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
#ifndef AGIDDS_RTPS_TRANSPORT_IPCONTAINER_H
#define AGIDDS_RTPS_TRANSPORT_IPCONTAINER_H

#include "rtps/basictypertps.h"

AGIDDS_NAMESPACE_BEGIN

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

	InfoIP_t()
	{
		memset(address, 0, 16);
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
private:


};



AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_TRANSPORT_IPCONTAINER_H
