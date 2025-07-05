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
#ifndef AGIDDS_DCPS_LISTENER_LISTENER_H
#define AGIDDS_DCPS_LISTENER_LISTENER_H

#include "agidds_dll.h"

AGIDDS_NAMESPACE_BEGIN

class DDS_DLL Listener {
public:
	Listener() = default;

    virtual ~Listener() = default;
};

AGIDDS_NAMESPACE_END
#endif // !AGIDDS_DCPS_LISTENER_LISTENER_H