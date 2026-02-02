#include "rtps/flowcontrol/flowcontroller.h"
#include "rtps/endpoint/participant.h"
#include "rtps/endpoint/writer.h"
#include "rtps/endpoint/statefulwriter.h"
USING_TRAVODDS_NAMESPACE;

FlowController::FlowController(Participant* participant, const FlowControllerConfig& config)
	: config_(config), participant_(participant), scheduler_(nullptr)
{
	if(config_.kind == FlowControllerKind::SYNC) {
		return;
	}
	switch (config.policy) {
	    case SchedulerPolicy::FIFO: {
			scheduler_ = std::make_unique<FlowControlFifoScheduler>();
			break;
		}
		default:
			break;
	}
}

FlowSyncController::FlowSyncController(Participant* participant, const FlowControllerConfig& config)
	: FlowController(participant, config)
{
}

void FlowController::RegisterWriter(std::shared_ptr<Writer> writer)
{
	if(scheduler_ != nullptr) {
		scheduler_->RegisterWriter(writer);	
	}
}

void FlowController::UnregisterWriter(std::shared_ptr<Writer> writer)
{
	if(scheduler_ != nullptr) {
		scheduler_->UnregisterWriter(writer);
	}
}

ReturnCode_t FlowSyncController::AddNewData(Writer* writer, std::shared_ptr<CacheChange> change, const Duration_t& timeOut)
{
	return writer->DeliveryData(change, timeOut);
}

void FlowSyncController::ReSendDataFrag(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& FragSet)
{
	writer->ReSendDataFrag(readerProxy, FragSet);
}

void FlowSyncController::ReSendData(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy)
{
	writer->ReSendData(readerProxy);
}

FlowAsyncController::FlowAsyncController(Participant* participant, const FlowControllerConfig& config)
	: FlowController(participant, config)
{
}

ReturnCode_t FlowAsyncController::AddNewData(Writer* writer, std::shared_ptr<CacheChange> change, const Duration_t& timeOut)
{
	static_cast<void>(timeOut);
	 scheduler_->AddSendData(writer, change);
	 return RETCODE_OK;
}

void FlowAsyncController::ReSendDataFrag(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& FragSet)
{
	scheduler_->ReSendDataFrag(writer, readerProxy, FragSet);
}
void FlowAsyncController::ReSendData(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy)
{
	scheduler_->ReSendData(writer, readerProxy);
}

FlowSemiSyncController::FlowSemiSyncController(Participant* participant, const FlowControllerConfig& config)
	: FlowAsyncController(participant, config)
{
}

ReturnCode_t FlowSemiSyncController::AddNewData(Writer* writer, std::shared_ptr<CacheChange> change, const Duration_t& timeOut)
{
	return writer->DeliveryData(change, timeOut);
}