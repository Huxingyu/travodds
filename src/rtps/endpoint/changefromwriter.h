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
#ifndef AGIDDS_RTPS_ENDPOINT_CHANGEFORWRITER_H
#define AGIDDS_RTPS_ENDPOINT_CHANGEFORWRITER_H

#include "rtps/basictypertps.h"

AGIDDS_NAMESPACE_BEGIN

enum ChangeFromWriterStatusKind
{
	MISSING        = (1 << 0),
	RECEIVED       = (1 << 1),
	UNKNOWN        = (1 << 2),
	NA_FILTERED    = (1 << 3),
	NA_REMOVED     = (1 << 4),
	NA_UNSPECIFIED = (1 << 5),
	NOT_AVAILABLE  = NA_FILTERED | NA_REMOVED | NA_UNSPECIFIED
};

class CacheChange;
struct ChangeFromWriter {
	SequenceNumber_t sequenceNumber;
	ChangeFromWriterStatusKind status;
	bool is_relevant;
	std::shared_ptr<CacheChange> cache;
};

using ChangeFromWriterSeq = Sequence<ChangeFromWriter>;


AGIDDS_NAMESPACE_END
#endif // !AGIDDS_RTPS_ENDPOINT_CHANGEFORWRITER_H
