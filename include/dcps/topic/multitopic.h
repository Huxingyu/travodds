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
#ifndef AGIDDS_DCPS_TOPIC_MULTITOPIC_H
#define AGIDDS_DCPS_TOPIC_MULTITOPIC_H

#include "dcps/topic/topicdescription.h"
#include "dcps/base/retcode.h"
#include "type/basictype.h"

AGIDDS_NAMESPACE_BEGIN
/**
 * @brief MultiTopic 是 DDS (Data Distribution Service) 中的一个高级特性，
 * 它允许订阅者通过单个 DataReader 接收来自多个 Topic 的数据，并对这些数据进行过滤、组合或转换。
 * 这种机制为复杂的数据订阅场景提供了更灵活的解决方案。
 */
class MultiTopic : public virtual TopicDescription {
public:
    /**
     * @brief 析构函数。
     */
    virtual ~MultiTopic() = default;

	/**
     * @brief 获取订阅端多主题表达式。
     *
     * @return 多主题表达式。
     */
	virtual const std::string&  get_subscription_expression() const = 0;

    /**
     * @brief 获取表达式参数。
     *
     * @param [out] expression_parameters 表达式参数。
     *
     * @return RETCODE_OK。
     */
    virtual ReturnCode_t get_expression_parameters(StringSeq& expression_parameters) const = 0;

    /**
     * @brief 设置表达式参数。
     *
     * @param [in] expression_parameters 表达式参数。
     *
     * @return RETCODE_OK。
     */
    virtual ReturnCode_t set_expression_parameters(const StringSeq& expression_parameters) = 0;
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_TOPIC_MULTITOPIC_H
