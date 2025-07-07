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

#ifndef DYNAMIC_TYPESUPPORT_H
#define DYNAMIC_TYPESUPPORT_H

#include "dcps/topic/typesupport.h"

AGIDDS_NAMESPACE_BEGIN

class DomainParticipant;
class DynamicType;

class DynamicTypeSupport : public TypeSupport {   
public:
    /**
     * @brief   创建并返回一个新的DynamicTypeSupport对象，该对象能够将给定类型注册到DomainParticipant。
     * 
     * @details 实现应确保新的类型支持具有给定类型对象的“副本”，以便随后对参数对象的更改或删除不会影响新的类型支持。
     *          所有由此操作返回的对象最终应通过调用delete_type_support删除。
     * 
     * @param type 要为其创建类型支持的类型。如果此参数为空或是@nested内部类型，则操作应失败并返回空值。
     * 
     * @return  指向新创建的DynamicTypeSupport对象的指针，如果发生错误则返回空值。
     */
    static DynamicTypeSupport* create_type_support(DynamicType* type);

    /**
     * @brief 删除由此工厂先前创建的给定类型支持对象，如果此参数为空，则操作应成功返回而没有任何可观察到的效果。
     * 
     * @param type_support 要删除的类型支持对象。如果此参数是已删除的对象，并且实现能够检测到这一事实（这不是必需的），
     *                     则此操作应失败并返回RETCODE_ALREADY_DELETED。如果发生实现特定的错误，此方法应失败并返回RETCODE_ERROR。
     * 
     * @return  如果操作成功则返回RETCODE_OK，如果对象已删除则返回RETCODE_ALREADY_DELETED，如果发生实现特定的错误则返回RETCODE_ERROR。
     */
    static ReturnCode_t delete_type_support(DynamicTypeSupport *type_support);
};

AGIDDS_NAMESPACE_END

#endif // DYNAMIC_TYPESUPPORT_H