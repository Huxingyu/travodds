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

#include "xml/dds_xml_repo.h"
#include "common/log/logger.h"
#include <iostream>
#include <string>

TRAVODDS_NAMESPACE_BEGIN

#ifdef _XML_INTERFACE_CPP

DDSXMLRepo::DDSXMLRepo() {
    // Constructor implementation
}

DDSXMLRepo::~DDSXMLRepo() {
    // Destructor implementation
}

ReturnCode_t DDSXMLRepo::load(const std::string& xmlFilePath) {
    XMLDocument doc(true, Whitespace::COLLAPSE_WHITESPACE);
    XMLError result = doc.LoadFile(xmlFilePath.c_str());
    if (result != XML_SUCCESS) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to load XML file %s\n", xmlFilePath.c_str());
        return RETCODE_ERROR;
    }

    XMLElement* root = doc.RootElement();
    if (!root) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to get root element\n");
        return RETCODE_ERROR;
    }

    // 判断根节点必须为dds_qos
    if (strcmp(root->Name(), "dds_qos") != 0) {
        TRAVODDS_LOG(LOG_ERROR, "Root element must be dds_qos\n");
        return RETCODE_ERROR;
    }
    // Parse QoS configurations
    for (XMLElement* library = root->FirstChildElement(); library; library = library->NextSiblingElement("qos_library")) {
        // 只允许qos_library子节点
        if (strcmp(library->Name(), "qos_library") != 0) {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Only qos_library is allowed\n", library->GetLineNum());
            return RETCODE_ERROR;
        }
        const char* libraryName = library->Attribute("name");
        if (!libraryName) {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Missing library name\n", library->GetLineNum());
            return RETCODE_ERROR;
        }
        // Parse QoS profiles
        for (XMLElement* profile = library->FirstChildElement("qos_profile"); profile; profile = profile->NextSiblingElement("qos_profile")) {
            const char* profileName = profile->Attribute("name");
            if (!profileName) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Missing profile name\n", profile->GetLineNum());
                return RETCODE_ERROR;
            }
            if (!parseQosProfile(profile, libraryName, profileName)) {
                return RETCODE_ERROR;
            }
        }
        // Parse direct QoS configurations
        if (!parseQosProfile(library, libraryName, "default")) {
            return RETCODE_ERROR;
        }
    }

    return RETCODE_OK;
}

bool DDSXMLRepo::parseQosProfile(XMLElement* profile, const std::string& libraryName, const std::string& profileName) {
    // Parse DomainParticipantFactoryQos
    for (XMLElement* element = profile->FirstChildElement(); element; element = element->NextSiblingElement()) {
        // 根据子节点名称判断Qos类型
        const char* nodeName = element->Name();
        // 排除已经处理过的qos_profile节点
        if (strcmp(nodeName, "qos_profile") == 0) {
            continue;
        }
        if (strcmp(nodeName, "domain_participant_factory_qos") == 0) {
            DomainParticipantFactoryQos qos;
            dpfQosDefault(qos);
            if (!parseDomainParticipantFactoryQosProfile(element, qos, libraryName, profileName))
            {
                return false;
            }
        } else if (strcmp(nodeName, "domain_participant_qos") == 0) {
            DomainParticipantQos qos;
            dpQosDefault(qos);
            if (!parseDomainParticipantQosProfile(element, qos, libraryName, profileName)) {
                return false;
            }
        } else if (strcmp(nodeName, "publisher_qos") == 0) {
            PublisherQos qos;
            publisherQosDefault(qos);
            if (!parsePublisherQosProfile(element, qos, libraryName, profileName)) {
                return false;
            }
        } else if (strcmp(nodeName, "subscriber_qos") == 0) {
            SubscriberQos qos;
            subscriberQosDefault(qos);
            if (!parseSubscriberQosProfile(element, qos, libraryName, profileName)) {
                return false;
            }
        } else if (strcmp(nodeName, "datawriter_qos") == 0) {
            DataWriterQos qos;
            datawriterQosDefault(qos);
            if (!parseDataWriterQosProfile(element, qos, libraryName, profileName)) {
                return false;
            }
        } else if (strcmp(nodeName, "datareader_qos") == 0) {
            DataReaderQos qos;
            datareaderQosDefault(qos);
            if (!parseDataReaderQosProfile(element, qos, libraryName, profileName)) {
                return false;
            }
        } else if (strcmp(nodeName, "topic_qos") == 0) {
            TopicQos qos;
            topicQosDefault(qos);
            if (!parseTopicQosProfile(element, qos, libraryName, profileName)) {
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown Qos type %s\n", element->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseDomainParticipantFactoryQosProfile(XMLElement* element, DomainParticipantFactoryQos& qos, const std::string& libraryName, const std::string& profileName) {
    // name
    std::string qosNameStr = "default";
    const char* qosName = element->Attribute("name");
    if (qosName) {
        qosNameStr = qosName;
    }
    std::string key = libraryName + "::" + profileName + "::" + qosNameStr;
    // base_name
    const char* baseName = element->Attribute("base_name");
    if (baseName) {
        // 可能的base_name集合
        std::vector<std::string> baseNames;
        // 完整的base_name
        baseNames.push_back(baseName);
        // 省略library_name
        baseNames.push_back(libraryName + "::" + baseName);
        // 省略了library_name和profile_name
        baseNames.push_back(libraryName + "::" + profileName + "::" + baseName);
        bool found = false;
        for (const auto& base : baseNames) {
            auto it = m_participantFactoryQosMap.find(base);
            if (it != m_participantFactoryQosMap.end()) {
                qos = it->second;
                found = true;
                break;
            }
        }
        if (!found) {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to find base_name %s\n", element->GetLineNum(), baseName);
            return false;
        }
    }
    bool ret = parseDomainParticipantFactoryQos(element, qos);
    if (!ret)
    {
        TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DomainParticipantFactoryQos in library %s profile %s\n", element->GetLineNum(), libraryName.c_str(), profileName.c_str());
        return RETCODE_ERROR;
    }
    m_participantFactoryQosMap[key] = qos;
    return true;
}

bool DDSXMLRepo::parseDomainParticipantQosProfile(XMLElement* element, DomainParticipantQos& qos, const std::string& libraryName, const std::string& profileName) {
    // name
    std::string qosNameStr = "default";
    const char* qosName = element->Attribute("name");
    if (qosName) {
        qosNameStr = qosName;
    }
    std::string key = libraryName + "::" + profileName + "::" + qosNameStr;
    // base_name
    const char* baseName = element->Attribute("base_name");
    if (baseName) {
        // 可能的base_name集合
        std::vector<std::string> baseNames;
        // 完整的base_name
        baseNames.push_back(baseName);
        // 省略library_name
        baseNames.push_back(libraryName + "::" + baseName);
        // 省略了library_name和profile_name
        baseNames.push_back(libraryName + "::" + profileName + "::" + baseName);
        bool found = false;
        for (const auto& base : baseNames) {
            auto it = m_participantQosMap.find(base);
            if (it != m_participantQosMap.end()) {
                qos = it->second;
                found = true;
                break;
            }
        }
        if (!found) {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to find base_name %s\n", element->GetLineNum(), baseName);
            return false;
        }
    }
    bool ret = parseDomainParticipantQos(element, qos);
    if (!ret)
    {
        TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DomainParticipantQos in library %s profile %s\n", element->GetLineNum(), libraryName.c_str(), profileName.c_str());
        return RETCODE_ERROR;
    }
    m_participantQosMap[key] = qos;
    return true;
}

bool DDSXMLRepo::parsePublisherQosProfile(XMLElement* element, PublisherQos& qos, const std::string& libraryName, const std::string& profileName) {
    // name
    std::string qosNameStr = "default";
    const char* qosName = element->Attribute("name");
    if (qosName) {
        qosNameStr = qosName;
    }
    std::string key = libraryName + "::" + profileName + "::" + qosNameStr;
    // base_name
    const char* baseName = element->Attribute("base_name");
    if (baseName) {
        // 可能的base_name集合
        std::vector<std::string> baseNames;
        // 完整的base_name
        baseNames.push_back(baseName);
        // 省略library_name
        baseNames.push_back(libraryName + "::" + baseName);
        // 省略了library_name和profile_name
        baseNames.push_back(libraryName + "::" + profileName + "::" + baseName);
        bool found = false;
        for (const auto& base : baseNames) {
            auto it = m_publisherQosMap.find(base);
            if (it != m_publisherQosMap.end()) {
                qos = it->second;
                found = true;
                break;
            }
        }
        if (!found) {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to find base_name %s\n", element->GetLineNum(), baseName);
            return false;
        }
    }
    bool ret = parsePublisherQos(element, qos);
    if (!ret)
    {
        TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse PublisherQos in library %s profile %s\n", element->GetLineNum(), libraryName.c_str(), profileName.c_str());
        return RETCODE_ERROR;
    }
    m_publisherQosMap[key] = qos;
    return true;
}

bool DDSXMLRepo::parseSubscriberQosProfile(XMLElement* element, SubscriberQos& qos, const std::string& libraryName, const std::string& profileName) {
    // name
    std::string qosNameStr = "default";
    const char* qosName = element->Attribute("name");
    if (qosName) {
        qosNameStr = qosName;
    }
    std::string key = libraryName + "::" + profileName + "::" + qosNameStr;
    // base_name
    const char* baseName = element->Attribute("base_name");
    if (baseName) {
        // 可能的base_name集合
        std::vector<std::string> baseNames;
        // 完整的base_name
        baseNames.push_back(baseName);
        // 省略library_name
        baseNames.push_back(libraryName + "::" + baseName);
        // 省略了library_name和profile_name
        baseNames.push_back(libraryName + "::" + profileName + "::" + baseName);
        bool found = false;
        for (const auto& base : baseNames) {
            auto it = m_subscriberQosMap.find(base);
            if (it != m_subscriberQosMap.end()) {
                qos = it->second;
                found = true;
                break;
            }
        }
        if (!found) {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to find base_name %s\n", element->GetLineNum(), baseName);
            return false;
        }
    }
    bool ret = parseSubscriberQos(element, qos);
    if (!ret)
    {
        TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse SubscriberQos in library %s profile %s\n", element->GetLineNum(), libraryName.c_str(), profileName.c_str());
        return RETCODE_ERROR;
    }
    m_subscriberQosMap[key] = qos;
    return true;
}

bool DDSXMLRepo::parseDataWriterQosProfile(XMLElement* element, DataWriterQos& qos, const std::string& libraryName, const std::string& profileName) {
    // name
    std::string qosNameStr = "default";
    const char* qosName = element->Attribute("name");
    if (qosName) {
        qosNameStr = qosName;
    }
    std::string key = libraryName + "::" + profileName + "::" + qosNameStr;
    // base_name
    const char* baseName = element->Attribute("base_name");
    if (baseName) {
        // 可能的base_name集合
        std::vector<std::string> baseNames;
        // 完整的base_name
        baseNames.push_back(baseName);
        // 省略library_name
        baseNames.push_back(libraryName + "::" + baseName);
        // 省略了library_name和profile_name
        baseNames.push_back(libraryName + "::" + profileName + "::" + baseName);
        bool found = false;
        for (const auto& base : baseNames) {
            auto it = m_dataWriterQosMap.find(base);
            if (it != m_dataWriterQosMap.end()) {
                qos = it->second;
                found = true;
                break;
            }
        }
        if (!found) {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to find base_name %s\n", element->GetLineNum(), baseName);
            return false;
        }
    }
    bool ret = parseDataWriterQos(element, qos);
    if (!ret)
    {
        TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos in library %s profile %s\n", element->GetLineNum(), libraryName.c_str(), profileName.c_str());
        return RETCODE_ERROR;
    }
    m_dataWriterQosMap[key] = qos;
    return true;
}

bool DDSXMLRepo::parseDataReaderQosProfile(XMLElement* element, DataReaderQos& qos, const std::string& libraryName, const std::string& profileName) {
    // name
    std::string qosNameStr = "default";
    const char* qosName = element->Attribute("name");
    if (qosName) {
        qosNameStr = qosName;
    }
    std::string key = libraryName + "::" + profileName + "::" + qosNameStr;
    // base_name
    const char* baseName = element->Attribute("base_name");
    if (baseName) {
        // 可能的base_name集合
        std::vector<std::string> baseNames;
        // 完整的base_name
        baseNames.push_back(baseName);
        // 省略library_name
        baseNames.push_back(libraryName + "::" + baseName);
        // 省略了library_name和profile_name
        baseNames.push_back(libraryName + "::" + profileName + "::" + baseName);
        bool found = false;
        for (const auto& base : baseNames) {
            auto it = m_dataReaderQosMap.find(base);
            if (it != m_dataReaderQosMap.end()) {
                qos = it->second;
                found = true;
                break;
            }
        }
        if (!found) {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to find base_name %s\n", element->GetLineNum(), baseName);
            return false;
        }
    }
    bool ret = parseDataReaderQos(element, qos);
    if (!ret)
    {
        TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos in library %s profile %s\n", element->GetLineNum(), libraryName.c_str(), profileName.c_str());
        return RETCODE_ERROR;
    }
    m_dataReaderQosMap[key] = qos;
    return true;
}

bool DDSXMLRepo::parseTopicQosProfile(XMLElement* element, TopicQos& qos, const std::string& libraryName, const std::string& profileName) {
    // name
    std::string qosNameStr = "default";
    const char* qosName = element->Attribute("name");
    if (qosName) {
        qosNameStr = qosName;
    }
    std::string key = libraryName + "::" + profileName + "::" + qosNameStr;
    // base_name
    const char* baseName = element->Attribute("base_name");
    if (baseName) {
        // 可能的base_name集合
        std::vector<std::string> baseNames;
        // 完整的base_name
        baseNames.push_back(baseName);
        // 省略library_name
        baseNames.push_back(libraryName + "::" + baseName);
        // 省略了library_name和profile_name
        baseNames.push_back(libraryName + "::" + profileName + "::" + baseName);
        bool found = false;
        for (const auto& base : baseNames) {
            auto it = m_topicQosMap.find(base);
            if (it != m_topicQosMap.end()) {
                qos = it->second;
                found = true;
                break;
            }
        }
        if (!found) {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to find base_name %s\n", element->GetLineNum(), baseName);
            return false;
        }
    }
    bool ret = parseTopicQos(element, qos);
    if (!ret)
    {
        TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos in library %s profile %s\n", element->GetLineNum(), libraryName.c_str(), profileName.c_str());
        return RETCODE_ERROR;
    }
    m_topicQosMap[key] = qos;
    return true;
}

bool DDSXMLRepo::parseDomainParticipantFactoryQos(XMLElement* element, DomainParticipantFactoryQos& qos) {
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "entity_factory") == 0) {
            bool ret = parseEntityFactoryQosPolicy(child, qos.entity_factory);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DomainParticipantFactoryQos entity_factory\n", child->GetLineNum());
                return false;
            }
        } 
        else if (strcmp(nodeName, "property") == 0) {
            bool ret = parsePropertyQosPolicy(child, qos.property);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DomainParticipantFactoryQos property\n", child->GetLineNum());
                return false;
            }
        }
        else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown Qos policy %s\n", child->GetLineNum(), nodeName);
            return false;
        }
        
    }
    return true;
}

bool DDSXMLRepo::parseDomainParticipantQos(XMLElement* element, DomainParticipantQos& qos) {
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "entity_factory") == 0) {
            bool ret = parseEntityFactoryQosPolicy(child, qos.entity_factory);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DomainParticipantQos entity_factory\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "user_data") == 0) {
            bool ret = parseUserDataQosPolicy(child, qos.user_data);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DomainParticipantQos user_data\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "property") == 0) {
            bool ret = parsePropertyQosPolicy(child, qos.property);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DomainParticipantQos property\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "transport") == 0) {
            bool ret = parseTransportQosPolicy(child, qos.transport);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DomainParticipantQos transport\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "dicovery") == 0) {
            bool ret = parseDiscoveredMethodQosPolicy(child, qos.discovery);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DomainParticipantQos discovery\n", child->GetLineNum());
                return false;
			}
        } else if (strcmp(nodeName, "network_interface") == 0) {
            bool ret = parseNetworkInterfaceQosPolicy(child, qos.network_interface);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DomainParticipantQos network_interface\n", child->GetLineNum());
                return false;
			}
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown Qos policy %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parsePublisherQos(XMLElement* element, PublisherQos& qos) {
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "entity_factory") == 0) {
            bool ret = parseEntityFactoryQosPolicy(child, qos.entity_factory);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse PublisherQos entity_factory\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "presentation") == 0) {
            bool ret = parsePresentationQosPolicy(child, qos.presentation);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse PublisherQos presentation\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "partition") == 0) {
            bool ret = parsePartitionQosPolicy(child, qos.partition);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse PublisherQos partition\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "group_data") == 0) {
            bool ret = parseGroupDataQosPolicy(child, qos.group_data);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse PublisherQos group_data\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown Qos policy %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseSubscriberQos(XMLElement* element, SubscriberQos& qos) {
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "entity_factory") == 0) {
            bool ret = parseEntityFactoryQosPolicy(child, qos.entity_factory);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse SubscriberQos entity_factory\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "presentation") == 0) {
            bool ret = parsePresentationQosPolicy(child, qos.presentation);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse SubscriberQos presentation\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "partition") == 0) {
            bool ret = parsePartitionQosPolicy(child, qos.partition);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse SubscriberQos partition\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "group_data") == 0) {
            bool ret = parseGroupDataQosPolicy(child, qos.group_data);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse SubscriberQos group_data\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown Qos policy %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseDataWriterQos(XMLElement* element, DataWriterQos& qos) {
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "durability") == 0) {
            bool ret = parseDurabilityQosPolicy(child, qos.durability);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos durability\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "durability_service") == 0) {
            bool ret = parseDurabilityServiceQosPolicy(child, qos.durability_service);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos durability_service\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "deadline") == 0) {
            bool ret = parseDeadlineQosPolicy(child, qos.deadline);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos deadline\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "latency_budget") == 0) {
            bool ret = parseLatencyBudgetQosPolicy(child, qos.latency_budget);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos latency_budget\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "liveliness") == 0) {
            bool ret = parseLivelinessQosPolicy(child, qos.liveliness);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos liveliness\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "reliability") == 0) {
            bool ret = parseReliabilityQosPolicy(child, qos.reliability);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos reliability\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "destination_order") == 0) {
            bool ret = parseDestinationOrderQosPolicy(child, qos.destination_order);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos destination_order\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "history") == 0) {
            bool ret = parseHistoryQosPolicy(child, qos.history);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos history\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "resource_limits") == 0) {
            bool ret = parseResourceLimitsQosPolicy(child, qos.resource_limits);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos resource_limits\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "transport_priority") == 0) {
            bool ret = parseTransportPriorityQosPolicy(child, qos.transport_priority);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos transport_priority\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "lifespan") == 0) {
            bool ret = parseLifespanQosPolicy(child, qos.lifespan);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos lifespan\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "user_data") == 0) {
            bool ret = parseUserDataQosPolicy(child, qos.user_data);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos user_data\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "ownership") == 0) {
            bool ret = parseOwnershipQosPolicy(child, qos.ownership);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos ownership\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "ownership_strength") == 0) {
            bool ret = parseOwnershipStrengthQosPolicy(child, qos.ownership_strength);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos ownership_strength\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "writer_data_lifecycle") == 0) {
            bool ret = parseWriterDataLifecycleQosPolicy(child, qos.writer_data_lifecycle);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos writer_data_lifecycle\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "property") == 0) {
            bool ret = parsePropertyQosPolicy(child, qos.property);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataWriterQos property\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown Qos policy %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseDataReaderQos(XMLElement* element, DataReaderQos& qos) {
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "durability") == 0) {
            bool ret = parseDurabilityQosPolicy(child, qos.durability);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos durability\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "deadline") == 0) {
            bool ret = parseDeadlineQosPolicy(child, qos.deadline);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos deadline\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "latency_budget") == 0) {
            bool ret = parseLatencyBudgetQosPolicy(child, qos.latency_budget);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos latency_budget\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "liveliness") == 0) {
            bool ret = parseLivelinessQosPolicy(child, qos.liveliness);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos liveliness\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "reliability") == 0) {
            bool ret = parseReliabilityQosPolicy(child, qos.reliability);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos reliability\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "destination_order") == 0) {
            bool ret = parseDestinationOrderQosPolicy(child, qos.destination_order);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos destination_order\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "history") == 0) {
            bool ret = parseHistoryQosPolicy(child, qos.history);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos history\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "resource_limits") == 0) {
            bool ret = parseResourceLimitsQosPolicy(child, qos.resource_limits);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos resource_limits\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "user_data") == 0) {
            bool ret = parseUserDataQosPolicy(child, qos.user_data);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos user_data\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "ownership") == 0) {
            bool ret = parseOwnershipQosPolicy(child, qos.ownership);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos ownership\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "time_based_filter") == 0) {
            bool ret = parseTimeBasedFilterQosPolicy(child, qos.time_based_filter);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos time_based_filter\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "reader_data_lifecycle") == 0) {
            bool ret = parseReaderDataLifecycleQosPolicy(child, qos.reader_data_lifecycle);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos reader_data_lifecycle\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "property") == 0) {
            bool ret = parsePropertyQosPolicy(child, qos.property);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DataReaderQos property\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown Qos policy %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseTopicQos(XMLElement* element, TopicQos& qos) {
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "topic_data") == 0) {
            bool ret = parseTopicDataQosPolicy(child, qos.topic_data);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos topic_data\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "durability") == 0) {
            bool ret = parseDurabilityQosPolicy(child, qos.durability);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos durability\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "durability_service") == 0) {
            bool ret = parseDurabilityServiceQosPolicy(child, qos.durability_service);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos durability_service\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "deadline") == 0) {
            bool ret = parseDeadlineQosPolicy(child, qos.deadline);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos deadline\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "latency_budget") == 0) {
            bool ret = parseLatencyBudgetQosPolicy(child, qos.latency_budget);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos latency_budget\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "liveliness") == 0) {
            bool ret = parseLivelinessQosPolicy(child, qos.liveliness);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos liveliness\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "reliability") == 0) {
            bool ret = parseReliabilityQosPolicy(child, qos.reliability);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos reliability\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "destination_order") == 0) {
            bool ret = parseDestinationOrderQosPolicy(child, qos.destination_order);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos destination_order\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "history") == 0) {
            bool ret = parseHistoryQosPolicy(child, qos.history);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos history\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "resource_limits") == 0) {
            bool ret = parseResourceLimitsQosPolicy(child, qos.resource_limits);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos resource_limits\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "transport_priority") == 0) {
            bool ret = parseTransportPriorityQosPolicy(child, qos.transport_priority);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos transport_priority\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "lifespan") == 0) {
            bool ret = parseLifespanQosPolicy(child, qos.lifespan);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos lifespan\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "ownership") == 0) {
            bool ret = parseOwnershipQosPolicy(child, qos.ownership);
            if (!ret) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicQos ownership\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown Qos policy %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

ReturnCode_t DDSXMLRepo::clear() {
    m_participantFactoryQosMap.clear();
    m_participantQosMap.clear();
    m_publisherQosMap.clear();
    m_subscriberQosMap.clear();
    m_dataWriterQosMap.clear();
    m_dataReaderQosMap.clear();
    m_topicQosMap.clear();
    return RETCODE_OK;
}

ReturnCode_t DDSXMLRepo::lookup_participant_factory_qos(
    const std::string& library_name, 
    const std::string& profile_name, 
    const std::string& qos_name, 
    DomainParticipantFactoryQos& qos) {
    std::string key = (library_name.empty() ? "default" : library_name) + "::" 
        + (profile_name.empty() ? "default" : profile_name) + "::" 
        + (qos_name.empty() ? "default" : qos_name);
    auto it = m_participantFactoryQosMap.find(key);
    if (it != m_participantFactoryQosMap.end()) {
        qos = it->second;
        return RETCODE_OK;
    }
    return RETCODE_ERROR;
}

ReturnCode_t DDSXMLRepo::lookup_participant_qos(
    const std::string& library_name, 
    const std::string& profile_name, 
    const std::string& qos_name, 
    DomainParticipantQos& qos) {
    std::string key = (library_name.empty() ? "default" : library_name) + "::" 
        + (profile_name.empty() ? "default" : profile_name) + "::" 
        + (qos_name.empty() ? "default" : qos_name);
    auto it = m_participantQosMap.find(key);
    if (it != m_participantQosMap.end()) {
        qos = it->second;
        return RETCODE_OK;
    }
    return RETCODE_ERROR;
}

ReturnCode_t DDSXMLRepo::lookup_publisher_qos(
    const std::string& library_name, 
    const std::string& profile_name, 
    const std::string& qos_name, 
    PublisherQos& qos) {
    std::string key = (library_name.empty() ? "default" : library_name) + "::" 
        + (profile_name.empty() ? "default" : profile_name) + "::" 
        + (qos_name.empty() ? "default" : qos_name);
    auto it = m_publisherQosMap.find(key);
    if (it != m_publisherQosMap.end()) {
        qos = it->second;
        return RETCODE_OK;
    }
    return RETCODE_ERROR;
}

ReturnCode_t DDSXMLRepo::lookup_subscriber_qos(
    const std::string& library_name, 
    const std::string& profile_name, 
    const std::string& qos_name, 
    SubscriberQos& qos) {
    std::string key = (library_name.empty() ? "default" : library_name) + "::" 
        + (profile_name.empty() ? "default" : profile_name) + "::" 
        + (qos_name.empty() ? "default" : qos_name);
    auto it = m_subscriberQosMap.find(key);
    if (it != m_subscriberQosMap.end()) {
        qos = it->second;
        return RETCODE_OK;
    }
    return RETCODE_ERROR;
}

ReturnCode_t DDSXMLRepo::lookup_datawriter_qos(
    const std::string& library_name, 
    const std::string& profile_name, 
    const std::string& qos_name, 
    DataWriterQos& qos) {
    std::string key = (library_name.empty() ? "default" : library_name) + "::" 
        + (profile_name.empty() ? "default" : profile_name) + "::" 
        + (qos_name.empty() ? "default" : qos_name);
    auto it = m_dataWriterQosMap.find(key);
    if (it != m_dataWriterQosMap.end()) {
        qos = it->second;
        return RETCODE_OK;
    }
    return RETCODE_ERROR;
}

ReturnCode_t DDSXMLRepo::lookup_datareader_qos(
    const std::string& library_name, 
    const std::string& profile_name, 
    const std::string& qos_name, 
    DataReaderQos& qos) {
    std::string key = (library_name.empty() ? "default" : library_name) + "::" 
        + (profile_name.empty() ? "default" : profile_name) + "::" 
        + (qos_name.empty() ? "default" : qos_name);
    auto it = m_dataReaderQosMap.find(key);
    if (it != m_dataReaderQosMap.end()) {
        qos = it->second;
        return RETCODE_OK;
    }
    return RETCODE_ERROR;
}

ReturnCode_t DDSXMLRepo::lookup_topic_qos(
    const std::string& library_name, 
    const std::string& profile_name, 
    const std::string& qos_name, 
    TopicQos& qos) {
    std::string key = (library_name.empty() ? "default" : library_name) + "::" 
        + (profile_name.empty() ? "default" : profile_name) + "::" 
        + (qos_name.empty() ? "default" : qos_name);
    auto it = m_topicQosMap.find(key);
    if (it != m_topicQosMap.end()) {
        qos = it->second;
        return RETCODE_OK;
    }
    return RETCODE_ERROR;
}

bool DDSXMLRepo::parsePositiveIntegerUnlimited(XMLElement* element, int& value) {
    if (!element) {
        return true;
    }
    const char* text = element->GetText();
    if (text) {
        if (strcmp(text, "LENGTH_UNLIMITED") == 0) {
            value = LENGTH_UNLIMITED;
        } else {
            XMLError ret = element->QueryIntText((int*)&value);
            if (ret != XML_SUCCESS) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse positive integer unlimited\n", element->GetLineNum());
                return false;
            }
        }
    }
    return true;
}

bool DDSXMLRepo::parseDuration(XMLElement* element, Duration_t& duration) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "sec") == 0) {
            const char* text = child->GetText();
            if (strcmp("DURATION_INFINITE", text) == 0) {
                duration.sec = 0x7fffffff;
            } else if (strcmp("DURATION_INFINITE_SEC", text) == 0) {
                duration.sec = 0x7fffffff;
            } else {
                XMLError ret = child->QueryIntText((int*)&duration.sec);
                if (ret != XML_SUCCESS) {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse sec\n", child->GetLineNum());
                    return false;
                }
            }
        } else if (strcmp(nodeName, "nanosec") == 0) {
            const char* text = child->GetText();
            if (strcmp("DURATION_INFINITE", text) == 0) {
                duration.nanosec = 0x7fffffff;
            } else if (strcmp("DURATION_INFINITE_NSEC", text) == 0) {
                duration.nanosec = 0x7fffffff;
            } else {
                XMLError ret = child->QueryIntText((int*)&duration.nanosec);
                if (ret != XML_SUCCESS) {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse nanosec\n", child->GetLineNum());
                    return false;
                }
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown Duration element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseBase64Binary(XMLElement* element, Sequence<octet>& value) {
    if (!element) {
        return true;
    }
    const char* text = element->GetText();
    if (text) {
        std::string str(text);
        // 如果以base64:开头，则表示是base64编码的字符串
        if (str.find("base64:") == 0) {
            str = str.substr(7); // 去掉base64:
            // TODO base64解码
            // 这里需要实现base64解码，将解码后的数据存入value中
            return true; // 返回true表示解析成功
        }
        // 如果以hex:开头，则表示是hex编码的字符串
        if (str.find("hex:") == 0) {
            str = str.substr(4); // 去掉hex:
            for (size_t i = 0; i < str.length(); i += 2) {
                std::string byteString = str.substr(i, 2);
                char byte = (char)strtol(byteString.c_str(), nullptr, 16);
                value.push_back(byte); // 将byte添加到value中
            }
            return true;
        }
        // 如果不是以base64:或hex:开头，则表示是普通字符串
        for (size_t i = 0; i < str.length(); ++i) {
            value.push_back((octet)str[i]); // 将字符转换为octet并添加到value中
        }
        return true;
    }
    return false;
}

bool DDSXMLRepo::parseDurabilityQosPolicy(XMLElement* element, DurabilityQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "kind") == 0) {
            const char* kind = child->GetText();
            if (kind) {
                if (strcmp(kind, "VOLATILE_DURABILITY_QOS") == 0) {
                    qos.kind = VOLATILE_DURABILITY_QOS;
                } else if (strcmp(kind, "TRANSIENT_LOCAL_DURABILITY_QOS") == 0) {
                    qos.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
                } else if (strcmp(kind, "TRANSIENT_DURABILITY_QOS") == 0) {
                    qos.kind = TRANSIENT_DURABILITY_QOS;
                } else if (strcmp(kind, "PERSISTENT_DURABILITY_QOS") == 0) {
                    qos.kind = PERSISTENT_DURABILITY_QOS;
                } else {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Invalid DurabilityQosPolicy kind: %s\n", child->GetLineNum(), kind);
                    return false;
                }
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown DurabilityQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseUserDataQosPolicy(XMLElement* element, UserDataQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "value") == 0) {
            const char* value = child->GetText();
            if (value) {
                if (!parseBase64Binary(child, qos.value)) {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse UserDataQosPolicy value\n", child->GetLineNum());
                    return false;
                }
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown UserDataQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseProperty(XMLElement* element, Property_t& property)
{
    const char* memberName[] = {
        "name",
        "value",
        "propagate"
    };
    // 遍历子节点
    for (XMLElement* childEle = element->FirstChildElement(); childEle; childEle = childEle->NextSiblingElement()) {
        //验证名字是否合法
        if (strcmp(childEle->Name(), memberName[0]) == 0)
        {
            //取值
            property.name = childEle->GetText();
        }
        else if (strcmp(childEle->Name(), memberName[1]) == 0)
        {
            //取值
            property.value = childEle->GetText();
        }
        else if (strcmp(childEle->Name(), memberName[2]) == 0)
        {
        }
        else
        {
        }
    }
    return true;
}

bool DDSXMLRepo::parseBinaryProperty(XMLElement* element, BinaryProperty_t& binaryProperty)
{
    const char* memberName[] = {
        "name",
        "value",
        "propagate"
    };
    // 遍历子节点
    for (XMLElement* childEle = element->FirstChildElement(); childEle; childEle = childEle->NextSiblingElement()) {
        //验证名字是否合法
        if (strcmp(childEle->Name(), memberName[0]) == 0)
        {
            //取值
            //childEle->QueryUnsignedText((unsigned int*)(self->name));
            binaryProperty.name = childEle->GetText();
        }
        else if (strcmp(childEle->Name(), memberName[1]) == 0)
        {
            //取值
            const char* str = childEle->GetText();
            size_t length = strlen(str);
            for (size_t i = 0; i < length; ++i) {
                unsigned char octet = str[i];
                binaryProperty.value.push_back(octet);
            }
        }
        else if (strcmp(childEle->Name(), memberName[2]) == 0)
        {
            //验证是否是唯一且是叶节点
        }
        else
        {
        }
    }
    return true;
}

bool DDSXMLRepo::parsePropertySeq(XMLElement* element, PropertyQosPolicy& qos)
{
    //SequenceData<Property_t>
    //注释seq_clear，在qos继承时添加而不覆盖
    //DDS_PropertySeq_clear(self);
    //遍历子元素,value下有若干组element
    for (XMLElement* childEle = element->FirstChildElement(); childEle; childEle = childEle->NextSiblingElement("element"))
    {
        // 验证名字是否合法，依赖NextSiblingElement("element") 应该不用
        // 验证除了element是否有其他节点 这步应该可以没有
        // 取值
        // unsigned int curlen = qos.value.size();
        Property_t propertyEle;
        parseProperty(childEle, propertyEle);//解析出结构体
        qos.value.push_back(propertyEle);//插入seq
    }
    return true;
}

bool DDSXMLRepo::parseBinaryPropertySeq(XMLElement* element, PropertyQosPolicy& qos)
{
    //SequenceData<BinaryProperty_t>
    //注释seq_clear，在qos继承时添加而不覆盖
    //const char* memberName = "element";
    //遍历子元素,value下有若干组element
    for (XMLElement* childEle = element->FirstChildElement(); childEle; childEle = childEle->NextSiblingElement("element"))
    {
        // 验证名字是否合法，依赖NextSiblingElement("element") 应该不用
        // 验证除了element是否有其他节点 这步应该可以没有
        // 取值
        BinaryProperty_t binaryPropertyEle;
        parseBinaryProperty(childEle, binaryPropertyEle);//解析出结构体
        qos.binary_value.push_back(binaryPropertyEle);//插入seq
    }
    return true;
}

bool DDSXMLRepo::parsePropertyQosPolicy(XMLElement* element, PropertyQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* childEle = element->FirstChildElement(); childEle; childEle = childEle->NextSiblingElement()) {
        //检查property标签下是否有子标签value或binary_value
        const char* nodeName = childEle->Name();//获得的标签名称
        //如果是value标签，解析value标签
        if (strcmp(nodeName, "value") == 0) {
            //验证节点是否唯一

            //取值
            if (!parsePropertySeq(childEle, qos))
            {
                return false;
            }
        }
        //如果是binary_value标签，解析binary_value标签
        else if (strcmp(nodeName, "binary_value") == 0) {
            //验证节点是否唯一

            //取值
            if (!parseBinaryPropertySeq(childEle, qos))
            {
                return false;
            }
        }
        else {
            //忽略
        }
    }
    return true;
}

bool DDSXMLRepo::parsePresentationQosPolicy(XMLElement* element, PresentationQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "access_scope") == 0) {
            const char* access_scope = child->GetText();
            if (access_scope) {
                if (strcmp(access_scope, "INSTANCE_PRESENTATION_QOS") == 0) {
                    qos.access_scope = INSTANCE_PRESENTATION_QOS;
                } else if (strcmp(access_scope, "TOPIC_PRESENTATION_QOS") == 0) {
                    qos.access_scope = TOPIC_PRESENTATION_QOS;
                } else if (strcmp(access_scope, "GROUP_PRESENTATION_QOS") == 0) {
                    qos.access_scope = GROUP_PRESENTATION_QOS;
                } else {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Invalid PresentationQosPolicy access_scope: %s\n", child->GetLineNum(), access_scope);
                    return false;
                }
            }
        } else if (strcmp(nodeName, "coherent_access") == 0) {
            XMLError ret = child->QueryBoolText(&qos.coherent_access);
            if (ret != XML_SUCCESS) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse coherent_access\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "ordered_access") == 0) {
            XMLError ret = child->QueryBoolText(&qos.ordered_access);
            if (ret != XML_SUCCESS) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse ordered_access\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown PresentationQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseDeadlineQosPolicy(XMLElement* element, DeadlineQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "period") == 0) {
            if (!parseDuration(child, qos.period)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DeadlineQosPolicy period\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown DeadlineQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseLatencyBudgetQosPolicy(XMLElement* element, LatencyBudgetQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "duration") == 0) {
            if (!parseDuration(child, qos.duration)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse LatencyBudgetQosPolicy duration\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown LatencyBudgetQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseOwnershipQosPolicy(XMLElement* element, OwnershipQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "kind") == 0) {
            const char* kind = child->GetText();
            if (kind) {
                if (strcmp(kind, "SHARED_OWNERSHIP_QOS") == 0) {
                    qos.kind = SHARED_OWNERSHIP_QOS;
                } else if (strcmp(kind, "EXCLUSIVE_OWNERSHIP_QOS") == 0) {
                    qos.kind = EXCLUSIVE_OWNERSHIP_QOS;
                } else {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Invalid OwnershipQosPolicy kind: %s\n", child->GetLineNum(), kind);
                    return false;
                }
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown OwnershipQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseOwnershipStrengthQosPolicy(XMLElement* element, OwnershipStrengthQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "value") == 0) {
            XMLError ret = child->QueryIntText((int*)&qos.value);
            if (ret != XML_SUCCESS) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse OwnershipStrengthQosPolicy value\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown OwnershipStrengthQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseLivelinessQosPolicy(XMLElement* element, LivelinessQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "kind") == 0) {
            const char* kind = child->GetText();
            if (kind) {
                if (strcmp(kind, "AUTOMATIC_LIVELINESS_QOS") == 0) {
                    qos.kind = AUTOMATIC_LIVELINESS_QOS;
                } else if (strcmp(kind, "MANUAL_BY_PARTICIPANT_LIVELINESS_QOS") == 0) {
                    qos.kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
                } else if (strcmp(kind, "MANUAL_BY_TOPIC_LIVELINESS_QOS") == 0) {
                    qos.kind = MANUAL_BY_TOPIC_LIVELINESS_QOS;
                } else {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Invalid LivelinessQosPolicy kind: %s\n", child->GetLineNum(), kind);
                    return false;
                }
            }
        } else if (strcmp(nodeName, "lease_duration") == 0) {
            if (!parseDuration(child, qos.lease_duration)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse LivelinessQosPolicy lease_duration\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown LivelinessQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseTimeBasedFilterQosPolicy(XMLElement* element, TimeBasedFilterQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "minimum_separation") == 0) {
            if (!parseDuration(child, qos.minimum_separation)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TimeBasedFilterQosPolicy minimum_separation\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown TimeBasedFilterQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parsePartitionQosPolicy(XMLElement* element, PartitionQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "name") == 0) {
            for (XMLElement* item = child->FirstChildElement(); item; item = item->NextSiblingElement()) {
                const char *itemName = item->Name();
                if (strcmp(itemName, "element") != 0) {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown PartitionQosPolicy name element %s\n", item->GetLineNum(), itemName);
                    return false;
                }
                qos.name.push_back(std::string(item->GetText()));
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown PartitionQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseReliabilityQosPolicy(XMLElement* element, ReliabilityQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "kind") == 0) {
            const char* kind = child->GetText();
            if (kind) {
                if (strcmp(kind, "BEST_EFFORT_RELIABILITY_QOS") == 0) {
                    qos.kind = BEST_EFFORT_RELIABILITY_QOS;
                } else if (strcmp(kind, "RELIABLE_RELIABILITY_QOS") == 0) {
                    qos.kind = RELIABLE_RELIABILITY_QOS;
                } else {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Invalid ReliabilityQosPolicy kind: %s\n", child->GetLineNum(), kind);
                    return false;
                }
            }
        } else if (strcmp(nodeName, "max_blocking_time") == 0) {
            if (!parseDuration(child, qos.max_blocking_time)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse ReliabilityQosPolicy max_blocking_time\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown ReliabilityQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseDestinationOrderQosPolicy(XMLElement* element, DestinationOrderQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "kind") == 0) {
            const char* kind = child->GetText();
            if (kind) {
                if (strcmp(kind, "BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS") == 0) {
                    qos.kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
                } else if (strcmp(kind, "BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS") == 0) {
                    qos.kind = BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
                } else {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Invalid DestinationOrderQosPolicy kind: %s\n", child->GetLineNum(), kind);
                    return false;
                }
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown DestinationOrderQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseHistoryQosPolicy(XMLElement* element, HistoryQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "kind") == 0) {
            const char* kind = child->GetText();
            if (kind) {
                if (strcmp(kind, "KEEP_LAST_HISTORY_QOS") == 0) {
                    qos.kind = KEEP_LAST_HISTORY_QOS;
                } else if (strcmp(kind, "KEEP_ALL_HISTORY_QOS") == 0) {
                    qos.kind = KEEP_ALL_HISTORY_QOS;
                } else {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Invalid HistoryQosPolicy kind: %s\n", child->GetLineNum(), kind);
                    return false;
                }
            }
        } else if (strcmp(nodeName, "depth") == 0) {
            XMLError ret = child->QueryIntText((int*)&qos.depth);
            if (ret != XML_SUCCESS) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse HistoryQosPolicy depth\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown HistoryQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseResourceLimitsQosPolicy(XMLElement* element, ResourceLimitsQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "max_samples") == 0) {
            if (!parsePositiveIntegerUnlimited(child, qos.max_samples)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse ResourceLimitsQosPolicy max_samples\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "max_instances") == 0) {
            if (!parsePositiveIntegerUnlimited(child, qos.max_instances)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse ResourceLimitsQosPolicy max_instances\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "max_samples_per_instance") == 0) {
            if (!parsePositiveIntegerUnlimited(child, qos.max_samples_per_instance)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse ResourceLimitsQosPolicy max_samples_per_instance\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown ResourceLimitsQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseEntityFactoryQosPolicy(XMLElement* element, EntityFactoryQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "autoenable_created_entities") == 0) {
            XMLError ret = child->QueryBoolText(&qos.autoenable_created_entities);
            if (ret != XML_SUCCESS) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse EntityFactoryQosPolicy autoenable_created_entities\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown EntityFactoryQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseWriterDataLifecycleQosPolicy(XMLElement* element, WriterDataLifecycleQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "autodispose_unregistered_instances") == 0) {
            XMLError ret = child->QueryBoolText(&qos.autodispose_unregistered_instances);
            if (ret != XML_SUCCESS) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse WriterDataLifecycleQosPolicy autodispose_unregistered_instances\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown WriterDataLifecycleQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseReaderDataLifecycleQosPolicy(XMLElement* element, ReaderDataLifecycleQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "autopurge_nowriter_samples_delay") == 0) {
            if (!parseDuration(child, qos.autopurge_nowriter_samples_delay)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse ReaderDataLifecycleQosPolicy autopurge_nowriter_samples_delay\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "autopurge_disposed_samples_delay") == 0) {
            if (!parseDuration(child, qos.autopurge_disposed_samples_delay)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse ReaderDataLifecycleQosPolicy autopurge_disposed_samples_delay\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown ReaderDataLifecycleQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseTopicDataQosPolicy(XMLElement* element, TopicDataQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "value") == 0) {
            const char* value = child->GetText();
            if (value) {
                if (!parseBase64Binary(child, qos.value)) {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TopicDataQosPolicy value\n", child->GetLineNum());
                    return false;
                }
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown TopicDataQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseGroupDataQosPolicy(XMLElement* element, GroupDataQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "value") == 0) {
            const char* value = child->GetText();
            if (value) {
                if (!parseBase64Binary(child, qos.value)) {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse GroupDataQosPolicy value\n", child->GetLineNum());
                    return false;
                }
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown GroupDataQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseTransportPriorityQosPolicy(XMLElement* element, TransportPriorityQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "value") == 0) {
            XMLError ret = child->QueryIntText((int*)&qos.value);
            if (ret != XML_SUCCESS) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse TransportPriorityQosPolicy value\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown TransportPriorityQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseLifespanQosPolicy(XMLElement* element, LifespanQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "duration") == 0) {
            if (!parseDuration(child, qos.duration)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse LifespanQosPolicy duration\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown LifespanQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseDurabilityServiceQosPolicy(XMLElement* element, DurabilityServiceQosPolicy& qos) {
    if (!element) {
        return true;
    }
    // 遍历子节点
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "service_cleanup_delay") == 0) {
            if (!parseDuration(child, qos.service_cleanup_delay)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DurabilityServiceQosPolicy service_cleanup_delay\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "history_kind") == 0) {
            const char* history_kind = child->GetText();
            if (history_kind) {
                if (strcmp(history_kind, "KEEP_LAST_HISTORY_QOS") == 0) {
                    qos.history_kind = KEEP_LAST_HISTORY_QOS;
                } else if (strcmp(history_kind, "KEEP_ALL_HISTORY_QOS") == 0) {
                    qos.history_kind = KEEP_ALL_HISTORY_QOS;
                } else {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Invalid DurabilityServiceQosPolicy history_kind: %s\n", child->GetLineNum(), history_kind);
                    return false;
                }
            }
        } else if (strcmp(nodeName, "history_depth") == 0) {
            XMLError ret = child->QueryIntText((int*)&qos.history_depth);
            if (ret != XML_SUCCESS) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DurabilityServiceQosPolicy history_depth\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "max_samples") == 0) {
            if (!parsePositiveIntegerUnlimited(child, qos.max_samples)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DurabilityServiceQosPolicy max_samples\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "max_instances") == 0) {
            if (!parsePositiveIntegerUnlimited(child, qos.max_instances)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DurabilityServiceQosPolicy max_instances\n", child->GetLineNum());
                return false;
            }
        } else if (strcmp(nodeName, "max_samples_per_instance") == 0) {
            if (!parsePositiveIntegerUnlimited(child, qos.max_samples_per_instance)) {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Failed to parse DurabilityServiceQosPolicy max_samples_per_instance\n", child->GetLineNum());
                return false;
            }
        } else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown DurabilityServiceQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseTransportQosPolicy(XMLElement* element, TransportQosPolicy& qos)
{
    if (!element) {
        return true;
    }
    qos.removeTransport(TRANSPORT_UDPv4_QOS);
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "kinds") == 0) {
            for (XMLElement* item = child->FirstChildElement(); item; item = item->NextSiblingElement()) {
                const char* itemName = item->Name();
                if (strcmp(itemName, "element") != 0) {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown TransportQosPolicy element %s\n", item->GetLineNum(), itemName);
                    return false;
                }
                const char* transport_type = item->GetText();
                if (strcmp(transport_type, "TRANSPORT_UDPv4_QOS") == 0) {
                    qos.addTransport(TRANSPORT_UDPv4_QOS);
                }
                else if (strcmp(transport_type, "TRANSPORT_UDPv6_QOS") == 0) {
                    qos.addTransport(TRANSPORT_UDPv6_QOS);
                }
                else if (strcmp(transport_type, "TRANSPORT_TCPv4_QOS") == 0) {
                    qos.addTransport(TRANSPORT_TCPv4_QOS);
                }
                else if (strcmp(transport_type, "TRANSPORT_TCPv6_QOS") == 0) {
                    qos.addTransport(TRANSPORT_TCPv6_QOS);
                }
                else if (strcmp(transport_type, "TRANSPORT_SHM_QOS") == 0) {
                    qos.addTransport(TRANSPORT_SHM_QOS);
                }
                else {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Invalid TransportQosPolicy kind: %s\n", child->GetLineNum(), transport_type);
                    return false;
                }
            }


        }
        else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown TransportQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }

    return true;
}

bool DDSXMLRepo::parseDiscoveredMethodQosPolicy(XMLElement* element, DiscoveredMethodQosPolicy& qos)
{
    if (!element) {
        return true;
    }
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "kind") == 0) {
            const char* type = child->GetText();
            if (strcmp(type, "DISCOVER_UDPv4_QOS") == 0) {
                qos.kind = DISCOVER_UDPv4_QOS;
            }
            else if (strcmp(type, "DISCOVER_UDPv6_QOS") == 0) {
                qos.kind = DISCOVER_UDPv6_QOS;
            }
            else if (strcmp(type, "DISCOVER_UDPv4_AND_UDPv6_QOS") == 0) {
                qos.kind = DISCOVER_UDPv4_AND_UDPv6_QOS;
            }
            else {
                TRAVODDS_LOG(LOG_ERROR, "Line[%d] Invalid DiscoveredMethodQosPolicy kind: %s\n", child->GetLineNum(), type);
                return false;
            }
        }
        else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown DiscoveredMethodQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

bool DDSXMLRepo::parseNetworkInterfaceQosPolicy(XMLElement* element, NetworkInterfaceQosPolicy& qos)
{
    if (!element) {
        return true;
    }
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const char* nodeName = child->Name();
        if (strcmp(nodeName, "interface_whitelist") == 0) {
            for (XMLElement* item = child->FirstChildElement(); item; item = item->NextSiblingElement()) {
                const char* itemName = item->Name();
                if (strcmp(itemName, "element") != 0) {
                    TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown NetworkInterfaceQosPolicy interface_whitelist element %s\n", item->GetLineNum(), itemName);
                    return false;
                }
                qos.interface_whitelist.push_back(std::string(item->GetText()));
            }
        }
        else {
            TRAVODDS_LOG(LOG_ERROR, "Line[%d] Unknown NetworkInterfaceQosPolicy element %s\n", child->GetLineNum(), nodeName);
            return false;
        }
    }
    return true;
}

#endif //_XML_INTERFACE_CPP

TRAVODDS_NAMESPACE_END
