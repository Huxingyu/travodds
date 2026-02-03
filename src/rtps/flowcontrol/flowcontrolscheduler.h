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

#ifndef TRAVODDS_RTPS_FLOWCONTROL_FLOWCONTROLSCHEDULER_H
#define TRAVODDS_RTPS_FLOWCONTROL_FLOWCONTROLSCHEDULER_H

#include "rtps/flowcontrol/flowcontroltype.h"
#include "rtps/basictypertps.h"
#include "rtps/messages/cdrmessage_t.h"
TRAVODDS_NAMESPACE_BEGIN
class ThreadPool;
class CacheChange;
class Writer;
class StatefulWriter;
class ReaderProxy;
/* 策略均基于异步发送 */
class FlowControlScheduler {
public:
	FlowControlScheduler();
	virtual ~FlowControlScheduler();
	void RegisterWriter(std::shared_ptr<Writer> writer);
	void UnregisterWriter(std::shared_ptr<Writer> writer);
	std::shared_ptr<Writer> GetSharedWriterPtr(Writer* writer);

	virtual void AddSendData(Writer* writer, std::shared_ptr<CacheChange> change) = 0;

	virtual void ReSendDataFrag(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& FragSet) = 0;

	virtual void ReSendData(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy) = 0;


protected:
	std::unique_ptr<ThreadPool> asyncThreads_;
	std::map<Writer*, std::shared_ptr<Writer>> writers_;
	std::mutex mtx_;
};

class FlowControlFifoScheduler : public  FlowControlScheduler {
public:
	virtual void AddSendData(Writer* writer, std::shared_ptr<CacheChange> change) override;
	virtual void ReSendDataFrag(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& FragSet) override;
	virtual void ReSendData(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy) override;
};

TRAVODDS_NAMESPACE_END

#endif