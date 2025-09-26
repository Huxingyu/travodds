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
#ifndef PROPERTY_QOS_POLICY_H
#define PROPERTY_QOS_POLICY_H

#include "common/sequence/sequence.h"
#include <string>
#include "rtps/common/BinaryProperty.h"

TRAVODDS_NAMESPACE_BEGIN

/**
 * @struct PropertyQosPolicy
 *
 * @ingroup CoreQosStruct
 *
 * @brief   属性配置。
 *
 * @details
 *          成员 | 默认值 | 匹配规则检查 | 有效性检查 | 兼容性检查 | 可变性检查
 *          --- | --- | --- | --- | --- | ---
 *          #value | 空 | 无 | 无 | 无 | 不可变
 *          #binary_value | 空 | 无 | 无 | 无 | 不可变
 *
 */
    typedef struct PropertyQosPolicy
{
    /** @brief   字符串值。 */
    //DDS_PropertySeq value;
    PropertySeq value;
    /** @brief   缓冲区值。 */
    BinaryPropertySeq  binary_value;
    //!Get properties
    const PropertySeq& properties() const
    {
        return value;
    }

    //!Set properties
    PropertySeq& properties()
    {
        return value;
    }

    //!Get binary_properties
    const BinaryPropertySeq& binary_properties() const
    {
        return binary_value;
    }

    //!Set binary_properties
    BinaryPropertySeq& binary_properties()
    {
        return binary_value;
    }

}PropertyQosPolicy;

TRAVODDS_NAMESPACE_END

#endif // PROPERTY_QOS_POLICY_H
