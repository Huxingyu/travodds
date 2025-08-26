#include "rtps/transport/messagetransmitter.h"
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
		if (it.kind == kind_) {
			unicastLocatorList_.push_back(it);
		}
	}
}


bool MessageTransmitter::CheckSocketPort(uint16_t port, bool checkUdp)
{
	asio::io_context io;

	try {
		if (checkUdp) {
			asio::ip::udp::socket socket(io);
			socket.open(asio::ip::udp::v4());
			socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), port));
			socket.close();
		} else {
			asio::ip::tcp::socket socket(io);
			socket.open(asio::ip::tcp::v4());
			socket.bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
			socket.close();
		}
		return true;
	} catch (const std::exception& e) {
		static_cast<void>(e);
		TRAVODDS_LOG(LOG_ERROR, "CheckSocketPort Err[%s]", e.what());
		return false;
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
