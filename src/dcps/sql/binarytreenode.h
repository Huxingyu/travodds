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
#ifndef TRAVODDS_DCPS_SQL_BINARY_TREE_NODE_H
#define TRAVODDS_DCPS_SQL_BINARY_TREE_NODE_H
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include "dcps/sql/ddssqlfield.h"

TRAVODDS_NAMESPACE_BEGIN
namespace DDSSQL {
	namespace pegtl = tao::pegtl;

	struct BinaryTreeNode : pegtl::parse_tree::basic_node<BinaryTreeNode>
	{
        std::unique_ptr<TypicalValue> value;

        // 参数的形式为％n， 对应参数列表索引
        int32_t parameterIndex = -1;

        // 关键特点
        //
        // 1. 二叉树性质：
        // 每个运算符节点都有且只有两个子节点
        // 叶子节点都是字段名或字面值
        // 2. 保持原语义：
        // 括号优先级通过树结构自动保持
        // AND / OR 的结合性正确保留
        // 3. 标准化形式：
        // 所有二元操作都统一为左 - 操作符 - 右结构
        // 便于后续的优化和执行
        const BinaryTreeNode& left() const
        {
            return *(children[0]);
        }

        const BinaryTreeNode& right() const
        {
            return *(children[1]);
        }

	};


}

TRAVODDS_NAMESPACE_END

#endif