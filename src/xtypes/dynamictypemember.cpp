//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.

#include "xtypes/dynamictypemember.h"

TRAVODDS_NAMESPACE_BEGIN

DynamicTypeMember::DynamicTypeMember(const MemberDescriptor &descriptor)
    : descriptor_(descriptor) {
    // Constructor implementation
}

DynamicTypeMember::DynamicTypeMember(const DynamicTypeMember &other)
    : descriptor_(other.descriptor_), annotation_(other.annotation_) {
    // Copy constructor implementation
}

DynamicTypeMember &
DynamicTypeMember::operator=(const DynamicTypeMember &other) {
    if (this != &other) {
        descriptor_ = other.descriptor_;
        annotation_ = other.annotation_;
    }
    return *this;
}

ReturnCode_t
DynamicTypeMember::get_descriptor(MemberDescriptor &descriptor) const {
    descriptor = descriptor_;
    return RETCODE_OK;
}

bool DynamicTypeMember::equals(const DynamicTypeMember &other) const {
    return descriptor_.equals(other.descriptor_);
}

MemberId DynamicTypeMember::get_id() const { return descriptor_.id_; }

std::string DynamicTypeMember::get_name() const { return descriptor_.name_; }

bool DynamicTypeMember::is_key() const {
    for (const auto &annotation : annotation_) {
        if (annotation.value_.find("@key") != annotation.value_.end()) {
            return true;
        }
    }
    return false;
}

TRAVODDS_NAMESPACE_END
