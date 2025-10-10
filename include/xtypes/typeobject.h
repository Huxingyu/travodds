#ifndef typeobject_H
#define typeobject_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "cdr/travoddscdr.h"

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wsign-compare"

#if defined(DDS_TYPE_EXPORT) && defined(_WIN32)
#define DDS_TYPE_API __declspec(dllexport)
#else
#define DDS_TYPE_API 
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace TRAVODDS { class TypeLibrary; } 

namespace TRAVODDS {

typedef short TypeKind;

const TypeKind NO_TYPE = 0;

const TypeKind BOOLEAN_TYPE = 1;

const TypeKind BYTE_TYPE = 2;

const TypeKind INT_16_TYPE = 3;

const TypeKind UINT_16_TYPE = 4;

const TypeKind INT_32_TYPE = 5;

const TypeKind UINT_32_TYPE = 6;

const TypeKind INT_64_TYPE = 7;

const TypeKind UINT_64_TYPE = 8;

const TypeKind FLOAT_32_TYPE = 9;

const TypeKind FLOAT_64_TYPE = 10;

const TypeKind FLOAT_128_TYPE = 11;

const TypeKind CHAR_8_TYPE = 12;

const TypeKind CHAR_32_TYPE = 13;

const TypeKind ENUMERATION_TYPE = 14;

const TypeKind BITSET_TYPE = 15;

const TypeKind ALIAS_TYPE = 16;

const TypeKind ARRAY_TYPE = 17;

const TypeKind SEQUENCE_TYPE = 18;

const TypeKind STRING_TYPE = 19;

const TypeKind MAP_TYPE = 20;

const TypeKind UNION_TYPE = 21;

const TypeKind STRUCTURE_TYPE = 22;

const TypeKind ANNOTATION_TYPE = 23;

typedef short PrimitiveTypeId;

const PrimitiveTypeId NO_TYPE_ID = TRAVODDS::NO_TYPE;

const PrimitiveTypeId BOOLEAN_TYPE_ID = TRAVODDS::BOOLEAN_TYPE;

const PrimitiveTypeId BYTE_TYPE_ID = TRAVODDS::BYTE_TYPE;

const PrimitiveTypeId INT_16_TYPE_ID = TRAVODDS::INT_16_TYPE;

const PrimitiveTypeId UINT_16_TYPE_ID = TRAVODDS::UINT_16_TYPE;

const PrimitiveTypeId INT_32_TYPE_ID = TRAVODDS::INT_32_TYPE;

const PrimitiveTypeId UINT_32_TYPE_ID = TRAVODDS::UINT_32_TYPE;

const PrimitiveTypeId INT_64_TYPE_ID = TRAVODDS::INT_64_TYPE;

const PrimitiveTypeId UINT_64_TYPE_ID = TRAVODDS::UINT_64_TYPE;

const PrimitiveTypeId FLOAT_32_TYPE_ID = TRAVODDS::FLOAT_32_TYPE;

const PrimitiveTypeId FLOAT_64_TYPE_ID = TRAVODDS::FLOAT_64_TYPE;

const PrimitiveTypeId FLOAT_128_TYPE_ID = TRAVODDS::FLOAT_128_TYPE;

const PrimitiveTypeId CHAR_8_TYPE_ID = TRAVODDS::CHAR_8_TYPE;

const PrimitiveTypeId CHAR_32_TYPE_ID = TRAVODDS::CHAR_32_TYPE;

class DDS_TYPE_API _TypeId {
public:
    TRAVODDS::TypeKind u_;
    TRAVODDS::PrimitiveTypeId primitive_type_id;
    unsigned long long constructed_type_id;
};

typedef unsigned int MemberId;

const MemberId MEMBER_ID_INVALID = 268435455;

// @appendable
// @nested
class DDS_TYPE_API AnnotationMemberValue {
public:
    TRAVODDS::TypeKind u_;
    char boolean_value;
    unsigned char byte_value;
    short int_16_value;
    unsigned short uint_16_value;
    int int_32_value;
    unsigned int uint_32_value;
    long long int_64_value;
    unsigned long long uint_64_value;
    float float_32_value;
    double float_64_value;
    LongDouble float_128_value;
    char character_value;
    wchar_t wide_character_value;
    int enumeration_value;
    std::string string_value;
};

// @appendable
// @nested
class DDS_TYPE_API AnnotationUsageMember {
public:
    bool operator<(const AnnotationUsageMember& rhs) const {
        return false;
    }
    TRAVODDS::MemberId member_id;
    TRAVODDS::AnnotationMemberValue value;
};

// @appendable
// @nested
class DDS_TYPE_API AnnotationUsage {
public:
    bool operator<(const AnnotationUsage& rhs) const {
        return false;
    }
    TRAVODDS::_TypeId type_id;
    std::vector<TRAVODDS::AnnotationUsageMember> member;
};

// @bit_bound()
enum TypeFlag {
    IS_FINAL = 0,
    IS_MUTABLE = 1,
    IS_NESTED = 2,
};

// @appendable
// @nested
class DDS_TYPE_API TypeProperty {
public:
    bool operator<(const TypeProperty& rhs) const {
        return false;
    }
    TRAVODDS::TypeFlag flag;
    TRAVODDS::_TypeId type_id;
    std::string name;
};

enum TypeMemberId {
    PROPERTY_TYPE_MEMBER_ID = 0,
    ANNOTATION_TYPE_MEMBER_ID = 1,
};

// @mutable
// @nested
class DDS_TYPE_API Type {
public:
    bool operator<(const Type& rhs) const {
        return false;
    }
    TRAVODDS::TypeProperty property;// @id(TRAVODDS::PROPERTY_TYPE_MEMBER_ID) 
    std::vector<TRAVODDS::AnnotationUsage> annotation;// @id(TRAVODDS::ANNOTATION_TYPE_MEMBER_ID) 
};

// @bit_bound()
enum MemberFlag {
    IS_KEY = 0,
    IS_OPTIONAL = 1,
    IS_SHAREABLE = 2,
    IS_UNION_DEFAULT = 3,
};

// @appendable
// @nested
class DDS_TYPE_API MemberProperty {
public:
    bool operator<(const MemberProperty& rhs) const {
        return false;
    }
    TRAVODDS::MemberFlag flag;
    TRAVODDS::MemberId member_id;
    TRAVODDS::_TypeId type_id;
    std::string name;
};

enum MemberMemberId {
    PROPERTY_MEMBER_MEMBER_ID = 0,
    ANNOTATION_MEMBER_MEMBER_ID = 1,
};

// @mutable
// @nested
class DDS_TYPE_API Member {
public:
    bool operator<(const Member& rhs) const {
        return false;
    }
    TRAVODDS::MemberProperty property;// @id(TRAVODDS::PROPERTY_MEMBER_MEMBER_ID) 
    std::vector<TRAVODDS::AnnotationUsage> annotation;// @id(TRAVODDS::ANNOTATION_MEMBER_MEMBER_ID) 
};

enum StructureTypeMemberId {
    BASE_TYPE_STRUCTURETYPE_MEMBER_ID = 100,
    MEMBER_STRUCTURETYPE_MEMBER_ID = 101,
};

// @mutable
// @nested
class DDS_TYPE_API StructureType : public TRAVODDS::Type {
public:
    bool operator<(const StructureType& rhs) const {
        return false;
    }
    TRAVODDS::_TypeId base_type;// @id(TRAVODDS::BASE_TYPE_STRUCTURETYPE_MEMBER_ID) 
    std::vector<TRAVODDS::Member> member;// @id(TRAVODDS::MEMBER_STRUCTURETYPE_MEMBER_ID) 
};

enum UnionMemberMemberId {
    LABEL_UNIONMEMBER_MEMBER_ID = 100,
};

// @mutable
// @nested
class DDS_TYPE_API UnionMember : public TRAVODDS::Member {
public:
    bool operator<(const UnionMember& rhs) const {
        return false;
    }
    std::vector<int> label;// @id(TRAVODDS::LABEL_UNIONMEMBER_MEMBER_ID) 
};

enum UnionTypeMemberId {
    MEMBER_UNIONTYPE_MEMBER_ID = 100,
};

// @mutable
// @nested
class DDS_TYPE_API UnionType : public TRAVODDS::Type {
public:
    bool operator<(const UnionType& rhs) const {
        return false;
    }
    std::vector<TRAVODDS::UnionMember> member;// @id(TRAVODDS::MEMBER_UNIONTYPE_MEMBER_ID) 
};

enum AnnotationMemberMemberId {
    DEFAULT_VALUE_ANNOTATIONMEMBER_MEMBER_ID = 100,
};

// @mutable
// @nested
class DDS_TYPE_API AnnotationMember : public TRAVODDS::Member {
public:
    bool operator<(const AnnotationMember& rhs) const {
        return false;
    }
    TRAVODDS::AnnotationMemberValue default_value;// @id(TRAVODDS::DEFAULT_VALUE_ANNOTATIONMEMBER_MEMBER_ID) 
};

enum AnnotationTypeMemberId {
    BASE_TYPE_ANNOTATIONTYPE_MEMBER_ID = 100,
    MEMBER_ANNOTATIONTYPE_MEMBER_ID = 101,
};

// @mutable
// @nested
class DDS_TYPE_API AnnotationType : public TRAVODDS::Type {
public:
    bool operator<(const AnnotationType& rhs) const {
        return false;
    }
    TRAVODDS::_TypeId base_type;// @id(TRAVODDS::BASE_TYPE_ANNOTATIONTYPE_MEMBER_ID) 
    std::vector<TRAVODDS::AnnotationMember> member;// @id(TRAVODDS::MEMBER_ANNOTATIONTYPE_MEMBER_ID) 
};

enum AliasTypeMemberId {
    BASE_TYPE_ALIASTYPE_MEMBER_ID = 100,
};

// @mutable
// @nested
class DDS_TYPE_API AliasType : public TRAVODDS::Type {
public:
    bool operator<(const AliasType& rhs) const {
        return false;
    }
    TRAVODDS::_TypeId base_type;// @id(TRAVODDS::BASE_TYPE_ALIASTYPE_MEMBER_ID) 
};

typedef unsigned int Bound;

const Bound UNBOUNDED_COLLECTION = 0;

enum CollectionTypeMemberId {
    ELEMENT_TYPE_COLLECTIONTYPE_MEMBER_ID = 100,
    ELEMENT_SHARED_COLLECTIONTYPE_MEMBER_ID = 101,
};

// @mutable
// @nested
class DDS_TYPE_API CollectionType : public TRAVODDS::Type {
public:
    bool operator<(const CollectionType& rhs) const {
        return false;
    }
    TRAVODDS::_TypeId element_type;// @id(TRAVODDS::ELEMENT_TYPE_COLLECTIONTYPE_MEMBER_ID) 
    char element_shared;// @id(TRAVODDS::ELEMENT_SHARED_COLLECTIONTYPE_MEMBER_ID) 
};

enum ArrayTypeMemberId {
    BOUND_ARRAYTYPE_MEMBER_ID = 200,
};

// @mutable
// @nested
class DDS_TYPE_API ArrayType : public TRAVODDS::CollectionType {
public:
    bool operator<(const ArrayType& rhs) const {
        return false;
    }
    std::vector<TRAVODDS::Bound> bound;// @id(TRAVODDS::BOUND_ARRAYTYPE_MEMBER_ID) 
};

enum MapTypeMemberId {
    KEY_ELEMENT_TYPE_MAPTYPE_MEMBER_ID = 200,
    BOUND_MAPTYPE_MEMBER_ID = 201,
};

// @appendable
// @nested
class DDS_TYPE_API MapType : public TRAVODDS::CollectionType {
public:
    bool operator<(const MapType& rhs) const {
        return false;
    }
    TRAVODDS::_TypeId key_element_type;// @id(TRAVODDS::KEY_ELEMENT_TYPE_MAPTYPE_MEMBER_ID) 
    TRAVODDS::Bound bound;// @id(TRAVODDS::BOUND_MAPTYPE_MEMBER_ID) 
};

enum SequenceTypeMemberId {
    BOUND_SEQUENCETYPE_MEMBER_ID = 200,
};

// @appendable
// @nested
class DDS_TYPE_API SequenceType : public TRAVODDS::CollectionType {
public:
    bool operator<(const SequenceType& rhs) const {
        return false;
    }
    TRAVODDS::Bound bound;// @id(TRAVODDS::BOUND_SEQUENCETYPE_MEMBER_ID) 
};

enum StringTypeMemberId {
    BOUND_STRINGTYPE_MEMBER_ID = 200,
};

// @appendable
// @nested
class DDS_TYPE_API StringType : public TRAVODDS::CollectionType {
public:
    bool operator<(const StringType& rhs) const {
        return false;
    }
    TRAVODDS::Bound bound;// @id(TRAVODDS::BOUND_STRINGTYPE_MEMBER_ID) 
};

// @appendable
// @nested
class DDS_TYPE_API Bit {
public:
    bool operator<(const Bit& rhs) const {
        return false;
    }
    int index;
    std::string name;
};

enum BitSetTypeMemberId {
    BIT_BOUND_BITSETTYPE_MEMBER_ID = 100,
    BIT_BITSETTYPE_MEMBER_ID = 101,
};

// @mutable
// @nested
class DDS_TYPE_API BitSetType : public TRAVODDS::Type {
public:
    bool operator<(const BitSetType& rhs) const {
        return false;
    }
    TRAVODDS::Bound bit_bound;// @id(TRAVODDS::BIT_BOUND_BITSETTYPE_MEMBER_ID) 
    std::vector<TRAVODDS::Bit> bit;// @id(TRAVODDS::BIT_BITSETTYPE_MEMBER_ID) 
};

// @appendable
// @nested
class DDS_TYPE_API EnumeratedConstant {
public:
    bool operator<(const EnumeratedConstant& rhs) const {
        return false;
    }
    int value;
    std::string name;
};

enum EnumerationTypeMemberId {
    BIT_BOUND_ENUMERATIONTYPE_MEMBER_ID = 100,
    CONSTANT_ENUMERATIONTYPE_MEMBER_ID = 101,
};

// @mutable
// @nested
class DDS_TYPE_API EnumerationType : public TRAVODDS::Type {
public:
    bool operator<(const EnumerationType& rhs) const {
        return false;
    }
    TRAVODDS::Bound bit_bound;// @id(TRAVODDS::BIT_BOUND_ENUMERATIONTYPE_MEMBER_ID) 
    std::vector<TRAVODDS::EnumeratedConstant> constant;// @id(TRAVODDS::CONSTANT_ENUMERATIONTYPE_MEMBER_ID) 
};

// @appendable
// @nested
class DDS_TYPE_API _Module {
public:
    bool operator<(const _Module& rhs) const {
        return false;
    }
    std::string name;
    TypeLibrary* library;// @shared 
};

// @bit_bound()
enum TypeLibraryElementKind {
    ALIAS_TYPE_ELEMENT = TRAVODDS::ALIAS_TYPE,
    ANNOTATION_TYPE_ELEMENT = TRAVODDS::ANNOTATION_TYPE,
    ARRAY_TYPE_ELEMENT = TRAVODDS::ARRAY_TYPE,
    BITSET_TYPE_ELEMENT = TRAVODDS::BITSET_TYPE,
    ENUMERATION_TYPE_ELEMENT = TRAVODDS::ENUMERATION_TYPE,
    MAP_TYPE_ELEMENT = TRAVODDS::MAP_TYPE,
    SEQUENCE_TYPE_ELEMENT = TRAVODDS::SEQUENCE_TYPE,
    STRING_TYPE_ELEMENT = TRAVODDS::STRING_TYPE,
    STRUCTURE_TYPE_ELEMENT = TRAVODDS::STRUCTURE_TYPE,
    UNION_TYPE_ELEMENT = TRAVODDS::UNION_TYPE,
    MODULE_ELEMENT = 24,
};

// @mutable
// @nested
class DDS_TYPE_API TypeLibraryElement {
public:
    TRAVODDS::TypeLibraryElementKind u_;
    TRAVODDS::AliasType alias_type;// @id(1) 
    TRAVODDS::AnnotationType annotation_type;// @id(2) 
    TRAVODDS::ArrayType array_type;// @id(3) 
    TRAVODDS::BitSetType bitset_type;// @id(4) 
    TRAVODDS::EnumerationType enumeration_type;// @id(5) 
    TRAVODDS::MapType map_type;// @id(6) 
    TRAVODDS::SequenceType sequence_type;// @id(7) 
    TRAVODDS::StringType string_type;// @id(8) 
    TRAVODDS::StructureType structure_type;// @id(9) 
    TRAVODDS::UnionType union_type;// @id(10) 
    TRAVODDS::_Module mod;// @id(11) 
};

// @appendable
// @nested
class DDS_TYPE_API TypeLibrary {
public:
    bool operator<(const TypeLibrary& rhs) const {
        return false;
    }
    std::vector<TRAVODDS::TypeLibraryElement> element;
};

// @mutable
class DDS_TYPE_API TypeObject {
public:
    bool operator<(const TypeObject& rhs) const {
        return false;
    }
    TRAVODDS::TypeLibrary library;// @id(0) 
    std::vector<TRAVODDS::_TypeId> the_type;// @id(1) 
};

} // namespace TRAVODDS

#ifdef __cplusplus
}
#endif
#endif /* typeobject_H */
