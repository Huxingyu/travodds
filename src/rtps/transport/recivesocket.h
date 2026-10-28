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
#include <memory>
#include <thread>
#include "travodds_dll.h"
#include "common/sys/sysinfo.h"
TRAVODDS_NAMESPACE_BEGIN

class ReciverSocket
{
    struct Runtime {
        asio::io_context context;
        asio::executor_work_guard<asio::io_context::executor_type> guard{asio::make_work_guard(context)};
    };
    std::shared_ptr<Runtime> runtime_;
public:
	ReciverSocket(const std::string& name)
		: runtime_(std::make_shared<Runtime>()), ioContext_(runtime_->context),
		  ioThread_([name, runtime = runtime_]() {
		      ThreadFunction::SetThreadName(name);
		      runtime->context.run();
		  }) {
	}
	virtual ~ReciverSocket() {
		runtime_->guard.reset();
		ioContext_.stop();
		if (ioThread_.joinable()) {
			if (std::this_thread::get_id() == ioThread_.get_id()) {
				// The last socket reference can disappear inside its handler.
				// Keep the context alive through run() and avoid self-join or
				// destroying a joinable std::thread on this same worker.
				ioThread_.detach();
			} else ioThread_.join();
		}
	}
protected:
	asio::io_context& ioContext_;
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

