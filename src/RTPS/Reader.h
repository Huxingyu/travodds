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

#include "RTPS/Endpoint.h"
#include "RTPS/HistoryCache.h"

AGIDDS_NAMESPACE_BEGIN

class ReaderListener {

	virtual void on_data_available(){};
};

class Reader : public Endpoint {
public:

	virtual ~Reader();

	/* read 获取数据 */
	ReturnCode_t ReadOrTake(SerializedBufferSeq & dataValues, SampleInfoSeq & sampleInfos, int maxSamples,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake = false);

	ReturnCode_t ReadOrTakeInstance(SerializedBufferSeq & dataValues, SampleInfoSeq & sampleInfos, int maxSamples, InstanceHandle_t handle,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake = false);

	ReturnCode_t ReadOrTakeNextInstance(SerializedBufferSeq & dataValues, SampleInfoSeq & sampleInfos, int maxSamples, InstanceHandle_t handle,
		SampleStateMask sampleStates, ViewStateMask viewStates, InstanceStateMask instanceStates, bool shouldTake = false);


	/* 获取下一个实例句柄 */
	ReturnCode_t FindNextInstance(InstanceHandle_t& preHander,InstanceHandle_t& instanceHandle);

	CacheChange* new_change(ChangeKind_t kind, SerializedBuffer data, InstanceHandle_t handle,InstanceHandle_t pubHandle,Time_t sourceTimespace);


private:

	/*****************************************/
	/* 下方函数没有具体实现功能 TODO */
	/* 获取保存在缓冲区中最新样本的状态信息  */
	SampleInfo GetLastInstanceSampleInfo(const InstanceHandle_t& handle);
	/* 本地DataReader触发LivelinessQoS，修改本地实例样本的状态   */
	void ChangeSampleInfoByLiveliness();
	/* 远端的DW调用Dispose函数，修改对应instance实例的状态 */
	void ChangeSampleInfoByDWDisposed(const InstanceHandle_t& handl);
	/******************************************/



protected:
	Duration_t heartbeatResponseDelay_;
	Duration_t heartbeatSuppressionDuration_;
	HistoryCache readerCache_;
	bool expectsInlineQos_;

	/* jd 保存样本的实例信息,并且大小进行排序 */
	std::list<InstanceHandle_t> instanceHandles_;

	/* 每个实例对应的样本的信息  */
	std::map<InstanceHandle_t, SampleInfo> sampleInfos_;

	/* 接收到的数据信息的最新的数据序号  */
	SequenceNumber_t lastSequenceNumber_;

	std::map<InstanceHandle_t, SampleInfo> statusCache_;

	ReaderListener* readerListener_;



};

AGIDDS_NAMESPACE_END
