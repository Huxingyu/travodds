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
#ifndef TRAVODDS_RTPS_TRANSPORT_RECIVER_SOCKET_H
#define TRAVODDS_RTPS_TRANSPORT_RECIVER_SOCKET_H

#include <asio.hpp>
#include "travodds_dll.h"
#include "common/sys/sysinfo.h"
TRAVODDS_NAMESPACE_BEGIN

class ReciverSocket
{
public:
	ReciverSocket(const std::string& name)
		: workGuard_(asio::make_work_guard(ioContext_)),
		  ioThread_([name,this]() {
		      ThreadFunction::SetThreadName(name);
		      ioContext_.run(); 
		  }) {
	}
	virtual ~ReciverSocket() {
		workGuard_.reset();
		ioContext_.stop();
		if (std::this_thread::get_id() != ioThread_.get_id()) {
			ioThread_.join();
		}
	}
protected:
	asio::io_context ioContext_;
	asio::executor_work_guard<asio::io_context::executor_type> workGuard_;
	std::thread ioThread_;

};

class UDPReciverSocket : public ReciverSocket, public asio::ip::udp::socket {
public:
	UDPReciverSocket(const asio::ip::udp& protocol, const std::string& name)
		: ReciverSocket(name), asio::ip::udp::socket(ioContext_, protocol) {
	}
	virtual ~UDPReciverSocket() = default;
};

class TCPReciverSocket : public ReciverSocket, public asio::ip::tcp::socket {
public:
	TCPReciverSocket(const std::string& name)
		: ReciverSocket(name),asio::ip::tcp::socket(ioContext_) {
	}
	virtual ~TCPReciverSocket() = default;
};


TRAVODDS_NAMESPACE_END

#endif

