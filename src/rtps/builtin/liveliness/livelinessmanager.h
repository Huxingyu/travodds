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

extern std::mutex writerMutex_;
AGIDDS_NAMESPACE_BEGIN

class LivelinessManager : public ReaderListener {
public:

	LivelinessManager(Participant* participant);

	virtual ~LivelinessManager();

	/*
	 * 发现新节点时，为内置主题添加代理
	 */
	void AddParticipantMessageProxy(const SPDPdiscoveredParticipantData & data);

	void RemoveParticipantMessageProxy(GUID_t guid);

	/*
	 * 添加writer 根据qos设置定时器
	 */
	bool AddLivelinessWriter(std::shared_ptr<Writer> writerPtr, const LivelinessQosPolicy & dwQos);

	bool RemoveLivelinessWriter(std::shared_ptr<Writer> writerPtr);

	bool AddLivelinessReader(std::shared_ptr<Reader> readerPtr, const LivelinessQosPolicy & drQos);

	bool RemoveLivelinessReader(std::shared_ptr<Reader> readerPtr);

	/*
	 * 发送Data(m)报文
	 */
	bool AutoLivelinessChecker();

	bool ParticipantLivelinessChecker();

	bool SendLivelinessMessage(const InstanceHandle_t & instance);

	/*
	 * 关联内置主题
	 */
	void SetParticipantMessageDataWriter(std::shared_ptr<DataWriterImpl> dataWriter);

	void SetParticipantMessageDataReader(std::shared_ptr<DataReaderImpl> dataReader);

	/*
	 * 断言writer存活性 
	 */
	bool AssertLiveliness(GUID_t guid,bool localWriter, LivelinessQosPolicyKind kind);

	//bool AssertLivelinessManualByParticipant();

	/*
	 * 管理的writer存活性状态发生变更时会调用该函数进行通知
	 * WriterStatus 状态为变化之前的状态
	 */
	void LiveLinessStatusChanged(const GUID_t & writer, bool localWriter,const LivelinessQosPolicy& livelinessQos, WriterStatus status);

	void LocalStatusChanged(const GUID_t & writer, const LivelinessQosPolicy& livelinessQos, WriterStatus status);

	void RemoteStatusChanged(const GUID_t & writer,const LivelinessQosPolicy& livelinessQos, WriterStatus status);

	std::shared_ptr<LivelinessAssert> GetLocalWriterLivelinessManager();
	std::shared_ptr<LivelinessAssert> GetRemoteWriterLivelinessManager();

	std::shared_ptr<StatefulReader> GetParticipantDataReader();

	/**
	* 接收data(m)报文 更新writer存活性状态
	*/
	void on_data_available(std::shared_ptr<CacheChange> change);

private:
	void UpdateLivelinessLostStatus(const GUID_t& writerGuid, std::map<GUID_t, std::shared_ptr<Writer>> writers);

	/*
	 * 创建定时器或者修改定时器执行周期，发送Data(m)报文
	 */
	void SetTimer(const LivelinessQosPolicy & qos);


	bool RemoveWriterMap(GUID_t guid, std::map<GUID_t, std::shared_ptr<Writer>>& writers);

	/*
	 * 获取现有writer中最小周期，并更新定时器周期
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
};

AGIDDS_NAMESPACE_END
