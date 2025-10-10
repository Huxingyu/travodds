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

#ifndef TRAVODDS_RTPS_FLOWCONTROL_FLOWCONTROLLER_H
#define TRAVODDS_RTPS_FLOWCONTROL_FLOWCONTROLLER_H

#include "rtps/flowcontrol/flowcontroltype.h"
#include "rtps/flowcontrol/flowcontrolscheduler.h"
#include "rtps/basictypertps.h"
TRAVODDS_NAMESPACE_BEGIN

class Writer;
class StatefulWriter;
class CacheChange;
class Participant;
class ReaderProxy;

class FlowController {
public:
	FlowController(Participant* participant, const FlowControllerConfig& config);
	virtual ~FlowController() = default;
	void RegisterWriter(std::shared_ptr<Writer> writer);
	void UnregisterWriter(std::shared_ptr<Writer> writer);
	virtual void AddNewData(Writer* writer, std::shared_ptr<CacheChange> change) = 0;
	virtual void ReSendDataFrag(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& FragSet) = 0;
	virtual void ReSendData(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy) = 0;
protected:
	FlowControllerConfig config_;
	Participant* participant_;
	std::unique_ptr<FlowControlScheduler> scheduler_;

};


class FlowSyncController : public FlowController {
public:
	FlowSyncController(Participant* participant, const FlowControllerConfig& config);
	virtual ~FlowSyncController() = default;

	void AddNewData(Writer* writer, std::shared_ptr<CacheChange> change);

	void ReSendDataFrag(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& FragSet);

	void ReSendData(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy);
	
};

class FlowAsyncController : public FlowController {
public:
	FlowAsyncController(Participant* participant, const FlowControllerConfig& config);
	virtual ~FlowAsyncController() = default;

	virtual void AddNewData(Writer* writer, std::shared_ptr<CacheChange> change);

	void ReSendDataFrag(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy, const NackFragSubMessage_t& FragSet);

	void ReSendData(StatefulWriter* writer, std::shared_ptr<ReaderProxy> readerProxy);
};

class FlowSemiSyncController : public FlowAsyncController {
public:
	FlowSemiSyncController(Participant* participant, const FlowControllerConfig& config);
	virtual ~FlowSemiSyncController() = default;

	void AddNewData(Writer* writer, std::shared_ptr<CacheChange> change) override;
};


TRAVODDS_NAMESPACE_END
#endif