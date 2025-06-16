#include "rtps/endpoint/writer.h"
#include "rtps/endpoint/participant.h"
#include "common/log/logger.h"
#include <chrono>
USING_AGIDDS_NAMESPACE;

Writer::Writer(const DataWriterQos& qos, const TopicKind_t& kind)
	: qos_(qos),
	  nackResponseDelay_({ 0, 200u * 1000u * 1000u }),
	  nackSuppressionDuration_(TIME_ZERO),
	  lastChangeSequenceNumber_(TIME_ZERO),
	  heartBeatPeriod_(TIME_ZERO),
	  writerCache_(new HistoryCache(qos.history, qos.resource_limits, kind))
{
}

Writer::~Writer()
{
}


std::shared_ptr<CacheChange> Writer::NewChange(ChangeKind_t kind, SerializedBuffer* data, InstanceHandle_t handle,const Time_t& timespace)
{
	lastChangeSequenceNumber_++;

	std::shared_ptr<CacheChange> cacheChange = std::make_shared<CacheChange>();
	cacheChange->kind = kind;
	cacheChange->serializedBuffer = data;
	cacheChange->instanceHandle = handle;
	cacheChange->sequenceNumber = lastChangeSequenceNumber_;
	cacheChange->writerGuid = guid_;
	cacheChange->sourceTimespace = timespace;

	return cacheChange;

}

/*******************************
添加cache需要考虑的QoS有ResourceLimitsQosPolicy和HistoryQosPolicy

ResourceLimitsQosPolicy
1. 添加的cache数据的instance对应的数字是否满足 resource_limit.max_sample_per_instance;
2. 添加的cache数据是否超过resource_limit.max_instance的设置
3. 添加的cache数据是否超过qos_.resource_limits.max_samples，或者history.depth的设置;
HistoryQosPolicy
1.keep_last  保存最新的数据
2.keep_all   保存所有的数据

//TODO 内置主题是否满足下方逻辑
// 测试rti，测试fast
1.keep_last的情况下,
	1.1 instance的个数 > max_instance的在上一层发送数据失败
	1.2 对应的instance的个数 > depth， 删除最旧的数据
	// 1.3 样本总数 > max_sample，删除最旧的数据。 max_sample = max_sample_per_instance * max_instance
	//  不满足1.1 、 1.2的条件，必然样本总数必然满足: 样本总数 < max_sample
2.keep_all的情况下
	2.1 不满足max_instance的在上一层发送数据失败
	2.2 不满足max_sample_per_instance,
			RELIABLE模式进行等待，等待时间到达删除数据
			BEST_EFFORT模式删除最旧的数据
	// 2.3 不满足max_sample，进行等待等待时间到达返回失败。 max_sample = max_sample_per_instance * max_instance
	// 不满足2.1 、 2.2的条件, 必然样本总数必然满足:样本总数 < max_sample
******************************/
ReturnCode_t Writer::AddNewCachce(std::shared_ptr<CacheChange> cacheChange)
{
	ReturnCode_t ret = RETCODE_OK;

	if (nullptr == cacheChange) {
		return RETCODE_ERROR;
	}

	if (writerCache_->IsFull(cacheChange->instanceHandle)) {
		if (KEEP_LAST_HISTORY_QOS == qos_.history.kind || BEST_EFFORT_RELIABILITY_QOS == qos_.reliability.kind) {
			/*  满了，删除最旧的数据  */
			writerCache_->RemoveOldestChangeByInstance(cacheChange->instanceHandle);
		}
		else {
			std::chrono::nanoseconds time_out = std::chrono::seconds(qos_.reliability.max_blocking_time.sec) + std::chrono::nanoseconds(qos_.reliability.max_blocking_time.nanosec);
			bool ret0 = writerCache_->WriterWaitFor(time_out, this, cacheChange->instanceHandle, TRANSIENT_LOCAL_DURABILITY_QOS == qos_.durability.kind);

			if (!ret0) {
				AGILOG(AGILOG_ERROR, "WriterWaitFor timeout");
				return RETCODE_TIMEOUT;
			}
		}
	}

	writerCache_->AddChange(cacheChange);

	ret = DeliveryData(cacheChange);

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

std::shared_ptr<HistoryCache> Writer::GetWriteCache()
{
	return writerCache_;
}

bool Writer::DataFilter(std::shared_ptr<CacheChange> change)
{
	return true;
}

ReturnCode_t Writer::DeliveryData(std::shared_ptr<CacheChange> change)
{
	return RETCODE_UNSUPPORTED;
}

void Writer::SetRelatedDataWriter(DataWriterImpl*  datawriter)
{
	datawriter_ = datawriter;
}
DataWriterImpl*  Writer::GetRelatedDataWriter()
{
	return datawriter_;
}

DataWriterQos & Writer::GetQos()
{
	return qos_;
}

ReturnCode_t Writer::AddBuiltinCache(std::shared_ptr<CacheChange> cacheChange)
{
	ReturnCode_t ret = RETCODE_ERROR;
	/* 1.添加key */
	if (!writerCache_->IsKeyRegistered(cacheChange->instanceHandle)) {
		SerializedBuffer* buffer = nullptr;
		ret = writerCache_->AddKeyHistory(cacheChange->instanceHandle, buffer);
		if (RETCODE_OK != ret) {
			AGILOG(AGILOG_ERROR, "AddKeyHistory Err ret = %d", ret);
			return ret;
		}
	}

	ret = AddNewCachce(cacheChange);

	return ret;
}

std::shared_ptr<HistoryCache> Writer::GetHistoryCache()
{
	return writerCache_;
}

void Writer::RemoveCacheByHandle(const InstanceHandle_t& handle)
{
	writerCache_->RemoveChangeByInstance(handle);
}

void Writer::GetMatchedSubscriptions(InstanceHandleSeq& subHandles) const
{
	participant_->GetMatchedEntitys(guid_.entityId, subHandles);
}

