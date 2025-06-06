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
 
	~DomainParticipantFactoryImpl();

    DomainParticipant* create_participant(DomainId_t domain_id,
        const DomainParticipantQos& qos,
        DomainParticipantListener* listener,
        const StatusMask& mask);

    ReturnCode_t delete_participant(DomainParticipant*& participant);

    DomainParticipant* lookup_participant(DomainId_t domain_id) const;

    ReturnCode_t set_default_participant_qos(const DomainParticipantQos& qos);

    ReturnCode_t get_default_participant_qos(DomainParticipantQos& qos) const;

    ReturnCode_t set_qos(const DomainParticipantFactoryQos& qos);

    ReturnCode_t get_qos(DomainParticipantFactoryQos& qos) const;
#ifdef _XML_INTERFACE_CPP

    virtual ReturnCode_t load_qos_from_xml(const char* xml_path);

    virtual ReturnCode_t unload_qos();

    virtual DomainParticipant* create_participant_with_profile(
        DomainId_t domain_id,
        const char* library_name,
        const char* profile_name,
        const char* qos_name,
        DomainParticipantListener* listener,
        StatusMask mask);

    virtual ReturnCode_t set_default_participant_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual ReturnCode_t set_qos_with_profile(
        const char* library_name,
        const char* profile_name,
        const char* qos_name);
    virtual ReturnCode_t get_participant_factory_qos_from_profile(
        DomainParticipantFactoryQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual ReturnCode_t get_participant_qos_from_profile(
        DomainParticipantQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual ReturnCode_t get_topic_qos_from_profile(
        TopicQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual ReturnCode_t get_publisher_qos_from_profile(
        PublisherQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual ReturnCode_t get_subscriber_qos_from_profile(
        SubscriberQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual ReturnCode_t get_datareader_qos_from_profile(
        DataReaderQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

    virtual ReturnCode_t get_datawriter_qos_from_profile(
        DataWriterQos& qos,
        const char* library_name,
        const char* profile_name,
        const char* qos_name);

#endif // _XML_INTERFACE_CPP
public:
    static DomainParticipantFactoryImpl* get_instance();

	static void finalize();
private:
	DomainParticipantFactoryImpl();
	std::map<InstanceHandle_t, std::unique_ptr<DomainParticipantImpl>> participants_;
	mutable std::mutex mtxParticipants_;
    DomainParticipantFactoryQos qos_ = {};
    DomainParticipantQos defaultParticipantQos_ = {};
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_DOMAIN_DOMAINPARTICIPANTFACTORYIMPL_H
