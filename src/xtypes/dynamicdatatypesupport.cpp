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

#include "xtypes/dynamicdatatypesupport.h"
#include "xtypes/dynamicdata.h"
#include "xtypes/dynamicdatafactory.h"

USING_TRAVODDS_NAMESPACE

DynamicDataTypeSupport::DynamicDataTypeSupport(DynamicType *type) : type_(type) {}

void *DynamicDataTypeSupport::create_data() { return DynamicDataFactory::get_instance().create_data(*type_); }

void DynamicDataTypeSupport::delete_data(void *data) {
    if (data) {
        DynamicDataFactory::get_instance().delete_data(*static_cast<DynamicData *>(data));
    }
}

int DynamicDataTypeSupport::copy_data(void *dst, void *src) {
    if (!dst || !src) {
        return RETCODE_BAD_PARAMETER;
    }
    DynamicData *dynamicDataDst = static_cast<DynamicData *>(dst);
    DynamicData *dynamicDataSrc = static_cast<DynamicData *>(src);
    if (!dynamicDataDst || !dynamicDataSrc) {
        return RETCODE_BAD_PARAMETER;
    }
    *dynamicDataDst = *dynamicDataSrc;
    return RETCODE_OK;
}

unsigned int DynamicDataTypeSupport::get_member_serialize_size(DynamicData *dynamicData, DynamicType *type, MemberId id,
                                                               const std::string &name, unsigned int currentAlignment) {
    ReturnCode_t retcode;
    unsigned int initialAlignment = currentAlignment;
    switch (type->get_kind()) {
    case TK_BOOLEAN: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(bool), currentAlignment);
        break;
    }
    case TK_CHAR8:
    case TK_BYTE: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(octet), currentAlignment);
        break;
    }
    case TK_CHAR16:
    case TK_INT16: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int16_t), currentAlignment);
        break;
    }
    case TK_INT32: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int32_t), currentAlignment);
        break;
    }
    case TK_INT64: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int64_t), currentAlignment);
        break;
    }
    case TK_UINT16: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint16_t), currentAlignment);
        break;
    }
    case TK_UINT32: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
        break;
    }
    case TK_UINT64: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint64_t), currentAlignment);
        break;
    }
    case TK_FLOAT32: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(float), currentAlignment);
        break;
    }
    case TK_FLOAT64: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(double), currentAlignment);
        break;
    }
    case TK_FLOAT128: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(long double), currentAlignment);
        break;
    }
    case TK_STRING8: {
        std::string value;
        if (dynamicData->get_string_value(value, id) != RETCODE_OK) {
            printf("Failed to get string8 value for member ID %u\n", id);
            return 0;
        }
        currentAlignment += CdrSerializer::getStringSize(static_cast<uint32_t>(value.length() + 1), currentAlignment);
        break;
    }
    case TK_STRING16: {
        std::wstring value;
        if (dynamicData->get_wstring_value(value, id) != RETCODE_OK) {
            printf("Failed to get string16 value for member ID %u\n", id);
            return 0;
        }
        currentAlignment += CdrSerializer::getWStringSize(static_cast<uint32_t>(value.length()), currentAlignment);
        break;
    }
    case TK_ENUM: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int32_t), currentAlignment);
        break;
    }
    case TK_BITMASK: {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
        break;
    }
    case TK_SEQUENCE:
    case TK_ARRAY: {
        bool isSequence = (type->get_kind() == TK_SEQUENCE);
        DynamicType *eleType = type->descriptor_.element_type_;
        if (!eleType) {
            printf("Element type is not defined for sequence member %s\n", name.c_str());
            return -1;
        }
        if (isSequence) {
            // 序列化长度
            currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
        }
        size_t eleCount = dynamicData->get_memberid_item_count(id);
        switch (eleType->get_kind()) {
        case TK_BOOLEAN: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(bool), eleCount, currentAlignment);
            break;
        }
        case TK_CHAR8:
        case TK_BYTE: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(octet), eleCount, currentAlignment);
            break;
        }
        case TK_CHAR16:
        case TK_INT16: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int16_t), eleCount, currentAlignment);
            break;
        }
        case TK_INT32: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int32_t), eleCount, currentAlignment);
            break;
        }
        case TK_INT64: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int64_t), eleCount, currentAlignment);
            break;
        }
        case TK_UINT16: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(uint16_t), eleCount, currentAlignment);
            break;
        }
        case TK_UINT32: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(uint32_t), eleCount, currentAlignment);
            break;
        }
        case TK_UINT64: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(uint64_t), eleCount, currentAlignment);
            break;
        }
        case TK_FLOAT32: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(float), eleCount, currentAlignment);
            break;
        }
        case TK_FLOAT64: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(double), eleCount, currentAlignment);
            break;
        }
        case TK_FLOAT128: {
            currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(long double), eleCount, currentAlignment);
            break;
        }
        case TK_STRING8: {
            DynamicData::StringSeq values;
            if (dynamicData->get_string_values(values, id) != RETCODE_OK) {
                printf("Failed to get string8 value for member ID %u\n", id);
                return 0;
            }
            for (std::string& value : values) {
                currentAlignment += CdrSerializer::getStringSize(static_cast<uint32_t>(value.length() + 1), currentAlignment);
            }
            break;
        }
        case TK_STRING16: {
            DynamicData::WstringSeq values;
            if (dynamicData->get_wstring_values(values, id) != RETCODE_OK) {
                printf("Failed to get string16 value for member ID %u\n", id);
                return -1;
            }
            for (std::wstring& value : values) {
                currentAlignment += CdrSerializer::getWStringSize(static_cast<uint32_t>(value.length()), currentAlignment);
            }
            break;
        }
        case TK_STRUCTURE:
        case TK_UNION: {
            DynamicDataSeq values;
            retcode = dynamicData->get_complex_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get structure sequence for member %s\n", name.c_str());
                return -1;
            }
            for (DynamicData *value : values) {
                if (!value) {
                    printf("Null value in sequence for member %s\n", name.c_str());
                    return -1;
                }
                currentAlignment += get_serialized_data_size(value, currentAlignment);
            }
        }
        }
        break;
    }
    case TK_STRUCTURE: {
        DynamicData memberValue(*type);
        retcode = dynamicData->get_complex_value(memberValue, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get structure value for member ID %u\n", id);
            return 0;
        }
        currentAlignment += get_serialized_data_size(&memberValue, currentAlignment);
        break;
    }
    case TK_UNION: {
    }
    case TK_MAP: {
        DynamicData memberValue(*type);
        retcode = dynamicData->get_complex_value(memberValue, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get structure value for member ID %u\n", id);
            return 0;
        }
        currentAlignment += get_serialized_data_size(&memberValue, currentAlignment);
        break;

    }
    default:
        printf("Unsupported type kind for member ID %u\n", id);
        return 0;
    }

    return currentAlignment - initialAlignment;
}

unsigned int DynamicDataTypeSupport::get_serialized_data_size(void *data, unsigned int currentAlignment) {
    DynamicData *dynamicData = static_cast<DynamicData *>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    DynamicType &type = dynamicData->get_type();
    for (uint32_t i = 0; i < type.get_member_count(); ++i) {
        DynamicTypeMember member;
        MemberDescriptor memberDescriptor;
        ReturnCode_t retcode = type.get_member_by_index(member, i);
        if (retcode != RETCODE_OK) {
            printf("Failed to get member by index %u\n", i);
            return RETCODE_ERROR;
        }
        retcode = member.get_descriptor(memberDescriptor);
        if (retcode != RETCODE_OK) {
            printf("Failed to get member descriptor for member %u\n", i);
            return RETCODE_ERROR;
        }
        currentAlignment += get_member_serialize_size(dynamicData, memberDescriptor.type_, memberDescriptor.id_, memberDescriptor.name_, currentAlignment);
    }

    return currentAlignment - initialAlignment;
}

unsigned int DynamicDataTypeSupport::get_max_serialized_data_size(void *data, unsigned int currentAlignment) {
    return -1;
}

int DynamicDataTypeSupport::serialize_member(DynamicData *dynamicData, CdrSerializer *cdr, DynamicType *type, MemberId id,
                                             const std::string &name, int endian) {
    ReturnCode_t retcode;
    switch (type->get_kind()) {
    case TK_BOOLEAN: {
        bool value;
        retcode = dynamicData->get_boolean_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get boolean value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize boolean value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_BYTE: {
        octet value;
        retcode = dynamicData->get_byte_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get byte value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize byte value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_INT16: {
        int16_t value;
        retcode = dynamicData->get_int16_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get int16 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize int16 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_INT32: {
        long value;
        retcode = dynamicData->get_int32_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get int32 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize int32 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_INT64: {
        long long value;
        retcode = dynamicData->get_int64_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get int64 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize int64 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_UINT16: {
        uint16_t value;
        retcode = dynamicData->get_uint16_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get uint16 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize uint16 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_UINT32: {
        unsigned long value;
        retcode = dynamicData->get_uint32_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get uint32 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize uint32 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_UINT64: {
        unsigned long long value;
        retcode = dynamicData->get_uint64_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get uint64 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize uint64 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_FLOAT32: {
        float value;
        retcode = dynamicData->get_float32_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get float32 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize float32 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_FLOAT64: {
        double value;
        retcode = dynamicData->get_float64_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get float64 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize float64 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_FLOAT128: {
        long double value;
        retcode = dynamicData->get_float128_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get float128 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        LongDouble ldValue;
        ldValue.ld = value;
        if (!cdr->serializeLongDouble(ldValue)) {
            printf("Failed to serialize float128 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_CHAR8: {
        char value;
        retcode = dynamicData->get_char8_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get char8 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize char8 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_CHAR16: {
        wchar_t value;
        retcode = dynamicData->get_char16_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get char16 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize char16 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_STRING8: {
        std::string value;
        retcode = dynamicData->get_string_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get string8 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeString(value)) {
            printf("Failed to serialize string8 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_STRING16: {
        std::wstring value;
        retcode = dynamicData->get_wstring_value(value, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get string16 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeWString(value.c_str(), value.length())) {
            printf("Failed to serialize string16 value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_ENUM: {
        long value;
        retcode = dynamicData->get_int32_value(value, id);
        // TODO - handle bitbound
        if (retcode != RETCODE_OK) {
            printf("Failed to get enum value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        if (!cdr->serializeBaseType(value)) {
            printf("Failed to serialize enum value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        break;
    }
    case TK_BITMASK: {
        printf("Bitmask serialization is not implemented for member %s\n", name.c_str());
        return RETCODE_ERROR;
    }
    case TK_BITSET: {
        printf("Bitset serialization is not implemented for member %s\n", name.c_str());
        return RETCODE_ERROR;
    }
    case TK_ANNOTATION: {
        printf("Annotation serialization is not implemented for member %s\n", name.c_str());
        return RETCODE_ERROR;
    }
    case TK_STRUCTURE: {
        DynamicData memberValue(*type);
        retcode = dynamicData->get_complex_value(memberValue, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get structure value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        int result = this->serialize_data(&memberValue, cdr, endian);
        if (result != RETCODE_OK) {
            printf("Failed to serialize structure value for member %s\n", name.c_str());
            return result;
        }
        break;
    }
    case TK_UNION: {
        DynamicData memberValue(*type);
        retcode = dynamicData->get_complex_value(memberValue, id);
        if (retcode != RETCODE_OK) {
            printf("Failed to get union value for member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        int result = this->serialize_data(&memberValue, cdr, endian);
        if (result != RETCODE_OK) {
            printf("Failed to serialize union value for member %s\n", name.c_str());
            return result;
        }
        break;
    }
    case TK_SEQUENCE:
    case TK_ARRAY: {
        bool isSequence = (type->get_kind() == TK_SEQUENCE);
        DynamicType *eleType = type->descriptor_.element_type_;
        if (!eleType) {
            printf("Element type is not defined for sequence member %s\n", name.c_str());
            return RETCODE_ERROR;
        }
        DynamicData memberValue(*type);
        switch (eleType->get_kind()) {
        case TK_BOOLEAN: {
            DynamicData::BooleanSeq values;
            retcode = dynamicData->get_boolean_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get boolean sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            for (const auto &value : values) {
                if (!cdr->serializeBaseType<bool>(value)) {
                    printf("Failed to serialize boolean sequence for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            break;
        }
        case TK_BYTE: {
            DynamicData::ByteSeq values;
            retcode = dynamicData->get_byte_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get byte sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize byte sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_INT16: {
            DynamicData::Int16Seq values;
            retcode = dynamicData->get_int16_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get int16 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize int16 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_ENUM:
        case TK_INT32: {
            DynamicData::Int32Seq values;
            retcode = dynamicData->get_int32_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get int32 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize int32 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_INT64: {
            DynamicData::Int64Seq values;
            retcode = dynamicData->get_int64_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get int64 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize int64 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_UINT16: {
            DynamicData::UInt16Seq values;
            retcode = dynamicData->get_uint16_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get uint16 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize uint16 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_UINT32: {
            DynamicData::UInt32Seq values;
            retcode = dynamicData->get_uint32_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get uint32 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize uint32 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_UINT64: {
            DynamicData::UInt64Seq values;
            retcode = dynamicData->get_uint64_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get uint64 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize uint64 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_FLOAT32: {
            DynamicData::Float32Seq values;
            retcode = dynamicData->get_float32_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get float32 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize float32 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_FLOAT64: {
            DynamicData::Float64Seq values;
            retcode = dynamicData->get_float64_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get float64 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize float64 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_FLOAT128: {
            DynamicData::Float128Seq values;
            retcode = dynamicData->get_float128_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get float128 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            for (const auto &value : values) {
                LongDouble ldValue;
                ldValue.ld = value;
                if (!cdr->serializeLongDouble(ldValue)) {
                    printf("Failed to serialize float128 sequence for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            break;
        }
        case TK_CHAR8: {
            DynamicData::CharSeq values;
            retcode = dynamicData->get_char8_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get char8 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize char8 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_CHAR16: {
            DynamicData::WcharSeq values;
            retcode = dynamicData->get_char16_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get char16 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            if (!cdr->serializeBaseTypeArray(values.data(), static_cast<uint32_t>(values.size()))) {
                printf("Failed to serialize char16 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        case TK_STRING8: {
            DynamicData::StringSeq values;
            retcode = dynamicData->get_string_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get string8 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            for (const auto &str : values) {
                if (!cdr->serializeString(str)) {
                    printf("Failed to serialize string8 value for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            break;
        }
        case TK_STRING16: {
            DynamicData::WstringSeq values;
            retcode = dynamicData->get_wstring_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get string16 sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            for (const auto &wstr : values) {
                if (!cdr->serializeWString(wstr.c_str(), wstr.length())) {
                    printf("Failed to serialize string16 value for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            break;
        }
        case TK_BITMASK: {
            printf("Bitmask sequence serialization is not implemented for "
                   "member %s\n",
                   name.c_str());
            return RETCODE_ERROR;
        }
        case TK_BITSET: {
            printf("Bitset sequence serialization is not implemented for "
                   "member %s\n",
                   name.c_str());
            return RETCODE_ERROR;
        }
        case TK_ANNOTATION: {
            printf("Annotation sequence serialization is not implemented for "
                   "member %s\n",
                   name.c_str());
            return RETCODE_ERROR;
        }
        case TK_STRUCTURE:
        case TK_UNION: {
            DynamicDataSeq values;
            retcode = dynamicData->get_complex_values(values, id);
            if (retcode != RETCODE_OK) {
                printf("Failed to get structure sequence for member %s\n", name.c_str());
                return RETCODE_ERROR;
            }
            if (isSequence) {
                // 序列化长度
                if (!cdr->serializeBaseType(static_cast<uint32_t>(values.size()))) {
                    printf("Failed to serialize sequence length for member %s\n", name.c_str());
                    return RETCODE_ERROR;
                }
            }
            for (const auto &structValue : values) {
                int result = this->serialize_data(structValue, cdr, endian);
                if (result != RETCODE_OK) {
                    printf("Failed to serialize structure value for member %s\n", name.c_str());
                    return result;
                }
            }
            break;
        }
        default:
            break;
        }
    }
    default:
        break;
    }
    return RETCODE_OK;
}

int DynamicDataTypeSupport::serialize_data(void *data, CdrSerializer *cdr, int endian) {
    if (!data || !cdr) {
        return -1;
    }
    DynamicData *dynamic_data = static_cast<DynamicData *>(data);
    DynamicType &type = dynamic_data->get_type();
    for (uint32_t i = 0; i < type.get_member_count(); ++i) {
        DynamicTypeMember member;
        MemberDescriptor memberDescriptor;
        ReturnCode_t retcode = type.get_member_by_index(member, i);
        if (retcode != RETCODE_OK) {
            printf("Failed to get member by index %u\n", i);
            return RETCODE_ERROR;
        }
        retcode = member.get_descriptor(memberDescriptor);
        if (retcode != RETCODE_OK) {
            printf("Failed to get member descriptor for member %u\n", i);
            return RETCODE_ERROR;
        }
        int ret = serialize_member(dynamic_data, cdr, memberDescriptor.type_, memberDescriptor.id_, memberDescriptor.name_, endian);
        if (ret != RETCODE_OK) {
            printf("Failed to serialize member %s\n", memberDescriptor.name_.c_str());
            return ret;
        }
    }
    return 0;
}

int DynamicDataTypeSupport::deserialize_data(void *data, CdrDeserializer *cdr, int endian) {
    if (!data || !cdr) {
        return RETCODE_BAD_PARAMETER;
    }

    DynamicData *dynamic_data = static_cast<DynamicData *>(data);
    DynamicType &type = dynamic_data->get_type();

    for (uint32_t i = 0; i < type.get_member_count(); ++i) {
        DynamicTypeMember member;
        MemberDescriptor memberDescriptor;
        ReturnCode_t retcode = type.get_member_by_index(member, i);
        if (retcode != RETCODE_OK) {
            printf("Failed to get member by index %u\n", i);
            return RETCODE_ERROR;
        }
        retcode = member.get_descriptor(memberDescriptor);
        if (retcode != RETCODE_OK) {
            printf("Failed to get member descriptor for member %u\n", i);
            return RETCODE_ERROR;
        }

        switch (memberDescriptor.type_->get_kind()) {
        case TK_BOOLEAN: {
            bool value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize boolean value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_boolean_value(memberDescriptor.id_, value);
            break;
        }
        case TK_CHAR8: {
            char value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize char value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_char8_value(memberDescriptor.id_, value);
            break;
        }
        case TK_BYTE: {
            octet value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize byte value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_byte_value(memberDescriptor.id_, value);
            break;
        }
        case TK_CHAR16: {
            wchar_t value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize char16 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_char16_value(memberDescriptor.id_, value);
            break;
        }
        case TK_INT16: {
            int16_t value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize int16 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_int16_value(memberDescriptor.id_, value);
            break;
        }
        case TK_ENUM:
        case TK_INT32: {
            long value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize int32 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_int32_value(memberDescriptor.id_, value);
            break;
        }
        case TK_INT64: {
            long long value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize int64 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_int64_value(memberDescriptor.id_, value);
            break;
        }
        case TK_UINT16: {
            uint16_t value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize uint16 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_uint16_value(memberDescriptor.id_, value);
            break;
        }
        case TK_UINT32: {
            unsigned long value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize uint32 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_uint32_value(memberDescriptor.id_, value);
            break;
        }
        case TK_UINT64: {
            unsigned long long value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize uint64 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_uint64_value(memberDescriptor.id_, value);
            break;
        }
        case TK_FLOAT32: {
            float value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize float32 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_float32_value(memberDescriptor.id_, value);
            break;
        }
        case TK_FLOAT64: {
            double value;
            if (!cdr->deserializeBaseType(value)) {
                printf("Failed to deserialize float64 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_float64_value(memberDescriptor.id_, value);
            break;
        }
        case TK_FLOAT128: {
            LongDouble ldValue;
            if (!cdr->deserializeLongDouble(ldValue)) {
                printf("Failed to deserialize float128 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_float128_value(memberDescriptor.id_, ldValue.ld);
            break;
        }
        case TK_STRING8: {
            std::string value;
            if (!cdr->deserializeString(value)) {
                printf("Failed to deserialize string8 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_string_value(memberDescriptor.id_, value);
            break;
        }
        case TK_STRING16: {
            std::wstring value;
            if (!cdr->deserializeWString(value)) {
                printf("Failed to deserialize string16 value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_wstring_value(memberDescriptor.id_, value);
            break;
        }
        case TK_STRUCTURE:
        case TK_UNION: {
            DynamicData memberValue(*memberDescriptor.type_);
            if (deserialize_data(&memberValue, cdr, endian) != RETCODE_OK) {
                printf("Failed to deserialize structure/union value for member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            dynamic_data->set_complex_value(memberDescriptor.id_, memberValue);
            break;
        }
        case TK_SEQUENCE:
        case TK_ARRAY: {
            bool isSequence = (memberDescriptor.type_->get_kind() == TK_SEQUENCE);
            DynamicType *eleType = memberDescriptor.type_->descriptor_.element_type_;
            if (!eleType) {
                printf("Element type is not defined for sequence/array member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            uint32_t length = 0;
            if (isSequence) {
                if (!cdr->deserializeBaseType(length)) {
                    printf("Failed to deserialize sequence length for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
            } else {
                length = memberDescriptor.type_->descriptor_.get_bound_count();
            }
            switch (eleType->get_kind()) {
            case TK_BOOLEAN: {
                DynamicData::BooleanSeq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize boolean sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_boolean_values(memberDescriptor.id_, values);
                break;
            }
            case TK_CHAR8: {
                DynamicData::CharSeq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize char8 sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_char8_values(memberDescriptor.id_, values);
                break;
            }
            case TK_CHAR16: {
                DynamicData::WcharSeq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize char16 sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_char16_values(memberDescriptor.id_, values);
                break;
            }
            case TK_BYTE: {
                DynamicData::ByteSeq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize byte sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_byte_values(memberDescriptor.id_, values);
                break;
            }
            case TK_INT16: {
                DynamicData::Int16Seq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize int16 sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_int16_values(memberDescriptor.id_, values);
                break;
            }
            case TK_ENUM:
            case TK_INT32: {
                DynamicData::Int32Seq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize int32 sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_int32_values(memberDescriptor.id_, values);
                break;
            }
            case TK_INT64: {
                DynamicData::Int64Seq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize int64 sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_int64_values(memberDescriptor.id_, values);
                break;
            }
            case TK_UINT16: {
                DynamicData::UInt16Seq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize uint16 sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_uint16_values(memberDescriptor.id_, values);
                break;
            }
            case TK_UINT32: {
                DynamicData::UInt32Seq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize uint32 sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_uint32_values(memberDescriptor.id_, values);
                break;
            }
            case TK_UINT64: {
                DynamicData::UInt64Seq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize uint64 sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_uint64_values(memberDescriptor.id_, values);
                break;
            }
            case TK_FLOAT32: {
                DynamicData::Float32Seq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize float32 sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_float32_values(memberDescriptor.id_, values);
                break;
            }
            case TK_FLOAT64: {
                DynamicData::Float64Seq values(length);
                if (!cdr->deserializeBaseTypeArray(values.data(), length)) {
                    printf("Failed to deserialize float64 sequence for member %s\n", memberDescriptor.name_.c_str());
                    return RETCODE_ERROR;
                }
                dynamic_data->set_float64_values(memberDescriptor.id_, values);
                break;
            }
            case TK_FLOAT128: {
                DynamicData::Float128Seq values(length);
                for (uint32_t j = 0; j < length; ++j) {
                    LongDouble ldValue;
                    if (!cdr->deserializeLongDouble(ldValue)) {
                        printf("Failed to deserialize float128 sequence for member %s\n", memberDescriptor.name_.c_str());
                        return RETCODE_ERROR;
                    }
                    values[j] = ldValue.ld;
                }
                dynamic_data->set_float128_values(memberDescriptor.id_, values);
                break;
            }
            case TK_STRING8: {
                DynamicData::StringSeq values(length);
                for (uint32_t j = 0; j < length; ++j) {
                    if (!cdr->deserializeString(values[j])) {
                        printf("Failed to deserialize string8 sequence for member %s\n", memberDescriptor.name_.c_str());
                        return RETCODE_ERROR;
                    }
                }
                dynamic_data->set_string_values(memberDescriptor.id_, values);
                break;
            }
            case TK_STRING16: {
                DynamicData::WstringSeq values(length);
                for (uint32_t j = 0; j < length; ++j) {
                    if (!cdr->deserializeWString(values[j])) {
                        printf("Failed to deserialize string16 sequence for member %s\n", memberDescriptor.name_.c_str());
                        return RETCODE_ERROR;
                    }
                }
                dynamic_data->set_wstring_values(memberDescriptor.id_, values);
                break;
            }
            case TK_STRUCTURE:
            case TK_UNION: {
                DynamicDataSeq values(length);
                for (uint32_t j = 0; j < length; ++j) {
                    values[j] = new DynamicData(*eleType);
                    if (deserialize_data(values[j], cdr, endian) != RETCODE_OK) {
                        printf("Failed to deserialize structure/union sequence for member %s\n", memberDescriptor.name_.c_str());
                        return RETCODE_ERROR;
                    }
                }
                dynamic_data->set_complex_values(memberDescriptor.id_, values);
                break;
            }
            default:
                printf("Unsupported element type for sequence/array member %s\n", memberDescriptor.name_.c_str());
                return RETCODE_ERROR;
            }
            break;
        }
        default:
            printf("Unsupported type kind for member %s\n", memberDescriptor.name_.c_str());
            return RETCODE_ERROR;
        }
    }

    return RETCODE_OK;
}

TypeObject *DynamicDataTypeSupport::get_typeobject() {
    // Placeholder implementation; actual logic should return the type object.
    return nullptr;
}

ReturnCode_t DynamicDataTypeSupport::get_instancehandle(void *data, CdrSerializer *cdr, InstanceHandle_t &iHandle,
                                                                bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char *)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool DynamicDataTypeSupport::has_key() {
    if (!type_) {
        return false;
    }
    for (uint32_t i = 0; i < type_->get_member_count(); ++i) {
        DynamicTypeMember member;
        ReturnCode_t retcode = type_->get_member_by_index(member, i);
        if (retcode != RETCODE_OK) {
            printf("Failed to get member by index %u\n", i);
            return false;
        }
        if (member.is_key()) {
            return true;
        }
    }
    return false;
}

const char *DynamicDataTypeSupport::get_typename() { return type_ ? type_->get_name().c_str() : nullptr; }

int DynamicDataTypeSupport::MakeKey(const void *data, InstanceHandle_t &iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void *)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void *)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

int DynamicDataTypeSupport::serialize_key(void *data, CdrSerializer *cdr, int endian) {
    if (!data || !cdr) {
        return -1;
    }
    DynamicData *dynamic_data = static_cast<DynamicData *>(data);
    DynamicType &type = dynamic_data->get_type();
    for (uint32_t i = 0; i < type.get_member_count(); ++i) {
        DynamicTypeMember member;
        MemberDescriptor memberDescriptor;
        ReturnCode_t retcode = type.get_member_by_index(member, i);
        if (retcode != RETCODE_OK) {
            printf("Failed to get member by index %u\n", i);
            return RETCODE_ERROR;
        }
        retcode = member.get_descriptor(memberDescriptor);
        if (retcode != RETCODE_OK) {
            printf("Failed to get member descriptor for member %u\n", i);
            return RETCODE_ERROR;
        }
        // Only serialize key members
        if (!member.is_key()) {
            continue;
        }
        int ret = serialize_member(dynamic_data, cdr, memberDescriptor.type_, memberDescriptor.id_, memberDescriptor.name_, endian);
        if (ret != RETCODE_OK) {
            printf("Failed to serialize member %s\n", memberDescriptor.name_.c_str());
            return ret;
        }
    }
    return 0;
}
