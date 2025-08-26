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

#ifndef DYNAMIC_TYPE_BUILDER_FACTORY_H
#define DYNAMIC_TYPE_BUILDER_FACTORY_H

#include "travodds_dll.h"
#include "dcps/base/retcode.h"
#include <vector>
#include <string>

TRAVODDS_NAMESPACE_BEGIN

class DynamicType;
class DynamicTypeBuilder;
class TypeDescriptor;
class TypeObject;

/**
 * @class DynamicTypeBuilderFactory
 * 
 * @brief 该类在逻辑上是一个单例，它的“唯一”实例是创建和删除DynamicTypeBuilder对象的入口。
 */
class DDS_DLL DynamicTypeBuilderFactory {
public:

    /**
     * @brief 返回一个行为类似于单例的DynamicTypeBuilderFactory实例，多线程不安全函数。
     * 
     * @return  指向DynamicTypeBuilderFactory实例的指针，如果发生错误则返回空值。
     */
    static DynamicTypeBuilderFactory* get_instance();

    /**
     * @brief 回收以前从get_instance返回的任何对象相关的资源，多线程不安全函数。
     * 
     * @return 如果操作成功则返回RETCODE_OK，如果因任何实现特定原因失败则返回RETCODE_ERROR。
     */
    static ReturnCode_t delete_instance();

    /**
     * @brief 检索与指定的原始类型种类对应的DynamicType对象。
     * 
     * @param kind 要返回其表示的原始类型的种类。如果给定的种类不对应于原始类型，则操作应失败并返回空值。
     * @return 指向DynamicType对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicType* get_primitive_type(int kind) = 0;

    /**
     * @brief 根据给定的类型描述符创建并返回一个新的DynamicTypeBuilder对象。
     * 
     * @param descriptor 要创建的新类型的属性。如果此参数为空或不一致，则操作应失败并返回空值。
     * @return 指向新创建的DynamicTypeBuilder对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicTypeBuilder* create_type(TypeDescriptor* descriptor) = 0;

    /**
     * @brief 创建并返回一个具有给定类型状态副本的新DynamicTypeBuilder对象。
     * 
     * @param type 要创建的新类型的初始状态。如果此参数为空，则操作应失败并返回空值。
     * @return 指向新创建的DynamicTypeBuilder对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicTypeBuilder* create_type_copy(DynamicType* type) = 0;

    /**
     * @brief 创建并返回一个描述与给定TypeObject对象相同类型的新DynamicTypeBuilder对象。
     * 
     * @param type_object 要创建的新类型的初始状态。
     * @return DynamicTypeBuilder* 指向新创建的DynamicTypeBuilder对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicTypeBuilder* create_type_w_type_object(TypeObject* type_object) = 0;

    /**
     * @brief 创建并返回一个表示字符串类型的新DynamicTypeBuilder对象。
     * 
     * @param bound 可以存储在新类型字符串中的元素的最大数量。如果此参数等于LENGTH_UNLIMITED，则字符串类型应被视为无界。
     * @return 指向新创建的DynamicTypeBuilder对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicTypeBuilder* create_string_type(unsigned int bound) = 0;

    /**
     * @brief 创建并返回一个表示宽字符串类型的新DynamicTypeBuilder对象。
     * 
     * @param bound 可以存储在新类型宽字符串中的元素的最大数量。如果此参数等于LENGTH_UNLIMITED，则宽字符串类型应被视为无界。
     * @return 指向新创建的DynamicTypeBuilder对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicTypeBuilder* create_wstring_type(unsigned int bound) = 0;

    /**
     * @brief 创建并返回一个表示序列类型的新DynamicTypeBuilder对象。
     * 
     * @param element_type 可以存储在新类型序列中的所有对象的类型。如果此参数为空，则操作应失败并返回RETCODE_BAD_PARAMETER。
     * @param bound 可以存储在新类型序列中的元素的最大数量。如果此参数等于LENGTH_UNLIMITED，则序列类型应被视为无界。
     * @return 指向新创建的DynamicTypeBuilder对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicTypeBuilder* create_sequence_type(DynamicType* element_type, unsigned int bound) = 0;

    /**
     * @brief 创建并返回一个表示数组类型的新DynamicTypeBuilder对象。
     * 
     * @param element_type 可以存储在新类型数组中的所有对象的类型。如果此参数为空，则操作应失败并返回RETCODE_BAD_PARAMETER。
     * @param bound 一个无符号整数集合，其长度等于新数组类型中的维数，其值为每个维度的边界。
     * @return 指向新创建的DynamicTypeBuilder对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicTypeBuilder* create_array_type(DynamicType* element_type, const std::vector<unsigned int>& bound) = 0;

    /**
     * @brief 创建并返回一个表示映射类型的新DynamicTypeBuilder对象。
     * 
     * @param key_element_type 可以存储在新类型映射中的所有键对象的类型。如果此参数为空，则操作应失败并返回RETCODE_BAD_PARAMETER。
     * @param element_type 可以存储在新类型映射中的所有值对象的类型。如果此参数为空，则操作应失败并返回RETCODE_BAD_PARAMETER。
     * @param bound 可以存储在新类型映射中的键值对的最大数量。如果此参数等于LENGTH_UNLIMITED，则映射类型应被视为无界。
     * @return DynamicTypeBuilder* 指向新创建的DynamicTypeBuilder对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicTypeBuilder* create_map_type(DynamicType* key_element_type, DynamicType* element_type, unsigned int bound) = 0;

    /**
     * @brief 创建并返回一个表示位掩码类型的新DynamicTypeBuilder对象。
     * 
     * @param bound 位掩码中保留的位数。如果此值超出范围，则操作应失败并返回RETCODE_BAD_PARAMETER。
     * @return DynamicTypeBuilder* 指向新创建的DynamicTypeBuilder对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicTypeBuilder* create_bitmask_type(unsigned int bound) = 0;

    /**
     * @brief 通过解析给定URL的类型描述创建并返回一个新的DynamicType对象。
     * 
     * @param document_url 指示类型描述文档的URL，该文档应被解析以创建DynamicType对象。
     * @param type_name 要从URL目标文档加载的类型的全名。
     * @param include_paths 要搜索的目录URL集合，以查找可能被document_url目标文档直接或间接包含的其他类型描述文档。
     * @return 指向新创建的DynamicType对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicType* create_type_w_uri(const std::string& document_url, const std::string& type_name, const std::vector<std::string>& include_paths) = 0;

    /**
     * @brief 通过解析给定字符串中包含的类型描述创建并返回一个新的DynamicType对象。
     * 
     * @param document 类型描述文档，应解析该文档以创建DynamicType对象。
     * @param type_name 要从文档加载的类型的全名。
     * @param include_paths 要搜索的目录URL集合，以查找可能被document参数直接或间接包含的其他类型描述文档。
     * @return DynamicType* 指向新创建的DynamicType对象的指针，如果发生错误则返回空值。
     */
    virtual DynamicType* create_type_w_document(const std::string& document, const std::string& type_name, const std::vector<std::string>& include_paths) = 0;

    /**
     * @brief 删除由此工厂先前创建的给定DynamicType对象。
     * 
     * @param type 要删除的类型。如果此参数是已删除的对象，则此操作应失败并返回RETCODE_ALREADY_DELETED。如果发生实现特定的错误，此方法应失败并返回RETCODE_ERROR。
     * @return ReturnCode_t 如果操作成功则返回RETCODE_OK，如果对象已删除则返回RETCODE_ALREADY_DELETED，如果发生实现特定的错误则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t delete_type(DynamicType* type) = 0;
    
    /**
     * @brief 删除由此工厂先前创建的给定DynamicTypeBuilder对象。
     * 
     * @param type_builder 要删除的类型构建器。如果此参数是已删除的对象，则此操作应失败并返回RETCODE_ALREADY_DELETED。如果发生实现特定的错误，此方法应失败并返回RETCODE_ERROR。
     * 
     * @return 如果操作成功则返回RETCODE_OK，如果对象已删除则返回RETCODE_ALREADY_DELETED，如果发生实现特定的错误则返回RETCODE_ERROR。
     */
    virtual ReturnCode_t delete_type_builder(DynamicTypeBuilder* type_builder) = 0;
protected:
    DynamicTypeBuilderFactory() = default;
    ~DynamicTypeBuilderFactory() = default;
    DynamicTypeBuilderFactory(const DynamicTypeBuilderFactory&) = default;
    DynamicTypeBuilderFactory& operator=(const DynamicTypeBuilderFactory&) = default;
};

TRAVODDS_NAMESPACE_END

#endif // DYNAMIC_TYPE_BUILDER_FACTORY_H