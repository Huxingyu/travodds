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
#ifndef TRAVODDS_RTPS_ENDPOINT_READER_H
#define TRAVODDS_RTPS_ENDPOINT_READER_H

#include "rtps/endpoint/endpoint.h"
#include "rtps/historycache/historycache.h"
#include "rtps/messages/cdrmessage_t.h"
#include "common/time/ddstimer.h"

TRAVODDS_NAMESPACE_BEGIN

class ReaderListener {
public:
	virtual void on_data_available(std::shared_ptr<CacheChange> change) = 0;
};

class DataReaderImpl;
struct DiscoveredWriterData;
/**
 * Reader类,管理从其匹配的Writer接收数据。
 * 需要使用来自RTPS层Participant的CreateReader函数创建。
 */
class Reader : public Endpoint {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] qos DataReader Qos配置。
	 * @param [in] kind 主题种类（NO_KEY/WITH_KEY）。
	 */
	Reader(const DataReaderQos& qos, const TopicKind_t& kind);
	/**
	 * @brief 析构函数。
	 */
	virtual ~Reader();

    // Serialize application selection/materialization/commit; ingress may continue.
    std::recursive_mutex sampleAccessMutex;
    // Uses this Reader's configured directory; discovery never supplies paths.
    bool StartDataSharing(const std::shared_ptr<Reader>& self, const DiscoveredWriterData& writer);
    bool DataSharingSessionChanged(const DiscoveredWriterData& writer);
    void StopDataSharing(const GUID_t& writer);
    void StopDataSharing();
    void StartMaintenance(const std::shared_ptr<Reader>& self);
    void MaintainHistory();
#if defined(__linux__)
    ReturnCode_t ReceiveNativeSample(const GUID_t& writer, const NativeTypeInfo& type,
        class SharedReadLoan&& loan);
#endif
    virtual ReturnCode_t SubmitDataChange(std::shared_ptr<CacheChange>) { return RETCODE_UNSUPPORTED; }

	/**
	 * @brief 处理传入的DATA消息。
	 *
	 * @param [in] headerMsg  RTPS消息参数。
	 * @param [in] dataMsg Data消息参数。
	 * @param [in] data 消息内容。
	 * @param [in] len 内容长度。
	 *
	 * @return 处理成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t ReceiveDataMessage(const Receive_t&  headerMsg, const DataSubMessage_t& dataMsg, const SerializedPayload& data) = 0;

	/**
	 * @brief 处理传入的DATA_FRAG消息。
	 *
	 * @param [in] headerMsg RTPS消息参数。
	 * @param [in] dataMsg Data消息参数。
	 * @param [in] data 消息内容。
	 * @param [in] len 内容长度。
	 *
	 * @return 处理成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t ReceiveDataFragMessage(const Receive_t&  headerMsg, const DataFragSubMessage_t& dataMsg, const SerializedPayload& data) = 0;

	/**
	 * @brief 处理传入的HEARTBEAT消息。
	 *
	 * @param [in] headerMsg RTPS消息参数。
	 * @param [in] heartMsg HEARTBEAT消息。
	 *
	 * @return 处理成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartMsg) = 0;

	/**
	 * @brief 处理传入的HEARTBEAT_FRAG消息。
	 *
	 * @param [in] headerMsg RTPS消息参数。
	 * @param [in] heartFragMsg HEARTBEAT_FRAG消息。
	 *
	 * @return 处理成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t ReceiveHeartFragMessage(const Receive_t&  headerInfo, const HeartBeatFragSubMessage_t& heartFragMsg) = 0;

	//ReturnCode_t ReceiveHeartbeatMessage(const Receive_t&  headerInfo, const HeartBeatSubMessage_t& heartInfo);

	/**
	 * @brief 处理传入的Gap消息。
	 *
	 * @param [in] headerMsg RTPS消息参数。
	 * @param [in] gapInfo Gap消息。
	 *
	 * @return 读取成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	virtual ReturnCode_t ReceiveGapMessage(const Receive_t&  headerInfo, const GapSubMessage_t& gapInfo) = 0;
	/* read 获取数据 */

	/**
	 * @brief 循环读取所有的实例的数据。
	 * 
	 * @param [in,out] dataValues     将返回接收到的数据样本的LoanableCollection对象。
     * @param [in,out] sampleInfos    将返回接收到的样本信息的SampleInfoSeq对象。
     * @param [in]     maxSamples     要返回的最大样本数。如果提供了特殊值LENGTH_UNLIMITED，
     *                                 则将返回尽可能多的样本，直到达到上述限制。
     * @param [in]     sampleStates   只有sampleStates与其中一个匹配的数据样本才会返回。
     * @param [in]     viewStates     只有viewStates与其中一个匹配的数据样本才会返回。
     * @param [in]     instanceStates 仅返回instanceStates与其中一个匹配的数据样本。
	 * @param [in]     shouldTake     获取后是否删除数据标志。
	 *
	 * @return 读取成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t ReadOrTake(std::vector<std::shared_ptr<CacheChange>>& dataValues, SampleInfoSeq& sampleInfos, uint32_t maxSamples,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake = false, bool preview = false);

	/**
	 * @brief 读取所有的实例的数据。
	 *
	 * @param [in,out] dataValues     将返回接收到的数据样本的LoanableCollection对象。
	 * @param [in,out] sampleInfos    将返回接收到的样本信息的SampleInfoSeq对象。
	 * @param [in]     maxSamples     要返回的最大样本数。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 则将返回尽可能多的样本，直到达到上述限制。
	 * @param [in]     handle         实例句柄。
	 * @param [in]     sampleStates   只有sampleStates与其中一个匹配的数据样本才会返回。
	 * @param [in]     viewStates     只有viewStates与其中一个匹配的数据样本才会返回。
	 * @param [in]     instanceStates 仅返回instanceStates与其中一个匹配的数据样本。
	 * @param [in]     shouldTake     获取后是否删除数据标志。
	 *
	 * @return 读取成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t ReadOrTakeInstance(std::vector<std::shared_ptr<CacheChange>>& dataValues, SampleInfoSeq& sampleInfos, uint32_t maxSamples, InstanceHandle_t handle,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake = false, bool preview = false);

	/**
	 * @brief 从列表中获取大于handle的实例句柄，
	 *        如果没有获取到说明没有其他数据。
	 *
	 * @param [in,out] dataValues     将返回接收到的数据样本的LoanableCollection对象。
	 * @param [in,out] sampleInfos    将返回接收到的样本信息的SampleInfoSeq对象。
	 * @param [in]     maxSamples     要返回的最大样本数。如果提供了特殊值LENGTH_UNLIMITED，
	 *                                 则将返回尽可能多的样本，直到达到上述限制。
	 * @param [in]     handle         实例句柄。
	 * @param [in]     sampleStates   只有sampleStates与其中一个匹配的数据样本才会返回。
	 * @param [in]     viewStates     只有viewStates与其中一个匹配的数据样本才会返回。
	 * @param [in]     instanceStates 仅返回instanceStates与其中一个匹配的数据样本。
	 * @param [in]     shouldTake     获取后是否删除数据标志。
	 *
	 * @return 读取成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t ReadOrTakeNextInstance(std::vector<std::shared_ptr<CacheChange>>& dataValues, SampleInfoSeq& sampleInfos, uint32_t maxSamples, InstanceHandle_t handle,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake = false, bool preview = false);

	/**
	 * @brief 获取下一个实例句柄 。
	 *
	 * @param [in]     preHander		 实例句柄标志。
	 * @param [in,out] instanceHandle    下一个实例句柄。
	 *
	 * @return 查找成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t FindNextInstance(const InstanceHandle_t& preHander,InstanceHandle_t& instanceHandle);

	/**
	 * @brief 新CacheChange对象生成与赋值。
	 *
	 * @param [in]	   kind                报文状态标识。
	 * @param [in]     data                数据报文。
	 * @param [in]     handle              实例句柄。
	 * @param [in]     pubHandle		   本地标识修改实例的DataWriter句柄。
	 * @param [in]     sourceTimespace     报文源时间戳。
	 *
	 * @return 生成的CacheChange指针。
	 */
	std::shared_ptr<CacheChange> new_change(ChangeKind_t kind, SerializedBuffer* data, const InstanceHandle_t& handle, const InstanceHandle_t& pubHandle, const Time_t& sourceTimespace);

	/**
	 * @brief 设置Reader监听者。
	 *
	 * @param [in] listener  监听者指针。
	 */
	void SetReaderListener(ReaderListener* listener);

	/**
	 * @brief 添加新的CacheChange对象。
	 *
	 * @param [in]     cache		 新的CacheChange对象指针。
	 *
	 * @return 添加成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t AddChanges(std::shared_ptr<CacheChange> cache);

	/**
	 * @brief 处理 InlineQos 参数。
	 *
	 * @param [in]  kind	  报文状态标识。
	 * @param [out] handle	  实例句柄。
	 * @param [in]  inlineQos inlineQos列表。
	 *
	 * @return 处理成功返回RETCODE_OK，否则返回RETCODE_ERROR。
	 */
	ReturnCode_t ProcessInlineQos(ChangeKind_t& kind, InstanceHandle_t& handle, const ParameterList_t& inlineQos);

	/* 判断是否为可靠传输端点 */

	/**
	 * @brief 判断是否为可靠传输端点。
	 * 
	 * @return 是返回true，否则返回false。
	 */
	bool IsReliable();

	/**
	 * @brief 获取DataReaderImpl类指针。
	 *
	 * @return 返回DataReaderImpl类指针。
	 */
	DataReaderImpl* GetRelatedDataReader();
	/**
	 * @brief 设置DataReaderImpl类指针。
	 *
	 * @param [in] reader DataReaderImpl类指针。
	 */
	void SetRelatedDataReader(DataReaderImpl* reader);

    // Hold the ingress barrier until the DCPS status callback has returned.
    // A retained RTPS Reader alone does not keep its DCPS owner alive.
    void NotifySubscriptionMatched(InstanceHandle_t handle, bool add = true);

	/**
	 * @brief 获取DataReader Qos配置信息。
	 *
	 * @return 返回DataReader Qos配置信息。
	 */
	const DataReaderQos& GetQos();

	virtual void SetQos(const DataReaderQos& qos);

	/**
	 * @brief 判定一个特定的Writer是否与当前Reader已匹配。
	 *
	 * @param [in] writerGuid writer Guid。
	 * 
	 * @return 匹配返回true，否则返回false。
	 */
	virtual bool MatchedWriterIsMatched(GUID_t writerGuid) = 0;

	/**
	 * @brief 获取与当前数据写者匹配的发布者实例句柄。
	 *
	 * @param [in] publication_handles 发布者实例句柄列表。
	 */
    void GetMatchedPublications(InstanceHandleSeq& publication_handles) const;

	/**
	 * @brief 根据与当前数据写者匹配的发布者实例句柄获取其配置参数。
	 *
	 * @param [in] subscriptionData 发布者实例句柄对应的发布者通信配置参数。
	 * @param [in] remotehandle     远端发布者实例句柄。
	 *
	 * @return 获取成功返回true，否则返回false。
	 */
	bool GetMatchedPublicationData(PublicationBuiltinTopicData& publicationData, const InstanceHandle_t& remotehandle) const;

	/**
	 * @brief 获取数据读者缓冲区。
	 *
	 * @return 返回数据读者缓冲区指针。
	 */
	std::shared_ptr<HistoryCache> GetReaderCache();

protected:
	Duration_t heartbeatResponseDelay_;
	Duration_t heartbeatSuppressionDuration_;
	std::shared_ptr<HistoryCache> readerCache_;
	bool expectsInlineQos_;

	/* jd 保存样本的实例信息,并且大小进行排序 */
	std::list<InstanceHandle_t> instanceHandles_;

	/* 接收到的数据信息的最新的数据序号  */
	SequenceNumber_t lastSequenceNumber_;

	ReaderListener* readerListener_;
	
	DataReaderQos qos_;

	DataReaderImpl* datareader_;

	/* wangyi 最新的样本 */
	std::shared_ptr<Timer> lastNewCacheTimer_;
    std::shared_ptr<Timer> maintenanceTimer_;
	InstanceHandle_t lastInstance_;
	mutable std::mutex lastInstanceMtx_;

	/* 报文处理锁 */
	std::recursive_mutex dataMutex_;
	std::mutex hbMutex_;
	std::mutex gapMutex_;

    struct NativeConnection;
    std::mutex nativeConnectionsMutex_;
    std::map<GUID_t, std::shared_ptr<NativeConnection>> nativeConnections_;
    bool nativeStopping_ = false;
	friend class DataReaderImpl;
};

TRAVODDS_NAMESPACE_END

#endif
