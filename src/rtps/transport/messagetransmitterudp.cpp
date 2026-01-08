#include "rtps/transport/messagetransmitterudp.h"
#include "common/log/logger.h"

USING_TRAVODDS_NAMESPACE;

MessageTransmitterUDP::MessageTransmitterUDP(std::shared_ptr<MessageProcessor> messageProcessor)
	: MessageTransmitter(messageProcessor), ioContext_()
{
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
	ioContext_.stop();
}

ReturnCode_t MessageTransmitterUDP::SendAddrAdd(Locator_t source, Locator_t destination, uint16_t priority)
{
	if (source.kind != destination.kind || source.kind != kind_) {
		return RETCODE_ERROR;
	}
	AsioLocator src(source);

	const uint32_t uiAddress = src.GetUIAddress();
	/*  端口号为0时 只进行ip地址判断是否已创建 */
	if (0 == src.port) {
		/* 该发送IP的地址和优先级已创建发送socket，直接返回 */
		if (0 != sendSocketMap_.count(uiAddress) && 0 != sendSocketMap_[uiAddress].count(priority)) {
			return RETCODE_OK;
		} 
	} else {
		/* 该发送IP+端口号已创建发送socket，直接返回 */
		if (sendPortSocketMap_.count(src) != 0) {
			return RETCODE_OK;
		}
	}

	try {
		// IPPROTO_UDP
		std::shared_ptr<asio::ip::udp::socket> NewSendSocket = GetSocketInit(); //AF_INET/AF_INET6, SOCK_DGRAM;

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
		if (0 == src.port && src.Empty()) {
			sendSocketMap_[uiAddress][priority] = NewSendSocket;
			return RETCODE_OK;
		}
		/* 绑定发送socket到本地端点，使用指定的IP地址和端口号 */
		asio::ip::udp::endpoint local_endpoint = GetEndpoint(src);

		// 检查 socket 是否已经打开
		if (!NewSendSocket->is_open()) {
			// 绑定socket到本地端点
			NewSendSocket->open(local_endpoint.protocol());
		}
		NewSendSocket->bind(local_endpoint);
		if (0 == src.port) {
			sendSocketMap_[uiAddress][priority] = std::move(NewSendSocket);
		} else {
			sendPortSocketMap_.insert(std::pair<Locator_t, std::shared_ptr<asio::ip::udp::socket>>(src, NewSendSocket));
		}


	}
	catch (std::exception& e) {
		static_cast<void>(e);
		//std::cerr << "Exception: " << e.what() << std::endl;
		TRAVODDS_LOG(LOG_ERROR, "ioContext Err e[%s]", e.what());
		return RETCODE_ERROR;
	}
	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterUDP::SendAddrRemove(Locator_t destination)
{
	return ReturnCode_t();
}

ReturnCode_t MessageTransmitterUDP::MessageSend(Locator_t source1, Locator_t destination1, char* message, uint32_t length, uint16_t priority)
{
	if (source1.kind != destination1.kind || source1.kind != kind_) {
		return RETCODE_ERROR;
	}
	char* pMsg = message;
	uint32_t uiMsgLen = length;
	AsioLocator src(source1);
	AsioLocator dest(destination1);

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
	asio::ip::udp::endpoint target_endpoint = GetEndpoint(dest);
	/* 查找对应的发送socket */
	std::shared_ptr<asio::ip::udp::socket> SendSocket = nullptr;
	if (0 == src.port) {
		SendSocket = sendSocketMap_[src.GetUIAddress()][priority];
	} else {
		SendSocket = sendPortSocketMap_[src];
	}

	if (nullptr == SendSocket) {
		TRAVODDS_LOG(LOG_ERROR, "SendSocket is nullptr");
		return RETCODE_ERROR;
	}

	mtxSendSocket_.lock();
	/* 发送组播数据且未指定发送地址时，每个网卡都需要发送组播数据 */
	if (dest.IsMulticast() && src.Empty()) {
		for (auto& itor : unicastLocatorList_) {
			try {
				asio::ip::multicast::outbound_interface option = GetOutboundInterfaceOption(AsioLocator(itor));
				SendSocket->set_option(option);

				// 发送数据到目标端点
				SendSocket->send_to(asio::const_buffer(pMsg, uiMsgLen), target_endpoint);
			}
			catch (asio::system_error& e) {
				static_cast<void>(e);
				//std::cerr << "Error: " << e.what() << ":" << e.code()<<std::endl;
				//AGILOG(AGILOG_ERROR, "send_to error", e.what());
				continue;
			}
		}
	}
	else {
		try {
			SendSocket->send_to(asio::const_buffer(pMsg, uiMsgLen), target_endpoint);
		}
		catch (const std::exception& e) {
			static_cast<void>(e);
			mtxSendSocket_.unlock();
			//AGILOG(AGILOG_ERROR, "send_to error", e.what());
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
	if (receiveAddr.kind != kind_) {
		return RETCODE_ERROR;
	}
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
	if (receiveAddr.kind != kind_) {
		return RETCODE_ERROR;
	}
	ReturnCode_t ret;
	/*  只退组，不删除socket */
	ret = DropMemberShip(receiveAddr);
	if (RETCODE_OK != ret) {
		TRAVODDS_LOG(LOG_ERROR, "DropMemberShip error, ret = %d", ret);
		return ret;
	}

	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterUDP::AddRecvSocket(Locator_t receiveAddr)
{
	AsioLocator rAddr(receiveAddr);

	/* 端口号已存在 */
	if (receiveSocketMap_.count(rAddr.port) > 0) {
		return RETCODE_OK;
	}

	try {
		// 创建一个UDP监听socket
		std::shared_ptr<asio::ip::udp::socket> newRecvSocket = GetReciveSocketInit(rAddr.port);
		// 监听的本地端点，使用系统自动分配的端口
		asio::ip::udp::endpoint local_endpoint = GetEndpoint(rAddr, true);

		// 检查 socket 是否已经打开
		if (!newRecvSocket->is_open()) {
			// 绑定socket到本地端点
			newRecvSocket->open(local_endpoint.protocol());
		}

		if (rAddr.IsMulticast()) {
			/* 端口复用 */
			asio::socket_base::reuse_address option_reuse_address(true);
			newRecvSocket->set_option(option_reuse_address);
        } else {
			// Windows：禁止其他进程复用（关键！）
#ifdef _WIN32
			newRecvSocket->set_option(asio::detail::socket_option::integer<ASIO_OS_DEF(SOL_SOCKET), SO_EXCLUSIVEADDRUSE>(1));
#endif
		}


		asio::socket_base::receive_buffer_size option_receive_buffer_size(RECEIVE_BUFFER_SIZE);
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
		newRecvSocket->bind(GetReciveEndpoint(rAddr.port));
		receiveSocketMap_.insert(std::pair<uint16_t, std::shared_ptr<asio::ip::udp::socket>>(rAddr.port, newRecvSocket));
		return RETCODE_OK;
	}
	catch (asio::system_error& e) {
		//AGILOG(AGILOG_ERROR, "Err[%s]", e.what());
		if (e.code() == asio::error::address_in_use) {
			return RETCODE_PORT_IN_USE;
		}
#ifdef _WIN32
		if (e.code().value() == WSAEACCES) {
			return RETCODE_PORT_IN_USE;
		}
#endif
        //std::cerr << "Bind error: " << e.what() << std::endl;
		return RETCODE_ERROR;
	}
}

ReturnCode_t MessageTransmitterUDP::AddMemberShip(Locator_t receiveAddr)
{
	AsioLocator rAddr(receiveAddr);
	/* 判断接收地址是否是组播地址，非组播直接返回 */
	if (!rAddr.IsMulticast()) {
		return RETCODE_OK;
	}

	std::shared_ptr<asio::ip::udp::socket> socke = receiveSocketMap_[rAddr.port];

	/* 根据DomainParticipant中的IP地址进行加组  */
	for (auto& itor : unicastLocatorList_) {

		AsioLocator it(itor);
		asio::ip::multicast::join_group option = GetJoinGroupOption(rAddr, it);
		try {
			socke->set_option(option);
		}
		catch (const std::system_error& e) {
			static_cast<void>(e);
			TRAVODDS_LOG(LOG_ERROR, "Err[%s]", e.what());
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
	AsioLocator rAddr(receiveAddr);
	/* 判断接收地址是否是组播地址，非组播直接返回 */
	if (!rAddr.IsMulticast()) {
		return RETCODE_OK;
	}

	/* 退出组播组，因为该socket可能还同时加入其它组播组，不能关闭该socket */
	try {
		asio::ip::multicast::leave_group option = GetLeaveGroupOption(rAddr);
		receiveSocketMap_[rAddr.port]->set_option(option);
	}
	catch (const std::exception& e) {
		static_cast<void>(e);
		//std::cerr << "Error: " << e.what() << std::endl;
		TRAVODDS_LOG(LOG_ERROR, "Err[%s]", e.what());
		return RETCODE_ERROR;
	}
	return RETCODE_OK;
}

void MessageTransmitterUDP::SetStop()
{
	if (!runFlag_.load()) {
		return;
	}
	MessageTransmitter::SetStop();
	for (auto& socketItor : receiveSocketMap_) {
		socketItor.second->cancel();
	}
	for (auto& socketItor : sendSocketMap_) {
		for (auto& itor : socketItor.second) {
			itor.second->cancel();
		}
	}
	uint32_t tries = 0;
	while (flagNum_.load(std::memory_order_acquire) > 0 && tries++ < 20 ) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void MessageTransmitterUDP::AsyncReceive(std::shared_ptr<asio::ip::udp::socket> socket)
{
	flagNum_.fetch_add(1, std::memory_order_release);
	char* buffer = nullptr;
	bool ret = receivePool_->getBuffer(UDP_MAX_MESSAGE_SIZE, buffer);

	while (!ret && IsRun()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		ret = receivePool_->getBuffer(UDP_MAX_MESSAGE_SIZE, buffer);
	}
	if (!IsRun()) {
		flagNum_.fetch_sub(1, std::memory_order_release);
		return;
	}

	std::shared_ptr<asio::ip::udp::endpoint> sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
	socket->async_receive_from(asio::buffer(buffer, UDP_MAX_MESSAGE_SIZE), *sender_endpoint,
		[this, buffer, socket, sender_endpoint](const std::error_code& ec, std::size_t bytes_transferred) {
			ReturnCode_t ret = RETCODE_OK;
			if (!ec) {
				InterpretMessage(buffer, bytes_transferred);
			} 
			flagNum_.fetch_sub(1, std::memory_order_release);
			if (IsRun()) {
				AsyncReceive(socket);
			}
		});
}

MessageTransmitterUDPv4::MessageTransmitterUDPv4(std::shared_ptr<MessageProcessor> messageProcessor)
	: MessageTransmitterUDP(messageProcessor)
{
	kind_ = LOCATOR_KIND_UDPv4;
}

asio::ip::multicast::leave_group MessageTransmitterUDPv4::GetLeaveGroupOption(const AsioLocator& locator)
{
	return asio::ip::multicast::leave_group(locator.GetAddress<asio::ip::address_v4>());;
}
asio::ip::multicast::join_group MessageTransmitterUDPv4::GetJoinGroupOption(const AsioLocator& multicastLoc, const AsioLocator& interfaceLoc)
{
	return asio::ip::multicast::join_group(multicastLoc.GetAddress<asio::ip::address_v4>(), interfaceLoc.GetAddress<asio::ip::address_v4>());
}
asio::ip::multicast::outbound_interface MessageTransmitterUDPv4::GetOutboundInterfaceOption(const AsioLocator& locator)
{
	return asio::ip::multicast::outbound_interface(locator.GetAddress<asio::ip::address_v4>());
}
asio::ip::udp::endpoint MessageTransmitterUDPv4::GetEndpoint(const AsioLocator& locator, bool recive)
{
	if (locator.IsMulticast() && recive) {
		return asio::ip::udp::endpoint(asio::ip::udp::v4(), locator.port);
	}
	return asio::ip::udp::endpoint(locator.GetAddress<asio::ip::address_v4>(), locator.port);
}
asio::ip::udp::endpoint MessageTransmitterUDPv4::GetReciveEndpoint(const uint32_t port)
{
	return asio::ip::udp::endpoint(asio::ip::address::from_string("0.0.0.0"), port);
}
std::shared_ptr<asio::ip::udp::socket> MessageTransmitterUDPv4::GetSocketInit()
{
	return std::make_shared<asio::ip::udp::socket>(ioContext_, asio::ip::udp::v4());
}
std::shared_ptr<UDPReciverSocket> MessageTransmitterUDPv4::GetReciveSocketInit(const uint32_t port)
{
	return std::make_shared<UDPReciverSocket>(asio::ip::udp::v4(),"udp.v4."+std::to_string(port));
}
MessageTransmitterUDPv6::MessageTransmitterUDPv6(std::shared_ptr<MessageProcessor> messageProcessor)
	: MessageTransmitterUDP(messageProcessor)
{
	kind_ = LOCATOR_KIND_UDPv6;
}
asio::ip::multicast::leave_group MessageTransmitterUDPv6::GetLeaveGroupOption(const AsioLocator& locator)
{
	return asio::ip::multicast::leave_group(locator.GetAddress<asio::ip::address_v6>());;
}
asio::ip::multicast::join_group MessageTransmitterUDPv6::GetJoinGroupOption(const AsioLocator& multicastLoc, const AsioLocator& interfaceLoc)
{
	return asio::ip::multicast::join_group(multicastLoc.GetAddress<asio::ip::address_v6>(), interfaceLoc.GetUIAddress());
}
asio::ip::udp::endpoint MessageTransmitterUDPv6::GetEndpoint(const AsioLocator& locator, bool recive)
{
	if (locator.IsMulticast() && recive) {
		return asio::ip::udp::endpoint(asio::ip::udp::v6(), locator.port);
	}
	return asio::ip::udp::endpoint(locator.GetAddress<asio::ip::address_v6>(), locator.port);
}
asio::ip::udp::endpoint MessageTransmitterUDPv6::GetReciveEndpoint(const uint32_t port)
{
	return asio::ip::udp::endpoint(asio::ip::address::from_string("::"), port);
}
std::shared_ptr<asio::ip::udp::socket> MessageTransmitterUDPv6::GetSocketInit()
{
	return std::make_shared<asio::ip::udp::socket>(ioContext_, asio::ip::udp::v6());
}
std::shared_ptr<UDPReciverSocket> MessageTransmitterUDPv6::GetReciveSocketInit(const uint32_t port)
{
	return std::make_shared<UDPReciverSocket>(asio::ip::udp::v6(), "udp.v6." + std::to_string(port));
}
asio::ip::multicast::outbound_interface MessageTransmitterUDPv6::GetOutboundInterfaceOption(const AsioLocator& locator)
{
	return asio::ip::multicast::outbound_interface(locator.GetUIAddress());
}