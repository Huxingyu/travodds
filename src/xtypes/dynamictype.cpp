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

#include "xtypes/dynamictype.h"

TRAVODDS_NAMESPACE_BEGIN

DynamicType::DynamicType(const TypeDescriptor &descriptor)
    : descriptor_(descriptor), ref_count_(0) {

}

DynamicType::~DynamicType() {

}

bool DynamicType::equals(const DynamicType &other) const {
    if (!descriptor_.equals(&other.descriptor_)) {
        return false;
    }
    if (annotation_.size() != other.annotation_.size()) {
        return false;
    }
    for (unsigned int i = 0; i < annotation_.size(); ++i) {
        if (!annotation_[i].equals(other.annotation_[i])) {
            return false;
        }
    }
    if (member_.size() != other.member_.size()) {
        return false;
    }
    for (auto it = member_.begin(); it != member_.end(); ++it) {
        auto it_other = other.member_.find(it->first);
        if (it_other == other.member_.end()) {
            return false;
        }
        if (!it->second.equals((it_other->second))) {
            return false;
        }
    }
    return true;
}

unsigned int DynamicType::get_annotation_count() const {
    return static_cast<unsigned int>(annotation_.size());
}

ReturnCode_t DynamicType::get_annotation(AnnotationDescriptor &descriptor,
                                         unsigned int index) const {
    if (index >= annotation_.size()) {
        return RETCODE_BAD_PARAMETER;
    }
    descriptor = annotation_[index];
    return RETCODE_OK;
}

ReturnCode_t DynamicType::get_descriptor(TypeDescriptor &descriptor) const {
    descriptor = descriptor_;
    return RETCODE_OK;
}

TypeKind DynamicType::get_kind() const { return descriptor_.kind_; }

ReturnCode_t DynamicType::get_member(DynamicTypeMember &member,
                                     MemberId id) const {
    auto it = member_.find(id);
    if (it == member_.end()) {
        return RETCODE_BAD_PARAMETER;
    }
    member = (it->second);
    return RETCODE_OK;
}

ReturnCode_t DynamicType::get_member_by_index(DynamicTypeMember &member,
                                              unsigned int index) const {
    if (index >= member_.size()) {
        return RETCODE_BAD_PARAMETER;
    }
    auto it = member_.begin();
    std::advance(it, index);
    member = (it->second);
    return RETCODE_OK;
}

ReturnCode_t DynamicType::get_member_by_name(DynamicTypeMember &member,
                                             const std::string &name) const {
    auto it = member_by_name_.find(name);
    if (it == member_by_name_.end()) {
        return RETCODE_BAD_PARAMETER;
    }
    member = (it->second);
    return RETCODE_OK;
}

size_t DynamicType::get_member_count() const { return member_.size(); }

std::string DynamicType::get_name() const { return descriptor_.name_; }

void DynamicType::increment_ref_count() { ref_count_++; }

void DynamicType::decrement_ref_count() { ref_count_--; }

int DynamicType::get_ref_count() const { return ref_count_.load(); }

TRAVODDS_NAMESPACE_END
