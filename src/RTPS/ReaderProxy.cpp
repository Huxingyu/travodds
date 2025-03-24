#include  "RTPS/ReaderProxy.h"
#include  "RTPS/Writer.h"
USING_AGIDDS_NAMESPACE

ReaderProxy::ReaderProxy(GUID_t guid, LocatorSeq & unicastLocatorList, LocatorSeq & multicastLocatorList)
{
	remoteReaderGuid_ = guid;
	unicastLocatorList_ = unicastLocatorList;
	multicastLocatorList_ = multicastLocatorList;
}

void ReaderProxy::AckedChangesSet(SequenceNumber_t committedSeqNum)
{
	/* 注意 是否生效 */
	for (auto change : changesForReaders_) {
		if (change.sequenceNumber <= committedSeqNum) {
			change.status = ACKNOWLEDGED;
		}
	}
}

ChangeForReader ReaderProxy::NextRequestedChange()
{

	for (auto change : changesForReaders_) {
		if (REQUESTED == change.status) {
			return change;
		}
	}

	return ChangeForReader();
}

ChangeForReader ReaderProxy::NextUnsentChange()
{
	for (auto change : changesForReaders_) {
		if (UNSENT == change.status) {
			return change;
		}
	}

	return ChangeForReader();
}

vector<ChangeForReader> ReaderProxy::UnsentChanges()
{
	vector<ChangeForReader> changes;
	for (auto change : changesForReaders_) {
		if (UNSENT == change.status) {
			changes.push_back(change);
		}
	}

	return changes;
}

vector<ChangeForReader> ReaderProxy::RequestedChanges()
{
	vector<ChangeForReader> changes;
	for (auto change : changesForReaders_) {
		if (REQUESTED == change.status) {
			changes.push_back(change);
		}
	}

	return changes;
}

void ReaderProxy::RequestedChangesSet(vector<SequenceNumber_t> requested_changes_set)
{
	for (int i = 0; i < requested_changes_set.size(); i++) {
		for (auto change : changesForReaders_) {
			if (requested_changes_set[i] == change.sequenceNumber) {
				change.status = REQUESTED;
			}
		}
	}
}

vector<ChangeForReader> ReaderProxy::UnackedChanges()
{
	vector<ChangeForReader> changes;
	for (auto change : changesForReaders_) {
		if (UNACKNOWLEDGED == change.status) {
			changes.push_back(change);
		}
	}

	return changes;
}

GUID_t ReaderProxy::GetGuid()
{
	return remoteReaderGuid_;
}

bool ReaderProxy::IsAcked(SequenceNumber_t number)
{
	for (auto change : changesForReaders_) {
		if (number != change.sequenceNumber) {
			continue;
		}

		if (true == change.isRelevant && ACKNOWLEDGED == change.status) {
			return true;
		}
	}

	return false;
}

bool ReaderProxy::AddChange(ChangeForReader change)
{
	changesForReaders_.push_back(change);
	return true;
}


