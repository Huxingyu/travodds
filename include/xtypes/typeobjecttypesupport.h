#ifndef typeobjectTypeSupport_H
#define typeobjectTypeSupport_H

#include "dcps/topic/typesupport.h"
#include "xtypes/typeobject.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace TRAVODDS {

class DDS_TYPE_API _TypeIdTypeSupport : public TRAVODDS::TypeSupport {
public:
    static _TypeIdTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    _TypeIdTypeSupport() = default;
};

class DDS_TYPE_API AnnotationMemberValueTypeSupport : public TRAVODDS::TypeSupport {
public:
    static AnnotationMemberValueTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    AnnotationMemberValueTypeSupport() = default;
};

class DDS_TYPE_API AnnotationUsageMemberTypeSupport : public TRAVODDS::TypeSupport {
public:
    static AnnotationUsageMemberTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    AnnotationUsageMemberTypeSupport() = default;
};

class DDS_TYPE_API AnnotationUsageTypeSupport : public TRAVODDS::TypeSupport {
public:
    static AnnotationUsageTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    AnnotationUsageTypeSupport() = default;
};

class DDS_TYPE_API TypePropertyTypeSupport : public TRAVODDS::TypeSupport {
public:
    static TypePropertyTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    TypePropertyTypeSupport() = default;
};

class DDS_TYPE_API TypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static TypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    TypeTypeSupport() = default;
};

class DDS_TYPE_API MemberPropertyTypeSupport : public TRAVODDS::TypeSupport {
public:
    static MemberPropertyTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    MemberPropertyTypeSupport() = default;
};

class DDS_TYPE_API MemberTypeSupport : public TRAVODDS::TypeSupport {
public:
    static MemberTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    MemberTypeSupport() = default;
};

class DDS_TYPE_API StructureTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static StructureTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    StructureTypeTypeSupport() = default;
};

class DDS_TYPE_API UnionMemberTypeSupport : public TRAVODDS::TypeSupport {
public:
    static UnionMemberTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    UnionMemberTypeSupport() = default;
};

class DDS_TYPE_API UnionTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static UnionTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    UnionTypeTypeSupport() = default;
};

class DDS_TYPE_API AnnotationMemberTypeSupport : public TRAVODDS::TypeSupport {
public:
    static AnnotationMemberTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    AnnotationMemberTypeSupport() = default;
};

class DDS_TYPE_API AnnotationTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static AnnotationTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    AnnotationTypeTypeSupport() = default;
};

class DDS_TYPE_API AliasTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static AliasTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    AliasTypeTypeSupport() = default;
};

class DDS_TYPE_API CollectionTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static CollectionTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    CollectionTypeTypeSupport() = default;
};

class DDS_TYPE_API ArrayTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static ArrayTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    ArrayTypeTypeSupport() = default;
};

class DDS_TYPE_API MapTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static MapTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    MapTypeTypeSupport() = default;
};

class DDS_TYPE_API SequenceTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static SequenceTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    SequenceTypeTypeSupport() = default;
};

class DDS_TYPE_API StringTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static StringTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    StringTypeTypeSupport() = default;
};

class DDS_TYPE_API BitTypeSupport : public TRAVODDS::TypeSupport {
public:
    static BitTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    BitTypeSupport() = default;
};

class DDS_TYPE_API BitSetTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static BitSetTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    BitSetTypeTypeSupport() = default;
};

class DDS_TYPE_API EnumeratedConstantTypeSupport : public TRAVODDS::TypeSupport {
public:
    static EnumeratedConstantTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    EnumeratedConstantTypeSupport() = default;
};

class DDS_TYPE_API EnumerationTypeTypeSupport : public TRAVODDS::TypeSupport {
public:
    static EnumerationTypeTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    EnumerationTypeTypeSupport() = default;
};

class DDS_TYPE_API _ModuleTypeSupport : public TRAVODDS::TypeSupport {
public:
    static _ModuleTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    _ModuleTypeSupport() = default;
};

class DDS_TYPE_API TypeLibraryElementTypeSupport : public TRAVODDS::TypeSupport {
public:
    static TypeLibraryElementTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    TypeLibraryElementTypeSupport() = default;
};

class DDS_TYPE_API TypeLibraryTypeSupport : public TRAVODDS::TypeSupport {
public:
    static TypeLibraryTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    TypeLibraryTypeSupport() = default;
};

class DDS_TYPE_API TypeObjectTypeSupport : public TRAVODDS::TypeSupport {
public:
    static TypeObjectTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, TRAVODDS::CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, TRAVODDS::CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, TRAVODDS::CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, TRAVODDS::CdrSerializer* cdr, int endian);

private:
    TypeObjectTypeSupport() = default;
};

} // namespace TRAVODDS

#ifdef __cplusplus
}
#endif
#endif /* typeobjectTypeSupport_H */
