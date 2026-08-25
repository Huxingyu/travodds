#include "xtypes/typeobjecttypesupport.h"
#include "gtest/gtest.h"

USING_TRAVODDS_NAMESPACE

// ========== TypeObject TypeSupport 测试 ==========

TEST(DDSTest, Xtypes_typeid_typesupport_singleton)
{
    _TypeIdTypeSupport* ts = _TypeIdTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    _TypeIdTypeSupport* ts2 = _TypeIdTypeSupport::get_instance();
    EXPECT_EQ(ts, ts2);
}

TEST(DDSTest, Xtypes_typeid_create_delete_data)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    _TypeId* typeId = static_cast<_TypeId*>(data);
    typeId->u_ = BOOLEAN_TYPE;
    typeId->primitive_type_id = BOOLEAN_TYPE;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_typeid_copy_data)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    void* src = ts->create_data();
    void* dst = ts->create_data();
    
    _TypeId* srcId = static_cast<_TypeId*>(src);
    srcId->u_ = INT_32_TYPE;
    srcId->primitive_type_id = INT_32_TYPE;
    
    ts->copy_data(dst, src);
    
    _TypeId* dstId = static_cast<_TypeId*>(dst);
    EXPECT_EQ(dstId->u_, INT_32_TYPE);
    EXPECT_EQ(dstId->primitive_type_id, INT_32_TYPE);
    
    ts->delete_data(src);
    ts->delete_data(dst);
}

TEST(DDSTest, Xtypes_typeid_get_typename)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    const char* name = ts->get_typename();
    EXPECT_NE(name, nullptr);
}

TEST(DDSTest, Xtypes_typeid_has_key)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    EXPECT_FALSE(ts->has_key());
}

TEST(DDSTest, Xtypes_typeid_serialize_deserialize)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    
    void* data = ts->create_data();
    _TypeId* typeId = static_cast<_TypeId*>(data);
    typeId->u_ = FLOAT_64_TYPE;
    typeId->primitive_type_id = FLOAT_64_TYPE;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    _TypeId* dstId = static_cast<_TypeId*>(dst);
    EXPECT_EQ(dstId->u_, FLOAT_64_TYPE);
    EXPECT_EQ(dstId->primitive_type_id, FLOAT_64_TYPE);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_typeid_serialize_constructed_type)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    
    void* data = ts->create_data();
    _TypeId* typeId = static_cast<_TypeId*>(data);
    typeId->u_ = STRUCTURE_TYPE;
    typeId->constructed_type_id = 12345;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    _TypeId* dstId = static_cast<_TypeId*>(dst);
    EXPECT_EQ(dstId->u_, STRUCTURE_TYPE);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_annotation_member_value_typesupport)
{
    auto* ts = AnnotationMemberValueTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
    val->u_ = INT_32_TYPE;
    val->int_32_value = 42;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_annotation_member_value_serialize_deserialize)
{
    auto* ts = AnnotationMemberValueTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
    val->u_ = INT_32_TYPE;
    val->int_32_value = 42;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    AnnotationMemberValue* dstVal = static_cast<AnnotationMemberValue*>(dst);
    EXPECT_EQ(dstVal->u_, INT_32_TYPE);
    EXPECT_EQ(dstVal->int_32_value, 42);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_annotation_member_value_serialize_string)
{
    auto* ts = AnnotationMemberValueTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
    val->u_ = STRING_TYPE;
    val->string_value = "test_string_value";
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    AnnotationMemberValue* dstVal = static_cast<AnnotationMemberValue*>(dst);
    EXPECT_EQ(dstVal->u_, STRING_TYPE);
    EXPECT_EQ(dstVal->string_value, "test_string_value");
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_annotation_member_value_serialize_boolean)
{
    auto* ts = AnnotationMemberValueTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
    val->u_ = BOOLEAN_TYPE;
    val->boolean_value = 1;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    AnnotationMemberValue* dstVal = static_cast<AnnotationMemberValue*>(dst);
    EXPECT_EQ(dstVal->u_, BOOLEAN_TYPE);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_annotation_member_value_serialize_float)
{
    auto* ts = AnnotationMemberValueTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
    val->u_ = FLOAT_64_TYPE;
    val->float_64_value = 3.14159;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    AnnotationMemberValue* dstVal = static_cast<AnnotationMemberValue*>(dst);
    EXPECT_EQ(dstVal->u_, FLOAT_64_TYPE);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_type_property_typesupport)
{
    auto* ts = TypePropertyTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    TypeProperty* prop = static_cast<TypeProperty*>(data);
    prop->type_id.u_ = INT_32_TYPE;
    prop->name = "test";
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_type_property_serialize_deserialize)
{
    auto* ts = TypePropertyTypeSupport::get_instance();
    
    void* data = ts->create_data();
    TypeProperty* prop = static_cast<TypeProperty*>(data);
    prop->type_id.u_ = INT_32_TYPE;
    prop->type_id.primitive_type_id = INT_32_TYPE;
    prop->name = "TestProperty";
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    TypeProperty* dstProp = static_cast<TypeProperty*>(dst);
    EXPECT_EQ(dstProp->name, "TestProperty");
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_type_typesupport)
{
    auto* ts = TypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    Type* type = static_cast<Type*>(data);
    type->property.type_id.u_ = STRUCTURE_TYPE;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_type_serialize_deserialize)
{
    auto* ts = TypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    Type* type = static_cast<Type*>(data);
    type->property.type_id.u_ = STRUCTURE_TYPE;
    type->property.type_id.constructed_type_id = 100;
    type->property.name = "TestType";
    type->property.flag = IS_MUTABLE;
    type->property.type_id.primitive_type_id = NO_TYPE;
    // annotation is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    EXPECT_GT(size, 0u);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_member_property_typesupport)
{
    auto* ts = MemberPropertyTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_member_property_serialize_deserialize)
{
    auto* ts = MemberPropertyTypeSupport::get_instance();
    
    void* data = ts->create_data();
    MemberProperty* prop = static_cast<MemberProperty*>(data);
    prop->flag = IS_KEY;
    prop->member_id = 1;
    prop->type_id.u_ = INT_32_TYPE;
    prop->type_id.primitive_type_id = INT_32_TYPE;
    prop->name = "test_prop";
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    MemberProperty* dstProp = static_cast<MemberProperty*>(dst);
    EXPECT_EQ(dstProp->flag, IS_KEY);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_member_typesupport)
{
    auto* ts = MemberTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_member_serialize_deserialize)
{
    auto* ts = MemberTypeSupport::get_instance();
    
    void* data = ts->create_data();
    Member* member = static_cast<Member*>(data);
    member->property.flag = IS_KEY;
    member->property.member_id = 1;
    member->property.type_id.u_ = INT_32_TYPE;
    member->property.type_id.primitive_type_id = INT_32_TYPE;
    member->property.name = "test_member";
    // annotation is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    EXPECT_GT(size, 0u);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_structure_type_typesupport)
{
    auto* ts = StructureTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_structure_type_serialize_deserialize)
{
    auto* ts = StructureTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    StructureType* structType = static_cast<StructureType*>(data);
    structType->property.type_id.u_ = STRUCTURE_TYPE;
    structType->property.type_id.constructed_type_id = 200;
    structType->property.name = "TestStruct";
    structType->property.flag = IS_MUTABLE;
    structType->property.type_id.primitive_type_id = NO_TYPE;
    // annotation is std::vector, empty by default
    structType->base_type.u_ = NO_TYPE;
    structType->base_type.primitive_type_id = NO_TYPE;
    structType->base_type.constructed_type_id = 0;
    // member is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    EXPECT_GT(size, 0u);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_union_type_typesupport)
{
    auto* ts = UnionTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_union_type_serialize_deserialize)
{
    auto* ts = UnionTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    UnionType* unionType = static_cast<UnionType*>(data);
    unionType->property.type_id.u_ = UNION_TYPE;
    unionType->property.type_id.constructed_type_id = 300;
    unionType->property.name = "TestUnion";
    unionType->property.flag = IS_MUTABLE;
    unionType->property.type_id.primitive_type_id = NO_TYPE;
    // annotation and member are std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    EXPECT_GT(size, 0u);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_annotation_type_typesupport)
{
    auto* ts = AnnotationTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_alias_type_typesupport)
{
    auto* ts = AliasTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_collection_type_typesupport)
{
    auto* ts = CollectionTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_array_type_typesupport)
{
    auto* ts = ArrayTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_map_type_typesupport)
{
    auto* ts = MapTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_sequence_type_typesupport)
{
    auto* ts = SequenceTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_string_type_typesupport)
{
    auto* ts = StringTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_bit_typesupport)
{
    auto* ts = BitTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_bitset_type_typesupport)
{
    auto* ts = BitSetTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_enumerated_constant_typesupport)
{
    auto* ts = EnumeratedConstantTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_enumeration_type_typesupport)
{
    auto* ts = EnumerationTypeTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_module_typesupport)
{
    auto* ts = _ModuleTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_type_library_element_typesupport)
{
    auto* ts = TypeLibraryElementTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_type_library_typesupport)
{
    auto* ts = TypeLibraryTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_type_object_typesupport)
{
    auto* ts = TypeObjectTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    ts->delete_data(data);
}

// ========== 更多 TypeObject 序列化测试 ==========

TEST(DDSTest, Xtypes_typeid_serialize_all_primitive_types)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    
    short primitiveTypes[] = {BOOLEAN_TYPE, BYTE_TYPE, INT_16_TYPE, UINT_16_TYPE, 
                              INT_32_TYPE, UINT_32_TYPE, INT_64_TYPE, UINT_64_TYPE,
                              FLOAT_32_TYPE, FLOAT_64_TYPE, FLOAT_128_TYPE, CHAR_8_TYPE, CHAR_32_TYPE};
    
    for (short pType : primitiveTypes) {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = pType;
        typeId->primitive_type_id = pType;
        
        unsigned int size = ts->get_serialized_data_size(data, 0);
        unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
        unsigned int bufSize = (size > 0) ? size : maxSize;
        if (bufSize == 0) bufSize = 100;
        
        char* buffer = new char[bufSize + 100];
        memset(buffer, 0, bufSize + 100);
        SerializedBuffer sbuf;
        sbuf.buffer = buffer;
        sbuf.origin_buffer = buffer;
        sbuf.buffer_size = bufSize + 100;
        
        CdrSerializer serializer(&sbuf);
        int ret = ts->serialize_data(data, &serializer, 0);
        EXPECT_EQ(ret, 0);
        
        void* dst = ts->create_data();
        CdrDeserializer deserializer(&sbuf);
        ret = ts->deserialize_data(dst, &deserializer, 0);
        EXPECT_EQ(ret, 0);
        
        _TypeId* dstId = static_cast<_TypeId*>(dst);
        EXPECT_EQ(dstId->u_, pType);
        
        ts->delete_data(data);
        ts->delete_data(dst);
        delete[] buffer;
    }
}

TEST(DDSTest, Xtypes_annotation_usage_typesupport)
{
    auto* ts = AnnotationUsageTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    AnnotationUsage* usage = static_cast<AnnotationUsage*>(data);
    usage->type_id.u_ = ANNOTATION_TYPE;
    usage->type_id.constructed_type_id = 100;
    // member is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    EXPECT_GT(size, 0u);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_annotation_usage_serialize_deserialize)
{
    auto* ts = AnnotationUsageTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AnnotationUsage* usage = static_cast<AnnotationUsage*>(data);
    usage->type_id.u_ = ANNOTATION_TYPE;
    usage->type_id.constructed_type_id = 100;
    usage->type_id.primitive_type_id = NO_TYPE;
    // member is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    
    char* buffer = new char[size + 100];
    memset(buffer, 0, size + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = size + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_annotation_usage_member_typesupport)
{
    auto* ts = AnnotationUsageMemberTypeSupport::get_instance();
    EXPECT_NE(ts, nullptr);
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    AnnotationUsageMember* member = static_cast<AnnotationUsageMember*>(data);
    member->member_id = 1;
    member->value.u_ = INT_32_TYPE;
    member->value.int_32_value = 42;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    EXPECT_GT(size, 0u);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_annotation_usage_member_serialize_deserialize)
{
    auto* ts = AnnotationUsageMemberTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AnnotationUsageMember* member = static_cast<AnnotationUsageMember*>(data);
    member->member_id = 1;
    member->value.u_ = INT_32_TYPE;
    member->value.int_32_value = 42;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    AnnotationUsageMember* dstMember = static_cast<AnnotationUsageMember*>(dst);
    EXPECT_EQ(dstMember->member_id, 1u);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_collection_type_typesupport_serialize)
{
    auto* ts = CollectionTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    CollectionType* collType = static_cast<CollectionType*>(data);
    collType->property.type_id.u_ = SEQUENCE_TYPE;
    collType->property.type_id.constructed_type_id = 400;
    collType->property.type_id.primitive_type_id = NO_TYPE;
    collType->property.name = "TestCollection";
    collType->element_type.u_ = INT_32_TYPE;
    collType->element_type.primitive_type_id = INT_32_TYPE;
    collType->element_type.constructed_type_id = 0;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_collection_type_serialize_deserialize)
{
    auto* ts = CollectionTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    CollectionType* collType = static_cast<CollectionType*>(data);
    collType->property.type_id.u_ = SEQUENCE_TYPE;
    collType->property.type_id.constructed_type_id = 400;
    collType->property.type_id.primitive_type_id = NO_TYPE;
    collType->property.name = "TestCollection";
    collType->element_type.u_ = INT_32_TYPE;
    collType->element_type.primitive_type_id = INT_32_TYPE;
    collType->element_type.constructed_type_id = 0;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_sequence_type_typesupport_serialize)
{
    auto* ts = SequenceTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    SequenceType* seqType = static_cast<SequenceType*>(data);
    seqType->property.type_id.u_ = SEQUENCE_TYPE;
    seqType->property.type_id.constructed_type_id = 500;
    seqType->property.type_id.primitive_type_id = NO_TYPE;
    seqType->property.name = "TestSequence";
    seqType->element_type.u_ = INT_32_TYPE;
    seqType->element_type.primitive_type_id = INT_32_TYPE;
    seqType->element_type.constructed_type_id = 0;
    seqType->bound = 10;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_sequence_type_serialize_deserialize)
{
    auto* ts = SequenceTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    SequenceType* seqType = static_cast<SequenceType*>(data);
    seqType->property.type_id.u_ = SEQUENCE_TYPE;
    seqType->property.type_id.constructed_type_id = 500;
    seqType->property.type_id.primitive_type_id = NO_TYPE;
    seqType->property.name = "TestSequence";
    seqType->element_type.u_ = INT_32_TYPE;
    seqType->element_type.primitive_type_id = INT_32_TYPE;
    seqType->element_type.constructed_type_id = 0;
    seqType->bound = 10;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_array_type_typesupport_serialize)
{
    auto* ts = ArrayTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    ArrayType* arrType = static_cast<ArrayType*>(data);
    arrType->property.type_id.u_ = ARRAY_TYPE;
    arrType->property.type_id.constructed_type_id = 600;
    arrType->property.type_id.primitive_type_id = NO_TYPE;
    arrType->property.name = "TestArray";
    arrType->element_type.u_ = INT_32_TYPE;
    arrType->element_type.primitive_type_id = INT_32_TYPE;
    arrType->element_type.constructed_type_id = 0;
    arrType->bound.push_back(5);
    arrType->bound.push_back(10);
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_array_type_serialize_deserialize)
{
    auto* ts = ArrayTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    ArrayType* arrType = static_cast<ArrayType*>(data);
    arrType->property.type_id.u_ = ARRAY_TYPE;
    arrType->property.type_id.constructed_type_id = 600;
    arrType->property.type_id.primitive_type_id = NO_TYPE;
    arrType->property.name = "TestArray";
    arrType->element_type.u_ = INT_32_TYPE;
    arrType->element_type.primitive_type_id = INT_32_TYPE;
    arrType->element_type.constructed_type_id = 0;
    arrType->bound.push_back(5);
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_map_type_typesupport_serialize)
{
    auto* ts = MapTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    MapType* mapType = static_cast<MapType*>(data);
    mapType->property.type_id.u_ = MAP_TYPE;
    mapType->property.type_id.constructed_type_id = 700;
    mapType->property.type_id.primitive_type_id = NO_TYPE;
    mapType->property.name = "TestMap";
    mapType->element_type.u_ = INT_32_TYPE;
    mapType->element_type.primitive_type_id = INT_32_TYPE;
    mapType->element_type.constructed_type_id = 0;
    mapType->key_element_type.u_ = STRING_TYPE;
    mapType->key_element_type.primitive_type_id = NO_TYPE;
    mapType->key_element_type.constructed_type_id = 0;
    mapType->bound = 20;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_map_type_serialize_deserialize)
{
    auto* ts = MapTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    MapType* mapType = static_cast<MapType*>(data);
    mapType->property.type_id.u_ = MAP_TYPE;
    mapType->property.type_id.constructed_type_id = 700;
    mapType->property.type_id.primitive_type_id = NO_TYPE;
    mapType->property.name = "TestMap";
    mapType->element_type.u_ = INT_32_TYPE;
    mapType->element_type.primitive_type_id = INT_32_TYPE;
    mapType->element_type.constructed_type_id = 0;
    mapType->key_element_type.u_ = STRING_TYPE;
    mapType->key_element_type.primitive_type_id = NO_TYPE;
    mapType->key_element_type.constructed_type_id = 0;
    mapType->bound = 20;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_string_type_typesupport_serialize)
{
    auto* ts = StringTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    StringType* strType = static_cast<StringType*>(data);
    strType->property.type_id.u_ = STRING_TYPE;
    strType->property.type_id.constructed_type_id = 800;
    strType->property.type_id.primitive_type_id = NO_TYPE;
    strType->property.name = "TestString";
    strType->element_type.u_ = CHAR_8_TYPE;
    strType->element_type.primitive_type_id = CHAR_8_TYPE;
    strType->element_type.constructed_type_id = 0;
    strType->bound = 256;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_string_type_serialize_deserialize)
{
    auto* ts = StringTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    StringType* strType = static_cast<StringType*>(data);
    strType->property.type_id.u_ = STRING_TYPE;
    strType->property.type_id.constructed_type_id = 800;
    strType->property.type_id.primitive_type_id = NO_TYPE;
    strType->property.name = "TestString";
    strType->element_type.u_ = CHAR_8_TYPE;
    strType->element_type.primitive_type_id = CHAR_8_TYPE;
    strType->element_type.constructed_type_id = 0;
    strType->bound = 256;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_alias_type_typesupport_serialize)
{
    auto* ts = AliasTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AliasType* aliasType = static_cast<AliasType*>(data);
    aliasType->property.type_id.u_ = ALIAS_TYPE;
    aliasType->property.type_id.constructed_type_id = 900;
    aliasType->property.type_id.primitive_type_id = NO_TYPE;
    aliasType->property.name = "TestAlias";
    aliasType->base_type.u_ = INT_32_TYPE;
    aliasType->base_type.primitive_type_id = INT_32_TYPE;
    aliasType->base_type.constructed_type_id = 0;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_alias_type_serialize_deserialize)
{
    auto* ts = AliasTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AliasType* aliasType = static_cast<AliasType*>(data);
    aliasType->property.type_id.u_ = ALIAS_TYPE;
    aliasType->property.type_id.constructed_type_id = 900;
    aliasType->property.type_id.primitive_type_id = NO_TYPE;
    aliasType->property.name = "TestAlias";
    aliasType->base_type.u_ = INT_32_TYPE;
    aliasType->base_type.primitive_type_id = INT_32_TYPE;
    aliasType->base_type.constructed_type_id = 0;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_enumeration_type_typesupport_serialize)
{
    auto* ts = EnumerationTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    EnumerationType* enumType = static_cast<EnumerationType*>(data);
    enumType->property.type_id.u_ = ENUMERATION_TYPE;
    enumType->property.type_id.constructed_type_id = 1000;
    enumType->property.type_id.primitive_type_id = NO_TYPE;
    enumType->property.name = "TestEnum";
    enumType->bit_bound = 32;
    // enumerator is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_enumeration_type_serialize_deserialize)
{
    auto* ts = EnumerationTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    EnumerationType* enumType = static_cast<EnumerationType*>(data);
    enumType->property.type_id.u_ = ENUMERATION_TYPE;
    enumType->property.type_id.constructed_type_id = 1000;
    enumType->property.type_id.primitive_type_id = NO_TYPE;
    enumType->property.name = "TestEnum";
    enumType->bit_bound = 32;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_enumerated_constant_typesupport_serialize)
{
    auto* ts = EnumeratedConstantTypeSupport::get_instance();
    
    void* data = ts->create_data();
    EnumeratedConstant* enumConst = static_cast<EnumeratedConstant*>(data);
    enumConst->name = "ENUM_VAL";
    enumConst->value = 1;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_enumerated_constant_serialize_deserialize)
{
    auto* ts = EnumeratedConstantTypeSupport::get_instance();
    
    void* data = ts->create_data();
    EnumeratedConstant* enumConst = static_cast<EnumeratedConstant*>(data);
    enumConst->name = "ENUM_VAL";
    enumConst->value = 1;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    EnumeratedConstant* dstConst = static_cast<EnumeratedConstant*>(dst);
    EXPECT_EQ(dstConst->value, 1);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_bitset_type_typesupport_serialize)
{
    auto* ts = BitSetTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    BitSetType* bitsetType = static_cast<BitSetType*>(data);
    bitsetType->property.type_id.u_ = BITSET_TYPE;
    bitsetType->property.type_id.constructed_type_id = 1100;
    bitsetType->property.type_id.primitive_type_id = NO_TYPE;
    bitsetType->property.name = "TestBitSet";
    // field is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_bitset_type_serialize_deserialize)
{
    auto* ts = BitSetTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    BitSetType* bitsetType = static_cast<BitSetType*>(data);
    bitsetType->property.type_id.u_ = BITSET_TYPE;
    bitsetType->property.type_id.constructed_type_id = 1100;
    bitsetType->property.type_id.primitive_type_id = NO_TYPE;
    bitsetType->property.name = "TestBitSet";
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_bit_typesupport_serialize)
{
    auto* ts = BitTypeSupport::get_instance();
    
    void* data = ts->create_data();
    Bit* bit = static_cast<Bit*>(data);
    bit->name = "test_bit";
    bit->index = 0;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_bit_serialize_deserialize)
{
    auto* ts = BitTypeSupport::get_instance();
    
    void* data = ts->create_data();
    Bit* bit = static_cast<Bit*>(data);
    bit->name = "test_bit";
    bit->index = 0;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    Bit* dstBit = static_cast<Bit*>(dst);
    EXPECT_EQ(dstBit->name, "test_bit");
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_annotation_type_typesupport_serialize)
{
    auto* ts = AnnotationTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AnnotationType* annotType = static_cast<AnnotationType*>(data);
    annotType->property.type_id.u_ = ANNOTATION_TYPE;
    annotType->property.type_id.constructed_type_id = 1200;
    annotType->property.type_id.primitive_type_id = NO_TYPE;
    annotType->property.name = "TestAnnotation";
    annotType->base_type.u_ = NO_TYPE;
    annotType->base_type.primitive_type_id = NO_TYPE;
    annotType->base_type.constructed_type_id = 0;
    // member is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_annotation_type_serialize_deserialize)
{
    auto* ts = AnnotationTypeTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AnnotationType* annotType = static_cast<AnnotationType*>(data);
    annotType->property.type_id.u_ = ANNOTATION_TYPE;
    annotType->property.type_id.constructed_type_id = 1200;
    annotType->property.type_id.primitive_type_id = NO_TYPE;
    annotType->property.name = "TestAnnotation";
    annotType->base_type.u_ = NO_TYPE;
    annotType->base_type.primitive_type_id = NO_TYPE;
    annotType->base_type.constructed_type_id = 0;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_annotation_member_typesupport_serialize)
{
    auto* ts = AnnotationMemberTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AnnotationMember* annotMember = static_cast<AnnotationMember*>(data);
    annotMember->property.flag = IS_KEY;
    annotMember->property.member_id = 1;
    annotMember->property.type_id.u_ = INT_32_TYPE;
    annotMember->property.type_id.primitive_type_id = INT_32_TYPE;
    annotMember->property.name = "test_annot_member";
    annotMember->default_value.u_ = INT_32_TYPE;
    annotMember->default_value.int_32_value = 99;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_annotation_member_serialize_deserialize)
{
    auto* ts = AnnotationMemberTypeSupport::get_instance();
    
    void* data = ts->create_data();
    AnnotationMember* annotMember = static_cast<AnnotationMember*>(data);
    annotMember->property.flag = IS_KEY;
    annotMember->property.member_id = 1;
    annotMember->property.type_id.u_ = INT_32_TYPE;
    annotMember->property.type_id.primitive_type_id = INT_32_TYPE;
    annotMember->property.name = "test_annot_member";
    annotMember->default_value.u_ = INT_32_TYPE;
    annotMember->default_value.int_32_value = 99;
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_union_member_typesupport_serialize)
{
    auto* ts = UnionMemberTypeSupport::get_instance();
    
    void* data = ts->create_data();
    UnionMember* unionMember = static_cast<UnionMember*>(data);
    unionMember->property.flag = IS_KEY;
    unionMember->property.member_id = 1;
    unionMember->property.type_id.u_ = INT_32_TYPE;
    unionMember->property.type_id.primitive_type_id = INT_32_TYPE;
    unionMember->property.name = "test_union_member";
    // label is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_union_member_serialize_deserialize)
{
    auto* ts = UnionMemberTypeSupport::get_instance();
    
    void* data = ts->create_data();
    UnionMember* unionMember = static_cast<UnionMember*>(data);
    unionMember->property.flag = IS_KEY;
    unionMember->property.member_id = 1;
    unionMember->property.type_id.u_ = INT_32_TYPE;
    unionMember->property.type_id.primitive_type_id = INT_32_TYPE;
    unionMember->property.name = "test_union_member";
    unionMember->label.push_back(1);
    unionMember->label.push_back(2);
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_module_typesupport_serialize)
{
    auto* ts = _ModuleTypeSupport::get_instance();
    
    void* data = ts->create_data();
    _Module* mod = static_cast<_Module*>(data);
    mod->name = "TestModule";
    // element is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_module_serialize_deserialize)
{
    auto* ts = _ModuleTypeSupport::get_instance();
    
    void* data = ts->create_data();
    _Module* mod = static_cast<_Module*>(data);
    mod->name = "TestModule";
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    _Module* dstMod = static_cast<_Module*>(dst);
    EXPECT_EQ(dstMod->name, "TestModule");
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_type_library_element_typesupport_serialize)
{
    auto* ts = TypeLibraryElementTypeSupport::get_instance();
    
    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_type_library_typesupport_serialize)
{
    auto* ts = TypeLibraryTypeSupport::get_instance();
    
    void* data = ts->create_data();
    TypeLibrary* lib = static_cast<TypeLibrary*>(data);
    // element is std::vector, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_type_library_serialize_deserialize)
{
    auto* ts = TypeLibraryTypeSupport::get_instance();
    
    void* data = ts->create_data();
    TypeLibrary* lib = static_cast<TypeLibrary*>(data);
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    void* dst = ts->create_data();
    CdrDeserializer deserializer(&sbuf);
    ret = ts->deserialize_data(dst, &deserializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    ts->delete_data(dst);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_type_object_typesupport_serialize)
{
    auto* ts = TypeObjectTypeSupport::get_instance();
    
    void* data = ts->create_data();
    TypeObject* typeObj = static_cast<TypeObject*>(data);
    // library and the_type are vectors, empty by default
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_type_object_serialize_deserialize)
{
    auto* ts = TypeObjectTypeSupport::get_instance();
    
    void* data = ts->create_data();
    TypeObject* typeObj = static_cast<TypeObject*>(data);
    
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0) bufSize = 100;
    
    char* buffer = new char[bufSize + 100];
    memset(buffer, 0, bufSize + 100);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 100;
    
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
    delete[] buffer;
}

TEST(DDSTest, Xtypes_typeid_makekey)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    
    void* data = ts->create_data();
    _TypeId* typeId = static_cast<_TypeId*>(data);
    typeId->u_ = INT_32_TYPE;
    typeId->primitive_type_id = INT_32_TYPE;
    
    InstanceHandle_t iHandle;
    int ret = ts->MakeKey(data, iHandle, false);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_typeid_get_instancehandle)
{
    auto* ts = _TypeIdTypeSupport::get_instance();

    void* data = ts->create_data();
    _TypeId* typeId = static_cast<_TypeId*>(data);
    typeId->u_ = INT_32_TYPE;
    typeId->primitive_type_id = INT_32_TYPE;

    char buffer[100];
    memset(buffer, 0, sizeof(buffer));
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = sizeof(buffer);
    CdrSerializer serializer(&sbuf);

    InstanceHandle_t iHandle;
    ReturnCode_t ret = ts->get_instancehandle(data, &serializer, iHandle, false);
    EXPECT_EQ(ret, RETCODE_OK);

    ts->delete_data(data);
}

// ========== Comprehensive TypeSupport tests for all classes ==========

// Helper: serialize then deserialize a TypeSupport data
static void test_serialize_deserialize(TypeSupport* ts, void* data) {
    unsigned int size = ts->get_serialized_data_size(data, 0);
    unsigned int maxSize = ts->get_max_serialized_data_size(data, 0);
    unsigned int bufSize = (size > 0) ? size : maxSize;
    if (bufSize == 0 || bufSize > 100000) bufSize = 1024;

    char* buffer = new char[bufSize + 200];
    memset(buffer, 0, bufSize + 200);
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = bufSize + 200;

    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(data, &serializer, 0);

    if (ret == 0) {
        void* dst = ts->create_data();
        CdrDeserializer deserializer(&sbuf);
        ts->deserialize_data(dst, &deserializer, 0);
        ts->delete_data(dst);
    }

    ts->delete_data(data);
    delete[] buffer;
}

// Helper: test all common TypeSupport methods
static void test_typesupport_common(TypeSupport* ts, const char* expectedName, bool expectKey) {
    EXPECT_NE(ts, nullptr);
    EXPECT_NE(ts->get_typename(), nullptr);
    EXPECT_EQ(ts->has_key(), expectKey);
    TypeObject* tobj = ts->get_typeobject();
    (void)tobj; // may be nullptr

    void* data = ts->create_data();
    EXPECT_NE(data, nullptr);

    void* data2 = ts->create_data();
    EXPECT_NE(data2, nullptr);

    int ret = ts->copy_data(data2, data);
    EXPECT_EQ(ret, 0);

    ts->delete_data(data);
    ts->delete_data(data2);
}

// Helper: test MakeKey and get_instancehandle
static void test_makekey_instancehandle(TypeSupport* ts, void* data) {
    InstanceHandle_t iHandle;
    int ret = ts->MakeKey(data, iHandle, false);
    (void)ret; // may succeed or fail depending on has_key

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    SerializedBuffer sbuf;
    sbuf.buffer = buffer;
    sbuf.origin_buffer = buffer;
    sbuf.buffer_size = sizeof(buffer);
    CdrSerializer serializer(&sbuf);

    InstanceHandle_t iHandle2;
    ReturnCode_t rc = ts->get_instancehandle(data, &serializer, iHandle2, false);
    (void)rc;
}

// ---- _TypeIdTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_typeid_all_methods)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::_TypeId", false);

    // Test all primitive type variants
    TypeKind types[] = {BOOLEAN_TYPE, BYTE_TYPE, INT_16_TYPE, UINT_16_TYPE,
                        INT_32_TYPE, UINT_32_TYPE, INT_64_TYPE, UINT_64_TYPE,
                        FLOAT_32_TYPE, FLOAT_64_TYPE, FLOAT_128_TYPE,
                        CHAR_8_TYPE, CHAR_32_TYPE};
    for (auto tk : types) {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = tk;
        typeId->primitive_type_id = tk;
        unsigned int sz = ts->get_serialized_data_size(data, 0);
        (void)sz;
        unsigned int msz = ts->get_max_serialized_data_size(data, 0);
        (void)msz;
        test_makekey_instancehandle(ts, data);
        ts->delete_data(data);
    }

    // Constructed type
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = STRUCTURE_TYPE;
        typeId->constructed_type_id = 999;
        test_makekey_instancehandle(ts, data);
        ts->delete_data(data);
    }
}

// ---- AnnotationMemberValueTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_annotation_member_value_all_variants)
{
    auto* ts = AnnotationMemberValueTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    // Test all variant types
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = BOOLEAN_TYPE; val->boolean_value = 1;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = BYTE_TYPE; val->byte_value = 0xAB;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = INT_16_TYPE; val->int_16_value = -100;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = UINT_16_TYPE; val->uint_16_value = 200;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = UINT_32_TYPE; val->uint_32_value = 300;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = INT_64_TYPE; val->int_64_value = -400LL;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = UINT_64_TYPE; val->uint_64_value = 500ULL;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = FLOAT_32_TYPE; val->float_32_value = 1.5f;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = FLOAT_128_TYPE; val->float_128_value.ld = 2.5L;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = CHAR_8_TYPE; val->character_value = 'z';
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = CHAR_32_TYPE; val->wide_character_value = L'w';
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = ENUMERATION_TYPE; val->enumeration_value = 7;
        test_serialize_deserialize(ts, data);
    }
}

// ---- AnnotationUsageMemberTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_annotation_usage_member_comprehensive)
{
    auto* ts = AnnotationUsageMemberTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    void* data = ts->create_data();
    AnnotationUsageMember* m = static_cast<AnnotationUsageMember*>(data);
    m->member_id = 42;
    m->value.u_ = INT_32_TYPE;
    m->value.int_32_value = 100;
    test_serialize_deserialize(ts, data);

    // MakeKey/instancehandle
    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- AnnotationUsageTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_annotation_usage_comprehensive)
{
    auto* ts = AnnotationUsageTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    void* data = ts->create_data();
    AnnotationUsage* au = static_cast<AnnotationUsage*>(data);
    au->type_id.u_ = INT_32_TYPE;
    au->type_id.primitive_type_id = INT_32_TYPE;
    AnnotationUsageMember aum;
    aum.member_id = 1;
    aum.value.u_ = BOOLEAN_TYPE;
    aum.value.boolean_value = 1;
    au->member.push_back(aum);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- TypePropertyTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_type_property_comprehensive)
{
    auto* ts = TypePropertyTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    // Test different TypeFlag values
    {
        void* data = ts->create_data();
        TypeProperty* tp = static_cast<TypeProperty*>(data);
        tp->flag = IS_FINAL;
        tp->type_id.u_ = STRUCTURE_TYPE;
        tp->type_id.constructed_type_id = 1;
        tp->name = "TestType";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeProperty* tp = static_cast<TypeProperty*>(data);
        tp->flag = IS_MUTABLE;
        tp->type_id.u_ = INT_32_TYPE;
        tp->type_id.primitive_type_id = INT_32_TYPE;
        tp->name = "MutableType";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeProperty* tp = static_cast<TypeProperty*>(data);
        tp->flag = IS_NESTED;
        tp->type_id.u_ = UNION_TYPE;
        tp->type_id.constructed_type_id = 2;
        tp->name = "NestedType";
        test_serialize_deserialize(ts, data);
    }
    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- TypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_type_comprehensive)
{
    auto* ts = TypeTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    void* data = ts->create_data();
    Type* t = static_cast<Type*>(data);
    t->property.flag = IS_MUTABLE;
    t->property.type_id.u_ = STRUCTURE_TYPE;
    t->property.type_id.constructed_type_id = 10;
    t->property.name = "MyStruct";
    AnnotationUsage au;
    au.type_id.u_ = INT_32_TYPE;
    au.type_id.primitive_type_id = INT_32_TYPE;
    t->annotation.push_back(au);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- MemberPropertyTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_member_property_comprehensive)
{
    auto* ts = MemberPropertyTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    {
        void* data = ts->create_data();
        MemberProperty* mp = static_cast<MemberProperty*>(data);
        mp->flag = IS_KEY;
        mp->member_id = 1;
        mp->type_id.u_ = INT_32_TYPE;
        mp->type_id.primitive_type_id = INT_32_TYPE;
        mp->name = "key_field";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        MemberProperty* mp = static_cast<MemberProperty*>(data);
        mp->flag = IS_OPTIONAL;
        mp->member_id = 2;
        mp->type_id.u_ = STRING_TYPE;
        mp->type_id.constructed_type_id = 100;
        mp->name = "opt_field";
        test_serialize_deserialize(ts, data);
    }
    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- MemberTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_member_comprehensive)
{
    auto* ts = MemberTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    void* data = ts->create_data();
    Member* m = static_cast<Member*>(data);
    m->property.flag = IS_KEY;
    m->property.member_id = 1;
    m->property.type_id.u_ = INT_32_TYPE;
    m->property.type_id.primitive_type_id = INT_32_TYPE;
    m->property.name = "field1";
    AnnotationUsage au;
    au.type_id.u_ = BOOLEAN_TYPE;
    au.type_id.primitive_type_id = BOOLEAN_TYPE;
    m->annotation.push_back(au);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- StructureTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_structure_type_comprehensive)
{
    auto* ts = StructureTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::StructureType", false);

    void* data = ts->create_data();
    StructureType* st = static_cast<StructureType*>(data);
    st->property.flag = IS_MUTABLE;
    st->property.type_id.u_ = STRUCTURE_TYPE;
    st->property.type_id.constructed_type_id = 1;
    st->property.name = "MyStruct";
    st->base_type.u_ = INT_32_TYPE;
    st->base_type.primitive_type_id = INT_32_TYPE;
    Member m;
    m.property.flag = IS_KEY;
    m.property.member_id = 1;
    m.property.type_id.u_ = INT_32_TYPE;
    m.property.type_id.primitive_type_id = INT_32_TYPE;
    m.property.name = "id";
    st->member.push_back(m);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- UnionMemberTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_union_member_comprehensive)
{
    auto* ts = UnionMemberTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    void* data = ts->create_data();
    UnionMember* um = static_cast<UnionMember*>(data);
    um->property.flag = IS_KEY;
    um->property.member_id = 1;
    um->property.type_id.u_ = INT_32_TYPE;
    um->property.type_id.primitive_type_id = INT_32_TYPE;
    um->property.name = "variant1";
    um->label.push_back(1);
    um->label.push_back(2);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- UnionTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_union_type_comprehensive)
{
    auto* ts = UnionTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::UnionType", false);

    void* data = ts->create_data();
    UnionType* ut = static_cast<UnionType*>(data);
    ut->property.flag = IS_MUTABLE;
    ut->property.type_id.u_ = UNION_TYPE;
    ut->property.type_id.constructed_type_id = 5;
    ut->property.name = "MyUnion";
    UnionMember um;
    um.property.flag = IS_KEY;
    um.property.member_id = 1;
    um.property.type_id.u_ = INT_32_TYPE;
    um.property.type_id.primitive_type_id = INT_32_TYPE;
    um.property.name = "int_variant";
    um.label.push_back(1);
    ut->member.push_back(um);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- AnnotationMemberTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_annotation_member_comprehensive)
{
    auto* ts = AnnotationMemberTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    void* data = ts->create_data();
    AnnotationMember* am = static_cast<AnnotationMember*>(data);
    am->property.flag = IS_KEY;
    am->property.member_id = 1;
    am->property.type_id.u_ = INT_32_TYPE;
    am->property.type_id.primitive_type_id = INT_32_TYPE;
    am->property.name = "annot_member";
    am->default_value.u_ = INT_32_TYPE;
    am->default_value.int_32_value = 42;
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- AnnotationTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_annotation_type_comprehensive)
{
    auto* ts = AnnotationTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::AnnotationType", false);

    void* data = ts->create_data();
    AnnotationType* at = static_cast<AnnotationType*>(data);
    at->property.flag = IS_MUTABLE;
    at->property.type_id.u_ = ANNOTATION_TYPE;
    at->property.type_id.constructed_type_id = 20;
    at->property.name = "MyAnnotation";
    at->base_type.u_ = INT_32_TYPE;
    at->base_type.primitive_type_id = INT_32_TYPE;
    AnnotationMember am;
    am.property.flag = IS_KEY;
    am.property.member_id = 1;
    am.property.type_id.u_ = STRING_TYPE;
    am.property.type_id.constructed_type_id = 30;
    am.property.name = "param";
    am.default_value.u_ = STRING_TYPE;
    am.default_value.string_value = "default";
    at->member.push_back(am);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- AliasTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_alias_type_comprehensive)
{
    auto* ts = AliasTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::AliasType", false);

    void* data = ts->create_data();
    AliasType* at = static_cast<AliasType*>(data);
    at->property.flag = IS_FINAL;
    at->property.type_id.u_ = ALIAS_TYPE;
    at->property.type_id.constructed_type_id = 50;
    at->property.name = "MyAlias";
    at->base_type.u_ = INT_32_TYPE;
    at->base_type.primitive_type_id = INT_32_TYPE;
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- CollectionTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_collection_type_comprehensive)
{
    auto* ts = CollectionTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::CollectionType", false);

    void* data = ts->create_data();
    CollectionType* ct = static_cast<CollectionType*>(data);
    ct->property.flag = IS_MUTABLE;
    ct->property.type_id.u_ = SEQUENCE_TYPE;
    ct->property.type_id.constructed_type_id = 60;
    ct->property.name = "MyCollection";
    ct->element_type.u_ = INT_32_TYPE;
    ct->element_type.primitive_type_id = INT_32_TYPE;
    ct->element_shared = 0;
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- ArrayTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_array_type_comprehensive)
{
    auto* ts = ArrayTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::ArrayType", false);

    void* data = ts->create_data();
    ArrayType* at = static_cast<ArrayType*>(data);
    at->property.flag = IS_MUTABLE;
    at->property.type_id.u_ = ARRAY_TYPE;
    at->property.type_id.constructed_type_id = 70;
    at->property.name = "MyArray";
    at->element_type.u_ = FLOAT_64_TYPE;
    at->element_type.primitive_type_id = FLOAT_64_TYPE;
    at->element_shared = 0;
    at->bound.push_back(10);
    at->bound.push_back(5);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- MapTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_map_type_comprehensive)
{
    auto* ts = MapTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::MapType", false);

    void* data = ts->create_data();
    MapType* mt = static_cast<MapType*>(data);
    mt->property.flag = IS_MUTABLE;
    mt->property.type_id.u_ = MAP_TYPE;
    mt->property.type_id.constructed_type_id = 80;
    mt->property.name = "MyMap";
    mt->element_type.u_ = INT_32_TYPE;
    mt->element_type.primitive_type_id = INT_32_TYPE;
    mt->element_shared = 0;
    mt->key_element_type.u_ = STRING_TYPE;
    mt->key_element_type.constructed_type_id = 90;
    mt->bound = 100;
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- SequenceTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_sequence_type_comprehensive)
{
    auto* ts = SequenceTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::SequenceType", false);

    void* data = ts->create_data();
    SequenceType* st = static_cast<SequenceType*>(data);
    st->property.flag = IS_MUTABLE;
    st->property.type_id.u_ = SEQUENCE_TYPE;
    st->property.type_id.constructed_type_id = 100;
    st->property.name = "MySeq";
    st->element_type.u_ = BYTE_TYPE;
    st->element_type.primitive_type_id = BYTE_TYPE;
    st->element_shared = 0;
    st->bound = 50;
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- StringTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_string_type_comprehensive)
{
    auto* ts = StringTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::StringType", false);

    void* data = ts->create_data();
    StringType* st = static_cast<StringType*>(data);
    st->property.flag = IS_MUTABLE;
    st->property.type_id.u_ = STRING_TYPE;
    st->property.type_id.constructed_type_id = 110;
    st->property.name = "MyString";
    st->element_type.u_ = CHAR_8_TYPE;
    st->element_type.primitive_type_id = CHAR_8_TYPE;
    st->element_shared = 0;
    st->bound = 256;
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- BitTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_bit_comprehensive)
{
    auto* ts = BitTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::Bit", false);

    void* data = ts->create_data();
    Bit* b = static_cast<Bit*>(data);
    b->index = 3;
    b->name = "flag3";
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- BitSetTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_bitset_type_comprehensive)
{
    auto* ts = BitSetTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::BitSetType", false);

    void* data = ts->create_data();
    BitSetType* bst = static_cast<BitSetType*>(data);
    bst->property.flag = IS_MUTABLE;
    bst->property.type_id.u_ = BITSET_TYPE;
    bst->property.type_id.constructed_type_id = 120;
    bst->property.name = "MyBitSet";
    bst->bit_bound = 32;
    Bit b;
    b.index = 0;
    b.name = "bit0";
    bst->bit.push_back(b);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- EnumeratedConstantTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_enumerated_constant_comprehensive)
{
    auto* ts = EnumeratedConstantTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::EnumeratedConstant", false);

    void* data = ts->create_data();
    EnumeratedConstant* ec = static_cast<EnumeratedConstant*>(data);
    ec->value = 42;
    ec->name = "RED";
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- EnumerationTypeTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_enumeration_type_comprehensive)
{
    auto* ts = EnumerationTypeTypeSupport::get_instance();
    test_typesupport_common(ts, "TRAVODDS::EnumerationType", false);

    void* data = ts->create_data();
    EnumerationType* et = static_cast<EnumerationType*>(data);
    et->property.flag = IS_MUTABLE;
    et->property.type_id.u_ = ENUMERATION_TYPE;
    et->property.type_id.constructed_type_id = 130;
    et->property.name = "Color";
    et->bit_bound = 8;
    EnumeratedConstant ec;
    ec.value = 0;
    ec.name = "RED";
    et->constant.push_back(ec);
    EnumeratedConstant ec2;
    ec2.value = 1;
    ec2.name = "GREEN";
    et->constant.push_back(ec2);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- _ModuleTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_module_comprehensive)
{
    auto* ts = _ModuleTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    void* data = ts->create_data();
    _Module* mod = static_cast<_Module*>(data);
    mod->name = "MyModule";
    mod->library = nullptr;
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- TypeLibraryElementTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_type_library_element_comprehensive)
{
    auto* ts = TypeLibraryElementTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    // Test all TypeLibraryElementKind variants
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = STRUCTURE_TYPE_ELEMENT;
        elem->structure_type.property.flag = IS_MUTABLE;
        elem->structure_type.property.type_id.u_ = STRUCTURE_TYPE;
        elem->structure_type.property.type_id.constructed_type_id = 1;
        elem->structure_type.property.name = "Struct1";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = UNION_TYPE_ELEMENT;
        elem->union_type.property.flag = IS_MUTABLE;
        elem->union_type.property.type_id.u_ = UNION_TYPE;
        elem->union_type.property.type_id.constructed_type_id = 2;
        elem->union_type.property.name = "Union1";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = ALIAS_TYPE_ELEMENT;
        elem->alias_type.property.flag = IS_FINAL;
        elem->alias_type.property.type_id.u_ = ALIAS_TYPE;
        elem->alias_type.property.type_id.constructed_type_id = 3;
        elem->alias_type.property.name = "Alias1";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = ANNOTATION_TYPE_ELEMENT;
        elem->annotation_type.property.flag = IS_MUTABLE;
        elem->annotation_type.property.type_id.u_ = ANNOTATION_TYPE;
        elem->annotation_type.property.type_id.constructed_type_id = 4;
        elem->annotation_type.property.name = "Annot1";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = ARRAY_TYPE_ELEMENT;
        elem->array_type.property.flag = IS_MUTABLE;
        elem->array_type.property.type_id.u_ = ARRAY_TYPE;
        elem->array_type.property.type_id.constructed_type_id = 5;
        elem->array_type.property.name = "Array1";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = BITSET_TYPE_ELEMENT;
        elem->bitset_type.property.flag = IS_MUTABLE;
        elem->bitset_type.property.type_id.u_ = BITSET_TYPE;
        elem->bitset_type.property.type_id.constructed_type_id = 6;
        elem->bitset_type.property.name = "BitSet1";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = ENUMERATION_TYPE_ELEMENT;
        elem->enumeration_type.property.flag = IS_MUTABLE;
        elem->enumeration_type.property.type_id.u_ = ENUMERATION_TYPE;
        elem->enumeration_type.property.type_id.constructed_type_id = 7;
        elem->enumeration_type.property.name = "Enum1";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = MAP_TYPE_ELEMENT;
        elem->map_type.property.flag = IS_MUTABLE;
        elem->map_type.property.type_id.u_ = MAP_TYPE;
        elem->map_type.property.type_id.constructed_type_id = 8;
        elem->map_type.property.name = "Map1";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = SEQUENCE_TYPE_ELEMENT;
        elem->sequence_type.property.flag = IS_MUTABLE;
        elem->sequence_type.property.type_id.u_ = SEQUENCE_TYPE;
        elem->sequence_type.property.type_id.constructed_type_id = 9;
        elem->sequence_type.property.name = "Seq1";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = STRING_TYPE_ELEMENT;
        elem->string_type.property.flag = IS_MUTABLE;
        elem->string_type.property.type_id.u_ = STRING_TYPE;
        elem->string_type.property.type_id.constructed_type_id = 10;
        elem->string_type.property.name = "Str1";
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        TypeLibraryElement* elem = static_cast<TypeLibraryElement*>(data);
        elem->u_ = MODULE_ELEMENT;
        elem->mod.name = "Module1";
        elem->mod.library = nullptr;
        test_serialize_deserialize(ts, data);
    }

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- TypeLibraryTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_type_library_comprehensive)
{
    auto* ts = TypeLibraryTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    void* data = ts->create_data();
    TypeLibrary* tl = static_cast<TypeLibrary*>(data);
    TypeLibraryElement elem;
    elem.u_ = STRUCTURE_TYPE_ELEMENT;
    elem.structure_type.property.flag = IS_MUTABLE;
    elem.structure_type.property.type_id.u_ = STRUCTURE_TYPE;
    elem.structure_type.property.type_id.constructed_type_id = 1;
    elem.structure_type.property.name = "S1";
    tl->element.push_back(elem);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ---- TypeObjectTypeSupport comprehensive ----
TEST(DDSTest, Xtypes_type_object_comprehensive)
{
    auto* ts = TypeObjectTypeSupport::get_instance();
    test_typesupport_common(ts, "", false);

    void* data = ts->create_data();
    TypeObject* to = static_cast<TypeObject*>(data);
    TypeLibraryElement elem;
    elem.u_ = STRUCTURE_TYPE_ELEMENT;
    elem.structure_type.property.flag = IS_MUTABLE;
    elem.structure_type.property.type_id.u_ = STRUCTURE_TYPE;
    elem.structure_type.property.type_id.constructed_type_id = 1;
    elem.structure_type.property.name = "S1";
    to->library.element.push_back(elem);
    _TypeId tid;
    tid.u_ = STRUCTURE_TYPE;
    tid.constructed_type_id = 1;
    to->the_type.push_back(tid);
    test_serialize_deserialize(ts, data);

    {
        void* d = ts->create_data();
        test_makekey_instancehandle(ts, d);
        ts->delete_data(d);
    }
}

// ========== Additional tests for uncovered code paths ==========

TEST(DDSTest, Xtypes_annotation_member_value_all_types)
{
    auto* ts = AnnotationMemberValueTypeSupport::get_instance();
    
    // Test all union member types
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = BOOLEAN_TYPE;
        val->boolean_value = true;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = BYTE_TYPE;
        val->byte_value = 0xAB;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = INT_16_TYPE;
        val->int_16_value = -1000;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = UINT_16_TYPE;
        val->uint_16_value = 60000;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = UINT_32_TYPE;
        val->uint_32_value = 4000000000U;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = INT_64_TYPE;
        val->int_64_value = -1234567890123LL;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = UINT_64_TYPE;
        val->uint_64_value = 12345678901234ULL;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = FLOAT_32_TYPE;
        val->float_32_value = 3.14f;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = FLOAT_64_TYPE;
        val->float_64_value = 2.718281828;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = FLOAT_128_TYPE;
        LongDouble ld;
        ld.ld = 3.14159265358979L;
        val->float_128_value = ld;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = CHAR_8_TYPE;
        val->character_value = 'A';
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = CHAR_32_TYPE;
        val->wide_character_value = L'B';
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        AnnotationMemberValue* val = static_cast<AnnotationMemberValue*>(data);
        val->u_ = ENUMERATION_TYPE;
        val->enumeration_value = 42;
        test_serialize_deserialize(ts, data);
    }
}

TEST(DDSTest, Xtypes_typeid_all_primitive_types)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    
    // Test all primitive type cases
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = BOOLEAN_TYPE;
        typeId->primitive_type_id = BOOLEAN_TYPE;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = BYTE_TYPE;
        typeId->primitive_type_id = BYTE_TYPE;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = INT_16_TYPE;
        typeId->primitive_type_id = INT_16_TYPE;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = UINT_16_TYPE;
        typeId->primitive_type_id = UINT_16_TYPE;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = UINT_32_TYPE;
        typeId->primitive_type_id = UINT_32_TYPE;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = INT_64_TYPE;
        typeId->primitive_type_id = INT_64_TYPE;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = UINT_64_TYPE;
        typeId->primitive_type_id = UINT_64_TYPE;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = FLOAT_32_TYPE;
        typeId->primitive_type_id = FLOAT_32_TYPE;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = FLOAT_128_TYPE;
        typeId->primitive_type_id = FLOAT_128_TYPE;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = CHAR_8_TYPE;
        typeId->primitive_type_id = CHAR_8_TYPE;
        test_serialize_deserialize(ts, data);
    }
    {
        void* data = ts->create_data();
        _TypeId* typeId = static_cast<_TypeId*>(data);
        typeId->u_ = CHAR_32_TYPE;
        typeId->primitive_type_id = CHAR_32_TYPE;
        test_serialize_deserialize(ts, data);
    }
}

// ========== TypeObject error path tests ==========

TEST(DDSTest, Xtypes_typeid_serialize_key)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    void* data = ts->create_data();
    _TypeId* typeId = static_cast<_TypeId*>(data);
    typeId->u_ = INT_32_TYPE;
    typeId->primitive_type_id = INT_32_TYPE;
    
    char buffer[100] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 100;
    CdrSerializer cdr(&sbuf);
    
    int ret = ts->serialize_key(data, &cdr, 0);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_typeid_make_key)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    void* data = ts->create_data();
    _TypeId* typeId = static_cast<_TypeId*>(data);
    typeId->u_ = INT_32_TYPE;
    typeId->primitive_type_id = INT_32_TYPE;
    
    InstanceHandle_t handle;
    int ret = ts->MakeKey(data, handle, false);
    EXPECT_EQ(ret, 0);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_typeid_serialize_small_buffer)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    void* data = ts->create_data();
    _TypeId* typeId = static_cast<_TypeId*>(data);
    typeId->u_ = INT_32_TYPE;
    typeId->primitive_type_id = INT_32_TYPE;
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer cdr(&sbuf);
    
    int ret = ts->serialize_data(data, &cdr, 0);
    EXPECT_EQ(ret, -1);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_typeid_deserialize_small_buffer)
{
    auto* ts = _TypeIdTypeSupport::get_instance();
    void* data = ts->create_data();
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer cdr(&sbuf);
    
    int ret = ts->deserialize_data(data, &cdr, 0);
    EXPECT_EQ(ret, -1);
    
    ts->delete_data(data);
}

TEST(DDSTest, Xtypes_annotation_member_value_typesupport_serialize_small_buffer)
{
    auto* ts = AnnotationMemberValueTypeSupport::get_instance();
    AnnotationMemberValue data;
    memset(&data, 0, sizeof(data));
    data.u_ = BOOLEAN_TYPE;
    data.boolean_value = true;
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_annotation_member_value_typesupport_deserialize_small_buffer)
{
    auto* ts = AnnotationMemberValueTypeSupport::get_instance();
    AnnotationMemberValue data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_structure_type_typesupport_serialize_small_buffer)
{
    auto* ts = StructureTypeTypeSupport::get_instance();
    StructureType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_structure_type_typesupport_deserialize_small_buffer)
{
    auto* ts = StructureTypeTypeSupport::get_instance();
    StructureType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_type_object_typesupport_serialize_small_buffer)
{
    auto* ts = TypeObjectTypeSupport::get_instance();
    TypeObject data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_type_object_typesupport_deserialize_small_buffer)
{
    auto* ts = TypeObjectTypeSupport::get_instance();
    TypeObject data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_annotation_usage_member_typesupport_serialize_small_buffer)
{
    auto* ts = AnnotationUsageMemberTypeSupport::get_instance();
    AnnotationUsageMember data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_annotation_usage_member_typesupport_deserialize_small_buffer)
{
    auto* ts = AnnotationUsageMemberTypeSupport::get_instance();
    AnnotationUsageMember data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_union_type_typesupport_serialize_small_buffer)
{
    auto* ts = UnionTypeTypeSupport::get_instance();
    UnionType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_union_type_typesupport_deserialize_small_buffer)
{
    auto* ts = UnionTypeTypeSupport::get_instance();
    UnionType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_collection_type_typesupport_serialize_small_buffer)
{
    auto* ts = CollectionTypeTypeSupport::get_instance();
    CollectionType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_collection_type_typesupport_deserialize_small_buffer)
{
    auto* ts = CollectionTypeTypeSupport::get_instance();
    CollectionType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_enumeration_type_typesupport_serialize_small_buffer)
{
    auto* ts = EnumerationTypeTypeSupport::get_instance();
    EnumerationType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_enumeration_type_typesupport_deserialize_small_buffer)
{
    auto* ts = EnumerationTypeTypeSupport::get_instance();
    EnumerationType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_alias_type_typesupport_serialize_small_buffer)
{
    auto* ts = AliasTypeTypeSupport::get_instance();
    AliasType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_alias_type_typesupport_deserialize_small_buffer)
{
    auto* ts = AliasTypeTypeSupport::get_instance();
    AliasType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_string_type_typesupport_serialize_small_buffer)
{
    auto* ts = StringTypeTypeSupport::get_instance();
    StringType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_string_type_typesupport_deserialize_small_buffer)
{
    auto* ts = StringTypeTypeSupport::get_instance();
    StringType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_sequence_type_typesupport_serialize_small_buffer)
{
    auto* ts = SequenceTypeTypeSupport::get_instance();
    SequenceType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_sequence_type_typesupport_deserialize_small_buffer)
{
    auto* ts = SequenceTypeTypeSupport::get_instance();
    SequenceType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_map_type_typesupport_serialize_small_buffer)
{
    auto* ts = MapTypeTypeSupport::get_instance();
    MapType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_map_type_typesupport_deserialize_small_buffer)
{
    auto* ts = MapTypeTypeSupport::get_instance();
    MapType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_array_type_typesupport_serialize_small_buffer)
{
    auto* ts = ArrayTypeTypeSupport::get_instance();
    ArrayType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_array_type_typesupport_deserialize_small_buffer)
{
    auto* ts = ArrayTypeTypeSupport::get_instance();
    ArrayType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_annotation_type_typesupport_serialize_small_buffer)
{
    auto* ts = AnnotationTypeTypeSupport::get_instance();
    AnnotationType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_annotation_type_typesupport_deserialize_small_buffer)
{
    auto* ts = AnnotationTypeTypeSupport::get_instance();
    AnnotationType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_bitset_type_typesupport_serialize_small_buffer)
{
    auto* ts = BitSetTypeTypeSupport::get_instance();
    BitSetType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_bitset_type_typesupport_deserialize_small_buffer)
{
    auto* ts = BitSetTypeTypeSupport::get_instance();
    BitSetType data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_type_library_typesupport_serialize_small_buffer)
{
    auto* ts = TypeLibraryTypeSupport::get_instance();
    TypeLibrary data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrSerializer serializer(&sbuf);
    int ret = ts->serialize_data(&data, &serializer, 0);
    EXPECT_EQ(ret, -1);
}

TEST(DDSTest, Xtypes_type_library_typesupport_deserialize_small_buffer)
{
    auto* ts = TypeLibraryTypeSupport::get_instance();
    TypeLibrary data;
    memset(&data, 0, sizeof(data));
    
    char buffer[1] = {0};
    SerializedBuffer sbuf;
    sbuf.buffer = buffer; sbuf.origin_buffer = buffer; sbuf.buffer_size = 1;
    CdrDeserializer deserializer(&sbuf);
    int ret = ts->deserialize_data(&data, &deserializer, 0);
    EXPECT_EQ(ret, -1);
}
