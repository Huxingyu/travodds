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
#ifndef TRAVODDS_DCPS_SUBSCRIBE_SUBSCRIBER_H
#define TRAVODDS_DCPS_SUBSCRIBE_SUBSCRIBER_H

#include "dcps/subscribe/datareader.h"
#include "dcps/listener/subscriberlistener.h"

TRAVODDS_NAMESPACE_BEGIN
class DomainParticipant;
/**
 * Subscriber类，包含允许用户控制消息接收的公共API。
 * 该类不应直接实例化。
 * DomainParticipant类应用于正确创建此元素。
 */
class DDS_DLL Subscriber : public virtual DomainEntity {
public:
    /**
     * @brief 析构函数。
     */
    virtual ~Subscriber() = default;

    /**
     * @brief 允许修改订阅者Qos。
     * SubscriberQos必须支持输入的Qos。
     *
     * @param qos SubscriberQos的新值。
     * 
     * @return RETCODE_IMMUTABLE_POLICY（如果无法更改任何Qos），
     * RETCODE_INCONSISTENT_POLICY（如果Qos不是自一致的），、
     * RETCODE_OK（如果Qos更改正确）。
     */
    virtual ReturnCode_t set_qos(const SubscriberQos& qos) = 0;

    /**
     * @brief 获取订阅服务器Qos。
     *
     * @param qos [in] 返回的SubscriberQos对象。
     * 
     * @return RETCODE_OK
     */
    virtual ReturnCode_t get_qos(SubscriberQos& qos) const = 0;

    /**
     * @brief 设置SubscriberListener。
     *
     * @param [in] listener SubscriberListener的新值。
     * @param [in] mask     保存监听器响应的状态StatusMask位掩码。
     * 
     * @return RETCODE_OK
     */
    virtual ReturnCode_t set_listener(SubscriberListener* listener, const StatusMask& mask) = 0;

    /**
     * @brief 获取附加的SubscriberListener。
     *
     * @return 指向SubscriberListener的指针。
     */
    virtual const SubscriberListener* get_listener() const = 0;

    /**
     * @brief 此函数创建DataReader。返回属于当前订阅者的DataReader。
     *
     * @param [in] topic DataReader将要监听的主题。
     * @param [in] qos DataReader的Qos策略。
     * @param [in] listener 指向监听器的指针。
     * @param [in] mask 保存监听器响应的状态的StatusMask位掩码。
     * 
     * @return 指向创建的DataReader的指针。如果失败，则返回nullptr。
     */
    virtual DataReader* create_datareader(TopicDescription* topic, const DataReaderQos& qos, DataReaderListener* listener, const StatusMask& mask) = 0;

    /**
     * @brief 此函数删除属于订阅服务器的DataReader。
     *
     * 必须对用于创建datareader的同一订阅服务器对象调用delete_datareader操作。
     * 如果在其他订阅服务器上调用delete_datareader，则该操作将无效，并将返回错误。
     *
     * @param reader [in] 要删除的DataReader。
     * 
     * @return 如果数据读取器不属于此订阅者返回RETCODE_PRECONDITION_NOT_MET，
     * 如果它被正确删除返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t delete_datareader(DataReader*& dataReader) = 0;

    /**
     * @brief 此函数获取以前创建的属于订阅者的DataReader，该订阅者附加到具有匹配topic_name的主题。
     * 如果不存在这样的DataReader，则函数将返回nullptr。
     *
     * 如果当前订阅者存在多个DataReader满足此条件，则函数将返回其中一个。没有指定哪一个。
     *
     * @param topic_name [in] 与DataReader关联的主题的名称。
     * 
     * @return 指向以前在具有该topic_name的主题上创建的DataReader的指针。
     */
    virtual DataReader* lookup_datareader(const std::string& topic_name) const = 0;

    /**
     * @brief 指示应用程序将要访问附加到订阅者的任何DataReader对象中的数据样本。
     *
     * @return RETCODE_OK
     *
     * @warning 尚不支持。当前返回RETCODE_UNSUPPORTED
     */
    virtual ReturnCode_t begin_access() = 0;

    /**
     * @brief 指示应用程序已完成访问订阅服务器管理的DataReader对象中的数据样本。
     *
     * @return RETCODE_OK
     *
     * @warning 尚不支持。当前返回RETCODE_UNSUPPORTED
     */
    virtual ReturnCode_t end_access() = 0;

    /**
     * @brief 此函数允许应用程序访问包含具有指定sample_states、view_states和instance_states的样本的DataReader对象。
     *
     * @param [out] readers         返回存有读取器列表的DataReader的容器。
     * @param [in]  sample_states   SampleStateKind位掩码。
     * @param [in]  view_states     ViewStateKind位掩码。
     * @param [in]  instance_states InstanceStateKind位掩码。
     * 
     * @return RETCODE_OK
     */
    virtual ReturnCode_t get_datareaders(DataReaderSeq& readers, const SampleStateMask& sample_states, const ViewStateMask& view_states, const InstanceStateMask& instance_states) const = 0;

    /**
     * @brief 此函数调用当前订阅者包含的DataReader实体的DataReaderListener对象上的on_data_available操作。
     *
     * 此函数通常从SubscriberListener中的on_data_on_readers操作调用。
     * 这样SubscriberListener可以将数据处理委托给DataReaderListener。
     *
     * @return RETCODE_OK
     */
    virtual ReturnCode_t notify_datareaders() const = 0;

    /**
     * @brief 此函数返回订阅者所属的DomainParticipant。
     *
     * @return DomainParticipant指针。
     */
    virtual const DomainParticipant* get_participant() const = 0;

    /**
     * @brief 删除所有包含的DataReader。
     * 如果DataReader具有任何QueryCondition或ReadCondition，则在DataReader本身之前删除它们。
     *
     * @return 如果成功返回RETCODE_OK，否则返回错误代码。
     */
    virtual ReturnCode_t delete_contained_entities() = 0;

    /**
     * @brief 此函数设置DataReader QoS策略的默认值，该值将用于新创建的DataReader实体，
     * 前提是在create_datareader操作中默认了QoS政策。
     *
     * 此函数将检查生成的策略是否自一致；如果它们不是，则操作将无效，并返回false。
     *
     * 可以将特殊值DATAREADER_QOS_DEFAULT传递给该操作，
     * 以标识应将默认QOS重置回原来将使用的初始值，
     * 即从未调用set_default_datareader_qos操作时将使用的值。
     *
     * @param [in] qos 要设置为默认值的DataReaderQos的新值。
     * 
     * @return RETCODE_INCONSISTENT_POLICY（如果Qos不是自一致的），
     * RETCODE_OK（如果Qos更改正确）。
     */
    virtual ReturnCode_t set_default_datareader_qos(const DataReaderQos& Qos) = 0;

    /**
     * @brief 此函数获取DataReader QoS的默认值，即在create_datareader操作中默认QoS策略的情况下，
     * 将用于新创建的DataReader实体的QoS政策。
     *
     * 获取到的值get_default_datareader_qos将与上次成功
     * 调用get_default_datareader_qos时指定的值集匹配，否则，如果从未进行调用，则为默认值。
     *
     * @param [in] qos 返回default_qos对应的DataReaderQos。
     * 
     * @return RETCODE_OK
     */
    virtual ReturnCode_t get_default_datareader_qos(DataReaderQos& Qos) const = 0;

    /**
     * @brief 将TopicQos复制到相应的DataReaderQos中。
     *
     * @param [in, out] reader_qos
     * @param [in] topic_qos
     * @return 如果成功返回RETCODE_OK，否则返回错误代码。
     */
    static ReturnCode_t copy_from_topic_qos(DataReaderQos& datareader_qos, const TopicQos& topic_qos);
#ifdef _XML_INTERFACE_CPP

    /**
     * @brief 指定QoS仓库的Qos配置，并创建数据读取器。
     * 
     * @param [in] topic        关联的主题。
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name     QoS名称，允许为空，将转换为空字符串。
     * @param [in] listener     数据读取器监听器。
     * @param [in] mask         设置应用程序感兴趣的状态，只有应用程序感兴趣的状态发生变化时，才会通知应用程序。
     * 
     * @return 创建成功返回数据读取器指针，否则返回NULL。
     */
    virtual DataReader* create_datareader_with_qos_profile(
        TopicDescription* topic,
        const char* library_name, 
        const char* profile_name, 
        const char* qos_name, 
        DataReaderListener* listener, 
        StatusMask mask) = 0;

    /**
     * @brief 指定Qos仓库中的Qos设置为默认数据读取器Qos。
     * 
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name     QoS名称，允许为空，将转换为空字符串。
     * 
     * @return 设置成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t set_default_datareader_qos_with_profile(
        const char* library_name, 
        const char* profile_name, 
        const char* qos_name) = 0;

    /**
     * @brief 指定QoS仓库的Qos配置，并设置订阅者的QoS。
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
        const char* qos_name) = 0;
#endif // _XML_INTERFACE_CPP
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_SUBSCRIBE_SUBSCRIBER_H
