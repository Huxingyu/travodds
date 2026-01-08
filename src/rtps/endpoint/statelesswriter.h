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
#ifndef TRAVODDS_RTPS_ENDPOINT_STATELESSWRITER_H
#define TRAVODDS_RTPS_ENDPOINT_STATELESSWRITER_H

#include  "rtps/endpoint/writer.h"
#include  "rtps/endpoint/readerlocator.h"
#ifdef DDSSECURITY
#include "rtps/endpoint/readerproxy.h"
#endif

TRAVODDS_NAMESPACE_BEGIN
class Timer;

/**
 * StatelessWriter类，Writer子类,管理使用尽力传输模式向匹配的Reader发送的数据。
 * 需要使用来自RTPS层Participant的CreateWriter函数创建。
 */
class StatelessWriter : public Writer {

public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] qos  DataWriter Qos配置。
	 * @param [in] guid DataWriter GUID配置。
	 * @param [in] kind 主题种类（NO_KEY/WITH_KEY）。
	 */
	StatelessWriter(const DataWriterQos& qos, const GUID_t& guid, const TopicKind_t& kind);

	/**
	 * @brief 析构函数。
	 */
	virtual ~StatelessWriter();

	/**
	 * @brief 添加与当前Writer匹配的远端Reader参数信息。
	 *
	 * @param [in] subData  远端Reader配置参数。
	 */
	void MatchedReaderAdd(const DiscoveredReaderData &subData);
#ifdef DDSSECURITY
	void MatchedReaderAdd(const DiscoveredReaderSecureData& subData);
#endif // DDSSECURITY


	/**
	 * @brief 本地添加与当前Writer匹配的远端Reader通信信息。
	 *
	 * @param [in] locator  远端Reader通信信息。
	 */
	void ReaderLocatorAdd(Locator_t& locator, const GUID_t& guid = GUID_UNKNOWN);

	/**
	 * @brief 本地删除与当前Writer匹配的远端Reader通信信息。
	 *
	 * @param [in] locator  远端Reader通信信息。
	 */
	void ReaderLocatorRemove(Locator_t& locator, const GUID_t& guid);
	void ReaderLocatorRemove(const GUID_t& guid);

	/**
	 * @brief 对未发送的请求信息进行重整操作。
	 */
	void UnsentChangesReset();
	/**
     * @brief Data报文封装控制及发送控制函数。
     *
     * @param [in] change	  Data报文发送请求句柄。
     *
     * @return 发送成功返回RETCODE_OK，否则返回RETCODE_ERROR。
     */
	virtual ReturnCode_t DeliveryData(std::shared_ptr<CacheChange> change, const Duration_t& timeOut);

	/**
	 * @brief 设置重发数据周期，创建定时器用于周期性发送history中的数据。
	 *
	 * @param [in] period	  重发数据周期。
	 */
	void SetResendDataPeriod(Duration_t period);

	/**
	 * @brief 周期性发送history中的数据。
	 */
	void ResendDataPeriod();

	/**
	 * 功能未实现
	 * @brief Ack报文接收处理函数。
	 *
	 * @param [in] headerMsg  Ack报文参数。
	 * @param [in] ackMsg	  Ack子报文参数。
	 *
	 * @return 返回RETCODE_UNSUPPORTED。
	 */
	virtual ReturnCode_t ReceiveAckNackMessage(const Receive_t&  headerMsg, const AcknackSubMessage_t& ackMsg);

	/**
	 * 功能未实现
	 * @brief NAck报文接收处理函数。
	 *
	 * @param [in] headerMsg  NAck报文参数。
	 * @param [in] nackMsg	  NAck子报文参数。
	 *
	 * @return 返回RETCODE_UNSUPPORTED。
	 */
	virtual ReturnCode_t ReceiveNackFragMessage(const Receive_t&  headerMsg, const NackFragSubMessage_t& nackMsg);

#ifdef DDSSECURITY
	/**
	 * @brief 加密子消息。
	 *
	 * @return 返回0表示加密成功，其余失败。
	 */
	int EncodeSubMsgBuffer(std::shared_ptr<ReaderProxy> writerProxy, SerializedBuffer& buffer, SerializedBuffer& encodeBuffer);
#endif // DDSSECURITY

private:

	Duration_t resendDataPeriod_;

	std::map<GUID_t, std::shared_ptr<ReaderLocator>> readerLocators_;
	std::map<Locator_t, std::set<GUID_t>> readerMulticastLocators_;
	std::mutex readerLocatorsMutex_;

	std::shared_ptr<Timer> timer_;
};

TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_RTPS_ENDPOINT_STATELESSWRITER_H
