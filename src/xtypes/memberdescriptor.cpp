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

#include "xtypes/memberdescriptor.h"
#include "xtypes/dynamictype.h"

AGIDDS_NAMESPACE_BEGIN

MemberDescriptor::MemberDescriptor()
    : id_(0), index_(0), type_(nullptr), default_label_(false) {
    // Constructor implementation
}

MemberDescriptor::MemberDescriptor(const MemberDescriptor &other)
    : id_(other.id_), index_(other.index_), name_(other.name_),
      type_(other.type_), default_label_(other.default_label_),
      default_value_(other.default_value_), label_(other.label_) {
    other.type_->increment_ref_count();
}

MemberDescriptor &MemberDescriptor::operator=(const MemberDescriptor &other) {
    if (this != &other) {
        id_ = other.id_;
        index_ = other.index_;
        name_ = other.name_;
        type_ = other.type_;
        type_->increment_ref_count();
        default_label_ = other.default_label_;
        default_value_ = other.default_value_;
        label_ = other.label_;
    }
    return *this;
}

ReturnCode_t MemberDescriptor::copy_from(const MemberDescriptor &other) {
    if (this != &other) {
        id_ = other.id_;
        index_ = other.index_;
        name_ = other.name_;
        type_ = other.type_;
        type_->increment_ref_count();
        default_label_ = other.default_label_;
        default_value_ = other.default_value_;
        label_ = other.label_;
        return RETCODE_OK;
    }
    return RETCODE_ERROR;
}

bool MemberDescriptor::equals(const MemberDescriptor &other) const {
    return id_ == other.id_ && index_ == other.index_ && name_ == other.name_ &&
           type_ == other.type_ && default_label_ == other.default_label_ &&
           default_value_ == other.default_value_ && label_ == other.label_;
}

bool MemberDescriptor::is_consistent() const {
    // Implement consistency check logic here
    return true;
}

AGIDDS_NAMESPACE_END
