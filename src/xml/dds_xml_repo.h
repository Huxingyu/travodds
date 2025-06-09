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

using namespace tinyxml2;

AGIDDS_NAMESPACE_BEGIN

#ifdef _XML_INTERFACE_CPP

class DDSXMLRepo
{
public:
    DDSXMLRepo();
    ~DDSXMLRepo();

    /**
     * @brief 从指定的配置文件中加载Qos配置。
     * 
     * @param xmlFilePath 
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
     * @param library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param qos_name QoS名称，允许为空，将转换为空字符串。
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
     * @param library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param qos_name QoS名称，允许为空，将转换为空字符串。
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
     * @param library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param qos_name QoS名称，允许为空，将转换为空字符串。
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
     * @param library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param qos_name QoS名称，允许为空，将转换为空字符串。
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
     * @param library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param qos_name QoS名称，允许为空，将转换为空字符串。
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
     * @param library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param qos_name QoS名称，允许为空，将转换为空字符串。
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
     * @param library_name QoS库名称，允许为NULL，将转换为空字符串。 
     * @param profile_name QoS配置名称，允许为空，将转换为空字符串。
     * @param qos_name QoS名称，允许为空，将转换为空字符串。
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
    bool parseQosProfile(XMLElement* element, const std::string& libraryName, const std::string& profileName);
    bool parseDomainParticipantFactoryQosProfile(XMLElement* element, DomainParticipantFactoryQos& qos, const std::string& libraryName, const std::string& profileName);
    bool parseDomainParticipantQosProfile(XMLElement* element, DomainParticipantQos& qos, const std::string& libraryName, const std::string& profileName);
    bool parsePublisherQosProfile(XMLElement* element, PublisherQos& qos, const std::string& libraryName, const std::string& profileName);
    bool parseSubscriberQosProfile(XMLElement* element, SubscriberQos& qos, const std::string& libraryName, const std::string& profileName);
    bool parseDataWriterQosProfile(XMLElement* element, DataWriterQos& qos, const std::string& libraryName, const std::string& profileName);
    bool parseDataReaderQosProfile(XMLElement* element, DataReaderQos& qos, const std::string& libraryName, const std::string& profileName);
    bool parseTopicQosProfile(XMLElement* element, TopicQos& qos, const std::string& libraryName, const std::string& profileName);

    static bool parseDomainParticipantFactoryQos(XMLElement* element, DomainParticipantFactoryQos& qos);
    static bool parseDomainParticipantQos(XMLElement* element, DomainParticipantQos& qos);
    static bool parsePublisherQos(XMLElement* element, PublisherQos& qos);
    static bool parseSubscriberQos(XMLElement* element, SubscriberQos& qos);
    static bool parseDataWriterQos(XMLElement* element, DataWriterQos& qos);
    static bool parseDataReaderQos(XMLElement* element, DataReaderQos& qos);
    static bool parseTopicQos(XMLElement* element, TopicQos& qos);

    static bool parseUserDataQosPolicy(XMLElement* element, UserDataQosPolicy& qos);
    static bool parseDurabilityQosPolicy(XMLElement* element, DurabilityQosPolicy& qos);
    static bool parsePresentationQosPolicy(XMLElement* element, PresentationQosPolicy& qos);
    static bool parseDeadlineQosPolicy(XMLElement* element, DeadlineQosPolicy& qos);
    static bool parseLatencyBudgetQosPolicy(XMLElement* element, LatencyBudgetQosPolicy& qos);
    static bool parseOwnershipQosPolicy(XMLElement* element, OwnershipQosPolicy& qos);
    static bool parseOwnershipStrengthQosPolicy(XMLElement* element, OwnershipStrengthQosPolicy& qos);
    static bool parseLivelinessQosPolicy(XMLElement* element, LivelinessQosPolicy& qos);
    static bool parseTimeBasedFilterQosPolicy(XMLElement* element, TimeBasedFilterQosPolicy& qos);
    static bool parsePartitionQosPolicy(XMLElement* element, PartitionQosPolicy& qos);
    static bool parseReliabilityQosPolicy(XMLElement* element, ReliabilityQosPolicy& qos);
    static bool parseDestinationOrderQosPolicy(XMLElement* element, DestinationOrderQosPolicy& qos);
    static bool parseHistoryQosPolicy(XMLElement* element, HistoryQosPolicy& qos);
    static bool parseResourceLimitsQosPolicy(XMLElement* element, ResourceLimitsQosPolicy& qos);
    static bool parseEntityFactoryQosPolicy(XMLElement* element, EntityFactoryQosPolicy& qos);
    static bool parseWriterDataLifecycleQosPolicy(XMLElement* element, WriterDataLifecycleQosPolicy& qos);
    static bool parseReaderDataLifecycleQosPolicy(XMLElement* element, ReaderDataLifecycleQosPolicy& qos);
    static bool parseTopicDataQosPolicy(XMLElement* element, TopicDataQosPolicy& qos);
    static bool parseGroupDataQosPolicy(XMLElement* element, GroupDataQosPolicy& qos);
    static bool parseTransportPriorityQosPolicy(XMLElement* element, TransportPriorityQosPolicy& qos);
    static bool parseLifespanQosPolicy(XMLElement* element, LifespanQosPolicy& qos);
    static bool parseDurabilityServiceQosPolicy(XMLElement* element, DurabilityServiceQosPolicy& qos);
    static bool parseDuration(XMLElement* element, Duration_t& duration);
    static bool parsePositiveIntegerUnlimited(XMLElement* element, int& value);
    static bool parseBase64Binary(XMLElement* element, Sequence<octet>& value);
    
     static void dpfQosDefault(DomainParticipantFactoryQos& qos) {
         qos.entity_factory.autoenable_created_entities = true;
     }

     static void dpQosDefault(DomainParticipantQos& qos) {
         qos.user_data.value.clear();
         qos.entity_factory.autoenable_created_entities = true;
     }

     static void publisherQosDefault(PublisherQos& qos) {
         qos.presentation.access_scope = INSTANCE_PRESENTATION_QOS;
         qos.presentation.coherent_access = false;
         qos.presentation.ordered_access = false;
         qos.partition.name.clear();
         qos.group_data.value.clear();
         qos.entity_factory.autoenable_created_entities = true;
     }

     static void subscriberQosDefault(SubscriberQos& qos) {
         qos.presentation.access_scope = INSTANCE_PRESENTATION_QOS;
         qos.presentation.coherent_access = false;
         qos.presentation.ordered_access = false;
         qos.partition.name.clear();
         qos.group_data.value.clear();
         qos.entity_factory.autoenable_created_entities = true;
     }

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

AGIDDS_NAMESPACE_END

#endif //_XML_INTERFACE_CPP

#endif // DDS_XML_LOADER_H