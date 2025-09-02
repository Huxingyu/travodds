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

#include "rtps/endpoint/statefulwriter.h"
#include "rtps/endpoint/statefulreader.h"
#include "rtps/discovery/discoverydata.h"
#include "dcps/publish/datawriterimpl.h"
#include "dcps/subscribe/datareaderimpl.h"
#include "rtps/builtin/liveliness/livelinessassert.h"

TRAVODDS_NAMESPACE_BEGIN

/**
 * @brief LivelinessManager类，实现规范中描述的Liveliness Qos管理功能。
 */
class LivelinessManager : public ReaderListener {
public:

	/**
	 * @brief 构造函数。
	 *
	 * @param [in] participant Participant指针。
	 */
	LivelinessManager(Participant* participant);

	/**
	 * @brief 析构函数。
	 */
	virtual ~LivelinessManager();

	/**
	 * @brief 发现新节点时，为内置主题添加代理。
	 *
	 * @param [in] data 新节点信息。
	 */
	void AddParticipantMessageProxy(const SPDPdiscoveredParticipantData & data);

	/**
	 * @brief 已发现节点退出时，为内置主题删除代理。
	 *
	 * @param [in] guid 退出节点guid。
	 */
	void RemoveParticipantMessageProxy(GUID_t guid);

	/**
	 * @brief 添加本地writer，根据qos设置定时器。
	 *
	 * @param [in] writerPtr writer指针。
	 * @param [in] dwQos Liveliness Qos配置信息。
	 * 
	 * @return 添加成功返回true，否则返回false。
	 */
	bool AddLivelinessWriter(std::shared_ptr<Writer> writerPtr, const LivelinessQosPolicy & dwQos);

	 /**
	  * @brief 删除本地writer。
	  *
	  * @param [in] writerPtr writer指针。
	  *
	  * @return 删除成功返回true，否则返回false。
	  */
	bool RemoveLivelinessWriter(std::shared_ptr<Writer> writerPtr);

	/**
	 * @brief 添加本地reader。
	 *
	 * @param [in] readerPtr reader指针。
	 * @param [in] drQos Liveliness Qos配置信息。
	 *
	 * @return 添加成功返回true，否则返回false。
	 */
	bool AddLivelinessReader(std::shared_ptr<Reader> readerPtr, const LivelinessQosPolicy & drQos);


	/**
	 * @brief 删除本地reader。
	 *
	 * @param [in] readerPtr reader指针。
	 *
	 * @return 删除成功返回true，否则返回false。
	 */
	bool RemoveLivelinessReader(std::shared_ptr<Reader> readerPtr);

	/**
	 * @brief 定时器函数，在AUTOMATIC_LIVELINESS_QOS模式下定时发送Data(m)报文。
	 *
	 * @return 发送成功返回true，否则返回false。
	 */
	bool AutoLivelinessChecker();


	/**
	 * @brief 定时器函数，在MANUAL_BY_PARTICIPANT_LIVELINESS_QOS模式下定时给活跃节点发送Data(m)报文。
     *
	 * @return 发送成功返回true，否则返回false。
	 */
	bool ParticipantLivelinessChecker();

	/**
	 * @brief 构建封装并发送Data(m)报文。
	 *
	 * @return 发送成功返回true，否则返回false。
	 */
	bool SendLivelinessMessage(const InstanceHandle_t & instance);

	/**
	 * @brief 关联本地写者内置主题。
	 * 
	 * @param [in] dataWriter DataWriterImpl类指针。
	 */
	void SetParticipantMessageDataWriter(std::shared_ptr<DataWriterImpl> dataWriter);

	/**
	 * @brief 关联本地读者内置主题。
	 * 
	 * @param [in] dataReader DataReaderImpl类指针。
	 */
	void SetParticipantMessageDataReader(std::shared_ptr<DataReaderImpl> dataReader);

	/**
	 * @brief 声明writer存活性。
	 *
	 * @param [in] guid writer guid。
	 * @param [in] localWriter 是否本地writer标志。
	 * @param [in] kind Liveliness 类型 Qos配置信息。
	 *
	 * @return 声明成功返回true，否则返回false。
	 */
	bool AssertLiveliness(GUID_t guid,bool localWriter, LivelinessQosPolicyKind kind);

	//bool AssertLivelinessManualByParticipant();

	/**
	 * @brief 管理的writer存活性状态发生变更时会调用该函数进行通知。
	 * WriterStatus 为状态为变化之前的状态
	 *
	 * @param [in] writer writer guid。
	 * @param [in] localWriter 是否本地writer标志。
	 * @param [in] livelinessQos Liveliness Qos配置信息。
	 * @param [in] status writer存活状态。
	 */
	void LiveLinessStatusChanged(const GUID_t & writer, bool localWriter,const LivelinessQosPolicy& livelinessQos, WriterStatus status);

	/**
	 * @brief 管理的本地writer存活性状态发生变更时会调用该函数进行通知。
	 * WriterStatus 为状态为变化之前的状态
	 *
	 * @param [in] writer writer guid。
	 * @param [in] livelinessQos Liveliness Qos配置信息。
	 * @param [in] status writer存活状态。
	 */
	void LocalStatusChanged(const GUID_t & writer, const LivelinessQosPolicy& livelinessQos, WriterStatus status);

	/**
	 * @brief 管理的远端writer存活性状态发生变更时会调用该函数进行通知。
	 * WriterStatus 为状态为变化之前的状态
	 *
	 * @param [in] writer writer guid。
	 * @param [in] livelinessQos Liveliness Qos配置信息。
	 * @param [in] status writer存活状态。
	 */
	void RemoteStatusChanged(const GUID_t & writer,const LivelinessQosPolicy& livelinessQos, WriterStatus status);

	/**
	 * @brief 获取本地Liveliness Qos 管理指针。
	 *
	 * @return LivelinessAssert类指针。
	 */
	std::shared_ptr<LivelinessAssert> GetLocalWriterLivelinessManager();
	
	/**
	 * @brief 获取远端Liveliness Qos 管理指针。
	 *
	 * @return LivelinessAssert类指针。
	 */
	std::shared_ptr<LivelinessAssert> GetRemoteWriterLivelinessManager();

	/**
	 * @brief 获取内置data(m)报文数据读者指针。
	 *
	 * @return 内置data(m)报文数据读者指针。
	 */
	std::shared_ptr<StatefulReader> GetParticipantDataReader();

	/**
	 * @brief 内置data(m)报文数据读者回调函数。
	 * 
	 * @param [in] change data(m)报文数据。
	 */
	void on_data_available(std::shared_ptr<CacheChange> change);

private:

	/**
	 * @brief 管理的本地writer存活性状态发生变更时会调用该函数进行通知。
	 * WriterStatus 为状态为变化之前的状态
	 *
	 * @param [in] writerGuid writer guid。
	 * @param [in] writers 本地数据读者容器。
	 */
	void UpdateLivelinessLostStatus(const GUID_t& writerGuid, std::map<GUID_t, std::shared_ptr<Writer>> writers);

    /**
	 * @brief 创建定时器或者修改定时器执行周期，用于发送Data(m)报文
	 *
	 * @param [in] qos Liveliness Qos。
	 */
	void SetTimer(const LivelinessQosPolicy & qos);

	/**
	 * @brief 从本地数据读者容器中删除guid对应的本地数据读者。
	 *
	 * @param [in] guid writer guid。
	 * @param [in] writers 本地writer容器。
	 *
	 * @return 删除成功返回true，否则返回false。
	 */
	bool RemoveWriterMap(GUID_t guid, std::map<GUID_t, std::shared_ptr<Writer>>& writers);

	 /**
	  * @brief 获取现有writer中最小周期，并更新定时器周期。
	  *
	  * @param [in] kind Liveliness Qos类型。
	  *
	  * @return 更新成功返回true，否则返回false。
	  */
	bool UpdateTimer(LivelinessQosPolicyKind kind);


	//std::shared_ptr<Participant> participant_;
	Participant* participant_;

	/* 自动模式data(m)报文发送定时器 */
	std::shared_ptr<Timer> autoTimer_;
	std::shared_ptr<Timer> manualTimer_;

	std::map<GUID_t,std::shared_ptr<Writer>> automaticWriters_;
	std::map<GUID_t,std::shared_ptr<Writer>> manualByParticipantWriters_;
	std::map<GUID_t,std::shared_ptr<Writer>> manualByTopicWriters_;

	std::map<GUID_t, std::shared_ptr<Reader>> readers_;
	bool automaticReaders_;

	std::shared_ptr<LivelinessAssert> localWriterLivelinessManager_;
	std::shared_ptr<LivelinessAssert> remoteWriterLivelinessManager_;

	InstanceHandle_t autoHandle_;
	InstanceHandle_t manualHandle_;

	std::shared_ptr<StatefulWriter> ParticipantMessageDataWriter_;
	std::shared_ptr<StatefulReader> ParticipantMessageDataReader_;

	std::mutex writerMutex_;
};

TRAVODDS_NAMESPACE_END
