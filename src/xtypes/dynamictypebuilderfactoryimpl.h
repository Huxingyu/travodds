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

#ifndef DYNAMIC_TYPE_BUILDER_FACTORY_IMPL_H
#define DYNAMIC_TYPE_BUILDER_FACTORY_IMPL_H

#include "xtypes/dynamictypebuilderfactory.h"
#include "xtypes/typekind.h"
#include <map>
#include <mutex>

AGIDDS_NAMESPACE_BEGIN

/**
 * @brief DynamicTypeBuilderFactoryImpl类是DynamicTypeBuilderFactory的实现类。
 */
class DynamicTypeBuilderFactoryImpl : public DynamicTypeBuilderFactory {
  private:
    DynamicTypeBuilderFactoryImpl();
    ~DynamicTypeBuilderFactoryImpl();

  public:
    static DynamicTypeBuilderFactoryImpl *get_instance();
    static ReturnCode_t delete_instance();
    DynamicType *get_primitive_type(int kind) override;
    DynamicTypeBuilder *create_type(TypeDescriptor *descriptor) override;
    DynamicTypeBuilder *create_type_copy(DynamicType *type) override;
    DynamicTypeBuilder *
    create_type_w_type_object(TypeObject *type_object) override;
    DynamicTypeBuilder *create_string_type(unsigned int bound) override;
    DynamicTypeBuilder *create_wstring_type(unsigned int bound) override;
    DynamicTypeBuilder *create_sequence_type(DynamicType *element_type,
                                             unsigned int bound) override;
    DynamicTypeBuilder *
    create_array_type(DynamicType *element_type,
                      const std::vector<unsigned int> &bound) override;
    DynamicTypeBuilder *create_map_type(DynamicType *key_element_type,
                                        DynamicType *element_type,
                                        unsigned int bound) override;
    DynamicTypeBuilder *create_bitmask_type(unsigned int bound) override;
    DynamicType *
    create_type_w_uri(const std::string &document_url,
                      const std::string &type_name,
                      const std::vector<std::string> &include_paths) override;
    DynamicType *create_type_w_document(
        const std::string &document, const std::string &type_name,
        const std::vector<std::string> &include_paths) override;
    ReturnCode_t delete_type(DynamicType *type) override;
    ReturnCode_t delete_type_builder(DynamicTypeBuilder *type_builder) override;

  private:
    /** @brief  共用基本数据类型的DynamicTypr。 */
    std::map<TypeKind, DynamicType *> primitive_types_;
    /** @brief  单例对象。 */
    static DynamicTypeBuilderFactoryImpl *s_instance_;
    /** @brief  互斥锁。 */
    std::mutex mutex_;
};

AGIDDS_NAMESPACE_END

#endif // DYNAMIC_TYPE_BUILDER_FACTORY_IMPL_H
