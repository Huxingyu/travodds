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
#ifndef TRAVODDS_DCPS_SQL_GRAMMAR_H
#define TRAVODDS_DCPS_SQL_GRAMMAR_H

#include "travodds_dll.h"

#include <pegtl.hpp>

TRAVODDS_NAMESPACE_BEGIN

namespace DDSSQL {
	namespace pegtl = tao::pegtl;

    // 基础规则定义
	// struct ws : pegtl::star<pegtl::blank> {}; //不支持换行和制表符
    struct ws : pegtl::star<pegtl::space> {};
    struct sep : pegtl::seq<pegtl::one<','>, ws> {};
    struct integer : pegtl::plus< pegtl::digit > {};

    // 支持嵌套字段名（如 a.b.c），点数不限
    struct dotOp : pegtl::one< '.' > {};
    struct indexPart : pegtl::seq< pegtl::one<'['>, integer, pegtl::one<']'> > {};
    struct fieldnamePart : pegtl::seq<pegtl::identifier, pegtl::star<indexPart > > {};
    struct FIELDNAME : pegtl::list<fieldnamePart, dotOp> {};

    // TOPICNAME: 不能以数字开头的标识符
    using topicnameFirst = pegtl::ranges<'a', 'z', 'A', 'Z' >;
    using topicnameOther = pegtl::ranges<'a', 'z', 'A', 'Z', '0', '9'>;
    struct TOPICNAME : pegtl::seq<topicnameFirst, pegtl::star<topicnameOther > > {};

    // INTEGERVALUE: 支持十进制和十六进制，带可选正负号
    struct sig : pegtl::opt<pegtl::sor<pegtl::one<'+'>, pegtl::one<'-'>>> {};
    struct hexPrefix : pegtl::string<'0', 'x'> {};
    struct hexValue : pegtl::seq<hexPrefix, pegtl::plus<pegtl::xdigit>> {};
    struct INTEGERVALUE : pegtl::seq<sig, pegtl::sor<hexValue, integer>> {};

    // CHARVALUE: 单引号包裹的单个字符
    struct CHARVALUE : pegtl::seq<pegtl::one<'\''>, pegtl::not_one<'\''>, pegtl::one<'\''>> {};

    // FLOATVALUE: 支持正负号、浮点和科学计数法
    struct exp : pegtl::seq<pegtl::one<'e', 'E'>, sig, integer> {};
    struct floatCode : pegtl::seq<integer, dotOp, integer> {};
    struct FLOATVALUE : pegtl::seq<sig, floatCode, pegtl::opt<exp>> {};

    // STRING: 单引号包裹的字符串（不含换行和右引号）
    struct stringContent : pegtl::star<pegtl::not_one<'\'', '\r', '\n'>> {};
    struct STRINGVALUE : pegtl::seq<pegtl::one<'\''>, stringContent, pegtl::one<'\''>> {};

    //// ENUMERATEDVALUE: 单引号包裹的标识符
    //struct ENUMERATEDVALUE : pegtl::seq<pegtl::one<'\''>, pegtl::identifier, pegtl::one<'\''>> {};

    // PARAMETER: %n, n为0~99
    struct PARAMETER_Num : pegtl::sor<
        pegtl::string<'0'>,
        pegtl::seq<pegtl::ranges<'1', '9'>, pegtl::opt<pegtl::digit>>
    > {};
    struct PARAMETER : pegtl::seq<pegtl::one<'%'>, PARAMETER_Num> {};

	// 布尔值
    //struct trueValue : pegtl::pad<pegtl::keyword<'t', 'r', 'u', 'e'>, pegtl::space> {};
    //struct falseValue : pegtl::pad<pegtl::keyword<'f', 'a', 'l', 's', 'e'>, pegtl::space> {};
    //struct BOOLEANVALUE : pegtl::sor<falseValue, trueValue> {};

    // 操作符定义
    struct likeOp : pegtl::pad<pegtl::keyword<'L', 'I', 'K', 'E'>, pegtl::space> {};
    struct eqOp : pegtl::pad<pegtl::one<'='>, pegtl::space> {};
    struct gtOp : pegtl::pad<pegtl::one<'>'>, pegtl::space> {};
    struct geOp : pegtl::pad<pegtl::seq<pegtl::one<'>'>, pegtl::one<'='>>, pegtl::space> {};
    struct ltOp : pegtl::pad<pegtl::one<'<'>, pegtl::space> {};
    struct leOp : pegtl::pad<pegtl::seq<pegtl::one<'<'>, pegtl::one<'='>>, pegtl::space> {};
    struct neOp : pegtl::pad<pegtl::seq<pegtl::one<'<'>, pegtl::one<'>'>>, pegtl::space> {};

    struct RelOp : pegtl::sor<likeOp, geOp, leOp, neOp, eqOp, gtOp, ltOp
    > {
    };

	struct BaseType : pegtl::sor<FLOATVALUE, INTEGERVALUE, CHARVALUE, STRINGVALUE> {};
    // 参数规则 - 更新以包含PARAMETER
    struct Parameter : pegtl::sor<FLOATVALUE, INTEGERVALUE, CHARVALUE, STRINGVALUE, PARAMETER> {};

	// 关键词规则
    struct andOp : pegtl::pad<pegtl::keyword<'A', 'N', 'D'>, pegtl::space>{};
    struct notOp : pegtl::pad<pegtl::keyword<'N', 'O', 'T'>, pegtl::space> {};
    struct orOp : pegtl::pad<pegtl::keyword<'O', 'R'>, pegtl::space> {};
    struct betweenOp : pegtl::pad<pegtl::keyword<'B', 'E', 'T', 'W', 'E', 'E', 'N'>, pegtl::space> {};
    struct notBetweenOp : pegtl::pad<pegtl::keyword<'N', 'O', 'T', ' ', 'B', 'E', 'T', 'W', 'E', 'E', 'N'>, pegtl::space> {};

    // 范围规则
    struct Range : pegtl::seq<Parameter, andOp, Parameter> {};

    // 谓词规则
    struct BetweenPredicate;
    struct ComparisonPredicate;
    struct Predicate : pegtl::sor<ComparisonPredicate, BetweenPredicate> {};

    struct ComparisonPredicate : pegtl::sor<
        pegtl::seq<Parameter, RelOp, FIELDNAME>,
        pegtl::seq<FIELDNAME, RelOp, Parameter>,
        pegtl::seq<FIELDNAME, RelOp, FIELDNAME>
    > {
    };

    struct BetweenPredicate : pegtl::sor<
        pegtl::seq<FIELDNAME, pegtl::sor<betweenOp, notBetweenOp>, Range>
    > {
    };

    // 条件规则
    struct Condition;

    struct ConditionList : pegtl::list_must<Condition, pegtl::sor<andOp, orOp>> {};
    struct Condition : pegtl::sor<
        pegtl::seq<pegtl::one<'('>, ws, ConditionList, ws, pegtl::one<')'>>,
        pegtl::seq<notOp, ConditionList>,
        Predicate
    > {
    };

    // 过滤表达式规则 - 继承自 ConditionList
    struct FilterExpression : ConditionList {};

    /* TODO :  QueryExpression、TopicExpression待完善实现 */

    // 自然连接规则
    //struct INNER_NATURAL_JOIN 
    //    : pegtl::pad<pegtl::keyword<'I', 'N', 'N', 'E', 'R', ' ', 'N', 'A', 'T', 'U', 'R', 'A', 'L', ' ', 'J', 'O', 'I', 'N'>
    //    , pegtl::space
    //> {
    //};
    //struct NATURAL_JOIN
    //    : pegtl::pad<pegtl::keyword<'N', 'A', 'T', 'U', 'R', 'A', 'L', ' ', 'J', 'O', 'I', 'N'>
    //    ,  pegtl::space
    //> {
    //};

    //struct NATURAL_INNER_JOIN 
    //    : pegtl::pad< pegtl::keyword<'N', 'A', 'T', 'U', 'R', 'A', 'L', ' ', 'I', 'N', 'N', 'E', 'R', ' ', 'J', 'O', 'I', 'N'>
    //    , pegtl::space
    //> {
    //};

    //struct NaturalJoin : pegtl::sor<
    //    INNER_NATURAL_JOIN,
    //    NATURAL_JOIN,
    //    NATURAL_INNER_JOIN
    //> {
    //};

    // 连接项规则
    //struct JoinItem;
    //struct JoinItemBase : pegtl::sor<
    //    TOPICNAME,
    //    pegtl::seq<pegtl::one<'('>, ws, TOPICNAME, ws, NaturalJoin, ws, JoinItem, ws, pegtl::one<')'>>
    //> {
    //};

    //struct JoinItem : pegtl::sor<
    //    JoinItemBase,
    //    pegtl::seq<TOPICNAME, ws, NaturalJoin, ws, JoinItem>
    //> {
    //};

    // 选择规则 - 更新以支持TOPICNAME作为参数
    //struct Selection : pegtl::sor<
    //    TOPICNAME,
    //    pegtl::seq<TOPICNAME, ws, NaturalJoin, ws, JoinItem>
    //> {
    //};

    // 字段规范规则 - 更新以支持FIELDNAME作为参数
    //struct SubjectFieldSpec : pegtl::sor<
    //    FIELDNAME,
    //    pegtl::seq<FIELDNAME, ws, pegtl::keyword<'A', 'S'>, ws, FIELDNAME>,
    //    pegtl::seq<FIELDNAME, ws, FIELDNAME>
    //> {
    //};

    // 聚合规则
    //struct Aggregation : pegtl::sor<
    //    pegtl::one<'*'>,
    //    pegtl::list<SubjectFieldSpec, sep>
    //> {
    //};

    // SelectFrom 规则
    //struct SelectFrom : pegtl::seq<
    //    pegtl::keyword<'S', 'E', 'L', 'E', 'C', 'T'>, ws, Aggregation, ws,
    //    pegtl::keyword<'F', 'R', 'O', 'M'>, ws, Selection
    //> {
    //};

    // Where 规则
    //struct Where : pegtl::seq<pegtl::keyword<'W', 'H', 'E', 'R', 'E'>, ws, Condition> {};



    // 查询表达式规则
    //struct QueryExpression : pegtl::seq<
    //    pegtl::plus<Condition, ws>,
    //    pegtl::opt<pegtl::seq<ws, pegtl::keyword<'O', 'R', 'D', 'E', 'R', ' ', 'B', 'Y'>, ws,
    //    pegtl::list<FIELDNAME, sep>>>
    //    > {};

    // 主题表达式规则
    //struct TopicExpression : pegtl::seq<
    //    SelectFrom,
    //    pegtl::opt<pegtl::seq<ws, Where>>,
    //    ws, pegtl::one<';'>
    //> {
    //};
    
    // 顶层表达式规则
	struct BaseTypeGrammar : pegtl::must<BaseType, pegtl::eof> {};

    //struct Expression : pegtl::sor<FilterExpression, TopicExpression, QueryExpression> {};

        // 过滤表达式规则 主规则
    struct FilterExpressionGrammar : pegtl::must<FilterExpression, ws, pegtl::eof > {};

	// 查询表达式规则
    //struct QueryExpressionGrammar : pegtl::must<QueryExpression, pegtl::eof > {};
	// 主题表达式规则
    //struct TopicExpressionGrammar : pegtl::must<TopicExpression, pegtl::eof > {};
    // 顶层语法规则
    //struct ExpressionGrammar : pegtl::must<Expression, pegtl::eof > {};
}

TRAVODDS_NAMESPACE_END
#endif
