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
#ifndef TRAVODDS_DCPS_PUBLISH_PUBLISHER_H
#define TRAVODDS_DCPS_PUBLISH_PUBLISHER_H

#include "dcps/publish/datawriter.h"
#include "dcps/listener/publisherlistener.h"

TRAVODDS_NAMESPACE_BEGIN

class DomainParticipant;
/**
 * @brief Publisher类，用于将数据发送到关联的订阅者。
 */
class DDS_DLL Publisher : public virtual DomainEntity {
public:
    /**
     * @brief 析构函数。
     */
    virtual ~Publisher() = default;

    /**
     * @brief 允许修改发布者Qos。
     * 输入的Qos必须是PublisherQos支持的类型。
     *
     * @param [in] qos 要设置的PublisherQos。
     * 
     * @return RETCODE_IMMUTABLE_POLICY（如果无法更改任何Qos），
     * RETCODE_INCONSISTENT_POLICY（如果Qos不是自一致的），
     * RETCODE_OK（如果Qos更改正确）。
     */
    virtual ReturnCode_t set_qos(const PublisherQos& qos) = 0;

    /**
     * @brief 获取发布者Qos.
     *
     * @return RETCODE_OK
     */
    virtual ReturnCode_t get_qos(PublisherQos& qos) const = 0;

    /**
     * @brief 修改发布者监听器。
     *
     * @param [in] listener 新的发布者监听器句柄。
     * @param [in] mask     StatusMask位掩码，保存监听器响应的状态。
     * 
     * @return RETCODE_OK。
     */
    virtual ReturnCode_t set_listener(PublisherListener* listener, const StatusMask& mask) = 0;

    /**
     * @brief 获取发布者监听器。
     *
     * @return 发布者监听器指针句柄。
     */
    virtual const PublisherListener* get_listener() const = 0;

    /**
     * @brief 创建属于当前发布者的DataWriter。
     *
     * @param [in] topic    DataWriter将要侦听的主题。
     * @param [in] qos      创建的DataWriter的Qos.
     * @param [in] listener 发布者监听器(可以为nullptr).
     * @param [in] mask     StatusMask位掩码，保存监听器响应的状态。
     * 
     * @return 指向创建的DataWriter的指针。如果失败，则返回nullptr。
     */
    virtual	DataWriter* create_datawriter(Topic* topic, const DataWriterQos& qos, DataWriterListener* listener, const StatusMask& mask) = 0;

    /**
     * @brief 删除属于发布者的DataWriter。
     *
     * 必须使用创建该datawriter的发布者对象调用delete_datawriter函数。
     * 如果在其他发布者上调用delete_datawriter，则该函数将无效，并返回false。
     *
     * 删除DataWriter将自动注销所有实例。
     * 根据WRITER_DATA_LIFECYCLE QosPolicy的设置，删除DataWriter也可能释放所有实例。
     *
     * @param [in] writer 要删除的DataWriter
     * 
     * @return 如果它不属于此发布服务器返回RETCODE_PRECONDITION_NOT_MET，
     * 如果它被正确删除返回RETCODE_OK，否则返回RETCODE.ERROR。
     */
    virtual	ReturnCode_t delete_datawriter(DataWriter*& datawriter) = 0;

    /**
     * @brief 此函数检索以前创建的属于发布服务器的DataWriter，该发布服务器附加到具有匹配Topic_name的主题。
     * 如果不存在这样的DataWriter，则函数将返回nullptr。
     *
     * 如果附加在发布者的多个DataWriter满足此条件，则函数将返回其中一个。没有指定哪一个。
     *
     * @param [in] topic_name 主题的名称。
     * 
     * @return 指向之前创建的与具有请求的topic_name的主题关联的DataWriter的指针。
     */
    virtual	DataWriter* lookup_datawriter(const std::string& topic_name) const = 0;

    /**
     * @brief 向DDS指示将要修改包含的DataWriter。
     *
     * @return 如果成功则返回RETCODE_OK，否则返回错误代码。
     *
     * @warning 尚不支持。当前返回RETCODE_UNSUPPORTED
     */
    virtual	ReturnCode_t suspend_publications() = 0;

    /**
     * @brief 向DDS指示对DataWriter的修改已完成。
     *
     * @return 如果成功则返回RETCODE_OK，否则返回错误代码。
     *
     * @warning 尚不支持。当前返回RETCODE_UNSUPPORTED
     */
    virtual	ReturnCode_t resume_publications() = 0;

    /**
     * @brief 使用附加在发布者的DataWriter程序发送一组一致缓存更改的开始信号。
     *
     * @return 如果成功则返回RETCODE_OK，否则返回错误代码。
     *
     * @warning 尚不支持。当前返回RETCODE_UNSUPPORTED
     */
    virtual	ReturnCode_t begin_coherent_changes() = 0;

    /**
     * @brief 表示一组一致缓存更改的结束。
     *
     * @return 如果成功则返回RETCODE_OK，否则返回错误代码。
     *
     * @warning 尚不支持。当前返回RETCODE_UNSUPPORTED
     */
    virtual	ReturnCode_t end_coherent_changes() = 0;

    /**
     * @brief 此函数阻止调用线程，直到所有匹配的可靠DataReader实体确认可靠DataWriter实体写入的所有数据，
     * 或者超过max_wait参数指定的持续时间，以先发送的为准。
     * 返回值true表示写入的所有样本都已被所有可靠的匹配DataReader确认；
     * 返回值false表示在确认所有数据之前已经过了max_wait。
     *
     * @param [in] max_wait 此函数的最大阻塞时间。
     * 
     * @return RETCODE_TIMEOUT（如果函数占用的时间超过建立的最大阻塞时间），
     * RETCODE_OK（如果发布服务器收到确认），
     * 否则返回RETCODE.ERROR（错误）。
     */
    virtual	ReturnCode_t wait_for_acknowledgments(const Duration_t& max_wait) = 0;

    /**
     * @brief 此函数返回发布者所属的DomainParticipant。
     *
     * @return 指向DomainParticipant的指针。
     */
    virtual	const DomainParticipant* get_participant() const = 0;

    /**
     * @brief 删除所有包含的DataWriter。
     *
     * @return 如果成功返回RETCODE_OK，否则返回错误代码。
     */
    virtual	ReturnCode_t delete_contained_entities() = 0;

    /**
     * @brief 此函数设置DataWriter QoS策略的默认值，在create_datawriter函数中默认QoS政策的情况下，该值将用于新创建的DataWriter。
     *
     * 此函数将检查生成的策略是否自一致；如果它们不是，则函数将无效，并返回false。
     *
     * 可以将特殊值DATAWRITER_QOS_DEFAULT传递给该函数，以指示应将默认QOS重置回初始值，
     * 即从未调用set_default_datawriter_qos函数时将使用的值。
     *
     * @param [in] qos 要设置的DataWriterQos。
     * 
     * @return RETCODE_INCONSISTENT_POLICY（如果Qos不是自一致的），
     * RETCODE_OK（如果Qos更改正确）。
     */
    virtual	ReturnCode_t set_default_datawriter_qos(const DataWriterQos& qos) = 0;

    /**
     * @brief 此函数获取DataWriter QoS的默认值，即在create_datawriter函数中默认QoS策略的情况下，将用于新创建的DataWriter。
     *
     * get_default_datawriter_qos获取的值将与上次成功调用set_default_datawriter_qos时指定的值集匹配，
     * 否则，如果从未进行调用，则为默认值。
     *
     * @param [out] qos 对当前默认WriterQos的引用。
     * 
     * @return RETCODE_OK
     */
    virtual	ReturnCode_t get_default_datawriter_qos(DataWriterQos& qos) const = 0;

    /**
     * @brief 将TopicQos复制到相应的DataWriterQos中。
     *
     * @param [out] writer_qos
     * @param [in] topic_qos
     * 
     * @return 如果成功返回RETCODE_OK，否则返回错误代码。
     */
    static	ReturnCode_t copy_from_topic_qos(DataWriterQos& datawriter_qos, const TopicQos& topic_qos);
#ifdef _XML_INTERFACE_CPP

    /**
     * @brief 指定QoS仓库的Qos配置，并创建数据写入器。
     * 
     * @param [in] topic 关联的主题。
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
     * @param [in] listener 数据写入器监听器。
     * @param [in] mask 设置应用程序感兴趣的状态，只有应用程序感兴趣的状态发生变化时，才会通知应用程序。
     * 
     * @return 创建成功返回数据写入器指针，否则返回NULL。
     */
    virtual DataWriter* create_datawriter_with_qos_profile(
        Topic* topic,
        const char* library_name, 
        const char* profile_name, 
        const char* qos_name, 
        DataWriterListener* listener, 
        StatusMask mask) = 0;

    /**
     * @brief 指定Qos仓库中的Qos设置为默认数据写入器Qos。
     * 
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
     * 
     * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t set_default_datawriter_qos_with_profile(
        const char* library_name, 
        const char* profile_name, 
        const char* qos_name) = 0;

    /**
     * @brief 指定QoS仓库的Qos配置，并设置发布者的QoS。
     * 
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
     * 
     * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t set_qos_with_profile(
        const char* library_name, 
        const char* profile_name, 
        const char* qos_name) = 0;

#endif // _XML_INTERFACE_CPP
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_PUBLISH_PUBLISHER_H
