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
#ifndef PARTICIPANT_GENERIC_MESSAGE_TYPE_SUPPORT_H
#define PARTICIPANT_GENERIC_MESSAGE_TYPE_SUPPORT_H

#include "dcps/topic/typesupport.h"
#include "security/securitycommon.h"
#include "security/participantgenericmessage.h"
#ifdef DDSSECURITY
TRAVODDS_NAMESPACE_BEGIN

class ParticipantGenericMessageTypeSupport : public TRAVODDS::TypeSupport {
public:
    static ParticipantGenericMessageTypeSupport* get_instance();

    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, CdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, CdrDeserializer* cdr, int endian) override;

    virtual TRAVODDS::TypeObject* get_typeobject() override;

    virtual TRAVODDS::ReturnCode_t get_instancehandle(void* data, CdrSerializer* cdr, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, TRAVODDS::InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, CdrSerializer* cdr, int endian);

private:
    ParticipantGenericMessageTypeSupport() = default;
};

TRAVODDS_NAMESPACE_END
#endif
#endif // PARTICIPANT_GENERIC_MESSAGE_TYPE_SUPPORT_H
