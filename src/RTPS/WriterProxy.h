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
#include "HistoryCache.h"

AGIDDS_NAMESPACE_BEGIN

class WriterProxy {
public:
	WriterProxy(GUID_t guid, LocatorSeq& unicastLocatorList, LocatorSeq& multicastLocatorList);

	SequenceNumber_t AvailableChangesMax();

	void IrrelevantChangeSet(SequenceNumber_t seqNum);

	void LostChangesUpdate(SequenceNumber_t firstAvailableSeqNum);

	SequenceNumber_t MissingChanges();

	void MissingChangesUpdate(SequenceNumber_t lastAvailableSeqNum);

	void ReceivedChangeSet(SequenceNumber_t seqNum);

	GUID_t GetGuid();
private:
	GUID_t remoteWriterGuid_;
	LocatorSeq unicastLocatorList_;
	LocatorSeq multicastLocatorList_;
	CacheChange* changes_from_writer;
};

using WriterProxyMap = map<GUID_t, WriterProxy*>;

AGIDDS_NAMESPACE_END