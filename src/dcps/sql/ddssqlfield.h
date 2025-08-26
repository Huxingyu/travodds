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
#ifndef AGIDDS_DCPS_SQL_FIELD_H
#define AGIDDS_DCPS_SQL_FIELD_H

#include <cstdint>
#include <array>
#include <string>
#include <unordered_set>
#include <regex>

#include "agidds_dll.h"
#include "dcps/base/retcode.h"
#include "xtypes/typekind.h"
AGIDDS_NAMESPACE_BEGIN
class DynamicData;
class DynamicType;
namespace DDSSQL {
	class DDSSQLPredicate;
    using string_255 = std::array<char, 255>;

	enum TypicalKind
	{
        BOOLEAN,            // 布尔值
        ENUM,
        SIGNED_INTEGER,
        UNSIGNED_INTEGER,
        FLOAT,
        DOUBLE,
		LONG_DOUBLE,       // 长双精度浮点数
        CHAR,
        STRING            
	};

	class TypicalValue
	{
	public:
        TypicalValue()
            : kind(STRING), stringValue{ 0 } {
        };
        virtual ~TypicalValue() = default;
        union
        {
            bool booleanValue;                  // BOOLEAN
			char charValue;                     // CHAR
            int64_t signedIntegerValue;         // SIGNED_INTEGER / ENUM
            uint64_t unsignedIntegerValue;      // UNSIGNED_INTEGER
			long double floatValue;             // FLOAT / DOUBLE / LONG_DOUBLE
            string_255 stringValue;            // STRING
        };

        TypicalKind kind;

        virtual void Copy(const TypicalValue* other);
        /* 关联Predicate */
        virtual void AssociatedPredicate(DDSSQLPredicate* predicate) {
            static_cast<void>(predicate);
        }
        virtual bool HasValue() {
            return true;
        }
        virtual void Reset() {}

        inline bool operator ==(const TypicalValue& other) const noexcept {
            return Compare(*this, other) == 0;
        }

        inline bool operator !=(const TypicalValue& other) const noexcept {
            return Compare(*this, other) != 0;
        }

        inline bool operator <(const TypicalValue& other) const noexcept {
            return Compare(*this, other) < 0;
        }

        inline bool operator <=(const TypicalValue& other) const noexcept {
            return Compare(*this, other) <= 0;
        }

        inline bool operator >(const TypicalValue& other) const noexcept {
            return Compare(*this, other) > 0;
        }

        inline bool operator >=(const TypicalValue& other) const noexcept {
            return Compare(*this, other) >= 0;
        }

        bool IsLike(const TypicalValue& other) const;

        void MakeRegex();
        void ValueHasChanged();
    private:
        // 标注是否like的右值
        bool likeRight_ = false;
        std::unique_ptr<std::regex> regex_;
        static int Compare(const TypicalValue& lhs, const TypicalValue& rhs);
	};

    /**
     * 访问最终字段路径上的一个成员变量。
     */
    struct FieldAccessor
    {
		// 成员变量的名称
		std::string memberName;
        // 要访问的成员索引
        int32_t memberIndex = -1;
        // 数组/序列成员的索引 
        std::vector<int32_t> arrayIndex;
    };

    class DDSSQLFieldValue : public TypicalValue
    {
    public:
        DDSSQLFieldValue() {};
        DDSSQLFieldValue(std::vector<FieldAccessor>& accessors);
		virtual ~DDSSQLFieldValue() = default;

        void AssociatedPredicate(DDSSQLPredicate* predicate) override {
            if (predicate) {
                predicates_.emplace(predicate);
            }
		}
        void Copy(const TypicalValue* other) override;
        void Reset() override {
            hasValue_ = false;
        }

        bool SetValue(DynamicData* dynData, DynamicType* dynType);
    protected:
        bool SetSeqValue(DynamicData* memberData, MemberId memberId,TypeKind type, int32_t elementIndex);
        bool SetBaseValue(DynamicData* memberData, MemberId memberId,TypeKind type);

        bool GetMemberValue(DynamicData* dynData, DynamicType* dynType,
            std::vector<FieldAccessor>::iterator& itor,
            const std::vector<FieldAccessor>::iterator& end);
    private:
        bool hasValue_ = false;
		/* 字段值的访问器列表，表示如何访问字段的成员变量。 */
		std::vector<FieldAccessor> accessors_;
        /* Field 关联谓词 */
        std::unordered_set<DDSSQLPredicate*> predicates_;
    };
}

AGIDDS_NAMESPACE_END

#endif