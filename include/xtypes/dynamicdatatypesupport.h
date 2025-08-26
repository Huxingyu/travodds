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

#ifndef DYNAMIC_DATA_TYPESUPPORT_H
#define DYNAMIC_DATA_TYPESUPPORT_H

#include "travodds_dll.h"
#include "xtypes/dynamictype.h"
#include "xtypes/dynamicdata.h"
#include "dcps/topic/typesupport.h"

TRAVODDS_NAMESPACE_BEGIN

class DynamicDataTypeSupport : public TypeSupport {
public:
    DynamicDataTypeSupport(DynamicType* type);
    
    virtual void* create_data() override;

    virtual void delete_data(void* data) override;

    virtual int copy_data(void* dst, void* src) override;

    virtual unsigned int get_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual unsigned int get_max_serialized_data_size(void* data, unsigned int currentAlignment) override;

    virtual int serialize_data(void* data, AGIDDSCdrSerializer* cdr, int endian) override;

    virtual int deserialize_data(void* data, AGIDDSCdrDeserializer* cdr, int endian) override;

    virtual TypeObject* get_typeobject() override;

    virtual ReturnCode_t get_instancehandle(void* data, AGIDDSCdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    virtual bool has_key() override;

    virtual const char* get_typename() override;

    virtual int MakeKey(const void* data, InstanceHandle_t& iHandle, bool forceMd5 = false) override;

    int serialize_key(void* data, AGIDDSCdrSerializer* cdr, int endian);
private:
    unsigned int get_member_serialize_size(DynamicData* dynamicData, DynamicType* type, MemberId id, const std::string& name, unsigned int currentAlignment);
    int serialize_member(DynamicData* dynamicData, AGIDDSCdrSerializer* cdr, DynamicType* type, MemberId id, const std::string& name, int endian);
private:
    DynamicType* type_;
};

TRAVODDS_NAMESPACE_END

#endif // DYNAMIC_DATA_TYPESUPPORT_H