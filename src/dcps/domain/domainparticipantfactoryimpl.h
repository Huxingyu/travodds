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
#ifndef AGIDDS_DCPS_DOMAIN_DOMAINPARTICIPANTFACTORYIMPL_H
#define AGIDDS_DCPS_DOMAIN_DOMAINPARTICIPANTFACTORYIMPL_H

#include "dcps/domain/domainparticipantfactory.h"
#include "dcps/domain/domainparticipantimpl.h"

AGIDDS_NAMESPACE_BEGIN

class DomainParticipantFactoryImpl : public virtual DomainParticipantFactory {
public:
 
    /**
     * @brief 析构函数。
     */
	~DomainParticipantFactoryImpl();

    /**
     * @brief 创建参与者。
     *
     * @param [in] domain_id 域id。
     * @param [in] qos       DomainParticipantQos参考。
     * @param [in] listener  域参与者监听器指针。
     * @param [in] mask      StatusMask位掩码。
     *
     * @return DomainParticipant指针（如果未创建，则为nullptr）。
     */
    DomainParticipant* create_participant(DomainId_t domain_id,
        const DomainParticipantQos& qos,
        DomainParticipantListener* listener,
        const StatusMask& mask);

    /**
     * @brief 删除参与者以及所有关联的发布者和订阅者。
     *
     * @param [in] participant 指向参与者的指针。
     *
     * @return 如果参与者有活动实体，则返回RETCODE_PRECONDION_NOT_MET，
     * 如果参与者被正确删除，则返回RETCODE_OK，否则返回RETCODE_ERROR
     */
    ReturnCode_t delete_participant(DomainParticipant*& participant);

    /**
     * @brief 此函数检索属于指定domain_id的先前创建的DomainParticipant。
     * 如果不存在这样的DomainParticipant，则函数将返回“nullptr”。
     * 如果存在属于该domain_id的多个DomainParticipant实体，
     * 则函数将返回其中一个。没有具体说明是哪一个。
     *
     * @param [in] domain_id
     *
     * @return 以前在指定域中创建的DomainParticipant。
     */
    DomainParticipant* lookup_participant(DomainId_t domain_id) const;

    /**
     * @brief 此函数设置DomainParticipant QoS策略的默认值，
     * 在create_participant函数中默认QoS策略的情况下，
     * 该值将用于新创建的DomainParticitant实体。
     *
     * 此函数将检查生成的策略是否自一致；
     * 如果不是，则函数将无效，并返回INCONSISTENT_POLICY。
     *
     * 可以将特殊值PARTICIPANT_QOS_DEFAULT传递给该函数，
     * 以指示默认值QoS应重置回工厂将使用的初始值，
     * 即将在从未调用set_default_participantqos函数时使用。
     *
     * @param [in] qos 要设置的DomainParticipantQos。
     *
     * @return RETCODE_INCONSISTENT_POLICY（如果Qos不是自一致的），RETCODE_OK（如果Qos更改正确）。
     */
    ReturnCode_t set_default_participant_qos(const DomainParticipantQos& qos);

    /**
     * @brief 此函数检索DomainParticipant QoS的默认值，即QoS策略，
     * 在create_participant.函数中默认QoS政策的情况下，将用于新创建的DomainPartipant实体。
     * 检索到的值get_default_participant_qos将与上次成功调用
     * set_default_Participatnt_qos时指定的值集匹配，否则，如果从未进行调用，则为默认值。
     *
     * @param [in] qos 返回qos的DomainParticipantQos。
     * @return RETCODE_OK
     */
    ReturnCode_t get_default_participant_qos(DomainParticipantQos& qos) const;

    /**
     * @brief 此函数设置DomainParticipantFactory QoS策略的值。
     * 这些策略控制实体工厂对象的行为。
     *
     * 注意：尽管具有QoS，但DomainParticipantFactory不是实体。
     *
     * 此函数将检查生成的策略是否自一致；
     * 如果不是，则函数将无效，并返回INCONSISTENT_POLICY。
     *
     * @param [in] qos 要设置的DomainParticipantFactoryQos.
     * @return RETCODE_IMMUTABLE_POLICY（如果无法更改任何Qos），
     * RETCODE_INCONSISTENT_POLICY（如果Qos不是自一致的），
     * RETCODE_OK（如果Qos更改正确）。
     */
    ReturnCode_t set_qos(const DomainParticipantFactoryQos& qos);

    /**
     * @brief 此函数返回DomainParticipantFactory QoS策略的值。
     *
     * @param [in] qos 返回qos的DomaParticipantFactoryQos引用
     * @return RETCODE_OK
     */
    ReturnCode_t get_qos(DomainParticipantFactoryQos& qos) const;
#ifdef _XML_INTERFACE_CPP

    /**
     * @fn  virtual ReturnCode_t load_qos_from_xml(const char* xml_path);
     *
     * @brief   从XML文件中加载QoS配置，多次调用将覆盖上一次的内容。
     *
     * @param   xml_path    Qos XML文件的路径。
     *
     * @return  当前可能的返回值如下：
     *          - #RETCODE_OK :表示加载成功；
     *          - #RETCODE_ERROR :表示加载失败。
     */
    virtual ReturnCode_t load_qos_from_xml(const char* xml_path);

    /**
     * @fn  virtual ReturnCode_t unload_qos() = 0;
     *
     * @brief   卸载QoS配置。
     *
     * @return  当前可能的返回值如下：
     *         - #RETCODE_OK :表示卸载成功；
     *         - #RETCODE_ERROR :表示卸载失败。
     */
    virtual ReturnCode_t unload_qos();

    /**
      * @fn  virtual DomainParticipant* create_participant_with_profile(DomainId_t domain_id, const char* library_name, const char* profile_name, const char* qos_name, DomainParticipantListener* listener, StatusMask mask) = 0;
      *
      * @brief   从QoS仓库中获取域参与者Qos并用其创建域参与者。
      *
      * @param   domain_id       域ID。
      * @param   library_name    QoS库的名字，允许为NULL，将转换为空字符串。
      * @param   profile_name    QoS配置的名字，允许为NULL，将转换为空字符串。
      * @param   qos_name        QoS的名字，允许为NULL，将转换为空字符串。
      * @param   listener        为该域参与者设置的监听器。
      * @param   mask            设置应用程序感兴趣的状态，只有应用程序感兴趣的状态发生变化时，才会通知应用程序。
      */
    virtual DomainParticipant* create_participant_with_profile(
        DomainId_t domain_id,
        const char* library_name,
        const char* profile_name,
        const char* qos_name,
        DomainParticipantListener* listener,
        StatusMask mask);

    /**
     * @fn  virtual ReturnCode_t set_default_participant_qos_with_profile(const char* library_name, const char* profile_name, const char* qos_name) = 0;
     *
     * @brief   从QoS仓库获取域参与者QoS并将其设为默认域参与者Qos
     *
     * @param   library_name    QoS库的名字，允许为NULL，将转换为空字符串。
     * @param   profile_name    QoS配置的名字，允许为NULL，将转换为空字符串。
     * @param   qos_name        QoS的名字，允许为NULL，将转换为空字符串。
     */
    virtual ReturnCode_t set_default_participant_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @fn  virtual ReturnCode_t set_qos_with_profile(const char* library_name, const char* profile_name, const char* qos_name) = 0;
     *
     * @brief   从QoS仓库获取QoS配置并设置到域参与者工厂。
     *
     * @param   library_name    QoS库的名字，允许为NULL，将转换为空字符串。
     * @param   profile_name    QoS配置的名字，允许为NULL，将转换为空字符串。
     * @param   qos_name        QoS的名字，允许为NULL，将转换为空字符串。
     *
     * @return  当前可能的返回值如下：
     *        - #RETCODE_OK :表示设置成功；
     *        - #RETCODE_ERROR :表示未知错误导致的设置错误。
     */
    virtual ReturnCode_t set_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @fn  virtual ReturnCode_t get_participant_factory_qos_from_profile(DomainParticipantFactoryQos& qos, const char* library_name, const char* profile_name, const char* qos_name) = 0;
     *
     * @brief   从QoS仓库获取域参与者工厂QoS的配置。
     *
     * @param [in,out]  qos             获取到的QoS。
     * @param   library_name            QoS库的名字，允许为NULL，将转换为空字符串。
     * @param   profile_name            QoS配置的名字，允许为NULL，将转换为空字符串。
     * @param   qos_name                QoS的名字，允许为NULL，将转换为空字符串。
     *
     * @return  当前可能的返回值如下：
     *          - #RETCODE_OK :表示设置成功；
     *          - #RETCODE_ERROR :表示未知错误导致的设置错误。
     */
    virtual ReturnCode_t get_participant_factory_qos_from_profile(
        DomainParticipantFactoryQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @fn  virtual ReturnCode_t get_participant_qos_from_profile(DomainParticipantQos& qos, const char* library_name, const char* profile_name, const char* qos_name) = 0;
     *
     * @brief   从QoS仓库获取域参与者QoS的配置。
     *
     * @param [in,out]  qos             获取到的QoS。
     * @param   library_name            QoS库的名字，允许为NULL，将转换为空字符串。
     * @param   profile_name            QoS配置的名字，允许为NULL，将转换为空字符串。
     * @param   qos_name                QoS的名字，允许为NULL，将转换为空字符串。
     *
     * @return  当前可能的返回值如下：
     *          - #RETCODE_OK :表示设置成功；
     *          - #RETCODE_ERROR :表示未知错误导致的设置错误。
     */
    virtual ReturnCode_t get_participant_qos_from_profile(
        DomainParticipantQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @fn  virtual ReturnCode_t get_topic_qos_from_profile(TopicQos& qos, const char* library_name, const char* profile_name, const char* qos_name) = 0;
     *
     * @brief   从QoS仓库获取主题QoS配置。
     *
     * @param [in,out]  qos             获取到的QoS。
     * @param   library_name            QoS库的名字，允许为NULL，将转换为空字符串。
     * @param   profile_name            QoS配置的名字，允许为NULL，将转换为空字符串。
     * @param   qos_name                QoS的名字，允许为NULL，将转换为空字符串。
     *
     * @return  当前可能的返回值如下：
     *          - #RETCODE_OK :表示设置成功；
     *          - #RETCODE_ERROR :表示未知错误导致的设置错误。
     */

    virtual ReturnCode_t get_topic_qos_from_profile(
        TopicQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @fn  virtual ReturnCode_t get_publisher_qos_from_profile(PublisherQos& qos, const char* library_name, const char* profile_name, const char* qos_name) = 0;
     *
     * @brief   从QoS仓库获取发布者QoS配置。
     *
     * @param [in,out]  qos             获取到的QoS。
     * @param   library_name            QoS库的名字，允许为NULL，将转换为空字符串。
     * @param   profile_name            QoS配置的名字，允许为NULL，将转换为空字符串。
     * @param   qos_name                QoS的名字，允许为NULL，将转换为空字符串。
     *
     * @return  当前可能的返回值如下：
     *          - #RETCODE_OK :表示设置成功；
     *          - #RETCODE_ERROR :表示未知错误导致的设置错误。
     */
    virtual ReturnCode_t get_publisher_qos_from_profile(
        PublisherQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @fn  virtual ReturnCode_t get_subscriber_qos_from_profile(SubscriberQos& qos, const char* library_name, const char* profile_name, const char* qos_name) = 0;
     *
     * @brief   从QoS仓库获取订阅者QoS配置。
     *
     * @param [in,out]  qos             获取到的QoS。
     * @param   library_name            QoS库的名字，允许为NULL，将转换为空字符串。
     * @param   profile_name            QoS配置的名字，允许为NULL，将转换为空字符串。
     * @param   qos_name                QoS的名字，允许为NULL，将转换为空字符串。
     *
     * @return  当前可能的返回值如下：
     *          - #RETCODE_OK :表示设置成功；
     *          - #RETCODE_ERROR :表示未知错误导致的设置错误。
     */
    virtual ReturnCode_t get_subscriber_qos_from_profile(
        SubscriberQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @fn  virtual ReturnCode_t get_datareader_qos_from_profile(DataReaderQos& qos, const char* library_name, const char* profile_name, const char* qos_name) = 0;
     *
     * @brief   从QoS仓库获取数据读取器QoS配置。
     *
     * @param [in,out]  qos             获取到的QoS。
     * @param   library_name            QoS库的名字，允许为NULL，将转换为空字符串。
     * @param   profile_name            QoS配置的名字，允许为NULL，将转换为空字符串。
     * @param   qos_name                QoS的名字，允许为NULL，将转换为空字符串。
     *
     * @return  当前可能的返回值如下：
     *          - #RETCODE_OK :表示设置成功；
     *          - #RETCODE_ERROR :表示未知错误导致的设置错误。
     */
    virtual ReturnCode_t get_datareader_qos_from_profile(
        DataReaderQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    /**
     * @fn  virtual ReturnCode_t get_datawriter_qos_from_profile(DataWriterQos& qos, const char* library_name, const char* profile_name, const char* qos_name) = 0;
     *
     * @brief   从QoS仓库获取数据写入器QoS配置。
     *
     * @param [in,out]  qos             获取到的QoS。
     * @param   library_name            QoS库的名字，允许为NULL，将转换为空字符串。
     * @param   profile_name            QoS配置的名字，允许为NULL，将转换为空字符串。
     * @param   qos_name                QoS的名字，允许为NULL，将转换为空字符串。
     *
     * @return  当前可能的返回值如下：
     *          - #RETCODE_OK :表示设置成功；
     *          - #RETCODE_ERROR :表示未知错误导致的设置错误。
     */
    virtual ReturnCode_t get_datawriter_qos_from_profile(
        DataWriterQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

#endif // _XML_INTERFACE_CPP
public:
    /**
     * @brief 返回DomainParticipantFactory单例。
     *
     * @return DomainParticipantFactory单例。
     */
    static DomainParticipantFactoryImpl* get_instance();

    /**
     * @brief 完成事务处理并执行资源清理
     *
     * 该方法通常在事务结束时调用，用于：
     * 1. 提交或回滚未完成的事务
     * 2. 释放占用的系统资源（内存、句柄等）
     * 3. 关闭打开的文件、数据库连接或网络套接字
     * 4. 执行其他必要的清理操作
     *
     * @note 注意事项：
     * - 调用后，对象可能进入不可用状态（取决于实现）
     * - 应保证多次调用是安全的（幂等性）
     * - 子类必须调用父类的 finalize() 以确保完整清理
     */
	static void finalize();
private:
    /**
     * @brief 构造函数。
     */
	DomainParticipantFactoryImpl();
	std::map<InstanceHandle_t, std::unique_ptr<DomainParticipantImpl>> participants_;
	mutable std::mutex mtxParticipants_;
    DomainParticipantFactoryQos qos_ = {};
    DomainParticipantQos defaultParticipantQos_ = {};
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_DOMAIN_DOMAINPARTICIPANTFACTORYIMPL_H
