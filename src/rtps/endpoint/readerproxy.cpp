#include  "rtps/endpoint/readerproxy.h"
#include  "rtps/endpoint/writer.h"
#include  <algorithm>
USING_TRAVODDS_NAMESPACE

ReaderProxy::ReaderProxy(const GUID_t& readerGuid, const GUID_t groupGuid,
	const LocatorSeq& unicastLocatorList, const LocatorSeq& multicastLocatorList, bool active)
	: remoteReaderGuid_(readerGuid),
	remoteGroupGuid_(groupGuid),
	unicastLocatorList_(unicastLocatorList),
	multicastLocatorList_(multicastLocatorList),
	highestSentChangeSN_(SEQUENCENUMBER_INVALID),
	expectsInlineQos_(false),
	isActive_(active),
	writerCache_(nullptr),
	ackCount_(0),
	hbSendCount_(1),
	ackedSeq_(SEQUENCENUMBER_ZERO),
	relatedSeqnum_(SEQUENCENUMBER_ZERO)
{
}

void ReaderProxy::AckedChangesSet(SequenceNumber_t committedSeqNum)
{
	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();
	SequenceNumber_t maxSeq = writerCache_->GetSeqNumMax();
	
	{
		std::lock_guard<std::mutex> lock(ackmtex_);
		acknowledgeChanges_.clear();
	}

	for (; minSeq <= maxSeq; minSeq++) {
		std::shared_ptr<CacheChange> change = writerCache_->GetChange(minSeq);
		if (nullptr == change) {
			continue;
		}	
		if (minSeq <= committedSeqNum) {
			std::lock_guard<std::mutex> lock(ackmtex_);
			acknowledgeChanges_.push_back(minSeq);
			
		}		
	}
	
}

SequenceNumber_t ReaderProxy::NextRequestedChange()
{
	if (requestedChanges_.size() > 0) {
		return *requestedChanges_.begin();
	}
		
	return SEQUENCENUMBER_UNKNOWN;
}

SequenceNumber_t ReaderProxy::NextUnsentChange()
{
	SeqNumberVec unsentChanges;

	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();
	SequenceNumber_t maxSeq = writerCache_->GetSeqNumMax();

	for (; minSeq <= maxSeq; minSeq++) {
		std::shared_ptr<CacheChange> change = writerCache_->GetChange(minSeq);
		if (nullptr == change) {
			continue;
		}
		if (minSeq > highestSentChangeSN_) {
			unsentChanges.push_back(minSeq);
		}
			
	}

	if (unsentChanges.empty()) {
		return SEQUENCENUMBER_INVALID;
	}
		
	return unsentChanges[0];
}

bool ReaderProxy::UnsentChanges()
{
	return (SEQUENCENUMBER_INVALID != NextUnsentChange());
}

SeqNumberVec ReaderProxy::RequestedChanges()
{
	return requestedChanges_;
}

void ReaderProxy::RequestedChangesSet(SeqNumberVec requested_changes_set)
{
	requestedChanges_ = requested_changes_set;
}

bool ReaderProxy::UnackedChanges()
{
	SequenceNumber_t highestAvailableSeqNum = SEQUENCENUMBER_ZERO, highestAckedSeqNum = ackedSeq_;
	highestAvailableSeqNum = writerCache_->GetSeqNumMax();
	{
		std::lock_guard<std::mutex> lock(ackmtex_);
		if (!acknowledgeChanges_.empty()) {
			highestAckedSeqNum = acknowledgeChanges_.back();
		}
	}

	return (highestAvailableSeqNum > highestAckedSeqNum);
}

GUID_t ReaderProxy::GetGuid()
{
	return remoteReaderGuid_;
}

bool ReaderProxy::IsAcked(SequenceNumber_t number)
{
	//statelessreader 默认全都acked
	if (false == isActive_)
		return true;

	if (ackedSeq_ > number) {
		return true;
	}

	std::lock_guard<std::mutex> lock(ackmtex_);
	auto it = std::find(acknowledgeChanges_.begin(), acknowledgeChanges_.end(), number);
	if (it != acknowledgeChanges_.end()) {
		return true;
	}
	
	return false;

}

bool ReaderProxy::IsAllAcked(const SequenceNumber_t& maxNumber) const
{
	//statelessreader 默认全都acked
	if (false == isActive_)
		return true;

	if (ackedSeq_ >= maxNumber) {
		return true;
	}
	return false;
}

void ReaderProxy::SetHistoryCache(std::shared_ptr<HistoryCache> writerCache)
{
	writerCache_ = writerCache;
}
bool ReaderProxy::AddChange(ChangeForReader change)
{
	//changesForReaders_.push_back(change);
	return false;
}

ReturnCode_t ReaderProxy::RecvAcknack(const AcknackSubMessage_t& ackMsg)
{
	//TODO 
	ReturnCode_t ret = RETCODE_OK;
	const SequenceNumberSet_t& seqSet = ackMsg.readerSNState;
	auto seq = seqSet.bitmapBase;

	if (ackMsg.count <= ackCount_.load()) {
		return RETCODE_RECEIVE_DUPLICATE;
	}
	ackCount_ = ackMsg.count;

	//if (ackMsg.readerSNState.bitmapBase < relatedSeqnum_) {
	//	return RETCODE_RECEIVE_DUPLICATE;
	//}
	ackedSeq_ = ackMsg.readerSNState.bitmapBase - 1;
	AckedChangesSet(ackedSeq_);
	RemoveFragNackCounts(ackedSeq_);
	SeqNumberVec seqVec;
	for (uint32_t i = 0; i < seqSet.bitmap.size; i++) {
		if (seqSet.bitmap.IsBitSet(i)) {
			seq = seqSet.bitmapBase + i;
			seqVec.push_back(seq);
			ret = RETCODE_REPEAT_REQUEXT;
		} else {
			seq = seqSet.bitmapBase + i;
			AckedChangeSet(seq);
		}
	}

	RequestedChangesSet(seqVec);

	return ret;
}

ReturnCode_t ReaderProxy::RecvNackFrag(const NackFragSubMessage_t& nackMsg)
{
	ReturnCode_t ret = RETCODE_OK;

	/**/

	const FragMentNumberSet_t& fragSet = nackMsg.fragmentNumberState;
	auto itor = nackCounts_.find(nackMsg.writerSN);
	if (itor == nackCounts_.end()) {
		nackCounts_[nackMsg.writerSN] = nackMsg.count;
	}
	else if(itor->second >= nackMsg.count){
		/* 接收异常  */
		return RETCODE_RECEIVE_DUPLICATE;
	}
	else {
		itor->second = nackMsg.count;
	}

	return ret;
}

LocatorSeq& ReaderProxy::GetUnicastLocatorList()
{
	return unicastLocatorList_;
}

LocatorSeq& ReaderProxy::GetMulticastLocatorList()
{
	return multicastLocatorList_;
}

bool ReaderProxy::isActive()
{
	return isActive_;
}

int32_t ReaderProxy::HeaertSendCountAdd()
{
	int32_t count = hbSendCount_.load();

	hbSendCount_++;

	return count;
}

void ReaderProxy::AckedChangeSet(SequenceNumber_t committedSeqNum)
{
	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();
	SequenceNumber_t maxSeq = writerCache_->GetSeqNumMax();

	for (; minSeq <= maxSeq; minSeq++) {
		std::shared_ptr<CacheChange> change = writerCache_->GetChange(minSeq);
		if (nullptr == change) {
			continue;
		}

		if (minSeq == committedSeqNum) {
			std::lock_guard<std::mutex> lock(ackmtex_);
			acknowledgeChanges_.push_back(minSeq);
		}
	}
}

void ReaderProxy::InitChangeForReader(SequenceNumber_t relatedSeq)
{
	relatedSeqnum_ = relatedSeq;
}

SequenceNumber_t ReaderProxy::GetRelatedSequenceNumber()
{
	return relatedSeqnum_;
}

void ReaderProxy::RemoveFragNackCounts(const SequenceNumber_t& ackSeq)
{
	for (auto count = nackCounts_.begin(); count != nackCounts_.end(); ) {

		if (count->first <= ackSeq) {
			count = nackCounts_.erase(count);
		}
		else {
			break;
		}
	}
}

bool ReaderProxy::IsNackFrag(const SequenceNumber_t& ackSeq)
{
	auto itor = nackCounts_.find(ackSeq);
	if (itor == nackCounts_.end()) {
		return false;
	}
	/* 接收到的nack的序号 < ack的序号 */
	if (itor->second < ackCount_.load()) {
		return false;
	}

	return true;
}