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
#ifndef TRAVODDS_RTPS_TRANSPORT_ASIOCONTEXT_H
#define TRAVODDS_RTPS_TRANSPORT_ASIOCONTEXT_H

#include <thread>
#include <asio.hpp>

#include "travodds_dll.h"

TRAVODDS_NAMESPACE_BEGIN

/**
 * AsioContext类，asio功能封装。
 */
class AsioContext {
private:
	/**
	 * @brief 构造函数。
	 */
	AsioContext();
public:
	/**
	 * @brief 获取AsioContext类单例。
	 * 
	 * @return AsioContext类单例。
	 */
	static AsioContext& GetInstance();
	/**
	 * @brief 获取AsioContext类单例。
	 *
	 * @return AsioContext类单例。
	 */
	static asio::io_context& GetIOContext();
	/**
	 * @brief 析构函数。
	 */
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

TRAVODDS_NAMESPACE_END

#endif //TRAVODDS_RTPS_TRANSPORT_ASIOCONTEXT_H