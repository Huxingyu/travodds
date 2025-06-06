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
#ifndef AGIDDS_RTPS_ENDPOINT_READERLOCATOR_H
#define AGIDDS_RTPS_ENDPOINT_READERLOCATOR_H

#include "rtps/basictypertps.h"
#include "rtps/historycache/historycache.h"

AGIDDS_NAMESPACE_BEGIN

class ReaderLocator {
public:
	ReaderLocator(const Locator_t& locator);

	SequenceNumber_t NextRequestedChange();

	SequenceNumber_t NextUnsentChange();

	SeqNumberVec RequestedChanges();

	void RequestedChangesSet(SeqNumberVec reqSeqNumSet);

	bool UnsentChanges();

	void SetHistoryCache(std::shared_ptr<HistoryCache> writerCache);

	bool ResetHighestSentChange();

	Locator_t GetLocator();

	//bool sendData();
private:
	SequenceNumber_t highestSentChangeSN_;
	SeqNumberVec requestedChanges_;
	Locator_t locator_;
	bool expectsInlineQos_;

	std::shared_ptr<HistoryCache> writerCache_ = nullptr;
};
AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_ENDPOINT_READERLOCATOR_H
