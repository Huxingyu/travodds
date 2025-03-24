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
#pragma once

#include "DDS/Entity.h"

AGIDDS_NAMESPACE_BEGIN

#define DEF_NARROW_METHOD(TYPE) \
    static int type_id(); \
    virtual void *narrow_helper(long type_const)

#define IMPL_NARROW_METHOD(TYPE)\
    int \
    TYPE::type_id()\
{\
    return 0;\
    }\
    void * \
    TYPE::narrow_helper(long type_const) {\
    void *rval = NULL; \
    if (type_const == (long)&TYPE::type_id) \
    rval = this; \
    return rval; \
    }

class DDS_DLL DomainEntity : public virtual Entity
{
public:
    virtual ~DomainEntity() = default;

};

AGIDDS_NAMESPACE_END
