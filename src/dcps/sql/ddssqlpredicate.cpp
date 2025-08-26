#include "dcps/sql/ddssqlpredicate.h"

using namespace TRAVODDS::DDSSQL;

DDSSQLPredicate::DDSSQLPredicate(RelOperationKind op, const std::shared_ptr<TypicalValue>& left, const std::shared_ptr<TypicalValue>& right)
    : op_(op), left_(left), right_(right)
{
    left_->AssociatedPredicate(this);
    right_->AssociatedPredicate(this);

    if (RelOperationKind::LIKE == op_) {
        right_->MakeRegex();
    }
}

void DDSSQLPredicate::Check()
{
    if (left_->HasValue() && right_->HasValue()) {
        switch (op_)
        {
        case RelOperationKind::EQUAL:
            SetResult(*left_ == *right_);
            break;

        case RelOperationKind::NOT_EQUAL:
            SetResult(*left_ != *right_);
            break;

        case RelOperationKind::LESS_THAN:
            SetResult(*left_ < *right_);
            break;

        case RelOperationKind::LESS_EQUAL:
            SetResult(*left_ <= *right_);
            break;

        case RelOperationKind::GREATER_THAN:
            SetResult(*left_ > *right_);
            break;

        case RelOperationKind::GREATER_EQUAL:
            SetResult(*left_ >= *right_);
            break;

        case RelOperationKind::LIKE:
            SetResult(left_->IsLike(*right_));
            break;

        default:
            assert(false);
        }
    }
}

void DDSSQLPredicate::ChildReset() noexcept
{
    left_->Reset();
    right_->Reset();
}
