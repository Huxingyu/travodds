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

std::vector<InfoIP_t> IpContainer::ips_;
std::mutex IpContainer::mtx_;
bool IpContainer::GetInfoIPs(std::vector<InfoIP_t>& ips, bool containLoopback)
{
	std::lock_guard<std::mutex> lock(mtx_);
    if (!ips_.empty()) {
		for (const auto& ip : ips_) {
            if (containLoopback && (ip.type == IPV4_LOCAL || ip.type == IPV6_LOCAL)) {
                continue;
            }
            ips.push_back(ip);
        }
		return true;
     }
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
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, aa->FriendlyName, -1, NULL, 0, NULL, NULL);
            if (size_needed > 0) {
                info.dev.resize(size_needed, 0);
                WideCharToMultiByte(CP_UTF8, 0, aa->FriendlyName, -1, &info.dev[0], size_needed, NULL, NULL);

            }

            // 移除末尾的 null 终止符
            if (!info.dev.empty() && info.dev.back() == '\0') {
                info.dev.pop_back();
            }

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
				info.scope_id = sin6->sin6_scope_id;
            }

            info.name = std::string(ipStr);

            // 获取网络前缀长度(子网掩码)
            if (ua->Length <= offsetof(IP_ADAPTER_UNICAST_ADDRESS_LH, OnLinkPrefixLength)) {
                info.mask = (family == AF_INET) ? 32 : 128; // 默认全掩码
            } else {
                info.mask = ua->OnLinkPrefixLength;
            }

			ips_.push_back(info);
            if (!containLoopback && (info.type == IPV6_LOCAL || info.type == IPV4_LOCAL)) {
                continue;
            }

            ips.push_back(info);
        }
    }
#else
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
		TRAVODDS_LOG(LOG_ERROR, "getifaddrs Err[%d]", errno);
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
            info.scope_id = sin6->sin6_scope_id;
        }

        info.name = std::string(ipStr);

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

        ips_.push_back(info);
        if (!containLoopback && (info.type == IPV4_LOCAL || info.type == IPV6_LOCAL)) {
            continue;
        }

        ips.push_back(info);
    }

    freeifaddrs(ifaddr);
#endif
    return true;
}

uint32_t IpContainer::GetAdapterScopeId(const octet* address)
{
    for (auto& ip : ips_) {
        if (ip.type == IPV6 || ip.type == IPV6_LOCAL) {
            uint8_t prefix_len = ip.mask;

            // 计算需要比较的字节数和位掩码
            int bytes_to_compare = prefix_len / 8;
            int bits_to_compare = prefix_len % 8;

            // 比较完整的字节
            if (bytes_to_compare > 0 &&
                memcmp(ip.address, address, bytes_to_compare) != 0) {
                continue;
            }

            // 比较剩余的位
            if (bits_to_compare > 0) {
                uint8_t mask = 0xFF << (8 - bits_to_compare);
                if ((ip.address[bytes_to_compare] & mask) !=
                    (address[bytes_to_compare] & mask)) {
                    continue;
                }
            }

            // 找到匹配的网络
            return ip.scope_id;
        }
    }

    return 0;
}