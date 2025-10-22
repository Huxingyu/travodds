#include "rtps/transport/messagetransmittertcp.h"
#include "common/log/logger.h"

USING_TRAVODDS_NAMESPACE;

MessageTransmitterTCP::MessageTransmitterTCP(std::shared_ptr<MessageProcessor> messageProcessor)
	: MessageTransmitter(messageProcessor), ioContext_(AsioContext::GetIOContext()),
	  strandWrite_(AsioContext::GetIOContext().get_executor())
{
	kind_ = LOCATOR_KIND_TCPv4;
}


MessageTransmitterTCP::~MessageTransmitterTCP()
{
	asio::error_code ec;
	if (acceptor_ && acceptor_->is_open()) {
		acceptor_->close(ec);
		acceptor_.reset();
	}

	/* 关闭所有接收socket */
	for (auto& it : sessions_) {
		if (it.second->messageBuffer) {
			receivePool_->releaseBuffer(it.second->messageBuffer);
		}
		if (it.second->socket && it.second->socket->is_open()) {
			it.second->socket->close(ec);
		}
	}
	sessions_.clear();

	/* 关闭所有发送socket */
	for (auto& it : sendSocketMap_) {
		if (it.second->socket->is_open()) {
			it.second->socket->shutdown(asio::ip::tcp::socket::shutdown_send, ec);
			it.second->socket->close(ec);
		}
	}
	sendSocketMap_.clear();
}

ReturnCode_t MessageTransmitterTCP::SendAddrAdd(Locator_t source, Locator_t destination, uint16_t priority)
{
	if (source.kind != destination.kind || destination.kind != kind_ || !IsRun()) {
		return RETCODE_ERROR;
	}

	AsioLocator src(source);
	AsioLocator dest(destination);

	{
		std::lock_guard<std::mutex> lock(sendSocketsMtx_);
		if (sendSocketMap_.count(dest) != 0) {
			auto& session = sendSocketMap_[dest];
			if (session->phase != SendPhase::DISCONNECT) {
				return RETCODE_OK;
			}
		}
	}

	// 创建连接任务
	 AsyncConnect(src, dest, priority);
	 return RETCODE_OK;
}


ReturnCode_t MessageTransmitterTCP::SendAddrRemove(Locator_t destination) 
{
	std::lock_guard<std::mutex> lock(sendSocketsMtx_);
	auto it = sendSocketMap_.find(destination);
	if (it != sendSocketMap_.end()) {
		try {
			if (it->second->socket->is_open()) {
				it->second->socket->shutdown(asio::ip::tcp::socket::shutdown_send);
				it->second->socket->close();
			}
		} catch (const std::exception& e) {
		}
		sendSocketMap_.erase(it);
	}
	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterTCP::ReceiveAddrAdd(Locator_t receiveAddr)
{
	if (receiveAddr.kind != kind_ || !IsRun()) {
		return RETCODE_ERROR;
	}

	/* 已经存在监听端口号 不能再监听其他地址 */
	if (acceptor_ && acceptor_->is_open())
		return RETCODE_OK;

	AsioLocator rAddr(receiveAddr);

	try
	{
		// 创建acceptor
		asio::ip::tcp::endpoint endpoint =  GetEndpoint(rAddr, true);
		acceptor_ = std::make_shared<asio::ip::tcp::acceptor>(ioContext_);
		if (!acceptor_->is_open()) {
			// 打开acceptor
			acceptor_->open(endpoint.protocol());
		}
		//// 设置SO_REUSEADDR选项 端口复用
		//acceptor_->set_option(asio::ip::tcp::acceptor::reuse_address(true));

		// Windows：禁止其他进程复用（关键！）
#ifdef _WIN32
		acceptor_->set_option(asio::detail::socket_option::integer<ASIO_OS_DEF(SOL_SOCKET), SO_EXCLUSIVEADDRUSE>(1));
#endif

		// 绑定到端点
		acceptor_->bind(GetReciveEndpoint(rAddr.port));

		// 开始监听，设置最大连接队列为100
		acceptor_->listen(100);

		// 启动异步接受连接
		AsyncAccept();
	} catch (const asio::system_error& e) {
		if (acceptor_ && acceptor_->is_open()) {
			acceptor_->close();
		}
		acceptor_.reset();
		if (e.code() == asio::error::address_in_use) {
			return RETCODE_PORT_IN_USE;
		}
#ifdef _WIN32
		if (e.code().value() == WSAEACCES) {
			return RETCODE_PORT_IN_USE;
		}
#endif
		return RETCODE_ERROR;
	}

	return RETCODE_OK;
}

ReturnCode_t MessageTransmitterTCP::ReceiveAddrRemove(Locator_t ReceiveAddr)
{
	return ReturnCode_t();
}

ReturnCode_t MessageTransmitterTCP::MessageSend(Locator_t source, Locator_t destination, char* message, uint32_t length, uint16_t priority)
{
	if (source.kind != destination.kind || source.kind != kind_ || !IsRun()) {
		return RETCODE_ERROR;
	}

	AsioLocator dest(destination);

	std::shared_ptr<SendSession> session = nullptr;
	std::shared_ptr<asio::ip::tcp::socket> sendsocket;
	{
		std::lock_guard<std::mutex> lock(sendSocketsMtx_);
		auto it = sendSocketMap_.find(dest);
		if (it != sendSocketMap_.end()) {
			session = it->second;
		}
	}

	if (!session || session->phase == SendPhase::DISCONNECT) {
		SendAddrAdd(source, destination, priority);
		return RETCODE_ERROR;
	} else if (session->phase == SendPhase::CONNETING) {
		return RETCODE_ERROR;
	}
	sendsocket = session->socket;

	TCPHeader tcphead;
	tcphead.length = length;
	tcphead.SetCRC();
	std::vector<asio::const_buffer> buffers;
	buffers.push_back(asio::buffer(&tcphead, sizeof(TCPHeader)));
	buffers.push_back(asio::buffer(message, length));

	std::promise<size_t> bytesPromise;
	asio::error_code ec;
	auto bytesFuture = bytesPromise.get_future();
	asio::post(strandWrite_, [&]() {
		if(sendsocket->lowest_layer().is_open() == false) {
			bytesPromise.set_value(0);
		}
		size_t bytesSent = asio::write(*sendsocket, buffers, ec);
		bytesPromise.set_value(bytesSent);

	});
	if (bytesFuture.get() != (sizeof(TCPHeader) + length)) {
		if (ec) {
			if (ec.value() == asio::error::connection_reset ||
				ec.value() == asio::error::broken_pipe) {
				std::lock_guard<std::mutex> lock(sendSocketsMtx_);
				auto it = sendSocketMap_.find(dest);
				if (it == sendSocketMap_.end()) {
					return RETCODE_ERROR;
				}
				if (it->second->phase == SendPhase::CONNECT) {
					it->second->phase = SendPhase::DISCONNECT;
					it->second->socket.reset();
				}
			}
		} else {
			TRAVODDS_LOG(LOG_ERROR
				, "[TCP Mode] Failed to send message, port %d , len = %d\n",
				destination.port, length + sizeof(TCPHeader));
		}
		return RETCODE_ERROR;
	}
	 
	return RETCODE_OK;
}

void MessageTransmitterTCP::InterpreHeader(std::shared_ptr<ReceiveSession> session, const asio::error_code& ec, size_t bytesTransferred)
{
	if (ec) {
		// 主动释放异常连接
		CleanupSession(session);
		return;
	}
	if (bytesTransferred != sizeof(TCPHeader)) {
		TRAVODDS_LOG(LOG_ERROR,
			"Incomplete header received: %zu/%zu bytes",
			bytesTransferred, sizeof(TCPHeader));
		CleanupSession(session);
		return;
	}
	// 验证RTCP标识
	if (session->header.rtcp[0] != 'R' || session->header.rtcp[1] != 'T' ||
		session->header.rtcp[2] != 'C' || session->header.rtcp[3] != 'P') {
		TRAVODDS_LOG(LOG_ERROR,
			"Invalid RTCP identifier");
		CleanupSession(session);
		return;
	}
	// 验证CRC
	if (!session->header.ValidateCRC()) {
		TRAVODDS_LOG(LOG_ERROR,
			"Header CRC validation failed");
		CleanupSession(session);
		return;
	}
	// 获取消息长度（处理字节序）
	uint32_t messagelength = session->header.GetLength();
	// 验证消息长度
	if (messagelength > TCP_MAX_MESSAGE_SIZE) {
		TRAVODDS_LOG(LOG_ERROR,
			"Message too large: %u bytes", messagelength);
		CleanupSession(session);
		return;
	}


	bool ret = receivePool_->getBuffer(messagelength, session->messageBuffer);

	while (!ret && IsRun()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		ret = receivePool_->getBuffer(messagelength, session->messageBuffer);
	}
	session->bytesReceived = 0;
	session->phase = ReceivePhase::MESSAGE;
	AsyncReceive(session);
}

void MessageTransmitterTCP::InterpretMessage(std::shared_ptr<ReceiveSession> session, const asio::error_code& ec, size_t bytesTransferred)
{
	if (ec) {
		// 主动释放异常连接
		CleanupSession(session);
		return;
	}
	session->bytesReceived += bytesTransferred;
	if (session->bytesReceived == session->header.GetLength()) {
		MessageTransmitter::InterpretMessage(session->messageBuffer, session->header.GetLength());
		// 重置会话状态，准备接收下一个消息
		session->phase = ReceivePhase::HEADER;
		session->bytesReceived = 0;
		receivePool_->releaseBuffer(session->messageBuffer);
	} else {
		CleanupSession(session);
		return;
	}
	AsyncReceive(session);
}

void MessageTransmitterTCP::AsyncAccept()
{
	flagNum_.fetch_add(1);
	if (!acceptor_ || !acceptor_->is_open() || !IsRun()) {
		flagNum_.fetch_sub(1);
		return;
	}

	try {
		// 创建新socket
		auto newSocket = std::make_shared<asio::ip::tcp::socket>(ioContext_);
		// 异步接受连接
		acceptor_->async_accept(*newSocket,
			[this, newSocket](const asio::error_code& ec) {
				if (!ec) {
					// 获取客户端地址信息
					//asio::ip::tcp::endpoint remote_endpoint = newSocket->remote_endpoint();
#ifdef WIN32
					/* 单位 毫秒 */
					int tv = 200;
#else
					struct timeval tv;
					tv.tv_sec = 0;
					tv.tv_usec = 200 * 100;
#endif
					if (-1 == setsockopt(newSocket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv))) {

						throw std::exception("setsockopt SO_RCVTIMEO failed");
					}

					// 添加到连接列表
					// 创建接收会话
					auto session = std::make_shared<ReceiveSession>();
					session->socket = newSocket;
					{
						std::lock_guard<std::mutex> lock(sessionsMtx_);
						sessions_[newSocket] = session;
					}

					AsyncReceive(session);

				} else {
					TRAVODDS_LOG(LOG_ERROR,
						"[TCP Mode] Accept error: %s", ec.message().c_str());
				}
				flagNum_.fetch_sub(1);
				// 继续接受下一个连接
				if (acceptor_ && acceptor_->is_open()) {
					AsyncAccept();
				}
			}
		);

	} catch(const std::exception& e){
		TRAVODDS_LOG(LOG_ERROR,
			"[TCP Mode] socket_accept exception: %s", e.what());
		flagNum_.fetch_sub(1);
		return;

	}
}

void MessageTransmitterTCP::AsyncReceive(std::shared_ptr<ReceiveSession> session)
{
	flagNum_.fetch_add(1);
	if (!session->socket->is_open() || !IsRun()) {
		CleanupSession(session);
		flagNum_.fetch_sub(1);
		return;
	}

	if (session->phase == ReceivePhase::HEADER) {
		// 接收TCP头部
		asio::async_read(*session->socket,
			asio::buffer(&session->header, sizeof(TCPHeader)),
			[this, session](const asio::error_code& ec, size_t bytes_transferred) {
				InterpreHeader(session, ec, bytes_transferred);
				flagNum_.fetch_sub(1);
			});
	} else {
		// 接收消息体
		asio::async_read(*session->socket,
			asio::buffer(session->messageBuffer + session->bytesReceived,
				session->header.GetLength() - session->bytesReceived),
			[this, session](const asio::error_code& ec, size_t bytes_transferred) {
				InterpretMessage(session, ec, bytes_transferred);
				flagNum_.fetch_sub(1);
			});
	}


}

void MessageTransmitterTCP::AsyncConnect(const AsioLocator& src, const AsioLocator& dest, uint16_t priority)
{
	try {
		// 标记为正在连接中
		{
			std::lock_guard<std::mutex> lock(sendSocketsMtx_);
			sendSocketMap_[dest] = std::make_shared<SendSession>(nullptr, SendPhase::CONNETING, priority);
		}

		// TODO v4和v6的处理
		std::shared_ptr<asio::ip::tcp::socket> newSendSocket = GetSocketInit();

		/* 设置端口复用  */
		asio::socket_base::reuse_address reuse_address_option(true);
		newSendSocket->set_option(reuse_address_option);
#ifdef __linux__
		if (priority != static_cast<uint16_t>(-1)) {
			int iPriority = priority;
			// 使用原生socket选项设置SO_PRIORITY
			asio::detail::socket_option::integer<SOL_SOCKET, SO_PRIORITY> priority_option(iPriority);
			newSendSocket->set_option(priority_option);
		}
#endif

		// 设置TCP_NODELAY选项
		asio::ip::tcp::no_delay no_delay_option(true);
		newSendSocket->set_option(no_delay_option);

#ifdef WIN32
		/* 单位: 毫秒 */
		int tv = 2000;
#else
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 200 * 1000;
#endif

		if (-1 == setsockopt(newSendSocket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv))) {

			throw std::exception("setsockopt SO_SNDTIMEO failed");
		}
		/* 如果端口号不为0,绑定端口号 */
		if (0 != src.port)
		{
			newSendSocket->bind(GetEndpoint(src, true));
		}

		// 创建目标端点并连接
		asio::ip::tcp::endpoint remote_endpoint = GetEndpoint(dest); 


		// 异步连接
		newSendSocket->async_connect(remote_endpoint,
			[this, newSendSocket, src, dest, priority] (asio::error_code ec) {
				std::lock_guard<std::mutex> lock(sendSocketsMtx_);
				// 检查是否已经被其他连接覆盖
				auto it = sendSocketMap_.find(dest);
				if (it == sendSocketMap_.end() || it->second->phase == SendPhase::CONNECT) {
					return;
				}
				if (!ec) {
					it->second->socket = newSendSocket;
					it->second->phase = SendPhase::CONNECT;
					it->second->priority = priority;
				} else {
					// 连接失败
					TRAVODDS_LOG(LOG_ERROR, 
						"Connection failed to %s:%d: %s",
						dest.GetStringAddress().data(), dest.port, ec.message().data());
					sendSocketMap_.erase(it);
				}

			});

	} catch (std::exception& e) {
		static_cast<void>(e);
		//std::cerr << "Exception: " << e.what() << std::endl;
		TRAVODDS_LOG(LOG_ERROR, "ioContext Err e[%s]", e.what());
	}

}

void MessageTransmitterTCP::CleanupSession(std::shared_ptr<ReceiveSession> session) {
	if(session->messageBuffer) {
		receivePool_->releaseBuffer(session->messageBuffer);
	}
	std::lock_guard<std::mutex> lock(sessionsMtx_);

	if (session->socket && session->socket->is_open()) {
		asio::error_code ec;
		session->socket->close(ec);
	}
	sessions_.erase(session->socket);

}

MessageTransmitterTCPv4::MessageTransmitterTCPv4(std::shared_ptr<MessageProcessor> messageProcessor)
	: MessageTransmitterTCP(messageProcessor)
{
	kind_ = LOCATOR_KIND_TCPv4;
}

asio::ip::tcp::endpoint MessageTransmitterTCPv4::GetEndpoint(const AsioLocator& locator, bool recive)
{
	if (recive) {
		return asio::ip::tcp::endpoint(asio::ip::tcp::v4(), locator.port);
	}
	return asio::ip::tcp::endpoint(locator.GetAddress<asio::ip::address_v4>(), locator.port);
}
asio::ip::tcp::endpoint MessageTransmitterTCPv4::GetReciveEndpoint(const uint32_t port)
{
	return asio::ip::tcp::endpoint(asio::ip::address::from_string("0.0.0.0"), port);
}
std::shared_ptr<asio::ip::tcp::socket> MessageTransmitterTCPv4::GetSocketInit()
{
	return std::make_shared<asio::ip::tcp::socket>(ioContext_, asio::ip::tcp::v4());
}
MessageTransmitterTCPv6::MessageTransmitterTCPv6(std::shared_ptr<MessageProcessor> messageProcessor)
	: MessageTransmitterTCP(messageProcessor)
{
	kind_ = LOCATOR_KIND_TCPv6;
}

asio::ip::tcp::endpoint MessageTransmitterTCPv6::GetEndpoint(const AsioLocator& locator, bool recive)
{
	if (recive) {
		return asio::ip::tcp::endpoint(asio::ip::tcp::v6(), locator.port);
	}
	return asio::ip::tcp::endpoint(locator.GetAddress<asio::ip::address_v6>(), locator.port);
}
asio::ip::tcp::endpoint MessageTransmitterTCPv6::GetReciveEndpoint(const uint32_t port)
{
	return asio::ip::tcp::endpoint(asio::ip::address::from_string("::"), port);
}
std::shared_ptr<asio::ip::tcp::socket> MessageTransmitterTCPv6::GetSocketInit()
{
	return std::make_shared<asio::ip::tcp::socket>(ioContext_, asio::ip::tcp::v6());
}
