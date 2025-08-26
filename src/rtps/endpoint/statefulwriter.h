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
#pragma once
#include <condition_variable>
#include "rtps/endpoint/writer.h"
#include "rtps/endpoint/readerproxy.h"

AGIDDS_NAMESPACE_BEGIN

class Timer;

/**
 * StatefulWriter类，Writer子类,管理使用可靠传输模式向匹配的Reader发送的数据。
 * 需要使用来自RTPS层Participant的CreateWriter函数创建。
 */
class StatefulWriter : public Writer {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] qos  DataWriter Qos配置。
	 * @param [in] guid DataWriter GUID配置。
	 * @param [in] kind 主题种类（NO_KEY/WITH_KEY）。
	 */
	StatefulWriter(const DataWriterQos& qos, const GUID_t& guid, const TopicKind_t& kind);

	/**
	 * @brief 析构函数。
	 */
	virtual ~StatefulWriter();

	/**
	 * @brief 添加与当前Writer匹配的远端Reader参数信息。
	 *
	 * @param [in] subData  远端Reader配置参数。
	 */
	virtual void MatchedReaderAdd(const DiscoveredReaderData &subData);

	/**
	 * @brief 添加与当前Writer匹配的远端Reader代理。
	 *
	 * @param [in] readerProxy  远端Reader代理指针。
	 */
	void MatchedReaderAdd(std::shared_ptr<ReaderProxy> readerProxy);

	/**
	 * @brief 删除与当前writer匹配的远端Reader代理。
	 *
	 * @param [in] ReaderProxy  远端Reader代理指针。
	 */
	void MatchedReaderRemove(std::shared_ptr<ReaderProxy> readerProxy);

	/**
	 * @brief 根据GUID查找与当前Reader匹配的远端writer代理。
	 *
	 * @param [in] writerGuid  远端writer代理GUID。
	 *
	 * @return 远端writer代理指针。
	 */
	std::shared_ptr<ReaderProxy> MatchedReaderLookup(GUID_t readerGuid);

	/**
	 * @brief 可靠传输发送请求等待确认函数。
	 * 在规定的等待时间内确认当前发送报文是否被Reader接收，根据情况更改报文发送请求状态。
	 *
	 * @param [in] max_wait  报文请求确认最大等待时间。
	 *
	 * @return 处理成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t WaitForAcknowledgments(const Duration_t &max_wait) override;

	/**
	 * @brief 根据writerCache_中发送请求序列号所有发送请求是否得到数据读者确认。
	 */
	bool CheckAllAcked();

	/**
	 * @brief 判决可靠传输发送请求是否已全部确认函数。
	 * 循环对比确认当前序列号对应的发送报文请求是否已被所有对应的Reader接收成功并回复有Ack报文。
	 *
	 * @param [in] changeSeqNum  报文请求序列号。
	 *
	 * @return 已全部确认返回true，否则返回false。
	 */
	virtual bool IsAckedByAll(SequenceNumber_t changeSeqNum);

	/**
	 * @brief Data报文封装控制及发送控制函数。
	 *
	 * @param [in] change	  Data报文发送请求句柄。
	 *
	 * @return 发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t DeliveryData(std::shared_ptr<CacheChange> change, bool flowControl = false);

	/**
	 * @brief Ack报文接收处理函数。
	 *
	 * @param [in] headerMsg  Ack报文参数。
	 * @param [in] ackMsg	  Ack子报文参数。
	 *
	 * @return 处理成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t ReceiveAckNackMessage(const Receive_t& headerMsg, const AcknackSubMessage_t& ackMsg);

	/**
	 * @brief NAck报文接收处理函数。
	 *
	 * @param [in] headerMsg  NAck报文参数。
	 * @param [in] nackMsg	  NAck子报文参数。
	 *
	 * @return 返回RETCODE_UNSUPPORTED。
	 */
	virtual ReturnCode_t ReceiveNackFragMessage(const Receive_t& headerMsg, const NackFragSubMessage_t& nackMsg);
	/**
 * @brief Data报文封装控制及发送控制函数。
 *
 * @param [in] change		  Data报文发送请求句柄。
 * @param [in] readerProxys	  Data报文接收的DataReader句柄。
 *
 * @return 发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
 */
	ReturnCode_t DeliveryDataToReader(std::shared_ptr<CacheChange> change, const std::vector<std::shared_ptr<ReaderProxy>>& readerProxys, bool flowControl);

	/**
	 * @brief Heartbeat报文发送控制函数。
	 *
	 * @return 发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t WriteHeartbeat();

	/**
	 * @brief livelinessQos对应的Heartbeat报文发送控制函数。
	 *
	 * @param [in] liveliness livelinessQos是否启用标志位。
	 * 
	 * @return 发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t WriteHeartbeat(bool liveliness);

	/**
	 * @brief Heartbeat报文参数赋值、封装控制函数。
	 *
	 * @param [in] readerProxy Heartbeat报文接收Reader句柄。
	 * @param [in] force	   Heartbeat强制回复标志位。
	 * @param [in] liveliness  livelinessQos是否启用标志位（默认值：false）。
	 *
	 * @return 发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t WriteHeartbeat(std::shared_ptr<ReaderProxy> readerProxy, bool force, bool liveliness = false);

	/**
	 * 暂未实现
	 * @brief HeartbeatFrag报文参数赋值、封装控制函数。
	 *
	 * @param [in] readerProxy Heartbeat报文接收Reader句柄。
	 *
	 * @return RETCODE_UNSUPPORTED。
	 */
	ReturnCode_t WriteHeartFragbeat(std::shared_ptr<ReaderProxy> readerProxy);
	/**
 * @brief 根据报文发送请求需重传序列号列表，发送需要重传的Data报文。
 *
 * @param [in] readerProxy 报文接收Reader句柄。
 *
 * @return 发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
 */
	ReturnCode_t ReSendData(std::shared_ptr<ReaderProxy> readerProxy, bool flowControl = false);
	/**
 * @brief 根据分片报文发送请求需重传序列号列表，重新封装并发送需要重传的分片报文数据。
 *
 * @param [in] readerProxy 报文接收Reader句柄。
 * @param [in] FragSet     NackFrag子报文参数。
 *
 * @return 发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
 */
	/* 重传分片数据 */
	ReturnCode_t ReSendDataFrag(std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& FragSet, bool flowControl = false);
private:
	/**
	 * @brief 根据输入的报文发送请求生成ChangeForReader类对象。
	 *
	 * @param [in] change 报文发送请求。
	 *
	 * @return ChangeForReader类对象。
	 */
	ChangeForReader CreateChangeForReader(std::shared_ptr<CacheChange> change);

	/**
	 * @brief 移除所有的已经确认的数据。
	 */
	void RemoveAllAckedData();

	/**
	 * @brief Gap报文封装、发送函数。
	 *
	 * @param [in] seqVec 数据因为某种原因被删除，需要发送Gap报文的序号列表。
	 * @param [in] proxy  报文接收Reader句柄。
	 *
	 * @return 发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	void SendGap(const SeqNumberVec& seqVec, std::shared_ptr<ReaderProxy>);

private:
	std::map<GUID_t, std::shared_ptr<ReaderProxy>> matchedReaders_;

	std::shared_ptr<Timer> timer_;

	std::mutex readerPorxyMutex_;

    std::mutex allAckedMtx_;
	std::condition_variable condAllAcked_;

};

AGIDDS_NAMESPACE_END
