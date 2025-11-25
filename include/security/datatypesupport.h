//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef TRAVODDS_SECURITY_DATATYPESUPPORT_H
#define TRAVODDS_SECURITY_DATATYPESUPPORT_H
#include "dcps/topic/typesupport.h"
#ifdef DDSSECURITY
TRAVODDS_NAMESPACE_BEGIN

class BuiltinTopicKey_tTypeSupport : public TRAVODDS::TypeSupport {
public:
    static BuiltinTopicKey_tTypeSupport* get_instance();

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
    BuiltinTopicKey_tTypeSupport() = default;
};

class SequenceNumber_tTypeSupport : public TRAVODDS::TypeSupport {
public:
    static SequenceNumber_tTypeSupport* get_instance();

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
    SequenceNumber_tTypeSupport() = default;
};

class MessageIdentityTypeSupport : public TRAVODDS::TypeSupport {
public:
    static MessageIdentityTypeSupport* get_instance();

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
    MessageIdentityTypeSupport() = default;
};

class Property_tTypeSupport : public TRAVODDS::TypeSupport {
public:
    static Property_tTypeSupport* get_instance();

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
    Property_tTypeSupport() = default;
};

class DDS_DLL BinaryProperty_tTypeSupport : public TRAVODDS::TypeSupport {
public:
    static BinaryProperty_tTypeSupport* get_instance();

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
    BinaryProperty_tTypeSupport() = default;
};

TRAVODDS_NAMESPACE_END
#endif // DDSSECURITY
#endif // !TRAVODDS_SECURITY_DATATYPESUPPORT_H
