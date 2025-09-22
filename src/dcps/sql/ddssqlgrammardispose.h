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
#ifndef TRAVODDS_DCPS_SQL_GRAMMAR_DISPOSE_H
#define TRAVODDS_DCPS_SQL_GRAMMAR_DISPOSE_H
#include "numericconstants.h"
#include "dcps/sql/binarytreenode.h"

TRAVODDS_NAMESPACE_BEGIN

namespace DDSSQL {
	namespace pegtl = tao::pegtl;


    struct CurrentFieldState
    {
		std::vector<FieldAccessor> fields;
        DynamicType* rootType;
        DynamicType* currentType;
    };

	struct TypicalDispose 
		: pegtl::parse_tree::apply<TypicalDispose>
	{
        
		template< typename ... States >
		static void transform(
			std::unique_ptr<BinaryTreeNode>& n,
			States&&... /*st*/) {
            n->value.reset(new TypicalValue());
            //if (n->is<trueValue>())
            //{
            //    n->value->kind = BOOLEAN;
            //    n->value->booleanValue = true;
            //}
            //else if (n->is<falseValue>())
            //{
            //    n->value->kind = BOOLEAN;
            //    n->value->booleanValue = false;
            //} else 
            if (n->is<INTEGERVALUE>()) {
                if (n->m_begin.data[0] == '-' || n->m_begin.data[0] == '+') {
                    n->value->kind = SIGNED_INTEGER;
                    n->value->signedIntegerValue = std::stoll(n->content(), nullptr, 0);
                } else {
                    n->value->kind = UNSIGNED_INTEGER;
			        n->value->unsignedIntegerValue = std::stoull(n->content(), nullptr, 0);
                }
            } else if (n->is<FLOATVALUE>()) {
                n->value->kind = FLOAT;
                n->value->floatValue = std::stod(n->content());
            } else if (n->is<CHARVALUE>()) {
                n->value->kind = CHAR;
                n->value->charValue = n->m_begin.data[1];
			} else if (n->is<STRINGVALUE>()) {
                const BinaryTreeNode& lNode = n->left();
                size_t strLen = lNode.m_end.byte - lNode.m_begin.byte;
                if (strLen > BYTE_ORDER_LENGTH_MAX) {
                    throw pegtl::parse_error("string size more than 255.", n->end());
                }
                n->value->kind = STRING;
                std::copy(lNode.m_begin.data, lNode.m_begin.data + strLen, n->value->stringValue.begin());
            }
            n->children.clear();
		}
	};

    struct ParameterProcessor
        : pegtl::parse_tree::apply<ParameterProcessor>
    {
        template< typename ... States >
        static void transform(
            std::unique_ptr<BinaryTreeNode>& n,
            States&&... /*st*/) {
            n->parameterIndex = static_cast<int32_t>(n->m_begin.data[1] - '0');
            if (n->m_end.byte - n->m_begin.byte == 3) {
                n->parameterIndex *= 10;
                n->parameterIndex += static_cast<int32_t>(n->m_begin.data[2] - '0');
            }
        }
    };
    struct FieldProcessor
        : pegtl::parse_tree::apply< FieldProcessor >
    {

        template< typename ... States >
        static void transform(
            std::unique_ptr<BinaryTreeNode>& n,
			CurrentFieldState& state,
            States&&... /*st*/)
        {
            if (n->is<FIELDNAME>()) {
                n->value.reset(new DDSSQLFieldValue(state.fields));
                // state.fields 内容清空
                n->typeDesc = state.currentType;
                switch (state.currentType->descriptor_.kind_) {
                case TypeKind::TK_BOOLEAN: {
                    n->value->kind = BOOLEAN;
					break;
                }
                case TypeKind::TK_CHAR8: {
                    n->value->kind = CHAR;
                    break;
                }
                case TypeKind::TK_ENUM:
                case TypeKind::TK_INT16: 
                case TypeKind::TK_INT32: 
                case TypeKind::TK_INT64: {
                    n->value->kind = state.currentType->descriptor_.kind_ == TypeKind::TK_ENUM ? ENUM: SIGNED_INTEGER;
                    break;
                }
				case TypeKind::TK_BYTE:
                case TypeKind::TK_UINT16: 
                case TypeKind::TK_UINT32:
                case TypeKind::TK_UINT64: {
                    n->value->kind = UNSIGNED_INTEGER;
                    break;
                }
                case TypeKind::TK_FLOAT32: {
                    n->value->kind = FLOAT;
                    break;
                }
                case TypeKind::TK_FLOAT64: {
                    n->value->kind = DOUBLE;
                    break;
                }
                case TypeKind::TK_FLOAT128: {
                    n->value->kind = LONG_DOUBLE;
                    break;
                }
                case TypeKind::TK_STRING8: {
                    n->value->kind = STRING;
                    break;
                }
                default:
                    throw pegtl::parse_error(
                        "Unsupported type '" + state.currentType->get_name() + "' for field '" + n->content() + "'.",
                        n->end());
                }
                state.currentType = nullptr;
            } else {
                if (state.currentType == nullptr) {
                    state.currentType = state.rootType;
                }
				std::string memberName = n->children[0]->content();
                DynamicTypeMember member;
                if (RETCODE_OK != state.currentType->get_member_by_name(member, memberName)) {
                    throw pegtl::parse_error(
                        "Field '" + memberName + "' not found in type '" + state.currentType->get_name() + "'.",
                        n->end());
                }
                MemberDescriptor mdesc;
                member.get_descriptor(mdesc);
                if (n->children.size() > 1) {

                    if (n->children.size() - 1 != mdesc.type_->descriptor_.bound_.size()) {
                        throw pegtl::parse_error(
                            "Array dimension mismatch for field '" + memberName + "'.",
                            n->end());
                    }
                }
                std::vector<int32_t> arrayIndex; // 默认无数组索引
                for(int i = 1;i< n->children.size();i++) {
                    int32_t index = static_cast<int32_t>(std::stoul(n->children[i]->children[0]->content()));
                    if (mdesc.type_->descriptor_.kind_ == TK_ARRAY && index >= mdesc.type_->descriptor_.bound_[i - 1]) {
                        throw pegtl::parse_error(
                            "Array index out of bounds for field '" + memberName + "'.",
                            n->end()); 
                    }
					arrayIndex.emplace_back(index);
                }
                state.fields.emplace_back(FieldAccessor{ memberName, -1, arrayIndex });
                if (mdesc.type_->descriptor_.kind_ == TK_ARRAY || mdesc.type_->descriptor_.kind_ == TK_SEQUENCE) {
                    state.currentType = mdesc.type_->descriptor_.element_type_;
                } else {
                    state.currentType = mdesc.type_;
                }
            }
            n->children.clear();
        }
    };

	struct SerialBinaryTree
		: pegtl::parse_tree::apply<SerialBinaryTree>
	{
        /*
		 * 递归重排节点。基本原则是：
         *
         * 原始结构：   SEQ_BASED_RULE
         *                /   |   \          （LHS... 可以是一个或多个子节点，后面跟着 OP，）
         *             LHS... OP   RHS       （OP 是一个操作符，RHS 是一个单独的子节点）
         *
         * 转换后：         OP
         *                  /  \             （OP 现在有两个子节点，分别是原来的 PROD/EXPR 和 RHS）
         *    SEQ_BASED_RULE    RHS          （注意 PROD/EXPR 现在少了两个子节点）
         *          |
         *         LHS...
         *
         * 如果 LHS... 只剩下一个子节点，则用该子节点直接替换 SEQ_BASED_RULE。
         * 否则，执行上述转换，然后递归应用，直到 LHS... 变成单一子节点，最后用它替换父节点，递归结束。
         */

        template< typename ... States >
        static void transform(
            std::unique_ptr<BinaryTreeNode>& n,
            States&&... st)
        {
            // 基本情况：单子节点直接提升
            if (n->children.size() == 1) {
                n = std::move(n->children.back());
            } else {
                n->remove_content();
                auto& c = n->children;
                // 提取右操作数和操作符
                auto rhs = std::move(c.back()); // 右操作数 (RHS)
                c.pop_back();
                auto op = std::move(c.back()); // 操作符 (OP)
                c.pop_back();
                if (c.empty()) {
                    // 无剩余左操作数：直接组合 OP 和 RHS
                    op->children.emplace_back(std::move(rhs));
                    n = std::move(op);
                } else {
                    // 有剩余左操作数
                    op->children.emplace_back(std::move(n)); // 原节点变为左子树
                    op->children.emplace_back(std::move(rhs)); // 右操作数
                    n = std::move(op);
                    // 递归处理
                    transform(n->children.front(), st ...);
                }
            }
        }
	};




}
TRAVODDS_NAMESPACE_END


#endif// TRAVODDS_DCPS_SQL_GRAMMAR_DISPOSE_H