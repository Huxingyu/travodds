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
#include "rtps/transport/locator.h"

AGIDDS_NAMESPACE_BEGIN

using _SEND_SOCKET_MAP = std::map<uint32_t, std::map<uint32_t, std::shared_ptr<asio::ip::udp::socket>>>;
using _RECV_SOCKET_MAP = std::map<uint16_t, std::shared_ptr<asio::ip::udp::socket>>;
using _SEND_PORT_SOCKET_MAP = std::map<Locator_t, std::shared_ptr<asio::ip::udp::socket>>;


//接收报文的最大长度 基于UDP
#define MAXSIZE 65527
class MessageTransmitterUDPv4;
class MessageTransmitterUDPv6;
/**
 * @brief MessageTransmitterUDP类，MessageTransmitter的派生类，RTPS报文使用UDP协议发送管理类。
 */
class MessageTransmitterUDP : public MessageTransmitter
{
public:
	/**
	 * @brief 析构函数。
	 */
	virtual ~MessageTransmitterUDP();

	/**
	 * @brief RTPS报文发送Socket创建与存储并绑定发送IP。
	 *
	 * @param [in] source 源地址。
	 * @param [in] destination 目的地址。
	 * @param [in] priority 发送IP的优先级。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t SendAddrAdd(Locator_t source, Locator_t destination, uint16_t priority = -1);

	/**
	 * @brief 发送RTPS报文，可使用LOCATOR_INVALID不指定网卡，或使用指定IP的网卡进行发送。
	 *
	 * @param [in] source 源地址。
	 * @param [in] destination 目的地址。
	 * @param [in] message RTPS报文。
	 * @param [in] length 报文长度。
	 * @param [in] priority 发送IP的优先级。
	 *
	 * @return 报文发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t MessageSend(
		Locator_t source,
		Locator_t destination,
		char* message,
		uint32_t length,
		uint16_t priority = -1);

	/**
	 * @brief 发送多播报文，可使用LOCATOR_INVALID不指定网卡，
	 * 或使用指定IP的网卡进行发送 UDP发送中其实不区分单播和组播。
	 *
	 * @param [in] source 源地址。
	 * @param [in] destination 目的地址。
	 * @param [in] message RTPS报文。
	 * @param [in] length 报文长度。
	 *
	 * @return 报文发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t SendMultiMessage(
		Locator_t source,
		Locator_t destination,
		char* message,
		uint32_t length);

	/**
	 * @brief 添加RTPS报文监听地址。
	 *
	 * @param [in] receiveAddr 监听地址。
	 *
	 * @return 监听地址添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t ReceiveAddrAdd(Locator_t receiveAddr);

	/**
	 * @brief 删除RTPS报文监听地址。
	 *
	 * @param [in] receiveAddr 监听地址。
	 *
	 * @return 监听地址删除成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t ReceiveAddrRemove(Locator_t receiveAddr);

	/**
	 * @brief 设置当前发送线程运行状态为停止，即更改runFlag_变量值为false。
	 */
	virtual void SetStop() override;
public:
	/**
	 * @brief 报文接收后临时回调函数，触发报文解析模块。
	 *
	 * @param [in] message RTPS报文。
	 * @param [in] len 报文长度。
	 *
	 * @return 报文解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t InterpretMessage(char* message, uint32_t len);

private:
	/**
	 * @brief UDP报文接收函数。
	 *
	 * @param [in] socket UDP通信句柄。
	 */
	void AsyncReceive(std::shared_ptr<asio::ip::udp::socket>);

	virtual asio::ip::multicast::leave_group GetLeaveGroupOption(const AsioLocator& locator) = 0;
	virtual asio::ip::multicast::join_group GetJoinGroupOption(const AsioLocator& multicastLoc, const AsioLocator& interfaceLoc) = 0;
	virtual asio::ip::multicast::outbound_interface GetOutboundInterfaceOption(const AsioLocator& locator) = 0;
	virtual asio::ip::udp::endpoint GetEndpoint(const AsioLocator& locator, bool recive = false) = 0;
	virtual std::shared_ptr<asio::ip::udp::socket> GetSocketInit() = 0;


protected:
	/**
     * @brief 构造函数。
     *
     * @param [in] messageProcessor MessageProcessor指针。
     */
	MessageTransmitterUDP(std::shared_ptr<MessageProcessor> messageProcessor);

protected:
	asio::io_context& ioContext_;
private:
	/**
	 * @brief 创建接收socket。
	 *
	 * @param [in] receiveAddr 报文接收地址。
	 *
	 * @return 创建成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t AddRecvSocket(Locator_t receiveAddr);

	/**
	 * @brief 加入组播组。
	 *
	 * @param [in] receiveAddr 组播地址。
	 *
	 * @return 加入成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t AddMemberShip(Locator_t receiveAddr);

	/**
	 * @brief 退出组播组。
	 *
	 * @param [in] receiveAddr 组播地址。
	 *
	 * @return 退出成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t DropMemberShip(Locator_t receiveAddr);

	/* 发送socket，发送ip为key值 */
	_SEND_SOCKET_MAP		sendSocketMap_;

	/* 发送socket，发送ip+port为key值 */
	_SEND_PORT_SOCKET_MAP	sendPortSocketMap_;

	/* 接收socket，以端口号为key，端口号一致时，可绑定多个接收 地址 */
	_RECV_SOCKET_MAP		receiveSocketMap_;

	/* 保护socket，防止并发使用 */
	std::mutex mtxSendSocket_;

	friend MessageTransmitterUDPv4;
	friend MessageTransmitterUDPv6;
};

class MessageTransmitterUDPv4 : public MessageTransmitterUDP {
public:
	MessageTransmitterUDPv4(std::shared_ptr<MessageProcessor> messageProcessor);
private:
	virtual asio::ip::multicast::leave_group GetLeaveGroupOption(const AsioLocator& locator) override;
	virtual asio::ip::multicast::join_group GetJoinGroupOption(const AsioLocator& multicastLoc, const AsioLocator& interfaceLoc) override;
	virtual asio::ip::multicast::outbound_interface GetOutboundInterfaceOption(const AsioLocator& locator) override;

	virtual asio::ip::udp::endpoint GetEndpoint(const AsioLocator& locator, bool recive) override;
	virtual std::shared_ptr<asio::ip::udp::socket> GetSocketInit() override;
};

class MessageTransmitterUDPv6 : public MessageTransmitterUDP {
public:
	MessageTransmitterUDPv6(std::shared_ptr<MessageProcessor> messageProcessor);
private:
	virtual asio::ip::multicast::leave_group GetLeaveGroupOption(const AsioLocator& locator) override;
	virtual asio::ip::multicast::join_group GetJoinGroupOption(const AsioLocator& multicastLoc, const AsioLocator& interfaceLoc) override;
	virtual asio::ip::multicast::outbound_interface GetOutboundInterfaceOption(const AsioLocator& locator) override;

	virtual asio::ip::udp::endpoint GetEndpoint(const AsioLocator& locator, bool recive) override;
	virtual std::shared_ptr<asio::ip::udp::socket> GetSocketInit() override;
};

AGIDDS_NAMESPACE_END

#endif
