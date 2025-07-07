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

USING_AGIDDS_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

AGIDDS::_TypeIdTypeSupport* AGIDDS::_TypeIdTypeSupport::get_instance() {
    static AGIDDS::_TypeIdTypeSupport instance;
    return &instance;
}

void* AGIDDS::_TypeIdTypeSupport::create_data() {
    return new AGIDDS::_TypeId();
}

void AGIDDS::_TypeIdTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::_TypeId*>(data);
}

int AGIDDS::_TypeIdTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::_TypeId* dstData = static_cast<AGIDDS::_TypeId*>(dst);
    AGIDDS::_TypeId* srcData = static_cast<AGIDDS::_TypeId*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::_TypeIdTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::_TypeId* structData = static_cast<AGIDDS::_TypeId*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
    switch(structData->u_) {
    case AGIDDS::BOOLEAN_TYPE:
    case AGIDDS::BYTE_TYPE:
    case AGIDDS::INT_16_TYPE:
    case AGIDDS::UINT_16_TYPE:
    case AGIDDS::INT_32_TYPE:
    case AGIDDS::UINT_32_TYPE:
    case AGIDDS::INT_64_TYPE:
    case AGIDDS::UINT_64_TYPE:
    case AGIDDS::FLOAT_32_TYPE:
    case AGIDDS::FLOAT_64_TYPE:
    case AGIDDS::FLOAT_128_TYPE:
    case AGIDDS::CHAR_8_TYPE:
    case AGIDDS::CHAR_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
        break;
    default:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
        break;
    }
    return 0;
}

unsigned int AGIDDS::_TypeIdTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::_TypeId* structData = static_cast<AGIDDS::_TypeId*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
    switch(structData->u_) {
    case AGIDDS::BOOLEAN_TYPE:
    case AGIDDS::BYTE_TYPE:
    case AGIDDS::INT_16_TYPE:
    case AGIDDS::UINT_16_TYPE:
    case AGIDDS::INT_32_TYPE:
    case AGIDDS::UINT_32_TYPE:
    case AGIDDS::INT_64_TYPE:
    case AGIDDS::UINT_64_TYPE:
    case AGIDDS::FLOAT_32_TYPE:
    case AGIDDS::FLOAT_64_TYPE:
    case AGIDDS::FLOAT_128_TYPE:
    case AGIDDS::CHAR_8_TYPE:
    case AGIDDS::CHAR_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
        break;
    default:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
        break;
    }
    return 0;
}

int AGIDDS::_TypeIdTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::_TypeId* structData = static_cast<AGIDDS::_TypeId*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->u_)) {
        fprintf(stderr, "Serialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case AGIDDS::BOOLEAN_TYPE:
    case AGIDDS::BYTE_TYPE:
    case AGIDDS::INT_16_TYPE:
    case AGIDDS::UINT_16_TYPE:
    case AGIDDS::INT_32_TYPE:
    case AGIDDS::UINT_32_TYPE:
    case AGIDDS::INT_64_TYPE:
    case AGIDDS::UINT_64_TYPE:
    case AGIDDS::FLOAT_32_TYPE:
    case AGIDDS::FLOAT_64_TYPE:
    case AGIDDS::FLOAT_128_TYPE:
    case AGIDDS::CHAR_8_TYPE:
    case AGIDDS::CHAR_32_TYPE:
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

int AGIDDS::_TypeIdTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::_TypeId* structData = static_cast<AGIDDS::_TypeId*>(data);
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->u_)) {
        fprintf(stderr, "Deserialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case AGIDDS::BOOLEAN_TYPE:
    case AGIDDS::BYTE_TYPE:
    case AGIDDS::INT_16_TYPE:
    case AGIDDS::UINT_16_TYPE:
    case AGIDDS::INT_32_TYPE:
    case AGIDDS::UINT_32_TYPE:
    case AGIDDS::INT_64_TYPE:
    case AGIDDS::UINT_64_TYPE:
    case AGIDDS::FLOAT_32_TYPE:
    case AGIDDS::FLOAT_64_TYPE:
    case AGIDDS::FLOAT_128_TYPE:
    case AGIDDS::CHAR_8_TYPE:
    case AGIDDS::CHAR_32_TYPE:
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

TypeObject* AGIDDS::_TypeIdTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::_TypeIdTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::_TypeId* structData = static_cast<AGIDDS::_TypeId*>(data);
    return 0;
}

int AGIDDS::_TypeIdTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::_TypeIdTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::_TypeIdTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::_TypeIdTypeSupport::get_typename() {
    return "AGIDDS::_TypeId";
}

AGIDDS::AnnotationMemberValueTypeSupport* AGIDDS::AnnotationMemberValueTypeSupport::get_instance() {
    static AGIDDS::AnnotationMemberValueTypeSupport instance;
    return &instance;
}

void* AGIDDS::AnnotationMemberValueTypeSupport::create_data() {
    return new AGIDDS::AnnotationMemberValue();
}

void AGIDDS::AnnotationMemberValueTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::AnnotationMemberValue*>(data);
}

int AGIDDS::AnnotationMemberValueTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::AnnotationMemberValue* dstData = static_cast<AGIDDS::AnnotationMemberValue*>(dst);
    AGIDDS::AnnotationMemberValue* srcData = static_cast<AGIDDS::AnnotationMemberValue*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::AnnotationMemberValueTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::AnnotationMemberValue* structData = static_cast<AGIDDS::AnnotationMemberValue*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
    switch(structData->u_) {
    case AGIDDS::BOOLEAN_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
        break;
    case AGIDDS::BYTE_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned char), currentAlignment);
        break;
    case AGIDDS::INT_16_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
        break;
    case AGIDDS::UINT_16_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned short), currentAlignment);
        break;
    case AGIDDS::INT_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
        break;
    case AGIDDS::UINT_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
        break;
    case AGIDDS::INT_64_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(long long), currentAlignment);
        break;
    case AGIDDS::UINT_64_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
        break;
    case AGIDDS::FLOAT_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(float), currentAlignment);
        break;
    case AGIDDS::FLOAT_64_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(double), currentAlignment);
        break;
    case AGIDDS::FLOAT_128_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(LongDouble), currentAlignment);
        break;
    case AGIDDS::CHAR_8_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
        break;
    case AGIDDS::CHAR_32_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(wchar_t), currentAlignment);
        break;
    case AGIDDS::ENUMERATION_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
        break;
    case AGIDDS::STRING_TYPE:
        currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->string_value.size() + 1, currentAlignment);
        break;
    }
    return 0;
}

unsigned int AGIDDS::AnnotationMemberValueTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::AnnotationMemberValueTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AnnotationMemberValue* structData = static_cast<AGIDDS::AnnotationMemberValue*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->u_)) {
        fprintf(stderr, "Serialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case AGIDDS::BOOLEAN_TYPE:
        if (!cdr->serializeBaseType(structData->boolean_value)) {
            fprintf(stderr, "Serialization failed for field: structData->boolean_value\n");
            return -1;
        }
        break;
    case AGIDDS::BYTE_TYPE:
        if (!cdr->serializeBaseType(structData->byte_value)) {
            fprintf(stderr, "Serialization failed for field: structData->byte_value\n");
            return -1;
        }
        break;
    case AGIDDS::INT_16_TYPE:
        if (!cdr->serializeBaseType(structData->int_16_value)) {
            fprintf(stderr, "Serialization failed for field: structData->int_16_value\n");
            return -1;
        }
        break;
    case AGIDDS::UINT_16_TYPE:
        if (!cdr->serializeBaseType(structData->uint_16_value)) {
            fprintf(stderr, "Serialization failed for field: structData->uint_16_value\n");
            return -1;
        }
        break;
    case AGIDDS::INT_32_TYPE:
        if (!cdr->serializeBaseType(structData->int_32_value)) {
            fprintf(stderr, "Serialization failed for field: structData->int_32_value\n");
            return -1;
        }
        break;
    case AGIDDS::UINT_32_TYPE:
        if (!cdr->serializeBaseType(structData->uint_32_value)) {
            fprintf(stderr, "Serialization failed for field: structData->uint_32_value\n");
            return -1;
        }
        break;
    case AGIDDS::INT_64_TYPE:
        if (!cdr->serializeBaseType(structData->int_64_value)) {
            fprintf(stderr, "Serialization failed for field: structData->int_64_value\n");
            return -1;
        }
        break;
    case AGIDDS::UINT_64_TYPE:
        if (!cdr->serializeBaseType(structData->uint_64_value)) {
            fprintf(stderr, "Serialization failed for field: structData->uint_64_value\n");
            return -1;
        }
        break;
    case AGIDDS::FLOAT_32_TYPE:
        if (!cdr->serializeBaseType(structData->float_32_value)) {
            fprintf(stderr, "Serialization failed for field: structData->float_32_value\n");
            return -1;
        }
        break;
    case AGIDDS::FLOAT_64_TYPE:
        if (!cdr->serializeBaseType(structData->float_64_value)) {
            fprintf(stderr, "Serialization failed for field: structData->float_64_value\n");
            return -1;
        }
        break;
    case AGIDDS::FLOAT_128_TYPE:
        if (!cdr->serializeLongDouble(*(LongDouble*)&structData->float_128_value)) {
            fprintf(stderr, "Serialization failed for field: structData->float_128_value\n");
            return -1;
        }
        break;
    case AGIDDS::CHAR_8_TYPE:
        if (!cdr->serializeBaseType(structData->character_value)) {
            fprintf(stderr, "Serialization failed for field: structData->character_value\n");
            return -1;
        }
        break;
    case AGIDDS::CHAR_32_TYPE:
        if (!cdr->serializeBaseType(structData->wide_character_value)) {
            fprintf(stderr, "Serialization failed for field: structData->wide_character_value\n");
            return -1;
        }
        break;
    case AGIDDS::ENUMERATION_TYPE:
        if (!cdr->serializeBaseType(structData->enumeration_value)) {
            fprintf(stderr, "Serialization failed for field: structData->enumeration_value\n");
            return -1;
        }
        break;
    case AGIDDS::STRING_TYPE:
        if (!cdr->serializeString(structData->string_value)) {
            fprintf(stderr, "Serialization failed for field: structData->string_value\n");
            return -1;
        }
        break;
    }
    return 0;
}

int AGIDDS::AnnotationMemberValueTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::AnnotationMemberValue* structData = static_cast<AGIDDS::AnnotationMemberValue*>(data);
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->u_)) {
        fprintf(stderr, "Deserialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case AGIDDS::BOOLEAN_TYPE:
        if (!cdr->deserializeBaseType(structData->boolean_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->boolean_value\n");
            return -1;
        }
        break;
    case AGIDDS::BYTE_TYPE:
        if (!cdr->deserializeBaseType(structData->byte_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->byte_value\n");
            return -1;
        }
        break;
    case AGIDDS::INT_16_TYPE:
        if (!cdr->deserializeBaseType(structData->int_16_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->int_16_value\n");
            return -1;
        }
        break;
    case AGIDDS::UINT_16_TYPE:
        if (!cdr->deserializeBaseType(structData->uint_16_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->uint_16_value\n");
            return -1;
        }
        break;
    case AGIDDS::INT_32_TYPE:
        if (!cdr->deserializeBaseType(structData->int_32_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->int_32_value\n");
            return -1;
        }
        break;
    case AGIDDS::UINT_32_TYPE:
        if (!cdr->deserializeBaseType(structData->uint_32_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->uint_32_value\n");
            return -1;
        }
        break;
    case AGIDDS::INT_64_TYPE:
        if (!cdr->deserializeBaseType(structData->int_64_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->int_64_value\n");
            return -1;
        }
        break;
    case AGIDDS::UINT_64_TYPE:
        if (!cdr->deserializeBaseType(structData->uint_64_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->uint_64_value\n");
            return -1;
        }
        break;
    case AGIDDS::FLOAT_32_TYPE:
        if (!cdr->deserializeBaseType(structData->float_32_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->float_32_value\n");
            return -1;
        }
        break;
    case AGIDDS::FLOAT_64_TYPE:
        if (!cdr->deserializeBaseType(structData->float_64_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->float_64_value\n");
            return -1;
        }
        break;
    case AGIDDS::FLOAT_128_TYPE:
        if (!cdr->deserializeLongDouble(*(LongDouble*)&structData->float_128_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->float_128_value\n");
            return -1;
        }
        break;
    case AGIDDS::CHAR_8_TYPE:
        if (!cdr->deserializeBaseType(structData->character_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->character_value\n");
            return -1;
        }
        break;
    case AGIDDS::CHAR_32_TYPE:
        if (!cdr->deserializeBaseType(structData->wide_character_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->wide_character_value\n");
            return -1;
        }
        break;
    case AGIDDS::ENUMERATION_TYPE:
        if (!cdr->deserializeBaseType(structData->enumeration_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->enumeration_value\n");
            return -1;
        }
        break;
    case AGIDDS::STRING_TYPE:
        if (!cdr->deserializeString(structData->string_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->string_value\n");
            return -1;
        }
        break;
    }
    return 0;
}

TypeObject* AGIDDS::AnnotationMemberValueTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::AnnotationMemberValueTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AnnotationMemberValue* structData = static_cast<AGIDDS::AnnotationMemberValue*>(data);
    return 0;
}

int AGIDDS::AnnotationMemberValueTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::AnnotationMemberValueTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::AnnotationMemberValueTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::AnnotationMemberValueTypeSupport::get_typename() {
    return "AGIDDS::AnnotationMemberValue";
}

AGIDDS::AnnotationUsageMemberTypeSupport* AGIDDS::AnnotationUsageMemberTypeSupport::get_instance() {
    static AGIDDS::AnnotationUsageMemberTypeSupport instance;
    return &instance;
}

void* AGIDDS::AnnotationUsageMemberTypeSupport::create_data() {
    return new AGIDDS::AnnotationUsageMember();
}

void AGIDDS::AnnotationUsageMemberTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::AnnotationUsageMember*>(data);
}

int AGIDDS::AnnotationUsageMemberTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::AnnotationUsageMember* dstData = static_cast<AGIDDS::AnnotationUsageMember*>(dst);
    AGIDDS::AnnotationUsageMember* srcData = static_cast<AGIDDS::AnnotationUsageMember*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::AnnotationUsageMemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::AnnotationUsageMember* structData = static_cast<AGIDDS::AnnotationUsageMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += AGIDDS::AnnotationMemberValueTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->value, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::AnnotationUsageMemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::AnnotationUsageMember* structData = static_cast<AGIDDS::AnnotationUsageMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += AGIDDS::AnnotationMemberValueTypeSupport::get_instance()->get_max_serialized_data_size(&structData->value, currentAlignment);
    return currentAlignment - initialAlignment;
}

int AGIDDS::AnnotationUsageMemberTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AnnotationUsageMember* structData = static_cast<AGIDDS::AnnotationUsageMember*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->member_id)) {
        fprintf(stderr, "Serialization failed for field: structData->member_id\n");
        return -1;
    }
    if (AGIDDS::AnnotationMemberValueTypeSupport::get_instance()->serialize_data((void*)&structData->value, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

int AGIDDS::AnnotationUsageMemberTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::AnnotationUsageMember* structData = static_cast<AGIDDS::AnnotationUsageMember*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->member_id)) {
        fprintf(stderr, "Deserialization failed for field: structData->member_id\n");
        return -1;
    }
    if (AGIDDS::AnnotationMemberValueTypeSupport::get_instance()->deserialize_data(&structData->value, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

TypeObject* AGIDDS::AnnotationUsageMemberTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::AnnotationUsageMemberTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AnnotationUsageMember* structData = static_cast<AGIDDS::AnnotationUsageMember*>(data);
    bool memberHasKey = false;
    return 0;
}

int AGIDDS::AnnotationUsageMemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::AnnotationUsageMemberTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::AnnotationUsageMemberTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::AnnotationUsageMemberTypeSupport::get_typename() {
    return "AGIDDS::AnnotationUsageMember";
}

AGIDDS::AnnotationUsageTypeSupport* AGIDDS::AnnotationUsageTypeSupport::get_instance() {
    static AGIDDS::AnnotationUsageTypeSupport instance;
    return &instance;
}

void* AGIDDS::AnnotationUsageTypeSupport::create_data() {
    return new AGIDDS::AnnotationUsage();
}

void AGIDDS::AnnotationUsageTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::AnnotationUsage*>(data);
}

int AGIDDS::AnnotationUsageTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::AnnotationUsage* dstData = static_cast<AGIDDS::AnnotationUsage*>(dst);
    AGIDDS::AnnotationUsage* srcData = static_cast<AGIDDS::AnnotationUsage*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::AnnotationUsageTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::AnnotationUsage* structData = static_cast<AGIDDS::AnnotationUsage*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->type_id, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += AGIDDS::AnnotationUsageMemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::AnnotationUsageTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::AnnotationUsageTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AnnotationUsage* structData = static_cast<AGIDDS::AnnotationUsage*>(data);
    uint32_t tmpLength = 0;
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->type_id\n");
        return -1;
    }
    tmpLength = static_cast<uint32_t>(structData->member.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->member\n");
        return -1;
    }
    for (size_t i = 0; i < structData->member.size(); ++i) {
        if (AGIDDS::AnnotationUsageMemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    return 0;
}

int AGIDDS::AnnotationUsageTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::AnnotationUsage* structData = static_cast<AGIDDS::AnnotationUsage*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->member\n");
        return -1;
    }
    structData->member.resize(tmpLength);
    for (size_t i = 0; i < tmpLength; ++i) {
        if (AGIDDS::AnnotationUsageMemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    return 0;
}

TypeObject* AGIDDS::AnnotationUsageTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::AnnotationUsageTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AnnotationUsage* structData = static_cast<AGIDDS::AnnotationUsage*>(data);
    bool memberHasKey = false;
    return 0;
}

int AGIDDS::AnnotationUsageTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::AnnotationUsageTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::AnnotationUsageTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::AnnotationUsageTypeSupport::get_typename() {
    return "AGIDDS::AnnotationUsage";
}

AGIDDS::TypePropertyTypeSupport* AGIDDS::TypePropertyTypeSupport::get_instance() {
    static AGIDDS::TypePropertyTypeSupport instance;
    return &instance;
}

void* AGIDDS::TypePropertyTypeSupport::create_data() {
    return new AGIDDS::TypeProperty();
}

void AGIDDS::TypePropertyTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::TypeProperty*>(data);
}

int AGIDDS::TypePropertyTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::TypeProperty* dstData = static_cast<AGIDDS::TypeProperty*>(dst);
    AGIDDS::TypeProperty* srcData = static_cast<AGIDDS::TypeProperty*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::TypePropertyTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::TypeProperty* structData = static_cast<AGIDDS::TypeProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->type_id, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::TypePropertyTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::TypeProperty* structData = static_cast<AGIDDS::TypeProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->type_id, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int AGIDDS::TypePropertyTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::TypeProperty* structData = static_cast<AGIDDS::TypeProperty*>(data);
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
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int AGIDDS::TypePropertyTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::TypeProperty* structData = static_cast<AGIDDS::TypeProperty*>(data);
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
        structData->flag = static_cast<AGIDDS::TypeFlag>(tmpCharEnum);
        break;
    case 2:
        if (!cdr->deserializeBaseType(tmpShortEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<AGIDDS::TypeFlag>(tmpShortEnum);
        break;
    case 4:
        if (!cdr->deserializeBaseType(tmpIntEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<AGIDDS::TypeFlag>(tmpIntEnum);
        break;
    default:
        fprintf(stderr, "Deserialization failed for field: structData->flag\n");
        return -1;
        break;
    }
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* AGIDDS::TypePropertyTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::TypePropertyTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::TypeProperty* structData = static_cast<AGIDDS::TypeProperty*>(data);
    bool memberHasKey = false;
    return 0;
}

int AGIDDS::TypePropertyTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::TypePropertyTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::TypePropertyTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::TypePropertyTypeSupport::get_typename() {
    return "AGIDDS::TypeProperty";
}

AGIDDS::TypeTypeSupport* AGIDDS::TypeTypeSupport::get_instance() {
    static AGIDDS::TypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::TypeTypeSupport::create_data() {
    return new AGIDDS::Type();
}

void AGIDDS::TypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::Type*>(data);
}

int AGIDDS::TypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::Type* dstData = static_cast<AGIDDS::Type*>(dst);
    AGIDDS::Type* srcData = static_cast<AGIDDS::Type*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::TypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::Type* structData = static_cast<AGIDDS::Type*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::TypePropertyTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->property, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->annotation.size(); ++i) {
        currentAlignment += AGIDDS::AnnotationUsageTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->annotation[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::TypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::TypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::Type* structData = static_cast<AGIDDS::Type*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::PROPERTY_TYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (AGIDDS::TypePropertyTypeSupport::get_instance()->serialize_data((void*)&structData->property, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::ANNOTATION_TYPE_MEMBER_ID);
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
        if (AGIDDS::AnnotationUsageTypeSupport::get_instance()->serialize_data((void*)&structData->annotation[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->annotation[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AGIDDS::TypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::Type* structData = static_cast<AGIDDS::Type*>(data);
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
            fprintf(stderr, "Deserialization failed for field: AGIDDS::Type\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::PROPERTY_TYPE_MEMBER_ID:
            if (AGIDDS::TypePropertyTypeSupport::get_instance()->deserialize_data(&structData->property, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->property\n");
                return -1;
            }
            break;
        case AGIDDS::ANNOTATION_TYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->annotation\n");
                return -1;
            }
            structData->annotation.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (AGIDDS::AnnotationUsageTypeSupport::get_instance()->deserialize_data(&structData->annotation[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->annotation[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::Type\n");
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

TypeObject* AGIDDS::TypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::TypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::Type* structData = static_cast<AGIDDS::Type*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    return 0;
}

int AGIDDS::TypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::TypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::TypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::TypeTypeSupport::get_typename() {
    return "AGIDDS::Type";
}

AGIDDS::MemberPropertyTypeSupport* AGIDDS::MemberPropertyTypeSupport::get_instance() {
    static AGIDDS::MemberPropertyTypeSupport instance;
    return &instance;
}

void* AGIDDS::MemberPropertyTypeSupport::create_data() {
    return new AGIDDS::MemberProperty();
}

void AGIDDS::MemberPropertyTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::MemberProperty*>(data);
}

int AGIDDS::MemberPropertyTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::MemberProperty* dstData = static_cast<AGIDDS::MemberProperty*>(dst);
    AGIDDS::MemberProperty* srcData = static_cast<AGIDDS::MemberProperty*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::MemberPropertyTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::MemberProperty* structData = static_cast<AGIDDS::MemberProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->type_id, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::MemberPropertyTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::MemberProperty* structData = static_cast<AGIDDS::MemberProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->type_id, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int AGIDDS::MemberPropertyTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::MemberProperty* structData = static_cast<AGIDDS::MemberProperty*>(data);
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
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int AGIDDS::MemberPropertyTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::MemberProperty* structData = static_cast<AGIDDS::MemberProperty*>(data);
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
        structData->flag = static_cast<AGIDDS::MemberFlag>(tmpCharEnum);
        break;
    case 2:
        if (!cdr->deserializeBaseType(tmpShortEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<AGIDDS::MemberFlag>(tmpShortEnum);
        break;
    case 4:
        if (!cdr->deserializeBaseType(tmpIntEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<AGIDDS::MemberFlag>(tmpIntEnum);
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
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* AGIDDS::MemberPropertyTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::MemberPropertyTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::MemberProperty* structData = static_cast<AGIDDS::MemberProperty*>(data);
    bool memberHasKey = false;
    return 0;
}

int AGIDDS::MemberPropertyTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::MemberPropertyTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::MemberPropertyTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::MemberPropertyTypeSupport::get_typename() {
    return "AGIDDS::MemberProperty";
}

AGIDDS::MemberTypeSupport* AGIDDS::MemberTypeSupport::get_instance() {
    static AGIDDS::MemberTypeSupport instance;
    return &instance;
}

void* AGIDDS::MemberTypeSupport::create_data() {
    return new AGIDDS::Member();
}

void AGIDDS::MemberTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::Member*>(data);
}

int AGIDDS::MemberTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::Member* dstData = static_cast<AGIDDS::Member*>(dst);
    AGIDDS::Member* srcData = static_cast<AGIDDS::Member*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::MemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::Member* structData = static_cast<AGIDDS::Member*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::MemberPropertyTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->property, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->annotation.size(); ++i) {
        currentAlignment += AGIDDS::AnnotationUsageTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->annotation[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::MemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::MemberTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::Member* structData = static_cast<AGIDDS::Member*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::PROPERTY_MEMBER_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (AGIDDS::MemberPropertyTypeSupport::get_instance()->serialize_data((void*)&structData->property, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::ANNOTATION_MEMBER_MEMBER_ID);
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
        if (AGIDDS::AnnotationUsageTypeSupport::get_instance()->serialize_data((void*)&structData->annotation[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->annotation[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AGIDDS::MemberTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::Member* structData = static_cast<AGIDDS::Member*>(data);
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
            fprintf(stderr, "Deserialization failed for field: AGIDDS::Member\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::PROPERTY_MEMBER_MEMBER_ID:
            if (AGIDDS::MemberPropertyTypeSupport::get_instance()->deserialize_data(&structData->property, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->property\n");
                return -1;
            }
            break;
        case AGIDDS::ANNOTATION_MEMBER_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->annotation\n");
                return -1;
            }
            structData->annotation.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (AGIDDS::AnnotationUsageTypeSupport::get_instance()->deserialize_data(&structData->annotation[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->annotation[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::Member\n");
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

TypeObject* AGIDDS::MemberTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::MemberTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::Member* structData = static_cast<AGIDDS::Member*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    return 0;
}

int AGIDDS::MemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::MemberTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::MemberTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::MemberTypeSupport::get_typename() {
    return "AGIDDS::Member";
}

AGIDDS::StructureTypeTypeSupport* AGIDDS::StructureTypeTypeSupport::get_instance() {
    static AGIDDS::StructureTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::StructureTypeTypeSupport::create_data() {
    return new AGIDDS::StructureType();
}

void AGIDDS::StructureTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::StructureType*>(data);
}

int AGIDDS::StructureTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::StructureType* dstData = static_cast<AGIDDS::StructureType*>(dst);
    AGIDDS::StructureType* srcData = static_cast<AGIDDS::StructureType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::StructureTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::StructureType* structData = static_cast<AGIDDS::StructureType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += AGIDDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += AGIDDS::MemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::StructureTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::StructureTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::StructureType* structData = static_cast<AGIDDS::StructureType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->annotation[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::BASE_TYPE_STRUCTURETYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->base_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::MEMBER_STRUCTURETYPE_MEMBER_ID);
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
        if (AGIDDS::MemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AGIDDS::StructureTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::StructureType* structData = static_cast<AGIDDS::StructureType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->annotation[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AGIDDS::StructureType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::BASE_TYPE_STRUCTURETYPE_MEMBER_ID:
            if (AGIDDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->base_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
                return -1;
            }
            break;
        case AGIDDS::MEMBER_STRUCTURETYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->member\n");
                return -1;
            }
            structData->member.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (AGIDDS::MemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::StructureType\n");
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

TypeObject* AGIDDS::StructureTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::StructureTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::StructureType* structData = static_cast<AGIDDS::StructureType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::StructureTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::StructureTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::StructureTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::StructureTypeTypeSupport::get_typename() {
    return "AGIDDS::StructureType";
}

AGIDDS::UnionMemberTypeSupport* AGIDDS::UnionMemberTypeSupport::get_instance() {
    static AGIDDS::UnionMemberTypeSupport instance;
    return &instance;
}

void* AGIDDS::UnionMemberTypeSupport::create_data() {
    return new AGIDDS::UnionMember();
}

void AGIDDS::UnionMemberTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::UnionMember*>(data);
}

int AGIDDS::UnionMemberTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::UnionMember* dstData = static_cast<AGIDDS::UnionMember*>(dst);
    AGIDDS::UnionMember* srcData = static_cast<AGIDDS::UnionMember*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::UnionMemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::UnionMember* structData = static_cast<AGIDDS::UnionMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += AGIDDS::MemberTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::Member*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeArraySize(sizeof(int), structData->label.size(), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::UnionMemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::UnionMemberTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::UnionMember* structData = static_cast<AGIDDS::UnionMember*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::LABEL_UNIONMEMBER_MEMBER_ID);
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

int AGIDDS::UnionMemberTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::UnionMember* structData = static_cast<AGIDDS::UnionMember*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (AGIDDS::MemberTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AGIDDS::UnionMember\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::LABEL_UNIONMEMBER_MEMBER_ID:
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
                fprintf(stderr, "Deserialization failed for field: AGIDDS::UnionMember\n");
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

TypeObject* AGIDDS::UnionMemberTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::UnionMemberTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::UnionMember* structData = static_cast<AGIDDS::UnionMember*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::UnionMemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::UnionMemberTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::UnionMemberTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::UnionMemberTypeSupport::get_typename() {
    return "AGIDDS::UnionMember";
}

AGIDDS::UnionTypeTypeSupport* AGIDDS::UnionTypeTypeSupport::get_instance() {
    static AGIDDS::UnionTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::UnionTypeTypeSupport::create_data() {
    return new AGIDDS::UnionType();
}

void AGIDDS::UnionTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::UnionType*>(data);
}

int AGIDDS::UnionTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::UnionType* dstData = static_cast<AGIDDS::UnionType*>(dst);
    AGIDDS::UnionType* srcData = static_cast<AGIDDS::UnionType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::UnionTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::UnionType* structData = static_cast<AGIDDS::UnionType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += AGIDDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += AGIDDS::UnionMemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::UnionTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::UnionTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::UnionType* structData = static_cast<AGIDDS::UnionType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->label\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::MEMBER_UNIONTYPE_MEMBER_ID);
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
        if (AGIDDS::UnionMemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AGIDDS::UnionTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::UnionType* structData = static_cast<AGIDDS::UnionType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->label\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AGIDDS::UnionType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::MEMBER_UNIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->member\n");
                return -1;
            }
            structData->member.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (AGIDDS::UnionMemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::UnionType\n");
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

TypeObject* AGIDDS::UnionTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::UnionTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::UnionType* structData = static_cast<AGIDDS::UnionType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::UnionTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::UnionTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::UnionTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::UnionTypeTypeSupport::get_typename() {
    return "AGIDDS::UnionType";
}

AGIDDS::AnnotationMemberTypeSupport* AGIDDS::AnnotationMemberTypeSupport::get_instance() {
    static AGIDDS::AnnotationMemberTypeSupport instance;
    return &instance;
}

void* AGIDDS::AnnotationMemberTypeSupport::create_data() {
    return new AGIDDS::AnnotationMember();
}

void AGIDDS::AnnotationMemberTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::AnnotationMember*>(data);
}

int AGIDDS::AnnotationMemberTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::AnnotationMember* dstData = static_cast<AGIDDS::AnnotationMember*>(dst);
    AGIDDS::AnnotationMember* srcData = static_cast<AGIDDS::AnnotationMember*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::AnnotationMemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::AnnotationMember* structData = static_cast<AGIDDS::AnnotationMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += AGIDDS::MemberTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::Member*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::AnnotationMemberValueTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->default_value, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::AnnotationMemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::AnnotationMember* structData = static_cast<AGIDDS::AnnotationMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::AnnotationMemberValueTypeSupport::get_instance()->get_max_serialized_data_size(&structData->default_value, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

int AGIDDS::AnnotationMemberTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AnnotationMember* structData = static_cast<AGIDDS::AnnotationMember*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::DEFAULT_VALUE_ANNOTATIONMEMBER_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->default_value\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (AGIDDS::AnnotationMemberValueTypeSupport::get_instance()->serialize_data((void*)&structData->default_value, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->default_value\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AGIDDS::AnnotationMemberTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::AnnotationMember* structData = static_cast<AGIDDS::AnnotationMember*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (AGIDDS::MemberTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AGIDDS::AnnotationMember\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::DEFAULT_VALUE_ANNOTATIONMEMBER_MEMBER_ID:
            if (AGIDDS::AnnotationMemberValueTypeSupport::get_instance()->deserialize_data(&structData->default_value, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->default_value\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::AnnotationMember\n");
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

TypeObject* AGIDDS::AnnotationMemberTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::AnnotationMemberTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AnnotationMember* structData = static_cast<AGIDDS::AnnotationMember*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::AnnotationMemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::AnnotationMemberTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::AnnotationMemberTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::AnnotationMemberTypeSupport::get_typename() {
    return "AGIDDS::AnnotationMember";
}

AGIDDS::AnnotationTypeTypeSupport* AGIDDS::AnnotationTypeTypeSupport::get_instance() {
    static AGIDDS::AnnotationTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::AnnotationTypeTypeSupport::create_data() {
    return new AGIDDS::AnnotationType();
}

void AGIDDS::AnnotationTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::AnnotationType*>(data);
}

int AGIDDS::AnnotationTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::AnnotationType* dstData = static_cast<AGIDDS::AnnotationType*>(dst);
    AGIDDS::AnnotationType* srcData = static_cast<AGIDDS::AnnotationType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::AnnotationTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::AnnotationType* structData = static_cast<AGIDDS::AnnotationType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += AGIDDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += AGIDDS::AnnotationMemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::AnnotationTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::AnnotationTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AnnotationType* structData = static_cast<AGIDDS::AnnotationType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->default_value\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::BASE_TYPE_ANNOTATIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->base_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::MEMBER_ANNOTATIONTYPE_MEMBER_ID);
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
        if (AGIDDS::AnnotationMemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AGIDDS::AnnotationTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::AnnotationType* structData = static_cast<AGIDDS::AnnotationType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->default_value\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AGIDDS::AnnotationType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::BASE_TYPE_ANNOTATIONTYPE_MEMBER_ID:
            if (AGIDDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->base_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
                return -1;
            }
            break;
        case AGIDDS::MEMBER_ANNOTATIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->member\n");
                return -1;
            }
            structData->member.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (AGIDDS::AnnotationMemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::AnnotationType\n");
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

TypeObject* AGIDDS::AnnotationTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::AnnotationTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AnnotationType* structData = static_cast<AGIDDS::AnnotationType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::AnnotationTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::AnnotationTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::AnnotationTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::AnnotationTypeTypeSupport::get_typename() {
    return "AGIDDS::AnnotationType";
}

AGIDDS::AliasTypeTypeSupport* AGIDDS::AliasTypeTypeSupport::get_instance() {
    static AGIDDS::AliasTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::AliasTypeTypeSupport::create_data() {
    return new AGIDDS::AliasType();
}

void AGIDDS::AliasTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::AliasType*>(data);
}

int AGIDDS::AliasTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::AliasType* dstData = static_cast<AGIDDS::AliasType*>(dst);
    AGIDDS::AliasType* srcData = static_cast<AGIDDS::AliasType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::AliasTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::AliasType* structData = static_cast<AGIDDS::AliasType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += AGIDDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::AliasTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::AliasType* structData = static_cast<AGIDDS::AliasType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

int AGIDDS::AliasTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AliasType* structData = static_cast<AGIDDS::AliasType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::BASE_TYPE_ALIASTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->base_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AGIDDS::AliasTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::AliasType* structData = static_cast<AGIDDS::AliasType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AGIDDS::AliasType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::BASE_TYPE_ALIASTYPE_MEMBER_ID:
            if (AGIDDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->base_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::AliasType\n");
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

TypeObject* AGIDDS::AliasTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::AliasTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::AliasType* structData = static_cast<AGIDDS::AliasType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::AliasTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::AliasTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::AliasTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::AliasTypeTypeSupport::get_typename() {
    return "AGIDDS::AliasType";
}

AGIDDS::CollectionTypeTypeSupport* AGIDDS::CollectionTypeTypeSupport::get_instance() {
    static AGIDDS::CollectionTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::CollectionTypeTypeSupport::create_data() {
    return new AGIDDS::CollectionType();
}

void AGIDDS::CollectionTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::CollectionType*>(data);
}

int AGIDDS::CollectionTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::CollectionType* dstData = static_cast<AGIDDS::CollectionType*>(dst);
    AGIDDS::CollectionType* srcData = static_cast<AGIDDS::CollectionType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::CollectionTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::CollectionType* structData = static_cast<AGIDDS::CollectionType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += AGIDDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->element_type, currentAlignment);
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

unsigned int AGIDDS::CollectionTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::CollectionType* structData = static_cast<AGIDDS::CollectionType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->element_type, currentAlignment);
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

int AGIDDS::CollectionTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::CollectionType* structData = static_cast<AGIDDS::CollectionType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::ELEMENT_TYPE_COLLECTIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->element_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->element_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->element_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::ELEMENT_SHARED_COLLECTIONTYPE_MEMBER_ID);
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

int AGIDDS::CollectionTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::CollectionType* structData = static_cast<AGIDDS::CollectionType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AGIDDS::CollectionType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::ELEMENT_TYPE_COLLECTIONTYPE_MEMBER_ID:
            if (AGIDDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->element_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->element_type\n");
                return -1;
            }
            break;
        case AGIDDS::ELEMENT_SHARED_COLLECTIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(structData->element_shared)) {
                fprintf(stderr, "Deserialization failed for field: structData->element_shared\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::CollectionType\n");
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

TypeObject* AGIDDS::CollectionTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::CollectionTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::CollectionType* structData = static_cast<AGIDDS::CollectionType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::CollectionTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::CollectionTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::CollectionTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::CollectionTypeTypeSupport::get_typename() {
    return "AGIDDS::CollectionType";
}

AGIDDS::ArrayTypeTypeSupport* AGIDDS::ArrayTypeTypeSupport::get_instance() {
    static AGIDDS::ArrayTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::ArrayTypeTypeSupport::create_data() {
    return new AGIDDS::ArrayType();
}

void AGIDDS::ArrayTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::ArrayType*>(data);
}

int AGIDDS::ArrayTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::ArrayType* dstData = static_cast<AGIDDS::ArrayType*>(dst);
    AGIDDS::ArrayType* srcData = static_cast<AGIDDS::ArrayType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::ArrayTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::ArrayType* structData = static_cast<AGIDDS::ArrayType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += AGIDDS::CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), currentAlignment);
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

unsigned int AGIDDS::ArrayTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::ArrayTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::ArrayType* structData = static_cast<AGIDDS::ArrayType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->element_shared\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::BOUND_ARRAYTYPE_MEMBER_ID);
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

int AGIDDS::ArrayTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::ArrayType* structData = static_cast<AGIDDS::ArrayType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->element_shared\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AGIDDS::ArrayType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::BOUND_ARRAYTYPE_MEMBER_ID:
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
                fprintf(stderr, "Deserialization failed for field: AGIDDS::ArrayType\n");
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

TypeObject* AGIDDS::ArrayTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::ArrayTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::ArrayType* structData = static_cast<AGIDDS::ArrayType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::ArrayTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::ArrayTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::ArrayTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::ArrayTypeTypeSupport::get_typename() {
    return "AGIDDS::ArrayType";
}

AGIDDS::MapTypeTypeSupport* AGIDDS::MapTypeTypeSupport::get_instance() {
    static AGIDDS::MapTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::MapTypeTypeSupport::create_data() {
    return new AGIDDS::MapType();
}

void AGIDDS::MapTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::MapType*>(data);
}

int AGIDDS::MapTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::MapType* dstData = static_cast<AGIDDS::MapType*>(dst);
    AGIDDS::MapType* srcData = static_cast<AGIDDS::MapType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::MapTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::MapType* structData = static_cast<AGIDDS::MapType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDS::CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), currentAlignment);
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->key_element_type, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::MapTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::MapType* structData = static_cast<AGIDDS::MapType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->key_element_type, currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int AGIDDS::MapTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::MapType* structData = static_cast<AGIDDS::MapType*>(data);
    uint32_t tmpLength = 0;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->bound[i]\n");
        return -1;
    }
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->key_element_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->key_element_type\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

int AGIDDS::MapTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::MapType* structData = static_cast<AGIDDS::MapType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->bound[i]\n");
        return -1;
    }
    if (AGIDDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->key_element_type, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->key_element_type\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->bound)) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

TypeObject* AGIDDS::MapTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::MapTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::MapType* structData = static_cast<AGIDDS::MapType*>(data);
    bool memberHasKey = false;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::MapTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::MapTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::MapTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::MapTypeTypeSupport::get_typename() {
    return "AGIDDS::MapType";
}

AGIDDS::SequenceTypeTypeSupport* AGIDDS::SequenceTypeTypeSupport::get_instance() {
    static AGIDDS::SequenceTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::SequenceTypeTypeSupport::create_data() {
    return new AGIDDS::SequenceType();
}

void AGIDDS::SequenceTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::SequenceType*>(data);
}

int AGIDDS::SequenceTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::SequenceType* dstData = static_cast<AGIDDS::SequenceType*>(dst);
    AGIDDS::SequenceType* srcData = static_cast<AGIDDS::SequenceType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::SequenceTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::SequenceType* structData = static_cast<AGIDDS::SequenceType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDS::CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::SequenceTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::SequenceType* structData = static_cast<AGIDDS::SequenceType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int AGIDDS::SequenceTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::SequenceType* structData = static_cast<AGIDDS::SequenceType*>(data);
    uint32_t tmpLength = 0;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

int AGIDDS::SequenceTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::SequenceType* structData = static_cast<AGIDDS::SequenceType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->bound)) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

TypeObject* AGIDDS::SequenceTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::SequenceTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::SequenceType* structData = static_cast<AGIDDS::SequenceType*>(data);
    bool memberHasKey = false;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::SequenceTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::SequenceTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::SequenceTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::SequenceTypeTypeSupport::get_typename() {
    return "AGIDDS::SequenceType";
}

AGIDDS::StringTypeTypeSupport* AGIDDS::StringTypeTypeSupport::get_instance() {
    static AGIDDS::StringTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::StringTypeTypeSupport::create_data() {
    return new AGIDDS::StringType();
}

void AGIDDS::StringTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::StringType*>(data);
}

int AGIDDS::StringTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::StringType* dstData = static_cast<AGIDDS::StringType*>(dst);
    AGIDDS::StringType* srcData = static_cast<AGIDDS::StringType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::StringTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::StringType* structData = static_cast<AGIDDS::StringType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDS::CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::StringTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::StringType* structData = static_cast<AGIDDS::StringType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int AGIDDS::StringTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::StringType* structData = static_cast<AGIDDS::StringType*>(data);
    uint32_t tmpLength = 0;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

int AGIDDS::StringTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::StringType* structData = static_cast<AGIDDS::StringType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->bound)) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

TypeObject* AGIDDS::StringTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::StringTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::StringType* structData = static_cast<AGIDDS::StringType*>(data);
    bool memberHasKey = false;
    if (AGIDDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::StringTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::StringTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::StringTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::StringTypeTypeSupport::get_typename() {
    return "AGIDDS::StringType";
}

AGIDDS::BitTypeSupport* AGIDDS::BitTypeSupport::get_instance() {
    static AGIDDS::BitTypeSupport instance;
    return &instance;
}

void* AGIDDS::BitTypeSupport::create_data() {
    return new AGIDDS::Bit();
}

void AGIDDS::BitTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::Bit*>(data);
}

int AGIDDS::BitTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::Bit* dstData = static_cast<AGIDDS::Bit*>(dst);
    AGIDDS::Bit* srcData = static_cast<AGIDDS::Bit*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::BitTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::Bit* structData = static_cast<AGIDDS::Bit*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::BitTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::Bit* structData = static_cast<AGIDDS::Bit*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int AGIDDS::BitTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::Bit* structData = static_cast<AGIDDS::Bit*>(data);
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

int AGIDDS::BitTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::Bit* structData = static_cast<AGIDDS::Bit*>(data);
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

TypeObject* AGIDDS::BitTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::BitTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::Bit* structData = static_cast<AGIDDS::Bit*>(data);
    bool memberHasKey = false;
    return 0;
}

int AGIDDS::BitTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::BitTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::BitTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::BitTypeSupport::get_typename() {
    return "AGIDDS::Bit";
}

AGIDDS::BitSetTypeTypeSupport* AGIDDS::BitSetTypeTypeSupport::get_instance() {
    static AGIDDS::BitSetTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::BitSetTypeTypeSupport::create_data() {
    return new AGIDDS::BitSetType();
}

void AGIDDS::BitSetTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::BitSetType*>(data);
}

int AGIDDS::BitSetTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::BitSetType* dstData = static_cast<AGIDDS::BitSetType*>(dst);
    AGIDDS::BitSetType* srcData = static_cast<AGIDDS::BitSetType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::BitSetTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::BitSetType* structData = static_cast<AGIDDS::BitSetType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += AGIDDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::Type*>(structData), currentAlignment);
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
        currentAlignment += AGIDDS::BitTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->bit[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::BitSetTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::BitSetTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::BitSetType* structData = static_cast<AGIDDS::BitSetType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::BIT_BOUND_BITSETTYPE_MEMBER_ID);
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
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::BIT_BITSETTYPE_MEMBER_ID);
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
        if (AGIDDS::BitTypeSupport::get_instance()->serialize_data((void*)&structData->bit[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->bit[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AGIDDS::BitSetTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::BitSetType* structData = static_cast<AGIDDS::BitSetType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AGIDDS::BitSetType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::BIT_BOUND_BITSETTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(structData->bit_bound)) {
                fprintf(stderr, "Deserialization failed for field: structData->bit_bound\n");
                return -1;
            }
            break;
        case AGIDDS::BIT_BITSETTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->bit\n");
                return -1;
            }
            structData->bit.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (AGIDDS::BitTypeSupport::get_instance()->deserialize_data(&structData->bit[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->bit[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::BitSetType\n");
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

TypeObject* AGIDDS::BitSetTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::BitSetTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::BitSetType* structData = static_cast<AGIDDS::BitSetType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::BitSetTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::BitSetTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::BitSetTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::BitSetTypeTypeSupport::get_typename() {
    return "AGIDDS::BitSetType";
}

AGIDDS::EnumeratedConstantTypeSupport* AGIDDS::EnumeratedConstantTypeSupport::get_instance() {
    static AGIDDS::EnumeratedConstantTypeSupport instance;
    return &instance;
}

void* AGIDDS::EnumeratedConstantTypeSupport::create_data() {
    return new AGIDDS::EnumeratedConstant();
}

void AGIDDS::EnumeratedConstantTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::EnumeratedConstant*>(data);
}

int AGIDDS::EnumeratedConstantTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::EnumeratedConstant* dstData = static_cast<AGIDDS::EnumeratedConstant*>(dst);
    AGIDDS::EnumeratedConstant* srcData = static_cast<AGIDDS::EnumeratedConstant*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::EnumeratedConstantTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::EnumeratedConstant* structData = static_cast<AGIDDS::EnumeratedConstant*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::EnumeratedConstantTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::EnumeratedConstant* structData = static_cast<AGIDDS::EnumeratedConstant*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int AGIDDS::EnumeratedConstantTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::EnumeratedConstant* structData = static_cast<AGIDDS::EnumeratedConstant*>(data);
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

int AGIDDS::EnumeratedConstantTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::EnumeratedConstant* structData = static_cast<AGIDDS::EnumeratedConstant*>(data);
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

TypeObject* AGIDDS::EnumeratedConstantTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::EnumeratedConstantTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::EnumeratedConstant* structData = static_cast<AGIDDS::EnumeratedConstant*>(data);
    bool memberHasKey = false;
    return 0;
}

int AGIDDS::EnumeratedConstantTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::EnumeratedConstantTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::EnumeratedConstantTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::EnumeratedConstantTypeSupport::get_typename() {
    return "AGIDDS::EnumeratedConstant";
}

AGIDDS::EnumerationTypeTypeSupport* AGIDDS::EnumerationTypeTypeSupport::get_instance() {
    static AGIDDS::EnumerationTypeTypeSupport instance;
    return &instance;
}

void* AGIDDS::EnumerationTypeTypeSupport::create_data() {
    return new AGIDDS::EnumerationType();
}

void AGIDDS::EnumerationTypeTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::EnumerationType*>(data);
}

int AGIDDS::EnumerationTypeTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::EnumerationType* dstData = static_cast<AGIDDS::EnumerationType*>(dst);
    AGIDDS::EnumerationType* srcData = static_cast<AGIDDS::EnumerationType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::EnumerationTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::EnumerationType* structData = static_cast<AGIDDS::EnumerationType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += AGIDDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<AGIDDS::Type*>(structData), currentAlignment);
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
        currentAlignment += AGIDDS::EnumeratedConstantTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->constant[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::EnumerationTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::EnumerationTypeTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::EnumerationType* structData = static_cast<AGIDDS::EnumerationType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::BIT_BOUND_ENUMERATIONTYPE_MEMBER_ID);
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
    lenAddr = cdr->serializeParameterHeader(useExtend, true, AGIDDS::CONSTANT_ENUMERATIONTYPE_MEMBER_ID);
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
        if (AGIDDS::EnumeratedConstantTypeSupport::get_instance()->serialize_data((void*)&structData->constant[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->constant[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AGIDDS::EnumerationTypeTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::EnumerationType* structData = static_cast<AGIDDS::EnumerationType*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: AGIDDS::EnumerationType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case AGIDDS::BIT_BOUND_ENUMERATIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(structData->bit_bound)) {
                fprintf(stderr, "Deserialization failed for field: structData->bit_bound\n");
                return -1;
            }
            break;
        case AGIDDS::CONSTANT_ENUMERATIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->constant\n");
                return -1;
            }
            structData->constant.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (AGIDDS::EnumeratedConstantTypeSupport::get_instance()->deserialize_data(&structData->constant[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->constant[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::EnumerationType\n");
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

TypeObject* AGIDDS::EnumerationTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::EnumerationTypeTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::EnumerationType* structData = static_cast<AGIDDS::EnumerationType*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (AGIDDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<AGIDDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int AGIDDS::EnumerationTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::EnumerationTypeTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::EnumerationTypeTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::EnumerationTypeTypeSupport::get_typename() {
    return "AGIDDS::EnumerationType";
}

AGIDDS::_ModuleTypeSupport* AGIDDS::_ModuleTypeSupport::get_instance() {
    static AGIDDS::_ModuleTypeSupport instance;
    return &instance;
}

void* AGIDDS::_ModuleTypeSupport::create_data() {
    return new AGIDDS::_Module();
}

void AGIDDS::_ModuleTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::_Module*>(data);
}

int AGIDDS::_ModuleTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::_Module* dstData = static_cast<AGIDDS::_Module*>(dst);
    AGIDDS::_Module* srcData = static_cast<AGIDDS::_Module*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::_ModuleTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::_Module* structData = static_cast<AGIDDS::_Module*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::_ModuleTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::_Module* structData = static_cast<AGIDDS::_Module*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int AGIDDS::_ModuleTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::_Module* structData = static_cast<AGIDDS::_Module*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int AGIDDS::_ModuleTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::_Module* structData = static_cast<AGIDDS::_Module*>(data);
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

TypeObject* AGIDDS::_ModuleTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::_ModuleTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::_Module* structData = static_cast<AGIDDS::_Module*>(data);
    bool memberHasKey = false;
    return 0;
}

int AGIDDS::_ModuleTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::_ModuleTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::_ModuleTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::_ModuleTypeSupport::get_typename() {
    return "AGIDDS::_Module";
}

AGIDDS::TypeLibraryElementTypeSupport* AGIDDS::TypeLibraryElementTypeSupport::get_instance() {
    static AGIDDS::TypeLibraryElementTypeSupport instance;
    return &instance;
}

void* AGIDDS::TypeLibraryElementTypeSupport::create_data() {
    return new AGIDDS::TypeLibraryElement();
}

void AGIDDS::TypeLibraryElementTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::TypeLibraryElement*>(data);
}

int AGIDDS::TypeLibraryElementTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::TypeLibraryElement* dstData = static_cast<AGIDDS::TypeLibraryElement*>(dst);
    AGIDDS::TypeLibraryElement* srcData = static_cast<AGIDDS::TypeLibraryElement*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::TypeLibraryElementTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::TypeLibraryElement* structData = static_cast<AGIDDS::TypeLibraryElement*>(data);
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
    case AGIDDS::ALIAS_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::AliasTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->alias_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::ANNOTATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::AnnotationTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->annotation_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::ARRAY_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::ArrayTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->array_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::BITSET_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::BitSetTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->bitset_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::ENUMERATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::EnumerationTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->enumeration_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::MAP_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::MapTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->map_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::SEQUENCE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::SequenceTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->sequence_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::STRING_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::StringTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->string_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::STRUCTURE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::StructureTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->structure_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::UNION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::UnionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->union_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::MODULE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::_ModuleTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->mod, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    }
    return 0;
}

unsigned int AGIDDS::TypeLibraryElementTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::TypeLibraryElement* structData = static_cast<AGIDDS::TypeLibraryElement*>(data);
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
    case AGIDDS::ALIAS_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::AliasTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->alias_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::ANNOTATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::AnnotationTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->annotation_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::ARRAY_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::ArrayTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->array_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::BITSET_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::BitSetTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->bitset_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::ENUMERATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::EnumerationTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->enumeration_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::MAP_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::MapTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->map_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::SEQUENCE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::SequenceTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->sequence_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::STRING_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::StringTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->string_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::STRUCTURE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::StructureTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->structure_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::UNION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::UnionTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->union_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case AGIDDS::MODULE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += AGIDDS::_ModuleTypeSupport::get_instance()->get_max_serialized_data_size(&structData->mod, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    }
    return 0;
}

int AGIDDS::TypeLibraryElementTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::TypeLibraryElement* structData = static_cast<AGIDDS::TypeLibraryElement*>(data);
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
    case AGIDDS::ALIAS_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 1);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->alias_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::AliasTypeTypeSupport::get_instance()->serialize_data((void*)&structData->alias_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->alias_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case AGIDDS::ANNOTATION_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 2);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->annotation_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::AnnotationTypeTypeSupport::get_instance()->serialize_data((void*)&structData->annotation_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->annotation_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case AGIDDS::ARRAY_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 3);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->array_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::ArrayTypeTypeSupport::get_instance()->serialize_data((void*)&structData->array_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->array_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case AGIDDS::BITSET_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 4);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->bitset_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::BitSetTypeTypeSupport::get_instance()->serialize_data((void*)&structData->bitset_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->bitset_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case AGIDDS::ENUMERATION_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 5);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->enumeration_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::EnumerationTypeTypeSupport::get_instance()->serialize_data((void*)&structData->enumeration_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->enumeration_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case AGIDDS::MAP_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 6);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->map_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::MapTypeTypeSupport::get_instance()->serialize_data((void*)&structData->map_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->map_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case AGIDDS::SEQUENCE_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 7);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->sequence_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::SequenceTypeTypeSupport::get_instance()->serialize_data((void*)&structData->sequence_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->sequence_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case AGIDDS::STRING_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 8);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->string_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::StringTypeTypeSupport::get_instance()->serialize_data((void*)&structData->string_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->string_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case AGIDDS::STRUCTURE_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 9);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->structure_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::StructureTypeTypeSupport::get_instance()->serialize_data((void*)&structData->structure_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->structure_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case AGIDDS::UNION_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 10);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->union_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::UnionTypeTypeSupport::get_instance()->serialize_data((void*)&structData->union_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->union_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case AGIDDS::MODULE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 11);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->mod\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (AGIDDS::_ModuleTypeSupport::get_instance()->serialize_data((void*)&structData->mod, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->mod\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    }
    return 0;
}

int AGIDDS::TypeLibraryElementTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::TypeLibraryElement* structData = static_cast<AGIDDS::TypeLibraryElement*>(data);
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
            fprintf(stderr, "Deserialization failed for field: AGIDDS::TypeLibraryElement\n");
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
                structData->u_ = static_cast<AGIDDS::TypeLibraryElementKind>(tmpCharEnum);
                break;
            case 2:
                if (!cdr->deserializeBaseType(tmpShortEnum)) {
                    fprintf(stderr, "Deserialization failed for field: structData->u_\n");
                    return -1;
                }
                structData->u_ = static_cast<AGIDDS::TypeLibraryElementKind>(tmpShortEnum);
                break;
            case 4:
                if (!cdr->deserializeBaseType(tmpIntEnum)) {
                    fprintf(stderr, "Deserialization failed for field: structData->u_\n");
                    return -1;
                }
                structData->u_ = static_cast<AGIDDS::TypeLibraryElementKind>(tmpIntEnum);
                break;
            default:
                fprintf(stderr, "Deserialization failed for field: structData->u_\n");
                return -1;
                break;
            }
            break;
        case 1:
            if (AGIDDS::AliasTypeTypeSupport::get_instance()->deserialize_data(&structData->alias_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->alias_type\n");
                return -1;
            }
            break;
        case 2:
            if (AGIDDS::AnnotationTypeTypeSupport::get_instance()->deserialize_data(&structData->annotation_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->annotation_type\n");
                return -1;
            }
            break;
        case 3:
            if (AGIDDS::ArrayTypeTypeSupport::get_instance()->deserialize_data(&structData->array_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->array_type\n");
                return -1;
            }
            break;
        case 4:
            if (AGIDDS::BitSetTypeTypeSupport::get_instance()->deserialize_data(&structData->bitset_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->bitset_type\n");
                return -1;
            }
            break;
        case 5:
            if (AGIDDS::EnumerationTypeTypeSupport::get_instance()->deserialize_data(&structData->enumeration_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->enumeration_type\n");
                return -1;
            }
            break;
        case 6:
            if (AGIDDS::MapTypeTypeSupport::get_instance()->deserialize_data(&structData->map_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->map_type\n");
                return -1;
            }
            break;
        case 7:
            if (AGIDDS::SequenceTypeTypeSupport::get_instance()->deserialize_data(&structData->sequence_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->sequence_type\n");
                return -1;
            }
            break;
        case 8:
            if (AGIDDS::StringTypeTypeSupport::get_instance()->deserialize_data(&structData->string_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->string_type\n");
                return -1;
            }
            break;
        case 9:
            if (AGIDDS::StructureTypeTypeSupport::get_instance()->deserialize_data(&structData->structure_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->structure_type\n");
                return -1;
            }
            break;
        case 10:
            if (AGIDDS::UnionTypeTypeSupport::get_instance()->deserialize_data(&structData->union_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->union_type\n");
                return -1;
            }
            break;
        case 11:
            if (AGIDDS::_ModuleTypeSupport::get_instance()->deserialize_data(&structData->mod, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->mod\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::TypeLibraryElement\n");
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

TypeObject* AGIDDS::TypeLibraryElementTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::TypeLibraryElementTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::TypeLibraryElement* structData = static_cast<AGIDDS::TypeLibraryElement*>(data);
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    return 0;
}

int AGIDDS::TypeLibraryElementTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::TypeLibraryElementTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::TypeLibraryElementTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::TypeLibraryElementTypeSupport::get_typename() {
    return "AGIDDS::TypeLibraryElement";
}

AGIDDS::TypeLibraryTypeSupport* AGIDDS::TypeLibraryTypeSupport::get_instance() {
    static AGIDDS::TypeLibraryTypeSupport instance;
    return &instance;
}

void* AGIDDS::TypeLibraryTypeSupport::create_data() {
    return new AGIDDS::TypeLibrary();
}

void AGIDDS::TypeLibraryTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::TypeLibrary*>(data);
}

int AGIDDS::TypeLibraryTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::TypeLibrary* dstData = static_cast<AGIDDS::TypeLibrary*>(dst);
    AGIDDS::TypeLibrary* srcData = static_cast<AGIDDS::TypeLibrary*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::TypeLibraryTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::TypeLibrary* structData = static_cast<AGIDDS::TypeLibrary*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->element.size(); ++i) {
        currentAlignment += AGIDDS::TypeLibraryElementTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->element[i], currentAlignment);
    }
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::TypeLibraryTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::TypeLibraryTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::TypeLibrary* structData = static_cast<AGIDDS::TypeLibrary*>(data);
    uint32_t tmpLength = 0;
    tmpLength = static_cast<uint32_t>(structData->element.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->element\n");
        return -1;
    }
    for (size_t i = 0; i < structData->element.size(); ++i) {
        if (AGIDDS::TypeLibraryElementTypeSupport::get_instance()->serialize_data((void*)&structData->element[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->element[i]\n");
            return -1;
        }
    }
    return 0;
}

int AGIDDS::TypeLibraryTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::TypeLibrary* structData = static_cast<AGIDDS::TypeLibrary*>(data);
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
        if (AGIDDS::TypeLibraryElementTypeSupport::get_instance()->deserialize_data(&structData->element[i], cdr, endian) != 0) {
            fprintf(stderr, "Deserialization failed for field: structData->element[i]\n");
            return -1;
        }
    }
    return 0;
}

TypeObject* AGIDDS::TypeLibraryTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::TypeLibraryTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::TypeLibrary* structData = static_cast<AGIDDS::TypeLibrary*>(data);
    bool memberHasKey = false;
    return 0;
}

int AGIDDS::TypeLibraryTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::TypeLibraryTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::TypeLibraryTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::TypeLibraryTypeSupport::get_typename() {
    return "AGIDDS::TypeLibrary";
}

AGIDDS::TypeObjectTypeSupport* AGIDDS::TypeObjectTypeSupport::get_instance() {
    static AGIDDS::TypeObjectTypeSupport instance;
    return &instance;
}

void* AGIDDS::TypeObjectTypeSupport::create_data() {
    return new AGIDDS::TypeObject();
}

void AGIDDS::TypeObjectTypeSupport::delete_data(void* data) {
    delete static_cast<AGIDDS::TypeObject*>(data);
}

int AGIDDS::TypeObjectTypeSupport::copy_data(void* dst, void* src) {
    AGIDDS::TypeObject* dstData = static_cast<AGIDDS::TypeObject*>(dst);
    AGIDDS::TypeObject* srcData = static_cast<AGIDDS::TypeObject*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int AGIDDS::TypeObjectTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    AGIDDS::TypeObject* structData = static_cast<AGIDDS::TypeObject*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDS::TypeLibraryTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->library, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->the_type.size(); ++i) {
        currentAlignment += AGIDDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->the_type[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int AGIDDS::TypeObjectTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int AGIDDS::TypeObjectTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::TypeObject* structData = static_cast<AGIDDS::TypeObject*>(data);
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
    if (AGIDDS::TypeLibraryTypeSupport::get_instance()->serialize_data((void*)&structData->library, cdr, endian) != 0) {
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
        if (AGIDDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->the_type[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->the_type[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int AGIDDS::TypeObjectTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    AGIDDS::TypeObject* structData = static_cast<AGIDDS::TypeObject*>(data);
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
            fprintf(stderr, "Deserialization failed for field: AGIDDS::TypeObject\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case 0:
            if (AGIDDS::TypeLibraryTypeSupport::get_instance()->deserialize_data(&structData->library, cdr, endian) != 0) {
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
                if (AGIDDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->the_type[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->the_type[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: AGIDDS::TypeObject\n");
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

TypeObject* AGIDDS::TypeObjectTypeSupport::get_typeobject() {
    return nullptr;
}

int AGIDDS::TypeObjectTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    AGIDDS::TypeObject* structData = static_cast<AGIDDS::TypeObject*>(data);
    bool memberHasKey = false;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    return 0;
}

int AGIDDS::TypeObjectTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t AGIDDS::TypeObjectTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool AGIDDS::TypeObjectTypeSupport::has_key() {
    return false;
}

const char* AGIDDS::TypeObjectTypeSupport::get_typename() {
    return "AGIDDS::TypeObject";
}

#ifdef __cplusplus
}
#endif
