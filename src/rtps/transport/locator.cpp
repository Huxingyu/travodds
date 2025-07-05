#include "rtps/transport/locator.h"

USING_AGIDDS_NAMESPACE;

UDPv4Locator::UDPv4Locator(const Locator_t& loc)
	: Locator_t(loc)
{
	if (kind != LOCATOR_KIND_UDPv4) {
		valid_ = false;
	}
}
uint32_t UDPv4Locator::GetUIAddressV4()
{
	uint32_t uiAddress = *(uint32_t*)(address + 12);
	return uiAddress;
}
asio::ip::address_v4 UDPv4Locator::GetAddressV4()
{
	return asio::ip::address_v4({ address[12], address[13], address[14], address[15] });
}
bool UDPv4Locator::Valid()
{
	return valid_;
}
