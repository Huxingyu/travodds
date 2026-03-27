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

#include "xtypes/dynamictypebuilderfactoryimpl.h"
#include "xtypes/dynamictype.h"
#include "xtypes/dynamictypebuilder.h"

TRAVODDS_NAMESPACE_BEGIN

DynamicTypeBuilderFactoryImpl *DynamicTypeBuilderFactoryImpl::s_instance_ =
    nullptr;

DynamicTypeBuilderFactory *DynamicTypeBuilderFactory::get_instance() {
    DynamicTypeBuilderFactoryImpl *instance =
        DynamicTypeBuilderFactoryImpl::get_instance();
    return instance;
}

ReturnCode_t DynamicTypeBuilderFactory::delete_instance() {
    return DynamicTypeBuilderFactoryImpl::delete_instance();
}

DynamicTypeBuilderFactoryImpl *DynamicTypeBuilderFactoryImpl::get_instance() {
    if (s_instance_ == nullptr) {
        s_instance_ = new DynamicTypeBuilderFactoryImpl();
    }
    return s_instance_;
}

ReturnCode_t DynamicTypeBuilderFactoryImpl::delete_instance() {
    if (s_instance_ != nullptr) {
        delete s_instance_;
        s_instance_ = nullptr;
    }
    return RETCODE_OK;
}

DynamicTypeBuilderFactoryImpl::DynamicTypeBuilderFactoryImpl() {
    // 依次创建基本数据类型的DynamicType
    std::map<TypeKind, std::string> primitive_kind_name = {
        {TK_BOOLEAN, "boolean"},   {TK_BYTE, "byte"},
        {TK_INT16, "int16"},       {TK_INT32, "int32"},
        {TK_INT64, "int64"},       {TK_UINT16, "uint16"},
        {TK_UINT32, "uint32"},     {TK_UINT64, "uint64"},
        {TK_FLOAT32, "float32"},   {TK_FLOAT64, "float64"},
        {TK_FLOAT128, "float128"}, {TK_CHAR8, "char8"},
        {TK_CHAR16, "char16"}};
    for (auto it = primitive_kind_name.begin(); it != primitive_kind_name.end();
         ++it) {
        TypeDescriptor descriptor;
        descriptor.kind_ = it->first;
        descriptor.name_ = it->second;
        DynamicType *type = new DynamicType(descriptor);
        primitive_types_[it->first] = type;
    }
}

DynamicTypeBuilderFactoryImpl::~DynamicTypeBuilderFactoryImpl() {
    // 回收基本数据类型的DynamicType
    for (auto it = primitive_types_.begin(); it != primitive_types_.end();
         ++it) {
        delete it->second;
    }
}

DynamicTypeBuilder *DynamicTypeBuilderFactoryImpl::create_array_type(
    DynamicType *element_type, const std::vector<unsigned int> &bound) {
    TypeDescriptor descriptor;
    descriptor.kind_ = TK_ARRAY;
    descriptor.element_type_ = element_type;
    descriptor.bound_ = bound;
    // 将bound转化为字符串
    std::string bound_str = "";
    for (auto it = bound.begin(); it != bound.end(); ++it) {
        bound_str += std::to_string(*it) + "_";
    }
    descriptor.name_ = element_type->get_name() + "array_" + bound_str;
    DynamicTypeBuilder *builder = create_type(&descriptor);
    return builder;
}

DynamicTypeBuilder *
DynamicTypeBuilderFactoryImpl::create_bitmask_type(unsigned int bound) {
    // 实现create_bitmask_type
    TypeDescriptor descriptor;
    descriptor.kind_ = TK_BITMASK;
    descriptor.bound_.push_back(bound);
    descriptor.name_ = "bitmask<" + std::to_string(bound) + ">";
    DynamicTypeBuilder *builder = create_type(&descriptor);
    return builder;
}

DynamicTypeBuilder *
DynamicTypeBuilderFactoryImpl::create_map_type(DynamicType *key_element_type,
                                               DynamicType *element_type,
                                               unsigned int bound) {
    // 实现create_map_type
    TypeDescriptor descriptor;
    descriptor.kind_ = TK_MAP;
    descriptor.key_element_type_ = key_element_type;
    descriptor.element_type_ = element_type;
    descriptor.bound_.push_back(bound);
    descriptor.name_ = "map<" + key_element_type->get_name() + ", " +
                       element_type->get_name() + ", " + std::to_string(bound) +
                       ">";
    DynamicTypeBuilder *builder = create_type(&descriptor);
    return builder;
}

DynamicTypeBuilder *
DynamicTypeBuilderFactoryImpl::create_sequence_type(DynamicType *element_type,
                                                    unsigned int bound) {
    // 实现create_sequence_type
    TypeDescriptor descriptor;
    descriptor.kind_ = TK_SEQUENCE;
    descriptor.element_type_ = element_type;
    descriptor.bound_.push_back(bound);
    descriptor.name_ = "sequence<" + element_type->get_name() + ", " +
                       std::to_string(bound) + ">";
    DynamicTypeBuilder *builder = create_type(&descriptor);
    return builder;
}

DynamicTypeBuilder *
DynamicTypeBuilderFactoryImpl::create_string_type(unsigned int bound) {
    // 实现create_string_type
    TypeDescriptor descriptor;
    descriptor.kind_ = TK_STRING8;
    descriptor.bound_.push_back(bound);
    descriptor.name_ = "string<" + std::to_string(bound) + ">";
    DynamicTypeBuilder *builder = create_type(&descriptor);
    return builder;
}

DynamicTypeBuilder *
DynamicTypeBuilderFactoryImpl::create_wstring_type(unsigned int bound) {
    // 实现create_wstring_type
    TypeDescriptor descriptor;
    descriptor.kind_ = TK_STRING16;
    descriptor.bound_.push_back(bound);
    descriptor.name_ = "wstring<" + std::to_string(bound) + ">";
    DynamicTypeBuilder *builder = create_type(&descriptor);
    return builder;
}

DynamicTypeBuilder *
DynamicTypeBuilderFactoryImpl::create_type(TypeDescriptor *descriptor) {
    // 实现create_type
    DynamicTypeBuilder *builder = new DynamicTypeBuilder(*descriptor);
    return builder;
}

ReturnCode_t DynamicTypeBuilderFactoryImpl::delete_type_builder(
    DynamicTypeBuilder *type_builder) {
    // 实现delete_type_builder
    delete type_builder;
    return RETCODE_OK;
}

ReturnCode_t DynamicTypeBuilderFactoryImpl::delete_type(DynamicType *type) {
    type->decrement_ref_count();
    return RETCODE_OK;
}

DynamicTypeBuilder *
DynamicTypeBuilderFactoryImpl::create_type_copy(DynamicType *type) {
    // 实现create_type_copy
    TypeDescriptor descriptor;
    type->get_descriptor(descriptor);
    DynamicTypeBuilder *builder = create_type(&descriptor);
    return builder;
}

DynamicTypeBuilder *DynamicTypeBuilderFactoryImpl::create_type_w_type_object(
    TypeObject *type_object) {
    (void)type_object;
    // 实现create_type_w_type_object
    return nullptr;
}

DynamicType *DynamicTypeBuilderFactoryImpl::get_primitive_type(int kind) {
    if (s_instance_->primitive_types_.find((TypeKind)kind) ==
        s_instance_->primitive_types_.end()) {
        return nullptr;
    }
    // 实现get_primitive_type
    return s_instance_->primitive_types_[(TypeKind)kind];
}

DynamicType *DynamicTypeBuilderFactoryImpl::create_type_w_uri(
    const std::string &document_url, const std::string &type_name,
    const std::vector<std::string> &include_paths) {
    (void)document_url;
    (void)type_name;
    (void)include_paths;
    // 实现create_type_w_uri
    return nullptr;
}

DynamicType *DynamicTypeBuilderFactoryImpl::create_type_w_document(
    const std::string &document, const std::string &type_name,
    const std::vector<std::string> &include_paths) {
    (void)document;
    (void)type_name;
    (void)include_paths;
    // 实现create_type_w_document
    return nullptr;
}

TRAVODDS_NAMESPACE_END
