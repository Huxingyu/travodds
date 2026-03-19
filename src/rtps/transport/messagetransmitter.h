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
#ifndef TRAVODDS_RTPS_TRANSPORT_MESSAGETRANSMITTER_H
#define TRAVODDS_RTPS_TRANSPORT_MESSAGETRANSMITTER_H

#include <chrono>
#include <thread>
#include <atomic>
#include "type/basictype.h"
#include "rtps/basictypertps.h"
#include "dcps/base/retcode.h"
#include "common/memory/memorypool.h"

TRAVODDS_NAMESPACE_BEGIN

class MessageProcessor;
/**
 * @brief MessageTransmitter类，RTPS报文发送管理类。
 */
class MessageTransmitter
{
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] messageProcessor MessageProcessor指针。
	 */
	MessageTransmitter(std::shared_ptr<MessageProcessor> messageProcessor)
		: runFlag_(true), flagNum_(0), kind_(LOCATOR_KIND_INVALID) , receivePool_(MemoryPool::GetInstance()), messageProcessor_(messageProcessor)
	{
	}

	/**
	 * @brief 析构函数。
	 */
	virtual ~MessageTransmitter()
	{
	};

	/**
	 * @brief RTPS报文发送Socket创建与存储并绑定发送IP。
	 *
	 * @param [in] source 源地址。
	 * @param [in] destination 目的地址。
	 * @param [in] priority 发送IP的优先级。
	 *
	 * @return 封装成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t SendAddrAdd(Locator_t source, Locator_t destination, uint16_t priority = -1) = 0;

	virtual ReturnCode_t SendAddrRemove(Locator_t destination) = 0;

	/**
	 * @brief RTPS报文发送函数（虚函数）
	 *
	 * @param [in] source 源地址。
	 * @param [in] destination 目的地址。
	 * @param [in] message RTPS报文。
	 * @param [in] length 报文长度。
	 * @param [in] priority 发送IP的优先级。
	 *
	 * @return 报文发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t MessageSend(
		Locator_t source,
		Locator_t destination,
		char* message,
		uint32_t length,
		uint16_t priority = -1) = 0;

    /**
	 * @brief 添加RTPS报文监听地址。
	 *
	 * @param [in] receiveAddr 监听地址。
	 *
	 * @return 监听地址添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t ReceiveAddrAdd(Locator_t receiveAddr) = 0;

	//添加监听多播地址
	//virtual ReturnCode_t ReceiveMultiAddrAdd(Locator_t sourceAddr, Locator_t receiveAddr) = 0;

	/**
	 * @brief 删除RTPS报文监听地址。
	 *
	 * @param [in] receiveAddr 监听地址。
	 *
	 * @return 监听地址删除成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t ReceiveAddrRemove(Locator_t receiveAddr) = 0;

	/**
	 * @brief 获取当前发送线程运行状态。
	 *
	 * @return runFlag_变量值。
	 */
	bool IsRun();

	/**
	 * @brief 设置当前发送线程运行状态为启动，即更改runFlag_变量值为true。
	 */
	void SetRun();

	/**
	 * @brief 获取当前发送线程运行状态是否停止。
	 *
	 * @return 停止返回true，否则返回false。
	 */
	bool IsStop();

	/**
	 * @brief 设置当前发送线程运行状态为停止，即更改runFlag_变量值为false。
	 */
	virtual void SetStop();

	/**
	 * @brief 获取当前传输类型。
	 *
	 * @return kind_变量值。
	 */
	int32_t GetTransmitterKind() const;

	/**
	 * @brief 设置默认的IP地址。
	 *
	 * @param [in] locatorlist 默认的IP地址容器。
	 */
	void SetUnicastLocatorList(const LocatorVec& locatorlist);


	/**
	 * @brief 关闭socket。
	 *
	 * @param [in] fd socket句柄指针。
	 */
	template <typename Socket>
	static void CloseSocket(std::shared_ptr<Socket> fd) {
		if (fd) {
			fd->close();
		}
	}
	
	/**
	 * @brief 报文接收后临时回调函数，触发报文解析模块。
	 *
	 * @param [in] message RTPS报文。
	 * @param [in] len 报文长度。
	 *
	 * @return 报文解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t InterpretMessage(char* message, uint32_t len);

protected:
	std::atomic_bool runFlag_;

	std::atomic_uint32_t flagNum_;

	//监听地址列表
	LocatorVec acceptLocators_;

	/* 传输类型 */
	LocatorKind kind_;

	/* 默认的IP地址，在加组中需要使用 */
	LocatorVec unicastLocatorList_;
	//接收缓冲区
	std::shared_ptr<MemoryPool> receivePool_;

	std::shared_ptr<MessageProcessor> messageProcessor_;
};

TRAVODDS_NAMESPACE_END

#endif