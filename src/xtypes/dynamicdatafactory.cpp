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

#include "xtypes/dynamicdatafactory.h"
#include "xtypes/dynamicdata.h"

TRAVODDS_NAMESPACE_BEGIN

DynamicDataFactory &DynamicDataFactory::get_instance() {
    static DynamicDataFactory instance;
    return instance;
}

ReturnCode_t DynamicDataFactory::delete_instance() { return RETCODE_OK; }

DynamicData *DynamicDataFactory::create_data(DynamicType &type) const {
    return new DynamicData(type);
}

ReturnCode_t DynamicDataFactory::delete_data(DynamicData &data) const {
    delete &data;
    return RETCODE_OK;
}

TRAVODDS_NAMESPACE_END