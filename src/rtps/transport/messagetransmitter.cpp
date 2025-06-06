#include "rtps/transport/messagetransmitter.h"
USING_AGIDDS_NAMESPACE

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
	unicastLocatorList_ = locatorlist;
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
		return false;
	}
}

ReturnCode_t MessageTransmitter::MessageSend(LocatorVec& sourceList, LocatorVec& destinationList, char* message, uint32_t length, uint16_t priority)
{
	for (auto& itSource : sourceList) {
		for (auto& itDest : destinationList) {
			MessageSend(itSource, itDest, message, length, priority);
		}
	}
	return RETCODE_OK;
}
