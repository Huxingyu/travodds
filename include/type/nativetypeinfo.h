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

TRAVODDS_NAMESPACE_END
#endif
