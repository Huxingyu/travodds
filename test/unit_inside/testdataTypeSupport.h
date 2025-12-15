#ifndef testdataTypeSupport_H
#define testdataTypeSupport_H

#include "dcps/topic/typesupport.h"
#include "testdata.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace TestModule {

class DDS_TYPE_API BasicDataTypeSupport : public TRAVODDS::TypeSupport {
public:
    static BasicDataTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual TRAVODDS::DynamicType* get_dynamictype() override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    BasicDataTypeSupport() = default;
};

class DDS_TYPE_API KeyDataTypeSupport : public TRAVODDS::TypeSupport {
public:
    static KeyDataTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual TRAVODDS::DynamicType* get_dynamictype() override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    KeyDataTypeSupport() = default;
};

class DDS_TYPE_API SequenceDataTypeSupport : public TRAVODDS::TypeSupport {
public:
    static SequenceDataTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual TRAVODDS::DynamicType* get_dynamictype() override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    SequenceDataTypeSupport() = default;
};

} // namespace TestModule

#ifdef __cplusplus
}
#endif
#endif /* testdataTypeSupport_H */
