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

#include "dcps/domain/domainparticipantfactoryimpl.h"
#include "xml/dds_xml_repo.h"
#include "common/log/logger.h"

TRAVODDS_NAMESPACE_BEGIN

#ifdef _XML_INTERFACE_CPP

static DDSXMLRepo g_xmlRepo;

ReturnCode_t DomainParticipantFactoryImpl::load_qos_from_xml(const char* xml_path) {
    return g_xmlRepo.load(xml_path);
}

ReturnCode_t DomainParticipantFactoryImpl::unload_qos() {
    return g_xmlRepo.clear();
}

DomainParticipant* DomainParticipantFactoryImpl::create_participant_with_profile(
    DomainId_t domain_id, 
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name, 
    DomainParticipantListener* listener, 
    StatusMask mask) {
    DomainParticipantQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_participant_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup participant qos %s %s %s.\n", library_name, profile_name, qos_name);
        return nullptr;
    }
    return create_participant(domain_id, qos, listener, mask);
}

ReturnCode_t DomainParticipantFactoryImpl::get_participant_factory_qos_from_profile(
    DomainParticipantFactoryQos& qos, 
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    return g_xmlRepo.lookup_participant_factory_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
}

ReturnCode_t DomainParticipantFactoryImpl::set_default_participant_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    DomainParticipantQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_participant_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup participant  qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_default_participant_qos(qos);
}

ReturnCode_t DomainParticipantFactoryImpl::get_participant_qos_from_profile(
    DomainParticipantQos& qos, 
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    return g_xmlRepo.lookup_participant_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
}

ReturnCode_t DomainParticipantFactoryImpl::get_publisher_qos_from_profile(
    PublisherQos& qos, 
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    return g_xmlRepo.lookup_publisher_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
}

ReturnCode_t DomainParticipantFactoryImpl::get_subscriber_qos_from_profile(
    SubscriberQos& qos, 
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    return g_xmlRepo.lookup_subscriber_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
}

ReturnCode_t DomainParticipantFactoryImpl::get_datawriter_qos_from_profile(
    DataWriterQos& qos, 
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    return g_xmlRepo.lookup_datawriter_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
}

ReturnCode_t DomainParticipantFactoryImpl::get_datareader_qos_from_profile(
    DataReaderQos& qos, 
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    return g_xmlRepo.lookup_datareader_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
}

ReturnCode_t DomainParticipantFactoryImpl::get_topic_qos_from_profile(
    TopicQos& qos, 
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    return g_xmlRepo.lookup_topic_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
}

ReturnCode_t DomainParticipantFactoryImpl::set_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    DomainParticipantFactoryQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_participant_factory_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup participant factory qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_qos(qos);
}

ReturnCode_t DomainParticipantImpl::set_default_publisher_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    PublisherQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_publisher_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup publisher qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_default_publisher_qos(qos);
}

ReturnCode_t DomainParticipantImpl::set_default_subscriber_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    SubscriberQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_subscriber_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup subscriber qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_default_subscriber_qos(qos);
}

ReturnCode_t DomainParticipantImpl::set_default_topic_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    TopicQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_topic_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup topic qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_default_topic_qos(qos);
}

Publisher* DomainParticipantImpl::create_publisher_with_qos_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name, 
    PublisherListener* listener, 
    StatusMask mask) {
    PublisherQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_publisher_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup publisher qos %s %s %s.\n", library_name, profile_name, qos_name);
        return nullptr;
    }
    return create_publisher(qos, listener, mask);
}

Subscriber* DomainParticipantImpl::create_subscriber_with_qos_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name, 
    SubscriberListener* listener, 
    StatusMask mask) {
    SubscriberQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_subscriber_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup subscriber qos %s %s %s.\n", library_name, profile_name, qos_name);
        return nullptr;
    }
    return create_subscriber(qos, listener, mask);
}

Topic* DomainParticipantImpl::create_topic_with_qos_profile(
    const std::string& topic_name, 
    const std::string& type_name, 
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name, 
    TopicListener* listener, 
    StatusMask mask) {
    TopicQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_topic_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup topic qos %s %s %s.\n", library_name, profile_name, qos_name);
        return nullptr;
    }
    return create_topic(topic_name, type_name, qos, listener, mask);
}

ReturnCode_t DomainParticipantImpl::set_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    DomainParticipantQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_participant_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup participant qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_qos(qos);
}

ReturnCode_t PublisherImpl::set_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    PublisherQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_publisher_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup publisher qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_qos(qos);
}

DataWriter* PublisherImpl::create_datawriter_with_qos_profile(
    Topic* topic,
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name, 
    DataWriterListener* listener, 
    StatusMask mask) {
    DataWriterQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_datawriter_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup datawriter qos %s %s %s.\n", library_name, profile_name, qos_name);
        return nullptr;
    }
    return create_datawriter(topic, qos, listener, mask); // Assuming nullptr for Topic* as it's not provided
}

ReturnCode_t PublisherImpl::set_default_datawriter_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    DataWriterQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_datawriter_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup datawriter qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_default_datawriter_qos(qos);
}

ReturnCode_t SubscriberImpl::set_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    SubscriberQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_subscriber_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup subscriber qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_qos(qos);
}

DataReader* SubscriberImpl::create_datareader_with_qos_profile(
    TopicDescription* topic,
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name, 
    DataReaderListener* listener, 
    StatusMask mask) {
    DataReaderQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_datareader_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup datareader qos %s %s %s.\n", library_name, profile_name, qos_name);
        return nullptr;
    }
    return create_datareader(topic, qos, listener, mask); // Assuming nullptr for TopicDescription* as it's not provided
}

ReturnCode_t SubscriberImpl::set_default_datareader_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    DataReaderQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_datareader_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup datareader qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_default_datareader_qos(qos);
}

ReturnCode_t DataWriterImpl::set_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    DataWriterQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_datawriter_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup datawriter qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_qos(qos);
}

ReturnCode_t DataReaderImpl::set_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    DataReaderQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_datareader_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup datareader qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_qos(qos);
}

ReturnCode_t TopicImpl::set_qos_with_profile(
    const char* library_name, 
    const char* profile_name, 
    const char* qos_name) {
    TopicQos qos;
    ReturnCode_t ret = g_xmlRepo.lookup_topic_qos(library_name ? library_name : "", profile_name ? profile_name : "", qos_name ? qos_name : "", qos);
    if (ret != RETCODE_OK) {
        TRAVODDS_LOG(LOG_ERROR, "Failed to lookup topic qos %s %s %s.\n", library_name, profile_name, qos_name);
        return ret;
    }
    return set_qos(qos);
}

TRAVODDS_NAMESPACE_END

#endif //_XML_INTERFACE_CPP
