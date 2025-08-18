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
#ifndef AGIDDS_DCPS_SQL_FILTER_H
#define AGIDDS_DCPS_SQL_FILTER_H

#include <vector>
#include <memory>
#include <string>
#include "cdr/agicdr.h"
#include "dcps/base/retcode.h"
#include "dcps/sql/ddssqlfield.h"
#include "dcps/sql/binarytreenode.h"
#include "dcps/sql/ddssqlcondition.h"
#include "dcps/sql/ddssqlpredicate.h"
#include "xtypes/dynamictype.h"


AGIDDS_NAMESPACE_BEGIN
class TypeObject;
namespace DDSSQL {

	class DDSSQLFilter
	{
	public:
		DDSSQLFilter(TypeObject* typeObject);

		ReturnCode_t InputSQL(const std::string& expression, const std::vector<std::string>& parameters);

		bool Validate(SerializedBuffer* buffer);
	protected:
		std::shared_ptr<DDSSQLCondition> TransformTreeToCondition(const BinaryTreeNode& node, const std::vector<std::string>& parameters);

		std::shared_ptr<DDSSQLCondition> TransformTreeToPredicate(const BinaryTreeNode& node, const std::vector<std::string>& parameters);

		std::shared_ptr<TypicalValue> TransformTreeToValue(const BinaryTreeNode& node, const std::vector<std::string>& parameters);

		// and, or, not
		std::shared_ptr<DDSSQLCondition> TransformTreeToLogical(const BinaryTreeNode& node, const std::vector<std::string>& parameters);
		// between, not betwenn
		std::shared_ptr<DDSSQLCondition> TransformTreeToBetween(const BinaryTreeNode& node, const std::vector<std::string>& parameters);

		void TransformEnum(std::shared_ptr<TypicalValue> left, std::shared_ptr<TypicalValue> right);
	protected:
		bool CheckValueCompatibility(TypicalKind left, TypicalKind right, bool ignoreEnum);

		RelOperationKind GetPredicateOp(const BinaryTreeNode& node);

	private:
		std::unique_ptr<DynamicType> dynType_;

		std::shared_ptr<DDSSQLCondition> root_; // 根节点

		// field 占位
		std::map<std::string, std::shared_ptr<DDSSQLFieldValue>> fields_;

	};
}
AGIDDS_NAMESPACE_END

#endif
