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
#ifndef TRAVODDS_DCPS_TOPIC_TOPICIMPL_H
#define TRAVODDS_DCPS_TOPIC_TOPICIMPL_H

#include "dcps/topic/topic.h"
#include "dcps/topic/topicdescriptionimpl.h"
#include "dcps/domain/domainentityimpl.h"
#include "dcps/base/statusmanager.h"

TRAVODDS_NAMESPACE_BEGIN
class DomainParticipantImpl;
/**
 * @brief TopicImpl类是Topic类子类及功能实现类，表示发布和订阅都绑定到单个数据类型的实际情况。
 */
class TopicImpl : public virtual Topic, public DomainEntityImpl, public TopicDescriptionImpl {
public:
    /**
     * @brief 构造函数。
     *
     * @param [in] name         主题名。
     * @param [in] type_name    主题通信数据类型名。
     * @param [in] qos          Topic Qos策略。
     * @param [in] listener	    主题监听器。
     * @param [in] mask	        StatusMask位掩码。
     * @param [in] participant	域参与者DomainParticipant类指针。
     */
    TopicImpl(const std::string& name, const std::string& type_name, const TopicQos& qos, TopicListener* listener, StatusMask mask, DomainParticipant* participant);

    /**
     * @brief 析构函数。
     */
    virtual ~TopicImpl();

    /**
     * @brief 允许修改主题Qos。
     * 给定的Qos必须由主题支持。
     *
     * @param [in] qos 要为主题设置的新TopicQos值。
     *
     * @return RETCODE_IMMUTABLE_POLICY（如果不允许更改）。
     * RETCODE_INCONSISTENT_POLICY，如果新的qos具有不一致的值。
     * RETCODE_OK（如果更新了qos）。
     */
    ReturnCode_t set_qos(const TopicQos& qos);

    /**
     * @brief 获取主题Qos。
     *
     * @param [in] qos 返回qos的TopicQos。
     *
     * @return RETCODE_OK
     */
    ReturnCode_t get_qos(TopicQos& qos) const;

    /**
     * @brief 设置TopicListener。
     *
     * @param [in] listener 监听器TopicListener的新值
     * @param [in] mask     保存监听器响应的状态StatusMask位掩码。
     *
     * @return RETCODE_OK
     */
    ReturnCode_t set_listener(TopicListener* listener, const StatusMask& mask);

    /**
     * @brief 获取附加的TopicListener。
     *
     * @return 指向TopicListener的指针。
     */
    const TopicListener* get_listener() const;

    /**
     * @brief 允许应用程序获取的主题的INCONSISTENT_TOPIC_STATUS状态。
     *
     * @param [out] status  要获取的状态的数据结构。
     *
     * @return RETCODE_OK
     */
    ReturnCode_t get_inconsistent_topic_status(InconsistentTopicStatus& status) const;

    /**
     * @brief 更新不一致的主题状态信息。
     */
	void update_inconsistent_topic_status();

#ifdef _XML_INTERFACE_CPP

    /**
     * @brief 指定QoS仓库的Qos配置，并设置主题的QoS。
     *
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name     QoS名称，允许为空，将转换为空字符串。
     *
     * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t set_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

#endif // _XML_INTERFACE_CPP
public:
    /**
     * @brief 此函数用于启用实体
     *
     * @return 启用成功返回RETCODE_OK，否则返回RETCODE_PRECONDITION_NOT_MET。
     */
	ReturnCode_t enable();

private:
    TopicQos qos_;
    TopicListener* listener_ = nullptr;
	std::unique_ptr<TopicStatusManager> statusMgr_;
};
TRAVODDS_NAMESPACE_END    
#endif // !TRAVODDS_DCPS_TOPIC_TOPICIMPL_H
