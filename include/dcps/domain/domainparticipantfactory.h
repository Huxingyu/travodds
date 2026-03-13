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
#ifndef TRAVODDS_DCPS_DOMAIN_DOMAINPARTICIPANTFACTORY_H
#define TRAVODDS_DCPS_DOMAIN_DOMAINPARTICIPANTFACTORY_H

#include "dcps/domain/domainparticipant.h"
#include "dcps/listener/domainparticipantlistener.h"

TRAVODDS_NAMESPACE_BEGIN

#define TheParticipantFactory  DomainParticipantFactory::get_instance()

class DDS_DLL DomainParticipantFactory {
public:
    /**
     * @brief 析构函数。
     */
    virtual ~DomainParticipantFactory() = default;

    /**
     * @brief 创建域参与者，并绑定参数中的QoS和监听器。创建完成的域参与者可用于后续对发布者和订阅者等进行管理。
     *
     * @param [in] domain_id 域id。
     * @param [in] qos       DomainParticipantQos参考。
     * @param [in] listener  域参与者监听器指针。
     * @param [in] mask      StatusMask位掩码参考，用于指定哪些特定事件会触发监听器的回调。
     * 
     * @return DomainParticipant指针（如果未创建，则为nullptr）。
     */
    virtual DomainParticipant* create_participant(DomainId_t domain_id,
                                                  const DomainParticipantQos& qos,
                                                  DomainParticipantListener* listener,
                                                  const StatusMask& mask) = 0;

    /**
	 * @brief 创建域参与者，并绑定参数中的QoS和监听器。创建完成的域参与者可用于后续对发布者和订阅者等进行管理。
     * 
	 * @param [in] domain_id 域id。
     * @param [in] qos       DomainParticipantQos参考。
     * @param [in] listener  域参与者监听器指针。
     * @param [in] mask      StatusMask位掩码参考，用于指定哪些特定事件会触发监听器的回调。
	 * @param [in] guid      GUID_t参考，用于指定创建的DomainParticipant的全局唯一标识符。
     *
     * @return DomainParticipant指针（如果未创建，则为nullptr）。
     */
    virtual DomainParticipant* create_participant(DomainId_t domain_id,
        const DomainParticipantQos& qos,
        DomainParticipantListener* listener,
        const StatusMask& mask,
        const GUID_t& guid) = 0;

    /**
     * @brief 删除参与者以及所有关联的发布者和订阅者。
     *
     * @param [in] participant 指向参与者的指针。
     * 
     * @return 如果参与者有活动实体，则返回RETCODE_PRECONDION_NOT_MET，
     * 如果参与者被正确删除，则返回RETCODE_OK，否则返回RETCODE_ERROR
     */
    virtual ReturnCode_t delete_participant(DomainParticipant*& participant) = 0;

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
    virtual DomainParticipant* lookup_participant(DomainId_t domain_id) const = 0;

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
    virtual ReturnCode_t set_default_participant_qos(const DomainParticipantQos& qos) = 0;

    /**
     * @brief 此函数检索DomainParticipant QoS的默认值，即QoS策略，
     * 在create_participant.函数中默认QoS政策的情况下，将用于新创建的DomainPartipant实体。
     * 检索到的值get_default_participant_qos将与上次成功调用
     * set_default_Participatnt_qos时指定的值集匹配，否则，如果从未进行调用，则为默认值。
     *
     * @param [out] qos 返回qos的DomainParticipantQos。
     * @return RETCODE_OK
     */
    virtual ReturnCode_t get_default_participant_qos(DomainParticipantQos& qos) const = 0;

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
    virtual ReturnCode_t set_qos(const DomainParticipantFactoryQos& qos) = 0;
    
    /**
     * @brief 此函数返回DomainParticipantFactory QoS策略的值。
     *
     * @param [out] qos 返回qos的DomaParticipantFactoryQos引用
     * @return RETCODE_OK
     */
    virtual ReturnCode_t get_qos(DomainParticipantFactoryQos& qos) const = 0;

    /**
     * @brief 返回DomainParticipantFactory单例。
     *
     * @return DomainParticipantFactory单例。
     */
    static DomainParticipantFactory* get_instance();

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

    virtual ReturnCode_t load_qos_from_xml(const char* xml_path) = 0;

    /**
     * @fn  virtual ReturnCode_t unload_qos() = 0;
     * 
     * @brief   卸载QoS配置。
     * 
     * @return  当前可能的返回值如下：
     *         - #RETCODE_OK :表示卸载成功；
     *         - #RETCODE_ERROR :表示卸载失败。
     */
    virtual ReturnCode_t unload_qos() = 0;

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
        StatusMask mask) = 0;
    
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
        const char* qos_name) = 0;

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
        const char* qos_name) = 0;
    
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
        const char* qos_name) = 0;

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
        const char* qos_name) = 0;

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
        const char* qos_name) = 0;

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
        const char* qos_name) = 0;

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
        const char* qos_name) = 0;

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
        const char* qos_name) = 0;

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
        const char* qos_name) = 0;

#endif // _XML_INTERFACE_CPP

	/**
	 * @fn virtual ReturnCode_t DomainParticipantFactory::load_security_plugin( const Char* plugin_name, const Char* file_name, const Char* retrieve_instance_func_name, const Char* finalize_instance_func_name) = 0;
	 *
	 * @brief 加载安全插件
	 *
	 * @details - 在Windows和Linux内核系统中，安全插件以动态库的形式提供（Windows：.dll，Linux：.so），
	 *          可以在运行时通过调用该函数加载。
	 *          - 在VxWorks和ReWorks中，安全插件以开发库的形式提供（.a），必须在开发应用时一同编译到应用中，
	 *          并在需要使用安全插件之前调用该函数。此时的调用 @e file_name 参数不起作用。
	 *          注意：由于ReWorks的基础开发环境ReDe在编译时未被直接引用的符号会在编译过程中被删除，
	 *          因此需要在使用安全插件的应用中放置一个函数，在函数中调用安全插件获取实例和删除实例的函数，
	 *          避免这两个函数被编译器优化。
	 *
	 * @param plugin_name                 为当前加载的安全插件赋予的名称，用于在创建域参与者时引用安全插件。
	 * @param file_name                   安全插件的文件名称，在Windows中是动态链接库（dll），Linux中是动态库（so），VxWorks和ReWorks中此参数不起作用。
	 * @param retrieve_instance_func_name 安全插件中获取实例的函数名称，需要参考安全插件的使用手册获知。
	 * @param finalize_instance_func_name 安全插件中删除实例的函数名称，需要参考安全插件的使用手册获知。
	 *
	 * @return 当前可能的返回值：
	 *          - #RETCODE_OK :表示加载成功；
	 *          - #RETCODE_ERROR :表示加载失败，失败的原因包含：插件文件不存在、加载文件失败、获取安全插件中的函数失败、获取实例失败，详见运行时日志信息。
	 */
	virtual ReturnCode_t load_security_plugin(
		const char* plugin_name,
		const char* file_name,
		const char* retrieve_instance_func_name,
		const char* finalize_instance_func_name) = 0;
    virtual ReturnCode_t unload_security_plugin(const char* plugin_name) = 0;
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_DOMAIN_DOMAINPARTICIPANTFACTORY_H
