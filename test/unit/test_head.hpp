#ifndef TEST_HEAD_CPP
#define TEST_HEAD_CPP

#include "gtest/gtest.h"
#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/publish/datawriter.h"
#include "qos/qos.h"
#include "dcps/domain/domainparticipant.h"
#include "dcps/publish/publisher.h"
#include "dcps/subscribe/subscriber.h"
#include "dcps/topic/typesupport.h"
#include <chrono>

using namespace AGIDDS;

class Stype : virtual public TypeSupport
{
public:
    virtual ~Stype() {};
    virtual void* create_data() { return nullptr; };

    virtual void delete_data(void* data) {};

    virtual int32_t copy_data(void* dst, void* src) { return 0; };

    virtual uint32_t get_serialized_data_size(void* data) { return 0; };

    virtual uint32_t get_max_serialized_data_size() { return 0; };

    virtual int32_t serialize_data(const void* data, SerializedBuffer* buffer, int32_t endian) { return 0; };

    virtual int32_t deserialize_data(SerializedBuffer* buffer, void* data, int32_t endian) { return 0; };

    virtual TypeObject* get_typeobject() { return nullptr; };

    virtual int32_t MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5 = false) { return 0; };

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) { return 0; };

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) { return 0; };

    virtual int serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) { return 0; };

    virtual int deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) { return 0; };

    virtual ReturnCode_t get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5 = false) { return 0; };

    virtual bool has_key() { return 0; };

    virtual const char* get_typename() { return "data"; };

};

//inline StatusMask mask = STATUS_MASK_ALL;
inline TypeSupport* type = new Stype;
inline TypeSupport* type_ = new Stype;

inline StringSeq strSeq;


class DDSTest : public testing::Environment {
protected:
    // 在每个测试用例开始前执行
    void SetUp() override {
        // 可以在这里进行一些初始化操作
    }

    // 在每个测试用例结束后执行
    void TearDown() override {
    }
};


#endif // !TEST_HEAD_CPP
