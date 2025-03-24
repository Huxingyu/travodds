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
#include "BuiltinTopicData.h"


AGIDDS_NAMESPACE_BEGIN

class Writer:public Endpoint {
public:
	Writer(DataWriterQos& qos);

	CacheChange* NewChange(ChangeKind_t kind, SerializedBuffer* data, InstanceHandle_t handle);

	ReturnCode_t AddNewCachce(CacheChange* cacheChange);

	virtual bool WaitForAcknowledgments(const Duration_t &max_wait);

	virtual bool IsAckedByAll(SequenceNumber_t changeSeqNum);
	
	bool IsPushMode();

	HistoryCache* GetWriteCache();

	bool DataFilter(CacheChange* change);

	virtual ReturnCode_t DeliveryData(CacheChange* change);

protected:

	bool pushMode_ = true;

	Duration_t nackResponseDelay_;
	Duration_t heartBeatPeriod_;
	Duration_t nackSuppressionDuration_;
	SequenceNumber_t lastChangeSequenceNumber_;
	HistoryCache* writerCache_;

	DataWriterQos qos_;
};

AGIDDS_NAMESPACE_END
