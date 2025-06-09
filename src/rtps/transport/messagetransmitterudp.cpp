#include "rtps/transport/messagetransmitterudp.h"
#include "rtps/messages/messageprocessor.h"
#include "rtps/transport/locator.h"
#include "common/log/logger.h"
#include <iostream>
USING_AGIDDS_NAMESPACE;

MessageTransmitterUDP::MessageTransmitterUDP(std::shared_ptr<MessageProcessor> messageProcessor)
	: MessageTransmitter(messageProcessor), ioContext_(AsioContext::GetIOContext())
{
	kind_ = LOCATOR_KIND_UDPv4;
}

MessageTransmitterUDP::~MessageTransmitterUDP()
{
	/* 关闭所有接收socket */
	for (auto& socketItor : receiveSocketMap_) {
		socketItor.second->close();
	}
	receiveSocketMap_.clear();

	/* 关闭所有发送socket */
	for (auto& socketItor : sendSocketMap_) {
		for (auto& itor : socketItor.second) {
			itor.second->close();
		}
		socketItor.second.clear();
	}
	sendSocketMap_.clear();
}

ReturnCode_t MessageTransmitterUDP::SendAddrAdd(Locator_t source, Locator_t destination, uint16_t priority)
{
	UDPv4Locator src(source);

	if (!src.Valid()) {
		return RETCODE_OK;
	}
	const uint32_t uiAddress = src.GetUIAddressV4();
	/*  端口号为0时 只进行ip地址判断是否已创建 */
	if (0 == src.port) {
		/* 该发送IP的地址和优先级已创建发送socket，直接返回 */
		if (0 != sendSocketMap_.count(uiAddress) && 0 != sendSocketMap_[uiAddress].count(priority)) {
			return RETCODE_OK;
		}
	}
	else {
		/* 该发送IP+端口号已创建发送socket，直接返回 */
		if (sendPortSocketMap_.count(src) != 0) {
			return RETCODE_OK;
		}
	}

	try {
		// IPPROTO_UDP
		std::shared_ptr<asio::ip::udp::socket> NewSendSocket = std::make_shared<asio::ip::udp::socket>(ioContext_, asio::ip::udp::v4());//AF_INET, SOCK_DGRAM

		// 设置 SO_REUSEADDR
		asio::socket_base::reuse_address reuse_option(true);
		NewSendSocket->set_option(reuse_option);

		// 设置 IP_MULTICAST_TTL
		asio::ip::multicast::hops multicast_ttl_option(64);
		NewSendSocket->set_option(multicast_ttl_option);

		// 设置 IP_TTL
		asio::ip::unicast::hops unicast_ttl_option(63);
		NewSendSocket->set_option(unicast_ttl_option);
		// 设置 IP_TOS
		/* IP_TOS,设置优先级  IP_TOS,一共8bite,
		前 3bite用 000 - 111（0-7）
		111–Network Control（网络控制）；
		110–Internetwork Control（网间控制）；
		101–Critic（关键）；
		100–Flash Override（疾速）；
		011–Flash（闪速）；
		010–Immediate（快速）；
		001–Priority（优先）
		000–Routine（普通）。
		中间四个bit
		0000–normal service；
		1000–minimize delay；
		0100–maximize throughput；
		0010–maximize reliability；
		0001–minimize monetary cost。
		*/
		int32_t tos_value = 1;
		asio::detail::socket_option::integer<IPPROTO_IP, IP_TOS> tos_option(tos_value);
		NewSendSocket->set_option(tos_option);

		/* 既不指定发送ip也不指定发送端口号时，不进行绑定操作，由系统自动选择 */
		if (0 == src.port && 0 == uiAddress) {
			sendSocketMap_[uiAddress][priority] = NewSendSocket;

			return RETCODE_OK;
		}
		asio::ip::address_v4 address = src.GetAddressV4();
		/* 发送socket绑定发送地址, 端口号设置为0 让系统自动分配, */
		asio::ip::udp::endpoint local_endpoint(address, src.port);

		// 检查 socket 是否已经打开
		if (!NewSendSocket->is_open()) {
			// 绑定socket到本地端点
			NewSendSocket->open(local_endpoint.protocol());
		}
		NewSendSocket->bind(local_endpoint);

		if (0 == src.port) {
			sendSocketMap_[uiAddress][priority] = std::move(NewSendSocket);
		}
		else {
			sendPortSocketMap_.insert(std::pair<Locator_t, std::shared_ptr<asio::ip::udp::socket>>(src, NewSendSocket));
		}
	}
	catch (std::exception& e) {
		static_cast<void>(e);
		//std::cerr << "Exception: " << e.what() << std::endl;
		AGILOG(AGILOG_ERROR, "ioContext Err e[%s]", e.what());
		return RETCODE_ERROR;
	}
	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterUDP::MessageSend(Locator_t source1, Locator_t destination1, char* message, uint32_t length, uint16_t priority)
{
	char* pMsg = message;
	uint32_t uiMsgLen = length;
	UDPv4Locator src(source1);
	UDPv4Locator dest(destination1);

	if (!src.Valid() || !dest.Valid()) {
		return RETCODE_OK;
	}

	/* 判断收发地址是否相同，0表示本地程序之间进行发送 */
	if (0 == memcmp(src.address, dest.address, sizeof(src.address))) {
		/* 判断是否也接收该端口号的数据,0即表示是程序内部自发自收 */
		if (0 != receiveSocketMap_.count(dest.port)) {
			/*  将数据内容写入缓冲区 */
			//m_pReceiveBuffer->Write(pMsg, uiMsgLen);
			return RETCODE_OK;
		}
	}

	/* 目的地址 */
	asio::ip::address_v4 address = dest.GetAddressV4();
	asio::ip::udp::endpoint target_endpoint(address, dest.port);
	/* 查找对应的发送socket */
	std::shared_ptr<asio::ip::udp::socket> SendSocket = 0;


	if (0 == src.port) {
		SendSocket = sendSocketMap_[src.GetUIAddressV4()][priority];
	}
	else {
		SendSocket = sendPortSocketMap_[src];
	}

	if (NULL == SendSocket.get()) {
		AGILOG(AGILOG_ERROR, "SendSocket is nullptr");
		return RETCODE_ERROR;
	}

	mtxSendSocket_.lock();
	/* 发送组播数据且未指定发送地址时，每个网卡都需要发送组播数据 */
	if (0xE0 == (dest.address[12] & 0xE0) && 0 == *(int32_t*)(src.address + 12)) {
		for (auto& itor : unicastLocatorList_) {
			try {
				UDPv4Locator it(itor);
				asio::ip::address_v4 local_interface = it.GetAddressV4();
				asio::ip::multicast::outbound_interface option(local_interface);
				SendSocket->set_option(option);

				// 发送数据到目标端点
				SendSocket->send_to(asio::const_buffer(pMsg, uiMsgLen), target_endpoint);
			}
			catch (const std::exception& e) {
				static_cast<void>(e);
				//std::cerr << "Error: " << e.what() << std::endl;
				AGILOG(AGILOG_ERROR, "send_to error", e.what());
				continue;
			}
		}
	}
	else {
		try {
			asio::error_code ec;
			SendSocket->send_to(asio::const_buffer(pMsg, uiMsgLen), target_endpoint, 0, ec);
			asio::detail::throw_error(ec, "send_to"); 
		}
		catch (const std::exception& e) {
			static_cast<void>(e);
			mtxSendSocket_.unlock();
			AGILOG(AGILOG_ERROR, "send_to error", e.what());
			return RETCODE_ERROR;
		}
	}

	mtxSendSocket_.unlock();
	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterUDP::SendMultiMessage(Locator_t source, Locator_t destination, char* message, uint32_t length)
{
	return MessageSend(source, destination, message, length);
}

ReturnCode_t MessageTransmitterUDP::ReceiveAddrAdd(Locator_t receiveAddr)
{
	ReturnCode_t ret;
	/* 创建接收socket */
	ret = AddRecvSocket(receiveAddr);
	if (RETCODE_OK != ret) {
		//AGILOG(AGILOG_ERROR, "AddRecvSocket error, ret = %d", ret);
		return ret;
	}

	/* 加入组播组 */
	ret = AddMemberShip(receiveAddr);
	if (RETCODE_OK != ret) {
		//AGILOG(AGILOG_ERROR, "AddMemberShip error, ret = %d", ret);
		return ret;
	}

	AsyncReceive(receiveSocketMap_[receiveAddr.port]);

	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterUDP::ReceiveAddrRemove(Locator_t receiveAddr)
{
	ReturnCode_t ret;
	/*  只退组，不删除socket */
	ret = DropMemberShip(receiveAddr);
	if (RETCODE_OK != ret) {
		AGILOG(AGILOG_ERROR, "DropMemberShip error, ret = %d", ret);
		return ret;
	}

	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterUDP::AddRecvSocket(Locator_t receiveAddr)
{
	UDPv4Locator rAddr(receiveAddr);
	if (!rAddr.Valid()) {
		AGILOG(AGILOG_ERROR, "rAddr not valid");
		return RETCODE_ERROR;
	}

	/* 端口号已存在 */
	if (receiveSocketMap_.count(rAddr.port) > 0) {
		return RETCODE_OK;
	}

	try {
		// 创建一个UDP监听socket
		std::shared_ptr<asio::ip::udp::socket> newRecvSocket = std::make_shared<asio::ip::udp::socket>(ioContext_, asio::ip::udp::v4());//AF_INET, SOCK_DGRAM

		// 监听的本地端点，使用系统自动分配的端口
		asio::ip::udp::endpoint local_endpoint;
		if (rAddr.GetAddressV4().is_multicast()) {
			local_endpoint = asio::ip::udp::endpoint(asio::ip::udp::v4(), rAddr.port);

		} else {
			local_endpoint = asio::ip::udp::endpoint(rAddr.GetAddressV4(), rAddr.port);
		}

		// 检查 socket 是否已经打开
		if (!newRecvSocket->is_open()) {
			// 绑定socket到本地端点
			newRecvSocket->open(local_endpoint.protocol());
		}

		if (rAddr.GetAddressV4().is_multicast()) {
			/* 端口复用 */
			asio::socket_base::reuse_address option_reuse_address(true);
			newRecvSocket->set_option(option_reuse_address);
        } else {
			// Windows：禁止其他进程复用（关键！）
#ifdef _WIN32
			newRecvSocket->set_option(asio::detail::socket_option::integer<ASIO_OS_DEF(SOL_SOCKET), SO_EXCLUSIVEADDRUSE>(1));
#endif
		}


		asio::socket_base::receive_buffer_size option_receive_buffer_size(681574400);
		newRecvSocket->set_option(option_receive_buffer_size);

		// 设置接收超时
		int32_t timeout = 200;
#ifdef _WIN32
		asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO> tos_option(timeout);
		newRecvSocket->set_option(tos_option);
#else
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        if (setsockopt(newRecvSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) <0) {
			//AGILOG(AGILOG_ERROR, "setsockopt timeout Err[%d]", errno);
            return RETCODE_ERROR;
        }
#endif
		newRecvSocket->bind(local_endpoint);

		receiveSocketMap_.insert(std::pair<uint16_t, std::shared_ptr<asio::ip::udp::socket>>(rAddr.port, newRecvSocket));
		return RETCODE_OK;
	}
	catch (asio::system_error& e) {
		//AGILOG(AGILOG_ERROR, "Err[%s]", e.what());
		if (e.code() == asio::error::address_in_use) {
			return RETCODE_PORT_IN_USE;
		}
        //std::cerr << "Bind error: " << e.what() << std::endl;
		return RETCODE_ERROR;
	}
}

ReturnCode_t MessageTransmitterUDP::AddMemberShip(Locator_t receiveAddr)
{
	UDPv4Locator rAddr(receiveAddr);
	asio::ip::address_v4 multicast_address = rAddr.GetAddressV4();
	/* 判断接收地址是否是组播地址，非组播直接返回 */
	if (!multicast_address.is_multicast()) {
		return RETCODE_OK;
	}

	std::shared_ptr<asio::ip::udp::socket> socke = receiveSocketMap_[rAddr.port];


	/* 根据DomainParticipant中的IP地址进行加组  */
	for (auto& itor : unicastLocatorList_) {
		UDPv4Locator it(itor);
		if (!it.Valid()) {
			continue;
		}
		try {
			asio::ip::address_v4 interface_address = it.GetAddressV4();
			asio::ip::multicast::join_group option(multicast_address, interface_address);
			socke->set_option(option);

		}
		catch (const std::exception& e) {
			static_cast<void>(e);
			AGILOG(AGILOG_ERROR, "Err[%s]", e.what());
			//std::cerr << "Error: " << e.what() << std::endl;
			continue;
		}
		//printf("netcard ip : %d.%d.%d.%d add membership : %d.%d.%d.%d sucess !\n",
		//	itor.address[12], itor.address[13], itor.address[14], itor.address[15],
		//	rAddr.address[12], rAddr.address[13], rAddr.address[14], rAddr.address[15]);
	}

	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterUDP::DropMemberShip(Locator_t receiveAddr)
{
	UDPv4Locator rAddr(receiveAddr);
	asio::ip::address_v4 multicast_address = rAddr.GetAddressV4();
	/* 判断接收地址是否是组播地址，非组播直接返回 */
	if (!multicast_address.is_multicast()) {
		return RETCODE_OK;
	}

	/* 退出组播组，因为该socket可能还同时加入其它组播组，不能关闭该socket */
	try {
		asio::ip::multicast::leave_group option(multicast_address);
		receiveSocketMap_[rAddr.port]->set_option(option);

	}
	catch (const std::exception& e) {
		static_cast<void>(e);
		//std::cerr << "Error: " << e.what() << std::endl;
		AGILOG(AGILOG_ERROR, "Err[%s]", e.what());
		return RETCODE_ERROR;
	}
	return RETCODE_OK;
}

void MessageTransmitterUDP::SetStop()
{
	MessageTransmitter::SetStop();
	for (auto& socketItor : receiveSocketMap_) {
		socketItor.second->cancel();
	}
	for (auto& socketItor : sendSocketMap_) {
		for (auto& itor : socketItor.second) {
			itor.second->cancel();
		}
	}
	while (flagNum_.load() > 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

ReturnCode_t MessageTransmitterUDP::InterpretMessage(char* message, uint32_t len)
{
	return messageProcessor_->InterpretSubmessage(message, len);;
}

void MessageTransmitterUDP::AsyncReceive(std::shared_ptr<asio::ip::udp::socket> socket)
{
	flagNum_.fetch_add(1);
	char* buffer = nullptr;
	bool ret = receivePool_->getBuffer(MAXSIZE, buffer);

	while (!ret && IsRun()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		ret = receivePool_->getBuffer(MAXSIZE, buffer);
	}
	if (!IsRun()) {
		flagNum_.fetch_sub(1);
		return;
	}
	std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
	socket->async_receive_from(asio::buffer(buffer, MAXSIZE), *sender_endpoint,
		[this, buffer, socket, sender_endpoint](const std::error_code& ec, std::size_t bytes_transferred) {
			if (!ec) {
				InterpretMessage(buffer, bytes_transferred);
			}
			receivePool_->releaseBuffer(const_cast<char*&>(buffer));
			flagNum_.fetch_sub(1);
			if (IsRun()) {
				AsyncReceive(socket);
			}
		});
}
