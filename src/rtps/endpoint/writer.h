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
#ifndef TRAVODDS_RTPS_ENDPOINT_WRITER_H
#define TRAVODDS_RTPS_ENDPOINT_WRITER_H

#include "rtps/endpoint/endpoint.h"
#include "rtps/historycache/historycache.h"
#include "rtps/messages/cdrmessage_t.h"
TRAVODDS_NAMESPACE_BEGIN

class FlowController;
class DataWriterImpl;
/**
 * Writer类,管理向匹配的Reader发送数据。
 * 需要使用来自RTPS层Participant的CreateWriter函数创建。
 */
class Writer : public Endpoint {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] qos DataWriter Qos配置。
	 * @param [in] kind 主题种类（NO_KEY/WITH_KEY）。
	 */
	Writer(const DataWriterQos& qos, const TopicKind_t& kind);
	
	/**
	 * @brief 析构函数。
	 */
	virtual ~Writer();

	/**
	 * @brief 创建新的报文缓存空间。
	 *
	 * @param [in] kind      缓存状态。
	 * @param [in] data      报文指针。
	 * @param [in] handle    报文句柄。
	 * @param [in] timespace 报文时间戳。
	 *
	 * @return 创建的新缓存。
	 */
	std::shared_ptr<CacheChange> NewChange(ChangeKind_t kind, SerializedBuffer* data, InstanceHandle_t handle, const Time_t& timespace);

	/**
	 * @brief 添加新的报文缓存空间到HistoryCache管理容器中。
	 * 添加cache需要考虑的QoS有ResourceLimitsQosPolicy和HistoryQosPolicy
	 *
	 * ResourceLimitsQosPolicy
	 * 1. 添加的cache数据的instance对应的数字是否满足 resource_limit.max_sample_per_instance;
	 * 2. 添加的cache数据是否超过resource_limit.max_instance的设置
	 * 3. 添加的cache数据是否超过qos_.resource_limits.max_samples，或者history.depth的设置;
	 * HistoryQosPolicy
	 * 1.keep_last  保存最新的数据
	 * 2.keep_all   保存所有的数据
	 *	
	 * TODO 内置主题是否满足下方逻辑
	 * 测试rti，测试fast
	 * 1.keep_last的情况下,
 	 *	1.1 instance的个数 > max_instance的在上一层发送数据失败
	 *	1.2 对应的instance的个数 > depth， 删除最旧的数据
	 *	1.3 样本总数 > max_sample，删除最旧的数据。 max_sample = max_sample_per_instance * max_instance
	 *	不满足1.1 、 1.2的条件，必然样本总数必然满足: 样本总数 < max_sample
	 * 2.keep_all的情况下
	 *	2.1 不满足max_instance的在上一层发送数据失败
	 *	2.2 不满足max_sample_per_instance,
	 *			RELIABLE模式进行等待，等待时间到达删除数据
	 *			BEST_EFFORT模式删除最旧的数据
	 *	2.3 不满足max_sample，进行等待等待时间到达返回失败。 max_sample = max_sample_per_instance * max_instance
	 *	不满足2.1 、 2.2的条件, 必然样本总数必然满足:样本总数 < max_sample
	 * 
	 * @param [in] cacheChange 新缓存。
	 * 
	 * @return 添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t AddNewCachce(std::shared_ptr<CacheChange> cacheChange);

	/**
	 * @brief 等待确认。
	 *  虚函数，具体功能在子类中根据需要实现。
	 *
	 * @param [in] max_wait 最大等待时间。
	 *
	 * @return RETCODE_OK。
	 */
	virtual ReturnCode_t WaitForAcknowledgments(const Duration_t& max_wait);

	/**
	 * @brief 是否全部确认。
	 *  虚函数，具体功能在子类中根据需要实现。
	 *
	 * @param [in] changeSeqNum 序列号。
	 *
	 * @return true。
	 */
	virtual bool IsAckedByAll(SequenceNumber_t changeSeqNum);
	
	/**
	 * @brief 获取pushMode_变量值。
	 *
	 * @return pushMode_值。
	 */
	bool IsPushMode();

	/**
	 * @brief 获取HistoryCache缓存指针。
	 *
	 * @return writerCache_指针。
	 */
	std::shared_ptr<HistoryCache> GetWriteCache();

	/**
	 * @brief 数据筛选器。
	 * 
	 * @param [in] change 数据缓存。
	 *
	 * @return true。
	 */
	bool DataFilter(std::shared_ptr<CacheChange> change);
	/**
     * @brief 数据传输。
     *  虚函数，具体功能在子类中根据需要实现。
     *
     * @param [in] change 数据缓存。
     *
     * @return RETCODE_UNSUPPORTED。
     */
	virtual ReturnCode_t DeliveryData(std::shared_ptr<CacheChange> change, bool flowControl = false);

	/**
	 * @brief 数据传输。
	 *  虚函数，具体功能在子类中根据需要实现。
	 *
	 * @param [in] change 数据缓存。
	 *
	 * @return RETCODE_UNSUPPORTED。
	 */
	void SetRelatedDataWriter(DataWriterImpl* datawriter);

	/**
	 * @brief 获取DataWriterImpl指针。
	 *
	 * @return DataWriterImpl指针。
	 */
	DataWriterImpl* GetRelatedDataWriter();

	/**
	 * @brief 添加与当前Writer匹配的远端Reader。
	 *  虚函数，具体功能在子类中根据需要实现。
	 *
	 * @param [in] subData 远端Reader配置信息。
	 */
	virtual void MatchedReaderAdd(const DiscoveredReaderData &subData) = 0;
#ifdef DDSSECURITY
	virtual void MatchedReaderAddWithSecurity(const DiscoveredReaderSecureData& subData, bool isSecure) = 0;
#endif // DDSSECURITY

	/**
	 * @brief 接收Ack报文。
	 *  虚函数，具体功能在子类中根据需要实现。
	 *
	 * @param [in] headerMsg 报文头参数。
	 * @param [in] ackMsg    ACK报文参数。
	 */
	virtual ReturnCode_t ReceiveAckNackMessage(const Receive_t&  headerMsg, const AcknackSubMessage_t& ackMsg) = 0;

	/**
	 * @brief 接收NAck报文。
	 *  虚函数，具体功能在子类中根据需要实现。
	 *
	 * @param [in] headerMsg  报文头参数。
	 * @param [in] nackMsg    NACK报文参数。
	 */
	virtual ReturnCode_t ReceiveNackFragMessage(const Receive_t&  headerMsg, const NackFragSubMessage_t& nackMsg) = 0;

	/**
	 * @brief 获取DataWriter Qos配置。
	 *
	 * @return DataWriter Qos配置。
	 */
	DataWriterQos& GetQos();

	/**
	 * @brief 添加内置缓存。
	 *
	 * @param [in] cacheChange 内置缓存。
	 * 
	 * @return 添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t AddBuiltinCache(std::shared_ptr<CacheChange> cacheChange);

	/**
	 * @brief 获取历史缓存。
	 *
	 * @return HistoryCache指针。
	 */
	std::shared_ptr<HistoryCache> GetHistoryCache();

	/**
	 * @brief 删除与输入的句柄对应的缓存。
	 *
	 * @param [in] handle 缓存句柄。
	 */
	void RemoveCacheByHandle(const InstanceHandle_t& handle);

	/**
	 * @brief 获取与当前数据读者匹配的订阅者实例句柄。
	 *
	 * @param [in] subHandles 订阅者实例句柄列表。
	 */
	void GetMatchedSubscriptions(InstanceHandleSeq& subHandles) const;

	/**
	 * @brief 根据与当前数据读者匹配的订阅者实例句柄获取其配置参数。
	 *
	 * @param [in] subscriptionData 订阅者实例句柄对应的订阅者通信配置参数。
	 * @param [in] remotehandle     远端订阅者实例句柄。
	 * 
	 * @return 获取成功返回true，否则返回false。
	 */
	bool GetMatchedSubscriptionData(SubscriptionBuiltinTopicData& subscriptionData, const InstanceHandle_t& remotehandle) const;

	void SetFlowController(FlowController* controller);
	FlowController* GetFlowController ();

	/**
	 * @brief 获取数据写者缓冲区。
	 *
	 * @return 返回数据写者缓冲区指针。
	 */
	std::shared_ptr<HistoryCache> GetWriterCache();
	
#ifdef DDSSECURITY
	 void ResetLastChangeSequenceNumber();
#endif // DDSSECURITY
private:
    /* 写入锁  */
    mutable std::mutex wirterMtx_;
protected:

	bool pushMode_ = true;
	Duration_t nackResponseDelay_;
	Duration_t heartBeatPeriod_;
	Duration_t nackSuppressionDuration_;
	SequenceNumber_t lastChangeSequenceNumber_;
	DataWriterQos qos_;

	std::shared_ptr<HistoryCache> writerCache_;
	std::map<InstanceHandle_t, bool> matchHandles_;
	DataWriterImpl* datawriter_ = nullptr;
	FlowController* flowController_;
};

TRAVODDS_NAMESPACE_END

#endif
