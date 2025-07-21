#include "rtps/flowcontrol/flowcontrolscheduler.h"
#include "rtps/endpoint/writer.h"
#include "rtps/endpoint/statefulwriter.h"
#include "common/threads/threadpool.h"

USING_AGIDDS_NAMESPACE;

FlowControlScheduler::FlowControlScheduler()
	:asyncThreads_(new ThreadPool(1))
{
}

FlowControlScheduler::~FlowControlScheduler()
{
	asyncThreads_->shutdown();
}
void FlowControlScheduler::RegisterWriter(std::shared_ptr<Writer> writer)
{
	std::lock_guard<std::mutex> lock(mtx_);
	writers_[writer.get()] = writer;
}
void FlowControlScheduler::UnregisterWriter(std::shared_ptr<Writer> writer)
{
	std::lock_guard<std::mutex> lock(mtx_);
	writers_.erase(writer.get());
}
std::shared_ptr<Writer> FlowControlScheduler::GetSharedWriterPtr(Writer* writer)
{
	std::lock_guard<std::mutex> lock(mtx_);
	auto it = writers_.find(writer);
	if (it != writers_.end()) {
		return it->second;
	}
	return nullptr;
}

void FlowControlFifoScheduler::AddSendData(Writer* writer, std::shared_ptr<CacheChange> change)
{
	// 保证异步活性
	std::shared_ptr<Writer> writerPtr = GetSharedWriterPtr(writer);
	if( !writerPtr) {
		return; // Writer not registered
	}
	asyncThreads_->submit(
		[writerPtr, change]() {
			// 异步受流控
			writerPtr->DeliveryData(change, true);
		}
	);
}
void FlowControlFifoScheduler::ReSendDataFrag(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& FragSet)
{
	std::shared_ptr<StatefulWriter> writerPtr = std::dynamic_pointer_cast<StatefulWriter>(GetSharedWriterPtr(writer));
	if (!writerPtr) {
		return; // Writer not registered
	}
	asyncThreads_->submit(
		[writerPtr, readerProxy, FragSet]() {
			writerPtr->ReSendDataFrag(readerProxy, FragSet, true);
		}
	);
}

void FlowControlFifoScheduler::ReSendData(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy)
{
	std::shared_ptr<StatefulWriter> writerPtr = std::dynamic_pointer_cast<StatefulWriter>(GetSharedWriterPtr(writer));
	if (!writerPtr) {
		return; // Writer not registered
	}
	asyncThreads_->submit(
		[writerPtr, readerProxy]() {
			writerPtr->ReSendData(readerProxy, true);
		}
	);

}

