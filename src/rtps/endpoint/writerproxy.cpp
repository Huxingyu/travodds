#include "rtps/endpoint/writerproxy.h"
#include "common/log/logger.h"
#include  <algorithm>
USING_TRAVODDS_NAMESPACE;

WriterProxy::WriterProxy(const GUID_t& writerGuid, const GUID_t& groupGuid, 
	const LocatorSeq& unicastLocatorList, const LocatorSeq& multicastLocatorList)
	: remoteWriterGuid_(writerGuid),
      remoteGroupGuid_(groupGuid),
	  unicastLocatorList_(unicastLocatorList),
	  multicastLocatorList_(multicastLocatorList),
	  dataMaxSizeSerialized_(0),
	  submitSeq_(SEQUENCENUMBER_ZERO),
	  insertSeq_(SEQUENCENUMBER_ZERO),
	  changes_from_writer(nullptr),
	  hbCount_(0),
	  hbFragCount_(0),
	  ackSendCount_(0),
	  nackSendCount_(0)
{
}

GUID_t WriterProxy::GetGuid()
{
	return remoteWriterGuid_;
}

void WriterProxy::Init(SequenceNumber_t first, SequenceNumber_t last)
{
	static_cast<void>(first);
	static_cast<void>(last);
}

SequenceNumber_t WriterProxy::AvailableChangesMax()
{
	SequenceNumber_t seq = SEQUENCENUMBER_ZERO;

	auto change = changesFromWriters_.begin();
	if (change == changesFromWriters_.end()) {
		return seq;
	}

	if (change->second.status == RECEIVED || (change->second.status & NOT_AVAILABLE)) {
		seq = change->second.sequenceNumber;
	}
	return seq;
}

void WriterProxy::NonAvailableChangeSet(SeqNumberVec seqNumSeq, ChangeCount_t filteredCount)
{
	unsigned long changeCount = filteredCount.low + (filteredCount.high < 32);
	
	if (seqNumSeq.size() <= 0) {
		return;
	}
	SequenceNumber_t seq = seqNumSeq[seqNumSeq.size() - 1];

	std::lock_guard<std::mutex> lock(changesmtx_);
	if (insertSeq_ < seq) {
		for (insertSeq_ = seqNumSeq[0]; insertSeq_ <= seq; insertSeq_++) {
			ChangeFromWriter changeInfo;
			changeInfo.status = UNKNOWN;
			changeInfo.is_relevant = true;
			changeInfo.sequenceNumber = insertSeq_;
			changeInfo.cache = nullptr;

			changesFromWriters_[insertSeq_] = changeInfo;
		}
		insertSeq_ = insertSeq_ - 1; 
	}

	for (auto& change : changesFromWriters_)
	{
		if(!std::count(seqNumSeq.begin(), seqNumSeq.end(), change.second.sequenceNumber))
			continue;

		if (seqNumSeq.size() == changeCount) {
			change.second.status = NA_FILTERED;
		}
		else if (0 == changeCount)
			change.second.status = NA_REMOVED;
		else
			change.second.status = NA_UNSPECIFIED;
	}
	
}

void WriterProxy::LostChangesUpdate(SequenceNumber_t firstAvailableSeqNum, bool changesRemoved)
{
	for (auto& change : changesFromWriters_)	{

		if ((UNKNOWN == change.second.status || MISSING == change.second.status) && change.second.sequenceNumber < firstAvailableSeqNum) {
			
			if (changesRemoved) {
				change.second.status = NA_REMOVED;
			}
			else {
				change.second.status = NA_UNSPECIFIED;
			}			
		}
	}
}

SeqNumberVec WriterProxy::MissingChanges()
{
	SeqNumberVec missSeqNumbers;
	for (auto& change : changesFromWriters_) {

		if (MISSING == change.second.status){
			missSeqNumbers.push_back(change.second.sequenceNumber);
		}
	}
	return missSeqNumbers;
}

void WriterProxy::MissingChangesUpdate(SequenceNumber_t lastAvailableSeqNum)
{
	for (auto change = changesFromWriters_.begin(); change != changesFromWriters_.end(); change++) {
		if (UNKNOWN == change->second.status && change->second.sequenceNumber <= lastAvailableSeqNum) {
			change->second.status = MISSING;
		}
	}
}

ReturnCode_t WriterProxy::RecvHeartbeat(const HeartBeatSubMessage_t& hbMsg, int32_t &lostCount, SequenceNumberSet_t& set)
{
	//初始化一下变量
	lostCount = 0;
	set.bitmapBase = hbMsg.lastSN + 1;

	if (hbCount_.load() >=  hbMsg.count) {
		//AGILOG(AGILOG_ERROR, "hbCount_[%d] > hbMsg.count[%d]", hbCount_.load(), hbMsg.count);
		return RETCODE_RECEIVE_DUPLICATE;
	}
	hbCount_ = hbMsg.count;

	if (hbMsg.lastSN <= submitSeq_) {
		// 所有数据正常提交 
		return RETCODE_OK;
	}
	
	/* 如果插入的ChangeFromWriter的序号 小于 HB的最后一个值，开始发送数据 */
	if (insertSeq_ < hbMsg.lastSN) {
		/* 如果insertSeq_ <  hbMsg.firstSN,  说明小于 hbMsg.firseSN的数据都被提交 */
		insertSeq_ = (insertSeq_ < hbMsg.firstSN) ? hbMsg.firstSN : insertSeq_ + 1;
		/* 之前没有添加过，循环添加缺失数据 */
		for (; insertSeq_ <= hbMsg.lastSN; insertSeq_++) {
			ChangeFromWriter changeInfo;
			changeInfo.status = UNKNOWN;
			changeInfo.is_relevant = true;
			changeInfo.sequenceNumber = insertSeq_;
			changeInfo.cache = nullptr;

			changesFromWriters_[insertSeq_] = changeInfo;
		}
		insertSeq_ = insertSeq_ - 1;
	}

	std::lock_guard<std::mutex> lock(changesmtx_);
	if (hbMsg.firstSN > submitSeq_ + 1) {

		if (submitSeq_ != SEQUENCENUMBER_ZERO) {
			LostChangesUpdate(hbMsg.firstSN, true);
			lostCount = static_cast<int32_t>(LostChanges().size());
		}
		SequenceNumber_t seq = AvailableChangesMax();
		submitSeq_ = seq == SEQUENCENUMBER_ZERO ? hbMsg.firstSN : seq;
		submitSeq_ = submitSeq_ - 1;
	}
	if (submitSeq_ < hbMsg.lastSN) {
		MissingChangesUpdate(hbMsg.lastSN);
	}
	GetAckSequenceSet(set);

	return RETCODE_OK;
}
ReturnCode_t WriterProxy::RecvHeartbeatFrag(const HeartBeatFragSubMessage_t& hbFragMsg, FragMentNumberSet_t& set)
{
	//TODO 
	//TODO
	/*  目前没有使用到DataReader        */


	if (hbFragCount_.load() >= hbFragMsg.count) {
		TRAVODDS_LOG(LOG_ERROR, "hbFragCount_[%d] > hbFragCount_.count[%d]", hbFragCount_.load(), hbFragMsg.count);
		return RETCODE_RECEIVE_DUPLICATE;
	}
	hbCount_ = hbFragMsg.count;

	std::shared_ptr<CacheChange> cache = nullptr;
	ReturnCode_t ret = RETCODE_OK;

	std::map<SequenceNumber_t, ChangeFromWriter>::iterator changeInfo;

	{
		std::lock_guard<std::mutex> lock(changesmtx_);
		changeInfo = changesFromWriters_.find(hbFragMsg.writerSN);
		if (changesFromWriters_.end() == changeInfo) {
			return ret;
		}
		cache = changeInfo->second.cache;
		if (!cache) {
			return ret;
		}
	}

	SerializedBuffer* buffer = cache->serializedBuffer;
	set.bitmapBase = 0;
	/* 找到起始丢失序号 */
	for (uint32_t i = 1; i <= hbFragMsg.lastFragmentNum; i++) {
		if (nullptr == buffer) {
			break;
		}
		if (!buffer->buffer) {
			/* 没有收到数据 */  
			set.bitmapBase = i;
			break;
		}
		buffer = buffer->next;
	}
	/* 从起始开始序号，继续遍历 */
	for (uint32_t i = 1; i <= hbFragMsg.lastFragmentNum - set.bitmapBase; i++) {
		if (nullptr == buffer) {
			ret = RETCODE_ERROR;
			break;
		}
		if (i == 256) {	
			break;
		}
		if (!buffer->buffer) {
			ret = RETCODE_OK;
			/* 没有收到数据 */
			//TODO 
			set.bitmap.SetBit(i);
			set.bitmap.size = i;
			break;
		}
		buffer = buffer->next;
	}
	return ret;
}


ReturnCode_t WriterProxy::AddReliableDataCache(std::shared_ptr<CacheChange> cache, const DurabilityQosPolicyKind& durabilityKind, bool notFrag)
{
	if (nullptr == cache) {
		TRAVODDS_LOG(LOG_ERROR, "cache is nullptr");
		return RETCODE_ERROR;
	}

	/* TODO wangyi 接收Liveliness 状态报文时存在changesFromWriters_为空，导致报文被丢弃，数据超时 */
	/* 如果之前添加的数据，修改之前数据的状态  */
	if (cache->sequenceNumber <= insertSeq_) {
		std::lock_guard<std::mutex> lock(changesmtx_);
		auto changeInfo = changesFromWriters_.find(cache->sequenceNumber);
		if (changesFromWriters_.end() != changeInfo) {
			/* 存在数据，先删除在添加 (未提交数据) */
			changeInfo->second.cache = cache;
			if ((MISSING == changeInfo->second.status || UNKNOWN == changeInfo->second.status)&& (0 == changeInfo->second.cache->fragmentSeq.size())) {
				changeInfo->second.status = RECEIVED;
			}	
		} 
		else {
			//TODO 此处的数据应该是，已经提交过后，或者通过HB报文删除	
			return RETCODE_ERROR;
		}
	} 
	else {
		ChangeFromWriter changeInfo;
		changeInfo.status = UNKNOWN;
		changeInfo.is_relevant = true;
		changeInfo.cache = nullptr;
		
		std::lock_guard<std::mutex> lock(changesmtx_);
		if (durabilityKind == VOLATILE_DURABILITY_QOS && insertSeq_ == SEQUENCENUMBER_ZERO) {
			insertSeq_ = cache->sequenceNumber;
			changeInfo.sequenceNumber = insertSeq_;
			changesFromWriters_[insertSeq_] = changeInfo;

		} else {
		    /* 之前没有添加过，循环添加缺失数据 */
		    for (insertSeq_++; insertSeq_ <= cache->sequenceNumber; insertSeq_++) {
				changeInfo.sequenceNumber = insertSeq_;
			    changesFromWriters_[insertSeq_] = changeInfo;
		    }
		    insertSeq_ = insertSeq_ - 1;
		}

		changesFromWriters_[insertSeq_].cache = cache;
		if (notFrag) {
			changesFromWriters_[insertSeq_].status = RECEIVED;
		}
		else {
			changesFromWriters_[insertSeq_].status = MISSING;
		}
	}
	return RETCODE_OK;
}

std::shared_ptr<CacheChange> WriterProxy::GetSubmitCache()
{
	std::shared_ptr<CacheChange> cache = nullptr;

	std::lock_guard<std::mutex> lock(changesmtx_);
	SequenceNumber_t seq = AvailableChangesMax();

	while (seq != SEQUENCENUMBER_ZERO && seq == submitSeq_ + 1) {

		auto changeInfo = changesFromWriters_.find(seq);
		/* 不是容器中第一个,不进行提交  */
		if (changeInfo == changesFromWriters_.end()) {
			break;
		}

		cache = changeInfo->second.cache;
		changeInfo->second.cache = nullptr;
		submitSeq_ = seq;
	
		if (changeInfo->second.status == RECEIVED) {
			changesFromWriters_.erase(changeInfo);
			break;
		}
		else {
			// 为NOT_AVAILABLE的的数据 继续读取下一个数据 
			changesFromWriters_.erase(changeInfo);
			seq = AvailableChangesMax();
		}
	}

	return cache;
}

bool WriterProxy::ExistCache(const SequenceNumber_t& seq)
{
	bool ret = false;
	std::lock_guard<std::mutex> lock(changesmtx_);
	auto change = changesFromWriters_.find(seq);
	if (changesFromWriters_.end() != change ) {
		if (change->second.cache) {
			ret = true;
		}
	}

	return ret;
}

ReturnCode_t WriterProxy::AddFragData(const SequenceNumber_t& seq, const SerializedPayload& data, int32_t startNum, int32_t num)
{
	std::shared_ptr<CacheChange> cache = nullptr;
	ReturnCode_t ret = RETCODE_ERROR;
	
	std::map <SequenceNumber_t, ChangeFromWriter>::iterator changeInfo;
	{
		std::lock_guard<std::mutex> lock(changesmtx_);
		changeInfo = changesFromWriters_.find(seq);
		if (changesFromWriters_.end() == changeInfo) {
			//TODO 
			//AGILOG(AGILOG_ERROR, "seqdata not exist %d",seq.low);
			return ret;
		}

		cache = changeInfo->second.cache;
		if (!cache) {
			TRAVODDS_LOG(LOG_ERROR, "cache is nullptr", seq.low);
			return ret;
		}
	}

	/* 判断当前分片数据是否收到，没有收到进行封装  */
	if (cache->fragmentSeq.erase(startNum) > 0) {
		CacheSerializedBuffer* buffer = new CacheSerializedBuffer();
		buffer->writer_position = data.length;
		buffer->buffer_size = data.length;
		buffer->buffer = data.data;
		buffer->originMessage = data.originMessage;
		cache->fragmentBufferMap.insert({ startNum, buffer });
	} else {
		//TODO  
		/* 重复接收分片数据 */
	    //AGILOG(AGILOG_ERROR, "recv repeat dataFrg frag=%d ", num );
	}

	// yuguxu 移除其他包含的所有分片号
	for (int32_t i = startNum + 1; i < startNum + num; i++) {
		cache->fragmentSeq.erase(i);
	}

	if (0 == cache->fragmentSeq.size()) {
		for (auto fragItor = cache->fragmentBufferMap.begin(); fragItor != std::prev(cache->fragmentBufferMap.end()); fragItor++) {
			fragItor->second->next = std::next(fragItor)->second;
		}
		cache->serializedBuffer = cache->fragmentBufferMap.begin()->second;
		cache->fragmentBufferMap.clear();
		std::lock_guard<std::mutex> lock(changesmtx_);
		changeInfo = changesFromWriters_.find(seq);
		if (changesFromWriters_.end() == changeInfo) {
			//TODO 
			//AGILOG(AGILOG_ERROR, "seqdata not exist %d",seq.low);
			return ret;
		}
		changeInfo->second.status = RECEIVED;
	}

	return ret;
}

void WriterProxy::GetAckSequenceSet(SequenceNumberSet_t& set)
{
	SeqNumberVec vec = MissingChanges();
	if (vec.size() > 0) {
		set.bitmapBase = vec[0];
	}

	for (auto& seq : vec) {
		int32_t s = seq - set.bitmapBase;
		if (s >= 255) {
			break;
		}
		set.bitmap.size = s + 1;
		set.bitmap.SetBit(s);
	}

}

LocatorSeq WriterProxy::GetUnicastLocatorList()
{
	return unicastLocatorList_;
}

LocatorSeq WriterProxy::GetMulticastLocatorList()
{
	return multicastLocatorList_;
}


SeqNumberVec WriterProxy::LostChanges()
{
	SeqNumberVec seqVec;

	for (auto &change : changesFromWriters_) {
		if (change.second.status & NOT_AVAILABLE) {
			seqVec.push_back(change.first);
		}
	}

	return seqVec;
}

void WriterProxy::GetNackSequenceSet(std::vector<NackFragSubMessage_t>& set)
{
	std::lock_guard<std::mutex> lock(changesmtx_);
	for (auto& change : changesFromWriters_) {

		if (MISSING == change.second.status) {
			if ((nullptr != change.second.cache) && (0 != change.second.cache->dataSize)) {
				/* 为分片数据  */
				NackFragSubMessage_t nackMsg;
				int32_t s = 0;
				nackMsg.writerSN = change.second.sequenceNumber;
				nackMsg.fragmentNumberState.bitmapBase = *change.second.cache->fragmentSeq.begin();
				for (auto itor : change.second.cache->fragmentSeq) {
					s = itor - nackMsg.fragmentNumberState.bitmapBase;
					if (s >= 255) {
						break;
					}
					nackMsg.fragmentNumberState.bitmap.size = s + 1;
					nackMsg.fragmentNumberState.bitmap.SetBit(s);
				}

				set.push_back(nackMsg);
			}
		}
	}
}

bool WriterProxy::IsSubmitted(const SequenceNumber_t & seq)
{
	return seq <= submitSeq_;
}