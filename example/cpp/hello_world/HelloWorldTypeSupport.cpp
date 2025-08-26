#include "HelloWorldTypeSupport.h"

USING_TRAVODDS_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

HelloWorldTypeSupport* HelloWorldTypeSupport::get_instance() {
    static HelloWorldTypeSupport instance;
    return &instance;
}

void* HelloWorldTypeSupport::create_data() {
    return new HelloWorld();
}

void HelloWorldTypeSupport::delete_data(void* data) {
    delete static_cast<HelloWorld*>(data);
}

int HelloWorldTypeSupport::copy_data(void* dst, void* src) {
    HelloWorld* dstData = static_cast<HelloWorld*>(dst);
    HelloWorld* srcData = static_cast<HelloWorld*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int HelloWorldTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    HelloWorld* structData = static_cast<HelloWorld*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += AGIDDSCdrSerializer::getBaseTypeSize(sizeof(unsigned int), currentAlignment);
    currentAlignment += AGIDDSCdrSerializer::getStringSize(structData->message.size() + 1, currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int HelloWorldTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int HelloWorldTypeSupport::serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    HelloWorld* structData = static_cast<HelloWorld*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseType(structData->index)) {
        fprintf(stderr, "Serialization failed for field: structData->index\n");
        return -1;
    }
    if (!cdr->serializeString(structData->message)) {
        fprintf(stderr, "Serialization failed for field: structData->message\n");
        return -1;
    }
    return 0;
}

int HelloWorldTypeSupport::deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) {
    HelloWorld* structData = static_cast<HelloWorld*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeBaseType(structData->index)) {
        fprintf(stderr, "Deserialization failed for field: structData->index\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->message)) {
        fprintf(stderr, "Deserialization failed for field: structData->message\n");
        return -1;
    }
    return 0;
}

TypeObject* HelloWorldTypeSupport::get_typeobject() {
    return nullptr;
}

int HelloWorldTypeSupport::serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian) {
    HelloWorld* structData = static_cast<HelloWorld*>(data);
    bool memberHasKey = false;
    return 0;
}

int HelloWorldTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    AGIDDSCdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t HelloWorldTypeSupport::get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool HelloWorldTypeSupport::has_key() {
    return false;
}

const char* HelloWorldTypeSupport::get_typename() {
    return "HelloWorld";
}

#ifdef __cplusplus
}
#endif
