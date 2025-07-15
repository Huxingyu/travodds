#include "rtps/flowcontrol/flowcontrolmanager.h"

USING_AGIDDS_NAMESPACE;

FlowControlManager::FlowControlManager(Participant* participant)
	: participant_(participant)
{
}

FlowController* FlowControlManager::GetFlowController(const FlowControllerConfig& config)
{
	std::lock_guard<std::mutex> lock(mtx_);
	auto& policyMap = flowControllers_[config.kind];
	auto it = policyMap.find(config.policy);
	if ( it != policyMap.end()) {
		return it->second.get();
	}
	switch (config.kind) {
	    case FlowControllerKind::SYNC:{
			return policyMap.emplace(
				config.policy, std::make_unique<FlowSyncController>(participant_,config)
			).first->second.get();
		}
		case FlowControllerKind::ASYNC: {
			return policyMap.emplace(
				config.policy, std::make_unique<FlowAsyncController>(participant_, config)
			).first->second.get();
		}
		case FlowControllerKind::SEMI_SYNC: {
			return policyMap.emplace(
				config.policy, std::make_unique<FlowSemiSyncController>(participant_, config)
			).first->second.get();
		}
		default:
			break;

	}
	return nullptr;
}
