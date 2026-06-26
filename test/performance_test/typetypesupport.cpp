#include "typetypesupport.h"

USING_TRAVODDS_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

PerformanceTest::TestDataTypeSupport* PerformanceTest::TestDataTypeSupport::get_instance() {
    static PerformanceTest::TestDataTypeSupport instance;
    return &instance;
}

void* PerformanceTest::TestDataTypeSupport::create_data() {
    return new PerformanceTest::TestData();
}

void PerformanceTest::TestDataTypeSupport::delete_data(void* data) {
    delete static_cast<PerformanceTest::TestData*>(data);
}

int PerformanceTest::TestDataTypeSupport::copy_data(void* dst, void* src) {
    PerformanceTest::TestData* dstData = static_cast<PerformanceTest::TestData*>(dst);
    PerformanceTest::TestData* srcData = static_cast<PerformanceTest::TestData*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int PerformanceTest::TestDataTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    PerformanceTest::TestData* structData = static_cast<PerformanceTest::TestData*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(unsigned long long), currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(structData->data.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int PerformanceTest::TestDataTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int PerformanceTest::TestDataTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    PerformanceTest::TestData* structData = static_cast<PerformanceTest::TestData*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->sn)) {
        fprintf(stderr, "Serialization failed for field: structData->sn\n");
        return -1;
    }
    if (!cdr->serializeBaseType(structData->size)) {
        fprintf(stderr, "Serialization failed for field: structData->size\n");
        return -1;
    }
    if (!cdr->serializeString(structData->data)) {
        fprintf(stderr, "Serialization failed for field: structData->data\n");
        return -1;
    }
    return 0;
}

int PerformanceTest::TestDataTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    PerformanceTest::TestData* structData = static_cast<PerformanceTest::TestData*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->sn)) {
        fprintf(stderr, "Deserialization failed for field: structData->sn\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(structData->size)) {
        fprintf(stderr, "Deserialization failed for field: structData->size\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->data)) {
        fprintf(stderr, "Deserialization failed for field: structData->data\n");
        return -1;
    }
    return 0;
}

TypeObject* PerformanceTest::TestDataTypeSupport::get_typeobject() {
    return nullptr;
}

int PerformanceTest::TestDataTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    PerformanceTest::TestData* structData = static_cast<PerformanceTest::TestData*>(data);
    bool memberHasKey = false;
    return 0;
}

int PerformanceTest::TestDataTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t PerformanceTest::TestDataTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

DynamicType* PerformanceTest::TestDataTypeSupport::get_dynamictype() {
    return nullptr;
}

bool PerformanceTest::TestDataTypeSupport::has_key() {
    return false;
}

const char* PerformanceTest::TestDataTypeSupport::get_typename() {
    return "PerformanceTest::TestData";
}

#ifdef __cplusplus
}
#endif
