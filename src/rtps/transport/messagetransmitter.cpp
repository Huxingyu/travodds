#include "rtps/transport/messagetransmitter.h"
#include "rtps/messages/messageprocessor.h"
#include "common/log/logger.h"

USING_TRAVODDS_NAMESPACE

bool MessageTransmitter::IsRun()
{
	return runFlag_.load();
}

void MessageTransmitter::SetRun()
{
	runFlag_.store(true);
}

bool MessageTransmitter::IsStop()
{
	return flagNum_.load() == 0 && !runFlag_.load();
}

void MessageTransmitter::SetStop()
{
	runFlag_.store(false);
}
uint32_t MessageTransmitter::GetTransmitterKind()
{
	return kind_;
}

void MessageTransmitter::SetUnicastLocatorList(const LocatorVec& locatorlist)
{
	for (auto& it : locatorlist) {
		Locator_t loc = it;
		loc.kind = kind_;
		unicastLocatorList_.push_back(loc);
	}
}

ReturnCode_t MessageTransmitter::MessageSend(LocatorVec& sourceList, LocatorVec& destinationList, char* message, uint32_t length, uint16_t priority)
{
	for (auto& itSource : sourceList) {
		if (itSource.kind != kind_) {
			continue;
		}
		for (auto& itDest : destinationList) {
			if (itDest.kind != kind_) {
				continue;
			}
			MessageSend(itSource, itDest, message, length, priority);
		}
	}
	return RETCODE_OK;
}

ReturnCode_t MessageTransmitter::MessageSend(LocatorVec& destinationList, char* message, uint32_t length, uint16_t priority)
{
	Locator_t srcLocator;
	srcLocator.kind = kind_;
	for (auto& itDest : destinationList) {
		if (itDest.kind != kind_) {
			continue;
		}
		MessageSend(srcLocator, itDest, message, length, priority);
	}
	return RETCODE_OK;
}

ReturnCode_t MessageTransmitter::InterpretMessage(char* message, uint32_t len)
{
	std::shared_ptr<Message_t> messagePtr = std::shared_ptr<Message_t>(
	    new Message_t(message, len), 
		[](Message_t* p) {
			MemoryPool::GetInstance()->releaseBuffer(p->originBuffer);
			 delete p;
		});
	return messageProcessor_->InterpretSubmessage(messagePtr);
}