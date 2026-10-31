// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#ifndef TRAVODDS_TYPE_DATASHARINGENDPOINTINFO_H
#define TRAVODDS_TYPE_DATASHARINGENDPOINTINFO_H
#include "type/nativetypeinfo.h"
#include "qos/datasharingqospolicy.h"

TRAVODDS_NAMESPACE_BEGIN

// Discovery extension. Missing/unknown versions have version == 0 and must use
// RTPS. This is a candidate capability, not proof of a mapped/registered Reader.
// No peer-controlled filesystem path is advertised or opened.
struct DDS_DLL DataSharingEndpointInfo {
    // Version 5 requires the layout v7 publication-history ring, key-only
    // snapshots and construction without payload initialization. Older peers
    // retain RTPS: v1 used a data-plane metadata mutex; v2 could retain a full
    // sample as a key; v3 initialized payload; v4 had no publication ring.
    static constexpr uint32_t CURRENT_VERSION = 5;
    uint32_t version = 0;
    DataSharingQosPolicyKind kind = OFF_DATA_SHARING_QOS;
    NativeTypeInfo type;
    std::vector<uint32_t> domain_ids;
    std::string host_id;
    uint64_t pid_namespace_device = 0, pid_namespace_inode = 0;
    uint64_t session_high = 0, session_low = 0;
    uint64_t segment_bytes = 0;
    uint32_t slots = 0, max_readers = 0;

    bool valid(bool writer) const;
    bool compatible_with(const DataSharingEndpointInfo& reader) const;
};

TRAVODDS_NAMESPACE_END
#endif
