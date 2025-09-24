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
#ifndef TRAVODDS_DCPS_DOMAIN_DOMAINENTITY_H
#define TRAVODDS_DCPS_DOMAIN_DOMAINENTITY_H

#include "dcps/base/entity.h"

TRAVODDS_NAMESPACE_BEGIN

#define DEF_NARROW_METHOD(TYPE) \
    static int32_t type_id(); \
    virtual void* narrow_helper(int32_t type_const)

#define IMPL_NARROW_METHOD(TYPE)\
    int32_t \
    TYPE::type_id()\
{\
    return 0;\
    }\
    void* \
    TYPE::narrow_helper(int32_t type_const) {\
    void* rval = NULL; \
    if (type_const == (int32_t)&TYPE::type_id) \
    rval = this; \
    return rval; \
    }

/**
 * @brief DomainEntity类是Entity的一个子类，是为了区分DomainParticits和其他实体而创建的
 */
class DDS_DLL DomainEntity : public virtual Entity
{
public:
    /**
     * @brief 析构函数。
     */
    virtual ~DomainEntity() = default;

};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_DOMAIN_DOMAINENTITY_H