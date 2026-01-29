#include "rtps/basictypertps.h"
#include "security/datatypesupport.h"
#include "security/participantgenericmessage.h"

TRAVODDS_NAMESPACE_BEGIN
#ifdef DDSSECURITY
BuiltinTopicKey_tTypeSupport* BuiltinTopicKey_tTypeSupport::get_instance() {
    static BuiltinTopicKey_tTypeSupport instance;
    return &instance;
}

void* BuiltinTopicKey_tTypeSupport::create_data() {
    return new BuiltinTopicKey_t();
}

void BuiltinTopicKey_tTypeSupport::delete_data(void* data) {
    delete static_cast<BuiltinTopicKey_t*>(data);
}

int BuiltinTopicKey_tTypeSupport::copy_data(void* dst, void* src) {
    BuiltinTopicKey_t* dstData = static_cast<BuiltinTopicKey_t*>(dst);
    BuiltinTopicKey_t* srcData = static_cast<BuiltinTopicKey_t*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int BuiltinTopicKey_tTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    BuiltinTopicKey_t* structData = static_cast<BuiltinTopicKey_t*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int), sizeof(structData->value) / sizeof(int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int BuiltinTopicKey_tTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    BuiltinTopicKey_t* structData = static_cast<BuiltinTopicKey_t*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int), sizeof(structData->value) / sizeof(int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int BuiltinTopicKey_tTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    BuiltinTopicKey_t* structData = static_cast<BuiltinTopicKey_t*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseTypeArray((int*)structData->value, sizeof(structData->value) / sizeof(int))) {
        fprintf(stderr, "Serialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

int BuiltinTopicKey_tTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    BuiltinTopicKey_t* structData = static_cast<BuiltinTopicKey_t*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseTypeArray((int*)structData->value, sizeof(structData->value) / sizeof(int))) {
        fprintf(stderr, "Deserialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

TypeObject* BuiltinTopicKey_tTypeSupport::get_typeobject() {
    return nullptr;
}

int BuiltinTopicKey_tTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    BuiltinTopicKey_t* structData = static_cast<BuiltinTopicKey_t*>(data);
    bool memberHasKey = false;
    return 0;
}

int BuiltinTopicKey_tTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t BuiltinTopicKey_tTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool BuiltinTopicKey_tTypeSupport::has_key() {
    return false;
}

const char* BuiltinTopicKey_tTypeSupport::get_typename() {
    return "BuiltinTopicKey_t";
}

SequenceNumber_tTypeSupport* SequenceNumber_tTypeSupport::get_instance() {
    static SequenceNumber_tTypeSupport instance;
    return &instance;
}

void* SequenceNumber_tTypeSupport::create_data() {
    return new SequenceNumber_t();
}

void SequenceNumber_tTypeSupport::delete_data(void* data) {
    delete static_cast<SequenceNumber_t*>(data);
}

int SequenceNumber_tTypeSupport::copy_data(void* dst, void* src) {
    SequenceNumber_t* dstData = static_cast<SequenceNumber_t*>(dst);
    SequenceNumber_t* srcData = static_cast<SequenceNumber_t*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int SequenceNumber_tTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    SequenceNumber_t* structData = static_cast<SequenceNumber_t*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int SequenceNumber_tTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    SequenceNumber_t* structData = static_cast<SequenceNumber_t*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    return currentAlignment - initialAlignment;
}

int SequenceNumber_tTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    SequenceNumber_t* structData = static_cast<SequenceNumber_t*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->low)) {
        fprintf(stderr, "Serialization failed for field: structData->low\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->high)) {
        fprintf(stderr, "Serialization failed for field: structData->high\n");
        return -1;
    }
    return 0;
}

int SequenceNumber_tTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    SequenceNumber_t* structData = static_cast<SequenceNumber_t*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->low)) {
        fprintf(stderr, "Deserialization failed for field: structData->low\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->high)) {
        fprintf(stderr, "Deserialization failed for field: structData->high\n");
        return -1;
    }
    return 0;
}

TypeObject* SequenceNumber_tTypeSupport::get_typeobject() {
    return nullptr;
}

int SequenceNumber_tTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    SequenceNumber_t* structData = static_cast<SequenceNumber_t*>(data);
    bool memberHasKey = false;
    return 0;
}

int SequenceNumber_tTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t SequenceNumber_tTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool SequenceNumber_tTypeSupport::has_key() {
    return false;
}

const char* SequenceNumber_tTypeSupport::get_typename() {
    return "SequenceNumber_t";
}

MessageIdentityTypeSupport* MessageIdentityTypeSupport::get_instance() {
    static MessageIdentityTypeSupport instance;
    return &instance;
}

void* MessageIdentityTypeSupport::create_data() {
    return new MessageIdentity();
}

void MessageIdentityTypeSupport::delete_data(void* data) {
    delete static_cast<MessageIdentity*>(data);
}

int MessageIdentityTypeSupport::copy_data(void* dst, void* src) {
    MessageIdentity* dstData = static_cast<MessageIdentity*>(dst);
    MessageIdentity* srcData = static_cast<MessageIdentity*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int MessageIdentityTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    MessageIdentity* structData = static_cast<MessageIdentity*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += BuiltinTopicKey_tTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->sourceGuid, currentAlignment);
    currentAlignment += SequenceNumber_tTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->sequenceNumber, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int MessageIdentityTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    MessageIdentity* structData = static_cast<MessageIdentity*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += BuiltinTopicKey_tTypeSupport::get_instance()->get_max_serialized_data_size(&structData->sourceGuid, currentAlignment);
    currentAlignment += SequenceNumber_tTypeSupport::get_instance()->get_max_serialized_data_size(&structData->sequenceNumber, currentAlignment);
    return currentAlignment - initialAlignment;
}

int MessageIdentityTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    MessageIdentity* structData = static_cast<MessageIdentity*>(data);
    uint32_t tmpLength = 0;
    if (BuiltinTopicKey_tTypeSupport::get_instance()->serialize_data((void*)&structData->sourceGuid, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->sourceGuid\n");
        return -1;
    }
    if (SequenceNumber_tTypeSupport::get_instance()->serialize_data((void*)&structData->sequenceNumber, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->sequenceNumber\n");
        return -1;
    }
    return 0;
}

int MessageIdentityTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    MessageIdentity* structData = static_cast<MessageIdentity*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (BuiltinTopicKey_tTypeSupport::get_instance()->deserialize_data(&structData->sourceGuid, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->sourceGuid\n");
        return -1;
    }
    if (SequenceNumber_tTypeSupport::get_instance()->deserialize_data(&structData->sequenceNumber, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->sequenceNumber\n");
        return -1;
    }
    return 0;
}

TypeObject* MessageIdentityTypeSupport::get_typeobject() {
    return nullptr;
}

int MessageIdentityTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    MessageIdentity* structData = static_cast<MessageIdentity*>(data);
    bool memberHasKey = false;
    return 0;
}

int MessageIdentityTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t MessageIdentityTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool MessageIdentityTypeSupport::has_key() {
    return false;
}

const char* MessageIdentityTypeSupport::get_typename() {
    return "MessageIdentity";
}

Property_tTypeSupport* Property_tTypeSupport::get_instance() {
    static Property_tTypeSupport instance;
    return &instance;
}

void* Property_tTypeSupport::create_data() {
    return new Property_t();
}

void Property_tTypeSupport::delete_data(void* data) {
    delete static_cast<Property_t*>(data);
}

int Property_tTypeSupport::copy_data(void* dst, void* src) {
    Property_t* dstData = static_cast<Property_t*>(dst);
    Property_t* srcData = static_cast<Property_t*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int Property_tTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    Property_t* structData = static_cast<Property_t*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(structData->value.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int Property_tTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int Property_tTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    Property_t* structData = static_cast<Property_t*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    if (!cdr->serializeString(structData->value)) {
        fprintf(stderr, "Serialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

int Property_tTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    Property_t* structData = static_cast<Property_t*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->value)) {
        fprintf(stderr, "Deserialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

TypeObject* Property_tTypeSupport::get_typeobject() {
    return nullptr;
}

int Property_tTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    Property_t* structData = static_cast<Property_t*>(data);
    bool memberHasKey = false;
    return 0;
}

int Property_tTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t Property_tTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool Property_tTypeSupport::has_key() {
    return false;
}

const char* Property_tTypeSupport::get_typename() {
    return "Property_t";
}

BinaryProperty_tTypeSupport* BinaryProperty_tTypeSupport::get_instance() {
    static BinaryProperty_tTypeSupport instance;
    return &instance;
}

void* BinaryProperty_tTypeSupport::create_data() {
    return new BinaryProperty_t();
}

void BinaryProperty_tTypeSupport::delete_data(void* data) {
    delete static_cast<BinaryProperty_t*>(data);
}

int BinaryProperty_tTypeSupport::copy_data(void* dst, void* src) {
    BinaryProperty_t* dstData = static_cast<BinaryProperty_t*>(dst);
    BinaryProperty_t* srcData = static_cast<BinaryProperty_t*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int BinaryProperty_tTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    BinaryProperty_t* structData = static_cast<BinaryProperty_t*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(unsigned char), structData->value.size(), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int BinaryProperty_tTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int BinaryProperty_tTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    BinaryProperty_t* structData = static_cast<BinaryProperty_t*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    tmpLength = static_cast<uint32_t>(structData->value.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->value\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray(structData->value.data(), structData->value.size())) {
        fprintf(stderr, "Serialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

int BinaryProperty_tTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    BinaryProperty_t* structData = static_cast<BinaryProperty_t*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->value\n");
        return -1;
    }
    structData->value.resize(tmpLength);
    if (!cdr->deserializeBaseTypeArray(structData->value.data(), tmpLength)) {
        fprintf(stderr, "Deserialization failed for field: structData->value\n");
        return -1;
    }
    return 0;
}

TypeObject* BinaryProperty_tTypeSupport::get_typeobject() {
    return nullptr;
}

int BinaryProperty_tTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    BinaryProperty_t* structData = static_cast<BinaryProperty_t*>(data);
    bool memberHasKey = false;
    return 0;
}

int BinaryProperty_tTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t BinaryProperty_tTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool BinaryProperty_tTypeSupport::has_key() {
    return false;
}

const char* BinaryProperty_tTypeSupport::get_typename() {
    return "BinaryProperty_t";
}
#endif // DDSSECURITY
TRAVODDS_NAMESPACE_END