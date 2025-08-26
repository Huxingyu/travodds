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

#ifndef DYNAMIC_TYPE_BUILDER_H
#define DYNAMIC_TYPE_BUILDER_H

#include "travodds_dll.h"
#include "xtypes/dynamictype.h"
#include "xtypes/annotationdescriptor.h"
#include "xtypes/memberdescriptor.h"
#include <string>
#include <map>
#include <vector>
#include <memory>

TRAVODDS_NAMESPACE_BEGIN

/**
 * @brief DynamicTypeBuilder类表示根据类型系统定义的特定类型的过渡状态。
 * 
 * @details 它用于实例化具体的DynamicType对象。
 */
class DynamicTypeBuilder {
    friend class DynamicTypeBuilderFactoryImpl;
public:
    /**
     * @brief 目前所有应用于此类型的注释。
     */
    std::vector<AnnotationDescriptor> annotation_;

    /**
     * @brief 成员ID到成员的映射
     * 
     * @details 如果此类型是聚合类型，通过此属性可用的成员集合应等于通过member_by_name属性可用的成员集合（元素顺序除外）。
     *          如果此类型不是聚合类型，成员集合应为空。
     */
    std::map<MemberId, DynamicTypeMember> member_;

    /**
     * @brief 成员名称到成员的映射。
     * 
     * @details 此属性包含从此类型的成员名称到成员本身的映射。如在add_member的情况下所述，不仅聚合类型的成员被视为“成员”：枚举、位掩码和其他类型的组成部分也被视为此属性的“成员”。
     *          DynamicTypeMember对象的生命周期由包含它的DynamicTypeBuilder的生命周期管理。
     *          前者应被认为从相应成员添加到后者时起逻辑上存在，直到后者被删除。
     *          实现可以更频繁地分配和释放DynamicTypeMember对象，前提是：
     *          
     *          - DynamicTypeMember类的用户不需要显式删除该类的对象。
     *          - 对表示给定成员的一个DynamicTypeMember对象的更改应反映在所有表示相同成员的可观察DynamicTypeMember对象中。
     *          - 所有表示相同成员的DynamicTypeMember对象在其equals操作中应比较为相等。
     */
    std::map<std::string, DynamicTypeMember> member_by_name_;

public:
    /**
     * @brief 将给定的注释应用于此类型。它随后将出现在注释属性中。
     * 
     * @param descriptor 注释的描述符。
     * 
     * @return ReturnCode_t 成功返回RETCODE_OK，失败返回RETCODE_BAD_PARAMETER
     */
    ReturnCode_t apply_annotation(const AnnotationDescriptor& descriptor);

    /**
     * @brief 获取类型描述。
     * 
     * @param [in, out] descriptor 出口参数。
     * 
     * @return  成功返回RETCODE_OK，失败返回RETCODE_BAD_PARAMETER
     */
    ReturnCode_t get_descriptor(TypeDescriptor& descriptor) const;

    /**
     * @brief 添加成员
     * 
     * @details 将“成员”添加到此类型，其中新“成员”的含义在DynamicTypeMember类的规范中定义。
     *          对于该表中未给出的类型种类，此操作应失败并返回RETCODE_PRECONDITION_NOT_MET。
     *          成功返回后，新成员应出现在member属性中，并可能出现在member_by_id属性中，具体取决于该属性的定义。
     * 
     * @param descriptor 新成员的描述符。如果此参数为nil，则操作应失败并返回RETCODE_BAD_PARAMETER。
     * 
     * @return ReturnCode_t 成功返回RETCODE_OK，失败返回RETCODE_BAD_PARAMETER
     */
    ReturnCode_t add_member(const MemberDescriptor& descriptor);
    
    /**
     * @brief 应用注释到成员
     * 
     * @details 将给定的注释应用于此成员。它随后将出现在标识成员的注释属性中。
     * 
     * @param member_id 成员ID，用于标识要应用注释的成员。
     * @param descriptor 注释的描述符。如果此参数不一致，操作应失败并返回RETCODE_BAD_PARAMETER。
     * 
     * @return ReturnCode_t 成功返回RETCODE_OK，失败返回RETCODE_BAD_PARAMETER
     */
    ReturnCode_t apply_annotation_to_member(MemberId member_id, const AnnotationDescriptor& descriptor);

    /**
     * @brief 获取注释数量。
     * 
     * @return size_t 注释数量
     */
    unsigned int get_annotation_count() const;

    /**
     * @brief 获取指定索引的注释
     * 
     * @param descriptor [in,out]   应用程序提供的对象
     * @param index 注释索引
     * 
     * @return 成功返回RETCODE_OK，失败返回RETCODE_BAD_PARAMETER
     */
    ReturnCode_t get_annotation(AnnotationDescriptor& descriptor, unsigned int index) const;

    /**
     * @brief 获取类型种类
     * 
     * @return TypeKind 类型种类
     */
    TypeKind get_kind() const;

    /**
     * @brief 获取指定成员ID的成员
     * 
     * @param member [in,out]   用户提供的对象。
     * @param id 成员ID
     * 
     * @return 成功返回RETCODE_OK，失败返回RETCODE_BAD_PARAMETER
     */
    ReturnCode_t get_member(DynamicTypeMember& member, MemberId id) const;

    /**
     * @brief 获取指定索引的成员
     *
     * @param member [in,out]   用户提供的对象。 
     * @param index 成员索引
     * 
     * @return 成功返回RETCODE_OK，失败返回RETCODE_BAD_PARAMETER
     */
    ReturnCode_t get_member_by_index(DynamicTypeMember& member, unsigned int index) const;

    /**
     * @brief 获取指定名称的成员。
     *
     * @param member [in,out]   用户提供的对象。 
     * @param name 成员名称。
     * 
     * @return 成功返回RETCODE_OK，失败返回RETCODE_BAD_PARAMETER
     */
    ReturnCode_t get_member_by_name(DynamicTypeMember& member, const std::string& name) const;

    /**
     * @brief 获取成员数量
     * 
     * @return 成员数量
     */
    unsigned int get_member_count() const;

    /**
     * @brief 获取类型名称
     * 
     * @return std::string 类型名称
     */
    std::string get_name() const;

    /**
     * @brief 创建DynamicType对象
     * 
     * @return DynamicType 创建的DynamicType对象
     */
    DynamicType* build() const;

    /**
     * @brief 判断两个类型是否相等
     * 
     * @param other 另一个类型
     * @return bool 相等返回true，否则返回false
     */
    bool equals(const DynamicTypeBuilder& other) const;
private:

    /**
     * @brief 绑定指定TypeDescriptor的构造函数
     */
    DynamicTypeBuilder(const TypeDescriptor& descriptor);

    /**
     * @brief 析构函数
     */
    ~DynamicTypeBuilder();

    /** @brief  禁止拷贝构造函数 */
    DynamicTypeBuilder(const DynamicTypeBuilder& other);

    /** @brief  禁止赋值操作符 */
    DynamicTypeBuilder& operator=(const DynamicTypeBuilder& other);
private:
    /** @brief  类型描述 */
    TypeDescriptor descriptor_;
};

TRAVODDS_NAMESPACE_END

#endif // DYNAMIC_TYPE_BUILDER_H
