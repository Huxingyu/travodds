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
#pragma once

#include "RTPS/BasicTypeRTPS.h"
#include "RTPS/HistoryCache.h"

AGIDDS_NAMESPACE_BEGIN

class ReaderLocator {
public:
	ReaderLocator(Locator_t locator);

	SequenceNumber_t NextRequestedChange();

	SequenceNumber_t next_unsent_change(SequenceNumber_t seq_num_max);

	vector<SequenceNumber_t> RequestedChanges();

	void requested_changes_set(vector<SequenceNumber_t> req_seq_num_set);

	vector<SequenceNumber_t> UnsentChanges();

	bool ResetHighestSentChange();

	bool sendData();
private:
	SequenceNumber_t highestSentChangeSN_;
	vector<SequenceNumber_t> requestedChanges_;
	Locator_t locator_;
	bool expectsInlineQos_;
};
AGIDDS_NAMESPACE_END

