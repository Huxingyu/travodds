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

#include  "RTPS/Writer.h"
#include  "RTPS/ReaderLocator.h"

AGIDDS_NAMESPACE_BEGIN

class StatelessWriter: public Writer{

public:
	StatelessWriter(DataWriterQos& qos, Duration_t resendPeriod);

	void ReaderLocatorAdd(Locator_t& locator);

	void ReaderLocatorRemove(Locator_t& locator);

	void UnsentChangesReset();

	virtual ReturnCode_t DeliveryData(CacheChange* change);

	//TODO 周期性发送history中的数据
	void ResendDataPeriod();

private:

	Duration_t resendDataPeriod_;

	map<Locator_t,ReaderLocator*> readerLocators_;

};

AGIDDS_NAMESPACE_END
