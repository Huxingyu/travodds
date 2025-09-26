#ifndef _ATTRIBUTES__ENDPOINTSECURITYATTRIBUTES_H__
#define _ATTRIBUTES__ENDPOINTSECURITYATTRIBUTES_H__


#include <cstdint>

TRAVODDS_NAMESPACE_BEGIN
typedef uint32_t PluginEndpointSecurityAttributesMask;

#define PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED            (0x00000001UL << 0)
#define PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_ENCRYPTED               (0x00000001UL << 1)
#define PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED (0x00000001UL << 2)
#define PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID                           (0x00000001UL << 31)

#define PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_MASK_DEFAULT PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID

struct PluginEndpointSecurityAttributes
{
    PluginEndpointSecurityAttributes()
        : is_submessage_encrypted(false)
        , is_submessage_origin_authenticated(false)
        , is_payload_encrypted(false)
    {
    }

    explicit PluginEndpointSecurityAttributes(
            const PluginEndpointSecurityAttributesMask mask)
        : is_submessage_encrypted((mask & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ENCRYPTED) != 0)
        , is_submessage_origin_authenticated((mask &
                PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_ORIGIN_AUTHENTICATED) != 0)
        , is_payload_encrypted((mask & PLUGIN_ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_ENCRYPTED) != 0)
    {
    }

    bool is_submessage_encrypted;
    bool is_submessage_origin_authenticated;
    bool is_payload_encrypted;

    PluginEndpointSecurityAttributesMask mask() const;

};
TRAVODDS_NAMESPACE_END
#endif // _ATTRIBUTES__ENDPOINTSECURITYATTRIBUTES_H__