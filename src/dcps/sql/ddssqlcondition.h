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
#ifndef TRAVODDS_DCPS_SQL_CONDITION_H
#define TRAVODDS_DCPS_SQL_CONDITION_H

#include <vector>
#include "dcps/sql/ddssqlfield.h"
#include "dcps/sql/binarytreenode.h"

TRAVODDS_NAMESPACE_BEGIN
namespace DDSSQL {

    enum DDSSQLConditionResult {
        RESULT_UNSOLVED = 0,
        RESULT_TRUE = 1,
        RESULT_FALSE = 2  
    };
	class DDSSQLCondition
	{
	public:
        // DDSSQLLogicalCondition的子对象调用SetParent
        inline void SetParent(DDSSQLCondition* parent) noexcept {
            parent_ = parent;
        }
        virtual void ChildHasChanged() {
        }
        inline void Reset() noexcept {
            result_ = RESULT_UNSOLVED;
            ChildReset();
        }

        virtual void ChildReset() noexcept = 0;

        inline void SetResult(bool result) noexcept {
			DDSSQLConditionResult ret = result ? RESULT_TRUE : RESULT_FALSE;
            if (ret != result_) {
                result_ = ret;
                if (nullptr != parent_) {
                    parent_->ChildHasChanged();
                }
            }
        }

        inline bool GetResult() const noexcept {
            return result_ == RESULT_TRUE ? true : false;
        }
    private:
        DDSSQLCondition* parent_ = nullptr;

        // 
        DDSSQLConditionResult result_ = RESULT_UNSOLVED;
	};

    enum class LogicalOperationKind : uint8_t
    {
        NOT,
        AND,
        OR
    };

    class DDSSQLLogicalCondition : public DDSSQLCondition
    {
    public:
        DDSSQLLogicalCondition(
            LogicalOperationKind op,
            const std::shared_ptr<DDSSQLCondition>& left,
            const std::shared_ptr<DDSSQLCondition>& right);

        void ChildHasChanged() override;

        void ChildReset() noexcept override;
    private:
        LogicalOperationKind op_;
        std::shared_ptr<DDSSQLCondition> left_;
        std::shared_ptr<DDSSQLCondition> right_;
    };
}
TRAVODDS_NAMESPACE_END

#endif
