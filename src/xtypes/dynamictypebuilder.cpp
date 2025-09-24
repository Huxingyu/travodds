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

#include "xtypes/dynamictypebuilder.h"
#include "xtypes/dynamictype.h"
#include "xtypes/dynamictypemember.h"
#include "xtypes/typedescriptor.h"

TRAVODDS_NAMESPACE_BEGIN

DynamicTypeBuilder::DynamicTypeBuilder(const TypeDescriptor &descriptor)
    : descriptor_(descriptor) {}

DynamicTypeBuilder::~DynamicTypeBuilder() {}

ReturnCode_t
DynamicTypeBuilder::apply_annotation(const AnnotationDescriptor &descriptor) {
    annotation_.push_back(descriptor);
    return RETCODE_OK;
}

ReturnCode_t
DynamicTypeBuilder::get_descriptor(TypeDescriptor &descriptor) const {
    descriptor = descriptor_;
    return RETCODE_OK;
}

ReturnCode_t
DynamicTypeBuilder::add_member(const MemberDescriptor &descriptor) {
    DynamicTypeMember member(descriptor);
    member_[descriptor.id_] = member;
    member_by_name_[descriptor.name_] = member;
    return RETCODE_OK;
}

ReturnCode_t DynamicTypeBuilder::apply_annotation_to_member(
    MemberId member_id, const AnnotationDescriptor &descriptor) {
    auto it = member_.find(member_id);
    if (it == member_.end()) {
        return RETCODE_BAD_PARAMETER;
    }
    it->second.annotation_.push_back(descriptor);
    return RETCODE_OK;
}

unsigned int DynamicTypeBuilder::get_annotation_count() const {
    return annotation_.size();
}

ReturnCode_t
DynamicTypeBuilder::get_annotation(AnnotationDescriptor &descriptor,
                                   unsigned int index) const {
    if (index >= annotation_.size()) {
        return RETCODE_BAD_PARAMETER;
    }
    descriptor = annotation_[index];
    return RETCODE_OK;
}

TypeKind DynamicTypeBuilder::get_kind() const { return descriptor_.kind_; }

ReturnCode_t DynamicTypeBuilder::get_member(DynamicTypeMember &member,
                                            MemberId id) const {
    auto it = member_.find(id);
    if (it == member_.end()) {
        return RETCODE_BAD_PARAMETER;
    }
    member = (it->second);
    return RETCODE_OK;
}

ReturnCode_t DynamicTypeBuilder::get_member_by_index(DynamicTypeMember &member,
                                                     unsigned int index) const {
    if (index >= member_.size()) {
        return RETCODE_BAD_PARAMETER;
    }
    auto it = std::next(member_.begin(), index);
    member = (it->second);
    return RETCODE_OK;
}

ReturnCode_t
DynamicTypeBuilder::get_member_by_name(DynamicTypeMember &member,
                                       const std::string &name) const {
    auto it = member_by_name_.find(name);
    if (it == member_by_name_.end()) {
        return RETCODE_BAD_PARAMETER;
    }
    member = (it->second);
    return RETCODE_OK;
}

unsigned int DynamicTypeBuilder::get_member_count() const {
    return member_.size();
}

std::string DynamicTypeBuilder::get_name() const { return descriptor_.name_; }

DynamicType *DynamicTypeBuilder::build() const {
    DynamicType *type = new DynamicType(descriptor_);
    type->annotation_ = annotation_;
    type->member_ = member_;
    type->member_by_name_ = member_by_name_;
    type->increment_ref_count();
    return type;
}

bool DynamicTypeBuilder::equals(const DynamicTypeBuilder &other) const {
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

TRAVODDS_NAMESPACE_END
