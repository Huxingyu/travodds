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

#ifndef TYPE_OBJECT_H
#define TYPE_OBJECT_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "cdr/agicdr.h"

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

const PrimitiveTypeId NO_TYPE_ID = NO_TYPE;

const PrimitiveTypeId BOOLEAN_TYPE_ID = BOOLEAN_TYPE;

const PrimitiveTypeId BYTE_TYPE_ID = BYTE_TYPE;

const PrimitiveTypeId INT_16_TYPE_ID = INT_16_TYPE;

const PrimitiveTypeId UINT_16_TYPE_ID = UINT_16_TYPE;

const PrimitiveTypeId INT_32_TYPE_ID = INT_32_TYPE;

const PrimitiveTypeId UINT_32_TYPE_ID = UINT_32_TYPE;

const PrimitiveTypeId INT_64_TYPE_ID = INT_64_TYPE;

const PrimitiveTypeId UINT_64_TYPE_ID = UINT_64_TYPE;

const PrimitiveTypeId FLOAT_32_TYPE_ID = FLOAT_32_TYPE;

const PrimitiveTypeId FLOAT_64_TYPE_ID = FLOAT_64_TYPE;

const PrimitiveTypeId FLOAT_128_TYPE_ID = FLOAT_128_TYPE;

const PrimitiveTypeId CHAR_8_TYPE_ID = CHAR_8_TYPE;

const PrimitiveTypeId CHAR_32_TYPE_ID = CHAR_32_TYPE;

class DDS_TYPE_API _TypeId {
public:
    TypeKind u_;
    PrimitiveTypeId primitive_type_id;
    unsigned long long constructed_type_id;
};

typedef unsigned int MemberId;

const MemberId MEMBER_ID_INVALID = 268435455;

// @appendable
// @nested
class DDS_TYPE_API AnnotationMemberValue {
public:
    TypeKind u_;
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
    bool operator<(const AnnotationUsageMember& ) const {
        return false;
    }
    MemberId member_id;
    AnnotationMemberValue value;
};

// @appendable
// @nested
class DDS_TYPE_API AnnotationUsage {
public:
    bool operator<(const AnnotationUsage& ) const {
        return false;
    }
    _TypeId type_id;
    std::vector<AnnotationUsageMember> member;
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
    bool operator<(const TypeProperty& ) const {
        return false;
    }
    TypeFlag flag;
    _TypeId type_id;
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
    bool operator<(const Type& ) const {
        return false;
    }
    TypeProperty property;// @id(PROPERTY_TYPE_MEMBER_ID) 
    std::vector<AnnotationUsage> annotation;// @id(ANNOTATION_TYPE_MEMBER_ID) 
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
    bool operator<(const MemberProperty& ) const {
        return false;
    }
    MemberFlag flag;
    MemberId member_id;
    _TypeId type_id;
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
    bool operator<(const Member& ) const {
        return false;
    }
    MemberProperty property;// @id(PROPERTY_MEMBER_MEMBER_ID) 
    std::vector<AnnotationUsage> annotation;// @id(ANNOTATION_MEMBER_MEMBER_ID) 
};

enum StructureTypeMemberId {
    BASE_TYPE_STRUCTURETYPE_MEMBER_ID = 100,
    MEMBER_STRUCTURETYPE_MEMBER_ID = 101,
};

// @mutable
// @nested
class DDS_TYPE_API StructureType : public Type {
public:
    bool operator<(const StructureType& ) const {
        return false;
    }
    _TypeId base_type;// @id(BASE_TYPE_STRUCTURETYPE_MEMBER_ID) 
    std::vector<Member> member;// @id(MEMBER_STRUCTURETYPE_MEMBER_ID) 
};

enum UnionMemberMemberId {
    LABEL_UNIONMEMBER_MEMBER_ID = 100,
};

// @mutable
// @nested
class DDS_TYPE_API UnionMember : public Member {
public:
    bool operator<(const UnionMember& ) const {
        return false;
    }
    std::vector<int> label;// @id(LABEL_UNIONMEMBER_MEMBER_ID) 
};

enum UnionTypeMemberId {
    MEMBER_UNIONTYPE_MEMBER_ID = 100,
};

// @mutable
// @nested
class DDS_TYPE_API UnionType : public Type {
public:
    bool operator<(const UnionType& ) const {
        return false;
    }
    std::vector<UnionMember> member;// @id(MEMBER_UNIONTYPE_MEMBER_ID) 
};

enum AnnotationMemberMemberId {
    DEFAULT_VALUE_ANNOTATIONMEMBER_MEMBER_ID = 100,
};

// @mutable
// @nested
class DDS_TYPE_API AnnotationMember : public Member {
public:
    bool operator<(const AnnotationMember& ) const {
        return false;
    }
    AnnotationMemberValue default_value;// @id(DEFAULT_VALUE_ANNOTATIONMEMBER_MEMBER_ID) 
};

enum AnnotationTypeMemberId {
    BASE_TYPE_ANNOTATIONTYPE_MEMBER_ID = 100,
    MEMBER_ANNOTATIONTYPE_MEMBER_ID = 101,
};

// @mutable
// @nested
class DDS_TYPE_API AnnotationType : public Type {
public:
    bool operator<(const AnnotationType& ) const {
        return false;
    }
    _TypeId base_type;// @id(BASE_TYPE_ANNOTATIONTYPE_MEMBER_ID) 
    std::vector<AnnotationMember> member;// @id(MEMBER_ANNOTATIONTYPE_MEMBER_ID) 
};

enum AliasTypeMemberId {
    BASE_TYPE_ALIASTYPE_MEMBER_ID = 100,
};

// @mutable
// @nested
class DDS_TYPE_API AliasType : public Type {
public:
    bool operator<(const AliasType& ) const {
        return false;
    }
    _TypeId base_type;// @id(BASE_TYPE_ALIASTYPE_MEMBER_ID) 
};

typedef unsigned int Bound;

const Bound UNBOUNDED_COLLECTION = 0;

enum CollectionTypeMemberId {
    ELEMENT_TYPE_COLLECTIONTYPE_MEMBER_ID = 100,
    ELEMENT_SHARED_COLLECTIONTYPE_MEMBER_ID = 101,
};

// @mutable
// @nested
class DDS_TYPE_API CollectionType : public Type {
public:
    bool operator<(const CollectionType& ) const {
        return false;
    }
    _TypeId element_type;// @id(ELEMENT_TYPE_COLLECTIONTYPE_MEMBER_ID) 
    char element_shared;// @id(ELEMENT_SHARED_COLLECTIONTYPE_MEMBER_ID) 
};

enum ArrayTypeMemberId {
    BOUND_ARRAYTYPE_MEMBER_ID = 200,
};

// @mutable
// @nested
class DDS_TYPE_API ArrayType : public CollectionType {
public:
    bool operator<(const ArrayType& ) const {
        return false;
    }
    std::vector<Bound> bound;// @id(BOUND_ARRAYTYPE_MEMBER_ID) 
};

enum MapTypeMemberId {
    KEY_ELEMENT_TYPE_MAPTYPE_MEMBER_ID = 200,
    BOUND_MAPTYPE_MEMBER_ID = 201,
};

// @appendable
// @nested
class DDS_TYPE_API MapType : public CollectionType {
public:
    bool operator<(const MapType& ) const {
        return false;
    }
    _TypeId key_element_type;// @id(KEY_ELEMENT_TYPE_MAPTYPE_MEMBER_ID) 
    Bound bound;// @id(BOUND_MAPTYPE_MEMBER_ID) 
};

enum SequenceTypeMemberId {
    BOUND_SEQUENCETYPE_MEMBER_ID = 200,
};

// @appendable
// @nested
class DDS_TYPE_API SequenceType : public CollectionType {
public:
    bool operator<(const SequenceType& ) const {
        return false;
    }
    Bound bound;// @id(BOUND_SEQUENCETYPE_MEMBER_ID) 
};

enum StringTypeMemberId {
    BOUND_STRINGTYPE_MEMBER_ID = 200,
};

// @appendable
// @nested
class DDS_TYPE_API StringType : public CollectionType {
public:
    bool operator<(const StringType& ) const {
        return false;
    }
    Bound bound;// @id(BOUND_STRINGTYPE_MEMBER_ID) 
};

// @appendable
// @nested
class DDS_TYPE_API Bit {
public:
    bool operator<(const Bit& ) const {
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
class DDS_TYPE_API BitSetType : public Type {
public:
    bool operator<(const BitSetType& ) const {
        return false;
    }
    Bound bit_bound;// @id(BIT_BOUND_BITSETTYPE_MEMBER_ID) 
    std::vector<Bit> bit;// @id(BIT_BITSETTYPE_MEMBER_ID) 
};

// @appendable
// @nested
class DDS_TYPE_API EnumeratedConstant {
public:
    bool operator<(const EnumeratedConstant& ) const {
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
class DDS_TYPE_API EnumerationType : public Type {
public:
    bool operator<(const EnumerationType& ) const {
        return false;
    }
    Bound bit_bound;// @id(BIT_BOUND_ENUMERATIONTYPE_MEMBER_ID) 
    std::vector<EnumeratedConstant> constant;// @id(CONSTANT_ENUMERATIONTYPE_MEMBER_ID) 
};

// @appendable
// @nested
class DDS_TYPE_API _Module {
public:
    bool operator<(const _Module& ) const {
        return false;
    }
    std::string name;
    TypeLibrary* library;// @shared 
};

// @bit_bound()
enum TypeLibraryElementKind {
    ALIAS_TYPE_ELEMENT = ALIAS_TYPE,
    ANNOTATION_TYPE_ELEMENT = ANNOTATION_TYPE,
    ARRAY_TYPE_ELEMENT = ARRAY_TYPE,
    BITSET_TYPE_ELEMENT = BITSET_TYPE,
    ENUMERATION_TYPE_ELEMENT = ENUMERATION_TYPE,
    MAP_TYPE_ELEMENT = MAP_TYPE,
    SEQUENCE_TYPE_ELEMENT = SEQUENCE_TYPE,
    STRING_TYPE_ELEMENT = STRING_TYPE,
    STRUCTURE_TYPE_ELEMENT = STRUCTURE_TYPE,
    UNION_TYPE_ELEMENT = UNION_TYPE,
    MODULE_ELEMENT = 24,
};

// @mutable
// @nested
class DDS_TYPE_API TypeLibraryElement {
public:
    TypeLibraryElementKind u_;
    AliasType alias_type;// @id(1) 
    AnnotationType annotation_type;// @id(2) 
    ArrayType array_type;// @id(3) 
    BitSetType bitset_type;// @id(4) 
    EnumerationType enumeration_type;// @id(5) 
    MapType map_type;// @id(6) 
    SequenceType sequence_type;// @id(7) 
    StringType string_type;// @id(8) 
    StructureType structure_type;// @id(9) 
    UnionType union_type;// @id(10) 
    _Module mod;// @id(11) 
};

// @appendable
// @nested
class DDS_TYPE_API TypeLibrary {
public:
    bool operator<(const TypeLibrary& ) const {
        return false;
    }
    std::vector<TypeLibraryElement> element;
};

// @mutable
class DDS_TYPE_API TypeObject {
public:
    bool operator<(const TypeObject& ) const {
        return false;
    }
    TypeLibrary library;// @id(0) 
    std::vector<_TypeId> the_type;// @id(1) 
};

} // namespace AGIDDS

#ifdef __cplusplus
}
#endif
#endif /* TYPE_OBJECT_H */
