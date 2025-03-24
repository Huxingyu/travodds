#include "RTPS/Writer.h"

USING_AGIDDS_NAMESPACE

Writer::Writer(DataWriterQos& qos) :qos_(qos)
{
	nackResponseDelay_.sec = 0;
	nackResponseDelay_.nanosec = 200 * 1000 * 1000;
	nackSuppressionDuration_.sec = 0;
	nackSuppressionDuration_.nanosec = 0;
	lastChangeSequenceNumber_ = { 0,0 };
}


CacheChange* Writer::NewChange(ChangeKind_t kind, SerializedBuffer* data, InstanceHandle_t handle)
{
	lastChangeSequenceNumber_++;

	CacheChange* cacheChange = new CacheChange;
	cacheChange->kind = kind;
	cacheChange->serializedBuffer = data;
	cacheChange->instanceHandle = handle;
	cacheChange->sequenceNumber = lastChangeSequenceNumber_;
	cacheChange->writerGuid = guid_;

	return cacheChange;

}

ReturnCode_t Writer::AddNewCachce(CacheChange* cacheChange)
{
	ReturnCode_t ret = RETCODE_OK;
	if (nullptr == cacheChange) {
		return RETCODE_ERROR;
	}

	if (true == writerCache_->IsFull()) {
		SequenceNumber_t seq = writerCache_->GetSeqNumMin();

		if (KEEP_LAST_HISTORY_QOS == qos_.history.kind) {
			writerCache_->RemoveChange(writerCache_->GetChange(seq));
		}

		//TODO 添加时间待定机制
		if (KEEP_ALL_HISTORY_QOS == qos_.history.kind) {
			if (true == IsAckedByAll(seq)) {
				writerCache_->RemoveChange(writerCache_->GetChange(seq));
			} else {
				return RETCODE_OUT_OF_RESOURCES;
			}
		}
	}

	writerCache_->AddChange(cacheChange);


	ret = DeliveryData(cacheChange);

	if (BEST_EFFORT_RELIABILITY_QOS == qos_.reliability.kind) {
		writerCache_->RemoveChange(cacheChange);
	}

	return RETCODE_OK;

}

bool Writer::WaitForAcknowledgments(const Duration_t & max_wait)
{
	return true;
}

bool Writer::IsAckedByAll(SequenceNumber_t changeSeqNum)
{
	return true;
}

bool Writer::IsPushMode()
{
	return pushMode_;
}

HistoryCache * Writer::GetWriteCache()
{
	return writerCache_;
}

bool Writer::DataFilter(CacheChange * change)
{
	return true;
}

ReturnCode_t Writer::DeliveryData(CacheChange * change)
{
	return ReturnCode_t();
}
