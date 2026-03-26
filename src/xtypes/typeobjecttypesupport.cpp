#include "xtypes/typeobjecttypesupport.h"

USING_TRAVODDS_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

TRAVODDS::_TypeIdTypeSupport* TRAVODDS::_TypeIdTypeSupport::get_instance() {
    static TRAVODDS::_TypeIdTypeSupport instance;
    return &instance;
}

void* TRAVODDS::_TypeIdTypeSupport::create_data() {
    return new TRAVODDS::_TypeId();
}

void TRAVODDS::_TypeIdTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::_TypeId*>(data);
}

int TRAVODDS::_TypeIdTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::_TypeId* dstData = static_cast<TRAVODDS::_TypeId*>(dst);
    TRAVODDS::_TypeId* srcData = static_cast<TRAVODDS::_TypeId*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::_TypeIdTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::_TypeId* structData = static_cast<TRAVODDS::_TypeId*>(data);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
    switch(structData->u_) {
    case TRAVODDS::BOOLEAN_TYPE:
    case TRAVODDS::BYTE_TYPE:
    case TRAVODDS::INT_16_TYPE:
    case TRAVODDS::UINT_16_TYPE:
    case TRAVODDS::INT_32_TYPE:
    case TRAVODDS::UINT_32_TYPE:
    case TRAVODDS::INT_64_TYPE:
    case TRAVODDS::UINT_64_TYPE:
    case TRAVODDS::FLOAT_32_TYPE:
    case TRAVODDS::FLOAT_64_TYPE:
    case TRAVODDS::FLOAT_128_TYPE:
    case TRAVODDS::CHAR_8_TYPE:
    case TRAVODDS::CHAR_32_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
        break;
    default:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
        break;
    }
    return 0;
}

unsigned int TRAVODDS::_TypeIdTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::_TypeId* structData = static_cast<TRAVODDS::_TypeId*>(data);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
    switch(structData->u_) {
    case TRAVODDS::BOOLEAN_TYPE:
    case TRAVODDS::BYTE_TYPE:
    case TRAVODDS::INT_16_TYPE:
    case TRAVODDS::UINT_16_TYPE:
    case TRAVODDS::INT_32_TYPE:
    case TRAVODDS::UINT_32_TYPE:
    case TRAVODDS::INT_64_TYPE:
    case TRAVODDS::UINT_64_TYPE:
    case TRAVODDS::FLOAT_32_TYPE:
    case TRAVODDS::FLOAT_64_TYPE:
    case TRAVODDS::FLOAT_128_TYPE:
    case TRAVODDS::CHAR_8_TYPE:
    case TRAVODDS::CHAR_32_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
        break;
    default:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
        break;
    }
    return 0;
}

int TRAVODDS::_TypeIdTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    (void)endian;
    TRAVODDS::_TypeId* structData = static_cast<TRAVODDS::_TypeId*>(data);
    if (!cdr->serializeBaseType(structData->u_)) {
        fprintf(stderr, "Serialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case TRAVODDS::BOOLEAN_TYPE:
    case TRAVODDS::BYTE_TYPE:
    case TRAVODDS::INT_16_TYPE:
    case TRAVODDS::UINT_16_TYPE:
    case TRAVODDS::INT_32_TYPE:
    case TRAVODDS::UINT_32_TYPE:
    case TRAVODDS::INT_64_TYPE:
    case TRAVODDS::UINT_64_TYPE:
    case TRAVODDS::FLOAT_32_TYPE:
    case TRAVODDS::FLOAT_64_TYPE:
    case TRAVODDS::FLOAT_128_TYPE:
    case TRAVODDS::CHAR_8_TYPE:
    case TRAVODDS::CHAR_32_TYPE:
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

int TRAVODDS::_TypeIdTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    (void)endian;
    TRAVODDS::_TypeId* structData = static_cast<TRAVODDS::_TypeId*>(data);
    if (!cdr->deserializeBaseType(structData->u_)) {
        fprintf(stderr, "Deserialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case TRAVODDS::BOOLEAN_TYPE:
    case TRAVODDS::BYTE_TYPE:
    case TRAVODDS::INT_16_TYPE:
    case TRAVODDS::UINT_16_TYPE:
    case TRAVODDS::INT_32_TYPE:
    case TRAVODDS::UINT_32_TYPE:
    case TRAVODDS::INT_64_TYPE:
    case TRAVODDS::UINT_64_TYPE:
    case TRAVODDS::FLOAT_32_TYPE:
    case TRAVODDS::FLOAT_64_TYPE:
    case TRAVODDS::FLOAT_128_TYPE:
    case TRAVODDS::CHAR_8_TYPE:
    case TRAVODDS::CHAR_32_TYPE:
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

TypeObject* TRAVODDS::_TypeIdTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::_TypeIdTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    return 0;
}

int TRAVODDS::_TypeIdTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::_TypeIdTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::_TypeIdTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::_TypeIdTypeSupport::get_typename() {
    return "TRAVODDS::_TypeId";
}

TRAVODDS::AnnotationMemberValueTypeSupport* TRAVODDS::AnnotationMemberValueTypeSupport::get_instance() {
    static TRAVODDS::AnnotationMemberValueTypeSupport instance;
    return &instance;
}

void* TRAVODDS::AnnotationMemberValueTypeSupport::create_data() {
    return new TRAVODDS::AnnotationMemberValue();
}

void TRAVODDS::AnnotationMemberValueTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::AnnotationMemberValue*>(data);
}

int TRAVODDS::AnnotationMemberValueTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::AnnotationMemberValue* dstData = static_cast<TRAVODDS::AnnotationMemberValue*>(dst);
    TRAVODDS::AnnotationMemberValue* srcData = static_cast<TRAVODDS::AnnotationMemberValue*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::AnnotationMemberValueTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::AnnotationMemberValue* structData = static_cast<TRAVODDS::AnnotationMemberValue*>(data);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
    switch(structData->u_) {
    case TRAVODDS::BOOLEAN_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
        break;
    case TRAVODDS::BYTE_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned char), currentAlignment);
        break;
    case TRAVODDS::INT_16_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
        break;
    case TRAVODDS::UINT_16_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned short), currentAlignment);
        break;
    case TRAVODDS::INT_32_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
        break;
    case TRAVODDS::UINT_32_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
        break;
    case TRAVODDS::INT_64_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(long long), currentAlignment);
        break;
    case TRAVODDS::UINT_64_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
        break;
    case TRAVODDS::FLOAT_32_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(float), currentAlignment);
        break;
    case TRAVODDS::FLOAT_64_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(double), currentAlignment);
        break;
    case TRAVODDS::FLOAT_128_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(LongDouble), currentAlignment);
        break;
    case TRAVODDS::CHAR_8_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
        break;
    case TRAVODDS::CHAR_32_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(wchar_t), currentAlignment);
        break;
    case TRAVODDS::ENUMERATION_TYPE:
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
        break;
    case TRAVODDS::STRING_TYPE:
        currentAlignment += CdrSerializer::getStringSize(static_cast<uint32_t>(structData->string_value.size()) + 1, currentAlignment);
        break;
    }
    return 0;
}

unsigned int TRAVODDS::AnnotationMemberValueTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::AnnotationMemberValueTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    (void)endian;
    TRAVODDS::AnnotationMemberValue* structData = static_cast<TRAVODDS::AnnotationMemberValue*>(data);
    if (!cdr->serializeBaseType(structData->u_)) {
        fprintf(stderr, "Serialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case TRAVODDS::BOOLEAN_TYPE:
        if (!cdr->serializeBaseType(structData->boolean_value)) {
            fprintf(stderr, "Serialization failed for field: structData->boolean_value\n");
            return -1;
        }
        break;
    case TRAVODDS::BYTE_TYPE:
        if (!cdr->serializeBaseType(structData->byte_value)) {
            fprintf(stderr, "Serialization failed for field: structData->byte_value\n");
            return -1;
        }
        break;
    case TRAVODDS::INT_16_TYPE:
        if (!cdr->serializeBaseType(structData->int_16_value)) {
            fprintf(stderr, "Serialization failed for field: structData->int_16_value\n");
            return -1;
        }
        break;
    case TRAVODDS::UINT_16_TYPE:
        if (!cdr->serializeBaseType(structData->uint_16_value)) {
            fprintf(stderr, "Serialization failed for field: structData->uint_16_value\n");
            return -1;
        }
        break;
    case TRAVODDS::INT_32_TYPE:
        if (!cdr->serializeBaseType(structData->int_32_value)) {
            fprintf(stderr, "Serialization failed for field: structData->int_32_value\n");
            return -1;
        }
        break;
    case TRAVODDS::UINT_32_TYPE:
        if (!cdr->serializeBaseType(structData->uint_32_value)) {
            fprintf(stderr, "Serialization failed for field: structData->uint_32_value\n");
            return -1;
        }
        break;
    case TRAVODDS::INT_64_TYPE:
        if (!cdr->serializeBaseType(structData->int_64_value)) {
            fprintf(stderr, "Serialization failed for field: structData->int_64_value\n");
            return -1;
        }
        break;
    case TRAVODDS::UINT_64_TYPE:
        if (!cdr->serializeBaseType(structData->uint_64_value)) {
            fprintf(stderr, "Serialization failed for field: structData->uint_64_value\n");
            return -1;
        }
        break;
    case TRAVODDS::FLOAT_32_TYPE:
        if (!cdr->serializeBaseType(structData->float_32_value)) {
            fprintf(stderr, "Serialization failed for field: structData->float_32_value\n");
            return -1;
        }
        break;
    case TRAVODDS::FLOAT_64_TYPE:
        if (!cdr->serializeBaseType(structData->float_64_value)) {
            fprintf(stderr, "Serialization failed for field: structData->float_64_value\n");
            return -1;
        }
        break;
    case TRAVODDS::FLOAT_128_TYPE:
        if (!cdr->serializeLongDouble(*(LongDouble*)&structData->float_128_value)) {
            fprintf(stderr, "Serialization failed for field: structData->float_128_value\n");
            return -1;
        }
        break;
    case TRAVODDS::CHAR_8_TYPE:
        if (!cdr->serializeBaseType(structData->character_value)) {
            fprintf(stderr, "Serialization failed for field: structData->character_value\n");
            return -1;
        }
        break;
    case TRAVODDS::CHAR_32_TYPE:
        if (!cdr->serializeBaseType(structData->wide_character_value)) {
            fprintf(stderr, "Serialization failed for field: structData->wide_character_value\n");
            return -1;
        }
        break;
    case TRAVODDS::ENUMERATION_TYPE:
        if (!cdr->serializeBaseType(structData->enumeration_value)) {
            fprintf(stderr, "Serialization failed for field: structData->enumeration_value\n");
            return -1;
        }
        break;
    case TRAVODDS::STRING_TYPE:
        if (!cdr->serializeString(structData->string_value)) {
            fprintf(stderr, "Serialization failed for field: structData->string_value\n");
            return -1;
        }
        break;
    }
    return 0;
}

int TRAVODDS::AnnotationMemberValueTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    (void)endian;
    TRAVODDS::AnnotationMemberValue* structData = static_cast<TRAVODDS::AnnotationMemberValue*>(data);
    if (!cdr->deserializeBaseType(structData->u_)) {
        fprintf(stderr, "Deserialization failed for field: structData->u_\n");
        return -1;
    }
    switch(structData->u_) {
    case TRAVODDS::BOOLEAN_TYPE:
        if (!cdr->deserializeBaseType(structData->boolean_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->boolean_value\n");
            return -1;
        }
        break;
    case TRAVODDS::BYTE_TYPE:
        if (!cdr->deserializeBaseType(structData->byte_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->byte_value\n");
            return -1;
        }
        break;
    case TRAVODDS::INT_16_TYPE:
        if (!cdr->deserializeBaseType(structData->int_16_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->int_16_value\n");
            return -1;
        }
        break;
    case TRAVODDS::UINT_16_TYPE:
        if (!cdr->deserializeBaseType(structData->uint_16_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->uint_16_value\n");
            return -1;
        }
        break;
    case TRAVODDS::INT_32_TYPE:
        if (!cdr->deserializeBaseType(structData->int_32_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->int_32_value\n");
            return -1;
        }
        break;
    case TRAVODDS::UINT_32_TYPE:
        if (!cdr->deserializeBaseType(structData->uint_32_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->uint_32_value\n");
            return -1;
        }
        break;
    case TRAVODDS::INT_64_TYPE:
        if (!cdr->deserializeBaseType(structData->int_64_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->int_64_value\n");
            return -1;
        }
        break;
    case TRAVODDS::UINT_64_TYPE:
        if (!cdr->deserializeBaseType(structData->uint_64_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->uint_64_value\n");
            return -1;
        }
        break;
    case TRAVODDS::FLOAT_32_TYPE:
        if (!cdr->deserializeBaseType(structData->float_32_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->float_32_value\n");
            return -1;
        }
        break;
    case TRAVODDS::FLOAT_64_TYPE:
        if (!cdr->deserializeBaseType(structData->float_64_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->float_64_value\n");
            return -1;
        }
        break;
    case TRAVODDS::FLOAT_128_TYPE:
        if (!cdr->deserializeLongDouble(*(LongDouble*)&structData->float_128_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->float_128_value\n");
            return -1;
        }
        break;
    case TRAVODDS::CHAR_8_TYPE:
        if (!cdr->deserializeBaseType(structData->character_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->character_value\n");
            return -1;
        }
        break;
    case TRAVODDS::CHAR_32_TYPE:
        if (!cdr->deserializeBaseType(structData->wide_character_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->wide_character_value\n");
            return -1;
        }
        break;
    case TRAVODDS::ENUMERATION_TYPE:
        if (!cdr->deserializeBaseType(structData->enumeration_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->enumeration_value\n");
            return -1;
        }
        break;
    case TRAVODDS::STRING_TYPE:
        if (!cdr->deserializeString(structData->string_value)) {
            fprintf(stderr, "Deserialization failed for field: structData->string_value\n");
            return -1;
        }
        break;
    }
    return 0;
}

TypeObject* TRAVODDS::AnnotationMemberValueTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::AnnotationMemberValueTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    return 0;
}

int TRAVODDS::AnnotationMemberValueTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::AnnotationMemberValueTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::AnnotationMemberValueTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::AnnotationMemberValueTypeSupport::get_typename() {
    return "TRAVODDS::AnnotationMemberValue";
}

TRAVODDS::AnnotationUsageMemberTypeSupport* TRAVODDS::AnnotationUsageMemberTypeSupport::get_instance() {
    static TRAVODDS::AnnotationUsageMemberTypeSupport instance;
    return &instance;
}

void* TRAVODDS::AnnotationUsageMemberTypeSupport::create_data() {
    return new TRAVODDS::AnnotationUsageMember();
}

void TRAVODDS::AnnotationUsageMemberTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::AnnotationUsageMember*>(data);
}

int TRAVODDS::AnnotationUsageMemberTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::AnnotationUsageMember* dstData = static_cast<TRAVODDS::AnnotationUsageMember*>(dst);
    TRAVODDS::AnnotationUsageMember* srcData = static_cast<TRAVODDS::AnnotationUsageMember*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::AnnotationUsageMemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::AnnotationUsageMember* structData = static_cast<TRAVODDS::AnnotationUsageMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += TRAVODDS::AnnotationMemberValueTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->value, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::AnnotationUsageMemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::AnnotationUsageMember* structData = static_cast<TRAVODDS::AnnotationUsageMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += TRAVODDS::AnnotationMemberValueTypeSupport::get_instance()->get_max_serialized_data_size(&structData->value, currentAlignment);
    return currentAlignment - initialAlignment;
}

int TRAVODDS::AnnotationUsageMemberTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::AnnotationUsageMember* structData = static_cast<TRAVODDS::AnnotationUsageMember*>(data);
    if (!cdr->serializeBaseType(structData->member_id)) {
        fprintf(stderr, "Serialization failed for field: structData->member_id\n");
        return -1;
    }
    if (TRAVODDS::AnnotationMemberValueTypeSupport::get_instance()->serialize_data((void*)&structData->value, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

int TRAVODDS::AnnotationUsageMemberTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::AnnotationUsageMember* structData = static_cast<TRAVODDS::AnnotationUsageMember*>(data);
    if (!cdr->deserializeBaseType(structData->member_id)) {
        fprintf(stderr, "Deserialization failed for field: structData->member_id\n");
        return -1;
    }
    if (TRAVODDS::AnnotationMemberValueTypeSupport::get_instance()->deserialize_data(&structData->value, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

TypeObject* TRAVODDS::AnnotationUsageMemberTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::AnnotationUsageMemberTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    return 0;
}

int TRAVODDS::AnnotationUsageMemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::AnnotationUsageMemberTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::AnnotationUsageMemberTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::AnnotationUsageMemberTypeSupport::get_typename() {
    return "TRAVODDS::AnnotationUsageMember";
}

TRAVODDS::AnnotationUsageTypeSupport* TRAVODDS::AnnotationUsageTypeSupport::get_instance() {
    static TRAVODDS::AnnotationUsageTypeSupport instance;
    return &instance;
}

void* TRAVODDS::AnnotationUsageTypeSupport::create_data() {
    return new TRAVODDS::AnnotationUsage();
}

void TRAVODDS::AnnotationUsageTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::AnnotationUsage*>(data);
}

int TRAVODDS::AnnotationUsageTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::AnnotationUsage* dstData = static_cast<TRAVODDS::AnnotationUsage*>(dst);
    TRAVODDS::AnnotationUsage* srcData = static_cast<TRAVODDS::AnnotationUsage*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::AnnotationUsageTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::AnnotationUsage* structData = static_cast<TRAVODDS::AnnotationUsage*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->type_id, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += TRAVODDS::AnnotationUsageMemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::AnnotationUsageTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::AnnotationUsageTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::AnnotationUsage* structData = static_cast<TRAVODDS::AnnotationUsage*>(data);
    uint32_t tmpLength = 0;
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->type_id\n");
        return -1;
    }
    tmpLength = static_cast<uint32_t>(structData->member.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->member\n");
        return -1;
    }
    for (size_t i = 0; i < structData->member.size(); ++i) {
        if (TRAVODDS::AnnotationUsageMemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    return 0;
}

int TRAVODDS::AnnotationUsageTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::AnnotationUsage* structData = static_cast<TRAVODDS::AnnotationUsage*>(data);
    unsigned int tmpLength = 0;
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->member\n");
        return -1;
    }
    structData->member.resize(tmpLength);
    for (size_t i = 0; i < tmpLength; ++i) {
        if (TRAVODDS::AnnotationUsageMemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    return 0;
}

TypeObject* TRAVODDS::AnnotationUsageTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::AnnotationUsageTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    return 0;
}

int TRAVODDS::AnnotationUsageTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::AnnotationUsageTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::AnnotationUsageTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::AnnotationUsageTypeSupport::get_typename() {
    return "TRAVODDS::AnnotationUsage";
}

TRAVODDS::TypePropertyTypeSupport* TRAVODDS::TypePropertyTypeSupport::get_instance() {
    static TRAVODDS::TypePropertyTypeSupport instance;
    return &instance;
}

void* TRAVODDS::TypePropertyTypeSupport::create_data() {
    return new TRAVODDS::TypeProperty();
}

void TRAVODDS::TypePropertyTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::TypeProperty*>(data);
}

int TRAVODDS::TypePropertyTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::TypeProperty* dstData = static_cast<TRAVODDS::TypeProperty*>(dst);
    TRAVODDS::TypeProperty* srcData = static_cast<TRAVODDS::TypeProperty*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::TypePropertyTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::TypeProperty* structData = static_cast<TRAVODDS::TypeProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += CdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->type_id, currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(static_cast<uint32_t>(structData->name.size()) + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::TypePropertyTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::TypeProperty* structData = static_cast<TRAVODDS::TypeProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += CdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->type_id, currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int TRAVODDS::TypePropertyTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::TypeProperty* structData = static_cast<TRAVODDS::TypeProperty*>(data);
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
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int TRAVODDS::TypePropertyTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::TypeProperty* structData = static_cast<TRAVODDS::TypeProperty*>(data);
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    switch(16 / 8) {
    case 1:
        if (!cdr->deserializeBaseType(tmpCharEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<TRAVODDS::TypeFlag>(tmpCharEnum);
        break;
    case 2:
        if (!cdr->deserializeBaseType(tmpShortEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<TRAVODDS::TypeFlag>(tmpShortEnum);
        break;
    case 4:
        if (!cdr->deserializeBaseType(tmpIntEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<TRAVODDS::TypeFlag>(tmpIntEnum);
        break;
    default:
        fprintf(stderr, "Deserialization failed for field: structData->flag\n");
        return -1;
        break;
    }
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* TRAVODDS::TypePropertyTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::TypePropertyTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    return 0;
}

int TRAVODDS::TypePropertyTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::TypePropertyTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::TypePropertyTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::TypePropertyTypeSupport::get_typename() {
    return "TRAVODDS::TypeProperty";
}

TRAVODDS::TypeTypeSupport* TRAVODDS::TypeTypeSupport::get_instance() {
    static TRAVODDS::TypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::TypeTypeSupport::create_data() {
    return new TRAVODDS::Type();
}

void TRAVODDS::TypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::Type*>(data);
}

int TRAVODDS::TypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::Type* dstData = static_cast<TRAVODDS::Type*>(dst);
    TRAVODDS::Type* srcData = static_cast<TRAVODDS::Type*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::TypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::Type* structData = static_cast<TRAVODDS::Type*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::TypePropertyTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->property, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->annotation.size(); ++i) {
        currentAlignment += TRAVODDS::AnnotationUsageTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->annotation[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::TypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::TypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::Type* structData = static_cast<TRAVODDS::Type*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::PROPERTY_TYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (TRAVODDS::TypePropertyTypeSupport::get_instance()->serialize_data((void*)&structData->property, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::ANNOTATION_TYPE_MEMBER_ID);
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
        if (TRAVODDS::AnnotationUsageTypeSupport::get_instance()->serialize_data((void*)&structData->annotation[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->annotation[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::TypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::Type* structData = static_cast<TRAVODDS::Type*>(data);
    unsigned int tmpLength = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::Type\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::PROPERTY_TYPE_MEMBER_ID:
            if (TRAVODDS::TypePropertyTypeSupport::get_instance()->deserialize_data(&structData->property, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->property\n");
                return -1;
            }
            break;
        case TRAVODDS::ANNOTATION_TYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->annotation\n");
                return -1;
            }
            structData->annotation.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (TRAVODDS::AnnotationUsageTypeSupport::get_instance()->deserialize_data(&structData->annotation[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->annotation[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::Type\n");
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

TypeObject* TRAVODDS::TypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::TypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    return 0;
}

int TRAVODDS::TypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::TypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::TypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::TypeTypeSupport::get_typename() {
    return "TRAVODDS::Type";
}

TRAVODDS::MemberPropertyTypeSupport* TRAVODDS::MemberPropertyTypeSupport::get_instance() {
    static TRAVODDS::MemberPropertyTypeSupport instance;
    return &instance;
}

void* TRAVODDS::MemberPropertyTypeSupport::create_data() {
    return new TRAVODDS::MemberProperty();
}

void TRAVODDS::MemberPropertyTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::MemberProperty*>(data);
}

int TRAVODDS::MemberPropertyTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::MemberProperty* dstData = static_cast<TRAVODDS::MemberProperty*>(dst);
    TRAVODDS::MemberProperty* srcData = static_cast<TRAVODDS::MemberProperty*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::MemberPropertyTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::MemberProperty* structData = static_cast<TRAVODDS::MemberProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += CdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->type_id, currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(static_cast<uint32_t>(structData->name.size()) + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::MemberPropertyTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::MemberProperty* structData = static_cast<TRAVODDS::MemberProperty*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += CdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->type_id, currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int TRAVODDS::MemberPropertyTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::MemberProperty* structData = static_cast<TRAVODDS::MemberProperty*>(data);
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
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int TRAVODDS::MemberPropertyTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::MemberProperty* structData = static_cast<TRAVODDS::MemberProperty*>(data);
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    switch(16 / 8) {
    case 1:
        if (!cdr->deserializeBaseType(tmpCharEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<TRAVODDS::MemberFlag>(tmpCharEnum);
        break;
    case 2:
        if (!cdr->deserializeBaseType(tmpShortEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<TRAVODDS::MemberFlag>(tmpShortEnum);
        break;
    case 4:
        if (!cdr->deserializeBaseType(tmpIntEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->flag\n");
            return -1;
        }
        structData->flag = static_cast<TRAVODDS::MemberFlag>(tmpIntEnum);
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
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->type_id, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->type_id\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* TRAVODDS::MemberPropertyTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::MemberPropertyTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    return 0;
}

int TRAVODDS::MemberPropertyTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::MemberPropertyTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::MemberPropertyTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::MemberPropertyTypeSupport::get_typename() {
    return "TRAVODDS::MemberProperty";
}

TRAVODDS::MemberTypeSupport* TRAVODDS::MemberTypeSupport::get_instance() {
    static TRAVODDS::MemberTypeSupport instance;
    return &instance;
}

void* TRAVODDS::MemberTypeSupport::create_data() {
    return new TRAVODDS::Member();
}

void TRAVODDS::MemberTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::Member*>(data);
}

int TRAVODDS::MemberTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::Member* dstData = static_cast<TRAVODDS::Member*>(dst);
    TRAVODDS::Member* srcData = static_cast<TRAVODDS::Member*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::MemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::Member* structData = static_cast<TRAVODDS::Member*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::MemberPropertyTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->property, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->annotation.size(); ++i) {
        currentAlignment += TRAVODDS::AnnotationUsageTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->annotation[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::MemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::MemberTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::Member* structData = static_cast<TRAVODDS::Member*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::PROPERTY_MEMBER_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (TRAVODDS::MemberPropertyTypeSupport::get_instance()->serialize_data((void*)&structData->property, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->property\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::ANNOTATION_MEMBER_MEMBER_ID);
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
        if (TRAVODDS::AnnotationUsageTypeSupport::get_instance()->serialize_data((void*)&structData->annotation[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->annotation[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::MemberTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::Member* structData = static_cast<TRAVODDS::Member*>(data);
    unsigned int tmpLength = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::Member\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::PROPERTY_MEMBER_MEMBER_ID:
            if (TRAVODDS::MemberPropertyTypeSupport::get_instance()->deserialize_data(&structData->property, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->property\n");
                return -1;
            }
            break;
        case TRAVODDS::ANNOTATION_MEMBER_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->annotation\n");
                return -1;
            }
            structData->annotation.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (TRAVODDS::AnnotationUsageTypeSupport::get_instance()->deserialize_data(&structData->annotation[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->annotation[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::Member\n");
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

TypeObject* TRAVODDS::MemberTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::MemberTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    return 0;
}

int TRAVODDS::MemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::MemberTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::MemberTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::MemberTypeSupport::get_typename() {
    return "TRAVODDS::Member";
}

TRAVODDS::StructureTypeTypeSupport* TRAVODDS::StructureTypeTypeSupport::get_instance() {
    static TRAVODDS::StructureTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::StructureTypeTypeSupport::create_data() {
    return new TRAVODDS::StructureType();
}

void TRAVODDS::StructureTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::StructureType*>(data);
}

int TRAVODDS::StructureTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::StructureType* dstData = static_cast<TRAVODDS::StructureType*>(dst);
    TRAVODDS::StructureType* srcData = static_cast<TRAVODDS::StructureType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::StructureTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::StructureType* structData = static_cast<TRAVODDS::StructureType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TRAVODDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += TRAVODDS::MemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::StructureTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::StructureTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::StructureType* structData = static_cast<TRAVODDS::StructureType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->annotation[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::BASE_TYPE_STRUCTURETYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->base_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::MEMBER_STRUCTURETYPE_MEMBER_ID);
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
        if (TRAVODDS::MemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::StructureTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::StructureType* structData = static_cast<TRAVODDS::StructureType*>(data);
    unsigned int tmpLength = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->annotation[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::StructureType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::BASE_TYPE_STRUCTURETYPE_MEMBER_ID:
            if (TRAVODDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->base_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
                return -1;
            }
            break;
        case TRAVODDS::MEMBER_STRUCTURETYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->member\n");
                return -1;
            }
            structData->member.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (TRAVODDS::MemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::StructureType\n");
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

TypeObject* TRAVODDS::StructureTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::StructureTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::StructureType* structData = static_cast<TRAVODDS::StructureType*>(data);
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::StructureTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::StructureTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::StructureTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::StructureTypeTypeSupport::get_typename() {
    return "TRAVODDS::StructureType";
}

TRAVODDS::UnionMemberTypeSupport* TRAVODDS::UnionMemberTypeSupport::get_instance() {
    static TRAVODDS::UnionMemberTypeSupport instance;
    return &instance;
}

void* TRAVODDS::UnionMemberTypeSupport::create_data() {
    return new TRAVODDS::UnionMember();
}

void TRAVODDS::UnionMemberTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::UnionMember*>(data);
}

int TRAVODDS::UnionMemberTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::UnionMember* dstData = static_cast<TRAVODDS::UnionMember*>(dst);
    TRAVODDS::UnionMember* srcData = static_cast<TRAVODDS::UnionMember*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::UnionMemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::UnionMember* structData = static_cast<TRAVODDS::UnionMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TRAVODDS::MemberTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::Member*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int), static_cast<uint32_t>(structData->label.size()), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::UnionMemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::UnionMemberTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::UnionMember* structData = static_cast<TRAVODDS::UnionMember*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TRAVODDS::MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::LABEL_UNIONMEMBER_MEMBER_ID);
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
    if (!cdr->serializeBaseTypeArray(structData->label.data(), static_cast<uint32_t>(structData->label.size()))) {
        fprintf(stderr, "Serialization failed for field: structData->label\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::UnionMemberTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::UnionMember* structData = static_cast<TRAVODDS::UnionMember*>(data);
    unsigned int tmpLength = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TRAVODDS::MemberTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::UnionMember\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::LABEL_UNIONMEMBER_MEMBER_ID:
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
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::UnionMember\n");
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

TypeObject* TRAVODDS::UnionMemberTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::UnionMemberTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::UnionMember* structData = static_cast<TRAVODDS::UnionMember*>(data);
    if (TRAVODDS::MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::UnionMemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::UnionMemberTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::UnionMemberTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::UnionMemberTypeSupport::get_typename() {
    return "TRAVODDS::UnionMember";
}

TRAVODDS::UnionTypeTypeSupport* TRAVODDS::UnionTypeTypeSupport::get_instance() {
    static TRAVODDS::UnionTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::UnionTypeTypeSupport::create_data() {
    return new TRAVODDS::UnionType();
}

void TRAVODDS::UnionTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::UnionType*>(data);
}

int TRAVODDS::UnionTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::UnionType* dstData = static_cast<TRAVODDS::UnionType*>(dst);
    TRAVODDS::UnionType* srcData = static_cast<TRAVODDS::UnionType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::UnionTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::UnionType* structData = static_cast<TRAVODDS::UnionType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TRAVODDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += TRAVODDS::UnionMemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::UnionTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::UnionTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::UnionType* structData = static_cast<TRAVODDS::UnionType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->label\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::MEMBER_UNIONTYPE_MEMBER_ID);
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
        if (TRAVODDS::UnionMemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::UnionTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::UnionType* structData = static_cast<TRAVODDS::UnionType*>(data);
    uint32_t tmpLength = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->label\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::UnionType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::MEMBER_UNIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->member\n");
                return -1;
            }
            structData->member.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (TRAVODDS::UnionMemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::UnionType\n");
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

TypeObject* TRAVODDS::UnionTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::UnionTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::UnionType* structData = static_cast<TRAVODDS::UnionType*>(data);
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::UnionTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::UnionTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::UnionTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::UnionTypeTypeSupport::get_typename() {
    return "TRAVODDS::UnionType";
}

TRAVODDS::AnnotationMemberTypeSupport* TRAVODDS::AnnotationMemberTypeSupport::get_instance() {
    static TRAVODDS::AnnotationMemberTypeSupport instance;
    return &instance;
}

void* TRAVODDS::AnnotationMemberTypeSupport::create_data() {
    return new TRAVODDS::AnnotationMember();
}

void TRAVODDS::AnnotationMemberTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::AnnotationMember*>(data);
}

int TRAVODDS::AnnotationMemberTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::AnnotationMember* dstData = static_cast<TRAVODDS::AnnotationMember*>(dst);
    TRAVODDS::AnnotationMember* srcData = static_cast<TRAVODDS::AnnotationMember*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::AnnotationMemberTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::AnnotationMember* structData = static_cast<TRAVODDS::AnnotationMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TRAVODDS::MemberTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::Member*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::AnnotationMemberValueTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->default_value, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::AnnotationMemberTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::AnnotationMember* structData = static_cast<TRAVODDS::AnnotationMember*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::AnnotationMemberValueTypeSupport::get_instance()->get_max_serialized_data_size(&structData->default_value, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

int TRAVODDS::AnnotationMemberTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::AnnotationMember* structData = static_cast<TRAVODDS::AnnotationMember*>(data);
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TRAVODDS::MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::DEFAULT_VALUE_ANNOTATIONMEMBER_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->default_value\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (TRAVODDS::AnnotationMemberValueTypeSupport::get_instance()->serialize_data((void*)&structData->default_value, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->default_value\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::AnnotationMemberTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::AnnotationMember* structData = static_cast<TRAVODDS::AnnotationMember*>(data);
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TRAVODDS::MemberTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::AnnotationMember\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::DEFAULT_VALUE_ANNOTATIONMEMBER_MEMBER_ID:
            if (TRAVODDS::AnnotationMemberValueTypeSupport::get_instance()->deserialize_data(&structData->default_value, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->default_value\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::AnnotationMember\n");
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

TypeObject* TRAVODDS::AnnotationMemberTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::AnnotationMemberTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::AnnotationMember* structData = static_cast<TRAVODDS::AnnotationMember*>(data);
    if (TRAVODDS::MemberTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Member*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::AnnotationMemberTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::AnnotationMemberTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::AnnotationMemberTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::AnnotationMemberTypeSupport::get_typename() {
    return "TRAVODDS::AnnotationMember";
}

TRAVODDS::AnnotationTypeTypeSupport* TRAVODDS::AnnotationTypeTypeSupport::get_instance() {
    static TRAVODDS::AnnotationTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::AnnotationTypeTypeSupport::create_data() {
    return new TRAVODDS::AnnotationType();
}

void TRAVODDS::AnnotationTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::AnnotationType*>(data);
}

int TRAVODDS::AnnotationTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::AnnotationType* dstData = static_cast<TRAVODDS::AnnotationType*>(dst);
    TRAVODDS::AnnotationType* srcData = static_cast<TRAVODDS::AnnotationType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::AnnotationTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::AnnotationType* structData = static_cast<TRAVODDS::AnnotationType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TRAVODDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->member.size(); ++i) {
        currentAlignment += TRAVODDS::AnnotationMemberTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->member[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::AnnotationTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::AnnotationTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::AnnotationType* structData = static_cast<TRAVODDS::AnnotationType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->default_value\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::BASE_TYPE_ANNOTATIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->base_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::MEMBER_ANNOTATIONTYPE_MEMBER_ID);
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
        if (TRAVODDS::AnnotationMemberTypeSupport::get_instance()->serialize_data((void*)&structData->member[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::AnnotationTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::AnnotationType* structData = static_cast<TRAVODDS::AnnotationType*>(data);
    uint32_t tmpLength = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->default_value\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::AnnotationType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::BASE_TYPE_ANNOTATIONTYPE_MEMBER_ID:
            if (TRAVODDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->base_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
                return -1;
            }
            break;
        case TRAVODDS::MEMBER_ANNOTATIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->member\n");
                return -1;
            }
            structData->member.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (TRAVODDS::AnnotationMemberTypeSupport::get_instance()->deserialize_data(&structData->member[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::AnnotationType\n");
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

TypeObject* TRAVODDS::AnnotationTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::AnnotationTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::AnnotationType* structData = static_cast<TRAVODDS::AnnotationType*>(data);
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::AnnotationTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::AnnotationTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::AnnotationTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::AnnotationTypeTypeSupport::get_typename() {
    return "TRAVODDS::AnnotationType";
}

TRAVODDS::AliasTypeTypeSupport* TRAVODDS::AliasTypeTypeSupport::get_instance() {
    static TRAVODDS::AliasTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::AliasTypeTypeSupport::create_data() {
    return new TRAVODDS::AliasType();
}

void TRAVODDS::AliasTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::AliasType*>(data);
}

int TRAVODDS::AliasTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::AliasType* dstData = static_cast<TRAVODDS::AliasType*>(dst);
    TRAVODDS::AliasType* srcData = static_cast<TRAVODDS::AliasType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::AliasTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::AliasType* structData = static_cast<TRAVODDS::AliasType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TRAVODDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::AliasTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::AliasType* structData = static_cast<TRAVODDS::AliasType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->base_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

int TRAVODDS::AliasTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::AliasType* structData = static_cast<TRAVODDS::AliasType*>(data);
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->member[i]\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::BASE_TYPE_ALIASTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->base_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::AliasTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::AliasType* structData = static_cast<TRAVODDS::AliasType*>(data);
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->member[i]\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::AliasType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::BASE_TYPE_ALIASTYPE_MEMBER_ID:
            if (TRAVODDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->base_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::AliasType\n");
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

TypeObject* TRAVODDS::AliasTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::AliasTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::AliasType* structData = static_cast<TRAVODDS::AliasType*>(data);
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::AliasTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::AliasTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::AliasTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::AliasTypeTypeSupport::get_typename() {
    return "TRAVODDS::AliasType";
}

TRAVODDS::CollectionTypeTypeSupport* TRAVODDS::CollectionTypeTypeSupport::get_instance() {
    static TRAVODDS::CollectionTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::CollectionTypeTypeSupport::create_data() {
    return new TRAVODDS::CollectionType();
}

void TRAVODDS::CollectionTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::CollectionType*>(data);
}

int TRAVODDS::CollectionTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::CollectionType* dstData = static_cast<TRAVODDS::CollectionType*>(dst);
    TRAVODDS::CollectionType* srcData = static_cast<TRAVODDS::CollectionType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::CollectionTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::CollectionType* structData = static_cast<TRAVODDS::CollectionType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TRAVODDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->element_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::CollectionTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::CollectionType* structData = static_cast<TRAVODDS::CollectionType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->element_type, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

int TRAVODDS::CollectionTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::CollectionType* structData = static_cast<TRAVODDS::CollectionType*>(data);
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->base_type\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::ELEMENT_TYPE_COLLECTIONTYPE_MEMBER_ID);
    if (!lenAddr) {
        fprintf(stderr, "Serialization failed for field: structData->element_type\n");
        return -1;
    }
    beforeLen = cdr->getRemainingSpace();
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->element_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->element_type\n");
        return -1;
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::ELEMENT_SHARED_COLLECTIONTYPE_MEMBER_ID);
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

int TRAVODDS::CollectionTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::CollectionType* structData = static_cast<TRAVODDS::CollectionType*>(data);
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->base_type\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::CollectionType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::ELEMENT_TYPE_COLLECTIONTYPE_MEMBER_ID:
            if (TRAVODDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->element_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->element_type\n");
                return -1;
            }
            break;
        case TRAVODDS::ELEMENT_SHARED_COLLECTIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(structData->element_shared)) {
                fprintf(stderr, "Deserialization failed for field: structData->element_shared\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::CollectionType\n");
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

TypeObject* TRAVODDS::CollectionTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::CollectionTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::CollectionType* structData = static_cast<TRAVODDS::CollectionType*>(data);
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::CollectionTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::CollectionTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::CollectionTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::CollectionTypeTypeSupport::get_typename() {
    return "TRAVODDS::CollectionType";
}

TRAVODDS::ArrayTypeTypeSupport* TRAVODDS::ArrayTypeTypeSupport::get_instance() {
    static TRAVODDS::ArrayTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::ArrayTypeTypeSupport::create_data() {
    return new TRAVODDS::ArrayType();
}

void TRAVODDS::ArrayTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::ArrayType*>(data);
}

int TRAVODDS::ArrayTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::ArrayType* dstData = static_cast<TRAVODDS::ArrayType*>(dst);
    TRAVODDS::ArrayType* srcData = static_cast<TRAVODDS::ArrayType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::ArrayTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::ArrayType* structData = static_cast<TRAVODDS::ArrayType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TRAVODDS::CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->bound.size(); ++i) {
        currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::ArrayTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::ArrayTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::ArrayType* structData = static_cast<TRAVODDS::ArrayType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->element_shared\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::BOUND_ARRAYTYPE_MEMBER_ID);
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

int TRAVODDS::ArrayTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::ArrayType* structData = static_cast<TRAVODDS::ArrayType*>(data);
    unsigned int tmpLength = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->element_shared\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::ArrayType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::BOUND_ARRAYTYPE_MEMBER_ID:
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
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::ArrayType\n");
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

TypeObject* TRAVODDS::ArrayTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::ArrayTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::ArrayType* structData = static_cast<TRAVODDS::ArrayType*>(data);
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::ArrayTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::ArrayTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::ArrayTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::ArrayTypeTypeSupport::get_typename() {
    return "TRAVODDS::ArrayType";
}

TRAVODDS::MapTypeTypeSupport* TRAVODDS::MapTypeTypeSupport::get_instance() {
    static TRAVODDS::MapTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::MapTypeTypeSupport::create_data() {
    return new TRAVODDS::MapType();
}

void TRAVODDS::MapTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::MapType*>(data);
}

int TRAVODDS::MapTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::MapType* dstData = static_cast<TRAVODDS::MapType*>(dst);
    TRAVODDS::MapType* srcData = static_cast<TRAVODDS::MapType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::MapTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::MapType* structData = static_cast<TRAVODDS::MapType*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += TRAVODDS::CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), currentAlignment);
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->key_element_type, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::MapTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::MapType* structData = static_cast<TRAVODDS::MapType*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_max_serialized_data_size(&structData->key_element_type, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int TRAVODDS::MapTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::MapType* structData = static_cast<TRAVODDS::MapType*>(data);
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->bound[i]\n");
        return -1;
    }
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->key_element_type, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->key_element_type\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

int TRAVODDS::MapTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::MapType* structData = static_cast<TRAVODDS::MapType*>(data);
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->bound[i]\n");
        return -1;
    }
    if (TRAVODDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->key_element_type, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->key_element_type\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->bound)) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

TypeObject* TRAVODDS::MapTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::MapTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::MapType* structData = static_cast<TRAVODDS::MapType*>(data);
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::MapTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::MapTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::MapTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::MapTypeTypeSupport::get_typename() {
    return "TRAVODDS::MapType";
}

TRAVODDS::SequenceTypeTypeSupport* TRAVODDS::SequenceTypeTypeSupport::get_instance() {
    static TRAVODDS::SequenceTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::SequenceTypeTypeSupport::create_data() {
    return new TRAVODDS::SequenceType();
}

void TRAVODDS::SequenceTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::SequenceType*>(data);
}

int TRAVODDS::SequenceTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::SequenceType* dstData = static_cast<TRAVODDS::SequenceType*>(dst);
    TRAVODDS::SequenceType* srcData = static_cast<TRAVODDS::SequenceType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::SequenceTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::SequenceType* structData = static_cast<TRAVODDS::SequenceType*>(data);
    unsigned int initialAlignment = currentAlignment;
    //unsigned int tmpAlignment = 0;
    currentAlignment += TRAVODDS::CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::SequenceTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    //TRAVODDS::SequenceType* structData = static_cast<TRAVODDS::SequenceType*>(data);
    unsigned int initialAlignment = currentAlignment;
    //unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int TRAVODDS::SequenceTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::SequenceType* structData = static_cast<TRAVODDS::SequenceType*>(data);
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

int TRAVODDS::SequenceTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::SequenceType* structData = static_cast<TRAVODDS::SequenceType*>(data);
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->bound)) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

TypeObject* TRAVODDS::SequenceTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::SequenceTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::SequenceType* structData = static_cast<TRAVODDS::SequenceType*>(data);
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::SequenceTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::SequenceTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::SequenceTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::SequenceTypeTypeSupport::get_typename() {
    return "TRAVODDS::SequenceType";
}

TRAVODDS::StringTypeTypeSupport* TRAVODDS::StringTypeTypeSupport::get_instance() {
    static TRAVODDS::StringTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::StringTypeTypeSupport::create_data() {
    return new TRAVODDS::StringType();
}

void TRAVODDS::StringTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::StringType*>(data);
}

int TRAVODDS::StringTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::StringType* dstData = static_cast<TRAVODDS::StringType*>(dst);
    TRAVODDS::StringType* srcData = static_cast<TRAVODDS::StringType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::StringTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::StringType* structData = static_cast<TRAVODDS::StringType*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += TRAVODDS::CollectionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::StringTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    //TRAVODDS::StringType* structData = static_cast<TRAVODDS::StringType*>(data);
    unsigned int initialAlignment = currentAlignment;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int TRAVODDS::StringTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::StringType* structData = static_cast<TRAVODDS::StringType*>(data);
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->bound)) {
        fprintf(stderr, "Serialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

int TRAVODDS::StringTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::StringType* structData = static_cast<TRAVODDS::StringType*>(data);
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->bound)) {
        fprintf(stderr, "Deserialization failed for field: structData->bound\n");
        return -1;
    }
    return 0;
}

TypeObject* TRAVODDS::StringTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::StringTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::StringType* structData = static_cast<TRAVODDS::StringType*>(data);
    //bool memberHasKey = false;
    if (TRAVODDS::CollectionTypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::CollectionType*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::StringTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::StringTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::StringTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::StringTypeTypeSupport::get_typename() {
    return "TRAVODDS::StringType";
}

TRAVODDS::BitTypeSupport* TRAVODDS::BitTypeSupport::get_instance() {
    static TRAVODDS::BitTypeSupport instance;
    return &instance;
}

void* TRAVODDS::BitTypeSupport::create_data() {
    return new TRAVODDS::Bit();
}

void TRAVODDS::BitTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::Bit*>(data);
}

int TRAVODDS::BitTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::Bit* dstData = static_cast<TRAVODDS::Bit*>(dst);
    TRAVODDS::Bit* srcData = static_cast<TRAVODDS::Bit*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::BitTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::Bit* structData = static_cast<TRAVODDS::Bit*>(data);
    unsigned int initialAlignment = currentAlignment;
    //unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(static_cast<uint32_t>(structData->name.size()) + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::BitTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    //TRAVODDS::Bit* structData = static_cast<TRAVODDS::Bit*>(data);
    unsigned int initialAlignment = currentAlignment;
    //unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int TRAVODDS::BitTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    (void)endian;
    TRAVODDS::Bit* structData = static_cast<TRAVODDS::Bit*>(data);
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

int TRAVODDS::BitTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    (void)endian;
    TRAVODDS::Bit* structData = static_cast<TRAVODDS::Bit*>(data);
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

TypeObject* TRAVODDS::BitTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::BitTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    //TRAVODDS::Bit* structData = static_cast<TRAVODDS::Bit*>(data);
    //bool memberHasKey = false;
    return 0;
}

int TRAVODDS::BitTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::BitTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::BitTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::BitTypeSupport::get_typename() {
    return "TRAVODDS::Bit";
}

TRAVODDS::BitSetTypeTypeSupport* TRAVODDS::BitSetTypeTypeSupport::get_instance() {
    static TRAVODDS::BitSetTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::BitSetTypeTypeSupport::create_data() {
    return new TRAVODDS::BitSetType();
}

void TRAVODDS::BitSetTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::BitSetType*>(data);
}

int TRAVODDS::BitSetTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::BitSetType* dstData = static_cast<TRAVODDS::BitSetType*>(dst);
    TRAVODDS::BitSetType* srcData = static_cast<TRAVODDS::BitSetType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::BitSetTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::BitSetType* structData = static_cast<TRAVODDS::BitSetType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TRAVODDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->bit.size(); ++i) {
        currentAlignment += TRAVODDS::BitTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->bit[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::BitSetTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::BitSetTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::BitSetType* structData = static_cast<TRAVODDS::BitSetType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::BIT_BOUND_BITSETTYPE_MEMBER_ID);
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
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::BIT_BITSETTYPE_MEMBER_ID);
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
        if (TRAVODDS::BitTypeSupport::get_instance()->serialize_data((void*)&structData->bit[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->bit[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::BitSetTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::BitSetType* structData = static_cast<TRAVODDS::BitSetType*>(data);
    unsigned int tmpLength = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::BitSetType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::BIT_BOUND_BITSETTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(structData->bit_bound)) {
                fprintf(stderr, "Deserialization failed for field: structData->bit_bound\n");
                return -1;
            }
            break;
        case TRAVODDS::BIT_BITSETTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->bit\n");
                return -1;
            }
            structData->bit.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (TRAVODDS::BitTypeSupport::get_instance()->deserialize_data(&structData->bit[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->bit[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::BitSetType\n");
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

TypeObject* TRAVODDS::BitSetTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::BitSetTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::BitSetType* structData = static_cast<TRAVODDS::BitSetType*>(data);
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::BitSetTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::BitSetTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::BitSetTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::BitSetTypeTypeSupport::get_typename() {
    return "TRAVODDS::BitSetType";
}

TRAVODDS::EnumeratedConstantTypeSupport* TRAVODDS::EnumeratedConstantTypeSupport::get_instance() {
    static TRAVODDS::EnumeratedConstantTypeSupport instance;
    return &instance;
}

void* TRAVODDS::EnumeratedConstantTypeSupport::create_data() {
    return new TRAVODDS::EnumeratedConstant();
}

void TRAVODDS::EnumeratedConstantTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::EnumeratedConstant*>(data);
}

int TRAVODDS::EnumeratedConstantTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::EnumeratedConstant* dstData = static_cast<TRAVODDS::EnumeratedConstant*>(dst);
    TRAVODDS::EnumeratedConstant* srcData = static_cast<TRAVODDS::EnumeratedConstant*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::EnumeratedConstantTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::EnumeratedConstant* structData = static_cast<TRAVODDS::EnumeratedConstant*>(data);
    unsigned int initialAlignment = currentAlignment;
    //unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(static_cast<uint32_t>(structData->name.size()) + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::EnumeratedConstantTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    unsigned int initialAlignment = currentAlignment;
    //unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int TRAVODDS::EnumeratedConstantTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    (void)endian;
    TRAVODDS::EnumeratedConstant* structData = static_cast<TRAVODDS::EnumeratedConstant*>(data);
    //uint32_t tmpLength = 0;
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

int TRAVODDS::EnumeratedConstantTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    (void)endian;
    TRAVODDS::EnumeratedConstant* structData = static_cast<TRAVODDS::EnumeratedConstant*>(data);
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

TypeObject* TRAVODDS::EnumeratedConstantTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::EnumeratedConstantTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    //TRAVODDS::EnumeratedConstant* structData = static_cast<TRAVODDS::EnumeratedConstant*>(data);
    //bool memberHasKey = false;
    return 0;
}

int TRAVODDS::EnumeratedConstantTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::EnumeratedConstantTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::EnumeratedConstantTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::EnumeratedConstantTypeSupport::get_typename() {
    return "TRAVODDS::EnumeratedConstant";
}

TRAVODDS::EnumerationTypeTypeSupport* TRAVODDS::EnumerationTypeTypeSupport::get_instance() {
    static TRAVODDS::EnumerationTypeTypeSupport instance;
    return &instance;
}

void* TRAVODDS::EnumerationTypeTypeSupport::create_data() {
    return new TRAVODDS::EnumerationType();
}

void TRAVODDS::EnumerationTypeTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::EnumerationType*>(data);
}

int TRAVODDS::EnumerationTypeTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::EnumerationType* dstData = static_cast<TRAVODDS::EnumerationType*>(dst);
    TRAVODDS::EnumerationType* srcData = static_cast<TRAVODDS::EnumerationType*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::EnumerationTypeTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::EnumerationType* structData = static_cast<TRAVODDS::EnumerationType*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += TRAVODDS::TypeTypeSupport::get_instance()->get_serialized_data_size((void*)dynamic_cast<TRAVODDS::Type*>(structData), currentAlignment);
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->constant.size(); ++i) {
        currentAlignment += TRAVODDS::EnumeratedConstantTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->constant[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::EnumerationTypeTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::EnumerationTypeTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::EnumerationType* structData = static_cast<TRAVODDS::EnumerationType*>(data);
    uint32_t tmpLength = 0;
    bool useExtend = true;
    char* lenAddr = nullptr;
    unsigned int beforeLen = 0;
    unsigned int afterLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    useExtend = true;
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::BIT_BOUND_ENUMERATIONTYPE_MEMBER_ID);
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
    lenAddr = cdr->serializeParameterHeader(useExtend, true, TRAVODDS::CONSTANT_ENUMERATIONTYPE_MEMBER_ID);
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
        if (TRAVODDS::EnumeratedConstantTypeSupport::get_instance()->serialize_data((void*)&structData->constant[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->constant[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::EnumerationTypeTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::EnumerationType* structData = static_cast<TRAVODDS::EnumerationType*>(data);
    unsigned int tmpLength = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    if (TRAVODDS::TypeTypeSupport::get_instance()->deserialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::EnumerationType\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case TRAVODDS::BIT_BOUND_ENUMERATIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(structData->bit_bound)) {
                fprintf(stderr, "Deserialization failed for field: structData->bit_bound\n");
                return -1;
            }
            break;
        case TRAVODDS::CONSTANT_ENUMERATIONTYPE_MEMBER_ID:
            if (!cdr->deserializeBaseType(tmpLength)) {
                fprintf(stderr, "Deserialization length failed for field: structData->constant\n");
                return -1;
            }
            structData->constant.resize(tmpLength);
            for (size_t i = 0; i < tmpLength; ++i) {
                if (TRAVODDS::EnumeratedConstantTypeSupport::get_instance()->deserialize_data(&structData->constant[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->constant[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::EnumerationType\n");
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

TypeObject* TRAVODDS::EnumerationTypeTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::EnumerationTypeTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::EnumerationType* structData = static_cast<TRAVODDS::EnumerationType*>(data);
    if (TRAVODDS::TypeTypeSupport::get_instance()->serialize_data((void*)dynamic_cast<TRAVODDS::Type*>(structData), cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: \n");
        return -1;
    }
    return 0;
}

int TRAVODDS::EnumerationTypeTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::EnumerationTypeTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::EnumerationTypeTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::EnumerationTypeTypeSupport::get_typename() {
    return "TRAVODDS::EnumerationType";
}

TRAVODDS::_ModuleTypeSupport* TRAVODDS::_ModuleTypeSupport::get_instance() {
    static TRAVODDS::_ModuleTypeSupport instance;
    return &instance;
}

void* TRAVODDS::_ModuleTypeSupport::create_data() {
    return new TRAVODDS::_Module();
}

void TRAVODDS::_ModuleTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::_Module*>(data);
}

int TRAVODDS::_ModuleTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::_Module* dstData = static_cast<TRAVODDS::_Module*>(dst);
    TRAVODDS::_Module* srcData = static_cast<TRAVODDS::_Module*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::_ModuleTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::_Module* structData = static_cast<TRAVODDS::_Module*>(data);
    unsigned int initialAlignment = currentAlignment;
    //unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getStringSize(static_cast<uint32_t>(structData->name.size()) + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::_ModuleTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    //TRAVODDS::_Module* structData = static_cast<TRAVODDS::_Module*>(data);
    unsigned int initialAlignment = currentAlignment;
    //unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getStringSize(256, currentAlignment);
    return currentAlignment - initialAlignment;
}

int TRAVODDS::_ModuleTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    (void)endian;
    TRAVODDS::_Module* structData = static_cast<TRAVODDS::_Module*>(data);
    //uint32_t tmpLength = 0;
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int TRAVODDS::_ModuleTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    (void)endian;
    TRAVODDS::_Module* structData = static_cast<TRAVODDS::_Module*>(data);
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* TRAVODDS::_ModuleTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::_ModuleTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    //TRAVODDS::_Module* structData = static_cast<TRAVODDS::_Module*>(data);
    //bool memberHasKey = false;
    return 0;
}

int TRAVODDS::_ModuleTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::_ModuleTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::_ModuleTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::_ModuleTypeSupport::get_typename() {
    return "TRAVODDS::_Module";
}

TRAVODDS::TypeLibraryElementTypeSupport* TRAVODDS::TypeLibraryElementTypeSupport::get_instance() {
    static TRAVODDS::TypeLibraryElementTypeSupport instance;
    return &instance;
}

void* TRAVODDS::TypeLibraryElementTypeSupport::create_data() {
    return new TRAVODDS::TypeLibraryElement();
}

void TRAVODDS::TypeLibraryElementTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::TypeLibraryElement*>(data);
}

int TRAVODDS::TypeLibraryElementTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::TypeLibraryElement* dstData = static_cast<TRAVODDS::TypeLibraryElement*>(dst);
    TRAVODDS::TypeLibraryElement* srcData = static_cast<TRAVODDS::TypeLibraryElement*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::TypeLibraryElementTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::TypeLibraryElement* structData = static_cast<TRAVODDS::TypeLibraryElement*>(data);
    //unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    switch(structData->u_) {
    case TRAVODDS::ALIAS_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::AliasTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->alias_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::ANNOTATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::AnnotationTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->annotation_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::ARRAY_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::ArrayTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->array_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::BITSET_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::BitSetTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->bitset_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::ENUMERATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::EnumerationTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->enumeration_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::MAP_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::MapTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->map_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::SEQUENCE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::SequenceTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->sequence_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::STRING_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::StringTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->string_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::STRUCTURE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::StructureTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->structure_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::UNION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::UnionTypeTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->union_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::MODULE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::_ModuleTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->mod, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    }
    return 0;
}

unsigned int TRAVODDS::TypeLibraryElementTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::TypeLibraryElement* structData = static_cast<TRAVODDS::TypeLibraryElement*>(data);
    //unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(16 / 8, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    switch(structData->u_) {
    case TRAVODDS::ALIAS_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::AliasTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->alias_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::ANNOTATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::AnnotationTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->annotation_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::ARRAY_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::ArrayTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->array_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::BITSET_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::BitSetTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->bitset_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::ENUMERATION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::EnumerationTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->enumeration_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::MAP_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::MapTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->map_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::SEQUENCE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::SequenceTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->sequence_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::STRING_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::StringTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->string_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::STRUCTURE_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::StructureTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->structure_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::UNION_TYPE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::UnionTypeTypeSupport::get_instance()->get_max_serialized_data_size(&structData->union_type, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    case TRAVODDS::MODULE_ELEMENT:
        currentAlignment += 4;
        tmpAlignment = currentAlignment;
        currentAlignment = 0;
        currentAlignment += TRAVODDS::_ModuleTypeSupport::get_instance()->get_max_serialized_data_size(&structData->mod, currentAlignment);
        currentAlignment += tmpAlignment;
        currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
        break;
    }
    return 0;
}

int TRAVODDS::TypeLibraryElementTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::TypeLibraryElement* structData = static_cast<TRAVODDS::TypeLibraryElement*>(data);
    //uint32_t tmpLength = 0;
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
    case TRAVODDS::ALIAS_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 1);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->alias_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::AliasTypeTypeSupport::get_instance()->serialize_data((void*)&structData->alias_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->alias_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case TRAVODDS::ANNOTATION_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 2);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->annotation_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::AnnotationTypeTypeSupport::get_instance()->serialize_data((void*)&structData->annotation_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->annotation_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case TRAVODDS::ARRAY_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 3);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->array_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::ArrayTypeTypeSupport::get_instance()->serialize_data((void*)&structData->array_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->array_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case TRAVODDS::BITSET_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 4);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->bitset_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::BitSetTypeTypeSupport::get_instance()->serialize_data((void*)&structData->bitset_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->bitset_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case TRAVODDS::ENUMERATION_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 5);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->enumeration_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::EnumerationTypeTypeSupport::get_instance()->serialize_data((void*)&structData->enumeration_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->enumeration_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case TRAVODDS::MAP_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 6);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->map_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::MapTypeTypeSupport::get_instance()->serialize_data((void*)&structData->map_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->map_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case TRAVODDS::SEQUENCE_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 7);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->sequence_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::SequenceTypeTypeSupport::get_instance()->serialize_data((void*)&structData->sequence_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->sequence_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case TRAVODDS::STRING_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 8);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->string_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::StringTypeTypeSupport::get_instance()->serialize_data((void*)&structData->string_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->string_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case TRAVODDS::STRUCTURE_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 9);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->structure_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::StructureTypeTypeSupport::get_instance()->serialize_data((void*)&structData->structure_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->structure_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case TRAVODDS::UNION_TYPE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 10);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->union_type\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::UnionTypeTypeSupport::get_instance()->serialize_data((void*)&structData->union_type, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->union_type\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    case TRAVODDS::MODULE_ELEMENT:
        useExtend = true;
        lenAddr = cdr->serializeParameterHeader(useExtend, true, 11);
        if (!lenAddr) {
            fprintf(stderr, "Serialization failed for field: structData->mod\n");
            return -1;
        }
        beforeLen = cdr->getRemainingSpace();
        if (TRAVODDS::_ModuleTypeSupport::get_instance()->serialize_data((void*)&structData->mod, cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->mod\n");
            return -1;
        }
        afterLen = cdr->getRemainingSpace();
        cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
        break;
    }
    return 0;
}

int TRAVODDS::TypeLibraryElementTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::TypeLibraryElement* structData = static_cast<TRAVODDS::TypeLibraryElement*>(data);
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
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::TypeLibraryElement\n");
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
                structData->u_ = static_cast<TRAVODDS::TypeLibraryElementKind>(tmpCharEnum);
                break;
            case 2:
                if (!cdr->deserializeBaseType(tmpShortEnum)) {
                    fprintf(stderr, "Deserialization failed for field: structData->u_\n");
                    return -1;
                }
                structData->u_ = static_cast<TRAVODDS::TypeLibraryElementKind>(tmpShortEnum);
                break;
            case 4:
                if (!cdr->deserializeBaseType(tmpIntEnum)) {
                    fprintf(stderr, "Deserialization failed for field: structData->u_\n");
                    return -1;
                }
                structData->u_ = static_cast<TRAVODDS::TypeLibraryElementKind>(tmpIntEnum);
                break;
            default:
                fprintf(stderr, "Deserialization failed for field: structData->u_\n");
                return -1;
                break;
            }
            break;
        case 1:
            if (TRAVODDS::AliasTypeTypeSupport::get_instance()->deserialize_data(&structData->alias_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->alias_type\n");
                return -1;
            }
            break;
        case 2:
            if (TRAVODDS::AnnotationTypeTypeSupport::get_instance()->deserialize_data(&structData->annotation_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->annotation_type\n");
                return -1;
            }
            break;
        case 3:
            if (TRAVODDS::ArrayTypeTypeSupport::get_instance()->deserialize_data(&structData->array_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->array_type\n");
                return -1;
            }
            break;
        case 4:
            if (TRAVODDS::BitSetTypeTypeSupport::get_instance()->deserialize_data(&structData->bitset_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->bitset_type\n");
                return -1;
            }
            break;
        case 5:
            if (TRAVODDS::EnumerationTypeTypeSupport::get_instance()->deserialize_data(&structData->enumeration_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->enumeration_type\n");
                return -1;
            }
            break;
        case 6:
            if (TRAVODDS::MapTypeTypeSupport::get_instance()->deserialize_data(&structData->map_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->map_type\n");
                return -1;
            }
            break;
        case 7:
            if (TRAVODDS::SequenceTypeTypeSupport::get_instance()->deserialize_data(&structData->sequence_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->sequence_type\n");
                return -1;
            }
            break;
        case 8:
            if (TRAVODDS::StringTypeTypeSupport::get_instance()->deserialize_data(&structData->string_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->string_type\n");
                return -1;
            }
            break;
        case 9:
            if (TRAVODDS::StructureTypeTypeSupport::get_instance()->deserialize_data(&structData->structure_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->structure_type\n");
                return -1;
            }
            break;
        case 10:
            if (TRAVODDS::UnionTypeTypeSupport::get_instance()->deserialize_data(&structData->union_type, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->union_type\n");
                return -1;
            }
            break;
        case 11:
            if (TRAVODDS::_ModuleTypeSupport::get_instance()->deserialize_data(&structData->mod, cdr, endian) != 0) {
                fprintf(stderr, "Deserialization failed for field: structData->mod\n");
                return -1;
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::TypeLibraryElement\n");
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

TypeObject* TRAVODDS::TypeLibraryElementTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::TypeLibraryElementTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    //TRAVODDS::TypeLibraryElement* structData = static_cast<TRAVODDS::TypeLibraryElement*>(data);
    //bool useExtend = true;
    //char* lenAddr = nullptr;
    //unsigned int beforeLen = 0;
    //unsigned int afterLen = 0;
    return 0;
}

int TRAVODDS::TypeLibraryElementTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::TypeLibraryElementTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::TypeLibraryElementTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::TypeLibraryElementTypeSupport::get_typename() {
    return "TRAVODDS::TypeLibraryElement";
}

TRAVODDS::TypeLibraryTypeSupport* TRAVODDS::TypeLibraryTypeSupport::get_instance() {
    static TRAVODDS::TypeLibraryTypeSupport instance;
    return &instance;
}

void* TRAVODDS::TypeLibraryTypeSupport::create_data() {
    return new TRAVODDS::TypeLibrary();
}

void TRAVODDS::TypeLibraryTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::TypeLibrary*>(data);
}

int TRAVODDS::TypeLibraryTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::TypeLibrary* dstData = static_cast<TRAVODDS::TypeLibrary*>(dst);
    TRAVODDS::TypeLibrary* srcData = static_cast<TRAVODDS::TypeLibrary*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::TypeLibraryTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::TypeLibrary* structData = static_cast<TRAVODDS::TypeLibrary*>(data);
    unsigned int initialAlignment = currentAlignment;
    //unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->element.size(); ++i) {
        currentAlignment += TRAVODDS::TypeLibraryElementTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->element[i], currentAlignment);
    }
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::TypeLibraryTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::TypeLibraryTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::TypeLibrary* structData = static_cast<TRAVODDS::TypeLibrary*>(data);
    uint32_t tmpLength = 0;
    tmpLength = static_cast<uint32_t>(structData->element.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->element\n");
        return -1;
    }
    for (size_t i = 0; i < structData->element.size(); ++i) {
        if (TRAVODDS::TypeLibraryElementTypeSupport::get_instance()->serialize_data((void*)&structData->element[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->element[i]\n");
            return -1;
        }
    }
    return 0;
}

int TRAVODDS::TypeLibraryTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::TypeLibrary* structData = static_cast<TRAVODDS::TypeLibrary*>(data);
    unsigned int tmpLength = 0;
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->element\n");
        return -1;
    }
    structData->element.resize(tmpLength);
    for (size_t i = 0; i < tmpLength; ++i) {
        if (TRAVODDS::TypeLibraryElementTypeSupport::get_instance()->deserialize_data(&structData->element[i], cdr, endian) != 0) {
            fprintf(stderr, "Deserialization failed for field: structData->element[i]\n");
            return -1;
        }
    }
    return 0;
}

TypeObject* TRAVODDS::TypeLibraryTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::TypeLibraryTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    //TRAVODDS::TypeLibrary* structData = static_cast<TRAVODDS::TypeLibrary*>(data);
    //bool memberHasKey = false;
    return 0;
}

int TRAVODDS::TypeLibraryTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::TypeLibraryTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::TypeLibraryTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::TypeLibraryTypeSupport::get_typename() {
    return "TRAVODDS::TypeLibrary";
}

TRAVODDS::TypeObjectTypeSupport* TRAVODDS::TypeObjectTypeSupport::get_instance() {
    static TRAVODDS::TypeObjectTypeSupport instance;
    return &instance;
}

void* TRAVODDS::TypeObjectTypeSupport::create_data() {
    return new TRAVODDS::TypeObject();
}

void TRAVODDS::TypeObjectTypeSupport::delete_data(void* data) {
    delete static_cast<TRAVODDS::TypeObject*>(data);
}

int TRAVODDS::TypeObjectTypeSupport::copy_data(void* dst, void* src) {
    TRAVODDS::TypeObject* dstData = static_cast<TRAVODDS::TypeObject*>(dst);
    TRAVODDS::TypeObject* srcData = static_cast<TRAVODDS::TypeObject*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TRAVODDS::TypeObjectTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TRAVODDS::TypeObject* structData = static_cast<TRAVODDS::TypeObject*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += (currentAlignment & 1) == 0 ? 0 : (2 - (currentAlignment & 1));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += TRAVODDS::TypeLibraryTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->library, currentAlignment);
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    currentAlignment += 4;
    tmpAlignment = currentAlignment;
    currentAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->the_type.size(); ++i) {
        currentAlignment += TRAVODDS::_TypeIdTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->the_type[i], currentAlignment);
    }
    currentAlignment += tmpAlignment;
    currentAlignment += (currentAlignment & 3) == 0 ? 0 : (4 - (currentAlignment & 3));
    return currentAlignment - initialAlignment;
}

unsigned int TRAVODDS::TypeObjectTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int TRAVODDS::TypeObjectTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TRAVODDS::TypeObject* structData = static_cast<TRAVODDS::TypeObject*>(data);
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
    if (TRAVODDS::TypeLibraryTypeSupport::get_instance()->serialize_data((void*)&structData->library, cdr, endian) != 0) {
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
        if (TRAVODDS::_TypeIdTypeSupport::get_instance()->serialize_data((void*)&structData->the_type[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->the_type[i]\n");
            return -1;
        }
    }
    afterLen = cdr->getRemainingSpace();
    cdr->parameterLenSerialize(lenAddr, beforeLen - afterLen, useExtend ? 4 : 2);
    return 0;
}

int TRAVODDS::TypeObjectTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TRAVODDS::TypeObject* structData = static_cast<TRAVODDS::TypeObject*>(data);
    unsigned int tmpLength = 0;
    unsigned int memberId = 0;
    unsigned int length = 0;
    bool useExtend = false;
    bool mustUnderstand = false;
    unsigned int readLen = 0;
    while(memberId != PID_LIST_END) {
        if (!cdr->deserializeParameterHeader(memberId, length, useExtend, mustUnderstand)) {
            fprintf(stderr, "Deserialization failed for field: TRAVODDS::TypeObject\n");
            return -1;
        }
        readLen = cdr->getBufferLength();
        switch(memberId) {
        case 0:
            if (TRAVODDS::TypeLibraryTypeSupport::get_instance()->deserialize_data(&structData->library, cdr, endian) != 0) {
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
                if (TRAVODDS::_TypeIdTypeSupport::get_instance()->deserialize_data(&structData->the_type[i], cdr, endian) != 0) {
                    fprintf(stderr, "Deserialization failed for field: structData->the_type[i]\n");
                    return -1;
                }
            }
            break;
        case PID_LIST_END:
            break;
        default:
            if (mustUnderstand) {
                fprintf(stderr, "Deserialization failed for field: TRAVODDS::TypeObject\n");
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

TypeObject* TRAVODDS::TypeObjectTypeSupport::get_typeobject() {
    return nullptr;
}

int TRAVODDS::TypeObjectTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    //TRAVODDS::TypeObject* structData = static_cast<TRAVODDS::TypeObject*>(data);
    return 0;
}

int TRAVODDS::TypeObjectTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TRAVODDS::TypeObjectTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool TRAVODDS::TypeObjectTypeSupport::has_key() {
    return false;
}

const char* TRAVODDS::TypeObjectTypeSupport::get_typename() {
    return "TRAVODDS::TypeObject";
}

#ifdef __cplusplus
}
#endif
