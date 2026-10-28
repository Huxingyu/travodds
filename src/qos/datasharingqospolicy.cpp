#include "qos/datasharingqospolicy.h"
#include <algorithm>

TRAVODDS_NAMESPACE_BEGIN

bool DataSharingQosPolicy::valid() const {
    if (kind != AUTO_DATA_SHARING_QOS && kind != ON_DATA_SHARING_QOS &&
        kind != OFF_DATA_SHARING_QOS) return false;
    if (shared_memory_directory.empty() || shared_memory_directory.front() != '/' ||
        shared_memory_directory.size() > 4096 ||
        shared_memory_directory.find('\0') != std::string::npos ||
        !max_samples || max_samples > 1048576 || !max_readers || max_readers > 64 ||
        !max_segment_bytes || domain_ids.empty() || domain_ids.size() > 32) return false;
    for (auto i = domain_ids.begin(); i != domain_ids.end(); ++i)
        if (std::find(domain_ids.begin(), i, *i) != i) return false;
    return true;
}

bool DataSharingQosPolicy::shares_domain_with(const DataSharingQosPolicy& other) const {
    if (!valid() || !other.valid() || kind == OFF_DATA_SHARING_QOS ||
        other.kind == OFF_DATA_SHARING_QOS) return false;
    for (auto id : domain_ids)
        if (std::find(other.domain_ids.begin(), other.domain_ids.end(), id) != other.domain_ids.end()) return true;
    return false;
}

bool DataSharingQosPolicy::operator==(const DataSharingQosPolicy& other) const {
    return kind == other.kind && shared_memory_directory == other.shared_memory_directory &&
        domain_ids == other.domain_ids && max_samples == other.max_samples &&
        max_readers == other.max_readers && max_segment_bytes == other.max_segment_bytes;
}

TRAVODDS_NAMESPACE_END
