#include "security/securitycommon.h"
#ifdef DDSSECURITY
#include "rtps/basictypertps.h"
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
	if (rst == static_cast<ParticipantSecurityAttributesMask>(PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID))
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
	if (rst == static_cast<EndpointSecurityAttributesMask>(ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID))
	{
		rst = 0;
	}
	return rst;
}

int compareBuiltinTopicKey_impl(BuiltinTopicKey_t key1, BuiltinTopicKey_t key2)
{
	GUID_t g1 = Conversion::ToGuid(key1);
	GUID_t g2 = Conversion::ToGuid(key2);
	if (g1 == g2) {
		return 0;
	}
	else if(g1 > g2) {
		return 1;
	}
	else {
		return -1;
	}
}

TRAVODDS_NAMESPACE_END
#endif // DDSSECURITY
