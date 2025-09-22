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
#ifndef TRAVODDS_DCPS_TOPIC_CONTENTFILTEREDTOPIC_H
#define TRAVODDS_DCPS_TOPIC_CONTENTFILTEREDTOPIC_H

#include "dcps/topic/topicdescription.h"
#include "dcps/base/retcode.h"
#include "type/basictype.h"

TRAVODDS_NAMESPACE_BEGIN

class Topic;
/**
 * @brief TopicDescription的特例化，允许基于内容的订阅。
 */
class DDS_DLL ContentFilteredTopic : public virtual TopicDescription {
public:
    /**
     * @brief 析构函数。
     */
    virtual ~ContentFilteredTopic() = default;

    /**
     * @brief 获取相关主题指针。
     *
     * 此函数返回与ContentFilteredTopic关联的Topic。即创建ContentFilteredTopic时指定的Topic。
     */
    virtual Topic* get_related_topic() const = 0;

    /**
     * @brief 获取筛选器表达式。
     *
     * 此函数返回与此ContentFilteredTopic关联的筛选器表达式。
     * 它将返回上次成功调用set_expression时指定的filter_expressions，
     * 如果从未调用该方法，则返回创建ContentFilteredTopic时指定的表达式。
     *
     * @return filter_expression字符串。
     */
    virtual const std::string& get_filter_expression() const = 0;

    /**
     * @brief 获取表达式参数。
     *
     * 此函数返回与此ContentFilteredTopic关联的表达式参数。
     * 这些将是上次成功调用set_expression或set_expression_parameters时指定的expression_parameters。
     * 如果从未调用这些方法，则将返回创建ContentFilteredTopic时指定的表达式参数。
     *
     * @param [out] expression_parameters  当前与ContentFilteredTopic关联的表达式参数。
     *
     * @return RETCODE_OK
     */
    virtual ReturnCode_t get_expression_parameters(StringSeq& expression_parameters) const = 0;

    /**
     * @brief 设置表达式参数。
     *
     * 此操作更改与此ContentFilteredTopic关联的表达式参数。
     *
     * @param [in] expression_parameters  要设置的表达式参数。
     *
     * @return RETCODE_OK             表达式参数正确更新。
     *         RETCODE_BAD_PARAMETER  表达式参数与当前filterexpression不匹配。
     */
    virtual ReturnCode_t set_expression_parameters(const StringSeq& expression_parameters) = 0;
};

TRAVODDS_NAMESPACE_END
#endif // !CONTENT_FILTERED_TOPIC_H
