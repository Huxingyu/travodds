#include "StatelessWriter.h"
USING_AGIDDS_NAMESPACE

StatelessWriter::StatelessWriter(DataWriterQos& qos, Duration_t resendPeriod):
Writer(qos),resendDataPeriod_(resendPeriod)
{
}

void StatelessWriter::ReaderLocatorAdd(Locator_t & locator)
{
	if (0 != readerLocators_.count(locator)) {
		return;
	}

	ReaderLocator* readerLocator = new ReaderLocator(locator);
	readerLocators_[locator] = readerLocator;
}

void StatelessWriter::ReaderLocatorRemove(Locator_t & locator)
{
	if (0 != readerLocators_.count(locator)) {
		return;
	}

	delete  readerLocators_[locator];
	readerLocators_[locator] = nullptr;

	readerLocators_.erase(locator);
}

void StatelessWriter::UnsentChangesReset()
{
	for (auto itor : readerLocators_) {
		itor.second->ResetHighestSentChange();
	}
}

ReturnCode_t StatelessWriter::DeliveryData(CacheChange * change) {

	if (0 == readerLocators_.size()) {
		return RETCODE_OK;
	}
	//TODO 封装报文

	//TODO 发送报文

	return ReturnCode_t();
}

void StatelessWriter::ResendDataPeriod()
{
	SequenceNumber_t minSeq = writerCache_->GetSeqNumMin();
	SequenceNumber_t manSeq = writerCache_->GetSeqNumMax();

	for (; minSeq <= manSeq; minSeq++) {
		CacheChange* change = writerCache_->GetChange(minSeq);

		DeliveryData(change);
	}
}
