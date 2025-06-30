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
#ifndef AGIDDS_DCPS_DOMAIN_DOMAINENTITYIMPL_H
#define AGIDDS_DCPS_DOMAIN_DOMAINENTITYIMPL_H

#include "dcps/domain/domainentity.h"
#include "dcps/base/entityimpl.h"

AGIDDS_NAMESPACE_BEGIN
/**
 * @brief DomainEntityImpl类是DomainEntity和EntityImpl的一个子类，是为了区分DomainParticits和其他实体而创建的
 */
class DomainEntityImpl : public virtual DomainEntity, public EntityImpl {
public:
    /**
     * @brief 构造函数。
     */
    DomainEntityImpl(const StatusMask& mask);
    /**
     * @brief 析构函数。
     */
    ~DomainEntityImpl();
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_DOMAIN_DOMAINENTITYIMPL_H
