#include "testdataTypeSupport.h"
#include "testdataDynamicType.h"

USING_TRAVODDS_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

TestModule::BasicDataTypeSupport* TestModule::BasicDataTypeSupport::get_instance() {
    static TestModule::BasicDataTypeSupport instance;
    return &instance;
}

void* TestModule::BasicDataTypeSupport::create_data() {
    return new TestModule::BasicData();
}

void TestModule::BasicDataTypeSupport::delete_data(void* data) {
    delete static_cast<TestModule::BasicData*>(data);
}

int TestModule::BasicDataTypeSupport::copy_data(void* dst, void* src) {
    TestModule::BasicData* dstData = static_cast<TestModule::BasicData*>(dst);
    TestModule::BasicData* srcData = static_cast<TestModule::BasicData*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TestModule::BasicDataTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TestModule::BasicData* structData = static_cast<TestModule::BasicData*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(structData->name.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TestModule::BasicDataTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int TestModule::BasicDataTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TestModule::BasicData* structData = static_cast<TestModule::BasicData*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->id)) {
        fprintf(stderr, "Serialization failed for field: structData->id\n");
        return -1;
    }
    if (!cdr->serializeString(structData->name)) {
        fprintf(stderr, "Serialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

int TestModule::BasicDataTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TestModule::BasicData* structData = static_cast<TestModule::BasicData*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->id)) {
        fprintf(stderr, "Deserialization failed for field: structData->id\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->name)) {
        fprintf(stderr, "Deserialization failed for field: structData->name\n");
        return -1;
    }
    return 0;
}

TypeObject* TestModule::BasicDataTypeSupport::get_typeobject() {
    return nullptr;
}

int TestModule::BasicDataTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TestModule::BasicData* structData = static_cast<TestModule::BasicData*>(data);
    bool memberHasKey = false;
    return 0;
}

int TestModule::BasicDataTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TestModule::BasicDataTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

DynamicType* TestModule::BasicDataTypeSupport::get_dynamictype() {
    return const_cast<DynamicType*>(_DynamicType::TestModule::BasicData);
}

bool TestModule::BasicDataTypeSupport::has_key() {
    return false;
}

const char* TestModule::BasicDataTypeSupport::get_typename() {
    return "TestModule::BasicData";
}

TestModule::KeyDataTypeSupport* TestModule::KeyDataTypeSupport::get_instance() {
    static TestModule::KeyDataTypeSupport instance;
    return &instance;
}

void* TestModule::KeyDataTypeSupport::create_data() {
    return new TestModule::KeyData();
}

void TestModule::KeyDataTypeSupport::delete_data(void* data) {
    delete static_cast<TestModule::KeyData*>(data);
}

int TestModule::KeyDataTypeSupport::copy_data(void* dst, void* src) {
    TestModule::KeyData* dstData = static_cast<TestModule::KeyData*>(dst);
    TestModule::KeyData* srcData = static_cast<TestModule::KeyData*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TestModule::KeyDataTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TestModule::KeyData* structData = static_cast<TestModule::KeyData*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(structData->data.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TestModule::KeyDataTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int TestModule::KeyDataTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TestModule::KeyData* structData = static_cast<TestModule::KeyData*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->userId)) {
        fprintf(stderr, "Serialization failed for field: structData->userId\n");
        return -1;
    }
    if (!cdr->serializeString(structData->data)) {
        fprintf(stderr, "Serialization failed for field: structData->data\n");
        return -1;
    }
    return 0;
}

int TestModule::KeyDataTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TestModule::KeyData* structData = static_cast<TestModule::KeyData*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->userId)) {
        fprintf(stderr, "Deserialization failed for field: structData->userId\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->data)) {
        fprintf(stderr, "Deserialization failed for field: structData->data\n");
        return -1;
    }
    return 0;
}

TypeObject* TestModule::KeyDataTypeSupport::get_typeobject() {
    return nullptr;
}

int TestModule::KeyDataTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TestModule::KeyData* structData = static_cast<TestModule::KeyData*>(data);
    bool memberHasKey = false;
    if (!cdr->serializeBaseType(structData->userId)) {
        fprintf(stderr, "Serialization failed for field: structData->userId\n");
        return -1;
    }
    return 0;
}

int TestModule::KeyDataTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TestModule::KeyDataTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

DynamicType* TestModule::KeyDataTypeSupport::get_dynamictype() {
    return const_cast<DynamicType*>(_DynamicType::TestModule::KeyData);
}

bool TestModule::KeyDataTypeSupport::has_key() {
    return true;
}

const char* TestModule::KeyDataTypeSupport::get_typename() {
    return "TestModule::KeyData";
}

TestModule::SequenceDataTypeSupport* TestModule::SequenceDataTypeSupport::get_instance() {
    static TestModule::SequenceDataTypeSupport instance;
    return &instance;
}

void* TestModule::SequenceDataTypeSupport::create_data() {
    return new TestModule::SequenceData();
}

void TestModule::SequenceDataTypeSupport::delete_data(void* data) {
    delete static_cast<TestModule::SequenceData*>(data);
}

int TestModule::SequenceDataTypeSupport::copy_data(void* dst, void* src) {
    TestModule::SequenceData* dstData = static_cast<TestModule::SequenceData*>(dst);
    TestModule::SequenceData* srcData = static_cast<TestModule::SequenceData*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TestModule::SequenceDataTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TestModule::SequenceData* structData = static_cast<TestModule::SequenceData*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int), structData->numbers.size(), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TestModule::SequenceDataTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int TestModule::SequenceDataTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TestModule::SequenceData* structData = static_cast<TestModule::SequenceData*>(data);
    uint32_t tmpLength = 0;
    tmpLength = static_cast<uint32_t>(structData->numbers.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->numbers\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray(structData->numbers.data(), structData->numbers.size())) {
        fprintf(stderr, "Serialization failed for field: structData->numbers\n");
        return -1;
    }
    return 0;
}

int TestModule::SequenceDataTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TestModule::SequenceData* structData = static_cast<TestModule::SequenceData*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->numbers\n");
        return -1;
    }
    structData->numbers.resize(tmpLength);
    if (!cdr->deserializeBaseTypeArray(structData->numbers.data(), tmpLength)) {
        fprintf(stderr, "Deserialization failed for field: structData->numbers\n");
        return -1;
    }
    return 0;
}

TypeObject* TestModule::SequenceDataTypeSupport::get_typeobject() {
    return nullptr;
}

int TestModule::SequenceDataTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TestModule::SequenceData* structData = static_cast<TestModule::SequenceData*>(data);
    bool memberHasKey = false;
    return 0;
}

int TestModule::SequenceDataTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TestModule::SequenceDataTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

DynamicType* TestModule::SequenceDataTypeSupport::get_dynamictype() {
    return const_cast<DynamicType*>(_DynamicType::TestModule::SequenceData);
}

bool TestModule::SequenceDataTypeSupport::has_key() {
    return false;
}

const char* TestModule::SequenceDataTypeSupport::get_typename() {
    return "TestModule::SequenceData";
}

#ifdef __cplusplus
}
#endif
