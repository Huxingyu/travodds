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
#ifndef TRAVODDS_DCPS_SUBSCRIBE_STATES_H
#define TRAVODDS_DCPS_SUBSCRIBE_STATES_H

#include "travodds_dll.h"
TRAVODDS_NAMESPACE_BEGIN

// Sample states to support reads
enum SampleStateKind : uint16_t {
	READ_SAMPLE_STATE = 0x0001 << 0,
	NOT_READ_SAMPLE_STATE = 0x0001 << 1
};
// This is a bit-mask SampleStateKind
using SampleStateMask = uint16_t;
const SampleStateMask 		ANY_SAMPLE_STATE = 0xffff;

// View states to support reads
enum ViewStateKind : uint16_t {
	NEW_VIEW_STATE = 0x0001 << 0,
	NOT_NEW_VIEW_STATE = 0x0001 << 1
};

// This is a bit-mask ViewStateKind
using ViewStateMask = uint16_t;
const ViewStateMask 		ANY_VIEW_STATE = 0xffff;

// Instance states to support reads
enum InstanceStateKind : uint16_t {
	ALIVE_INSTANCE_STATE = 0x0001 << 0,
	NOT_ALIVE_DISPOSED_INSTANCE_STATE = 0x0001 << 1,
	NOT_ALIVE_NO_WRITERS_INSTANCE_STATE = 0x0001 << 2
};
// This is a bit-mask InstanceStateKind
using InstanceStateMask = uint16_t;
const  InstanceStateMask 	ANY_INSTANCE_STATE = 0xffff;
const  InstanceStateMask 	NOT_ALIVE_INSTANCE_STATE = 0x006;

TRAVODDS_NAMESPACE_END
#endif // ! TRAVODDS_DCPS_SUBSCRIBE_STATES_H