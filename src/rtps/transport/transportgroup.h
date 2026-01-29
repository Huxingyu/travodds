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
#ifndef TRAVODDS_RTPS_TRANSPORT_TRANSPORTGROUP_H
#define TRAVODDS_RTPS_TRANSPORT_TRANSPORTGROUP_H

#include "rtps/basictypertps.h"

TRAVODDS_NAMESPACE_BEGIN

class TransportGroup {
public:
    TransportGroup(){};
    ~TransportGroup(){};
    void AddLocator(const GuidPrefix_t& prefix,const LocatorSeq& locators);
    void DecreaseLocator(const GuidPrefix_t& prefix);
    void RemoveLocator(const GuidPrefix_t& prefix);
    void GetLocator(const std::set<GuidPrefix_t>  prefixs,std::vector<LocatorSeq>& locators);

private:
    std::map<GuidPrefix_t,std::pair<std::vector<Locator_t>, uint32_t>> locators_;
    std::mutex mtx_;
};



TRAVODDS_NAMESPACE_END
#endif