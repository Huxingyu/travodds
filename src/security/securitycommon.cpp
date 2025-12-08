#include "security/securitycommon.h"
#ifdef DDSSECURITY
TRAVODDS_NAMESPACE_BEGIN
const DDSSecHandle SEC_NIL_HANDLE = (void*)0;

std::string GMCLASSID_SECURITY_AUTH_HANDSHAKE = "dds.sec.auth";
std::string GMCLASSID_SECURITY_PARTICIPANT_CRYPTO_TOKENS = "dds.sec.participant_crypto_tokens";
std::string GMCLASSID_SECURITY_DATAWRITER_CRYPTO_TOKENS = "dds.sec.datawriter_crypto_tokens";
std::string GMCLASSID_SECURITY_DATAREADER_CRYPTO_TOKENS = "dds.sec.datareader_crypto_tokens";

const ParticipantSecurityInfo PARTICIPANT_SECURITY_ATTRIBUTES_INFO_DEFAULT = { 0UL, 0UL };
const EndpointSecurityInfo ENDPOINT_SECURITY_ATTRIBUTES_INFO_DEFAULT = { 0UL, 0UL };

ParticipantSecurityAttributesMask ParticipantSecurityAttributesToMask(ParticipantSecurityAttributes* secAttr)
{
	ParticipantSecurityAttributesMask rst = PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;
	if (secAttr->is_rtps_protected)
	{
		rst |= PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_RTPS_PROTECTED;
	}
	if (secAttr->is_discovery_protected)
	{
		rst |= PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED;
	}
	if (secAttr->is_liveliness_protected)
	{
		rst |= PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED;
	}
	// todo: add more attributes
	if (rst == PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID)
	{
		rst = 0;
	}
	return rst;
}

EndpointSecurityAttributesMask EndpointSecurityAttributesToMask(EndpointSecurityAttributes* secAttr)
{
	EndpointSecurityAttributesMask rst = ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID;
	if (secAttr->is_read_protected)
	{
		rst |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_READ_PROTECTED;
	}
	if (secAttr->is_write_protected)
	{
		rst |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_WRITE_PROTECTED;
	}
	if (secAttr->is_discovery_protected)
	{
		rst |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_DISCOVERY_PROTECTED;
	}
	if (secAttr->is_liveliness_protected)
	{
		rst |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_LIVELINESS_PROTECTED;
	}
	if (secAttr->is_submessage_protected)
	{
		rst |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_PROTECTED;
	}
	if (secAttr->is_payload_protected)
	{
		rst |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_PROTECTED;
	}
	if (secAttr->is_key_protected)
	{
		rst |= ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_KEY_PROTECTED;
	}
	// todo: add more attributes
	if (rst == ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID)
	{
		rst = 0;
	}
	return rst;
}

TRAVODDS_NAMESPACE_END
#endif // DDSSECURITY
