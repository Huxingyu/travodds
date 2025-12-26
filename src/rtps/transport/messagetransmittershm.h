//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef TRAVODDS_RTPS_TRANSPORT_SHAREDMEMMANAGER_H
#define TRAVODDS_RTPS_TRANSPORT_SHAREDMEMMANAGER_H

#include <array>

#include "travodds_dll.h"

#include "rtps/transport/messagetransmitter.h"

TRAVODDS_NAMESPACE_BEGIN

class SharedMemAccept;
class SharedMemSocket;

class MessageTransmitterSHM : public MessageTransmitter
{
public:
	MessageTransmitterSHM(std::shared_ptr<MessageProcessor> messageProcessor);
	virtual ~MessageTransmitterSHM();

	ReturnCode_t SendAddrAdd(Locator_t source, Locator_t destination, uint16_t priority = -1);

	ReturnCode_t SendAddrRemove(Locator_t destination);

	ReturnCode_t ReceiveAddrAdd(Locator_t receiveAddr);

	ReturnCode_t ReceiveAddrRemove(Locator_t ReceiveAddr) { return RETCODE_OK; };
	
	ReturnCode_t MessageSend(
		Locator_t source,
		Locator_t destination,
		char* message,
		uint32_t length,
		uint16_t priority = -1);

	virtual void SetStop() override;

	virtual ReturnCode_t InterpretMessage(char* message, uint32_t len);

	static const std::array<char, 16>& GetAddress() { return uuid_; }

private:
	bool IsLocalhost(Locator_t locator);

	void AsyncAccept();

private:
	asio::io_context& ioContext_; // 调用异步功能

	static const std::array<char, 16> uuid_;

	std::shared_ptr<SharedMemAccept> acceptor_;

	std::map<Locator_t, std::shared_ptr<SharedMemSocket>> sendSockets_;

	std::set<std::shared_ptr<SharedMemSocket>> recvSockets_;
	std::mutex sendSocketsMtx_;
};

TRAVODDS_NAMESPACE_END

#endif //TRAVODDS_RTPS_TRANSPORT_SHAREDMEMMANAGER_H
