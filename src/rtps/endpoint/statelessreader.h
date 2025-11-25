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
#ifndef TRAVODDS_RTPS_ENDPOINT_STATELESSREADER_H
#define TRAVODDS_RTPS_ENDPOINT_STATELESSREADER_H

#include "rtps/endpoint/reader.h"
#include "rtps/endpoint/writerproxy.h"

TRAVODDS_NAMESPACE_BEGIN

//struct RemoteWriterData {
//	GUID_t writerGuid;
//	OwnershipQosPolicyKind ownershipKind;
//	int32_t ownership_strength;
//	LivelinessQosPolicyKind livelinessKind;
//	Duration_t lease_duration;
//
//	Duration_t recvTime;
//};
/**
 * StatelessReader类,Reader类的子类，管理尽力传输模式下从其匹配的Writer接收数据。
 * 需要使用来自RTPS层Participant的CreateReader函数创建。
 */
class StatelessReader : public Reader {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] qos  DataReader Qos配置。
	 * @param [in] guid DataReader Guid。
	 * @param [in] kind 主题种类（NO_KEY/WITH_KEY）。
	 */
	StatelessReader(const DataReaderQos& qos, const GUID_t& guid, const TopicKind_t& kind);

	/**
	 * @brief 析构函数。
	 */
	virtual ~StatelessReader();

	/**
	 * @brief DATA报文参数解析，并将用户报文存储到CacheChange中等待回传到用户层。
	 *
	 * @param [in] headerMsg  报文头参数。
	 * @param [in] dataMsg	  DATA子报文参数。
	 * @param [in] data       DATA子报文内容。
	 * @param [in] len        DATA子报文长度。
	 *
	 * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t ReceiveDataMessage(const Receive_t&  headerMsg, const DataSubMessage_t& dataMsg, SerializedPayload data, uint32_t len);

	/**
	 * @brief DATA分片报文参数解析和DATA分片报文合并，并将合并后的用户报文存储到CacheChange中等待回传到用户层。
	 *
	 * @param [in] headerMsg  报文头参数。
	 * @param [in] dataMsg	  DataFrag子报文参数。
	 * @param [in] data       DataFrag子报文内容。
	 * @param [in] len        DataFrag子报文长度。
	 *
	 * @return 解析成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t ReceiveDataFragMessage(const Receive_t&  headerMsg, const DataFragSubMessage_t& dataMsg, SerializedPayload data, uint32_t len);

	/**
	 * 未实现
	 * @brief Heartbeat报文参数解析和并根据报文内容判断是否存在丢包，将报文接收情况进行分析并控制ACK报文的构建与发送。
	 *
	 * @param [in] headerInfo  报文头参数。
	 * @param [in] heartMsg	  Heartbeat子报文参数。
	 *
	 * @return RETCODE_UNSUPPORTED。
	 */
	ReturnCode_t ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartMsg);

	/**
	 * 未实现
	 * @brief HeartbeatFrag报文参数解析和并根据报文内容判断是否存在丢包，将报文接收情况进行分析并控制nack报文的构建与发送。
	 *
	 * @param [in] headerInfo  报文头参数。
	 * @param [in] heartFragMsg	  HeartbeatFrag子报文参数。
	 *
	 * @return RETCODE_UNSUPPORTED。
	 */
	ReturnCode_t ReceiveHeartFragMessage(const Receive_t&  headerInfo, const HeartBeatFragSubMessage_t& heartFragMsg);

	//ReturnCode_t ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartInfo);
	/**
	 * @brief Gap报文参数解析及报文有效性判断，报文有效则进行参数信息转换。
	 *  根据Gap报文参数设置报文序列号相关性。
	 *
	 * @param [in] headerInfo  报文头参数。
	 * @param [in] gapInfo	  Gap子报文参数。
	 *
	 * @return RETCODE_UNSUPPORTED。
	 */
	ReturnCode_t ReceiveGapMessage(const Receive_t&  headerInfo, const GapSubMessage_t& gapInfo);

	/**
	 * @brief 添加与当前Reader匹配的远端writer代理。
	 *
	 * @param [in] writerProxy  远端writer代理指针。
	 */
	void MatchedWriterAdd(std::shared_ptr<WriterProxy> writerData);

	/**
	 * @brief 根据GUID判断当前Reader是否与该GUID对应的远端writer代理匹配。
	 *
	 * @param [in] writerGuid  远端writer代理GUID。
	 *
	 * @return 远端writer代理指针。
	 */
	bool MatchedWriterIsMatched(GUID_t writerGuid);

	/**
	 * @brief 删除与当前Reader匹配的远端writer代理。
	 *
	 * @param [in] writerProxy  远端writer代理指针。
	 */
	void MatchedWriterRemove(GUID_t writerGuid);

	/**
	 * @brief 根据GUID查找与当前Reader匹配的远端writer代理。
	 *
	 * @param [in] writerGuid  远端writer代理GUID。
	 *
	 * @return 远端writer代理指针。
	 */
	std::shared_ptr<WriterProxy> MatchedWriterLookup(GUID_t writerGuid);

	/**
	 * @brief 更新远端writer活性。
	 *
	 * @param [in] writer  远端writer GUID。
	 *
	 * @return 更新成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	void AssertWriterLiveliness(const GUID_t & guid);

#ifdef DDSSECURITY
	std::map<GUID_t, std::shared_ptr<WriterProxy>>* GetMatchWriters() {
		return &matchWriters_;
	}
#endif // DDSSECURITY
private:
	std::map<GUID_t, std::shared_ptr<WriterProxy>> matchWriters_;

	std::map<GUID_t, std::shared_ptr<CacheChange>> dataFragCaches_;
};

TRAVODDS_NAMESPACE_END

#endif // !TRAVODDS_RTPS_ENDPOINT_STATELESSREADER_H
