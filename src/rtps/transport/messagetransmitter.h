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
#ifndef AGIDDS_RTPS_TRANSPORT_MESSAGETRANSMITTER_H
#define AGIDDS_RTPS_TRANSPORT_MESSAGETRANSMITTER_H

#include <chrono>
#include <thread>

#include "type/basictype.h"
#include "rtps/basictypertps.h"
#include "rtps/transport/asiocontext.h"
#include "dcps/base/retcode.h"
#include "common/memory/memorypool.h"

AGIDDS_NAMESPACE_BEGIN

class MessageProcessor;

class MessageTransmitter
{
public:
	//构造函数
	MessageTransmitter(std::shared_ptr<MessageProcessor> messageProcessor)
		: messageProcessor_(messageProcessor), receivePool_(new MemoryPool(64)), runFlag_(true), flagNum_(0), kind_(LOCATOR_PORT_INVALID)
	{
	}

	//析构函数
	virtual ~MessageTransmitter()
	{
	};

	virtual ReturnCode_t SendAddrAdd(Locator_t source, Locator_t destination, uint16_t priority = -1) = 0;

	//发送 虚函数提供接口
	virtual ReturnCode_t MessageSend(
		Locator_t source,
		Locator_t destination,
		char* message,
		uint32_t length,
		uint16_t priority = -1) = 0;

	virtual ReturnCode_t MessageSend(
		LocatorVec& sourceList,
		LocatorVec& destinationList,
		char* message,
		uint32_t length,
		uint16_t priority = -1
	);


	//添加监听地址
	virtual ReturnCode_t ReceiveAddrAdd(Locator_t receiveAddr) = 0;

	//添加监听多播地址
	//virtual ReturnCode_t ReceiveMultiAddrAdd(Locator_t sourceAddr, Locator_t receiveAddr) = 0;

	//删除监听地址
	virtual ReturnCode_t ReceiveAddrRemove(Locator_t receiveAddr) = 0;

	bool IsRun();

	void SetRun();

	bool IsStop();

	virtual void SetStop();

	uint32_t GetTransmitterKind();

	void SetUnicastLocatorList(const LocatorVec& locatorlist);

	/* 检测端口号是否被占用 端口号不解决 */
	static bool CheckSocketPort(uint16_t port, bool checkUdp = true);

	template <typename Socket>
	static void CloseSocket(std::shared_ptr<Socket> fd) {
		if (fd) {
			fd->close();
		}
	}

protected:
	std::atomic_bool runFlag_;

	std::atomic_uint32_t flagNum_;

	//监听地址列表
	LocatorVec acceptLocators_;

	/* 传输类型 */
	uint32_t kind_;

	/* 默认的IP地址，在加组中需要使用 */
	LocatorVec unicastLocatorList_;
	//接收缓冲区
	std::shared_ptr<MemoryPool> receivePool_;

	std::shared_ptr<MessageProcessor> messageProcessor_;
};

AGIDDS_NAMESPACE_END

#endif