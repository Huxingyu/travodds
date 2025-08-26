#include "rtps/transport/ipcontainer.h"

#if defined(_WIN32)
#pragma comment(lib, "Iphlpapi.lib")
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include "common/log/logger.h"

USING_TRAVODDS_NAMESPACE

bool IpContainer::GetInfoIPs(std::vector<InfoIP_t>& ips, bool containLoopback)
{
#if defined(_WIN32)
    constexpr DWORD IfOperStatusUp = 1;
    DWORD ret, size = 0;
    // 取长度
    GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &size);

    std::unique_ptr<IP_ADAPTER_ADDRESSES, decltype(&free)> adapters(
        static_cast<PIP_ADAPTER_ADDRESSES>(malloc(size)), &free);

    // 获取适配器信息
    ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX,
        nullptr, adapters.get(), &size);

    if (ERROR_SUCCESS != ret) {
		TRAVODDS_LOG(LOG_ERROR, "GetAdaptersAddresses Err[%d]", GetLastError());
        return false;
    }

    // 遍历适配器
    for (auto aa = adapters.get(); aa; aa = aa->Next) {
        if (aa->OperStatus != IfOperStatusUp) {
            continue;
        }
        if (aa->Flags & IP_ADAPTER_NO_MULTICAST) {
            continue;
        }
        // 遍历单播地址
        for (auto ua = aa->FirstUnicastAddress; nullptr != ua; ua = ua->Next) {
            // 检查地址有效性
            if (!ua || !ua->Address.lpSockaddr) {
                continue;
            }

            // 获取地址族类型
            int32_t family = ua->Address.lpSockaddr->sa_family;
            if (family != AF_INET && family != AF_INET6) {
               continue; // 只处理IPv4和IPv6
            }

            // 转换地址为字符串形式
            char ipStr[INET6_ADDRSTRLEN] = { 0 };
            DWORD ipStrLength = sizeof(ipStr);
            InfoIP_t info;
            info.dev = std::string(aa->AdapterName);

            if (family == AF_INET) {
                info.type = IPV4;
                sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
                memcpy(info.address + 12, &(sin->sin_addr), sizeof(sin->sin_addr));
                inet_ntop(AF_INET, &(sin->sin_addr), ipStr, ipStrLength);

                if (info.address[12] == 127
                    && info.address[13] == 0
                    && info.address[14] == 0
                    && info.address[15] == 1) {
                    info.type = IPV4_LOCAL;
                }

            } else { // AF_INET6
                info.type = IPV6;
                sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(ua->Address.lpSockaddr);
                memcpy(info.address, &(sin6->sin6_addr), sizeof(sin6->sin6_addr));
                inet_ntop(AF_INET6, &(sin6->sin6_addr), ipStr, ipStrLength);

                if (memcmp(info.address, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1", 16) == 0) {
                    info.type = IPV6_LOCAL;
                }
            }

            info.name = std::string(ipStr);

            // 跳过,异常info，或不需要的回环地址
            if (!containLoopback && (info.type == IPV6_LOCAL || info.type == IPV4_LOCAL)) {
                continue;
            }

            // 获取网络前缀长度(子网掩码)
            if (ua->Length <= offsetof(IP_ADAPTER_UNICAST_ADDRESS_LH, OnLinkPrefixLength)) {
                info.mask = (family == AF_INET) ? 32 : 128; // 默认全掩码
            } else {
                info.mask = ua->OnLinkPrefixLength;
            }

            ips.push_back(info);
        }
    }
#else
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
		AGILOG(AGILOG_ERROR, "getifaddrs Err[%d]", errno);
        return false;
    }

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || (ifa->ifa_flags & IFF_RUNNING) == 0) {
            continue;
        }

        int family = ifa->ifa_addr->sa_family;
        if (family != AF_INET && family != AF_INET6) {
            continue;
        }

        InfoIP_t info;
        info.dev = std::string(ifa->ifa_name);
        char ipStr[INET6_ADDRSTRLEN] = { 0 };

        if (family == AF_INET) {
            info.type = IPV4;
            sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
            inet_ntop(AF_INET, &(sin->sin_addr), ipStr, sizeof(ipStr));
            memcpy(info.address + 12, &(sin->sin_addr), sizeof(sin->sin_addr));
            if (info.address[12] == 127) {
                info.type = IPV4_LOCAL;
            }
        }
        else {
            info.type = IPV6;
            sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(ifa->ifa_addr);
            inet_ntop(AF_INET6, &(sin6->sin6_addr), ipStr, sizeof(ipStr));
            memcpy(info.address, &(sin6->sin6_addr), sizeof(sin6->sin6_addr));
            if (memcmp(info.address, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1", 16) == 0) {
                info.type = IPV6_LOCAL;
            }
        }

        info.name = std::string(ipStr);
        if (!containLoopback && (info.type == IPV4_LOCAL || info.type == IPV6_LOCAL)) {
            continue;
        }

        // 获取子网掩码长度
        if (ifa->ifa_netmask) {
            if (family == AF_INET) {
                struct sockaddr_in* netmask = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_netmask);
                info.mask = __builtin_popcount(netmask->sin_addr.s_addr);
            } else if (family == AF_INET6) {
                struct sockaddr_in6* netmask = reinterpret_cast<struct sockaddr_in6*>(ifa->ifa_netmask);
                uint8_t* maskBytes = reinterpret_cast<uint8_t*>(&(netmask->sin6_addr));
                info.mask = 0;
                for (int i = 0; i < 16; ++i) {
                    info.mask += __builtin_popcount(maskBytes[i]);
                }
            }
        } else {
            info.mask = (family == AF_INET) ? 32 : 128; // 默认全掩码
        }

        ips.push_back(info);
    }

    freeifaddrs(ifaddr);
#endif
    return true;
}
