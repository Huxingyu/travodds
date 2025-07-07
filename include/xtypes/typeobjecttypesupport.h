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

#ifndef TYPE_OBJECT_TYPESUPPORT_H
#define TYPE_OBJECT_TYPESUPPORT_H

#include "dcps/topic/typesupport.h"
#include "cdr/agicdr.h"
#include "xtypes/typeobject.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace AGIDDS {

class DDS_TYPE_API _TypeIdTypeSupport : public AGIDDS::TypeSupport {
public:
    static _TypeIdTypeSupport* get_instance();

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
    _TypeIdTypeSupport() = default;
};

class DDS_TYPE_API AnnotationMemberValueTypeSupport : public AGIDDS::TypeSupport {
public:
    static AnnotationMemberValueTypeSupport* get_instance();

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
    AnnotationMemberValueTypeSupport() = default;
};

class DDS_TYPE_API AnnotationUsageMemberTypeSupport : public AGIDDS::TypeSupport {
public:
    static AnnotationUsageMemberTypeSupport* get_instance();

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
    AnnotationUsageMemberTypeSupport() = default;
};

class DDS_TYPE_API AnnotationUsageTypeSupport : public AGIDDS::TypeSupport {
public:
    static AnnotationUsageTypeSupport* get_instance();

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
    AnnotationUsageTypeSupport() = default;
};

class DDS_TYPE_API TypePropertyTypeSupport : public AGIDDS::TypeSupport {
public:
    static TypePropertyTypeSupport* get_instance();

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
    TypePropertyTypeSupport() = default;
};

class DDS_TYPE_API TypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static TypeTypeSupport* get_instance();

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
    TypeTypeSupport() = default;
};

class DDS_TYPE_API MemberPropertyTypeSupport : public AGIDDS::TypeSupport {
public:
    static MemberPropertyTypeSupport* get_instance();

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
    MemberPropertyTypeSupport() = default;
};

class DDS_TYPE_API MemberTypeSupport : public AGIDDS::TypeSupport {
public:
    static MemberTypeSupport* get_instance();

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
    MemberTypeSupport() = default;
};

class DDS_TYPE_API StructureTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static StructureTypeTypeSupport* get_instance();

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
    StructureTypeTypeSupport() = default;
};

class DDS_TYPE_API UnionMemberTypeSupport : public AGIDDS::TypeSupport {
public:
    static UnionMemberTypeSupport* get_instance();

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
    UnionMemberTypeSupport() = default;
};

class DDS_TYPE_API UnionTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static UnionTypeTypeSupport* get_instance();

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
    UnionTypeTypeSupport() = default;
};

class DDS_TYPE_API AnnotationMemberTypeSupport : public AGIDDS::TypeSupport {
public:
    static AnnotationMemberTypeSupport* get_instance();

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
    AnnotationMemberTypeSupport() = default;
};

class DDS_TYPE_API AnnotationTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static AnnotationTypeTypeSupport* get_instance();

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
    AnnotationTypeTypeSupport() = default;
};

class DDS_TYPE_API AliasTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static AliasTypeTypeSupport* get_instance();

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
    AliasTypeTypeSupport() = default;
};

class DDS_TYPE_API CollectionTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static CollectionTypeTypeSupport* get_instance();

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
    CollectionTypeTypeSupport() = default;
};

class DDS_TYPE_API ArrayTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static ArrayTypeTypeSupport* get_instance();

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
    ArrayTypeTypeSupport() = default;
};

class DDS_TYPE_API MapTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static MapTypeTypeSupport* get_instance();

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
    MapTypeTypeSupport() = default;
};

class DDS_TYPE_API SequenceTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static SequenceTypeTypeSupport* get_instance();

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
    SequenceTypeTypeSupport() = default;
};

class DDS_TYPE_API StringTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static StringTypeTypeSupport* get_instance();

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
    StringTypeTypeSupport() = default;
};

class DDS_TYPE_API BitTypeSupport : public AGIDDS::TypeSupport {
public:
    static BitTypeSupport* get_instance();

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
    BitTypeSupport() = default;
};

class DDS_TYPE_API BitSetTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static BitSetTypeTypeSupport* get_instance();

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
    BitSetTypeTypeSupport() = default;
};

class DDS_TYPE_API EnumeratedConstantTypeSupport : public AGIDDS::TypeSupport {
public:
    static EnumeratedConstantTypeSupport* get_instance();

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
    EnumeratedConstantTypeSupport() = default;
};

class DDS_TYPE_API EnumerationTypeTypeSupport : public AGIDDS::TypeSupport {
public:
    static EnumerationTypeTypeSupport* get_instance();

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
    EnumerationTypeTypeSupport() = default;
};

class DDS_TYPE_API _ModuleTypeSupport : public AGIDDS::TypeSupport {
public:
    static _ModuleTypeSupport* get_instance();

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
    _ModuleTypeSupport() = default;
};

class DDS_TYPE_API TypeLibraryElementTypeSupport : public AGIDDS::TypeSupport {
public:
    static TypeLibraryElementTypeSupport* get_instance();

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
    TypeLibraryElementTypeSupport() = default;
};

class DDS_TYPE_API TypeLibraryTypeSupport : public AGIDDS::TypeSupport {
public:
    static TypeLibraryTypeSupport* get_instance();

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
    TypeLibraryTypeSupport() = default;
};

class DDS_TYPE_API TypeObjectTypeSupport : public AGIDDS::TypeSupport {
public:
    static TypeObjectTypeSupport* get_instance();

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
    TypeObjectTypeSupport() = default;
};

} // namespace AGIDDS

#ifdef __cplusplus
}
#endif
#endif /* TYPE_OBJECT_TYPESUPPORT_H */
