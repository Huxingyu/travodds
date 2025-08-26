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

#include "xtypes/dynamicdata.h"
#include "common/log/logger.h"
#include <sstream>

TRAVODDS_NAMESPACE_BEGIN

DynamicData::DynamicData(DynamicType &type) : type_(type) {
    // 增加类型的引用计数
    type.increment_ref_count();
    // 根据不同的类型初始化值
    ReturnCode_t ret = initialize_values();
    if (ret != RETCODE_OK) {
        throw std::runtime_error("Failed to initialize values");
    }
}

DynamicData::DynamicData(const DynamicData &other) : type_(other.type_) {
    // 增加类型的引用计数
    type_.increment_ref_count();
    // 复制值
    value_ = other.value_;
}

DynamicData& DynamicData::operator=(const DynamicData &other) {
    if (this != &other) {
        // 清除当前值
        clear_all_values();
        // 增加类型的引用计数
        type_.increment_ref_count();
        // 复制值
        value_ = other.value_;
    }
    return *this;
}

ReturnCode_t DynamicData::initialize_values() {
    ReturnCode_t ret = RETCODE_OK;
    switch (type_.get_kind()) {
    case TK_STRUCTURE:
    case TK_UNION: {
        for (const auto &member : type_.member_) {
            TypeValue &value = value_[member.first];
            MemberDescriptor descriptor;
            ret = member.second.get_descriptor(descriptor);
            if (ret != RETCODE_OK) {
                return ret;
            }
            value.complex_value = new DynamicData(*descriptor.type_);
        }
        break;
    }
    case TK_SEQUENCE: 
    case TK_ARRAY: {
        TypeDescriptor descriptor;
        ret = type_.get_descriptor(descriptor);
        if (ret != RETCODE_OK) {
            return ret;
        }
        size_t size = 1;
        for (const auto &bound : descriptor.bound_) {
            size *= bound;
        }
        size = type_.get_kind() == TK_SEQUENCE ? 0 : size;
        switch (descriptor.element_type_->descriptor_.kind_) {
        case TK_BOOLEAN: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.boolean_values) BooleanSeq(size);
            break;
        }
        case TK_BYTE: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.byte_values) ByteSeq(size);
            break;
        }
        case TK_INT16: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.int16_values) Int16Seq(size);
            break;
        }
        case TK_ENUM:
        case TK_INT32: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.int32_values) Int32Seq(size);
            break;
        }
        case TK_INT64: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.int64_values) Int64Seq(size);
            break;
        }
        case TK_UINT16: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.uint16_values) UInt16Seq(size);
            break;
        }
        case TK_UINT32: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.uint32_values) UInt32Seq(size);
            break;
        }
        case TK_UINT64: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.uint64_values) UInt64Seq(size);
            break;
        }
        case TK_FLOAT32: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.float32_values) Float32Seq(size);
            break;
        }
        case TK_FLOAT64: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.float64_values) Float64Seq(size);
            break;
        }
        case TK_FLOAT128: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.float128_values) Float128Seq(size);
            break;
        }
        case TK_CHAR8: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.char8_values) CharSeq(size);
            break;
        }
        case TK_CHAR16: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.char16_values) WcharSeq(size);
            break;
        }
        case TK_STRING8: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.string_values) StringSeq(size);
            break;
        }
        case TK_STRING16: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.wstring_values) WstringSeq(size);
            break;
        }
        case TK_ALIAS: {
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.complex_values) DynamicDataSeq(size);
            for (size_t i = 0; i < size; ++i) {
                value.complex_values[i] = new DynamicData(*descriptor.element_type_);
            }
            break;
        }
        case TK_BITMASK: {
            TRAVODDS_LOG(LOG_ERROR, "Bitmask type not supported in sequence initialization.\n");
            break;
        }
        case TK_BITSET: {
            TRAVODDS_LOG(LOG_ERROR, "Bitset type not supported in sequence initialization.\n");
            break;
        }
        case TK_STRUCTURE:
        case TK_UNION: {    
            TypeValue &value = value_[MEMBER_ID_INVALID];
            new (&value.complex_values) DynamicDataSeq(size);
            for (size_t i = 0; i < size; ++i) {
                value.complex_values[i] = new DynamicData(*descriptor.element_type_);
            }
            break;
        }
        default:
            break;
        }
        break;
    }
    case TK_BOOLEAN: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.boolean_value = false;
        break;
    }
    case TK_BYTE: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.byte_value = 0;
        break;
    }
    case TK_INT16: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.int16_value = 0;
        break;
    }
    case TK_INT32: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.int32_value = 0;
        break;
    }
    case TK_INT64: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.int64_value = 0;
        break;
    }
    case TK_UINT16: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.uint16_value = 0;
        break;
    }
    case TK_UINT32: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.uint32_value = 0;
        break;
    }
    case TK_UINT64: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.uint64_value = 0;
        break;
    }
    case TK_FLOAT32: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.float32_value = 0.0f;
        break;
    }
    case TK_FLOAT64: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.float64_value = 0.0;
        break;
    }
    case TK_FLOAT128: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.float128_value = 0.0;
        break;
    }
    case TK_CHAR8: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.charvalue = '\0';
        break;
    }
    case TK_CHAR16: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.wchar_value = L'\0';
        break;
    }
    case TK_STRING8: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.string_value = "";
        break;
    }
    case TK_STRING16: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.wstring_value = L"";
        break;
    }
    case TK_BITMASK: {
        TRAVODDS_LOG(LOG_ERROR, "Bitmask type not supported in value initialization.\n");
        break;
    }
    case TK_BITSET: {
        TRAVODDS_LOG(LOG_ERROR, "Bitset type not supported in value initialization.\n");
        break;
    }
    case TK_ANNOTATION: {
        TRAVODDS_LOG(LOG_ERROR, "Annotation type not supported in value initialization.\n");
        break;
    }
    case TK_ALIAS: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.complex_value = new DynamicData(*type_.descriptor_.element_type_);
        break;
    }
    case TK_ENUM: {
        TypeValue &value = value_[MEMBER_ID_INVALID];
        value.int32_value = 0; // 默认值为0
        break;
    }
    case TK_MAP: {
        value_[MEMBER_ID_MAP_KEY].complex_value = new DynamicData(*type_.descriptor_.key_element_type_);
        value_[MEMBER_ID_MAP_VALUE].complex_value = new DynamicData(*type_.descriptor_.element_type_);
    }
    default:
        break;
    }
    return RETCODE_OK;
}

DynamicData::~DynamicData() {
    // 减少类型的引用计数
    type_.decrement_ref_count();
}

bool DynamicData::equals(const DynamicData &other) const {
    if (&type_ != &other.type_) {
        return false;
    }
    // TODO 比较值
    return false;
}

MemberId DynamicData::get_member_id_by_name(const std::string &name) const {
    switch (type_.get_kind()) {
    case TK_STRUCTURE:
    case TK_UNION: {
        auto member = type_.member_by_name_.find(name);
        if (member != type_.member_by_name_.end()) {
            return member->second.get_id();
        }
        break;
    }
    case TK_SEQUENCE:
    case TK_STRING8:
    case TK_STRING16:
    case TK_ARRAY: {
        // must be accessed by index
        return MEMBER_ID_INVALID;
    }
    case TK_MAP: {
        // must be accessed by index
        return MEMBER_ID_INVALID;
    }
    case TK_BITMASK: {
        // TODO Named flags can be accessed using the name
        return MEMBER_ID_INVALID;
    }
    case TK_ENUM:
    // base types
    case TK_BOOLEAN:
    case TK_BYTE:
    case TK_INT16:
    case TK_INT32:
    case TK_INT64:
    case TK_UINT16:
    case TK_UINT32:
    case TK_UINT64:
    case TK_FLOAT32:
    case TK_FLOAT64:
    case TK_FLOAT128:
    case TK_CHAR8:
    case TK_CHAR16: {
        // indicated by "name" using nil or empty string
        return MEMBER_ID_INVALID;
    }
    default:
        break;
    }
    return MEMBER_ID_INVALID;
}

MemberId DynamicData::get_member_id_at_index(size_t index) const { 
    return MEMBER_ID_INVALID; 
}

void DynamicData::clear_value(MemberId id) {
    auto it = value_.find(id);
    if (it != value_.end()) {
        TypeValue &value = it->second;
        switch (type_.get_kind()) {
        case TK_STRUCTURE:
        case TK_UNION: {
            delete value.complex_value;
            value.complex_value = nullptr;
            break;
        }
        case TK_SEQUENCE:
        case TK_ARRAY: {
            switch (type_.descriptor_.kind_) {
            case TK_BOOLEAN:
                value.boolean_values.clear();
                break;
            case TK_BYTE:
                value.byte_values.clear();
                break;
            case TK_INT16:
                value.int16_values.clear();
                break;
            case TK_INT32:
                value.int32_values.clear();
                break;
            case TK_INT64:
                value.int64_values.clear();
                break;
            case TK_UINT16:
                value.uint16_values.clear();
                break;
            case TK_UINT32:
                value.uint32_values.clear();
                break;
            case TK_UINT64:
                value.uint64_values.clear();
                break;
            case TK_FLOAT32:
                value.float32_values.clear();
                break;
            case TK_FLOAT64:
                value.float64_values.clear();
                break;
            case TK_FLOAT128:
                value.float128_values.clear();
                break;
            case TK_CHAR8:
                value.char8_values.clear();
                break;
            case TK_CHAR16:
                value.char16_values.clear();
                break;
            case TK_STRING8:
                value.string_values.clear();
                break;
            case TK_STRING16:
                value.wstring_values.clear();
                break;
                break;
            case TK_ENUM: {
                value.int32_values.clear();
                break;
            }
            case TK_STRUCTURE:
            case TK_UNION: {    
                for (auto &complex_value : value.complex_values) {
                    delete complex_value;
                }
                value.complex_values.clear();
                break;
            }
            case TK_ALIAS:
            case TK_BITMASK:
            case TK_BITSET: {
                break;
            }
            default:
                break;
            }
            break;
        }
        case TK_MAP: {
            for (auto &complex_value : value.complex_values) {
                delete complex_value;
            }
            value.complex_values.clear();
            break;
        }
        default: {
            // 对于基本类型，直接清除值
            it->second = TypeValue(); // 重置为默认值
        }
        }
    }
}

void DynamicData::clear_all_values() {
    for (auto &pair : value_) {
        clear_value(pair.first);
    }
}

void DynamicData::clear_nonkey_values() {
    TRAVODDS_LOG(LOG_ERROR, "clear_nonkey_values not implemented yet.\n");
}

DynamicData *DynamicData::clone() const { 
    DynamicData *clone_data = new DynamicData(*this);
    return clone_data;
}

size_t DynamicData::get_item_count() const {
    ReturnCode_t ret = RETCODE_OK;
    switch (type_.get_kind()) {
    case TK_STRUCTURE:
    case TK_UNION: {
        return value_.size();
    }
    case TK_SEQUENCE: {
        TypeDescriptor descriptor;
        ret = type_.get_descriptor(descriptor);
        if (ret != RETCODE_OK) {
            return -1;
        }
        size_t size = 1;
        for (const auto &bound : descriptor.bound_) {
            size *= bound;
        }
        auto it = value_.find(MEMBER_ID_INVALID);
        if (it == value_.end()) {
            return -1;
        }
        switch (descriptor.element_type_->get_kind()) {
        case TK_BOOLEAN: {
            const TypeValue &value = it->second;
            return value.boolean_values.size();
        }
        case TK_BYTE: {
            const TypeValue &value = it->second;
            return value.byte_values.size();
        }
        case TK_INT16: {
            const TypeValue &value = it->second;
            return value.int16_values.size();
        }
        case TK_ENUM:
        case TK_INT32: {
            const TypeValue &value = it->second;
            return value.int32_values.size();
        }
        case TK_INT64: {
            const TypeValue &value = it->second;
            return value.int64_values.size();
        }
        case TK_UINT16: {
            const TypeValue &value = it->second;
            return value.uint16_values.size();
        }
        case TK_UINT32: {
            const TypeValue &value = it->second;
            return value.uint32_values.size();
        }
        case TK_UINT64: {
            const TypeValue &value = it->second;
            return value.uint64_values.size();
        }
        case TK_FLOAT32: {
            const TypeValue &value = it->second;
            return value.float32_values.size();
        }
        case TK_FLOAT64: {
            const TypeValue &value = it->second;
            return value.float64_values.size();
        }
        case TK_FLOAT128: {
            const TypeValue &value = it->second;
            return value.float128_values.size();
        }
        case TK_CHAR8: {
            const TypeValue &value = it->second;
            return value.char8_values.size();
        }
        case TK_CHAR16: {
            const TypeValue &value = it->second;
            return value.char16_values.size();
        }
        case TK_STRING8: {
            const TypeValue& value = it->second;
            return value.string_values.size();
        }
        case TK_STRING16: {
            const TypeValue& value = it->second;
            return value.wstring_values.size();
        }
        case TK_STRUCTURE: 
        case TK_UNION: {
            const TypeValue& value = it->second;
            return value.complex_values.size();
        }
        default:
            break;
        }
        break;
    }
    case TK_STRING8: {
        auto it = value_.find(MEMBER_ID_INVALID);
        if (it == value_.end()) {
            return -1;
        }
        return it->second.string_value.size();
    }
    case TK_STRING16: {
        auto it = value_.find(MEMBER_ID_INVALID);
        if (it == value_.end()) {
            return -1;
        }
        return it->second.wstring_value.size();
    }
    case TK_ARRAY: {
        TypeDescriptor descriptor;
        ret = type_.get_descriptor(descriptor);
        if (ret != RETCODE_OK) {
            return -1;
        }
        size_t size = 1;
        for (const auto &bound : descriptor.bound_) {
            size *= bound;
        }
        return size;
    }
    case TK_MAP: {
        // must be accessed by index
        return MEMBER_ID_INVALID;
    }
    case TK_BITMASK: {
        // TODO Named flags can be accessed using the name
        return MEMBER_ID_INVALID;
    }
    case TK_ENUM:
    // base types
    case TK_BOOLEAN:
    case TK_BYTE:
    case TK_INT16:
    case TK_INT32:
    case TK_INT64:
    case TK_UINT16:
    case TK_UINT32:
    case TK_UINT64:
    case TK_FLOAT32:
    case TK_FLOAT64:
    case TK_FLOAT128:
    case TK_CHAR8:
    case TK_CHAR16: {
        // indicated by "name" using nil or empty string
        return 1;
    }
    default:
        break;
    }
    return -1;
}

size_t TRAVODDS::DynamicData::get_memberid_item_count(MemberId id) const {
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        // 复杂类型的成员
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData* data = it->second.complex_value;
            return data->get_item_count();
        }
    }
    return 0;
}

DynamicData* DynamicData::loan_value(MemberId id) {
    // TODO: 实现值借用逻辑
    return nullptr;
}

int DynamicData::return_loaned_value(DynamicData &data) {
    // TODO: 实现值归还逻辑
    return RETCODE_OK;
}

DynamicType &DynamicData::get_type() const { return type_; }

ReturnCode_t DynamicData::get_int32_value(int32_t &value, MemberId id) {
    // 基本数据类型
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_ENUM:
        case TK_INT32: {
            value = value_[MEMBER_ID_INVALID].int32_value;
            return RETCODE_OK;
        }
        // 以下为promote: 比较小的类型转换为较大的类型
        case TK_BOOLEAN: {
            value = static_cast<int32_t>(value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        case TK_CHAR8: {
            value = static_cast<int32_t>(value_[MEMBER_ID_INVALID].charvalue);
            return RETCODE_OK;
        }
        case TK_CHAR16: {
            value = static_cast<int32_t>(value_[MEMBER_ID_INVALID].wchar_value);
            return RETCODE_OK;
        }
        case TK_INT16: {
            value = static_cast<int32_t>(value_[MEMBER_ID_INVALID].int16_value);
            return RETCODE_OK;
        }
        case TK_UINT16: {
            value = static_cast<int32_t>(value_[MEMBER_ID_INVALID].uint16_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        // 复杂类型的成员
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_int32_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_int32_value(MemberId id, int32_t value) {
    // 基本数据类型
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_ENUM:
        case TK_INT32: {
            value_[MEMBER_ID_INVALID].int32_value = value;
            return RETCODE_OK;
        }
        // 以下为promote: 比较小的类型转换为较大的类型
        case TK_INT64: {
            value_[MEMBER_ID_INVALID].int64_value =
                static_cast<int64_t>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT64: {
            value_[MEMBER_ID_INVALID].float64_value =
                static_cast<double>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT128: {
            value_[MEMBER_ID_INVALID].float128_value =
                static_cast<long double>(value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        // 复杂类型的成员
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_int32_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

// 基本类型的 get/set 方法实现
ReturnCode_t DynamicData::get_uint32_value(uint32_t &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_UINT32: {
            value = value_[MEMBER_ID_INVALID].uint32_value;
            return RETCODE_OK;
        }
        // promote: 较小类型转换为较大类型
        case TK_BOOLEAN: {
            value = static_cast<uint32_t>(
                value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        case TK_BYTE: {
            value = static_cast<uint32_t>(
                value_[MEMBER_ID_INVALID].byte_value);
            return RETCODE_OK;
        }
        case TK_UINT16: {
            value = static_cast<uint32_t>(
                value_[MEMBER_ID_INVALID].uint16_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_uint32_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_uint32_value(MemberId id, uint32_t value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_UINT32: {
            value_[MEMBER_ID_INVALID].uint32_value = value;
            return RETCODE_OK;
        }
        // promote: 较小类型转换为较大类型
        case TK_INT64: {
            value_[MEMBER_ID_INVALID].int64_value =
                static_cast<int64_t>(value);
            return RETCODE_OK;
        }
        case TK_UINT64: {
            value_[MEMBER_ID_INVALID].uint64_value =
                static_cast<uint64_t>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT64: {
            value_[MEMBER_ID_INVALID].float64_value =
                static_cast<double>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT128: {
            value_[MEMBER_ID_INVALID].float128_value =
                static_cast<long double>(value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_uint32_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_int16_value(short &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_INT16: {
            value = value_[MEMBER_ID_INVALID].int16_value;
            return RETCODE_OK;
        }
        // promote: 较小类型转换为较大类型
        case TK_BYTE: {
            value = static_cast<short>(value_[MEMBER_ID_INVALID].byte_value);
            return RETCODE_OK;
        }
        case TK_BOOLEAN: {
            value = static_cast<short>(value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        case TK_CHAR8: {
            value = static_cast<short>(value_[MEMBER_ID_INVALID].charvalue);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_int16_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_int16_value(MemberId id, short value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_INT16: {
            value_[MEMBER_ID_INVALID].int16_value = value;
            return RETCODE_OK;
        }
        case TK_INT32: {
            value_[MEMBER_ID_INVALID].int32_value = static_cast<int32_t>(value);
            return RETCODE_OK;
        }
        case TK_INT64: {
            value_[MEMBER_ID_INVALID].int64_value =
                static_cast<int64_t>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT32: {
            value_[MEMBER_ID_INVALID].float32_value = static_cast<float>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT64: {
            value_[MEMBER_ID_INVALID].float64_value =
                static_cast<double>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT128: {
            value_[MEMBER_ID_INVALID].float128_value =
                static_cast<long double>(value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_int16_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_uint16_value(unsigned short &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_UINT16: {
            value = value_[MEMBER_ID_INVALID].uint16_value;
            return RETCODE_OK;
        }
        case TK_BOOLEAN: {
            value = static_cast<unsigned short>(
                value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        case TK_BYTE: {
            value = static_cast<unsigned short>(
                value_[MEMBER_ID_INVALID].byte_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_uint16_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_uint16_value(MemberId id, unsigned short value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_UINT16: {
            value_[MEMBER_ID_INVALID].uint16_value = value;
            return RETCODE_OK;
        }
        case TK_UINT32: {
            value_[MEMBER_ID_INVALID].uint32_value =
                static_cast<uint32_t>(value);
            return RETCODE_OK;
        }
        case TK_UINT64: {
            value_[MEMBER_ID_INVALID].uint64_value =
                static_cast<uint64_t>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT32: {
            value_[MEMBER_ID_INVALID].float32_value = static_cast<float>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT64: {
            value_[MEMBER_ID_INVALID].float64_value =
                static_cast<double>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT128: {
            value_[MEMBER_ID_INVALID].float128_value =
                static_cast<long double>(value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_uint16_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_int64_value(int64_t &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_INT64: {
            value = value_[MEMBER_ID_INVALID].int64_value;
            return RETCODE_OK;
        }
        case TK_BOOLEAN: {
            value =
                static_cast<int64_t>(value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        case TK_CHAR8: {
            value = static_cast<int64_t>(value_[MEMBER_ID_INVALID].charvalue);
            return RETCODE_OK;
        }
        case TK_CHAR16: {
            value =
                static_cast<int64_t>(value_[MEMBER_ID_INVALID].wchar_value);
            return RETCODE_OK;
        }
        case TK_INT16: {
            value =
                static_cast<int64_t>(value_[MEMBER_ID_INVALID].int16_value);
            return RETCODE_OK;
        }
        case TK_INT32: {
            value =
                static_cast<int64_t>(value_[MEMBER_ID_INVALID].int32_value);
            return RETCODE_OK;
        }
        case TK_UINT16: {
            value =
                static_cast<int64_t>(value_[MEMBER_ID_INVALID].uint16_value);
            return RETCODE_OK;
        }
        case TK_UINT32: {
            value =
                static_cast<int64_t>(value_[MEMBER_ID_INVALID].uint32_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_int64_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_int64_value(MemberId id, int64_t value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_INT64: {
            value_[MEMBER_ID_INVALID].int64_value = value;
            return RETCODE_OK;
        }
        case TK_FLOAT64: {
            value_[MEMBER_ID_INVALID].float64_value =
                static_cast<double>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT128: {
            value_[MEMBER_ID_INVALID].float128_value =
                static_cast<long double>(value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_int64_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_uint64_value(uint64_t &value,
                                           MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_UINT64: {
            value = value_[MEMBER_ID_INVALID].uint64_value;
            return RETCODE_OK;
        }
        case TK_BOOLEAN: {
            value = static_cast<uint64_t>(
                value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        case TK_BYTE: {
            value = static_cast<uint64_t>(
                value_[MEMBER_ID_INVALID].byte_value);
            return RETCODE_OK;
        }
        case TK_UINT16: {
            value = static_cast<uint64_t>(
                value_[MEMBER_ID_INVALID].uint16_value);
            return RETCODE_OK;
        }
        case TK_UINT32: {
            value = static_cast<uint64_t>(
                value_[MEMBER_ID_INVALID].uint32_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_uint64_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_uint64_value(MemberId id,
                                           uint64_t value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_UINT64: {
            value_[MEMBER_ID_INVALID].uint64_value = value;
            return RETCODE_OK;
        }
        case TK_FLOAT64: {
            value_[MEMBER_ID_INVALID].float64_value =
                static_cast<double>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT128: {
            value_[MEMBER_ID_INVALID].float128_value =
                static_cast<long double>(value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_uint64_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_float32_value(float &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_FLOAT32: {
            value = value_[MEMBER_ID_INVALID].float32_value;
            return RETCODE_OK;
        }
        case TK_BOOLEAN: {
            value = static_cast<float>(value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        case TK_CHAR8: {
            value = static_cast<float>(value_[MEMBER_ID_INVALID].charvalue);
            return RETCODE_OK;
        }
        case TK_CHAR16: {
            value = static_cast<float>(value_[MEMBER_ID_INVALID].wchar_value);
            return RETCODE_OK;
        }
        case TK_INT16: {
            value = static_cast<float>(value_[MEMBER_ID_INVALID].int16_value);
            return RETCODE_OK;
        }
        case TK_INT32: {
            value = static_cast<float>(value_[MEMBER_ID_INVALID].int32_value);
            return RETCODE_OK;
        }
        case TK_INT64: {
            value = static_cast<float>(value_[MEMBER_ID_INVALID].int64_value);
            return RETCODE_OK;
        }
        case TK_UINT16: {
            value = static_cast<float>(value_[MEMBER_ID_INVALID].uint16_value);
            return RETCODE_OK;
        }
        case TK_UINT32: {
            value = static_cast<float>(value_[MEMBER_ID_INVALID].uint32_value);
            return RETCODE_OK;
        }
        case TK_UINT64: {
            value = static_cast<float>(value_[MEMBER_ID_INVALID].uint64_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_float32_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_float32_value(MemberId id, float value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_FLOAT32: {
            value_[MEMBER_ID_INVALID].float32_value = value;
            return RETCODE_OK;
        }
        case TK_FLOAT64: {
            value_[MEMBER_ID_INVALID].float64_value =
                static_cast<double>(value);
            return RETCODE_OK;
        }
        case TK_FLOAT128: {
            value_[MEMBER_ID_INVALID].float128_value =
                static_cast<long double>(value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_float32_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_float64_value(double &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_FLOAT64: {
            value = value_[MEMBER_ID_INVALID].float64_value;
            return RETCODE_OK;
        }
        case TK_BOOLEAN: {
            value =
                static_cast<double>(value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        case TK_CHAR8: {
            value = static_cast<double>(value_[MEMBER_ID_INVALID].charvalue);
            return RETCODE_OK;
        }
        case TK_CHAR16: {
            value = static_cast<double>(value_[MEMBER_ID_INVALID].wchar_value);
            return RETCODE_OK;
        }
        case TK_INT16: {
            value = static_cast<double>(value_[MEMBER_ID_INVALID].int16_value);
            return RETCODE_OK;
        }
        case TK_INT32: {
            value = static_cast<double>(value_[MEMBER_ID_INVALID].int32_value);
            return RETCODE_OK;
        }
        case TK_INT64: {
            value = static_cast<double>(value_[MEMBER_ID_INVALID].int64_value);
            return RETCODE_OK;
        }
        case TK_UINT16: {
            value = static_cast<double>(value_[MEMBER_ID_INVALID].uint16_value);
            return RETCODE_OK;
        }
        case TK_UINT32: {
            value = static_cast<double>(value_[MEMBER_ID_INVALID].uint32_value);
            return RETCODE_OK;
        }
        case TK_UINT64: {
            value = static_cast<double>(value_[MEMBER_ID_INVALID].uint64_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_float64_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_float64_value(MemberId id, double value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_FLOAT64: {
            value_[MEMBER_ID_INVALID].float64_value = value;
            return RETCODE_OK;
        }
        case TK_FLOAT128: {
            value_[MEMBER_ID_INVALID].float128_value =
                static_cast<long double>(value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_float64_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_float128_value(long double &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_FLOAT128: {
            value = value_[MEMBER_ID_INVALID].float128_value;
            return RETCODE_OK;
        }
        case TK_BOOLEAN: {
            value = static_cast<long double>(
                value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        case TK_CHAR8: {
            value =
                static_cast<long double>(value_[MEMBER_ID_INVALID].charvalue);
            return RETCODE_OK;
        }
        case TK_CHAR16: {
            value =
                static_cast<long double>(value_[MEMBER_ID_INVALID].wchar_value);
            return RETCODE_OK;
        }
        case TK_INT16: {
            value =
                static_cast<long double>(value_[MEMBER_ID_INVALID].int16_value);
            return RETCODE_OK;
        }
        case TK_INT32: {
            value =
                static_cast<long double>(value_[MEMBER_ID_INVALID].int32_value);
            return RETCODE_OK;
        }
        case TK_INT64: {
            value =
                static_cast<long double>(value_[MEMBER_ID_INVALID].int64_value);
            return RETCODE_OK;
        }
        case TK_UINT16: {
            value = static_cast<long double>(
                value_[MEMBER_ID_INVALID].uint16_value);
            return RETCODE_OK;
        }
        case TK_UINT32: {
            value = static_cast<long double>(
                value_[MEMBER_ID_INVALID].uint32_value);
            return RETCODE_OK;
        }
        case TK_UINT64: {
            value = static_cast<long double>(
                value_[MEMBER_ID_INVALID].uint64_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_float128_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_float128_value(MemberId id, long double value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_FLOAT128: {
            value_[MEMBER_ID_INVALID].float128_value = value;
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_float128_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_char8_value(char &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_CHAR8: {
            value = value_[MEMBER_ID_INVALID].charvalue;
            return RETCODE_OK;
        }
        case TK_BOOLEAN: {
            value = static_cast<char>(value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_char8_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_char8_value(MemberId id, char value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_CHAR8: {
            value_[MEMBER_ID_INVALID].charvalue = value;
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_char8_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_char16_value(wchar_t &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_CHAR16: {
            value = value_[MEMBER_ID_INVALID].wchar_value;
            return RETCODE_OK;
        }
        case TK_BOOLEAN: {
            value =
                static_cast<wchar_t>(value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_char16_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_char16_value(MemberId id, wchar_t value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_CHAR16: {
            value_[MEMBER_ID_INVALID].wchar_value = value;
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_char16_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_byte_value(octet &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_BYTE: {
            value = value_[MEMBER_ID_INVALID].byte_value;
            return RETCODE_OK;
        }
        case TK_BOOLEAN: {
            value = static_cast<octet>(value_[MEMBER_ID_INVALID].boolean_value);
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_byte_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_byte_value(MemberId id, octet value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_BYTE: {
            value_[MEMBER_ID_INVALID].byte_value = value;
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_byte_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_boolean_value(bool &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_BOOLEAN: {
            value = value_[MEMBER_ID_INVALID].boolean_value;
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_boolean_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_boolean_value(MemberId id, bool value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_BOOLEAN: {
            value_[MEMBER_ID_INVALID].boolean_value = value;
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_boolean_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

// 字符串类型的处理
ReturnCode_t DynamicData::get_string_value(std::string &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_STRING8: {
            value = value_[MEMBER_ID_INVALID].string_value;
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_string_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_string_value(MemberId id,
                                           const std::string &value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_STRING8: {
            value_[MEMBER_ID_INVALID].string_value = value;
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_string_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_wstring_value(std::wstring &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_STRING16: {
            value = value_[MEMBER_ID_INVALID].wstring_value;
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_wstring_value(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_wstring_value(MemberId id,
                                            const std::wstring &value) {
    if (id == MEMBER_ID_INVALID) {
        switch (type_.descriptor_.kind_) {
        case TK_STRING16: {
            value_[MEMBER_ID_INVALID].wstring_value = value;
            return RETCODE_OK;
        }
        default:
            break;
        }
    }

    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_wstring_value(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_complex_value(DynamicData &value, MemberId id) {
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            value = *data;
            return RETCODE_OK;
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_complex_value(MemberId id, const DynamicData &value) {
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            *data = value;
            return RETCODE_OK;
        }
    }
    return RETCODE_BAD_PARAMETER;
}


ReturnCode_t DynamicData::get_complex_values(DynamicDataSeq &value, MemberId id) {
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicDataSeq& data = it->second.complex_values;
            value = data;
            return RETCODE_OK;
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_complex_values(MemberId id, const DynamicDataSeq &value) {
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicDataSeq& data = it->second.complex_values;
            data = value;
            return RETCODE_OK;
        }
    }
    return RETCODE_BAD_PARAMETER;
}

// 序列类型的处理
ReturnCode_t DynamicData::get_int32_values(Int32Seq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_ENUM:
            case TK_INT32: {
                value = curValue.int32_values;
                return RETCODE_OK;
            }
            // 也适用于较小类型的提升
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int32_t>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_CHAR8: {
                value.resize(curValue.char8_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int32_t>(curValue.char8_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_CHAR16: {
                value.resize(curValue.char16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int32_t>(curValue.char16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_BYTE: {
                value.resize(curValue.byte_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int32_t>(curValue.byte_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT16: {
                value.resize(curValue.int16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int32_t>(curValue.int16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT16: {
                value.resize(curValue.uint16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int32_t>(curValue.uint16_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_int32_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_int32_values(MemberId id, const Int32Seq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_ENUM:
            case TK_INT32: {
                if (isArray) {
                    // 如果是数组类型，不能修改原本的大小，最多复制到原本的大小，多出来的部分丢弃
                    size_t copySize =
                        std::min(value.size(), curValue.int32_values.size());
                    curValue.int32_values.assign(value.begin(),
                                                 value.begin() + copySize);
                } else {
                    curValue.int32_values = value;
                }
                return RETCODE_OK;
            }
            // 也适用于提升为较大类型
            case TK_INT64: {
                if (isArray) {
                    // 如果是数组类型，不能修改原本的大小，最多复制到原本的大小，多出来的部分丢弃
                    size_t copySize =
                        std::min(value.size(), curValue.int64_values.size());
                    curValue.int64_values.assign(value.begin(),
                                                 value.begin() + copySize);
                } else {
                    curValue.int64_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.int64_values[i] =
                            static_cast<int64_t>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_FLOAT64: {
                if (isArray) {
                    // 如果是数组类型，不能修改原本的大小，最多复制到原本的大小，多出来的部分丢弃
                    size_t copySize =
                        std::min(value.size(), curValue.float64_values.size());
                    curValue.float64_values.assign(value.begin(),
                                                   value.begin() + copySize);
                } else {
                    curValue.float64_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_FLOAT128: {
                if (isArray) {
                    // 如果是数组类型，不能修改原本的大小，最多复制到原本的大小，多出来的部分丢弃
                    size_t copySize =
                        std::min(value.size(), curValue.float128_values.size());
                    curValue.float128_values.assign(value.begin(),
                                                    value.begin() + copySize);
                } else {
                    curValue.float128_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_int32_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_uint32_values(UInt32Seq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_UINT32: {
                value = curValue.uint32_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<uint32_t>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_BYTE: {
                value.resize(curValue.byte_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<uint32_t>(curValue.byte_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT16: {
                value.resize(curValue.uint16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<uint32_t>(curValue.uint16_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_uint32_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_uint32_values(MemberId id,
                                            const UInt32Seq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_UINT32: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.uint32_values.size());
                    curValue.uint32_values.assign(value.begin(),
                                                  value.begin() + copySize);
                } else {
                    curValue.uint32_values = value;
                }
                return RETCODE_OK;
            }
            case TK_UINT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.uint64_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.uint64_values[i] =
                            static_cast<uint64_t>(value[i]);
                    }
                } else {
                    curValue.uint64_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.uint64_values[i] =
                            static_cast<uint64_t>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_FLOAT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float64_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                } else {
                    curValue.float64_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_uint32_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_uint16_values(UInt16Seq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_UINT16: {
                value = curValue.uint16_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<uint16_t>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_BYTE: {
                value.resize(curValue.byte_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<uint16_t>(curValue.byte_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_uint16_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_uint16_values(MemberId id,
                                            const UInt16Seq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_UINT16: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.uint16_values.size());
                    curValue.uint16_values.assign(value.begin(),
                                                  value.begin() + copySize);
                } else {
                    curValue.uint16_values = value;
                }
                return RETCODE_OK;
            }
            case TK_UINT32: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.uint32_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.uint32_values[i] =
                            static_cast<uint32_t>(value[i]);
                    }
                } else {
                    curValue.uint32_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.uint32_values[i] =
                            static_cast<uint32_t>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_UINT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.uint64_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.uint64_values[i] =
                            static_cast<uint64_t>(value[i]);
                    }
                } else {
                    curValue.uint64_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.uint64_values[i] =
                            static_cast<uint64_t>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_uint16_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_int16_values(Int16Seq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_INT16: {
                value = curValue.int16_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int16_t>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_BYTE: {
                value.resize(curValue.byte_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int16_t>(curValue.byte_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_int16_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_int16_values(MemberId id, const Int16Seq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_INT16: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.int16_values.size());
                    curValue.int16_values.assign(value.begin(),
                                                 value.begin() + copySize);
                } else {
                    curValue.int16_values = value;
                }
                return RETCODE_OK;
            }
            case TK_INT32: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.int32_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.int32_values[i] =
                            static_cast<int32_t>(value[i]);
                    }
                } else {
                    curValue.int32_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.int32_values[i] =
                            static_cast<int32_t>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_INT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.int64_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.int64_values[i] =
                            static_cast<int64_t>(value[i]);
                    }
                } else {
                    curValue.int64_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.int64_values[i] =
                            static_cast<int64_t>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_FLOAT32: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float32_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float32_values[i] =
                            static_cast<float>(value[i]);
                    }
                } else {
                    curValue.float32_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float32_values[i] =
                            static_cast<float>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_FLOAT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float64_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                } else {
                    curValue.float64_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_FLOAT128: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float128_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                } else {
                    curValue.float128_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_int16_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_int64_values(Int64Seq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_INT64: {
                value = curValue.int64_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int64_t>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_CHAR8: {
                value.resize(curValue.char8_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int64_t>(curValue.char8_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_CHAR16: {
                value.resize(curValue.char16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int64_t>(curValue.char16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_BYTE: {
                value.resize(curValue.byte_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int64_t>(curValue.byte_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT16: {
                value.resize(curValue.int16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int64_t>(curValue.int16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT32: {
                value.resize(curValue.int32_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int64_t>(curValue.int32_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT16: {
                value.resize(curValue.uint16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int64_t>(curValue.uint16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT32: {
                value.resize(curValue.uint32_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int64_t>(curValue.uint32_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT64: {
                value.resize(curValue.uint64_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<int64_t>(curValue.uint64_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_int64_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_int64_values(MemberId id, const Int64Seq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_INT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.int64_values.size());
                    curValue.int64_values.assign(value.begin(),
                                                 value.begin() + copySize);
                } else {
                    curValue.int64_values = value;
                }
                return RETCODE_OK;
            }
            case TK_FLOAT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float64_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                } else {
                    curValue.float64_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_FLOAT128: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float128_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                } else {
                    curValue.float128_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_int64_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_uint64_values(UInt64Seq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_UINT64: {
                value = curValue.uint64_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<uint64_t>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_BYTE: {
                value.resize(curValue.byte_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<uint64_t>(curValue.byte_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT16: {
                value.resize(curValue.uint16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<uint64_t>(curValue.uint16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT32: {
                value.resize(curValue.uint32_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<uint64_t>(curValue.uint32_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_uint64_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_uint64_values(MemberId id,
                                            const UInt64Seq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_UINT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.uint64_values.size());
                    curValue.uint64_values.assign(value.begin(),
                                                  value.begin() + copySize);
                } else {
                    curValue.uint64_values = value;
                }
                return RETCODE_OK;
            }
            case TK_FLOAT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float64_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                } else {
                    curValue.float64_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_FLOAT128: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float128_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                } else {
                    curValue.float128_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_uint64_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_float32_values(Float32Seq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_FLOAT32: {
                value = curValue.float32_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<float>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_CHAR8: {
                value.resize(curValue.char8_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<float>(curValue.char8_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_CHAR16: {
                value.resize(curValue.char16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<float>(curValue.char16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_BYTE: {
                value.resize(curValue.byte_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<float>(curValue.byte_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT16: {
                value.resize(curValue.int16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<float>(curValue.int16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT32: {
                value.resize(curValue.int32_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<float>(curValue.int32_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT64: {
                value.resize(curValue.int64_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<float>(curValue.int64_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT16: {
                value.resize(curValue.uint16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<float>(curValue.uint16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT32: {
                value.resize(curValue.uint32_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<float>(curValue.uint32_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT64: {
                value.resize(curValue.uint64_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<float>(curValue.uint64_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_float32_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_float32_values(MemberId id,
                                             const Float32Seq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_FLOAT32: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float32_values.size());
                    curValue.float32_values.assign(value.begin(),
                                                   value.begin() + copySize);
                } else {
                    curValue.float32_values = value;
                }
                return RETCODE_OK;
            }
            case TK_FLOAT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float64_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                } else {
                    curValue.float64_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float64_values[i] =
                            static_cast<double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            case TK_FLOAT128: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float128_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                } else {
                    curValue.float128_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_float32_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_float64_values(Float64Seq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_FLOAT64: {
                value = curValue.float64_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<double>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_CHAR8: {
                value.resize(curValue.char8_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<double>(curValue.char8_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_CHAR16: {
                value.resize(curValue.char16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<double>(curValue.char16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_BYTE: {
                value.resize(curValue.byte_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<double>(curValue.byte_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT16: {
                value.resize(curValue.int16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<double>(curValue.int16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT32: {
                value.resize(curValue.int32_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<double>(curValue.int32_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT64: {
                value.resize(curValue.int64_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<double>(curValue.int64_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT16: {
                value.resize(curValue.uint16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<double>(curValue.uint16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT32: {
                value.resize(curValue.uint32_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<double>(curValue.uint32_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT64: {
                value.resize(curValue.uint64_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<double>(curValue.uint64_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_float64_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_float64_values(MemberId id,
                                             const Float64Seq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_FLOAT64: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float64_values.size());
                    curValue.float64_values.assign(value.begin(),
                                                   value.begin() + copySize);
                } else {
                    curValue.float64_values = value;
                }
                return RETCODE_OK;
            }
            case TK_FLOAT128: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float128_values.size());
                    for (size_t i = 0; i < copySize; ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                } else {
                    curValue.float128_values.resize(value.size());
                    for (size_t i = 0; i < value.size(); ++i) {
                        curValue.float128_values[i] =
                            static_cast<long double>(value[i]);
                    }
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_float64_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_float128_values(Float128Seq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_FLOAT128: {
                value = curValue.float128_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<long double>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_CHAR8: {
                value.resize(curValue.char8_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<long double>(curValue.char8_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_CHAR16: {
                value.resize(curValue.char16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<long double>(curValue.char16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_BYTE: {
                value.resize(curValue.byte_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<long double>(curValue.byte_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT16: {
                value.resize(curValue.int16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<long double>(curValue.int16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT32: {
                value.resize(curValue.int32_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<long double>(curValue.int32_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_INT64: {
                value.resize(curValue.int64_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<long double>(curValue.int64_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT16: {
                value.resize(curValue.uint16_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<long double>(curValue.uint16_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT32: {
                value.resize(curValue.uint32_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<long double>(curValue.uint32_values[i]);
                }
                return RETCODE_OK;
            }
            case TK_UINT64: {
                value.resize(curValue.uint64_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] =
                        static_cast<long double>(curValue.uint64_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_float128_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_float128_values(MemberId id,
                                              const Float128Seq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_FLOAT128: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.float128_values.size());
                    curValue.float128_values.assign(value.begin(),
                                                    value.begin() + copySize);
                } else {
                    curValue.float128_values = value;
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_float128_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_char8_values(CharSeq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_CHAR8: {
                value = curValue.char8_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<char>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_char8_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_char8_values(MemberId id, const CharSeq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_CHAR8: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.char8_values.size());
                    curValue.char8_values.assign(value.begin(),
                                                 value.begin() + copySize);
                } else {
                    curValue.char8_values = value;
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_char8_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_char16_values(WcharSeq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_CHAR16: {
                value = curValue.char16_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<wchar_t>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_char16_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_char16_values(MemberId id,
                                            const WcharSeq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_CHAR16: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.char16_values.size());
                    curValue.char16_values.assign(value.begin(),
                                                  value.begin() + copySize);
                } else {
                    curValue.char16_values = value;
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_char16_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_byte_values(ByteSeq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_BYTE: {
                value = curValue.byte_values;
                return RETCODE_OK;
            }
            case TK_BOOLEAN: {
                value.resize(curValue.boolean_values.size());
                for (size_t i = 0; i < value.size(); ++i) {
                    value[i] = static_cast<octet>(curValue.boolean_values[i]);
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_byte_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_byte_values(MemberId id, const ByteSeq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_BYTE: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.byte_values.size());
                    curValue.byte_values.assign(value.begin(),
                                                value.begin() + copySize);
                } else {
                    curValue.byte_values = value;
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_byte_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_boolean_values(BooleanSeq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_BOOLEAN: {
                value = curValue.boolean_values;
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_boolean_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_boolean_values(MemberId id,
                                             const BooleanSeq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_BOOLEAN: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.boolean_values.size());
                    curValue.boolean_values.assign(value.begin(),
                                                   value.begin() + copySize);
                } else {
                    curValue.boolean_values = value;
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_boolean_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_string_values(StringSeq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_STRING8: {
                value = curValue.string_values;
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_string_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_string_values(MemberId id,
                                            const StringSeq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_STRING8: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.string_values.size());
                    curValue.string_values.assign(value.begin(),
                                                  value.begin() + copySize);
                } else {
                    curValue.string_values = value;
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_string_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::get_wstring_values(WstringSeq &value, MemberId id) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            switch (eleKind) {
            case TK_STRING16: {
                value = curValue.wstring_values;
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->get_wstring_values(value, MEMBER_ID_INVALID);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

ReturnCode_t DynamicData::set_wstring_values(MemberId id,
                                             const WstringSeq &value) {
    if (id == MEMBER_ID_INVALID) {
        if (type_.descriptor_.kind_ == TK_SEQUENCE ||
            type_.descriptor_.kind_ == TK_ARRAY) {
            TypeValue &curValue = value_[MEMBER_ID_INVALID];
            TypeKind eleKind = type_.descriptor_.element_type_->get_kind();
            bool isArray = type_.descriptor_.kind_ == TK_ARRAY;
            switch (eleKind) {
            case TK_STRING16: {
                if (isArray) {
                    size_t copySize =
                        std::min(value.size(), curValue.wstring_values.size());
                    curValue.wstring_values.assign(value.begin(),
                                                   value.begin() + copySize);
                } else {
                    curValue.wstring_values = value;
                }
                return RETCODE_OK;
            }
            default:
                break;
            }
        }
    }
    if (type_.descriptor_.kind_ == TK_STRUCTURE ||
        type_.descriptor_.kind_ == TK_UNION) {
        auto it = value_.find(id);
        if (it != value_.end()) {
            DynamicData *data = it->second.complex_value;
            return data->set_wstring_values(MEMBER_ID_INVALID, value);
        }
    }
    return RETCODE_BAD_PARAMETER;
}

TRAVODDS_NAMESPACE_END
