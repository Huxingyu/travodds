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
#ifndef AGIDDS_RTPS_ENDPOINT_STATELESSWRITER_H
#define AGIDDS_RTPS_ENDPOINT_STATELESSWRITER_H

#include  "rtps/endpoint/writer.h"
#include  "rtps/endpoint/readerlocator.h"

AGIDDS_NAMESPACE_BEGIN
class Timer;

class StatelessWriter : public Writer {

public:
	StatelessWriter(const DataWriterQos& qos, const GUID_t& guid, const TopicKind_t& kind);

	virtual ~StatelessWriter();

	void MatchedReaderAdd(const DiscoveredReaderData &subData);

	void ReaderLocatorAdd(Locator_t& locator);

	void ReaderLocatorRemove(Locator_t& locator);

	void UnsentChangesReset();

	virtual ReturnCode_t DeliveryData(std::shared_ptr<CacheChange> change);

	void SetResendDataPeriod(Duration_t period);
	//TODO 周期性发送history中的数据
	void ResendDataPeriod();

	virtual ReturnCode_t ReceiveAckNackMessage(const Receive_t&  headerMsg, const AcknackSubMessage_t& ackMsg);

	virtual ReturnCode_t ReceiveNackFragMessage(const Receive_t&  headerMsg, const NackFragSubMessage_t& nackMsg);

private:

	Duration_t resendDataPeriod_;

	std::map<Locator_t, std::shared_ptr<ReaderLocator>> readerLocators_;

	std::shared_ptr<Timer> timer_;
};

AGIDDS_NAMESPACE_END

#endif // !AGIDDS_RTPS_ENDPOINT_STATELESSWRITER_H
