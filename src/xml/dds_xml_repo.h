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

#ifndef DDS_XML_LOADER_H
#define DDS_XML_LOADER_H

#include <string>
#include <map>
#include "qos/qos.h"
#include "dcps/base/retcode.h"
#include "xml/tinyxml2.h"
#ifdef DDSSECURITY
#include "security/propertyqospolicy.h"
#endif // DDSSECURITY

using namespace tinyxml2;

TRAVODDS_NAMESPACE_BEGIN

#ifdef _XML_INTERFACE_CPP

class DDSXMLRepo
{
public:
    DDSXMLRepo();
    ~DDSXMLRepo();

    /**
     * @brief 从指定的配置文件中加载Qos配置。
     * 
     * @param [in] xmlFilePath 配置文件路径。
     * 
     * @return 成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    ReturnCode_t load(const std::string& xmlFilePath);

    /**
     * @brief 清空已经加载的Qos配置。
     * 
     * @return 成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    ReturnCode_t clear();

    /**
     * @brief 获取域参与者工厂Qos配置。
     * 
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
     * @param [in, out] qos 出口参数，获取到的Qos。
     * 
     * @return 成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    ReturnCode_t lookup_participant_factory_qos(
        const std::string& library_name, 
        const std::string& profile_name, 
        const std::string& qos_name, 
        DomainParticipantFactoryQos& qos);
    
    /**
     * @brief 获取域参与者Qos配置。
     * 
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
     * @param [in, out] qos 出口参数，获取到的Qos。
     * 
     * @return 成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    ReturnCode_t lookup_participant_qos(
        const std::string& library_name, 
        const std::string& profile_name, 
        const std::string& qos_name, 
        DomainParticipantQos& qos);

    /**
     * @brief 获取发布者Qos配置。
     * 
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
     * @param [in, out] qos 出口参数，获取到的Qos。
     * 
     * @return 成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    ReturnCode_t lookup_publisher_qos(
        const std::string& library_name, 
        const std::string& profile_name, 
        const std::string& qos_name, 
        PublisherQos& qos);

    /**
     * @brief 获取订阅者Qos配置。
     * 
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
     * @param [in, out] qos 出口参数，获取到的Qos。
     * 
     * @return 成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    ReturnCode_t lookup_subscriber_qos(
        const std::string& library_name, 
        const std::string& profile_name, 
        const std::string& qos_name, 
        SubscriberQos& qos);
    
    /**
     * @brief 获取数据写入者Qos配置。
     * 
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
     * @param [in, out] qos 出口参数，获取到的Qos。
     * 
     * @return 成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */

    ReturnCode_t lookup_datawriter_qos(
        const std::string& library_name, 
        const std::string& profile_name, 
        const std::string& qos_name, 
        DataWriterQos& qos);

    /**
     * @brief 获取数据读取者Qos配置。
     * 
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
     * @param [in, out] qos 出口参数，获取到的Qos。
     * 
     * @return 成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */

    ReturnCode_t lookup_datareader_qos(
        const std::string& library_name, 
        const std::string& profile_name, 
        const std::string& qos_name, 
        DataReaderQos& qos);
    
    /**
     * @brief 获取主题Qos配置。
     * 
     * @param [in] library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param [in] profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] qos_name QoS名称，允许为空，将转换为空字符串。
     * @param [in, out] qos 出口参数，获取到的Qos。
     * 
     * @return 成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
    ReturnCode_t lookup_topic_qos(
        const std::string& library_name, 
        const std::string& profile_name, 
        const std::string& qos_name, 
        TopicQos& qos);

private:
    /** @brief key: library_name + "::" profile_name + "::" qos_name */
    std::map<std::string, DomainParticipantFactoryQos> m_participantFactoryQosMap;
    std::map<std::string, DomainParticipantQos> m_participantQosMap;
    std::map<std::string, PublisherQos> m_publisherQosMap;
    std::map<std::string, SubscriberQos> m_subscriberQosMap;
    std::map<std::string, DataWriterQos> m_dataWriterQosMap;
    std::map<std::string, DataReaderQos> m_dataReaderQosMap;
    std::map<std::string, TopicQos> m_topicQosMap;
private:
    /**
     * @brief 解析配置文件Qos标签获取主题Qos配置。
     *
     * @param [in] element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in] libraryName QoS配置名称，允许为空，将转换为空字符串。
     * @param [in] profileName QoS名称，允许为空，将转换为空字符串。
     *
     * @return 成功返回true，否则返回false。
     */
    bool parseQosProfile(XMLElement* element, const std::string& libraryName, const std::string& profileName);
    /**
     * @brief 解析配置文件DomainParticipantFactoryQos标签获取Qos配置并将配置信息存储在对应Qos列表中。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     * @param [in]      libraryName QoS配置名称，允许为空，将转换为空字符串。
     * @param [in]      profileName QoS名称，允许为空，将转换为空字符串。
     *
     * @return 成功返回true，否则返回false。
     */
    bool parseDomainParticipantFactoryQosProfile(XMLElement* element, DomainParticipantFactoryQos& qos, const std::string& libraryName, const std::string& profileName);
    /**
     * @brief 解析配置文件DomainParticipantQos标签获取Qos配置并将配置信息存储在对应Qos列表中。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     * @param [in]      libraryName QoS配置名称，允许为空，将转换为空字符串。
     * @param [in]      profileName QoS名称，允许为空，将转换为空字符串。
     *
     * @return 成功返回true，否则返回false。
     */
    bool parseDomainParticipantQosProfile(XMLElement* element, DomainParticipantQos& qos, const std::string& libraryName, const std::string& profileName);
    /**
     * @brief 解析配置文件PublisherQos标签获取Qos配置并将配置信息存储在对应Qos列表中。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     * @param [in]      libraryName QoS配置名称，允许为空，将转换为空字符串。
     * @param [in]      profileName QoS名称，允许为空，将转换为空字符串。
     *
     * @return 成功返回true，否则返回false。
     */
    bool parsePublisherQosProfile(XMLElement* element, PublisherQos& qos, const std::string& libraryName, const std::string& profileName);
    /**
     * @brief 解析配置文件SubscriberQos标签获取Qos配置并将配置信息存储在对应Qos列表中。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     * @param [in]      libraryName QoS配置名称，允许为空，将转换为空字符串。
     * @param [in]      profileName QoS名称，允许为空，将转换为空字符串。
     *
     * @return 成功返回true，否则返回false。
     */
    bool parseSubscriberQosProfile(XMLElement* element, SubscriberQos& qos, const std::string& libraryName, const std::string& profileName);
    /**
     * @brief 解析配置文件DataWriterQos标签获取Qos配置并将配置信息存储在对应Qos列表中。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     * @param [in]      libraryName QoS配置名称，允许为空，将转换为空字符串。
     * @param [in]      profileName QoS名称，允许为空，将转换为空字符串。
     *
     * @return 成功返回true，否则返回false。
     */
    bool parseDataWriterQosProfile(XMLElement* element, DataWriterQos& qos, const std::string& libraryName, const std::string& profileName);
    /**
     * @brief 解析配置文件DataReaderQos标签获取Qos配置并将配置信息存储在对应Qos列表中。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     * @param [in]      libraryName QoS配置名称，允许为空，将转换为空字符串。
     * @param [in]      profileName QoS名称，允许为空，将转换为空字符串。
     *
     * @return 成功返回true，否则返回false。
     */
    bool parseDataReaderQosProfile(XMLElement* element, DataReaderQos& qos, const std::string& libraryName, const std::string& profileName);
    /**
     * @brief 解析配置文件TopicQos标签获取Qos配置并将配置信息存储在对应Qos列表中。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     * @param [in]      libraryName QoS配置名称，允许为空，将转换为空字符串。
     * @param [in]      profileName QoS名称，允许为空，将转换为空字符串。
     *
     * @return 成功返回true，否则返回false。
     */
    bool parseTopicQosProfile(XMLElement* element, TopicQos& qos, const std::string& libraryName, const std::string& profileName);

    /**
     * @brief 遍历配置文件DomainParticipantFactoryQos标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseDomainParticipantFactoryQos(XMLElement* element, DomainParticipantFactoryQos& qos);
    /**
     * @brief 遍历配置文件DomainParticipantQos标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseDomainParticipantQos(XMLElement* element, DomainParticipantQos& qos);
    /**
     * @brief 遍历配置文件PublisherQos标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parsePublisherQos(XMLElement* element, PublisherQos& qos);
    /**
     * @brief 遍历配置文件SubscriberQos标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseSubscriberQos(XMLElement* element, SubscriberQos& qos);
    /**
     * @brief 遍历配置文件DataWriterQos标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseDataWriterQos(XMLElement* element, DataWriterQos& qos);
    /**
     * @brief 遍历配置文件DataReaderQos标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseDataReaderQos(XMLElement* element, DataReaderQos& qos);
    /**
     * @brief 遍历配置文件TopicQos标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseTopicQos(XMLElement* element, TopicQos& qos);
	
    /**
     * @brief 遍历配置文件的element所有子节点获取Property_t。
     *
     * @param [in]      element     。
     * @param [in, out] property         出口参数，获取到的Property_t。
     *
     * @return 成功返回true，否则返回false。
     */
	static bool parseProperty(XMLElement* element, Property_t& property);

    /**
     * @brief 遍历配置文件的element所有子节点获取BinaryProperty_t。
     *
     * @param [in]      element     。
     * @param [in, out] binaryProperty         出口参数，获取到的BinaryProperty_t。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseBinaryProperty(XMLElement* element, BinaryProperty_t& binaryProperty);

    /**
     * @brief 遍历配置文件UserDataQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseUserDataQosPolicy(XMLElement* element, UserDataQosPolicy& qos);
	
	static bool parsePropertySeq(XMLElement* element, PropertyQosPolicy& qos);

    static bool parseBinaryPropertySeq(XMLElement* element, PropertyQosPolicy& qos);

    static bool parsePropertyQosPolicy(XMLElement* element, PropertyQosPolicy& qos);
	
    /**
     * @brief 遍历配置文件DurabilityQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseDurabilityQosPolicy(XMLElement* element, DurabilityQosPolicy& qos);
    /**
     * @brief 遍历配置文件PresentationQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parsePresentationQosPolicy(XMLElement* element, PresentationQosPolicy& qos);
    /**
     * @brief 遍历配置文件DeadlineQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseDeadlineQosPolicy(XMLElement* element, DeadlineQosPolicy& qos);
    /**
     * @brief 遍历配置文件LatencyBudgetQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseLatencyBudgetQosPolicy(XMLElement* element, LatencyBudgetQosPolicy& qos);
    /**
     * @brief 遍历配置文件OwnershipQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseOwnershipQosPolicy(XMLElement* element, OwnershipQosPolicy& qos);
    /**
     * @brief 遍历配置文件OwnershipStrengthQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseOwnershipStrengthQosPolicy(XMLElement* element, OwnershipStrengthQosPolicy& qos);
    /**
     * @brief 遍历配置文件LivelinessQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseLivelinessQosPolicy(XMLElement* element, LivelinessQosPolicy& qos);
    /**
     * @brief 遍历配置文件TimeBasedFilterQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseTimeBasedFilterQosPolicy(XMLElement* element, TimeBasedFilterQosPolicy& qos);
    /**
     * @brief 遍历配置文件PartitionQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parsePartitionQosPolicy(XMLElement* element, PartitionQosPolicy& qos);
    /**
     * @brief 遍历配置文件ReliabilityQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseReliabilityQosPolicy(XMLElement* element, ReliabilityQosPolicy& qos);
    /**
     * @brief 遍历配置文件DestinationOrderQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseDestinationOrderQosPolicy(XMLElement* element, DestinationOrderQosPolicy& qos);
    /**
     * @brief 遍历配置文件HistoryQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseHistoryQosPolicy(XMLElement* element, HistoryQosPolicy& qos);
    /**
     * @brief 遍历配置文件ResourceLimitsQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseResourceLimitsQosPolicy(XMLElement* element, ResourceLimitsQosPolicy& qos);
    /**
     * @brief 遍历配置文件EntityFactoryQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseEntityFactoryQosPolicy(XMLElement* element, EntityFactoryQosPolicy& qos);
    /**
     * @brief 遍历配置文件WriterDataLifecycleQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseWriterDataLifecycleQosPolicy(XMLElement* element, WriterDataLifecycleQosPolicy& qos);
    /**
     * @brief 遍历配置文件ReaderDataLifecycleQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseReaderDataLifecycleQosPolicy(XMLElement* element, ReaderDataLifecycleQosPolicy& qos);
    /**
     * @brief 遍历配置文件TopicDataQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseTopicDataQosPolicy(XMLElement* element, TopicDataQosPolicy& qos);
    /**
     * @brief 遍历配置文件GroupDataQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseGroupDataQosPolicy(XMLElement* element, GroupDataQosPolicy& qos);
    /**
     * @brief 遍历配置文件TransportPriorityQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseTransportPriorityQosPolicy(XMLElement* element, TransportPriorityQosPolicy& qos);
    /**
     * @brief 遍历配置文件LifespanQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseLifespanQosPolicy(XMLElement* element, LifespanQosPolicy& qos);
    /**
     * @brief 遍历配置文件DurabilityServiceQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseDurabilityServiceQosPolicy(XMLElement* element, DurabilityServiceQosPolicy& qos);
    /**
     * @brief 遍历配置文件TransportQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
	 */
	static bool parseTransportQosPolicy(XMLElement* element, TransportQosPolicy& qos);
    /**
     * @brief 遍历配置文件DiscoveredMethodQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
	 */
	static bool parseDiscoveredMethodQosPolicy(XMLElement* element, DiscoveredMethodQosPolicy& qos);
    /**
     * @brief 遍历配置文件NetworkInterfaceQosPolicy标签所有子节点获取Qos配置。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] qos         出口参数，获取到的Qos。
     *
     * @return 成功返回true，否则返回false。
	 */
	static bool parseNetworkInterfaceQosPolicy(XMLElement* element, NetworkInterfaceQosPolicy& qos);
    /**
     * @brief 遍历配置文件Duration_t标签节点值获取。
     *
     * @param [in]      element     XML配置文件句柄。
     * @param [in, out] duration    Duration_t格式时间值。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseDuration(XMLElement* element, Duration_t& duration);
    /**
     * @brief 遍历配置文件整数标签子节点值配置获取。
     *
     * @param [in]      element     XML配置文件句柄。
     * @param [in, out] value       获取到的整数值。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parsePositiveIntegerUnlimited(XMLElement* element, int& value);
    /**
     * @brief 遍历配置文件base64编码的字符串和hex编码的字符串节点值获取。
     *
     * @param [in]      element     XML配置文件句柄，允许为NULL，将转换为空字符串。
     * @param [in, out] value       字符序列。
     *
     * @return 成功返回true，否则返回false。
     */
    static bool parseBase64Binary(XMLElement* element, Sequence<octet>& value);

    /**
     * @brief DomainParticipantFactoryQos初始化。
     *
     * @param [in, out] qos 要初始化的Qos。
     */
     static void dpfQosDefault(DomainParticipantFactoryQos& qos) {
         qos.entity_factory.autoenable_created_entities = true;
     }

     /**
      * @brief DomainParticipantQos初始化。
      *
      * @param [in, out] qos 要初始化的Qos。
      */
     static void dpQosDefault(DomainParticipantQos& qos) {
         qos.user_data.value.clear();
         qos.entity_factory.autoenable_created_entities = true;
     }

     /**
      * @brief PublisherQos初始化。
      *
      * @param [in, out] qos 要初始化的Qos。
      */
     static void publisherQosDefault(PublisherQos& qos) {
         qos.presentation.access_scope = INSTANCE_PRESENTATION_QOS;
         qos.presentation.coherent_access = false;
         qos.presentation.ordered_access = false;
         qos.partition.name.clear();
         qos.group_data.value.clear();
         qos.entity_factory.autoenable_created_entities = true;
     }

     /**
      * @brief SubscriberQos初始化。
      *
      * @param [in, out] qos 要初始化的Qos。
      */
     static void subscriberQosDefault(SubscriberQos& qos) {
         qos.presentation.access_scope = INSTANCE_PRESENTATION_QOS;
         qos.presentation.coherent_access = false;
         qos.presentation.ordered_access = false;
         qos.partition.name.clear();
         qos.group_data.value.clear();
         qos.entity_factory.autoenable_created_entities = true;
     }

     /**
      * @brief TopicQos初始化。
      *
      * @param [in, out] qos 要初始化的Qos。
      */
     static void topicQosDefault(TopicQos& qos) {
         qos.topic_data.value.clear();
         qos.durability.kind = VOLATILE_DURABILITY_QOS;
         qos.durability_service.service_cleanup_delay = TIME_ZERO;
         qos.durability_service.history_kind = KEEP_LAST_HISTORY_QOS;
         qos.durability_service.history_depth = 1;
         qos.durability_service.max_samples = LENGTH_UNLIMITED;
         qos.durability_service.max_instances = LENGTH_UNLIMITED;
         qos.durability_service.max_samples_per_instance = LENGTH_UNLIMITED;
         qos.deadline.period = TIME_INFINITE;
         qos.latency_budget.duration = TIME_ZERO;
         qos.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;
         qos.liveliness.lease_duration = TIME_INFINITE;
         qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
         qos.reliability.max_blocking_time = {0, 100000000};
         qos.destination_order.kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
         qos.history.kind = KEEP_LAST_HISTORY_QOS;
         qos.history.depth = 1;
         qos.resource_limits.max_samples = LENGTH_UNLIMITED;
         qos.resource_limits.max_instances = LENGTH_UNLIMITED;
         qos.resource_limits.max_samples_per_instance = LENGTH_UNLIMITED;
         qos.transport_priority.value = 0;
         qos.lifespan.duration = TIME_INFINITE;
         qos.ownership.kind = SHARED_OWNERSHIP_QOS;
     }

     /**
      * @brief DataWriterQos初始化。
      *
      * @param [in, out] qos 要初始化的Qos。
      */
     static void datawriterQosDefault(DataWriterQos& qos) {
         qos.durability.kind = VOLATILE_DURABILITY_QOS;
         qos.durability_service.service_cleanup_delay = TIME_ZERO;
         qos.durability_service.history_kind = KEEP_LAST_HISTORY_QOS;
         qos.durability_service.history_depth = 1;
         qos.durability_service.max_samples = LENGTH_UNLIMITED;
         qos.durability_service.max_instances = LENGTH_UNLIMITED;
         qos.durability_service.max_samples_per_instance = LENGTH_UNLIMITED;
         qos.deadline.period = TIME_INFINITE;
         qos.latency_budget.duration = TIME_ZERO;
         qos.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;
         qos.liveliness.lease_duration = TIME_INFINITE;
         qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
         qos.reliability.max_blocking_time = {0, 100000000};
         qos.destination_order.kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
         qos.history.kind = KEEP_LAST_HISTORY_QOS;
         qos.history.depth = 1;
         qos.resource_limits.max_samples = LENGTH_UNLIMITED;
         qos.resource_limits.max_instances = LENGTH_UNLIMITED;
         qos.resource_limits.max_samples_per_instance = LENGTH_UNLIMITED;
         qos.transport_priority.value = 0;
         qos.lifespan.duration = TIME_INFINITE;
         qos.user_data.value.clear();
         qos.ownership.kind = SHARED_OWNERSHIP_QOS;
         qos.ownership_strength.value = 0;
         qos.writer_data_lifecycle.autodispose_unregistered_instances = true;
     }

     /**
      * @brief DataReaderQos初始化。
      *
      * @param [in, out] qos 要初始化的Qos。
      */
     static void datareaderQosDefault(DataReaderQos& qos) {
         qos.durability.kind = VOLATILE_DURABILITY_QOS;
         qos.deadline.period = TIME_INFINITE;
         qos.latency_budget.duration = TIME_ZERO;
         qos.liveliness.kind = AUTOMATIC_LIVELINESS_QOS;
         qos.liveliness.lease_duration = TIME_INFINITE;
         qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
         qos.reliability.max_blocking_time = {0, 100000000};
         qos.destination_order.kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
         qos.history.kind = KEEP_LAST_HISTORY_QOS;
         qos.history.depth = 1;
         qos.resource_limits.max_samples = 5000;
         qos.resource_limits.max_instances = 10;
         qos.resource_limits.max_samples_per_instance = 400;
         qos.user_data.value.clear();
         qos.ownership.kind = SHARED_OWNERSHIP_QOS;
         qos.time_based_filter.minimum_separation = TIME_ZERO;
         qos.reader_data_lifecycle.autopurge_nowriter_samples_delay = TIME_INFINITE;
         qos.reader_data_lifecycle.autopurge_disposed_samples_delay = TIME_INFINITE;
     }
};

TRAVODDS_NAMESPACE_END

#endif //_XML_INTERFACE_CPP

#endif // DDS_XML_LOADER_H