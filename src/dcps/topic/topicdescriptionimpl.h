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
#ifndef TRAVODDS_DCPS_TOPIC_TOPICDESCRIPTIONIMPL_H
#define TRAVODDS_DCPS_TOPIC_TOPICDESCRIPTIONIMPL_H

#include "dcps/topic/topicdescription.h"
#include <string>
#include <atomic>
TRAVODDS_NAMESPACE_BEGIN
class DomainParticipantImpl;
/**
 * @brief TopicDescriptionImplTopicDescription类，表示发布和订阅都绑定到单个数据类型的真实情况。
 */
class TopicDescriptionImpl : public virtual TopicDescription {
public:
    /**
     * @brief 构造函数。
     *
     * @param [in] name         主题名。
     * @param [in] type_name    主题通信数据类型名。
     * @param [in] participant	域参与者DomainParticipant类指针。
     */
    TopicDescriptionImpl(const std::string& name, const std::string& type_name, DomainParticipantImpl* participant);

    /**
     * @brief 析构函数。
     */
    virtual ~TopicDescriptionImpl();

    /**
     * @brief 获取TopicDescription所属的DomainParticipant。
     *
     * @return TopicDescription所属的DomainParticipant。
     */
    DomainParticipant* get_participant() const;

    /**
     * @brief 获取关联的数据类型名称。
     *
     * @return 数据类型名.
     */
    const char* get_type_name() const;

    /**
     * @brief 获取用于创建此TopicDescription的名称。
     *
     * @return 用于创建此TopicDescription的名称。
     */
    const char* get_name() const;

public:
    /**
     * @brief 引用计算增加。
     */
    void Reference();

    /**
     * @brief 释放引用，减少引用计数。
     */
    void Dereference();

    /**
     * @brief 是否被引用状态获取函数。
     *
     * @return 被引用情况，被应用则返回true，否则返回false。
     */
    bool IsReferenced();
protected:
    std::string name_;
    std::string typeName_;
    DomainParticipantImpl* participant_ = nullptr;

    /* 引用计数 */
    std::atomic_size_t refs_;
};
TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_TOPIC_TOPICDESCRIPTIONIMPL_H
