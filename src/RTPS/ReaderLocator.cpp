#include "ReaderLocator.h"
USING_AGIDDS_NAMESPACE

ReaderLocator::ReaderLocator(Locator_t locator)
{
	locator_ = locator;
	highestSentChangeSN_ = SEQUENCENUMBER_UNKNOWN;
}

SequenceNumber_t ReaderLocator::NextRequestedChange()
{
	if (0 > requestedChanges_.size()) {
		return requestedChanges_[0];
	}
	return SEQUENCENUMBER_INVALID;
}

SequenceNumber_t ReaderLocator::next_unsent_change(SequenceNumber_t seq_num_max)
{
	if (seq_num_max <= highestSentChangeSN_) {
		return SEQUENCENUMBER_INVALID;
	}

	return highestSentChangeSN_  + 1;
}

vector<SequenceNumber_t> ReaderLocator::RequestedChanges()
{
	return requestedChanges_;
}

void ReaderLocator::requested_changes_set(vector<SequenceNumber_t> req_seq_num_set)
{
	requestedChanges_ = req_seq_num_set;
}

bool ReaderLocator::ResetHighestSentChange()
{
	highestSentChangeSN_ = { 0,0 };
	return false;
}

bool ReaderLocator::sendData()
{
	
	return false;
}
