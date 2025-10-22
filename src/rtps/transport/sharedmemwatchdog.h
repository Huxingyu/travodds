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
#ifndef TRAVODDS_RTPS_TRANSPORT_SHAREDMEMWATCHDOG_H
#define TRAVODDS_RTPS_TRANSPORT_SHAREDMEMWATCHDOG_H

#include "rtps/transport/sharedmementity.h"
#include "dcps/base/retcode.h"

TRAVODDS_NAMESPACE_BEGIN

class SharedMemWatchdog : public SharedMemEntity {
public:
	static const SharedMemWatchdog& GetInstance();
	virtual ~SharedMemWatchdog();
	ReturnCode_t InsertPortProcess(const uint32_t& port) const;
    
private:
	SharedMemWatchdog();
    InterprocessType::SharedList* sharedList_;
};

TRAVODDS_NAMESPACE_END

#endif //TRAVODDS_RTPS_TRANSPORT_SHAREDMEMWATCHDOG_H