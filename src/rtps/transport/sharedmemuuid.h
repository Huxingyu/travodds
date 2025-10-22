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
#ifndef TRAVODDS_RTPS_TRANSPORT_SHAREDMEMUUID_H
#define TRAVODDS_RTPS_TRANSPORT_SHAREDMEMUUID_H
#include <atomic>
#include "rtps/transport/sharedmementity.h"

TRAVODDS_NAMESPACE_BEGIN

class SharedMemUUID : public SharedMemEntity {

public:
	static const SharedMemUUID& GetInstance(); 
    
    virtual ~SharedMemUUID();

    std::array<char, 16> GetUUID() const;

private:
    SharedMemUUID();

    struct SharedUUID {
        std::atomic<uint32_t> referenceCount;
        std::array<char, 16> uuid;
    };

    SharedUUID* shareduuid_ = nullptr;
};


TRAVODDS_NAMESPACE_END

#endif