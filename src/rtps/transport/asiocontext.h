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
#ifndef AGIDDS_RTPS_TRANSPORT_ASIOCONTEXT_H
#define AGIDDS_RTPS_TRANSPORT_ASIOCONTEXT_H

#include <thread>
#include <asio.hpp>

#include "agidds_dll.h"

AGIDDS_NAMESPACE_BEGIN

class AsioContext {
private:
	AsioContext();
public:
	static AsioContext& GetInstance();
	static asio::io_context& GetIOContext();
	~AsioContext();

	AsioContext(const AsioContext&) = delete;
	AsioContext& operator=(const AsioContext&) = delete;
	AsioContext(AsioContext&&) = delete;
	AsioContext& operator=(AsioContext&&) = delete;
private:
	asio::io_context ioContext_;
	asio::executor_work_guard<asio::io_context::executor_type> workGuard_;
	std::thread ioThread_;
};

AGIDDS_NAMESPACE_END

#endif //AGIDDS_RTPS_TRANSPORT_ASIOCONTEXT_H