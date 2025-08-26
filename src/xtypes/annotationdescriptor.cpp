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

#include "xtypes/annotationdescriptor.h"
#include "xtypes/dynamictype.h"

TRAVODDS_NAMESPACE_BEGIN

AnnotationDescriptor::AnnotationDescriptor(const AnnotationDescriptor &other)
    : type_(other.type_), value_(other.value_) {
    type_->increment_ref_count();
}

AnnotationDescriptor::~AnnotationDescriptor() {
    if (type_ != nullptr) {
        type_->decrement_ref_count();
    }
}

AnnotationDescriptor &
AnnotationDescriptor::operator=(const AnnotationDescriptor &other) {
    if (this != &other) {
        type_ = other.type_;
        type_->increment_ref_count();
        value_ = other.value_;
    }
    return *this;
}

int AnnotationDescriptor::copy_from(const AnnotationDescriptor &other) {
    if (this == &other) {
        return RETCODE_BAD_PARAMETER;
    }
    type_ = other.type_;
    type_->increment_ref_count();
    value_ = other.value_;
    return RETCODE_OK;
}

bool AnnotationDescriptor::equals(const AnnotationDescriptor &other) const {
    if (type_ != nullptr && type_->equals(*other.type_)) {
        return false;
    }
    return (value_ == other.value_);
}

bool AnnotationDescriptor::is_consistent() const { return type_ != nullptr; }

TRAVODDS_NAMESPACE_END
