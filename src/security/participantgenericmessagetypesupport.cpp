#include "security/participantgenericmessagetypesupport.h"
#include "security/participantgenericmessage.h"
#include "security/datatypesupport.h"
TRAVODDS_NAMESPACE_BEGIN
#ifdef DDSSECURITY
DataHolderTypeSupport* DataHolderTypeSupport::get_instance() {
    static DataHolderTypeSupport instance;
    return &instance;
}

void* DataHolderTypeSupport::create_data() {
    return new DataHolder();
}

void DataHolderTypeSupport::delete_data(void* data) {
    delete static_cast<DataHolder*>(data);
}

int DataHolderTypeSupport::copy_data(void* dst, void* src) {
    DataHolder* dstData = static_cast<DataHolder*>(dst);
    DataHolder* srcData = static_cast<DataHolder*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int DataHolderTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    DataHolder* structData = static_cast<DataHolder*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    currentAlignment += CdrSerializer::getStringSize(structData->class_id.size() + 1, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->properties.size(); ++i) {
        currentAlignment += Property_tTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->properties[i], currentAlignment);
    }
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->binary_properties.size(); ++i) {
        currentAlignment += BinaryProperty_tTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->binary_properties[i], currentAlignment);
    }
    return currentAlignment - initialAlignment;
}

unsigned int DataHolderTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int DataHolderTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    DataHolder* structData = static_cast<DataHolder*>(data);
    uint32_t tmpLength = 0;
    if (!cdr->serializeString(structData->class_id)) {
        fprintf(stderr, "Serialization failed for field: structData->class_id\n");
        return -1;
    }
    tmpLength = static_cast<uint32_t>(structData->properties.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->properties\n");
        return -1;
    }
    for (size_t i = 0; i < structData->properties.size(); ++i) {
        if (Property_tTypeSupport::get_instance()->serialize_data((void*)&structData->properties[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->properties[i]\n");
            return -1;
        }
    }
    tmpLength = static_cast<uint32_t>(structData->binary_properties.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->binary_properties\n");
        return -1;
    }
    for (size_t i = 0; i < structData->binary_properties.size(); ++i) {
        if (BinaryProperty_tTypeSupport::get_instance()->serialize_data((void*)&structData->binary_properties[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->binary_properties[i]\n");
            return -1;
        }
    }
    return 0;
}

int DataHolderTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    DataHolder* structData = static_cast<DataHolder*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    if (!cdr->deserializeString(structData->class_id)) {
        fprintf(stderr, "Deserialization failed for field: structData->class_id\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->properties\n");
        return -1;
    }
    structData->properties.resize(tmpLength);
    for (size_t i = 0; i < tmpLength; ++i) {
        if (Property_tTypeSupport::get_instance()->deserialize_data(&structData->properties[i], cdr, endian) != 0) {
            fprintf(stderr, "Deserialization failed for field: structData->properties[i]\n");
            return -1;
        }
    }
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->binary_properties\n");
        return -1;
    }
    structData->binary_properties.resize(tmpLength);
    for (size_t i = 0; i < tmpLength; ++i) {
        if (BinaryProperty_tTypeSupport::get_instance()->deserialize_data(&structData->binary_properties[i], cdr, endian) != 0) {
            fprintf(stderr, "Deserialization failed for field: structData->binary_properties[i]\n");
            return -1;
        }
    }
    return 0;
}

TypeObject* DataHolderTypeSupport::get_typeobject() {
    return nullptr;
}

int DataHolderTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    DataHolder* structData = static_cast<DataHolder*>(data);
    bool memberHasKey = false;
    return 0;
}

int DataHolderTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t DataHolderTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool DataHolderTypeSupport::has_key() {
    return false;
}

const char* DataHolderTypeSupport::get_typename() {
    return "DataHolder";
}

ParticipantGenericMessageTypeSupport* ParticipantGenericMessageTypeSupport::get_instance() {
    static ParticipantGenericMessageTypeSupport instance;
    return &instance;
}

void* ParticipantGenericMessageTypeSupport::create_data() {
    return new ParticipantGenericMessage();
}

void ParticipantGenericMessageTypeSupport::delete_data(void* data) {
    delete static_cast<ParticipantGenericMessage*>(data);
}

int ParticipantGenericMessageTypeSupport::copy_data(void* dst, void* src) {
    ParticipantGenericMessage* dstData = static_cast<ParticipantGenericMessage*>(dst);
    ParticipantGenericMessage* srcData = static_cast<ParticipantGenericMessage*>(src);
    *dstData = *srcData;
    return 0;
}

unsigned int ParticipantGenericMessageTypeSupport::get_serialized_data_size(void* data, unsigned int currentAlignment) {
    ParticipantGenericMessage* structData = static_cast<ParticipantGenericMessage*>(data);
    unsigned int initialAlignment = currentAlignment;
    unsigned int tmpAlignment = 0;
    // PND
    currentAlignment += 4; //00010000
    currentAlignment += MessageIdentityTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->messageIdentity, currentAlignment);
    currentAlignment += MessageIdentityTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->relatedMessageIdentity, currentAlignment);
    currentAlignment += BuiltinTopicKey_tTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->destinationParticipantKey, currentAlignment);
    currentAlignment += BuiltinTopicKey_tTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->destinationEndpointKey, currentAlignment);
    currentAlignment += BuiltinTopicKey_tTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->sourceEndpointKey, currentAlignment);
    currentAlignment += CdrSerializer::getStringSize(structData->messageClassId.size() + 1, currentAlignment);
    currentAlignment += CdrSerializer::getBaseTypeSize(sizeof(uint32_t), currentAlignment);
    for (size_t i = 0; i < structData->messageData.size(); ++i) {
        currentAlignment += DataHolderTypeSupport::get_instance()->get_serialized_data_size((void*)&structData->messageData[i], currentAlignment);
    }
    return currentAlignment - initialAlignment;
}

unsigned int ParticipantGenericMessageTypeSupport::get_max_serialized_data_size(void* data, unsigned int currentAlignment) {
    return LENGTH_UNLIMITED;
}

int ParticipantGenericMessageTypeSupport::serialize_data(void* data, CdrSerializer* cdr, int endian) {
    ParticipantGenericMessage* structData = static_cast<ParticipantGenericMessage*>(data);
    uint32_t tmpLength = 0;
    //PND
    uint16_t dig = 0;
    if (!cdr->serializeBaseType((uint8_t)0)) {
        fprintf(stderr, "Serialization failed for field: cdr_le\n");
        return -1;
    }
    if (!cdr->serializeBaseType((uint8_t)1)) {
        fprintf(stderr, "Serialization failed for field: cdr_le\n");
        return -1;
    }
    if (!cdr->serializeBaseType(dig)) {
        fprintf(stderr, "Serialization failed for field: dig\n");
        return -1;
    }
    if (MessageIdentityTypeSupport::get_instance()->serialize_data((void*)&structData->messageIdentity, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->messageIdentity\n");
        return -1;
    }
    if (MessageIdentityTypeSupport::get_instance()->serialize_data((void*)&structData->relatedMessageIdentity, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->relatedMessageIdentity\n");
        return -1;
    }
    if (BuiltinTopicKey_tTypeSupport::get_instance()->serialize_data((void*)&structData->destinationParticipantKey, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->destinationParticipantKey\n");
        return -1;
    }
    if (BuiltinTopicKey_tTypeSupport::get_instance()->serialize_data((void*)&structData->destinationEndpointKey, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->destinationEndpointKey\n");
        return -1;
    }
    if (BuiltinTopicKey_tTypeSupport::get_instance()->serialize_data((void*)&structData->sourceEndpointKey, cdr, endian) != 0) {
        fprintf(stderr, "Serialization failed for field: structData->sourceEndpointKey\n");
        return -1;
    }
    if (!cdr->serializeString(structData->messageClassId)) {
        fprintf(stderr, "Serialization failed for field: structData->messageClassId\n");
        return -1;
    }
    tmpLength = static_cast<uint32_t>(structData->messageData.size());
    if (!cdr->serializeBaseType(tmpLength)) {
        fprintf(stderr, "Serialization length failed for field: structData->messageData\n");
        return -1;
    }
    for (size_t i = 0; i < structData->messageData.size(); ++i) {
        if (DataHolderTypeSupport::get_instance()->serialize_data((void*)&structData->messageData[i], cdr, endian) != 0) {
            fprintf(stderr, "Serialization failed for field: structData->messageData[i]\n");
            return -1;
        }
    }
    return 0;
}

int ParticipantGenericMessageTypeSupport::deserialize_data(void* data, CdrDeserializer* cdr, int endian) {
    ParticipantGenericMessage* structData = static_cast<ParticipantGenericMessage*>(data);
    unsigned int tmpLength = 0;
    char tmpCharEnum = 0;
    short tmpShortEnum = 0;
    int tmpIntEnum = 0;
    //PND
    uint8_t cdr_le;
    uint16_t dig = 0;
    if (!cdr->deserializeBaseType(cdr_le)) {
        fprintf(stderr, "Deserialization length failed for field: cdr_le\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(cdr_le)) {
        fprintf(stderr, "Deserialization length failed for field: cdr_le\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(dig)) {
        fprintf(stderr, "Deserialization length failed for field: dig\n");
        return -1;
    }

    if (MessageIdentityTypeSupport::get_instance()->deserialize_data(&structData->messageIdentity, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->messageIdentity\n");
        return -1;
    }
    if (MessageIdentityTypeSupport::get_instance()->deserialize_data(&structData->relatedMessageIdentity, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->relatedMessageIdentity\n");
        return -1;
    }
    if (BuiltinTopicKey_tTypeSupport::get_instance()->deserialize_data(&structData->destinationParticipantKey, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->destinationParticipantKey\n");
        return -1;
    }
    if (BuiltinTopicKey_tTypeSupport::get_instance()->deserialize_data(&structData->destinationEndpointKey, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->destinationEndpointKey\n");
        return -1;
    }
    if (BuiltinTopicKey_tTypeSupport::get_instance()->deserialize_data(&structData->sourceEndpointKey, cdr, endian) != 0) {
        fprintf(stderr, "Deserialization failed for field: structData->sourceEndpointKey\n");
        return -1;
    }
    if (!cdr->deserializeString(structData->messageClassId)) {
        fprintf(stderr, "Deserialization failed for field: structData->messageClassId\n");
        return -1;
    }
    if (!cdr->deserializeBaseType(tmpLength)) {
        fprintf(stderr, "Deserialization length failed for field: structData->messageData\n");
        return -1;
    }
    structData->messageData.resize(tmpLength);
    for (size_t i = 0; i < tmpLength; ++i) {
        if (DataHolderTypeSupport::get_instance()->deserialize_data(&structData->messageData[i], cdr, endian) != 0) {
            fprintf(stderr, "Deserialization failed for field: structData->messageData[i]\n");
            return -1;
        }
    }
    return 0;
}

TypeObject* ParticipantGenericMessageTypeSupport::get_typeobject() {
    return nullptr;
}

int ParticipantGenericMessageTypeSupport::serialize_key(void* data, CdrSerializer* cdr, int endian) {
    ParticipantGenericMessage* structData = static_cast<ParticipantGenericMessage*>(data);
    bool memberHasKey = false;
    return 0;
}

int ParticipantGenericMessageTypeSupport::MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5) {
    unsigned int serializedSize = get_serialized_data_size((void*)data, 0);
    SerializedBuffer buffer;
    buffer.buffer_size = serializedSize;
    buffer.buffer = new char[buffer.buffer_size];
    CdrSerializer cdr(&buffer);
    ReturnCode_t ret = get_instancehandle((void*)data, &cdr, iHandle, forceMd5);
    delete[] buffer.buffer;
    return ret == RETCODE_OK ? 0 : -1;
}

ReturnCode_t ParticipantGenericMessageTypeSupport::get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5) {
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

bool ParticipantGenericMessageTypeSupport::has_key() {
    return false;
}

const char* ParticipantGenericMessageTypeSupport::get_typename() {
    return "ParticipantGenericMessage";
}

#endif
TRAVODDS_NAMESPACE_END
