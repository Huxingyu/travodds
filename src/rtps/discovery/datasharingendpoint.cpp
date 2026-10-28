#include "rtps/discovery/datasharingendpoint.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#if defined(__linux__)
#include <sys/stat.h>
#endif

TRAVODDS_NAMESPACE_BEGIN
namespace {
bool hostIdValid(const std::string& id) {
    if (id.size() != 36) return false;
    for (size_t i = 0; i < id.size(); ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) { if (id[i] != '-') return false; }
        else if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f'))) return false;
    }
    return true;
}
struct Encoder {
    std::vector<uint8_t> bytes;
    void integer(uint64_t n, unsigned width) {
        for (unsigned i = width; i; --i) bytes.push_back(uint8_t(n >> (8 * (i - 1))));
    }
    void string(const std::string& s) {
        integer(s.size(), 2); bytes.insert(bytes.end(), s.begin(), s.end());
    }
};
struct Decoder {
    const uint8_t* bytes; size_t size, offset = 0;
    uint64_t integer(unsigned width) {
        if (width > size - offset) throw std::invalid_argument("Truncated DataSharing capability");
        uint64_t n = 0;
        for (unsigned i = 0; i < width; ++i) n = (n << 8) | bytes[offset++];
        return n;
    }
    std::string string(size_t limit) {
        const auto n = integer(2);
        if (n > limit || n > size - offset) throw std::invalid_argument("Invalid DataSharing string length");
        std::string s(reinterpret_cast<const char*>(bytes + offset), size_t(n));
        offset += size_t(n); return s;
    }
};
}

bool DataSharingEndpointInfo::valid(bool writer) const {
    if (version != 1 || (kind != AUTO_DATA_SHARING_QOS && kind != ON_DATA_SHARING_QOS) ||
        !type.loanable() || type.alignment > 4096 || type.layout_id.size() > 256 ||
        type.layout_id.find('\0') != std::string::npos || !hostIdValid(host_id) ||
        !pid_namespace_inode || domain_ids.empty() || domain_ids.size() > 32) return false;
    for (auto i = domain_ids.begin(); i != domain_ids.end(); ++i)
        if (std::find(domain_ids.begin(), i, *i) != i) return false;
    if (!writer) return !session_high && !session_low && !segment_bytes && !slots && !max_readers;
    return (session_high || session_low) && segment_bytes && slots && slots <= 1048576 &&
        max_readers && max_readers <= 64;
}

bool DataSharingEndpointInfo::compatible_with(const DataSharingEndpointInfo& reader) const {
    if (!valid(true) || !reader.valid(false) || !(type == reader.type) || host_id != reader.host_id ||
        pid_namespace_device != reader.pid_namespace_device || pid_namespace_inode != reader.pid_namespace_inode) return false;
    for (auto id : domain_ids)
        if (std::find(reader.domain_ids.begin(), reader.domain_ids.end(), id) != reader.domain_ids.end()) return true;
    return false;
}

std::vector<uint8_t> EncodeDataSharing(const DataSharingEndpointInfo& info) {
    if (!info.valid(info.segment_bytes != 0)) return {};
    Encoder e;
    e.integer(0x54524453, 4); // TRDS
    e.integer(info.version, 4); e.integer(info.kind, 4);
    e.integer(info.type.size, 4); e.integer(info.type.alignment, 4);
    e.string(info.type.layout_id); e.string(info.host_id);
    e.integer(info.pid_namespace_device, 8); e.integer(info.pid_namespace_inode, 8);
    e.integer(info.session_high, 8); e.integer(info.session_low, 8);
    e.integer(info.segment_bytes, 8); e.integer(info.slots, 4); e.integer(info.max_readers, 4);
    e.integer(info.domain_ids.size(), 4);
    for (auto id : info.domain_ids) e.integer(id, 4);
    while (e.bytes.size() % 4) e.bytes.push_back(0);
    return e.bytes;
}

bool DecodeDataSharing(const uint8_t* data, size_t size, DataSharingEndpointInfo& info) {
    info = {};
    if (!data || size < 4 || size > 1024 || size % 4) return false;
    try {
        Decoder d{data, size};
        DataSharingEndpointInfo parsed;
        if (d.integer(4) != 0x54524453 || d.integer(4) != 1) return false;
        parsed.version = 1;
        const auto kind = d.integer(4);
        if (kind > ON_DATA_SHARING_QOS) return false;
        parsed.kind = static_cast<DataSharingQosPolicyKind>(kind);
        parsed.type.bounded = parsed.type.plain = true;
        parsed.type.size = uint32_t(d.integer(4)); parsed.type.alignment = uint32_t(d.integer(4));
        parsed.type.layout_id = d.string(256); parsed.host_id = d.string(36);
        parsed.pid_namespace_device = d.integer(8); parsed.pid_namespace_inode = d.integer(8);
        parsed.session_high = d.integer(8); parsed.session_low = d.integer(8);
        parsed.segment_bytes = d.integer(8);
        parsed.slots = uint32_t(d.integer(4)); parsed.max_readers = uint32_t(d.integer(4));
        const auto count = d.integer(4);
        if (!count || count > 32) return false;
        for (uint64_t i = 0; i < count; ++i) parsed.domain_ids.push_back(uint32_t(d.integer(4)));
        if (size - d.offset > 3) return false;
        while (d.offset < size) if (data[d.offset++] != 0) return false;
        if (!parsed.valid(parsed.segment_bytes != 0)) return false;
        info = std::move(parsed); return true;
    } catch (const std::invalid_argument&) { return false; }
}

DataSharingEndpointInfo LocalDataSharingInfo(const DataSharingQosPolicy& qos, const NativeTypeInfo& type) {
    DataSharingEndpointInfo info;
#if defined(__linux__)
    if (!qos.valid() || qos.kind == OFF_DATA_SHARING_QOS || !type.loanable()) return info;
    std::ifstream boot("/proc/sys/kernel/random/boot_id");
    struct stat identity{};
    if (!std::getline(boot, info.host_id) || stat("/proc/self/ns/pid", &identity) != 0) return {};
    info.version = 1; info.kind = qos.kind; info.type = type; info.domain_ids = qos.domain_ids;
    info.pid_namespace_device = uint64_t(identity.st_dev); info.pid_namespace_inode = uint64_t(identity.st_ino);
    if (!info.valid(false)) return {};
#else
    (void)qos; (void)type;
#endif
    return info;
}
TRAVODDS_NAMESPACE_END
