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

#ifndef ANNOTATION_DESCRIPTOR_H
#define ANNOTATION_DESCRIPTOR_H

#include "travodds_dll.h"
#include <string>
#include <map>

TRAVODDS_NAMESPACE_BEGIN

class DynamicType;

/**
 * @class AnnotationDescriptor
 * 
 * @brief AnnotationDescriptor将注释的状态打包在一起，并将其应用于某些元素。该类对象具有值语义，允许它们被深度复制和比较。
 */

class AnnotationDescriptor {
public:
    /** @brief type属性包含对注释类型的引用，该注释类型描述了此描述符的实例。 */
    DynamicType* type_;

    /**
     * @brief   value属性包含从type定义的属性名称到该类型的有效值的映射。
     *          任何由type定义但在此属性中没有名称出现的属性都应被视为具有其默认值。
     *          尽管注释类型成员可以具有其他类型，但此属性中的每个属性值都表示为字符串。
     *          如果数据值的字符串表示形式是相应类型的有效IDL文字，则该字符串表示形式被视为格式良好，具有以下限定条件：
     *          
     *          - 字符串和字符文字不应被引号（‘"’或‘'’）包围。
     *          - 所有表达式应完全求值，以使值中不出现运算符或其他非文字字符。例如，“5”应被视为整数数量五的格式良好的字符串表示形式，但“2 + ENUM_VALUE_THREE”不应被视为格式良好的字符串表示形式。
     */
    std::map<std::string, std::string> value_;
public:

    /**
     * @brief 默认构造函数
     * 
     * 创建一个空的AnnotationDescriptor实例。
     */
    AnnotationDescriptor() = default;

    /**
     * @brief 拷贝构造函数
     */
    AnnotationDescriptor(const AnnotationDescriptor& other);

    /**
     * @brief 析构函数
     */
    ~AnnotationDescriptor();

    /**
     * @brief operator=构造函数
     */
    AnnotationDescriptor& operator=(const AnnotationDescriptor& other);

    /**
     * @brief 用另一个描述符的内容覆盖此描述符的内容。
     * @param other 要复制其内容的描述符。如果此参数为nil，则操作将失败并返回RETCODE_BAD_PARAMETER。
     * @return 如果操作失败，则返回RETCODE_ERROR，否则返回成功代码。
     */
    int copy_from(const AnnotationDescriptor& other);

    /**
     * @brief 检查此描述符是否等于另一个描述符。
     * @param other 要与此描述符进行比较的另一个描述符。如果此参数为nil，则操作将返回false。
     * @return 如果描述符相等，则返回true，否则返回false。
     */
    bool equals(const AnnotationDescriptor& other) const;

    /**
     * @brief 指示此描述符是否描述了一个有效的注释类型实例。
     * @return 如果描述符一致，则返回true，否则返回false。
     */
    bool is_consistent() const;
};

TRAVODDS_NAMESPACE_END

#endif // ANNOTATION_DESCRIPTOR_H