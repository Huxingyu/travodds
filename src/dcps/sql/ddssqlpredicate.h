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
#ifndef AGIDDS_DCPS_SQL_PREDICATE_H
#define AGIDDS_DCPS_SQL_PREDICATE_H

#include <vector>
#include <memory>
#include "dcps/sql/ddssqlfield.h"
#include "dcps/sql/ddssqlcondition.h"

AGIDDS_NAMESPACE_BEGIN
namespace DDSSQL {

    enum class RelOperationKind : uint8_t
    {
        EQUAL,          ///< left = right
        NOT_EQUAL,      ///< left <> right
        LESS_THAN,      ///< left < right
        LESS_EQUAL,     ///< left <= right
        GREATER_THAN,   ///< left > right
        GREATER_EQUAL,  ///< left >= right
        LIKE            ///< left LIKE right
    };

	class DDSSQLPredicate : public DDSSQLCondition
	{
	public:
		DDSSQLPredicate(
            RelOperationKind op,
            const std::shared_ptr<TypicalValue>& left,
            const std::shared_ptr<TypicalValue>& right);
        virtual ~DDSSQLPredicate() = default;

        void Check();

        void ChildReset() noexcept override;
    private:
        RelOperationKind op_;

        std::shared_ptr<TypicalValue> left_;

        std::shared_ptr<TypicalValue> right_;

	};
}
AGIDDS_NAMESPACE_END

#endif
