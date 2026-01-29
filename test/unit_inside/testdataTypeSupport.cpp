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

TestModule::NestedStructTypeSupport* TestModule::NestedStructTypeSupport::get_instance() {
    static TestModule::NestedStructTypeSupport instance;
    return &instance;
}

void* TestModule::NestedStructTypeSupport::create_data() {
    return new TestModule::NestedStruct();
}

void TestModule::NestedStructTypeSupport::delete_data(void* data) {
    delete static_cast<TestModule::NestedStruct*>(data);
}

int TestModule::NestedStructTypeSupport::copy_data(void* dst, void* src) {
    TestModule::NestedStruct* dstData = static_cast<TestModule::NestedStruct*>(dst);
    TestModule::NestedStruct* srcData = static_cast<TestModule::NestedStruct*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TestModule::NestedStructTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TestModule::NestedStruct* structData = static_cast<TestModule::NestedStruct*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(double), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(structData->nestedName.size() + 1, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int), sizeof(structData->nestedArray) / sizeof(int), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int TestModule::NestedStructTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int TestModule::NestedStructTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TestModule::NestedStruct* structData = static_cast<TestModule::NestedStruct*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->nestedId)) {
        fprintf(stderr, "Serialization failed for field: structData->nestedId\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->nestedValue)) {
        fprintf(stderr, "Serialization failed for field: structData->nestedValue\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->nestedFlag)) {
        fprintf(stderr, "Serialization failed for field: structData->nestedFlag\n");
        return -1;
    }
    if (!cdr->serializeString(structData->nestedName)) {
        fprintf(stderr, "Serialization failed for field: structData->nestedName\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray((int*)structData->nestedArray, sizeof(structData->nestedArray) / sizeof(int))) {
        fprintf(stderr, "Serialization failed for field: structData->nestedArray\n");
        return -1;
    }
    return 0;
}

int TestModule::NestedStructTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TestModule::NestedStruct* structData = static_cast<TestModule::NestedStruct*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->nestedId)) {
        fprintf(stderr, "Deserialization failed for field: structData->nestedId\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->nestedValue)) {
        fprintf(stderr, "Deserialization failed for field: structData->nestedValue\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->nestedFlag)) {
        fprintf(stderr, "Deserialization failed for field: structData->nestedFlag\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->nestedName)) {
        fprintf(stderr, "Deserialization failed for field: structData->nestedName\n");
        return -1;
    }
    if (!cdr->deserializeBaseTypeArray((int*)structData->nestedArray, sizeof(structData->nestedArray) / sizeof(int))) {
        fprintf(stderr, "Deserialization failed for field: structData->nestedArray\n");
        return -1;
    }
    return 0;
}

TypeObject* TestModule::NestedStructTypeSupport::get_typeobject() {
    return nullptr;
}

int TestModule::NestedStructTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TestModule::NestedStruct* structData = static_cast<TestModule::NestedStruct*>(data);
    bool memberHasKey = false;
    return 0;
}

int TestModule::NestedStructTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TestModule::NestedStructTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

DynamicType* TestModule::NestedStructTypeSupport::get_dynamictype() {
    return const_cast<DynamicType*>(_DynamicType::TestModule::NestedStruct);
}

bool TestModule::NestedStructTypeSupport::has_key() {
    return false;
}

const char* TestModule::NestedStructTypeSupport::get_typename() {
    return "TestModule::NestedStruct";
}

TestModule::FilterableDataTypeSupport* TestModule::FilterableDataTypeSupport::get_instance() {
    static TestModule::FilterableDataTypeSupport instance;
    return &instance;
}

void* TestModule::FilterableDataTypeSupport::create_data() {
    return new TestModule::FilterableData();
}

void TestModule::FilterableDataTypeSupport::delete_data(void* data) {
    delete static_cast<TestModule::FilterableData*>(data);
}

int TestModule::FilterableDataTypeSupport::copy_data(void* dst, void* src) {
    TestModule::FilterableData* dstData = static_cast<TestModule::FilterableData*>(dst);
    TestModule::FilterableData* srcData = static_cast<TestModule::FilterableData*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int TestModule::FilterableDataTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    TestModule::FilterableData* structData = static_cast<TestModule::FilterableData*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(short), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned short), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(long long), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(float), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(double), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(char), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned char), currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(structData->stringField.size() + 1, currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(structData->fixedStringField.size() + 1, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(32 / 8, currentAlignment);
    currentAlignment += TestModule::NestedStructTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->nestedField, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int), sizeof(structData->longArray) / sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(double), sizeof(structData->doubleArray) / sizeof(double), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(char), sizeof(structData->boolArray) / sizeof(char), currentAlignment);
    for (size_t i = 0; i < sizeof(structData->stringArray) / sizeof(structData->stringArray[0]); ++i) {
        currentAlignment += CdrSerializer::getStringSize(structData->stringArray[i].size() + 1, currentAlignment);
    }
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int), sizeof(structData->matrix2x3) / sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(float), sizeof(structData->floatMatrix) / sizeof(float), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int), sizeof(structData->matrix4x4) / sizeof(int), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(int), structData->seqLongField.size(), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->seqStringField.size(); ++i) {
        currentAlignment += CdrSerializer::getStringSize(structData->seqStringField[i].size() + 1, currentAlignment);
    }
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(short), sizeof(structData->shortBuffer) / sizeof(short), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(char), sizeof(structData->charBuffer) / sizeof(char), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(unsigned char), sizeof(structData->byteBuffer) / sizeof(unsigned char), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(double), sizeof(structData->measurements) / sizeof(double), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(TestModule::TestEnum), sizeof(structData->enumArray) / sizeof(TestModule::TestEnum), currentAlignment);
    for (size_t i = 0; i < sizeof(structData->structArray) / sizeof(structData->structArray[0]); ++i) {
        currentAlignment += TestModule::NestedStructTypeSupport::get_instance()->get_serialized_data_size(&structData->structArray[i], currentAlignment);
    }
    return currentAlignment - initialAlignment;
}

unsigned int TestModule::FilterableDataTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int TestModule::FilterableDataTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    TestModule::FilterableData* structData = static_cast<TestModule::FilterableData*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->shortField)) {
        fprintf(stderr, "Serialization failed for field: structData->shortField\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->ushortField)) {
        fprintf(stderr, "Serialization failed for field: structData->ushortField\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->longField)) {
        fprintf(stderr, "Serialization failed for field: structData->longField\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->ulongField)) {
        fprintf(stderr, "Serialization failed for field: structData->ulongField\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->longlongField)) {
        fprintf(stderr, "Serialization failed for field: structData->longlongField\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->ulonglongField)) {
        fprintf(stderr, "Serialization failed for field: structData->ulonglongField\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->floatField)) {
        fprintf(stderr, "Serialization failed for field: structData->floatField\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->doubleField)) {
        fprintf(stderr, "Serialization failed for field: structData->doubleField\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->boolField)) {
        fprintf(stderr, "Serialization failed for field: structData->boolField\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->charField)) {
        fprintf(stderr, "Serialization failed for field: structData->charField\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->byteField)) {
        fprintf(stderr, "Serialization failed for field: structData->byteField\n");
        return -1;
    }
    if (!cdr->serializeString(structData->stringField)) {
        fprintf(stderr, "Serialization failed for field: structData->stringField\n");
        return -1;
    }
    if (!cdr->serializeString(structData->fixedStringField)) {
        fprintf(stderr, "Serialization failed for field: structData->fixedStringField\n");
        return -1;
    }
    switch(32 / 8) {
    case 1:
        if (!cdr->serializeBaseType(static_cast<char>(structData->enumField))) {
            fprintf(stderr, "Serialization failed for field: structData->enumField\n");
            return -1;
        }
        break;
    case 2:
        if (!cdr->serializeBaseType(static_cast<short>(structData->enumField))) {
            fprintf(stderr, "Serialization failed for field: structData->enumField\n");
            return -1;
        }
        break;
    case 4:
        if (!cdr->serializeBaseType(static_cast<int>(structData->enumField))) {
            fprintf(stderr, "Serialization failed for field: structData->enumField\n");
            return -1;
        }
        break;
    default:
        fprintf(stderr, "Serialization failed for field: structData->enumField\n");
        return -1;
        break;
    }
    if (TestModule::NestedStructTypeSupport::get_instance()->serialize_data((void*)&structData->nestedField, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->nestedField\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray((int*)structData->longArray, sizeof(structData->longArray) / sizeof(int))) {
        fprintf(stderr, "Serialization failed for field: structData->longArray\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray((double*)structData->doubleArray, sizeof(structData->doubleArray) / sizeof(double))) {
        fprintf(stderr, "Serialization failed for field: structData->doubleArray\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray((char*)structData->boolArray, sizeof(structData->boolArray) / sizeof(char))) {
        fprintf(stderr, "Serialization failed for field: structData->boolArray\n");
        return -1;
    }
    for (size_t i = 0; i < sizeof(structData->stringArray) / sizeof(structData->stringArray[0]); ++i) {
        if (!cdr->serializeString(structData->stringArray[i])) {
            fprintf(stderr, "Serialization failed for field: structData->stringArray\n");
            return -1;
        }
    }
    if (!cdr->serializeBaseTypeArray((int*)structData->matrix2x3, sizeof(structData->matrix2x3) / sizeof(int))) {
        fprintf(stderr, "Serialization failed for field: structData->matrix2x3\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray((float*)structData->floatMatrix, sizeof(structData->floatMatrix) / sizeof(float))) {
        fprintf(stderr, "Serialization failed for field: structData->floatMatrix\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray((int*)structData->matrix4x4, sizeof(structData->matrix4x4) / sizeof(int))) {
        fprintf(stderr, "Serialization failed for field: structData->matrix4x4\n");
        return -1;
    }
    tmpLength = static_cast<uint32_t>(structData->seqLongField.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->seqLongField\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray(structData->seqLongField.data(), structData->seqLongField.size())) {
        fprintf(stderr, "Serialization failed for field: structData->seqLongField\n");
        return -1;
    }
    tmpLength = static_cast<uint32_t>(structData->seqStringField.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->seqStringField\n");
        return -1;
    }
    for (size_t i = 0; i < structData->seqStringField.size(); ++i) {
        if (!cdr->serializeString(structData->seqStringField[i])) {
            fprintf(stderr, "Serialization failed for field: structData->seqStringField[i]\n");
            return -1;
        }
    }
    if (!cdr->serializeBaseTypeArray((short*)structData->shortBuffer, sizeof(structData->shortBuffer) / sizeof(short))) {
        fprintf(stderr, "Serialization failed for field: structData->shortBuffer\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray((char*)structData->charBuffer, sizeof(structData->charBuffer) / sizeof(char))) {
        fprintf(stderr, "Serialization failed for field: structData->charBuffer\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray((unsigned char*)structData->byteBuffer, sizeof(structData->byteBuffer) / sizeof(unsigned char))) {
        fprintf(stderr, "Serialization failed for field: structData->byteBuffer\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray((double*)structData->measurements, sizeof(structData->measurements) / sizeof(double))) {
        fprintf(stderr, "Serialization failed for field: structData->measurements\n");
        return -1;
    }
    for (size_t i = 0; i < sizeof(structData->enumArray) / sizeof(structData->enumArray[0]); ++i) {
        switch(32 / 8) {
        case 1:
            if (!cdr->serializeBaseType(static_cast<char>(structData->enumArray[i]))) {
                fprintf(stderr, "Serialization failed for field: structData->enumArray[i]\n");
                return -1;
            }
            break;
        case 2:
            if (!cdr->serializeBaseType(static_cast<short>(structData->enumArray[i]))) {
                fprintf(stderr, "Serialization failed for field: structData->enumArray[i]\n");
                return -1;
            }
            break;
        case 4:
            if (!cdr->serializeBaseType(static_cast<int>(structData->enumArray[i]))) {
                fprintf(stderr, "Serialization failed for field: structData->enumArray[i]\n");
                return -1;
            }
            break;
        default:
            fprintf(stderr, "Serialization failed for field: structData->enumArray[i]\n");
            return -1;
            break;
        }
    }
    for (size_t i = 0; i < sizeof(structData->structArray) / sizeof(structData->structArray[0]); ++i) {
        if (TestModule::NestedStructTypeSupport::get_instance()->serialize_data((TestModule::NestedStruct*)(structData->structArray + i), cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->structArray[i]\n");
            return -1;
        }
    }
    return 0;
}

int TestModule::FilterableDataTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    TestModule::FilterableData* structData = static_cast<TestModule::FilterableData*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->shortField)) {
        fprintf(stderr, "Deserialization failed for field: structData->shortField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->ushortField)) {
        fprintf(stderr, "Deserialization failed for field: structData->ushortField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->longField)) {
        fprintf(stderr, "Deserialization failed for field: structData->longField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->ulongField)) {
        fprintf(stderr, "Deserialization failed for field: structData->ulongField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->longlongField)) {
        fprintf(stderr, "Deserialization failed for field: structData->longlongField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->ulonglongField)) {
        fprintf(stderr, "Deserialization failed for field: structData->ulonglongField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->floatField)) {
        fprintf(stderr, "Deserialization failed for field: structData->floatField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->doubleField)) {
        fprintf(stderr, "Deserialization failed for field: structData->doubleField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->boolField)) {
        fprintf(stderr, "Deserialization failed for field: structData->boolField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->charField)) {
        fprintf(stderr, "Deserialization failed for field: structData->charField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->byteField)) {
        fprintf(stderr, "Deserialization failed for field: structData->byteField\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->stringField)) {
        fprintf(stderr, "Deserialization failed for field: structData->stringField\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->fixedStringField)) {
        fprintf(stderr, "Deserialization failed for field: structData->fixedStringField\n");
        return -1;
    }
    switch(32 / 8) {
    case 1:
        if (!cdr->deserializeBaseType(tmpCharEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->enumField\n");
            return -1;
        }
        structData->enumField = static_cast<TestModule::TestEnum>(tmpCharEnum);
        break;
    case 2:
        if (!cdr->deserializeBaseType(tmpShortEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->enumField\n");
            return -1;
        }
        structData->enumField = static_cast<TestModule::TestEnum>(tmpShortEnum);
        break;
    case 4:
        if (!cdr->deserializeBaseType(tmpIntEnum)) {
            fprintf(stderr, "Deserialization failed for field: structData->enumField\n");
            return -1;
        }
        structData->enumField = static_cast<TestModule::TestEnum>(tmpIntEnum);
        break;
    default:
        fprintf(stderr, "Deserialization failed for field: structData->enumField\n");
        return -1;
        break;
    }
    if (TestModule::NestedStructTypeSupport::get_instance()->deserialize_data(&structData->nestedField, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->nestedField\n");
        return -1;
    }
    if (!cdr->deserializeBaseTypeArray((int*)structData->longArray, sizeof(structData->longArray) / sizeof(int))) {
        fprintf(stderr, "Deserialization failed for field: structData->longArray\n");
        return -1;
    }
    if (!cdr->deserializeBaseTypeArray((double*)structData->doubleArray, sizeof(structData->doubleArray) / sizeof(double))) {
        fprintf(stderr, "Deserialization failed for field: structData->doubleArray\n");
        return -1;
    }
    if (!cdr->deserializeBaseTypeArray((char*)structData->boolArray, sizeof(structData->boolArray) / sizeof(char))) {
        fprintf(stderr, "Deserialization failed for field: structData->boolArray\n");
        return -1;
    }
    for (size_t i = 0; i < sizeof(structData->stringArray) / sizeof(structData->stringArray[0]); ++i) {
        if (!cdr->deserializeString(structData->stringArray[i])) {
            fprintf(stderr, "Deserialization failed for field: structData->stringArray\n");
            return -1;
        }
    }
    if (!cdr->deserializeBaseTypeArray((int*)structData->matrix2x3, sizeof(structData->matrix2x3) / sizeof(int))) {
        fprintf(stderr, "Deserialization failed for field: structData->matrix2x3\n");
        return -1;
    }
    if (!cdr->deserializeBaseTypeArray((float*)structData->floatMatrix, sizeof(structData->floatMatrix) / sizeof(float))) {
        fprintf(stderr, "Deserialization failed for field: structData->floatMatrix\n");
        return -1;
    }
    if (!cdr->deserializeBaseTypeArray((int*)structData->matrix4x4, sizeof(structData->matrix4x4) / sizeof(int))) {
        fprintf(stderr, "Deserialization failed for field: structData->matrix4x4\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->seqLongField\n");
        return -1;
    }
    structData->seqLongField.resize(tmpLength);
    if (!cdr->deserializeBaseTypeArray(structData->seqLongField.data(), tmpLength)) {
        fprintf(stderr, "Deserialization failed for field: structData->seqLongField\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->seqStringField\n");
        return -1;
    }
    structData->seqStringField.resize(tmpLength);
    for (size_t i = 0; i < tmpLength; ++i) {
        if (!cdr->deserializeString(structData->seqStringField[i])) {
            fprintf(stderr, "Deserialization failed for field: structData->seqStringField[i]\n");
            return -1;
        }
    }
    if (!cdr->deserializeBaseTypeArray((short*)structData->shortBuffer, sizeof(structData->shortBuffer) / sizeof(short))) {
        fprintf(stderr, "Deserialization failed for field: structData->shortBuffer\n");
        return -1;
    }
    if (!cdr->deserializeBaseTypeArray((char*)structData->charBuffer, sizeof(structData->charBuffer) / sizeof(char))) {
        fprintf(stderr, "Deserialization failed for field: structData->charBuffer\n");
        return -1;
    }
    if (!cdr->deserializeBaseTypeArray((unsigned char*)structData->byteBuffer, sizeof(structData->byteBuffer) / sizeof(unsigned char))) {
        fprintf(stderr, "Deserialization failed for field: structData->byteBuffer\n");
        return -1;
    }
    if (!cdr->deserializeBaseTypeArray((double*)structData->measurements, sizeof(structData->measurements) / sizeof(double))) {
        fprintf(stderr, "Deserialization failed for field: structData->measurements\n");
        return -1;
    }
    for (size_t i = 0; i < sizeof(structData->enumArray) / sizeof(structData->enumArray[0]); ++i) {
        switch(32 / 8) {
        case 1:
            if (!cdr->deserializeBaseType(tmpCharEnum)) {
                fprintf(stderr, "Deserialization failed for field: structData->enumArray[i]\n");
                return -1;
            }
            structData->enumArray[i] = static_cast<TestModule::TestEnum>(tmpCharEnum);
            break;
        case 2:
            if (!cdr->deserializeBaseType(tmpShortEnum)) {
                fprintf(stderr, "Deserialization failed for field: structData->enumArray[i]\n");
                return -1;
            }
            structData->enumArray[i] = static_cast<TestModule::TestEnum>(tmpShortEnum);
            break;
        case 4:
            if (!cdr->deserializeBaseType(tmpIntEnum)) {
                fprintf(stderr, "Deserialization failed for field: structData->enumArray[i]\n");
                return -1;
            }
            structData->enumArray[i] = static_cast<TestModule::TestEnum>(tmpIntEnum);
            break;
        default:
            fprintf(stderr, "Deserialization failed for field: structData->enumArray[i]\n");
            return -1;
            break;
        }
    }
    for (size_t i = 0; i < sizeof(structData->structArray) / sizeof(structData->structArray[0]); ++i) {
        if (TestModule::NestedStructTypeSupport::get_instance()->deserialize_data((TestModule::NestedStruct*)(structData->structArray + i), cdr, endian) != 0) {
            fprintf(stderr, "Deserialization failed for field: structData->structArray[i]\n");
            return -1;
        }
    }
    return 0;
}

TypeObject* TestModule::FilterableDataTypeSupport::get_typeobject() {
    return nullptr;
}

int TestModule::FilterableDataTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    TestModule::FilterableData* structData = static_cast<TestModule::FilterableData*>(data);
    bool memberHasKey = false;
    return 0;
}

int TestModule::FilterableDataTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t TestModule::FilterableDataTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

DynamicType* TestModule::FilterableDataTypeSupport::get_dynamictype() {
    return const_cast<DynamicType*>(_DynamicType::TestModule::FilterableData);
}

bool TestModule::FilterableDataTypeSupport::has_key() {
    return false;
}

const char* TestModule::FilterableDataTypeSupport::get_typename() {
    return "TestModule::FilterableData";
}

#ifdef __cplusplus
}
#endif
