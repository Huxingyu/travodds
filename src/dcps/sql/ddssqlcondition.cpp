#include "dcps/sql/ddssqlcondition.h"

using namespace TRAVODDS::DDSSQL;

DDSSQLLogicalCondition::DDSSQLLogicalCondition(LogicalOperationKind op, const std::shared_ptr<DDSSQLCondition>& left, const std::shared_ptr<DDSSQLCondition>& right)
	: op_(op), left_(left), right_(right)
{
	assert(left_);
	assert(right_ || LogicalOperationKind::NOT == op_);
	left_->SetParent(this);
	if (LogicalOperationKind::NOT != op_ && right_) {
		right_->SetParent(this);
	}

}

void DDSSQLLogicalCondition::ChildHasChanged()
{
	bool lresult = left_->GetResult();
	bool rresult = true;
	if (LogicalOperationKind::NOT != op_) {
		rresult = right_->GetResult();
	}
	switch (op_) {
	case LogicalOperationKind::NOT:
		SetResult(!lresult);
		break;
	case LogicalOperationKind::AND: {
		if (lresult && rresult) {
			SetResult(true);
		} else {
			SetResult(false);
		}
		break;
	}
	case LogicalOperationKind::OR: {
		if (lresult || rresult) {
			SetResult(true);
		} else {
			SetResult(false);
		}
		break;
	}
	default:
		assert(false);
	}
}

void DDSSQLLogicalCondition::ChildReset() noexcept
{
	left_->Reset();
	if (right_) {
		right_->Reset();
	}
}
