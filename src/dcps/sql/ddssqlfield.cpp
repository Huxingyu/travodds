#include "dcps/sql/ddssqlfield.h"
#include "dcps/sql/ddssqlpredicate.h"
#include "xtypes/dynamicdata.h"
#include <stdexcept> 
#include <cassert> 
using namespace TRAVODDS::DDSSQL;

template<typename T>
int CompareValues(
    T lvalue,
    T rvalue)
{
    return lvalue < rvalue ? -1 :
        lvalue > rvalue ? 1 : 0;
}

static bool IsNegative(const TypicalValue& value)
{
    switch (value.kind) {
    case TypicalKind::BOOLEAN:
        return false;
    case TypicalKind::ENUM:
    case TypicalKind::SIGNED_INTEGER:
        return value.signedIntegerValue < 0;
    case TypicalKind::CHAR:
        return value.charValue < 0;
    case TypicalKind::FLOAT:
    case TypicalKind::DOUBLE:
    case TypicalKind::LONG_DOUBLE: {
        return value.floatValue < 0;
    }
    default:
        assert(false);
    }

    return false;
}


static int64_t ToSignedInteger(
    const TypicalValue& value)
{
    switch (value.kind) {
    case TypicalKind::BOOLEAN:
        return value.booleanValue ? 1 : 0;
    case TypicalKind::ENUM:
        return value.signedIntegerValue;
    case TypicalKind::CHAR:
        return value.charValue;
    case TypicalKind::FLOAT:
    case TypicalKind::DOUBLE:
    case TypicalKind::LONG_DOUBLE: {
        constexpr long double int64_min = static_cast<long double>(std::numeric_limits<int64_t>::min()) - 1.0L;
        constexpr long double int64_max = static_cast<long double>(std::numeric_limits<int64_t>::max()) + 1.0L;
        if (value.floatValue <= int64_min || value.floatValue >= int64_max) {
            assert(false);
        }

        return static_cast<int64_t>(value.floatValue);
    }
    default:
        assert(false);
    }

    return 0;
}

static uint64_t ToUnsignedInteger(
    const TypicalValue& value)
{
    switch (value.kind) {
    case TypicalKind::BOOLEAN:
        return value.booleanValue ? 1 : 0;

    case TypicalKind::ENUM:
    case TypicalKind::SIGNED_INTEGER:
        return static_cast<uint64_t>(value.signedIntegerValue);
    case TypicalKind::CHAR:
        return static_cast<uint64_t>(value.charValue);
    case TypicalKind::FLOAT:
    case TypicalKind::DOUBLE:
    case TypicalKind::LONG_DOUBLE: {
        constexpr long double uint64_max = static_cast<long double>(std::numeric_limits<uint64_t>::max());
        if (value.floatValue < 0 || value.floatValue > uint64_max) {
            assert(false);
        }
        return static_cast<uint64_t>(value.floatValue);
    }

    default:
        assert(false);
    }

    return 0;
}

static long double ToFloat(
    const TypicalValue& value)
{
    switch (value.kind) {
    case TypicalKind::ENUM:
    case TypicalKind::SIGNED_INTEGER:
        return static_cast<long double>(value.signedIntegerValue);
    case TypicalKind::UNSIGNED_INTEGER:
        return static_cast<long double>(value.unsignedIntegerValue);
    case TypicalKind::CHAR:
        return static_cast<long double>(value.charValue);
    case TypicalKind::FLOAT:
    case TypicalKind::DOUBLE:
    case TypicalKind::LONG_DOUBLE:
        return value.floatValue;
    default:
        assert(false);
    }

    return 0;
}

DDSSQLFieldValue::DDSSQLFieldValue(std::vector<FieldAccessor>& accessors)
    : accessors_(std::move(accessors))
{
}

void TypicalValue::Copy(const TypicalValue* other)
{
    if (nullptr == other) {
		throw std::runtime_error("Null pointer passed to Copy method.");
    }
    kind = other->kind;
    switch (kind)
    {
    case TypicalKind::BOOLEAN:
        booleanValue = other->booleanValue;
        break;

    case TypicalKind::CHAR:
        charValue = other->charValue;
        break;

    case TypicalKind::ENUM:
    case TypicalKind::SIGNED_INTEGER:
        signedIntegerValue = other->signedIntegerValue;
        break;

    case TypicalKind::UNSIGNED_INTEGER:
        unsignedIntegerValue = other->unsignedIntegerValue;
        break;

    case TypicalKind::FLOAT:
    case TypicalKind::DOUBLE:
    case TypicalKind::LONG_DOUBLE:
        floatValue = other->floatValue;
        break;

    case TypicalKind::STRING:
        stringValue = other->stringValue;
        break;

    default:
		// 抛出异常或处理未知类型
        throw std::runtime_error("Unknown TypicalKind encountered in Copy method.");
        break;
    }

}

bool TypicalValue::IsLike(const TypicalValue& other) const
{
    if (!other.regex_) {
        throw std::runtime_error("Regex not initialized for TypicalValue.");
    }
    std::string expr;

    switch (kind)
    {
    case CHAR:
        expr = charValue;
        break;

    case STRING:
        expr = stringValue.data();
        break;

    default:
        throw std::runtime_error("Cannot perform regex match on non-string value.");
    }

    return std::regex_match(expr, *other.regex_);
}

void TypicalValue::MakeRegex()
{
    likeRight_ = true;
    if (HasValue()) {
        ValueHasChanged();
    }
}

void TypicalValue::ValueHasChanged()
{
    if (likeRight_) {
        std::string expr;

        switch (kind)
        {
        case TypicalKind::CHAR:
            expr = charValue;
            break;

        case TypicalKind::STRING:
            expr = stringValue.data();
            break;

        default:
            throw std::runtime_error("Cannot create regex from non-string value.");
        }

        expr = std::regex_replace(expr, std::regex("\\*"), ".*");
        expr = std::regex_replace(expr, std::regex("\\?"), ".");
        expr = std::regex_replace(expr, std::regex("%"), ".*");
        expr = std::regex_replace(expr, std::regex("_"), ".");

        regex_.reset(new std::regex(expr));
    }
}

int TypicalValue::Compare(const TypicalValue& lhs, const TypicalValue& rhs)
{
    if (lhs.kind == rhs.kind) {
        switch (lhs.kind) {
        case TypicalKind::BOOLEAN:
            return lhs.booleanValue - rhs.booleanValue;

        case TypicalKind::CHAR:
            return lhs.charValue - rhs.charValue;

        case TypicalKind::SIGNED_INTEGER:
        case TypicalKind::ENUM:
            return CompareValues(lhs.signedIntegerValue, rhs.signedIntegerValue);

        case TypicalKind::UNSIGNED_INTEGER:
            return CompareValues(lhs.unsignedIntegerValue, rhs.unsignedIntegerValue);

        case TypicalKind::FLOAT:
            return CompareValues(static_cast<float>(lhs.floatValue), static_cast<float>(rhs.floatValue));

        case TypicalKind::DOUBLE:
            return CompareValues(static_cast<double>(lhs.floatValue), static_cast<double>(rhs.floatValue));

        case TypicalKind::LONG_DOUBLE:
            return CompareValues(lhs.floatValue, rhs.floatValue);

        case TypicalKind::STRING:
            return std::strcmp(lhs.stringValue.data(), rhs.stringValue.data());
        default:
            assert(false);
        }
    } else if (lhs.kind < rhs.kind) {
        return -Compare(rhs, lhs);
    } else {
        switch (lhs.kind) {
        case TypicalKind::ENUM:
        case TypicalKind::SIGNED_INTEGER: {
            return CompareValues(lhs.signedIntegerValue, ToSignedInteger(rhs));
        }

        case TypicalKind::UNSIGNED_INTEGER:
            return IsNegative(rhs) ? 1 : CompareValues(lhs.unsignedIntegerValue, ToUnsignedInteger(rhs));

        case TypicalKind::FLOAT:
            return CompareValues(static_cast<float>(lhs.floatValue), static_cast<float>(ToFloat(rhs)));

        case TypicalKind::DOUBLE:
            return CompareValues(static_cast<double>(lhs.floatValue), static_cast<double>(ToFloat(rhs)));

        case TypicalKind::LONG_DOUBLE:
            return CompareValues(lhs.floatValue, ToFloat(rhs));

        case TypicalKind::STRING: {
            assert(TypicalKind::CHAR == rhs.kind);
            return std::strcmp(lhs.stringValue.data(), rhs.stringValue.data());
        }

        case TypicalKind::BOOLEAN:
        case TypicalKind::CHAR:
            return CompareValues(ToSignedInteger(lhs), ToSignedInteger(rhs));
        default:
            assert(false);
            break;
        }
    }

    return 0;
}

void DDSSQLFieldValue::Copy(const TypicalValue* other)
{
    if (nullptr == dynamic_cast<const DDSSQLFieldValue*>(other)) {
		throw std::runtime_error("Type mismatch in Copy method of DDSSQLFieldValue.");
        return;
    }
	kind = other->kind;
    accessors_ = dynamic_cast<const DDSSQLFieldValue*>(other)->accessors_;
}

bool DDSSQLFieldValue::SetValue(DynamicData* dynData, DynamicType* dynType)
{
    auto itor = accessors_.begin();
    if (!GetMemberValue(
        dynData, dynType,
        itor, accessors_.end())) {
        return false;
    }
    hasValue_ = true;
    ValueHasChanged();
    for (DDSSQLPredicate* predicate : predicates_) {
        predicate->Check();
    }

    return true;
}

bool DDSSQLFieldValue::SetSeqValue(DynamicData* memberData, MemberId memberId, TypeKind type, int32_t elementIndex)
{
    switch (type) {
    case TypeKind::TK_BOOLEAN: {
        DynamicData::BooleanSeq boolseq;
        memberData->get_boolean_values(boolseq, memberId);
        kind = BOOLEAN;
        booleanValue = boolseq[elementIndex];
        return true;
    }
    case TypeKind::TK_CHAR8: {
        DynamicData::CharSeq charseq;
        memberData->get_char8_values(charseq, memberId);
        kind = CHAR;
        charValue = charseq[elementIndex];
        return true;
    }
    case TypeKind::TK_INT16: {
        DynamicData::Int16Seq intSeq;
        memberData->get_int16_values(intSeq, memberId);
        kind = SIGNED_INTEGER;
        signedIntegerValue = intSeq[elementIndex];
        return true;
    }
    case TypeKind::TK_ENUM:
    case TypeKind::TK_INT32: {
        DynamicData::Int32Seq intSeq;
        memberData->get_int32_values(intSeq, memberId);
        kind = type == TypeKind::TK_INT32 ? SIGNED_INTEGER : ENUM;
        signedIntegerValue = intSeq[elementIndex];
        return true;
    }
    case TypeKind::TK_INT64: {
        DynamicData::Int64Seq intSeq;
        memberData->get_int64_values(intSeq, memberId);
        kind = SIGNED_INTEGER;
        signedIntegerValue = intSeq[elementIndex];
        return true;
    }
    case TypeKind::TK_UINT16: {
        DynamicData::UInt16Seq uintSeq;
        memberData->get_uint16_values(uintSeq, memberId);
        kind = UNSIGNED_INTEGER;
        unsignedIntegerValue = uintSeq[elementIndex];
        return true;
    }
    case TypeKind::TK_UINT32: {
        DynamicData::UInt32Seq uintSeq;
        memberData->get_uint32_values(uintSeq, memberId);
        kind = UNSIGNED_INTEGER;
        unsignedIntegerValue = uintSeq[elementIndex];
        return true;
    }
    case TypeKind::TK_UINT64: {
        DynamicData::UInt64Seq uintSeq;
        memberData->get_uint64_values(uintSeq, memberId);
        kind = UNSIGNED_INTEGER;
        unsignedIntegerValue = uintSeq[elementIndex];
        return true;
    }
    case TypeKind::TK_FLOAT32: {
        DynamicData::Float32Seq floatSeq;
        memberData->get_float32_values(floatSeq, memberId);
        kind = FLOAT;
        floatValue = floatSeq[elementIndex];
        return true;
    }
    case TypeKind::TK_FLOAT64: {
        DynamicData::Float64Seq floatSeq;
        memberData->get_float64_values(floatSeq, memberId);
        kind = DOUBLE;
        floatValue = floatSeq[elementIndex];
        return true;
    }
    case TypeKind::TK_FLOAT128: {
        DynamicData::Float128Seq floatSeq;
        memberData->get_float128_values(floatSeq, memberId);
        kind = LONG_DOUBLE;
        floatValue = floatSeq[elementIndex];
        return true;
    }
    case TypeKind::TK_STRING8: {
        DynamicData::StringSeq strSeq;
        memberData->get_string_values(strSeq, memberId);
        kind = STRING;
        size_t strLen = strSeq[elementIndex].size();
        if (strLen > BYTE_ORDER_LENGTH_MAX) {
            return false;
        }
        std::copy(strSeq[elementIndex].begin(), strSeq[elementIndex].begin() + strLen, stringValue.begin());
        return true;
    }
    default:
        return false;
    }
    return false;
}

bool DDSSQLFieldValue::SetBaseValue(DynamicData* memberData, MemberId memberId, TypeKind type)
{
    switch (type) {
    case TypeKind::TK_BOOLEAN: {
        bool value;
        memberData->get_boolean_value(value, memberId);
        kind = BOOLEAN;
        booleanValue = booleanValue;
        return true;
    }
    case TypeKind::TK_CHAR8: {
        char value;
        memberData->get_char8_value(value, memberId);
        kind = CHAR;
        charValue = value;
        return true;
    }
    case TypeKind::TK_INT16: {
        int16_t value;
        memberData->get_int16_value(value, memberId);
        kind = SIGNED_INTEGER;
        signedIntegerValue = value;
        return true;
    }
    case TypeKind::TK_ENUM:
    case TypeKind::TK_INT32: {
        long value;
        memberData->get_int32_value(value, memberId);
        kind = type == TypeKind::TK_INT32 ? SIGNED_INTEGER : ENUM;
        signedIntegerValue = value;
        return true;
    }
    case TypeKind::TK_INT64: {
        long long value;
        memberData->get_int64_value(value, memberId);
        kind = SIGNED_INTEGER;
        signedIntegerValue = value;
        return true;
    }
    case TypeKind::TK_UINT16: {
        uint16_t value;
        memberData->get_uint16_value(value, memberId);
        kind = UNSIGNED_INTEGER;
        unsignedIntegerValue = value;
        return true;
    }
    case TypeKind::TK_UINT32: {
        unsigned long value;
        memberData->get_uint32_value(value, memberId);
        kind = UNSIGNED_INTEGER;
        unsignedIntegerValue = value;
        return true;
    }
    case TypeKind::TK_UINT64: {
        unsigned long long value;
        memberData->get_uint64_value(value, memberId);
        kind = UNSIGNED_INTEGER;
        unsignedIntegerValue = value;
        return true;
    }
    case TypeKind::TK_FLOAT32: {
        float value;
        memberData->get_float32_value(value, memberId);
        kind = FLOAT;
        floatValue = value;
        return true;
    }
    case TypeKind::TK_FLOAT64: {
        double value;
        memberData->get_float64_value(value, memberId);
        kind = DOUBLE;
        floatValue = value;
        return true;
    }
    case TypeKind::TK_FLOAT128: {
        long double value;
        memberData->get_float128_value(value, memberId);
        kind = LONG_DOUBLE;
        floatValue = value;
        return true;
    }
    case TypeKind::TK_STRING8: {
        std::string value;
        memberData->get_string_value(value, memberId);
        kind = STRING;
        size_t strLen = value.size();
        if (strLen > BYTE_ORDER_LENGTH_MAX) {
            return false;
        }
        std::copy(value.begin(), value.begin() + strLen, stringValue.begin());
        return true;
    }
    default:
        return false;
    }
    return false;
}

bool DDSSQLFieldValue::GetMemberValue(DynamicData* dynData, DynamicType* dynType, std::vector<FieldAccessor>::iterator& itor, const std::vector<FieldAccessor>::iterator& end)
{
    // 终止条件：已经处理完所有访问器
    if (itor == end) {
        return true;
    }
    FieldAccessor& accessor = *itor;

    // 获取成员ID
    MemberId memberId;
    if (!accessor.memberName.empty()) {
        memberId = dynData->get_member_id_by_name(accessor.memberName);
    } else if (accessor.memberIndex >= 0) {
        memberId = dynData->get_member_id_at_index(accessor.memberIndex);
    } else {
        return false;
    }
    if (memberId == MEMBER_ID_INVALID) {
        return false;
    };

    // 获取成员类型信息
    DynamicTypeMember member;
    if (dynType->get_member(member, memberId) != RETCODE_OK) {
        return false;
    }
    MemberDescriptor memberDescriptor;
    member.get_descriptor(memberDescriptor);
    DynamicType* memberType = memberDescriptor.type_;
    TypeKind memberKind = memberType->get_kind();

    // 处理数组 / 序列索引
    if (accessor.arrayIndex.size() > 0) {
        if (memberKind != TypeKind::TK_ARRAY && memberKind != TypeKind::TK_SEQUENCE) {
            return false;
        }
        // 检查索引是否有效
        // 检查维度是否一致
        if (accessor.arrayIndex.size() != memberType->descriptor_.bound_.size()) {
            return false;
        }
        //检查各个维度,并且将其转化一维index
        int32_t elementIndex = 0;
        for (int32_t i = 0; i < accessor.arrayIndex.size(); i++) {
            if (memberType->descriptor_.bound_[i] < 0 || accessor.arrayIndex[i] >= memberType->descriptor_.bound_[i]) {
                return false;
            } 
            int32_t product = 1;
            for (int32_t j = i + 1; j < memberType->descriptor_.bound_.size(); ++j) {
                product *= memberType->descriptor_.bound_[j];
            }
            elementIndex += accessor.arrayIndex[i] * product;
        }

        DynamicType* elementType = memberType->descriptor_.element_type_;

        if (std::next(itor) == end) {
            return SetSeqValue(dynData, memberId, elementType->get_kind(),elementIndex);
        } else {
            if (elementType->get_kind() != TypeKind::TK_STRUCTURE) {
                return false;
            }
            DynamicDataSeq structseq;
            dynData->get_complex_values(structseq, memberId);
            // 递归处理下一个访问器
            ++itor;
            return GetMemberValue(structseq[elementIndex], &structseq[elementIndex]->get_type(), itor, end);
        }
        
    } else {// 处理非数组/序列成员
        // 如果是基本类型
        if (std::next(itor) == end) {
            return SetBaseValue(dynData, memberId, memberKind);
        } else { // 如果是复杂类型（结构体、联合体等） 
            //TODO: 联合体
            if (memberKind != TypeKind::TK_STRUCTURE) {
                return false;
            }
            // 递归处理下一个访问器
            DynamicData nextData(*memberType);
			dynData->get_complex_value(nextData, memberId);
            ++itor;
            return GetMemberValue(&nextData, memberType, itor, end);
            }
        }

    return false;
}


