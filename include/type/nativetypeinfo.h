// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#ifndef TRAVODDS_TYPE_NATIVETYPEINFO_H
#define TRAVODDS_TYPE_NATIVETYPEINFO_H
#include "travodds_dll.h"
#include <cstdint>
#include <string>
TRAVODDS_NAMESPACE_BEGIN

// An explicit same-host layout contract. "plain" also promises that no field
// contains a process-local pointer and destruction requires no user callback.
struct NativeTypeInfo {
	bool bounded = false;
	bool plain = false;
	uint32_t size = 0;
	uint32_t alignment = 0;
	std::string layout_id;
	bool loanable() const {
		return bounded && plain && size && alignment &&
			!(alignment & (alignment - 1)) && !layout_id.empty();
	}
	bool operator==(const NativeTypeInfo& other) const {
		return bounded == other.bounded && plain == other.plain && size == other.size &&
			alignment == other.alignment && layout_id == other.layout_id;
	}
};

// Explicit local key-only snapshot capability. A zero layout does not support
// keyed native DataSharing; a supplied layout must fit the native sample.
struct NativeKeyInfo {
	uint32_t size = 0;
	uint32_t alignment = 0;
	bool provided() const { return size || alignment; }
	bool valid_for(const NativeTypeInfo& type) const {
		return type.loanable() && size && size <= type.size && alignment &&
			!(alignment & (alignment - 1)) && alignment <= type.alignment;
	}
};

TRAVODDS_NAMESPACE_END
#endif
