#include "rtps/transport/locator.h"

USING_TRAVODDS_NAMESPACE;
AsioLocator::AsioLocator(const Locator_t& loc)
	: Locator_t(loc)
{
}

bool AsioLocator::IsMulticast() const
{
	if(kind == LOCATOR_KIND_UDPv4) {
		return GetAddress<asio::ip::address_v4>().is_multicast();
	} else if( kind == LOCATOR_KIND_UDPv6) {
		return GetAddress<asio::ip::address_v6>().is_multicast();
	}
	return false;
}


bool AsioLocator::Empty() const
{
	return address[0] == 0 && address[1] == 0 && address[2] == 0 && address[3] == 0 &&
		address[4] == 0 && address[5] == 0 && address[6] == 0 && address[7] == 0 &&
		address[8] == 0 && address[9] == 0 && address[10] == 0 && address[11] == 0 &&
		address[12] == 0 && address[13] == 0 && address[14] == 0 && address[15] == 0;
}

std::string AsioLocator::GetStringAddress() const
{
	if (kind == LOCATOR_KIND_UDPv4 || kind == LOCATOR_KIND_TCPv4) {
		return GetAddress<asio::ip::address_v4>().to_string();
	}
	else if (kind == LOCATOR_KIND_UDPv6 || kind == LOCATOR_KIND_TCPv6) {
		return GetAddress<asio::ip::address_v6>().to_string();
	}
	return std::string();
}

uint32_t AsioLocator::GetUIAddress() const
{
	if (kind == LOCATOR_KIND_UDPv4 || kind == LOCATOR_KIND_TCPv4) {
		uint32_t uiAddress = *(uint32_t*)(address + 12);
		return uiAddress;
	} else if (kind == LOCATOR_KIND_UDPv6 || kind == LOCATOR_KIND_TCPv6) {
		return GetAddress<asio::ip::address_v6>().scope_id();
	}
	return ~0u;
}
