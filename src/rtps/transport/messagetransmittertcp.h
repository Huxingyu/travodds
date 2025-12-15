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
#ifndef TRAVODDS_RTPS_TRANSPORT_MESSAGETRANSMITTERTCP_H
#define TRAVODDS_RTPS_TRANSPORT_MESSAGETRANSMITTERTCP_H

#include "type/basictype.h"
#include "rtps/transport/messagetransmitter.h"
#include "rtps/transport/locator.h"

TRAVODDS_NAMESPACE_BEGIN

class MessageTransmitterTCPv4;
class MessageTransmitterTCPv6;
class MessageTransmitterTCP : public MessageTransmitter
{
public:
	/**
     * @brief 析构函数。
     */
	virtual ~MessageTransmitterTCP();

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

	ReturnCode_t SendAddrRemove(Locator_t destination);

	/**
	 * @brief 添加RTPS报文监听地址。
	 *
	 * @param [in] receiveAddr 监听地址。
	 *
	 * @return 监听地址添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
    ReturnCode_t ReceiveAddrAdd(Locator_t receiveAddr) ;

    ReturnCode_t ReceiveAddrRemove(Locator_t ReceiveAddr) ;

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
	 * @brief 设置当前发送线程运行状态为停止，即更改runFlag_变量值为false。
	 */
	virtual void SetStop() override;

protected:
	/**
     * @brief 构造函数。
     *
     * @param [in] messageProcessor MessageProcessor指针。
     */
	MessageTransmitterTCP(std::shared_ptr<MessageProcessor> messageProcessor);

private:
	struct TCPHeader {
		char rtcp[4];
		union {
			char flag;
			uint32_t value;
		}endian;
		/* 大小端 */
		uint32_t length; /* 报文数据大小 */
		/* 校验tcp标头 */
		uint32_t crc;
		TCPHeader() {
			rtcp[0] = 'R';
			rtcp[1] = 'T';
			rtcp[2] = 'C';
			rtcp[3] = 'P';

			endian.value = 1;
			length = 0;
			crc = 0;
		}

		uint32_t GetLength() const {
			uint32_t len = 0;
			if ((endian.flag == 1) == IS_LITTLE_ENDIAN) {
				len = length;
			} else {
				len = ntohl(length);
			}
			return len;
		}

		// 设置CRC值
		void SetCRC() {
			crc = CalculateCRC();
		}

		// 验证CRC值
		bool ValidateCRC() const {
			uint32_t calculated_crc = CalculateCRC();
			return calculated_crc == crc;
		}

	private:
		// 计算CRC32校验值（按实际内存布局计算）
		uint32_t CalculateCRC() const {
			// 临时副本，将crc字段置0后再计算
			TCPHeader temp = *this;
			temp.crc = 0;

			// CRC计算范围：从endian字段开始到length字段结束
			// 即内存中的 &endian 到 &length + sizeof(length)
			const char* data_start = reinterpret_cast<const char*>(&temp.endian);
			size_t data_length = sizeof(temp.endian) + sizeof(temp.length);

			return CalculateCRC32(data_start, data_length);
		}


		// CRC32计算实现（直接按内存字节计算）
		static uint32_t CalculateCRC32(const char* data, size_t length) {
			uint32_t crc = 0xFFFFFFFF;
			uint32_t polynomial = 0xEDB88320;

			for (size_t i = 0; i < length; ++i) {
				crc ^= static_cast<uint8_t>(data[i]);

				for (int j = 0; j < 8; j++) {
					if (crc & 1) {
						crc = (crc >> 1) ^ polynomial;
					}
					else {
						crc = crc >> 1;
					}
				}
			}

			return ~crc;
		}

	};

	enum class ReceivePhase {
		HEADER,
		MESSAGE
	};

	struct ReceiveSession {
		std::shared_ptr<asio::ip::tcp::socket> socket;
		TCPHeader header;
		char* messageBuffer;
		size_t bytesReceived;
		ReceivePhase phase;
		ReceiveSession() : 
			socket(nullptr), 
			messageBuffer(nullptr), 
			bytesReceived(0), 
			phase(ReceivePhase::HEADER) {
		}
	};
	enum class SendPhase {
		CONNETING, // 在创建连接中
		CONNECT, // 连接成功
		DISCONNECT //断开连接
	};
	struct SendSession {
		std::shared_ptr<asio::ip::tcp::socket> socket;
		SendPhase phase;
		uint16_t priority;
		SendSession() : socket(nullptr), phase(SendPhase::DISCONNECT), priority(0) {}
		SendSession(std::shared_ptr<asio::ip::tcp::socket> s, SendPhase c, uint16_t p)
			: socket(s), phase(c), priority(p) {
		}

	};
	using _SEND_TCPSOCKET_MAP = std::map<Locator_t, std::shared_ptr<SendSession>>;

	using _RECV_TCPSOCKET_MAP = std::map<std::shared_ptr<asio::ip::tcp::socket>, std::shared_ptr<ReceiveSession>>;

	void InterpreHeader(std::shared_ptr<ReceiveSession> session, const asio::error_code& ec, size_t bytesTransferred);


	void InterpretMessage(std::shared_ptr<ReceiveSession> session, const asio::error_code& ec, size_t bytesTransferred);

	/**
	 * 
	 */
	void AsyncAccept();

    /**
	 * @brief UDP报文接收函数。
	 *
	 * @param [in] socket UDP通信句柄。
	 */
	void AsyncReceive(std::shared_ptr<ReceiveSession> session);


	void AsyncConnect(const AsioLocator& src, const AsioLocator& dest, uint16_t priority);

	void CleanupSession(std::shared_ptr<ReceiveSession> session);

	virtual asio::ip::tcp::endpoint GetEndpoint(const AsioLocator& locator, bool recive = false) = 0;
	virtual asio::ip::tcp::endpoint GetReciveEndpoint(const uint32_t port) = 0;
	virtual std::shared_ptr<asio::ip::tcp::socket> GetSocketInit() = 0;

protected:
	asio::io_context& ioContext_;
private:
	asio::strand<asio::io_context::executor_type> strandWrite_;

	// Acceptor用于接受连接
	std::shared_ptr<asio::ip::tcp::acceptor> acceptor_;

	_RECV_TCPSOCKET_MAP sessions_;
	std::mutex sessionsMtx_;

	_SEND_TCPSOCKET_MAP sendSocketMap_;
	std::mutex sendSocketsMtx_;

	friend MessageTransmitterTCPv4;
	friend MessageTransmitterTCPv6;

};

class MessageTransmitterTCPv4 : public MessageTransmitterTCP {
public:
	MessageTransmitterTCPv4(std::shared_ptr<MessageProcessor> messageProcessor);
private:
	virtual asio::ip::tcp::endpoint GetEndpoint(const AsioLocator& locator, bool recive) override;
	virtual asio::ip::tcp::endpoint GetReciveEndpoint(const uint32_t port) override;
	virtual std::shared_ptr<asio::ip::tcp::socket> GetSocketInit() override;
};

class MessageTransmitterTCPv6 : public MessageTransmitterTCP {
public:
	MessageTransmitterTCPv6(std::shared_ptr<MessageProcessor> messageProcessor);
private:
	virtual asio::ip::tcp::endpoint GetEndpoint(const AsioLocator& locator, bool recive) override;
	virtual asio::ip::tcp::endpoint GetReciveEndpoint(const uint32_t port) override;
	virtual std::shared_ptr<asio::ip::tcp::socket> GetSocketInit() override;
};


TRAVODDS_NAMESPACE_END

#endif //TRAVODDS_RTPS_TRANSPORT_MESSAGETRANSMITTERTCP_H