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

#include "xtypes/typeobjecttypesupport.h"

#ifdef __cplusplus
extern "C" {
#endif

 USING_TRAVODDS_NAMESPACE

_TypeIdTypeSupport* _TypeIdTypeSupport::get_instance() {
    static _TypeIdTypeSupport instance;
    return &instance;
}

void* _TypeIdTypeSupport::create_data() {
    return new _TypeId();
}

void _TypeIdTypeSupport::delete_data(void* data) {
    delete static_cast<_TypeId*>(data);
}

int _TypeIdTypeSupport::copy_data(void* dst, void* src) {
    _TypeId* dstData = static_cast<_TypeId*>(dst);
    _TypeId* srcData = static_cast<_TypeId*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int _TypeIdTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    _TypeId* structData = static_cast<_TypeId*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
    switch(structData->u_) {
    case BOOLEAN_TYPE:
    case BYTE_TYPE:
    case INT_16_TYPE:
    case UINT_16_TYPE:
    case INT_32_TYPE:
    case UINT_32_TYPE:
    case INT_64_TYPE:
    case UINT_64_TYPE:
    case FLOAT_32_TYPE:
    case FLOAT_64_TYPE:
    case FLOAT_128_TYPE:
    case CHAR_8_TYPE:
    case CHAR_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
        break;
    default:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
        break;
    }
    return 0;
}

unsigned int _TypeIdTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    _TypeId* structData = static_cast<_TypeId*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
    switch(structData->u_) {
    case BOOLEAN_TYPE:
    case BYTE_TYPE:
    case INT_16_TYPE:
    case UINT_16_TYPE:
    case INT_32_TYPE:
    case UINT_32_TYPE:
    case INT_64_TYPE:
    case UINT_64_TYPE:
    case FLOAT_32_TYPE:
    case FLOAT_64_TYPE:
    case FLOAT_128_TYPE:
    case CHAR_8_TYPE:
    case CHAR_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
        break;
    default:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
        break;
    }
    return 0;
}

int _TypeIdTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    _TypeId* structData = static_cast<_TypeId*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->u_)) {
        fprintf(stderr, "Serialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case BOOLEAN_TYPE:
    case BYTE_TYPE:
    case INT_16_TYPE:
    case UINT_16_TYPE:
    case INT_32_TYPE:
    case UINT_32_TYPE:
    case INT_64_TYPE:
    case UINT_64_TYPE:
    case FLOAT_32_TYPE:
    case FLOAT_64_TYPE:
    case FLOAT_128_TYPE:
    case CHAR_8_TYPE:
    case CHAR_32_TYPE:
        if (!cdr->serializeBaseType(structData->primitive_type_id)) {
            fprintf(stderr, "Serialization failed for field: structData->primitive_type_id\n");
            return -1;
        }
        break;
    default:
        if (!cdr->serializeBaseType(structData->constructed_type_id)) {
            fprintf(stderr, "Serialization failed for field: structData->constructed_type_id\n");
            return -1;
        }
        break;
    }
    return 0;
}

int _TypeIdTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    _TypeId* structData = static_cast<_TypeId*>(data);
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->u_)) {
        fprintf(stderr, "Deserialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case BOOLEAN_TYPE:
    case BYTE_TYPE:
    case INT_16_TYPE:
    case UINT_16_TYPE:
    case INT_32_TYPE:
    case UINT_32_TYPE:
    case INT_64_TYPE:
    case UINT_64_TYPE:
    case FLOAT_32_TYPE:
    case FLOAT_64_TYPE:
    case FLOAT_128_TYPE:
    case CHAR_8_TYPE:
    case CHAR_32_TYPE:
        if (!cdr->deserializeBaseType(structData->primitive_type_id)) {
            fprintf(stderr, "Deserialization failed for field: structData->primitive_type_id\n");
            return -1;
        }
        break;
    default:
        if (!cdr->deserializeBaseType(structData->constructed_type_id)) {
            fprintf(stderr, "Deserialization failed for field: structData->constructed_type_id\n");
            return -1;
        }
        break;
    }
    return 0;
}

TypeObject* _TypeIdTypeSupport::get_typeobject() {
    return nullptr;
}

int _TypeIdTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    _TypeId* structData = static_cast<_TypeId*>(data);
    return 0;
}

int _TypeIdTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t _TypeIdTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool _TypeIdTypeSupport::has_key() {
    return false;
}

const char* _TypeIdTypeSupport::get_typename() {
    return "_TypeId";
}

AnnotationMemberValueTypeSupport* AnnotationMemberValueTypeSupport::get_instance() {
    static AnnotationMemberValueTypeSupport instance;
    return &instance;
}

void* AnnotationMemberValueTypeSupport::create_data() {
    return new AnnotationMemberValue();
}

void AnnotationMemberValueTypeSupport::delete_data(void* data) {
    delete static_cast<AnnotationMemberValue*>(data);
}

int AnnotationMemberValueTypeSupport::copy_data(void* dst, void* src) {
    AnnotationMemberValue* dstData = static_cast<AnnotationMemberValue*>(dst);
    AnnotationMemberValue* srcData = static_cast<AnnotationMemberValue*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AnnotationMemberValueTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AnnotationMemberValue* structData = static_cast<AnnotationMemberValue*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
    switch(structData->u_) {
    case BOOLEAN_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
        break;
    case BYTE_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned char), currentAlignment);
        break;
    case INT_16_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
        break;
    case UINT_16_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned short), currentAlignment);
        break;
    case INT_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
        break;
    case UINT_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
        break;
    case INT_64_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(long long), currentAlignment);
        break;
    case UINT_64_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
        break;
    case FLOAT_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(float), currentAlignment);
        break;
    case FLOAT_64_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(double), currentAlignment);
        break;
    case FLOAT_128_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(LongDouble), currentAlignment);
        break;
    case CHAR_8_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
        break;
    case CHAR_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(wchar_t), currentAlignment);
        break;
    case ENUMERATION_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
        break;
    case STRING_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->string_value.size() + 1, currentAlignment);
        break;
    }
    return 0;
}

unsigned int AnnotationMemberValueTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AnnotationMemberValueTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AnnotationMemberValue* structData = static_cast<AnnotationMemberValue*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->u_)) {
        fprintf(stderr, "Serialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case BOOLEAN_TYPE:
        if (!cdr->serializeBaseType(structData->boolean_value)) {
            fprintf(stderr, "Serialization failed for field: structData->boolean_value\n");
            return -1;
        }
        break;
    case BYTE_TYPE:
        if (!cdr->serializeBaseType(structData->byte_value)) {
            fprintf(stderr, "Serialization failed for field: structData->byte_value\n");
            return -1;
        }
        break;
    case INT_16_TYPE:
        if (!cdr->serializeBaseType(structData->int_16_value)) {
            fprintf(stderr, "Serialization failed for field: structData->int_16_value\n");
            return -1;
        }
        break;
    case UINT_16_TYPE:
        if (!cdr->serializeBaseType(structData->uint_16_value)) {
            fprintf(stderr, "Serialization failed for field: structData->uint_16_value\n");
            return -1;
        }
        break;
    case INT_32_TYPE:
        if (!cdr->serializeBaseType(structData->int_32_value)) {
            fprintf(stderr, "Serialization failed for field: structData->int_32_value\n");
            return -1;
        }
        break;
    case UINT_32_TYPE:
        if (!cdr->serializeBaseType(structData->uint_32_value)) {
            fprintf(stderr, "Serialization failed for field: structData->uint_32_value\n");
            return -1;
        }
        break;
    case INT_64_TYPE:
        if (!cdr->serializeBaseType(structData->int_64_value)) {
            fprintf(stderr, "Serialization failed for field: structData->int_64_value\n");
            return -1;
        }
        break;
    case UINT_64_TYPE:
        if (!cdr->serializeBaseType(structData->uint_64_value)) {
            fprintf(stderr, "Serialization failed for field: structData->uint_64_value\n");
            return -1;
        }
        break;
    case FLOAT_32_TYPE:
        if (!cdr->serializeBaseType(structData->float_32_value)) {
            fprintf(stderr, "Serialization failed for field: structData->float_32_value\n");
            return -1;
        }
        break;
    case FLOAT_64_TYPE:
        if (!cdr->serializeBaseType(structData->float_64_value)) {
            fprintf(stderr, "Serialization failed for field: structData->float_64_value\n");
            return -1;
        }
        break;
    case FLOAT_128_TYPE:
        if (!cdr->serializeLongDouble(*(LongDouble*)&structData->float_128_value)) {
            fprintf(stderr, "Serialization failed for field: structData->float_128_value\n");
            return -1;
        }
        break;
    case CHAR_8_TYPE:
        if (!cdr->serializeBaseType(structData->character_value)) {
            fprintf(stderr, "Serialization failed for field: structData->character_value\n");
            return -1;
        }
        break;
    case CHAR_32_TYPE:
        if (!cdr->serializeBaseType(structData->wide_character_value)) {
            fprintf(stderr, "Serialization failed for field: structData->wide_character_value\n");
            return -1;
        }
        break;
    case ENUMERATION_TYPE:
        if (!cdr->serializeBaseType(structData->enumeration_value)) {
            fprintf(stderr, "Serialization failed for field: structData->enumeration_value\n");
            return -1;
        }
        break;
    case STRING_TYPE:
        if (!cdr->serializeString(structData->string_value)) {
            fprintf(stderr, "Serialization failed for field: structData->string_value\n");
            return -1;
        }
        break;
    }
    return 0;
}

int AnnotationMemberValueTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AnnotationMemberValue* structData = static_cast<AnnotationMemberValue*>(data);
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->u_)) {
        fprintf(stderr, "Deserialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case BOOLEAN_TYPE:
        if (!cdr->deserializeBaseType(structData->boolean_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->boolean_value\n");
            return -1;
        }
        break;
    case BYTE_TYPE:
        if (!cdr->deserializeBaseType(structData->byte_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->byte_value\n");
            return -1;
        }
        break;
    case INT_16_TYPE:
        if (!cdr->deserializeBaseType(structData->int_16_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->int_16_value\n");
            return -1;
        }
        break;
    case UINT_16_TYPE:
        if (!cdr->deserializeBaseType(structData->uint_16_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->uint_16_value\n");
            return -1;
        }
        break;
    case INT_32_TYPE:
        if (!cdr->deserializeBaseType(structData->int_32_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->int_32_value\n");
            return -1;
        }
        break;
    case UINT_32_TYPE:
        if (!cdr->deserializeBaseType(structData->uint_32_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->uint_32_value\n");
            return -1;
        }
        break;
    case INT_64_TYPE:
        if (!cdr->deserializeBaseType(structData->int_64_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->int_64_value\n");
            return -1;
        }
        break;
    case UINT_64_TYPE:
        if (!cdr->deserializeBaseType(structData->uint_64_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->uint_64_value\n");
            return -1;
        }
        break;
    case FLOAT_32_TYPE:
        if (!cdr->deserializeBaseType(structData->float_32_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->float_32_value\n");
            return -1;
        }
        break;
    case FLOAT_64_TYPE:
        if (!cdr->deserializeBaseType(structData->float_64_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->float_64_value\n");
            return -1;
        }
        break;
    case FLOAT_128_TYPE:
        if (!cdr->deserializeLongDouble(*(LongDouble*)&structData->float_128_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->float_128_value\n");
            return -1;
        }
        break;
    case CHAR_8_TYPE:
        if (!cdr->deserializeBaseType(structData->character_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->character_value\n");
            return -1;
        }
        break;
    case CHAR_32_TYPE:
        if (!cdr->deserializeBaseType(structData->wide_character_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->wide_character_value\n");
            return -1;
        }
        break;
    case ENUMERATION_TYPE:
        if (!cdr->deserializeBaseType(structData->enumeration_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->enumeration_value\n");
            return -1;
        }
        break;
    case STRING_TYPE:
        if (!cdr->deserializeString(structData->string_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->string_value\n");
            return -1;
        }
        break;
    }
    return 0;
}

TypeObject* AnnotationMemberValueTypeSupport::get_typeobject() {
    return nullptr;
}

int AnnotationMemberValueTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AnnotationMemberValue* structData = static_cast<AnnotationMemberValue*>(data);
    return 0;
}

int AnnotationMemberValueTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AnnotationMemberValueTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool AnnotationMemberValueTypeSupport::has_key() {
    return false;
}

const char* AnnotationMemberValueTypeSupport::get_typename() {
    return "AnnotationMemberValue";
}

AnnotationUsageMemberTypeSupport* AnnotationUsageMemberTypeSupport::get_instance() {
    static AnnotationUsageMemberTypeSupport instance;
    return &instance;
}

void* AnnotationUsageMemberTypeSupport::create_data() {
    return new AnnotationUsageMember();
}

void AnnotationUsageMemberTypeSupport::delete_data(void* data) {
    delete static_cast<AnnotationUsageMember*>(data);
}

int AnnotationUsageMemberTypeSupport::copy_data(void* dst, void* src) {
    AnnotationUsageMember* dstData = static_cast<AnnotationUsageMember*>(dst);
    AnnotationUsageMember* srcData = static_cast<AnnotationUsageMember*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AnnotationUsageMemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AnnotationUsageMember* structData = static_cast<AnnotationUsageMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += AnnotationMemberValueTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->value, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int AnnotationUsageMemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AnnotationUsageMember* structData = static_cast<AnnotationUsageMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += AnnotationMemberValueTypeSupport::get_instance()->get_max_serialized_data_size(&structData->value, currentAlignment);
    return currentAlignment - initialAlignment;
}

int AnnotationUsageMemberTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AnnotationUsageMember* structData = static_cast<AnnotationUsageMember*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->member_id)) {
        fprintf(stderr, "Serialization failed for field: structData->member_id\n");
        return -1;
    }
    if (AnnotationMemberValueTypeSupport::get_instance()->serialize_data((void*)&structData->value, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

int AnnotationUsageMemberTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AnnotationUsageMember* structData = static_cast<AnnotationUsageMember*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->member_id)) {
        fprintf(stderr, "Deserialization failed for field: structData->member_id\n");
        return -1;
    }
    if (AnnotationMemberValueTypeSupport::get_instance()->deserialize_data(&structData->value, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

TypeObject* AnnotationUsageMemberTypeSupport::get_typeobject() {
    return nullptr;
}

int AnnotationUsageMemberTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AnnotationUsageMember* structData = static_cast<AnnotationUsageMember*>(data);
    bool memberHasKey = false;
    return 0;
}

int AnnotationUsageMemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AnnotationUsageMemberTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool AnnotationUsageMemberTypeSupport::has_key() {
    return false;
}

const char* AnnotationUsageMemberTypeSupport::get_typename() {
    return "AnnotationUsageMember";
}

AnnotationUsageTypeSupport* AnnotationUsageTypeSupport::get_instance() {
    static AnnotationUsageTypeSupport instance;
    return &instance;
}

void* AnnotationUsageTypeSupport::create_data() {
    return new AnnotationUsage();
}

void AnnotationUsageTypeSupport::delete_data(void* data) {
    delete static_cast<AnnotationUsage*>(data);
}

int AnnotationUsageTypeSupport::copy_data(void* dst, void* src) {
    AnnotationUsage* dstData = static_cast<AnnotationUsage*>(dst);
    AnnotationUsage* srcData = static_cast<AnnotationUsage*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AnnotationUsageTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AnnotationUsage* structData = static_cast<AnnotationUsage*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->type_id, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += AnnotationUsageMemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    return currentAlignment - initialAlignment;
}

unsigned int AnnotationUsageTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AnnotationUsageTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AnnotationUsage* structData = static_cast<AnnotationUsage*>(data);
    uint32_t tmpLength = 0;
    if (_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->type_id\n");
        return -1;
    }
    tmpLength = static_cast<uint32_t>(structData->member.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->member\n");
        return -1;
    }
    for (size_t i = 0; i < structData->member.size(); ++i) {
        if (AnnotationUsageMemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    return 0;
}

int AnnotationUsageTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AnnotationUsage* structData = static_cast<AnnotationUsage*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->member\n");
        return -1;
    }
    structData->member.resize(tmpLength);
    for (size_t i = 0; i < tmpLength; ++i) {
        if (AnnotationUsageMemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    return 0;
}

TypeObject* AnnotationUsageTypeSupport::get_typeobject() {
    return nullptr;
}

int AnnotationUsageTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AnnotationUsage* structData = static_cast<AnnotationUsage*>(data);
    bool memberHasKey = false;
    return 0;
}

int AnnotationUsageTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AnnotationUsageTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool AnnotationUsageTypeSupport::has_key() {
    return false;
}

const char* AnnotationUsageTypeSupport::get_typename() {
    return "AnnotationUsage";
}

TypePropertyTypeSupport* TypePropertyTypeSupport::get_instance() {
    static TypePropertyTypeSupport instance;
    return &instance;
}

void* TypePropertyTypeSupport::create_data() {
    return new TypeProperty();
}

void TypePropertyTypeSupport::delete_data(void* data) {
    delete static_cast<TypeProperty*>(data);
}

int TypePropertyTypeSupport::copy_data(void* dst, void* src) {
    TypeProperty* dstData = static_cast<TypeProperty*>(dst);
    TypeProperty* srcData = static_cast<TypeProperty*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TypePropertyTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TypeProperty* structData = static_cast<TypeProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->type_id, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TypePropertyTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TypeProperty* structData = static_cast<TypeProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->type_id, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int TypePropertyTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    TypeProperty* structData = static_cast<TypeProperty*>(data);
    uint32_t tmpLength = 0;
    switch(16 / 8) {
    case 1:
        if (!cdr->serializeBaseType(static_cast<char>(structData->flag))) {
            fprintf(stderr, "Serialization failed for field: structData->flag\n");
            return -1;
        }
        break;
    case 2:
        if (!cdr->serializeBaseType(static_cast<short>(structData->flag))) {
            fprintf(stderr, "Serialization failed for field: structData->flag\n");
            return -1;
        }
        break;
    case 4:
        if (!cdr->serializeBaseType(static_cast<int>(structData->flag))) {
            fprintf(stderr, "Serialization failed for field: structData->flag\n");
            return -1;
        }
        break;
    default:
        fprintf(stderr, "Serialization failed for field: structData->flag\n");
        return -1;
        break;
    }
    if (_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int TypePropertyTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    TypeProperty* structData = static_cast<TypeProperty*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    switch(16 / 8) {
    case 1:
        if (!cdr->deserializeBaseType(tmpCharEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<TypeFlag>(tmpCharEnum);
        break;
    case 2:
        if (!cdr->deserializeBaseType(tmpShortEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<TypeFlag>(tmpShortEnum);
        break;
    case 4:
        if (!cdr->deserializeBaseType(tmpIntEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<TypeFlag>(tmpIntEnum);
        break;
    default:
        fprintf(stderr, "Deserialization failed for field: structData->flag\n");
        return -1;
        break;
    }
    if (_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* TypePropertyTypeSupport::get_typeobject() {
    return nullptr;
}

int TypePropertyTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    TypeProperty* structData = static_cast<TypeProperty*>(data);
    bool memberHasKey = false;
    return 0;
}

int TypePropertyTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TypePropertyTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool TypePropertyTypeSupport::has_key() {
    return false;
}

const char* TypePropertyTypeSupport::get_typename() {
    return "TypeProperty";
}

TypeTypeSupport* TypeTypeSupport::get_instance() {
    static TypeTypeSupport instance;
    return &instance;
}

void* TypeTypeSupport::create_data() {
    return new Type();
}

void TypeTypeSupport::delete_data(void* data) {
    delete static_cast<Type*>(data);
}

int TypeTypeSupport::copy_data(void* dst, void* src) {
    Type* dstData = static_cast<Type*>(dst);
    Type* srcData = static_cast<Type*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    Type* structData = static_cast<Type*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TypePropertyTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->property, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->annotation.size(); ++i) {
        currentAlignment += AnnotationUsageTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->annotation[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int TypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    Type* structData = static_cast<Type*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, PROPERTY_TYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (TypePropertyTypeSupport::get_instance()->serialize_data((void*)&structData->property, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, ANNOTATION_TYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->annotation\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    tmpLength = static_cast<uint32_t>(structData->annotation.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->annotation\n");
        return -1;
    }
    for (size_t i = 0; i < structData->annotation.size(); ++i) {
        if (AnnotationUsageTypeSupport::get_instance()->serialize_data((void*)&structData->annotation[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->annotation[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    Type* structData = static_cast<Type*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: Type\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case PROPERTY_TYPE_MEMBER_ID:
            if (TypePropertyTypeSupport::get_instance()->deserialize_data(&structData->property, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->property\n");
                return -1;
            }
            break;
        case ANNOTATION_TYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->annotation\n");
                return -1;
            }
            structData->annotation.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (AnnotationUsageTypeSupport::get_instance()->deserialize_data(&structData->annotation[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->annotation[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: Type\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* TypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    Type* structData = static_cast<Type*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    return 0;
}

int TypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool TypeTypeSupport::has_key() {
    return false;
}

const char* TypeTypeSupport::get_typename() {
    return "Type";
}

MemberPropertyTypeSupport* MemberPropertyTypeSupport::get_instance() {
    static MemberPropertyTypeSupport instance;
    return &instance;
}

void* MemberPropertyTypeSupport::create_data() {
    return new MemberProperty();
}

void MemberPropertyTypeSupport::delete_data(void* data) {
    delete static_cast<MemberProperty*>(data);
}

int MemberPropertyTypeSupport::copy_data(void* dst, void* src) {
    MemberProperty* dstData = static_cast<MemberProperty*>(dst);
    MemberProperty* srcData = static_cast<MemberProperty*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int MemberPropertyTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    MemberProperty* structData = static_cast<MemberProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->type_id, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int MemberPropertyTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    MemberProperty* structData = static_cast<MemberProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->type_id, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int MemberPropertyTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    MemberProperty* structData = static_cast<MemberProperty*>(data);
    uint32_t tmpLength = 0;
    switch(16 / 8) {
    case 1:
        if (!cdr->serializeBaseType(static_cast<char>(structData->flag))) {
            fprintf(stderr, "Serialization failed for field: structData->flag\n");
            return -1;
        }
        break;
    case 2:
        if (!cdr->serializeBaseType(static_cast<short>(structData->flag))) {
            fprintf(stderr, "Serialization failed for field: structData->flag\n");
            return -1;
        }
        break;
    case 4:
        if (!cdr->serializeBaseType(static_cast<int>(structData->flag))) {
            fprintf(stderr, "Serialization failed for field: structData->flag\n");
            return -1;
        }
        break;
    default:
        fprintf(stderr, "Serialization failed for field: structData->flag\n");
        return -1;
        break;
    }
    if (!cdr->serializeBaseType(structData->member_id)) {
        fprintf(stderr, "Serialization failed for field: structData->member_id\n");
        return -1;
    }
    if (_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int MemberPropertyTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    MemberProperty* structData = static_cast<MemberProperty*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    switch(16 / 8) {
    case 1:
        if (!cdr->deserializeBaseType(tmpCharEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<MemberFlag>(tmpCharEnum);
        break;
    case 2:
        if (!cdr->deserializeBaseType(tmpShortEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<MemberFlag>(tmpShortEnum);
        break;
    case 4:
        if (!cdr->deserializeBaseType(tmpIntEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<MemberFlag>(tmpIntEnum);
        break;
    default:
        fprintf(stderr, "Deserialization failed for field: structData->flag\n");
        return -1;
        break;
    }
    if (!cdr->deserializeBaseType(structData->member_id)) {
        fprintf(stderr, "Deserialization failed for field: structData->member_id\n");
        return -1;
    }
    if (_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* MemberPropertyTypeSupport::get_typeobject() {
    return nullptr;
}

int MemberPropertyTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    MemberProperty* structData = static_cast<MemberProperty*>(data);
    bool memberHasKey = false;
    return 0;
}

int MemberPropertyTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t MemberPropertyTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool MemberPropertyTypeSupport::has_key() {
    return false;
}

const char* MemberPropertyTypeSupport::get_typename() {
    return "MemberProperty";
}

MemberTypeSupport* MemberTypeSupport::get_instance() {
    static MemberTypeSupport instance;
    return &instance;
}

void* MemberTypeSupport::create_data() {
    return new Member();
}

void MemberTypeSupport::delete_data(void* data) {
    delete static_cast<Member*>(data);
}

int MemberTypeSupport::copy_data(void* dst, void* src) {
    Member* dstData = static_cast<Member*>(dst);
    Member* srcData = static_cast<Member*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int MemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    Member* structData = static_cast<Member*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += MemberPropertyTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->property, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->annotation.size(); ++i) {
        currentAlignment += AnnotationUsageTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->annotation[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int MemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int MemberTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    Member* structData = static_cast<Member*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, PROPERTY_MEMBER_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (MemberPropertyTypeSupport::get_instance()->serialize_data((void*)&structData->property, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, ANNOTATION_MEMBER_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->annotation\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    tmpLength = static_cast<uint32_t>(structData->annotation.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->annotation\n");
        return -1;
    }
    for (size_t i = 0; i < structData->annotation.size(); ++i) {
        if (AnnotationUsageTypeSupport::get_instance()->serialize_data((void*)&structData->annotation[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->annotation[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int MemberTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    Member* structData = static_cast<Member*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: Member\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case PROPERTY_MEMBER_MEMBER_ID:
            if (MemberPropertyTypeSupport::get_instance()->deserialize_data(&structData->property, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->property\n");
                return -1;
            }
            break;
        case ANNOTATION_MEMBER_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->annotation\n");
                return -1;
            }
            structData->annotation.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (AnnotationUsageTypeSupport::get_instance()->deserialize_data(&structData->annotation[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->annotation[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: Member\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* MemberTypeSupport::get_typeobject() {
    return nullptr;
}

int MemberTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    Member* structData = static_cast<Member*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    return 0;
}

int MemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t MemberTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool MemberTypeSupport::has_key() {
    return false;
}

const char* MemberTypeSupport::get_typename() {
    return "Member";
}

StructureTypeTypeSupport* StructureTypeTypeSupport::get_instance() {
    static StructureTypeTypeSupport instance;
    return &instance;
}

void* StructureTypeTypeSupport::create_data() {
    return new StructureType();
}

void StructureTypeTypeSupport::delete_data(void* data) {
    delete static_cast<StructureType*>(data);
}

int StructureTypeTypeSupport::copy_data(void* dst, void* src) {
    StructureType* dstData = static_cast<StructureType*>(dst);
    StructureType* srcData = static_cast<StructureType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int StructureTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    StructureType* structData = static_cast<StructureType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += MemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int StructureTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int StructureTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    StructureType* structData = static_cast<StructureType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->annotation[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, BASE_TYPE_STRUCTURETYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->base_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, MEMBER_STRUCTURETYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->member\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    tmpLength = static_cast<uint32_t>(structData->member.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->member\n");
        return -1;
    }
    for (size_t i = 0; i < structData->member.size(); ++i) {
        if (MemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int StructureTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    StructureType* structData = static_cast<StructureType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->annotation[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: StructureType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case BASE_TYPE_STRUCTURETYPE_MEMBER_ID:
            if (_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->base_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
                return -1;
            }
            break;
        case MEMBER_STRUCTURETYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->member\n");
                return -1;
            }
            structData->member.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (MemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: StructureType\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* StructureTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int StructureTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    StructureType* structData = static_cast<StructureType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int StructureTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t StructureTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool StructureTypeTypeSupport::has_key() {
    return false;
}

const char* StructureTypeTypeSupport::get_typename() {
    return "StructureType";
}

UnionMemberTypeSupport* UnionMemberTypeSupport::get_instance() {
    static UnionMemberTypeSupport instance;
    return &instance;
}

void* UnionMemberTypeSupport::create_data() {
    return new UnionMember();
}

void UnionMemberTypeSupport::delete_data(void* data) {
    delete static_cast<UnionMember*>(data);
}

int UnionMemberTypeSupport::copy_data(void* dst, void* src) {
    UnionMember* dstData = static_cast<UnionMember*>(dst);
    UnionMember* srcData = static_cast<UnionMember*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int UnionMemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    UnionMember* structData = static_cast<UnionMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += MemberTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<Member*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeArraySize(sizeof(int), structData->label.size(), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int UnionMemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int UnionMemberTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    UnionMember* structData = static_cast<UnionMember*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, LABEL_UNIONMEMBER_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->label\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    tmpLength = static_cast<uint32_t>(structData->label.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->label\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray(structData->label.data(), structData->label.size())) {
        fprintf(stderr, "Serialization failed for field: structData->label\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int UnionMemberTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    UnionMember* structData = static_cast<UnionMember*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (MemberTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: UnionMember\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case LABEL_UNIONMEMBER_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->label\n");
                return -1;
            }
            structData->label.resize(tmpLength);
            if (!cdr->deserializeBaseTypeArray(structData->label.data(), tmpLength)) {
                fprintf(stderr, "Deserialization failed for field: structData->label\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: UnionMember\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* UnionMemberTypeSupport::get_typeobject() {
    return nullptr;
}

int UnionMemberTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    UnionMember* structData = static_cast<UnionMember*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int UnionMemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t UnionMemberTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool UnionMemberTypeSupport::has_key() {
    return false;
}

const char* UnionMemberTypeSupport::get_typename() {
    return "UnionMember";
}

UnionTypeTypeSupport* UnionTypeTypeSupport::get_instance() {
    static UnionTypeTypeSupport instance;
    return &instance;
}

void* UnionTypeTypeSupport::create_data() {
    return new UnionType();
}

void UnionTypeTypeSupport::delete_data(void* data) {
    delete static_cast<UnionType*>(data);
}

int UnionTypeTypeSupport::copy_data(void* dst, void* src) {
    UnionType* dstData = static_cast<UnionType*>(dst);
    UnionType* srcData = static_cast<UnionType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int UnionTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    UnionType* structData = static_cast<UnionType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += UnionMemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int UnionTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int UnionTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    UnionType* structData = static_cast<UnionType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->label\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, MEMBER_UNIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->member\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    tmpLength = static_cast<uint32_t>(structData->member.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->member\n");
        return -1;
    }
    for (size_t i = 0; i < structData->member.size(); ++i) {
        if (UnionMemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int UnionTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    UnionType* structData = static_cast<UnionType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->label\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: UnionType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case MEMBER_UNIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->member\n");
                return -1;
            }
            structData->member.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (UnionMemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: UnionType\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* UnionTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int UnionTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    UnionType* structData = static_cast<UnionType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int UnionTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t UnionTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool UnionTypeTypeSupport::has_key() {
    return false;
}

const char* UnionTypeTypeSupport::get_typename() {
    return "UnionType";
}

AnnotationMemberTypeSupport* AnnotationMemberTypeSupport::get_instance() {
    static AnnotationMemberTypeSupport instance;
    return &instance;
}

void* AnnotationMemberTypeSupport::create_data() {
    return new AnnotationMember();
}

void AnnotationMemberTypeSupport::delete_data(void* data) {
    delete static_cast<AnnotationMember*>(data);
}

int AnnotationMemberTypeSupport::copy_data(void* dst, void* src) {
    AnnotationMember* dstData = static_cast<AnnotationMember*>(dst);
    AnnotationMember* srcData = static_cast<AnnotationMember*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AnnotationMemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AnnotationMember* structData = static_cast<AnnotationMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += MemberTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<Member*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AnnotationMemberValueTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->default_value, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AnnotationMemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AnnotationMember* structData = static_cast<AnnotationMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AnnotationMemberValueTypeSupport::get_instance()->get_max_serialized_data_size(&structData->default_value, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

int AnnotationMemberTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AnnotationMember* structData = static_cast<AnnotationMember*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, DEFAULT_VALUE_ANNOTATIONMEMBER_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->default_value\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (AnnotationMemberValueTypeSupport::get_instance()->serialize_data((void*)&structData->default_value, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->default_value\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AnnotationMemberTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AnnotationMember* structData = static_cast<AnnotationMember*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (MemberTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AnnotationMember\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case DEFAULT_VALUE_ANNOTATIONMEMBER_MEMBER_ID:
            if (AnnotationMemberValueTypeSupport::get_instance()->deserialize_data(&structData->default_value, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->default_value\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AnnotationMember\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* AnnotationMemberTypeSupport::get_typeobject() {
    return nullptr;
}

int AnnotationMemberTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AnnotationMember* structData = static_cast<AnnotationMember*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AnnotationMemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AnnotationMemberTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool AnnotationMemberTypeSupport::has_key() {
    return false;
}

const char* AnnotationMemberTypeSupport::get_typename() {
    return "AnnotationMember";
}

AnnotationTypeTypeSupport* AnnotationTypeTypeSupport::get_instance() {
    static AnnotationTypeTypeSupport instance;
    return &instance;
}

void* AnnotationTypeTypeSupport::create_data() {
    return new AnnotationType();
}

void AnnotationTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AnnotationType*>(data);
}

int AnnotationTypeTypeSupport::copy_data(void* dst, void* src) {
    AnnotationType* dstData = static_cast<AnnotationType*>(dst);
    AnnotationType* srcData = static_cast<AnnotationType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AnnotationTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AnnotationType* structData = static_cast<AnnotationType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += AnnotationMemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AnnotationTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AnnotationTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AnnotationType* structData = static_cast<AnnotationType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->default_value\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, BASE_TYPE_ANNOTATIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->base_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, MEMBER_ANNOTATIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->member\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    tmpLength = static_cast<uint32_t>(structData->member.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->member\n");
        return -1;
    }
    for (size_t i = 0; i < structData->member.size(); ++i) {
        if (AnnotationMemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AnnotationTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AnnotationType* structData = static_cast<AnnotationType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->default_value\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AnnotationType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case BASE_TYPE_ANNOTATIONTYPE_MEMBER_ID:
            if (_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->base_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
                return -1;
            }
            break;
        case MEMBER_ANNOTATIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->member\n");
                return -1;
            }
            structData->member.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (AnnotationMemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AnnotationType\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* AnnotationTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AnnotationTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AnnotationType* structData = static_cast<AnnotationType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AnnotationTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AnnotationTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool AnnotationTypeTypeSupport::has_key() {
    return false;
}

const char* AnnotationTypeTypeSupport::get_typename() {
    return "AnnotationType";
}

AliasTypeTypeSupport* AliasTypeTypeSupport::get_instance() {
    static AliasTypeTypeSupport instance;
    return &instance;
}

void* AliasTypeTypeSupport::create_data() {
    return new AliasType();
}

void AliasTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AliasType*>(data);
}

int AliasTypeTypeSupport::copy_data(void* dst, void* src) {
    AliasType* dstData = static_cast<AliasType*>(dst);
    AliasType* srcData = static_cast<AliasType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AliasTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AliasType* structData = static_cast<AliasType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AliasTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AliasType* structData = static_cast<AliasType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

int AliasTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AliasType* structData = static_cast<AliasType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, BASE_TYPE_ALIASTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->base_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AliasTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AliasType* structData = static_cast<AliasType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AliasType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case BASE_TYPE_ALIASTYPE_MEMBER_ID:
            if (_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->base_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AliasType\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* AliasTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AliasTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AliasType* structData = static_cast<AliasType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AliasTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AliasTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool AliasTypeTypeSupport::has_key() {
    return false;
}

const char* AliasTypeTypeSupport::get_typename() {
    return "AliasType";
}

CollectionTypeTypeSupport* CollectionTypeTypeSupport::get_instance() {
    static CollectionTypeTypeSupport instance;
    return &instance;
}

void* CollectionTypeTypeSupport::create_data() {
    return new CollectionType();
}

void CollectionTypeTypeSupport::delete_data(void* data) {
    delete static_cast<CollectionType*>(data);
}

int CollectionTypeTypeSupport::copy_data(void* dst, void* src) {
    CollectionType* dstData = static_cast<CollectionType*>(dst);
    CollectionType* srcData = static_cast<CollectionType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int CollectionTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    CollectionType* structData = static_cast<CollectionType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->element_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int CollectionTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    CollectionType* structData = static_cast<CollectionType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->element_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

int CollectionTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    CollectionType* structData = static_cast<CollectionType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, ELEMENT_TYPE_COLLECTIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->element_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->element_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->element_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, ELEMENT_SHARED_COLLECTIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->element_shared\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (!cdr->serializeBaseType(structData->element_shared)) {
        fprintf(stderr, "Serialization failed for field: structData->element_shared\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int CollectionTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    CollectionType* structData = static_cast<CollectionType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: CollectionType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case ELEMENT_TYPE_COLLECTIONTYPE_MEMBER_ID:
            if (_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->element_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->element_type\n");
                return -1;
            }
            break;
        case ELEMENT_SHARED_COLLECTIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(structData->element_shared)) {
                fprintf(stderr, "Deserialization failed for field: structData->element_shared\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: CollectionType\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* CollectionTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int CollectionTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    CollectionType* structData = static_cast<CollectionType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int CollectionTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t CollectionTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool CollectionTypeTypeSupport::has_key() {
    return false;
}

const char* CollectionTypeTypeSupport::get_typename() {
    return "CollectionType";
}

ArrayTypeTypeSupport* ArrayTypeTypeSupport::get_instance() {
    static ArrayTypeTypeSupport instance;
    return &instance;
}

void* ArrayTypeTypeSupport::create_data() {
    return new ArrayType();
}

void ArrayTypeTypeSupport::delete_data(void* data) {
    delete static_cast<ArrayType*>(data);
}

int ArrayTypeTypeSupport::copy_data(void* dst, void* src) {
    ArrayType* dstData = static_cast<ArrayType*>(dst);
    ArrayType* srcData = static_cast<ArrayType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int ArrayTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    ArrayType* structData = static_cast<ArrayType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<CollectionType*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->bound.size(); ++i) {
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int ArrayTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int ArrayTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    ArrayType* structData = static_cast<ArrayType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->element_shared\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, BOUND_ARRAYTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    tmpLength = static_cast<uint32_t>(structData->bound.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->bound\n");
        return -1;
    }
    for (size_t i = 0; i < structData->bound.size(); ++i) {
        if (!cdr->serializeBaseType(structData->bound[i])) {
            fprintf(stderr, "Serialization failed for field: structData->bound[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int ArrayTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    ArrayType* structData = static_cast<ArrayType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->element_shared\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: ArrayType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case BOUND_ARRAYTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->bound\n");
                return -1;
            }
            structData->bound.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (!cdr->deserializeBaseType(structData->bound[i])) {
                    fprintf(stderr, "Deserialization failed for field: structData->bound[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: ArrayType\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* ArrayTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int ArrayTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    ArrayType* structData = static_cast<ArrayType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int ArrayTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t ArrayTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool ArrayTypeTypeSupport::has_key() {
    return false;
}

const char* ArrayTypeTypeSupport::get_typename() {
    return "ArrayType";
}

MapTypeTypeSupport* MapTypeTypeSupport::get_instance() {
    static MapTypeTypeSupport instance;
    return &instance;
}

void* MapTypeTypeSupport::create_data() {
    return new MapType();
}

void MapTypeTypeSupport::delete_data(void* data) {
    delete static_cast<MapType*>(data);
}

int MapTypeTypeSupport::copy_data(void* dst, void* src) {
    MapType* dstData = static_cast<MapType*>(dst);
    MapType* srcData = static_cast<MapType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int MapTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    MapType* structData = static_cast<MapType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<CollectionType*>(structData), currentAlignment);
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->key_element_type, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int MapTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    MapType* structData = static_cast<MapType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += _TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->key_element_type, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int MapTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    MapType* structData = static_cast<MapType*>(data);
    uint32_t tmpLength = 0;
    if (CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->bound[i]\n");
        return -1;
    }
    if (_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->key_element_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->key_element_type\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

int MapTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    MapType* structData = static_cast<MapType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->bound[i]\n");
        return -1;
    }
    if (_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->key_element_type, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->key_element_type\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->bound)) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

TypeObject* MapTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int MapTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    MapType* structData = static_cast<MapType*>(data);
    bool memberHasKey = false;
    if (CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int MapTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t MapTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool MapTypeTypeSupport::has_key() {
    return false;
}

const char* MapTypeTypeSupport::get_typename() {
    return "MapType";
}

SequenceTypeTypeSupport* SequenceTypeTypeSupport::get_instance() {
    static SequenceTypeTypeSupport instance;
    return &instance;
}

void* SequenceTypeTypeSupport::create_data() {
    return new SequenceType();
}

void SequenceTypeTypeSupport::delete_data(void* data) {
    delete static_cast<SequenceType*>(data);
}

int SequenceTypeTypeSupport::copy_data(void* dst, void* src) {
    SequenceType* dstData = static_cast<SequenceType*>(dst);
    SequenceType* srcData = static_cast<SequenceType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int SequenceTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    SequenceType* structData = static_cast<SequenceType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<CollectionType*>(structData), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int SequenceTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    SequenceType* structData = static_cast<SequenceType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int SequenceTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    SequenceType* structData = static_cast<SequenceType*>(data);
    uint32_t tmpLength = 0;
    if (CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

int SequenceTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    SequenceType* structData = static_cast<SequenceType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->bound)) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

TypeObject* SequenceTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int SequenceTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    SequenceType* structData = static_cast<SequenceType*>(data);
    bool memberHasKey = false;
    if (CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int SequenceTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t SequenceTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool SequenceTypeTypeSupport::has_key() {
    return false;
}

const char* SequenceTypeTypeSupport::get_typename() {
    return "SequenceType";
}

StringTypeTypeSupport* StringTypeTypeSupport::get_instance() {
    static StringTypeTypeSupport instance;
    return &instance;
}

void* StringTypeTypeSupport::create_data() {
    return new StringType();
}

void StringTypeTypeSupport::delete_data(void* data) {
    delete static_cast<StringType*>(data);
}

int StringTypeTypeSupport::copy_data(void* dst, void* src) {
    StringType* dstData = static_cast<StringType*>(dst);
    StringType* srcData = static_cast<StringType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int StringTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    StringType* structData = static_cast<StringType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<CollectionType*>(structData), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int StringTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    StringType* structData = static_cast<StringType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int StringTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    StringType* structData = static_cast<StringType*>(data);
    uint32_t tmpLength = 0;
    if (CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

int StringTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    StringType* structData = static_cast<StringType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->bound)) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

TypeObject* StringTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int StringTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    StringType* structData = static_cast<StringType*>(data);
    bool memberHasKey = false;
    if (CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int StringTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t StringTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool StringTypeTypeSupport::has_key() {
    return false;
}

const char* StringTypeTypeSupport::get_typename() {
    return "StringType";
}

BitTypeSupport* BitTypeSupport::get_instance() {
    static BitTypeSupport instance;
    return &instance;
}

void* BitTypeSupport::create_data() {
    return new Bit();
}

void BitTypeSupport::delete_data(void* data) {
    delete static_cast<Bit*>(data);
}

int BitTypeSupport::copy_data(void* dst, void* src) {
    Bit* dstData = static_cast<Bit*>(dst);
    Bit* srcData = static_cast<Bit*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int BitTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    Bit* structData = static_cast<Bit*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int BitTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    Bit* structData = static_cast<Bit*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int BitTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    Bit* structData = static_cast<Bit*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->index)) {
        fprintf(stderr, "Serialization failed for field: structData->index\n");
        return -1;
    }
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int BitTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    Bit* structData = static_cast<Bit*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->index)) {
        fprintf(stderr, "Deserialization failed for field: structData->index\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* BitTypeSupport::get_typeobject() {
    return nullptr;
}

int BitTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    Bit* structData = static_cast<Bit*>(data);
    bool memberHasKey = false;
    return 0;
}

int BitTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t BitTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool BitTypeSupport::has_key() {
    return false;
}

const char* BitTypeSupport::get_typename() {
    return "Bit";
}

BitSetTypeTypeSupport* BitSetTypeTypeSupport::get_instance() {
    static BitSetTypeTypeSupport instance;
    return &instance;
}

void* BitSetTypeTypeSupport::create_data() {
    return new BitSetType();
}

void BitSetTypeTypeSupport::delete_data(void* data) {
    delete static_cast<BitSetType*>(data);
}

int BitSetTypeTypeSupport::copy_data(void* dst, void* src) {
    BitSetType* dstData = static_cast<BitSetType*>(dst);
    BitSetType* srcData = static_cast<BitSetType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int BitSetTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    BitSetType* structData = static_cast<BitSetType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->bit.size(); ++i) {
        currentAlignment += BitTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->bit[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int BitSetTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int BitSetTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    BitSetType* structData = static_cast<BitSetType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, BIT_BOUND_BITSETTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->bit_bound\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (!cdr->serializeBaseType(structData->bit_bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bit_bound\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, BIT_BITSETTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->bit\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    tmpLength = static_cast<uint32_t>(structData->bit.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->bit\n");
        return -1;
    }
    for (size_t i = 0; i < structData->bit.size(); ++i) {
        if (BitTypeSupport::get_instance()->serialize_data((void*)&structData->bit[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->bit[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int BitSetTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    BitSetType* structData = static_cast<BitSetType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: BitSetType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case BIT_BOUND_BITSETTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(structData->bit_bound)) {
                fprintf(stderr, "Deserialization failed for field: structData->bit_bound\n");
                return -1;
            }
            break;
        case BIT_BITSETTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->bit\n");
                return -1;
            }
            structData->bit.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (BitTypeSupport::get_instance()->deserialize_data(&structData->bit[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->bit[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: BitSetType\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* BitSetTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int BitSetTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    BitSetType* structData = static_cast<BitSetType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int BitSetTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t BitSetTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool BitSetTypeTypeSupport::has_key() {
    return false;
}

const char* BitSetTypeTypeSupport::get_typename() {
    return "BitSetType";
}

EnumeratedConstantTypeSupport* EnumeratedConstantTypeSupport::get_instance() {
    static EnumeratedConstantTypeSupport instance;
    return &instance;
}

void* EnumeratedConstantTypeSupport::create_data() {
    return new EnumeratedConstant();
}

void EnumeratedConstantTypeSupport::delete_data(void* data) {
    delete static_cast<EnumeratedConstant*>(data);
}

int EnumeratedConstantTypeSupport::copy_data(void* dst, void* src) {
    EnumeratedConstant* dstData = static_cast<EnumeratedConstant*>(dst);
    EnumeratedConstant* srcData = static_cast<EnumeratedConstant*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int EnumeratedConstantTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    EnumeratedConstant* structData = static_cast<EnumeratedConstant*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int EnumeratedConstantTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    EnumeratedConstant* structData = static_cast<EnumeratedConstant*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int EnumeratedConstantTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    EnumeratedConstant* structData = static_cast<EnumeratedConstant*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->value)) {
        fprintf(stderr, "Serialization failed for field: structData->value\n");
        return -1;
    }
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int EnumeratedConstantTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    EnumeratedConstant* structData = static_cast<EnumeratedConstant*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->value)) {
        fprintf(stderr, "Deserialization failed for field: structData->value\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* EnumeratedConstantTypeSupport::get_typeobject() {
    return nullptr;
}

int EnumeratedConstantTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    EnumeratedConstant* structData = static_cast<EnumeratedConstant*>(data);
    bool memberHasKey = false;
    return 0;
}

int EnumeratedConstantTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t EnumeratedConstantTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool EnumeratedConstantTypeSupport::has_key() {
    return false;
}

const char* EnumeratedConstantTypeSupport::get_typename() {
    return "EnumeratedConstant";
}

EnumerationTypeTypeSupport* EnumerationTypeTypeSupport::get_instance() {
    static EnumerationTypeTypeSupport instance;
    return &instance;
}

void* EnumerationTypeTypeSupport::create_data() {
    return new EnumerationType();
}

void EnumerationTypeTypeSupport::delete_data(void* data) {
    delete static_cast<EnumerationType*>(data);
}

int EnumerationTypeTypeSupport::copy_data(void* dst, void* src) {
    EnumerationType* dstData = static_cast<EnumerationType*>(dst);
    EnumerationType* srcData = static_cast<EnumerationType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int EnumerationTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    EnumerationType* structData = static_cast<EnumerationType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->constant.size(); ++i) {
        currentAlignment += EnumeratedConstantTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->constant[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int EnumerationTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int EnumerationTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    EnumerationType* structData = static_cast<EnumerationType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, BIT_BOUND_ENUMERATIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->bit_bound\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (!cdr->serializeBaseType(structData->bit_bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bit_bound\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, CONSTANT_ENUMERATIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->constant\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    tmpLength = static_cast<uint32_t>(structData->constant.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->constant\n");
        return -1;
    }
    for (size_t i = 0; i < structData->constant.size(); ++i) {
        if (EnumeratedConstantTypeSupport::get_instance()->serialize_data((void*)&structData->constant[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->constant[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int EnumerationTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    EnumerationType* structData = static_cast<EnumerationType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: EnumerationType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case BIT_BOUND_ENUMERATIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(structData->bit_bound)) {
                fprintf(stderr, "Deserialization failed for field: structData->bit_bound\n");
                return -1;
            }
            break;
        case CONSTANT_ENUMERATIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->constant\n");
                return -1;
            }
            structData->constant.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (EnumeratedConstantTypeSupport::get_instance()->deserialize_data(&structData->constant[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->constant[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: EnumerationType\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* EnumerationTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int EnumerationTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    EnumerationType* structData = static_cast<EnumerationType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int EnumerationTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t EnumerationTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool EnumerationTypeTypeSupport::has_key() {
    return false;
}

const char* EnumerationTypeTypeSupport::get_typename() {
    return "EnumerationType";
}

_ModuleTypeSupport* _ModuleTypeSupport::get_instance() {
    static _ModuleTypeSupport instance;
    return &instance;
}

void* _ModuleTypeSupport::create_data() {
    return new _Module();
}

void _ModuleTypeSupport::delete_data(void* data) {
    delete static_cast<_Module*>(data);
}

int _ModuleTypeSupport::copy_data(void* dst, void* src) {
    _Module* dstData = static_cast<_Module*>(dst);
    _Module* srcData = static_cast<_Module*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int _ModuleTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    _Module* structData = static_cast<_Module*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int _ModuleTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    _Module* structData = static_cast<_Module*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int _ModuleTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    _Module* structData = static_cast<_Module*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int _ModuleTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    _Module* structData = static_cast<_Module*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* _ModuleTypeSupport::get_typeobject() {
    return nullptr;
}

int _ModuleTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    _Module* structData = static_cast<_Module*>(data);
    bool memberHasKey = false;
    return 0;
}

int _ModuleTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t _ModuleTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool _ModuleTypeSupport::has_key() {
    return false;
}

const char* _ModuleTypeSupport::get_typename() {
    return "_Module";
}

TypeLibraryElementTypeSupport* TypeLibraryElementTypeSupport::get_instance() {
    static TypeLibraryElementTypeSupport instance;
    return &instance;
}

void* TypeLibraryElementTypeSupport::create_data() {
    return new TypeLibraryElement();
}

void TypeLibraryElementTypeSupport::delete_data(void* data) {
    delete static_cast<TypeLibraryElement*>(data);
}

int TypeLibraryElementTypeSupport::copy_data(void* dst, void* src) {
    TypeLibraryElement* dstData = static_cast<TypeLibraryElement*>(dst);
    TypeLibraryElement* srcData = static_cast<TypeLibraryElement*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TypeLibraryElementTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TypeLibraryElement* structData = static_cast<TypeLibraryElement*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    switch(structData->u_) {
    case ALIAS_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AliasTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->alias_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case ANNOTATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AnnotationTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->annotation_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case ARRAY_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += ArrayTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->array_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case BITSET_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += BitSetTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->bitset_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case ENUMERATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += EnumerationTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->enumeration_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case MAP_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += MapTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->map_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case SEQUENCE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += SequenceTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->sequence_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case STRING_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += StringTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->string_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case STRUCTURE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += StructureTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->structure_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case UNION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += UnionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->union_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case MODULE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += _ModuleTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->mod, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    }
    return 0;
}

unsigned int TypeLibraryElementTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TypeLibraryElement* structData = static_cast<TypeLibraryElement*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    switch(structData->u_) {
    case ALIAS_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AliasTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->alias_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case ANNOTATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AnnotationTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->annotation_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case ARRAY_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += ArrayTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->array_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case BITSET_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += BitSetTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->bitset_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case ENUMERATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += EnumerationTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->enumeration_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case MAP_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += MapTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->map_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case SEQUENCE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += SequenceTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->sequence_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case STRING_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += StringTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->string_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case STRUCTURE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += StructureTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->structure_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case UNION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += UnionTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->union_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case MODULE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += _ModuleTypeSupport::get_instance()->get_max_serialized_data_size(&structData->mod, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    }
    return 0;
}

int TypeLibraryElementTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    TypeLibraryElement* structData = static_cast<TypeLibraryElement*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, 0);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->u_\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    switch(16 / 8) {
    case 1:
        if (!cdr->serializeBaseType(static_cast<char>(structData->u_))) {
            fprintf(stderr, "Serialization failed for field: structData->u_\n");
            return -1;
        }
        break;
    case 2:
        if (!cdr->serializeBaseType(static_cast<short>(structData->u_))) {
            fprintf(stderr, "Serialization failed for field: structData->u_\n");
            return -1;
        }
        break;
    case 4:
        if (!cdr->serializeBaseType(static_cast<int>(structData->u_))) {
            fprintf(stderr, "Serialization failed for field: structData->u_\n");
            return -1;
        }
        break;
    default:
        fprintf(stderr, "Serialization failed for field: structData->u_\n");
        return -1;
        break;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    switch(structData->u_) {
    case ALIAS_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 1);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->alias_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AliasTypeTypeSupport::get_instance()->serialize_data((void*)&structData->alias_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->alias_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case ANNOTATION_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 2);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->annotation_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AnnotationTypeTypeSupport::get_instance()->serialize_data((void*)&structData->annotation_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->annotation_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case ARRAY_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 3);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->array_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (ArrayTypeTypeSupport::get_instance()->serialize_data((void*)&structData->array_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->array_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case BITSET_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 4);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->bitset_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (BitSetTypeTypeSupport::get_instance()->serialize_data((void*)&structData->bitset_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->bitset_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case ENUMERATION_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 5);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->enumeration_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (EnumerationTypeTypeSupport::get_instance()->serialize_data((void*)&structData->enumeration_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->enumeration_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case MAP_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 6);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->map_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (MapTypeTypeSupport::get_instance()->serialize_data((void*)&structData->map_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->map_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case SEQUENCE_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 7);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->sequence_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (SequenceTypeTypeSupport::get_instance()->serialize_data((void*)&structData->sequence_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->sequence_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case STRING_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 8);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->string_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (StringTypeTypeSupport::get_instance()->serialize_data((void*)&structData->string_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->string_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case STRUCTURE_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 9);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->structure_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (StructureTypeTypeSupport::get_instance()->serialize_data((void*)&structData->structure_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->structure_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case UNION_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 10);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->union_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (UnionTypeTypeSupport::get_instance()->serialize_data((void*)&structData->union_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->union_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case MODULE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 11);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->mod\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (_ModuleTypeSupport::get_instance()->serialize_data((void*)&structData->mod, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->mod\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    }
    return 0;
}

int TypeLibraryElementTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    TypeLibraryElement* structData = static_cast<TypeLibraryElement*>(data);
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TypeLibraryElement\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case 0:
            switch(16 / 8) {
            case 1:
                if (!cdr->deserializeBaseType(tmpCharEnum)) {
                    fprintf(stderr, "Deserialization failed for field: structData->u_\n");
                    return -1;
                }
                structData->u_ = static_cast<TypeLibraryElementKind>(tmpCharEnum);
                break;
            case 2:
                if (!cdr->deserializeBaseType(tmpShortEnum)) {
                    fprintf(stderr, "Deserialization failed for field: structData->u_\n");
                    return -1;
                }
                structData->u_ = static_cast<TypeLibraryElementKind>(tmpShortEnum);
                break;
            case 4:
                if (!cdr->deserializeBaseType(tmpIntEnum)) {
                    fprintf(stderr, "Deserialization failed for field: structData->u_\n");
                    return -1;
                }
                structData->u_ = static_cast<TypeLibraryElementKind>(tmpIntEnum);
                break;
            default:
                fprintf(stderr, "Deserialization failed for field: structData->u_\n");
                return -1;
                break;
            }
            break;
        case 1:
            if (AliasTypeTypeSupport::get_instance()->deserialize_data(&structData->alias_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->alias_type\n");
                return -1;
            }
            break;
        case 2:
            if (AnnotationTypeTypeSupport::get_instance()->deserialize_data(&structData->annotation_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->annotation_type\n");
                return -1;
            }
            break;
        case 3:
            if (ArrayTypeTypeSupport::get_instance()->deserialize_data(&structData->array_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->array_type\n");
                return -1;
            }
            break;
        case 4:
            if (BitSetTypeTypeSupport::get_instance()->deserialize_data(&structData->bitset_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->bitset_type\n");
                return -1;
            }
            break;
        case 5:
            if (EnumerationTypeTypeSupport::get_instance()->deserialize_data(&structData->enumeration_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->enumeration_type\n");
                return -1;
            }
            break;
        case 6:
            if (MapTypeTypeSupport::get_instance()->deserialize_data(&structData->map_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->map_type\n");
                return -1;
            }
            break;
        case 7:
            if (SequenceTypeTypeSupport::get_instance()->deserialize_data(&structData->sequence_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->sequence_type\n");
                return -1;
            }
            break;
        case 8:
            if (StringTypeTypeSupport::get_instance()->deserialize_data(&structData->string_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->string_type\n");
                return -1;
            }
            break;
        case 9:
            if (StructureTypeTypeSupport::get_instance()->deserialize_data(&structData->structure_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->structure_type\n");
                return -1;
            }
            break;
        case 10:
            if (UnionTypeTypeSupport::get_instance()->deserialize_data(&structData->union_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->union_type\n");
                return -1;
            }
            break;
        case 11:
            if (_ModuleTypeSupport::get_instance()->deserialize_data(&structData->mod, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->mod\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TypeLibraryElement\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* TypeLibraryElementTypeSupport::get_typeobject() {
    return nullptr;
}

int TypeLibraryElementTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    TypeLibraryElement* structData = static_cast<TypeLibraryElement*>(data);
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    return 0;
}

int TypeLibraryElementTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TypeLibraryElementTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool TypeLibraryElementTypeSupport::has_key() {
    return false;
}

const char* TypeLibraryElementTypeSupport::get_typename() {
    return "TypeLibraryElement";
}

TypeLibraryTypeSupport* TypeLibraryTypeSupport::get_instance() {
    static TypeLibraryTypeSupport instance;
    return &instance;
}

void* TypeLibraryTypeSupport::create_data() {
    return new TypeLibrary();
}

void TypeLibraryTypeSupport::delete_data(void* data) {
    delete static_cast<TypeLibrary*>(data);
}

int TypeLibraryTypeSupport::copy_data(void* dst, void* src) {
    TypeLibrary* dstData = static_cast<TypeLibrary*>(dst);
    TypeLibrary* srcData = static_cast<TypeLibrary*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TypeLibraryTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TypeLibrary* structData = static_cast<TypeLibrary*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->element.size(); ++i) {
        currentAlignment += TypeLibraryElementTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->element[i], currentAlignment);
    }
    return currentAlignment - initialAlignment;
}

unsigned int TypeLibraryTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int TypeLibraryTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    TypeLibrary* structData = static_cast<TypeLibrary*>(data);
    uint32_t tmpLength = 0;
    tmpLength = static_cast<uint32_t>(structData->element.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->element\n");
        return -1;
    }
    for (size_t i = 0; i < structData->element.size(); ++i) {
        if (TypeLibraryElementTypeSupport::get_instance()->serialize_data((void*)&structData->element[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->element[i]\n");
            return -1;
        }
    }
    return 0;
}

int TypeLibraryTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    TypeLibrary* structData = static_cast<TypeLibrary*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->element\n");
        return -1;
    }
    structData->element.resize(tmpLength);
    for (size_t i = 0; i < tmpLength; ++i) {
        if (TypeLibraryElementTypeSupport::get_instance()->deserialize_data(&structData->element[i], cdr, endian) != 0) {
            fprintf(stderr, "Deserialization failed for field: structData->element[i]\n");
            return -1;
        }
    }
    return 0;
}

TypeObject* TypeLibraryTypeSupport::get_typeobject() {
    return nullptr;
}

int TypeLibraryTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    TypeLibrary* structData = static_cast<TypeLibrary*>(data);
    bool memberHasKey = false;
    return 0;
}

int TypeLibraryTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TypeLibraryTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool TypeLibraryTypeSupport::has_key() {
    return false;
}

const char* TypeLibraryTypeSupport::get_typename() {
    return "TypeLibrary";
}

TypeObjectTypeSupport* TypeObjectTypeSupport::get_instance() {
    static TypeObjectTypeSupport instance;
    return &instance;
}

void* TypeObjectTypeSupport::create_data() {
    return new TypeObject();
}

void TypeObjectTypeSupport::delete_data(void* data) {
    delete static_cast<TypeObject*>(data);
}

int TypeObjectTypeSupport::copy_data(void* dst, void* src) {
    TypeObject* dstData = static_cast<TypeObject*>(dst);
    TypeObject* srcData = static_cast<TypeObject*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TypeObjectTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TypeObject* structData = static_cast<TypeObject*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TypeLibraryTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->library, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->the_type.size(); ++i) {
        currentAlignment += _TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->the_type[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TypeObjectTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int TypeObjectTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    TypeObject* structData = static_cast<TypeObject*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, 0);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->library\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (TypeLibraryTypeSupport::get_instance()->serialize_data((void*)&structData->library, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->library\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, 1);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->the_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    tmpLength = static_cast<uint32_t>(structData->the_type.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->the_type\n");
        return -1;
    }
    for (size_t i = 0; i < structData->the_type.size(); ++i) {
        if (_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->the_type[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->the_type[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TypeObjectTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    TypeObject* structData = static_cast<TypeObject*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TypeObject\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case 0:
            if (TypeLibraryTypeSupport::get_instance()->deserialize_data(&structData->library, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->library\n");
                return -1;
            }
            break;
        case 1:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->the_type\n");
                return -1;
            }
            structData->the_type.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->the_type[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->the_type[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TypeObject\n");
                return -1;
            }
            break;
        }
        if (!cdr->aheadReaderPosition(readLen, length)) {
            fprintf(stderr, "move to next head(%d %d) failed.", readLen, length);
            return -1;
        }
    }
    return 0;
}

TypeObject* TypeObjectTypeSupport::get_typeobject() {
    return nullptr;
}

int TypeObjectTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    TypeObject* structData = static_cast<TypeObject*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    return 0;
}

int TypeObjectTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TypeObjectTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        fprintf(stderr, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        fprintf(stderr, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool TypeObjectTypeSupport::has_key() {
    return false;
}

const char* TypeObjectTypeSupport::get_typename() {
    return "TypeObject";
}

#ifdef __cplusplus
}
#endif
