#include "rtps/transport/messagetransmittershm.h"
#include "rtps/transport/sharedmemwatchdog.h"
#include "rtps/transport/sharedmemuuid.h"
#include "rtps/transport/sharedmemaccept.h"
#include "rtps/transport/sharedmemsocket.h"
#include "rtps/messages/messageprocessor.h"
#include "common/log/logger.h"
USING_TRAVODDS_NAMESPACE;


const std::array<char, 16> MessageTransmitterSHM::uuid_(SharedMemUUID::GetInstance().GetUUID());
MessageTransmitterSHM::MessageTransmitterSHM(std::shared_ptr<MessageProcessor> messageProcessor)
	: MessageTransmitter(messageProcessor)
	, ioContext_(AsioContext::GetIOContext())
{
	kind_ = LOCATOR_KIND_SHM;
}

MessageTransmitterSHM::~MessageTransmitterSHM()
{
	/* 关闭所有发送socket */
	for (auto& it : sendSockets_) {
		it.second->Close();
	}
	sendSockets_.clear();

	if (acceptor_) {
		acceptor_->Close();
		acceptor_.reset();
	}

	for (auto& it : recvSockets_) {
		it->Close();
	}
	sendSockets_.clear();
}


ReturnCode_t MessageTransmitterSHM::SendAddrAdd(Locator_t source, Locator_t destination, uint16_t priority)
{
	if (source.kind != destination.kind || destination.kind != kind_ || !IsLocalhost(destination) || !IsRun()) {
		return RETCODE_ERROR;
	}
	std::lock_guard<std::mutex> lock(sendSocketsMtx_);

	if (sendSockets_.find(destination) != sendSockets_.end()) {
		return RETCODE_OK;
	}

	std::shared_ptr<SharedMemSocket> sendsocket = std::make_shared<SharedMemSocket>();

	if (!sendsocket->Connect(std::to_string(destination.port))) {
		return RETCODE_ERROR;
	}
	sendSockets_[destination] = sendsocket;
	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterSHM::SendAddrRemove(Locator_t destination)
{
	std::lock_guard<std::mutex> lock(sendSocketsMtx_);
	auto it = sendSockets_.find(destination);
	if (it != sendSockets_.end()) {
		try {
			it->second->Shutdown();
			it->second->Close();
		} catch (const std::exception& e) {
		}
		sendSockets_.erase(it);
	}
	return RETCODE_OK;
}


ReturnCode_t  MessageTransmitterSHM::ReceiveAddrAdd(Locator_t receiveAddr)
{
	if (receiveAddr.kind != kind_ || !IsLocalhost(receiveAddr) || !IsRun()) {
		return RETCODE_ERROR;
	}

	/* 已经存在监听端口号 */
	if (acceptor_) {
		return RETCODE_OK;
	}
	try {
		ReturnCode_t ret = SharedMemWatchdog::GetInstance().InsertPortProcess(receiveAddr.port);
		if (ret != RETCODE_OK) {
			return ret;
		}
		acceptor_ = std::make_shared<SharedMemAccept>(std::to_string(receiveAddr.port));

		AsyncAccept();

	} catch (const std::exception& e) {
		acceptor_.reset();
		return RETCODE_ERROR;
	}
	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterSHM::MessageSend(
	Locator_t source,
	Locator_t destination,
	char* message,
	uint32_t length,
	uint16_t priority)
{
	if (source.kind != destination.kind || destination.kind != kind_ || !IsLocalhost(destination) || !IsRun()) {
		return RETCODE_ERROR;
	}
	std::shared_ptr<SharedMemSocket> sendsocket;
	{
		std::lock_guard<std::mutex> lock(sendSocketsMtx_);
		auto it = sendSockets_.find(destination);
		if (it == sendSockets_.end()) {
			return RETCODE_ERROR;
		}
		sendsocket = it->second;
	}
	try {
		sendsocket->Write(message, length);
	} catch (const std::exception& e) {
		TRAVODDS_LOG(LOG_ERROR,
			"send exception: %s", e.what());
		return RETCODE_ERROR;
	}
	return RETCODE_OK;
}

void MessageTransmitterSHM::SetStop()
{
	MessageTransmitter::SetStop();
	acceptor_->Shutdown();

	for (auto& socketItor : sendSockets_) {
		socketItor.second->Shutdown();
	}

	for (auto& socketItor : recvSockets_) {
		socketItor->Shutdown();
	}
	uint32_t tries = 0;
	while (flagNum_.load() > 0 && tries++ < 20) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

ReturnCode_t MessageTransmitterSHM::InterpretMessage(char* message, uint32_t len)
{
	return messageProcessor_->InterpretSubmessage(std::make_shared<Message_t>(message, len, true));
}

bool MessageTransmitterSHM::IsLocalhost(Locator_t locator)
{
	return memcmp(locator.address, uuid_.data(), uuid_.size()) == 0;
}


void MessageTransmitterSHM::AsyncAccept()
{
	flagNum_.fetch_add(1);
	if (!acceptor_ || !IsRun()) {
		flagNum_.fetch_sub(1);
		return;
	}

	try {
		std::shared_ptr<SharedMemSocket> newSocket = std::make_shared<SharedMemSocket>(
			[this](char* message, const size_t& len) {
				this->InterpretMessage(message, len);
			}
		);

		acceptor_->AsyncAccept(newSocket,
			[this, newSocket](const std::exception* ec) {
				if (!ec) {
				    recvSockets_.insert(newSocket);
				} else {
					TRAVODDS_LOG(LOG_ERROR,
						"Accept error: %s", ec->what());
				}
				flagNum_.fetch_sub(1);
				if (acceptor_ && !acceptor_->IsShutdown()) {
					AsyncAccept();
				}
			});

	} catch (const std::exception& e) {
		TRAVODDS_LOG(LOG_ERROR,
			"socket_accept exception: %s", e.what());
		flagNum_.fetch_sub(1);
		return;
	}

}