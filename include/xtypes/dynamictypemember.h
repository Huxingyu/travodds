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

#ifndef DYNAMIC_TYPE_MEMBER_H
#define DYNAMIC_TYPE_MEMBER_H

#include "travodds_dll.h"
#include "dcps/base/retcode.h"
#include "xtypes/annotationdescriptor.h"
#include "xtypes/memberdescriptor.h"
#include <string>
#include <map>

TRAVODDS_NAMESPACE_BEGIN

/**
 * @brief DynamicTypeMember表示类型的“成员”，具有引用语义。
 * 
 * @details “成员”可以是聚合类型的成员、枚举中的常量或其他类型的子结构。具体来说，其行为如下表所述，基于成员所属的DynamicType的TypeKind。
 *          
 *          类型种类 | 含义
 *          ---     | ---
 *          ANNOTATION_TYPE	| 对于这些聚合类型，“成员”在这种意义上与聚合类型定义中的含义相同。
 *          STRUCTURE_TYPE | 对于这些聚合类型，“成员”在这种意义上与聚合类型定义中的含义相同。
 *          UNION_TYPE | 对于这些聚合类型，“成员”在这种意义上与聚合类型定义中的含义相同。
 *          BITMASK_TYPE | 位掩码中的每个命名标志应被视为该位掩码的“成员”，其类型为布尔类型。
 *          ENUMERATION_TYPE | 枚举中的每个字面值应被视为该类型的“成员”。这些成员的类型应为包含它们的枚举本身的类型。
 *          ALIAS_TYPE | 行为与别名的基础类型相同。
 *          其他类型 | 不被视为具有成员。
 */
class DDS_DLL DynamicTypeMember {
public:
    /**
     * @brief 获取所有应用于此成员的注释
     */
    std::vector<AnnotationDescriptor> annotation_;

public:

    DynamicTypeMember() = default;
    /**
     * @brief 构造函数
     */
    DynamicTypeMember(const MemberDescriptor& descriptor);
    
    /**
     * @brief 拷贝构造。
     */
    DynamicTypeMember(const DynamicTypeMember&);

    /**
     * @brief operator=函数
     */
    DynamicTypeMember& operator=(const DynamicTypeMember&);

    /**
     * @brief 获取该成员的描述。
     * 
     * @param [in,out]  descriptor 出口参数。
     * 
     * @return 成功返回RETCODE_OK ，失败返回RETCODE_BAD_PARAMETER
     */
    ReturnCode_t get_descriptor(MemberDescriptor& descriptor) const;

    /**
     * @brief 判断两个成员是否相等
     * 
     * @param other 另一个成员。
     * 
     * @return bool 相等返回true，否则返回false
     */
    bool equals(const DynamicTypeMember& other) const;

    /**
     * @brief 获取成员ID。
     * 
     * @return MemberId 成员ID。
     */
    MemberId get_id() const;

    /**
     * @brief 获取成员名称。
     * 
     * @return std::string 成员名称。
     */
    std::string get_name() const;

    /**
     * @brief 该成员是否含有key标注。
     */
    bool is_key() const;
private:
    /** @brief 成员描述符 */
    MemberDescriptor descriptor_;
};

TRAVODDS_NAMESPACE_END

#endif // DYNAMIC_TYPE_MEMBER_H