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

#include "RTPS/ChangeForReader.h"
#include "RTPS/HistoryCache.h"

AGIDDS_NAMESPACE_BEGIN
class Writer;

class ReaderProxy {
public:
	ReaderProxy(GUID_t guid,LocatorSeq& unicastLocatorList,LocatorSeq& multicastLocatorList);

	void AckedChangesSet(SequenceNumber_t committedSeqNum);

	void RequestedChangesSet(SeqNumberVec requestedChangesSet);

	ChangeForReader NextRequestedChange();

	ChangeForReader NextUnsentChange();

	ChangeForReaderVec UnsentChanges();

	ChangeForReaderVec RequestedChanges();

	ChangeForReaderVec UnackedChanges();

	GUID_t GetGuid();

	bool IsAcked(SequenceNumber_t number);

	bool AddChange(ChangeForReader change);

private:
	GUID_t remoteReaderGuid_;
	LocatorSeq unicastLocatorList_;
	LocatorSeq multicastLocatorList_;
	// SequenceNumber_t highestSentChangeSN_;
	// vector<SequenceNumber_t> requestedChanges_;
	// vector<SequenceNumber_t> acknowledgeChanges_;
	ChangeForReaderVec changesForReaders_;
	bool expectsInlineQos_;
	bool isActive_;
};

AGIDDS_NAMESPACE_END