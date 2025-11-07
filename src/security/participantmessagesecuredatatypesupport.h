#ifndef PARTICIPANT_MESSAGE_SECURE_DATA_TYPE_SUPPORT_H
#define PARTICIPANT_MESSAGE_SECURE_DATA_TYPE_SUPPORT_H

#include "dcps/topic/typesupport.h"
#include "security/participantmessagedata.h"

TRAVODDS_NAMESPACE_BEGIN

class ParticipantMessageSecureDataTypeSupport : public TRAVODDS::TypeSupport {
public:
    static ParticipantMessageSecureDataTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, CdrSerializer* cdr, int endian);

private:
    ParticipantMessageSecureDataTypeSupport() = default;
};

TRAVODDS_NAMESPACE_END

#endif // PARTICIPANT_MESSAGE_SECURE_DATA_TYPE_SUPPORT_H
