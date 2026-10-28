#ifndef TRAVODDS_RTPS_DISCOVERY_DATASHARINGENDPOINT_H
#define TRAVODDS_RTPS_DISCOVERY_DATASHARINGENDPOINT_H
#include "type/datasharingendpointinfo.h"
#include <cstddef>

TRAVODDS_NAMESPACE_BEGIN
// Canonical big-endian body, independently of the enclosing ParameterList.
// Strict size/count limits precede allocations. Invalid input clears output.
std::vector<uint8_t> EncodeDataSharing(const DataSharingEndpointInfo& info);
bool DecodeDataSharing(const uint8_t* data, size_t size, DataSharingEndpointInfo& info);
DataSharingEndpointInfo LocalDataSharingInfo(const DataSharingQosPolicy& qos, const NativeTypeInfo& type);
TRAVODDS_NAMESPACE_END
#endif
