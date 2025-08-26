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
#ifndef AGIDDS_DCPS_TOPIC_CONTENTFILTEREDTOPICIMPL_H
#define AGIDDS_DCPS_TOPIC_CONTENTFILTEREDTOPICIMPL_H

#include "dcps/topic/contentfilteredtopic.h"
#include "dcps/topic/topicdescriptionimpl.h"
AGIDDS_NAMESPACE_BEGIN
class TopicImpl;
class TypeObject;
/**
 * @brief ContentFilteredTopicImpl类，TopicDescription的具体实现，允许基于内容的订阅。
 */
class ContentFilteredTopicImpl : public virtual ContentFilteredTopic, public TopicDescriptionImpl {
public:

    /**
     * @brief 构造函数。
     *
     * @param [in] name                  主题名。
     * @param [in] related_topic	     相关主题类指针。
     * @param [in] filter_expression	 筛选器表达式。
     * @param [in] expression_parameters 表达式参数。
     * @param [in] participant	         域参与者。
     */
    ContentFilteredTopicImpl(const std::string& name, Topic* related_topic, const std::string& filter_expression, const StringSeq& expression_parameters, DomainParticipant* participant);
    
    /**
     * @brief 析构函数。
     */
    virtual ~ContentFilteredTopicImpl();

    /**
     * @brief 获取筛选器表达式。
     *
     * 此函数返回与此ContentFilteredTopic关联的筛选器表达式。
     * 它将返回上次成功调用set_expression时指定的filter_expressions，
     * 如果从未调用该方法，则返回创建ContentFilteredTopic时指定的表达式。
     *
     * @return filter_expression字符串。
     */
    const std::string& get_filter_expression() const;

    /**
     * @brief 获取相关主题指针。
     *
     * 此函数返回与ContentFilteredTopic关联的Topic。即创建ContentFilteredTopic时指定的Topic。
     */
    Topic* get_related_topic() const;

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
    ReturnCode_t get_expression_parameters(StringSeq& expression_parameters) const;

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
    ReturnCode_t set_expression_parameters(const StringSeq& expression_parameters);

public:
    static bool ValidFilterExpression(const std::string& filter_expression, const StringSeq& expression_parameters, TypeObject* typeObject);
private:

    TopicImpl* relatedTopic_ = nullptr;
    std::string filterExpression_;
    StringSeq expressionParameters_;
};
AGIDDS_NAMESPACE_END
#endif //!AGIDDS_DCPS_TOPIC_CONTENTFILTEREDTOPICIMPL_H
