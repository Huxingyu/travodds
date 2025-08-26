#include "dcps/sql/ddssqlfilter.h"
#include "dcps/sql/ddssqlexpressionparser.h"
#include "xtypes/dynamictypebuilderfactory.h"
#include "xtypes/dynamictypebuilder.h"
#include "xtypes/dynamicdatafactory.h"
#include "xtypes/dynamicdatatypesupport.h"

USING_AGIDDS_NAMESPACE
using namespace AGIDDS::DDSSQL;
using namespace tao::pegtl;

DDSSQLFilter::DDSSQLFilter(TypeObject* typeObject)
	: root_(nullptr)
{
	DynamicTypeBuilderFactory* factory = DynamicTypeBuilderFactory::get_instance();
	// 从TypeObject创建DynamicTypeBuilder
	DynamicTypeBuilder* builder = factory->create_type_w_type_object(typeObject);

	// 构建DynamicType
	dynType_.reset(builder->build());
}

ReturnCode_t DDSSQLFilter::InputSQL(const std::string& expression, const std::vector<std::string>& parameters)
{
	try {
		memory_input<> in(expression, "");
		CurrentFieldState state{ {},dynType_.get(),nullptr};
		auto root = parse_tree::parse<FilterExpressionGrammar, BinaryTreeNode, FilterExpressionSelector>(in, state);
		if (!root) {
			return RETCODE_BAD_PARAMETER;
		}
		root_ =  TransformTreeToCondition(root->left(), parameters);
		if (!root_) {
			return RETCODE_BAD_PARAMETER;
		}
	} catch (const std::exception& e) {
		printf("Error parsing SQL expression: %s\n", e.what());
		return RETCODE_BAD_PARAMETER;
	}
	return RETCODE_OK;
}

bool DDSSQLFilter::Validate(SerializedBuffer* buffer)
{
	// 使用DynamicDataFactory创建数据容器
	DynamicData* dyn_data = DynamicDataFactory::get_instance().create_data(*dynType_);
	DynamicDataTypeSupport typeSupport(dynType_.get());
	AGIDDSCdrDeserializer cdr(buffer);
	typeSupport.deserialize_data(dyn_data, &cdr, DEFAULT_ENDIAN);

	root_->Reset();
	for (auto it = fields_.begin(); it != fields_.end() && !root_->GetResult(); ++it) {
		if (!it->second->SetValue(dyn_data, dynType_.get())) {
			return false;
		}
	}

	return !root_->GetResult();
}

std::shared_ptr<DDSSQLCondition> DDSSQLFilter::TransformTreeToCondition(const BinaryTreeNode& node, const std::vector<std::string>& parameters)
{
	if (node.is<andOp>() || node.is<orOp>() || node.is<notOp>()) {
		return TransformTreeToLogical(node, parameters);

	} else if(node.is<betweenOp>() || node.is<notBetweenOp>()) {
		return TransformTreeToBetween(node, parameters);
	}
	return TransformTreeToPredicate(node,parameters);
}

std::shared_ptr<DDSSQLCondition> DDSSQLFilter::TransformTreeToPredicate(const BinaryTreeNode& node, const std::vector<std::string>& parameters)
{
	std::shared_ptr<TypicalValue> left = TransformTreeToValue(node.left(), parameters);

	std::shared_ptr<TypicalValue> right = TransformTreeToValue(node.right(), parameters);

	bool ignoreEnum = false;
	if (node.is<likeOp>()) {
		ignoreEnum = true;
		if (((CHAR != left->kind) && (STRING != left->kind)) ||
			((CHAR != right->kind) && (STRING != right->kind))) {
				throw std::runtime_error("LIKE operation requires both sides to be CHAR or STRING.");
		}
	}

	if ((ENUM == left->kind) && (ENUM == right->kind)) {
		// TODO: Handle ENUM comparison logic here
		// throw std::runtime_error("Cannot compare two ENUM values directly.");
	} else if (!CheckValueCompatibility(left->kind, right->kind, ignoreEnum)) {
		throw std::runtime_error("Cannot compare two values directly");
	}

	TransformEnum(left, right);
	return std::make_shared<DDSSQLPredicate>(GetPredicateOp(node), left, right);
}

std::shared_ptr<TypicalValue> DDSSQLFilter::TransformTreeToValue(const BinaryTreeNode& node, const std::vector<std::string>& parameters)
{
	std::shared_ptr<TypicalValue> value = std::make_shared<TypicalValue>();
	
	if (node.is<FIELDNAME>()) {
		std::string field_name = node.content();
		auto it = fields_.find(field_name);
		if (it == fields_.end()) {
			value.reset(new DDSSQLFieldValue);
			value->Copy(node.value.get());
			fields_[field_name] = std::dynamic_pointer_cast<DDSSQLFieldValue>(value);
		} else {
			value = it->second;
		}

	} else if (node.is<PARAMETER>()) {
		if(node.parameterIndex < 0 || node.parameterIndex >= parameters.size()) {
			throw std::out_of_range("Parameter index out of range.");
		}
		pegtl::memory_input<> in(parameters[node.parameterIndex], "");

		auto parametersNode = pegtl::parse_tree::parse<BaseTypeGrammar, BinaryTreeNode, FilterExpressionSelector>(in);
        if (!parametersNode) {
			throw std::runtime_error("Failed to parse parameter expression.");
	    }
		value->Copy(parametersNode->left().value.get());
	} else {
		value->Copy(node.value.get());
	}
	return value;
}

std::shared_ptr<DDSSQLCondition> DDSSQLFilter::TransformTreeToLogical(const BinaryTreeNode& node, const std::vector<std::string>& parameters)
{
	LogicalOperationKind logicOp;
	std::shared_ptr<DDSSQLCondition> left = nullptr;
	std::shared_ptr<DDSSQLCondition> right = nullptr;

	if (node.is<notOp>()) {
		logicOp = LogicalOperationKind::NOT;
		left = TransformTreeToCondition(node.left(), parameters);
	} else if (node.is<andOp>()) {
		logicOp = LogicalOperationKind::AND;
		left = TransformTreeToCondition(node.left(), parameters);
		right = TransformTreeToCondition(node.right(), parameters);
	} else if (node.is<orOp>()) {
		logicOp = LogicalOperationKind::OR;
		left = TransformTreeToCondition(node.left(), parameters);
		right = TransformTreeToCondition(node.right(), parameters);
	} else {
		throw std::runtime_error("Invalid logical operation in filter expression.");
	}
	return std::make_shared<DDSSQLLogicalCondition>(
		logicOp, left, right
	);
}
/*
 * 结构输入：
 *          between_op
 *          /         \
 * fieldname           and_op
 *                    /      \
 *                 op1        op2
 */
std::shared_ptr<DDSSQLCondition> DDSSQL::DDSSQLFilter::TransformTreeToBetween(const BinaryTreeNode& node, const std::vector<std::string>& parameters)
{
	std::shared_ptr<TypicalValue> field =
		TransformTreeToValue(node.left(), parameters);

	const BinaryTreeNode& andNode = node.right();

	if (!andNode.is<andOp>()) {
		throw std::runtime_error("Expected 'and' operation in between condition.");
	}

	std::shared_ptr<TypicalValue> op1 = TransformTreeToValue(andNode.left(), parameters);

	std::shared_ptr<TypicalValue> op2 = TransformTreeToValue(andNode.right(), parameters);

	if (!CheckValueCompatibility(field->kind, op1->kind, false) ||
		!CheckValueCompatibility(field->kind, op2->kind, false)) {
		throw std::runtime_error("Incompatible types in between condition.");
	}
	RelOperationKind relOp = node.is<betweenOp>() ? RelOperationKind::LESS_EQUAL : RelOperationKind::GREATER_THAN;
	LogicalOperationKind logicOp = node.is<betweenOp>() ? LogicalOperationKind::AND : LogicalOperationKind::OR;

	return std::make_shared<DDSSQLLogicalCondition>(
		logicOp,
		std::make_shared<DDSSQLPredicate>(relOp, op1, field),
		std::make_shared<DDSSQLPredicate>(relOp, field, op2)
	);
}

void DDSSQLFilter::TransformEnum(std::shared_ptr<TypicalValue> left, std::shared_ptr<TypicalValue> right)
{
	// TODO: Implement logic to transform ENUM values for comparison

}

bool DDSSQLFilter::CheckValueCompatibility(TypicalKind left, TypicalKind right, bool ignoreEnum)
{
	if (!ignoreEnum && ENUM == right)
	{
		return ENUM == left || SIGNED_INTEGER == left || UNSIGNED_INTEGER == left || STRING == left;
	}

	switch (left)
	{
	case BOOLEAN:
		return BOOLEAN == right || SIGNED_INTEGER == right || UNSIGNED_INTEGER == right;

	case SIGNED_INTEGER:
	case UNSIGNED_INTEGER:
		return SIGNED_INTEGER == right ||
			UNSIGNED_INTEGER == right ||
			BOOLEAN == right ||
			FLOAT == right ||
			DOUBLE == right ||
			LONG_DOUBLE == right;

	case CHAR:
	case STRING:
		return CHAR == right ||STRING == right;

	case FLOAT:
	case DOUBLE:
	case LONG_DOUBLE:
		return FLOAT == right ||
			DOUBLE == right ||
			LONG_DOUBLE == right ||
			SIGNED_INTEGER == right ||
			UNSIGNED_INTEGER == right;

	case ENUM:
		if (!ignoreEnum)
		{
			return ENUM == right ||
				SIGNED_INTEGER == right ||
				UNSIGNED_INTEGER == right ||
				STRING == right;
		}
	}

	return false;
}

RelOperationKind DDSSQLFilter::GetPredicateOp(const BinaryTreeNode& node)
{
	RelOperationKind ret;
	if (node.is<eqOp>()) {
		ret = RelOperationKind::EQUAL;
	} else if (node.is<neOp>()) {
		ret = RelOperationKind::NOT_EQUAL;
	} else if (node.is<ltOp>()) {
		ret = RelOperationKind::LESS_THAN;
	} else if (node.is<leOp>()) {
		ret = RelOperationKind::LESS_EQUAL;
	} else if (node.is<gtOp>()) {
		ret = RelOperationKind::GREATER_THAN;
	} else if (node.is<geOp>()) {
		ret = RelOperationKind::GREATER_EQUAL;
	} else if (node.is<likeOp>()) {
		ret = RelOperationKind::LIKE;
	} else {
		throw std::runtime_error("Unsupported operation in predicate.");
	}

	return ret;
}
