// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#ifndef TRAVODDS_QOS_DATASHARINGQOSPOLICY_H
#define TRAVODDS_QOS_DATASHARINGQOSPOLICY_H

#include "travodds_dll.h"
#include <cstdint>
#include <string>
#include <vector>

TRAVODDS_NAMESPACE_BEGIN

enum DataSharingQosPolicyKind {
    AUTO_DATA_SHARING_QOS,
    ON_DATA_SHARING_QOS,
    OFF_DATA_SHARING_QOS
};

// Immutable endpoint configuration. Domains are application sharing groups,
// separate from the DDS domain ID. A common group permits negotiation; it does
// not establish host identity or grant filesystem access.
struct DDS_DLL DataSharingQosPolicy {
    DataSharingQosPolicyKind kind = AUTO_DATA_SHARING_QOS;
    std::string shared_memory_directory = "/dev/shm";
    std::vector<uint32_t> domain_ids{0};
    uint32_t max_samples = 64;
    uint32_t max_readers = 16;
    uint64_t max_segment_bytes = uint64_t(1) << 30;

    bool valid() const;
    bool shares_domain_with(const DataSharingQosPolicy& other) const;
    bool operator==(const DataSharingQosPolicy& other) const;
    bool operator!=(const DataSharingQosPolicy& other) const { return !(*this == other); }
};

TRAVODDS_NAMESPACE_END
#endif
