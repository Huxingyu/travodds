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
#ifndef TRAVODDS_RTPS_TRANSPORT_SHAREDMEMENTITY_H
#define TRAVODDS_RTPS_TRANSPORT_SHAREDMEMENTITY_H


#include "rtps/transport/interprocesstype.h"

TRAVODDS_NAMESPACE_BEGIN


class SharedMemEntity {

public:
    virtual ~SharedMemEntity();

protected:
    SharedMemEntity(
        const char* sharedMemoryName,
        const char* objectName,
        const char* mutexName,
        size_t size);

    void CleanupResources();

    const char* sharedMemoryName_;
    const char* objectName_;
    const char* mutexName_;
	size_t sharedMemSize_ = 1024;
    uint32_t remaining_ = ~0;

    std::unique_ptr<InterprocessType::managed_shared_memory> segment_;
    std::unique_ptr<InterprocessType::named_mutex> mutex_;
};

TRAVODDS_NAMESPACE_END

#endif //TRAVODDS_RTPS_TRANSPORT_SHAREDMEMENTITY_H