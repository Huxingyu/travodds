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

class IpContainer {

public:
    static bool GetInfoIPs(std::vector<InfoIP_t>& ips, bool containLoopback = false);

    static std::string GetIPv6Address(const std::string& name);

    static bool GetAllMACAddress(std::vector<InfoMAC_t>* macs);
private:


};



AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_TRANSPORT_IPCONTAINER_H
