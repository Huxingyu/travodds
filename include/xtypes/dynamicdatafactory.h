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

#ifndef DYNAMIC_DATA_FACTORY_H
#define DYNAMIC_DATA_FACTORY_H

#include "agidds_dll.h"
#include "xtypes/dynamictype.h"
#include "xtypes/dynamicdata.h"

AGIDDS_NAMESPACE_BEGIN

/**
 * @brief DynamicDataFactory类用于创建DynamicData对象。
 */
class DynamicDataFactory {
public:
    /**
     * @brief 获取DynamicDataFactory的单例实例
     * 
     * @return DynamicDataFactory& 单例实例
     */
    static DynamicDataFactory& get_instance();

    /**
     * @brief 删除单例实例
     * 
     * @return 成功返回RETCODE_OK，失败返回RETCODE_ERROR
     */
    ReturnCode_t delete_instance();

    /**
     * @brief 创建DynamicData对象
     * 
     * @param type DynamicType对象
     * @return DynamicData 创建的DynamicData对象
     */
    DynamicData* create_data(DynamicType& type) const;

    /**
     * @brief 删除DynamicData对象
     * 
     * @param data 要删除的DynamicData对象
     */
    ReturnCode_t delete_data(DynamicData& data) const;

private:
    /**
     * @brief 构造函数
     */
    DynamicDataFactory() = default;

    /**
     * @brief 析构函数
     */
    ~DynamicDataFactory() = default;
};

AGIDDS_NAMESPACE_END

#endif // DYNAMIC_DATA_FACTORY_H
