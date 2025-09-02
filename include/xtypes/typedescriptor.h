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

#ifndef TYPE_DESCRIPTOR_H
#define TYPE_DESCRIPTOR_H

#include "travodds_dll.h"
#include "dcps/base/retcode.h"
#include "xtypes/typekind.h"
#include <string>
#include <vector>

TRAVODDS_NAMESPACE_BEGIN

class DynamicType;

/**
 * @class TypeDescriptor
 * 
 * @brief TypeDescriptor将类型的状态打包在一起。TypeDescriptor对象具有值语义，允许它们被深度复制和比较。
 */
class DDS_DLL TypeDescriptor {
public:
    /** @brief  类型。 */
    TypeKind kind_;
    /** @brief  类型名称。 */
    std::string name_;
    /** @brief  父类型，如果存在。 */
    DynamicType* base_type_;
    /** @brief  辨别器类型，如果存在。 */
    DynamicType* discriminator_type_;
    /** @brief  数组、sequence、string等的维度。 */
    std::vector<unsigned int> bound_;
    /** @brief  sequence、数组等类型的元素类型。 */
    DynamicType* element_type_;
    /** @brief  map类型的键类型。 */
    DynamicType* key_element_type_;
public:

    /**
     * @brief 构造函数
     */
    TypeDescriptor();
    
    /**
     * @brief 拷贝构造函数
     */
    TypeDescriptor(const TypeDescriptor& other);

    /**
     * @brief operator=构造函数
     */
    TypeDescriptor& operator=(const TypeDescriptor& other);

    /**
     * @brief 从给定的类型拷贝一个新的类型描述符。
     * 
     * @param other 要拷贝的类型描述符。
     * 
     * @return ReturnCode_t 如果操作成功则返回RETCODE_OK，如果发生错误则返回RETCODE_ERROR。
     */
    ReturnCode_t copy_from(const TypeDescriptor* other);

    /**
     * @brief 判断是否与给定的类型描述相等。
     * 
     * @param other 要比较的类型描述符。
     * 
     * @return bool 如果相等则返回true，否则返回false。
     */
    bool equals(const TypeDescriptor* other) const;

    /**
     * @brief 判断类型描述符是否一致，属性之间是否存在冲突。
     * 
     * @return bool 如果一致则返回true，否则返回false。
     */
    bool is_consistent() const;

    /**
     * @brief 获取总的维度数量。
     * 
     * @return unsigned int 返回维度数量。
     */
    unsigned int get_bound_count() const;
};

TRAVODDS_NAMESPACE_END

#endif // TYPE_DESCRIPTOR_H
