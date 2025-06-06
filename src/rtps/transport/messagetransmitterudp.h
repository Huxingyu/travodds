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
#ifndef AGIDDS_RTPS_TRANSPORT_MESSAGETRANSMITTERUDP_H
#define AGIDDS_RTPS_TRANSPORT_MESSAGETRANSMITTERUDP_H

#include "type/basictype.h"
#include "rtps/transport/messagetransmitter.h"

AGIDDS_NAMESPACE_BEGIN

using _SEND_SOCKET_MAP = std::map<uint32_t, std::map<uint32_t, std::shared_ptr<asio::ip::udp::socket>>>;
using _RECV_SOCKET_MAP = std::map<uint16_t, std::shared_ptr<asio::ip::udp::socket>>;
using _SEND_PORT_SOCKET_MAP = std::map<Locator_t, std::shared_ptr<asio::ip::udp::socket>>;


//接收报文的最大长度 基于UDP
#define MAXSIZE 65527
class MessageTransmitterUDP : public MessageTransmitter
{
public:
	//构造函数
	MessageTransmitterUDP(std::shared_ptr<MessageProcessor> messageProcessor);

	//析构函数
	virtual ~MessageTransmitterUDP();

	/* 创建发送socket并绑定发送IP */
	ReturnCode_t SendAddrAdd(Locator_t source, Locator_t destination, uint16_t priority = -1);

	/* 发送报文，可使用LOCATOR_INVALID不指定网卡，或使用指定IP的网卡进行发送 */
	ReturnCode_t MessageSend(
		Locator_t source,
		Locator_t destination,
		char* message,
		uint32_t length,
		uint16_t priority = -1);

	/* 发送多播报文，可使用LOCATOR_INVALID不指定网卡，或使用指定IP的网卡进行发送 UDP发送中其实不区分单播和组播 */
	ReturnCode_t SendMultiMessage(
		Locator_t source,
		Locator_t destination,
		char* message,
		uint32_t length);

	//添加监听地址
	ReturnCode_t ReceiveAddrAdd(Locator_t receiveAddr);

	//删除监听地址
	ReturnCode_t ReceiveAddrRemove(Locator_t receiveAddr);

	virtual void SetStop() override;
public:
	//临时回调
	ReturnCode_t InterpretMessage(char* message, uint32_t len);

private:
	void AsyncReceive(std::shared_ptr<asio::ip::udp::socket>);
private:
	asio::io_context& ioContext_;

	/* 创建接收socket */
	ReturnCode_t AddRecvSocket(Locator_t receiveAddr);
	/* 加入组播组 */
	ReturnCode_t AddMemberShip(Locator_t receiveAddr);
	/* 退出组播组 */
	ReturnCode_t DropMemberShip(Locator_t receiveAddr);

	/* 发送socket，发送ip为key值 */
	_SEND_SOCKET_MAP		sendSocketMap_;

	/* 发送socket，发送ip+port为key值 */
	_SEND_PORT_SOCKET_MAP	sendPortSocketMap_;

	/* 接收socket，以端口号为key，端口号一致时，可绑定多个接收 地址 */
	_RECV_SOCKET_MAP		receiveSocketMap_;

	/* 保护socket，防止并发使用 */
	std::mutex mtxSendSocket_;
};

AGIDDS_NAMESPACE_END

#endif
