#include "testdataDynamicType.h"
#include "xtypes/dynamictypebuilderfactory.h"
#include "xtypes/dynamictypebuilder.h"

USING_TRAVODDS_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

namespace _DynamicType {

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

const DynamicType* BasicData = CreateDynamicTypeBasicData();

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

const DynamicType* KeyData = CreateDynamicTypeKeyData();

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

const DynamicType* SequenceData = CreateDynamicTypeSequenceData();

} // namespace TestModule

} // namespace _DynamicType

#ifdef __cplusplus
}
#endif
