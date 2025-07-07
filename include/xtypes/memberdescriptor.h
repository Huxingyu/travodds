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

#ifndef MEMBER_DESCRIPTOR_H
#define MEMBER_DESCRIPTOR_H

#include "agidds_dll.h"
#include "xtypes/typekind.h"
#include "dcps/base/retcode.h"
#include <string>
#include <vector>

AGIDDS_NAMESPACE_BEGIN

class DynamicType;

/**
 * @brief MemberDescriptor类打包了DynamicTypeMember的状态。
 * 
 * @details MemberDescriptor对象具有值语义，允许它们被深度复制和比较。
 */
class MemberDescriptor {
public:
    /**
     * @brief 成员ID
     */
    MemberId id_;

    /**
     * @brief 成员索引
     */
    unsigned int index_;

    /**
     * @brief 成员名称
     */
    std::string name_;

    /**
     * @brief 成员类型
     */
    DynamicType* type_;

    /**
     * @brief 默认标签
     */
    bool default_label_;

    /**
     * @brief 默认值
     */
    std::string default_value_;

    /**
     * @brief 标签
     */
    std::vector<uint64_t> label_;
public:
    /**
     * @brief 构造函数
     */
    MemberDescriptor();

    /**
     * @brief 拷贝构造函数
     */
    MemberDescriptor(const MemberDescriptor& other);

    /**
     * @brief operator=构造函数
     */
    MemberDescriptor& operator=(const MemberDescriptor& other);

    /**
     * @brief 复制内容
     * 
     * @param other 要复制内容成员描述。
     * 
     * @return 成功返回RETCODE_OK，失败返回RETCODE_ERROR
     */
    ReturnCode_t copy_from(const MemberDescriptor& other);

    /**
     * @brief 判断两个描述符是否相等
     * 
     * @param other 另一个描述符
     * @return bool 相等返回true，否则返回false
     */
    bool equals(const MemberDescriptor& other) const;

    /**
     * @brief 判断描述符是否一致
     * 
     * @return bool 一致返回true，否则返回false
     */
    bool is_consistent() const;
};

AGIDDS_NAMESPACE_END

#endif // MEMBER_DESCRIPTOR_H
