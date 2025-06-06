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
#ifndef AGIDDS_DCPS_SUBSCRIBE_STATES_H
#define AGIDDS_DCPS_SUBSCRIBE_STATES_H

#include "agidds_dll.h"
AGIDDS_NAMESPACE_BEGIN

// Sample states to support reads
using SampleStateKind = uint32_t;
const  SampleStateKind		READ_SAMPLE_STATE = 0x0001 << 0;
const  SampleStateKind		NOT_READ_SAMPLE_STATE = 0x0001 << 1;
// This is a bit-mask SampleStateKind
using SampleStateMask = uint32_t;
const  SampleStateMask 		ANY_SAMPLE_STATE = 0xffff;

// View states to support reads
using ViewStateKind = uint32_t;
const  ViewStateKind		NEW_VIEW_STATE = 0x0001 << 0;
const  ViewStateKind		NOT_NEW_VIEW_STATE = 0x0001 << 1;

// This is a bit-mask ViewStateKind
using ViewStateMask = uint32_t;
const  ViewStateMask 		ANY_VIEW_STATE = 0xffff;

// Instance states to support reads
using  InstanceStateKind = uint32_t;
const  InstanceStateKind		ALIVE_INSTANCE_STATE = 0x0001 << 0;
const  InstanceStateKind 		NOT_ALIVE_DISPOSED_INSTANCE_STATE = 0x0001 << 1;
const  InstanceStateKind 		NOT_ALIVE_NO_WRITERS_INSTANCE_STATE = 0x0001 << 2;

// This is a bit-mask InstanceStateKind
using InstanceStateMask = uint32_t;
const  InstanceStateMask 	ANY_INSTANCE_STATE = 0xffff;
const  InstanceStateMask 	NOT_ALIVE_INSTANCE_STATE = 0x006;

AGIDDS_NAMESPACE_END
#endif // ! AGIDDS_DCPS_SUBSCRIBE_STATES_H