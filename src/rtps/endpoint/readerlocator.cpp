#include "rtps/endpoint/readerlocator.h"
USING_AGIDDS_NAMESPACE;

ReaderLocator::ReaderLocator(const Locator_t& locator)
	: locator_(locator),
	  expectsInlineQos_(false),
	  highestSentChangeSN_(SEQUENCENUMBER_UNKNOWN)
{
}

SequenceNumber_t ReaderLocator::NextRequestedChange()
{
	if (!requestedChanges_.empty()) {
		return requestedChanges_[0];
	}
	return SEQUENCENUMBER_INVALID;
}

SequenceNumber_t ReaderLocator::NextUnsentChange()
{
	SeqNumberVec unsentChanges;

	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();
	SequenceNumber_t maxSeq = writerCache_->GetSeqNumMax();

	for (; minSeq <= maxSeq; minSeq++) {
		std::shared_ptr<CacheChange> change = writerCache_->GetChange(minSeq);
		if (nullptr == change)
			continue;
		if (minSeq > highestSentChangeSN_)
			unsentChanges.push_back(minSeq);
	}

	if (unsentChanges.empty())
		return SEQUENCENUMBER_INVALID;

	return unsentChanges[0];
}

SeqNumberVec ReaderLocator::RequestedChanges()
{
	return requestedChanges_;
}

void ReaderLocator::RequestedChangesSet(SeqNumberVec reqSeqNumSet)
{
	requestedChanges_ = reqSeqNumSet;
}

bool ReaderLocator::UnsentChanges()
{
	return (SEQUENCENUMBER_INVALID != NextUnsentChange());
}

void ReaderLocator::SetHistoryCache(std::shared_ptr<HistoryCache> writerCache)
{
	writerCache_ = writerCache;
}

bool ReaderLocator::ResetHighestSentChange()
{
	highestSentChangeSN_ = { 0,0 };
	return true;
}

Locator_t ReaderLocator::GetLocator()
{
	return locator_;
}
//
//bool ReaderLocator::sendData()
//{
//	
//	return false;
//}
