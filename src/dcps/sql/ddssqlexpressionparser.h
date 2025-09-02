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
#ifndef TRAVODDS_DCPS_SQL_EXPRESSION_PARSER_H
#define TRAVODDS_DCPS_SQL_EXPRESSION_PARSER_H

#include "dcps/sql/ddssqlgrammar.h"
#include "dcps/sql/ddssqlgrammardispose.h"
TRAVODDS_NAMESPACE_BEGIN

namespace DDSSQL {

    template< typename Rule >
    using FilterExpressionSelector = pegtl::parse_tree::selector <
        Rule,

        TypicalDispose::on<
            //trueValue,
            //falseValue,
            INTEGERVALUE,
            FLOATVALUE,
		    CHARVALUE,
            STRINGVALUE>,
        ParameterProcessor::on<
            PARAMETER>,
        pegtl::parse_tree::store_content::on<
            stringContent,
            integer,
            indexPart,
            pegtl::identifier>,
        pegtl::parse_tree::remove_content::on<
            likeOp,
            eqOp, 
            gtOp, 
            geOp, 
            ltOp, 
            leOp,
            neOp,
            andOp,
            orOp,
            notOp,
            dotOp,
            betweenOp,
            notBetweenOp>,
        SerialBinaryTree::on<
            //BOOLEANVALUE,
            ComparisonPredicate,
            BetweenPredicate,
            Range,
            Condition,
            FilterExpression>,
        FieldProcessor::on<
            fieldnamePart,
            FIELDNAME >
    > ;

}

TRAVODDS_NAMESPACE_END


#endif