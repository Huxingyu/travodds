#ifndef HelloWorldTypeSupport_H
#define HelloWorldTypeSupport_H

#include "dcps/topic/typesupport.h"
#include "HelloWorld.h"

USING_AGIDDS_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

class HelloWorldTypeSupport : public TypeSupport {
public:
    static HelloWorldTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) override;

    virtual TypeObject* get_typeobject() override;

    virtual ReturnCode_t get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5 = false) { return 0; }

    int serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian);

private:
    HelloWorldTypeSupport() = default;
};

#ifdef __cplusplus
}
#endif
#endif /* HelloWorldTypeSupport_H */
