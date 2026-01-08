#include "rtps/endpoint/readerlocator.h"
#include <algorithm>
USING_TRAVODDS_NAMESPACE;

ReaderLocator::ReaderLocator(const GUID_t& readerGuid, const LocatorSeq& locatorList)
	: remoteGuid_(readerGuid),
	  locatorList_(locatorList),
	  expectsInlineQos_(false),
	  highestSentChangeSN_(SEQUENCENUMBER_UNKNOWN)
{
	std::sort(locatorList_.begin(), locatorList_.end(),
		[](const Locator_t& a, const Locator_t& b) {
			return uint32_t(a.kind - 5) < uint32_t(b.kind - 5);
		});
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

void ReaderLocator::LocatorAdd(Locator_t& locator)
{
	for (auto& loc : locatorList_) {
		if (loc == locator) {
			return;
		}
    }
	locatorList_.push_back(locator);
	std::sort(locatorList_.begin(), locatorList_.end(),
		[](const Locator_t& a, const Locator_t& b) {
			return uint32_t(a.kind - 5) < uint32_t(b.kind - 5);
		});
}

void ReaderLocator::LocatorRemove(Locator_t& locator)
{
	for (auto it = locatorList_.begin(); it != locatorList_.end(); ++it) {
		if (*it == locator) {
			locatorList_.erase(it);
			return;
		}
	}
}

LocatorSeq ReaderLocator::GetLocatorList()
{
	return locatorList_;
}
const GUID_t& ReaderLocator::GetRemoteGuid()
{
	return remoteGuid_;
}
//
//bool ReaderLocator::sendData()
//{
//	
//	return false;
//}
