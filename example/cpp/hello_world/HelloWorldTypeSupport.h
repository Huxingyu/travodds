#ifndef HelloWorldTypeSupport_H
#define HelloWorldTypeSupport_H

#include "dcps/topic/typesupport.h"
#include "HelloWorld.h"

#ifdef __cplusplus
extern "C" {
#endif

class DDS_TYPE_API HelloWorldTypeSupport : public AGIDDS::TypeSupport {
public:
    static HelloWorldTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) override;

    virtual AGIDDS::TypeObject* get_typeobject() override;

    virtual AGIDDS::ReturnCode_t get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, AGIDDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, AGIDDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian);

private:
    HelloWorldTypeSupport() = default;
};

#ifdef __cplusplus
}
#endif
#endif /* HelloWorldTypeSupport_H */
