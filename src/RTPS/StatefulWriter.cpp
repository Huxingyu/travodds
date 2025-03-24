#include "StatefulWriter.h"
USING_AGIDDS_NAMESPACE

void StatefulWriter::MatchedReaderAdd(ReaderProxy * readerProxy)
{
	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();
	SequenceNumber_t manSeq = writerCache_->GetSeqNumMax();

	for (; minSeq <= manSeq; minSeq++) {
		CacheChange* change = writerCache_->GetChange(minSeq);
		ChangeForReader changeForReader = CreateChangeForReader(change);
		readerProxy->AddChange(changeForReader);
	}

	matchedReaders_[readerProxy->GetGuid()] = readerProxy;
}

void StatefulWriter::MatchedReaderRemove(ReaderProxy* readerProxy)
{
	matchedReaders_.erase(readerProxy->GetGuid());
	delete readerProxy;
}

ReaderProxy * StatefulWriter::MatchedReaderLookup(GUID_t readerGuid)
{
	return matchedReaders_[readerGuid];
}

bool StatefulWriter::WaitForAcknowledgments(const Duration_t & max_wait)
{
	SequenceNumber_t sequenceNumber = writerCache_->GetSeqNumMax();
	//TODO 超时机制待实现
	for (auto reader : matchedReaders_) {
		if (false == reader.second->IsAcked(sequenceNumber)) {
			return false;
		}
	}
	return true;
}

bool StatefulWriter::IsAckedByAll(SequenceNumber_t changeSeqNum)
{
	for (auto readerProxy : matchedReaders_) {
		if (false == readerProxy.second->IsAcked(changeSeqNum)) {
			return false;
		}
	}
	return true;
}

ReturnCode_t StatefulWriter::DeliveryData(CacheChange * change)
{
	if (nullptr == change || 0 == matchedReaders_.size()) {
		return RETCODE_OK;
	}

	ChangeForReader changeForReader = CreateChangeForReader(change);

	for (auto reader : matchedReaders_) {
		reader.second->AddChange(changeForReader);
	}

	//TODO 封装报文

	//TODO 发送报文

	return ReturnCode_t();
}

ChangeForReader StatefulWriter::CreateChangeForReader(CacheChange * change)
{
	ChangeForReader changeForReader;
	changeForReader.sequenceNumber = change->sequenceNumber;

	if (true == IsPushMode()) {
		changeForReader.status = UNSENT;
	} else {
		changeForReader.status = UNACKNOWLEDGED;
	}

	if (true == DataFilter(change)) {
		changeForReader.isRelevant = false;
	} else {
		changeForReader.isRelevant = true;
	}

	return changeForReader;
}
