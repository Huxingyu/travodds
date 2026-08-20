#include "testdataDynamicType.h"
#include "xtypes/dynamictypebuilderfactory.h"
#include "xtypes/dynamictypebuilder.h"

USING_TRAVODDS_NAMESPACE

namespace _DynamicType {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void AddMember(DynamicTypeBuilder* builder, const std::string& name, DynamicType* type, uint32_t& id, uint32_t& idx) {
    MemberDescriptor m;
    m.name_ = name; m.type_ = type; m.id_ = id++; m.index_ = idx++;
    builder->add_member(m);
}

static void InheritanceStructure(DynamicTypeBuilder * builder, const DynamicType * parent, uint32_t& id, uint32_t& idx) {
    for (auto& it : parent->member_) { 
        MemberDescriptor m;
        it.second.get_descriptor(m);
        builder->add_member(m);
        for (auto& ann : it.second.annotation_) {
            builder->apply_annotation_to_member(m.id_, ann);
        }
    }
    idx = id = builder->get_member_count();
}
#pragma GCC diagnostic pop

namespace TestModule {

static DynamicType* CreateDynamicTypeBasicData(){
    DynamicTypeBuilderFactory* factory = DynamicTypeBuilderFactory::get_instance();
    TypeDescriptor desc;
    desc.kind_ = TK_STRUCTURE;
    desc.name_ = "BasicData";
    DynamicTypeBuilder* builder = factory->create_type(&desc);
    uint32_t id = 0, idx = 0;
    AddMember(builder,"id", factory->get_primitive_type(TK_INT32), id, idx);
    AddMember(builder,"name", factory->create_string_type(0)->build(), id, idx);
    return builder->build();
};

const DynamicType* BasicData() {
    static const DynamicType* type = CreateDynamicTypeBasicData();
    return type;
}

static DynamicType* CreateDynamicTypeKeyData(){
    DynamicTypeBuilderFactory* factory = DynamicTypeBuilderFactory::get_instance();
    TypeDescriptor desc;
    desc.kind_ = TK_STRUCTURE;
    desc.name_ = "KeyData";
    DynamicTypeBuilder* builder = factory->create_type(&desc);
    uint32_t id = 0, idx = 0;
    AddMember(builder,"userId", factory->get_primitive_type(TK_INT32), id, idx);
    {
        unsigned long tid = id - 1;
        AnnotationDescriptor annDesc;
        annDesc.type_ = factory->get_primitive_type(TK_INT32);
        annDesc.value_["@key"] = "1";
        builder->apply_annotation_to_member(tid, annDesc);
    }
    AddMember(builder,"data", factory->create_string_type(0)->build(), id, idx);
    return builder->build();
};

const DynamicType* KeyData() {
    static const DynamicType* type = CreateDynamicTypeKeyData();
    return type;
}

static DynamicType* CreateDynamicTypeSequenceData(){
    DynamicTypeBuilderFactory* factory = DynamicTypeBuilderFactory::get_instance();
    TypeDescriptor desc;
    desc.kind_ = TK_STRUCTURE;
    desc.name_ = "SequenceData";
    DynamicTypeBuilder* builder = factory->create_type(&desc);
    uint32_t id = 0, idx = 0;
    AddMember(builder,"numbers", factory->create_sequence_type(factory->get_primitive_type(TK_INT32), 0)->build(), id, idx);
    return builder->build();
};

const DynamicType* SequenceData() {
    static const DynamicType* type = CreateDynamicTypeSequenceData();
    return type;
}

static DynamicType* CreateDynamicTypeTestEnum(){
    DynamicTypeBuilderFactory* factory = DynamicTypeBuilderFactory::get_instance();
    TypeDescriptor desc;
    desc.kind_ = TK_ENUM;
    desc.name_ = "TestEnum";
    DynamicTypeBuilder* builder = factory->create_type(&desc);
    uint32_t id = 0, idx = 0, enumValue = 0;
    {
        MemberDescriptor mbdesc;
        mbdesc.name_ = "VALUE_A"; mbdesc.id_=id++; mbdesc.index_ = idx++; mbdesc.type_ = factory->get_primitive_type(TK_INT32);
        mbdesc.default_value_ = std::to_string(enumValue++);
        builder->add_member(mbdesc); 
    }
    {
        MemberDescriptor mbdesc;
        mbdesc.name_ = "VALUE_B"; mbdesc.id_=id++; mbdesc.index_ = idx++; mbdesc.type_ = factory->get_primitive_type(TK_INT32);
        mbdesc.default_value_ = std::to_string(enumValue++);
        builder->add_member(mbdesc); 
    }
    {
        MemberDescriptor mbdesc;
        mbdesc.name_ = "VALUE_C"; mbdesc.id_=id++; mbdesc.index_ = idx++; mbdesc.type_ = factory->get_primitive_type(TK_INT32);
        mbdesc.default_value_ = std::to_string(enumValue++);
        builder->add_member(mbdesc); 
    }
    {
        MemberDescriptor mbdesc;
        mbdesc.name_ = "VALUE_D"; mbdesc.id_=id++; mbdesc.index_ = idx++; mbdesc.type_ = factory->get_primitive_type(TK_INT32);
        mbdesc.default_value_ = std::to_string(enumValue++);
        builder->add_member(mbdesc); 
    }
    {
        MemberDescriptor mbdesc;
        mbdesc.name_ = "VALUE_E"; mbdesc.id_=id++; mbdesc.index_ = idx++; mbdesc.type_ = factory->get_primitive_type(TK_INT32);
        mbdesc.default_value_ = std::to_string(enumValue++);
        builder->add_member(mbdesc); 
    }
    return builder->build();
};

const DynamicType* TestEnum() {
    static const DynamicType* type = CreateDynamicTypeTestEnum();
    return type;
}

static DynamicType* CreateDynamicTypeNestedStruct(){
    DynamicTypeBuilderFactory* factory = DynamicTypeBuilderFactory::get_instance();
    TypeDescriptor desc;
    desc.kind_ = TK_STRUCTURE;
    desc.name_ = "NestedStruct";
    DynamicTypeBuilder* builder = factory->create_type(&desc);
    uint32_t id = 0, idx = 0;
    AddMember(builder,"nestedId", factory->get_primitive_type(TK_INT32), id, idx);
    AddMember(builder,"nestedValue", factory->get_primitive_type(TK_FLOAT64), id, idx);
    AddMember(builder,"nestedFlag", factory->get_primitive_type(TK_CHAR8), id, idx);
    AddMember(builder,"nestedName", factory->create_string_type(0)->build(), id, idx);
    AddMember(builder,"nestedArray", factory->create_array_type(factory->get_primitive_type(TK_INT32), { 2 })->build(), id, idx);
    return builder->build();
};

const DynamicType* NestedStruct() {
    static const DynamicType* type = CreateDynamicTypeNestedStruct();
    return type;
}

static DynamicType* CreateDynamicTypeFilterableData(){
    DynamicTypeBuilderFactory* factory = DynamicTypeBuilderFactory::get_instance();
    TypeDescriptor desc;
    desc.kind_ = TK_STRUCTURE;
    desc.name_ = "FilterableData";
    DynamicTypeBuilder* builder = factory->create_type(&desc);
    uint32_t id = 0, idx = 0;
    AddMember(builder,"shortField", factory->get_primitive_type(TK_INT16), id, idx);
    AddMember(builder,"ushortField", factory->get_primitive_type(TK_UINT16), id, idx);
    AddMember(builder,"longField", factory->get_primitive_type(TK_INT32), id, idx);
    AddMember(builder,"ulongField", factory->get_primitive_type(TK_UINT32), id, idx);
    AddMember(builder,"longlongField", factory->get_primitive_type(TK_INT64), id, idx);
    AddMember(builder,"ulonglongField", factory->get_primitive_type(TK_UINT64), id, idx);
    AddMember(builder,"floatField", factory->get_primitive_type(TK_FLOAT32), id, idx);
    AddMember(builder,"doubleField", factory->get_primitive_type(TK_FLOAT64), id, idx);
    AddMember(builder,"long_doubleField", factory->get_primitive_type(TK_FLOAT128), id, idx);
    AddMember(builder,"boolField", factory->get_primitive_type(TK_CHAR8), id, idx);
    AddMember(builder,"charField", factory->get_primitive_type(TK_CHAR8), id, idx);
    AddMember(builder,"byteField", factory->get_primitive_type(TK_BYTE), id, idx);
    AddMember(builder,"stringField", factory->create_string_type(0)->build(), id, idx);
    AddMember(builder,"fixedStringField", factory->create_string_type(0)->build(), id, idx);
    AddMember(builder,"enumField", const_cast<DynamicType*>(::_DynamicType::TestModule::TestEnum()), id, idx);
    AddMember(builder,"nestedField", const_cast<DynamicType*>(::_DynamicType::TestModule::NestedStruct()), id, idx);
    AddMember(builder,"longArray", factory->create_array_type(factory->get_primitive_type(TK_INT32), { 10 })->build(), id, idx);
    AddMember(builder,"doubleArray", factory->create_array_type(factory->get_primitive_type(TK_FLOAT64), { 5 })->build(), id, idx);
    AddMember(builder,"boolArray", factory->create_array_type(factory->get_primitive_type(TK_CHAR8), { 3 })->build(), id, idx);
    AddMember(builder,"stringArray", factory->create_array_type(factory->create_string_type(0)->build(), { 4 })->build(), id, idx);
    AddMember(builder,"byteArray", factory->create_array_type(factory->get_primitive_type(TK_BYTE), { 10 })->build(), id, idx);
    AddMember(builder,"shortArray", factory->create_array_type(factory->get_primitive_type(TK_INT16), { 5 })->build(), id, idx);
    AddMember(builder,"usArray", factory->create_array_type(factory->get_primitive_type(TK_UINT16), { 5 })->build(), id, idx);
    AddMember(builder,"llArray", factory->create_array_type(factory->get_primitive_type(TK_INT64), { 3 })->build(), id, idx);
    AddMember(builder,"ullArray", factory->create_array_type(factory->get_primitive_type(TK_UINT64), { 3 })->build(), id, idx);
    AddMember(builder,"ldArray", factory->create_array_type(factory->get_primitive_type(TK_FLOAT128), { 10 })->build(), id, idx);
    AddMember(builder,"ulArray", factory->create_array_type(factory->get_primitive_type(TK_UINT32), { 2 })->build(), id, idx);
    AddMember(builder,"matrix2x3", factory->create_array_type(factory->get_primitive_type(TK_INT32), { 2, 3 })->build(), id, idx);
    AddMember(builder,"floatMatrix", factory->create_array_type(factory->get_primitive_type(TK_FLOAT32), { 3, 3 })->build(), id, idx);
    AddMember(builder,"matrix4x4", factory->create_array_type(factory->get_primitive_type(TK_INT32), { 4, 4 })->build(), id, idx);
    AddMember(builder,"seqLongField", factory->create_sequence_type(factory->get_primitive_type(TK_INT32), 0)->build(), id, idx);
    AddMember(builder,"seqStringField", factory->create_sequence_type(factory->create_string_type(0)->build(), 0)->build(), id, idx);
    AddMember(builder,"shortBuffer", factory->create_array_type(factory->get_primitive_type(TK_INT16), { 8 })->build(), id, idx);
    AddMember(builder,"charBuffer", factory->create_array_type(factory->get_primitive_type(TK_CHAR8), { 32 })->build(), id, idx);
    AddMember(builder,"byteBuffer", factory->create_array_type(factory->get_primitive_type(TK_BYTE), { 64 })->build(), id, idx);
    AddMember(builder,"measurements", factory->create_array_type(factory->get_primitive_type(TK_FLOAT64), { 10 })->build(), id, idx);
    AddMember(builder,"enumArray", factory->create_array_type(const_cast<DynamicType*>(::_DynamicType::TestModule::TestEnum()), { 5 })->build(), id, idx);
    AddMember(builder,"structArray", factory->create_array_type(const_cast<DynamicType*>(::_DynamicType::TestModule::NestedStruct()), { 3 })->build(), id, idx);
    return builder->build();
};

const DynamicType* FilterableData() {
    static const DynamicType* type = CreateDynamicTypeFilterableData();
    return type;
}

} // namespace TestModule

} // namespace _DynamicType

