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
#ifndef TRAVODDS_RTPS_ENDPOINT_CHANGEFORREADER_H
#define TRAVODDS_RTPS_ENDPOINT_CHANGEFORREADER_H

#include "rtps/basictypertps.h"

TRAVODDS_NAMESPACE_BEGIN

enum ChangeForReaderStatusKind
{
	UNSENT,
	UNACKNOWLEDGED,
	REQUESTED,
	ACKNOWLEDGED,
	UNDERWAY
};

/**
 * @brief ChangeForReader类。
 */
struct ChangeForReader
{
// public:
// 	void set_change_for_reader_status(ChangeForReaderStatusKind status);
//
// 	void set_relevant(bool isRelevant);
//
// 	void set_sequence_number(SequenceNumber_t number);
//
// 	SequenceNumber_t get_sequence_number();
//
// 	ChangeForReaderStatusKind get_change_for_reader_status();

// private:
	ChangeForReaderStatusKind status = ChangeForReaderStatusKind::UNSENT;
	bool isRelevant;
	SequenceNumber_t sequenceNumber = {0};

	/**
	 * @brief 构造函数。
	 */
	ChangeForReader()
	{
		isRelevant = true;
	}
};

using ChangeForReaderVec = std::vector<ChangeForReader>;

TRAVODDS_NAMESPACE_END
#endif
