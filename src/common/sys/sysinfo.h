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
#ifndef TRAVODDS_COMMON_SYS_SYSINFO_H
#define TRAVODDS_COMMON_SYS_SYSINFO_H

#include "type/basictype.h"
#include "rtps/basictypertps.h"
TRAVODDS_NAMESPACE_BEGIN

namespace SysInfo {
	/**
	 * @brief 获取当前主机名。
	 * 通过调用GetHostnameStatic()函数实现
	 *
	 * @return 获取成功返回当前主机名，否则返回"unknown"。
	 */
	std::string GetHostname();
	/**
	 * @brief 获取当前主机名。
	 *
	 * @return 获取成功返回当前主机名，否则返回"unknown"。
	 */
	std::string GetHostnameStatic();
	
	/**
	 * @brief 获取当前进程ID。
	 * 通过调用GetProcessIDStatic()函数实现
	 *
	 * @return 当前进程ID。
	 */
	std::string GetProcessID();
	int32_t GetProcessIDStatic();
	
	/**
	 * @brief 获取用户名ID。
	 * 通过调用GetUsernameStatic()函数实现
	 *
	 * @return 获取用户名ID。
	 */
	std::string GetUsername();
	std::string GetUsernameStatic();

	/**
	 * @brief 获取程序运行路径。
	 * 通过调用GetExecutableFilepathStatic()函数实现
	 *
	 * @return 程序运行路径。
	 */
	std::string GetExecutableFilepath();
	/**
	 * @brief 获取程序运行路径。
	 *
	 * @return 程序运行路径。
	 */
	std::string GetExecutableFilepathStatic();

	/**
	 * @brief 获取系统平台信息。
	 * 通过调用GetTargetStatic()函数实现
	 *
	 * @return 系统平台信息。
	 */
	std::string GetTarget();
	/**
	 * @brief 获取系统平台信息。
	 *
	 * @return 系统平台信息。
	 */
	std::string GetTargetStatic();
	
	/**
	 * @brief 获取编译时间。
	 * 通过调用GetCreationTimestampStatic()函数实现
	 *
	 * @return 编译时间。
	 */
	std::string GetCreationTimestamp();
	/**
	 * @brief 获取编译时间。
	 *
	 * @return 编译时间。
	 */
	std::string GetCreationTimestampStatic();

	/**
	 * @brief 获取软件启动时间。
	 * 通过调用GetExecutionTimestampStatic()函数实现
	 *
	 * @return 软件启动时间。
	 */
	std::string GetExecutionTimestamp();
	/**
	 * @brief 获取软件启动时间。
	 *
	 * @return 软件启动时间。
	 */
	std::string GetExecutionTimestampStatic();

	/**
	 * @brief 获取软件信息及运行环境信息并存储到信息列表中。
	 *
	 * @return 软件信息列表。
	 */
	PropertySeq GetSysInfoProperties();
};




TRAVODDS_NAMESPACE_END
#endif