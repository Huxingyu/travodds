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

#include "xtypes/typedescriptor.h"
#include "xtypes/dynamictype.h"

AGIDDS_NAMESPACE_BEGIN

TypeDescriptor::TypeDescriptor()
    : kind_(TK_NONE), name_(""), base_type_(nullptr),
      discriminator_type_(nullptr), element_type_(nullptr),
      key_element_type_(nullptr) {
    // 构造函数实现
}

TypeDescriptor::TypeDescriptor(const TypeDescriptor &other) {
    copy_from(&other);
}

TypeDescriptor &TypeDescriptor::operator=(const TypeDescriptor &other) {
    copy_from(&other);
    return *this;
}

ReturnCode_t TypeDescriptor::copy_from(const TypeDescriptor *other) {
    if (other == nullptr) {
        return RETCODE_BAD_PARAMETER;
    }

    kind_ = other->kind_;
    name_ = other->name_;
    base_type_ = other->base_type_;
    discriminator_type_ = other->discriminator_type_;
    bound_ = other->bound_;
    element_type_ = other->element_type_;
    key_element_type_ = other->key_element_type_;

    return RETCODE_OK;
}

bool TypeDescriptor::equals(const TypeDescriptor *other) const {
    if (other == nullptr) {
        return false;
    }

    if (kind_ != other->kind_) {
        return false;
    }

    if (name_ != other->name_) {
        return false;
    }

    if (bound_ != other->bound_) {
        return false;
    }

    if (base_type_ != nullptr && !base_type_->equals(*other->base_type_)) {
        return false;
    }

    if (discriminator_type_ != nullptr &&
        !discriminator_type_->equals(*other->discriminator_type_)) {
        return false;
    }

    if (element_type_ != nullptr &&
        !element_type_->equals(*other->element_type_)) {
        return false;
    }

    if (key_element_type_ != nullptr &&
        !key_element_type_->equals(*other->key_element_type_)) {
        return false;
    }

    return true;
}

bool TypeDescriptor::is_consistent() const {
    // 判断类型描述符是否一致
    return true;
}

unsigned int TypeDescriptor::get_bound_count() const {
    unsigned int total_bounds = 1;
    for (const auto &bound : bound_) {
        total_bounds *= bound;
    }
    return total_bounds;
}

AGIDDS_NAMESPACE_END
