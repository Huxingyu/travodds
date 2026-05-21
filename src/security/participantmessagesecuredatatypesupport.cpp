#include "participantmessagesecuredatatypesupport.h"
#include "common/log/logger.h"

TRAVODDS_NAMESPACE_BEGIN

ParticipantMessageSecureDataTypeSupport* ParticipantMessageSecureDataTypeSupport::get_instance() {
    static ParticipantMessageSecureDataTypeSupport instance;
    return &instance;
}

void* ParticipantMessageSecureDataTypeSupport::create_data() {
    return new ParticipantMessageSecureData();
}

void ParticipantMessageSecureDataTypeSupport::delete_data(void* data) {
    delete static_cast<ParticipantMessageSecureData*>(data);
}

int ParticipantMessageSecureDataTypeSupport::copy_data(void* dst, void* src) {
    ParticipantMessageSecureData* dstData = static_cast<ParticipantMessageSecureData*>(dst);
    ParticipantMessageSecureData* srcData = static_cast<ParticipantMessageSecureData*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int ParticipantMessageSecureDataTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    ParticipantMessageSecureData* structData = static_cast<ParticipantMessageSecureData*>(data);
    unsigned int initialAlignment = currentAlignment;
    // unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(char), sizeof(structData->m_kind) / sizeof(char), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeArraySize(sizeof(char), static_cast<uint32_t>(structData->m_extraData.size()), currentAlignment);
    return currentAlignment - initialAlignment;
}

unsigned int ParticipantMessageSecureDataTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    (void)data;
    (void)currentAlignment;
    return LENGTH_UNLIMITED;
}

int ParticipantMessageSecureDataTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) 
{
    (void)endian;
    ParticipantMessageSecureData* structData = static_cast<ParticipantMessageSecureData*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeBaseTypeArray((char*)structData->m_kind, sizeof(structData->m_kind) / sizeof(char))) {
        TRAVODDS_LOG(LOG_ERROR, "Serialization failed for field: structData->m_kind\n");
        return -1;
    }
    tmpLength = static_cast<uint32_t>(structData->m_extraData.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        TRAVODDS_LOG(LOG_ERROR, "Serialization length failed for field: structData->m_extraData\n");
        return -1;
    }
    if (!cdr->serializeBaseTypeArray(structData->m_extraData.data(), static_cast<uint32_t>(structData->m_extraData.size()))) {
        TRAVODDS_LOG(LOG_ERROR, "Serialization failed for field: structData->m_extraData\n");
        return -1;
    }
    return 0;
}

int ParticipantMessageSecureDataTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    (void)endian;
    ParticipantMessageSecureData* structData = static_cast<ParticipantMessageSecureData*>(data);
    unsigned int tmpLength = 0;
    if (!cdr->deserializeBaseTypeArray((char*)structData->m_kind, sizeof(structData->m_kind) / sizeof(char))) {
        TRAVODDS_LOG(LOG_ERROR, "Deserialization failed for field: structData->m_kind\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(tmpLength)) {
        TRAVODDS_LOG(LOG_ERROR, "Deserialization length failed for field: structData->m_extraData\n");
        return -1;
    }
    structData->m_extraData.resize(tmpLength);
    if (!cdr->deserializeBaseTypeArray(structData->m_extraData.data(), tmpLength)) {
        TRAVODDS_LOG(LOG_ERROR, "Deserialization failed for field: structData->m_extraData\n");
        return -1;
    }
    return 0;
}

TypeObject* ParticipantMessageSecureDataTypeSupport::get_typeobject() {
    return nullptr;
}

int ParticipantMessageSecureDataTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    (void)data;
    (void)cdr;
    (void)endian;
    //ParticipantMessageSecureData* structData = static_cast<ParticipantMessageSecureData*>(data);
    //bool memberHasKey = false;
    return 0;
}

int ParticipantMessageSecureDataTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t ParticipantMessageSecureDataTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
    if (!has_key()) {
        iHandle = HANDLE_NIL;
        return RETCODE_OK;
    }
    int ret = serialize_key(data, cdr, forceMd5);
    if (ret != 0) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to serialize key.\n");
        return RETCODE_ERROR;
    }
    if (!cdr->getKeyHash((char*)&iHandle, forceMd5)) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to get key hash\n");
        return RETCODE_ERROR;
    }
    return RETCODE_OK;
}

bool ParticipantMessageSecureDataTypeSupport::has_key() {
    return false;
}

const char* ParticipantMessageSecureDataTypeSupport::get_typename() {
    return "ParticipantMessageSecureData";
}

TRAVODDS_NAMESPACE_END
