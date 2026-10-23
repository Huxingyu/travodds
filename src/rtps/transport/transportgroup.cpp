#include "rtps/transport/transportgroup.h"
#include <algorithm>

USING_TRAVODDS_NAMESPACE

void TransportGroup::AddLocator(const GuidPrefix_t& prefix, const LocatorSeq& locators)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto itor = locators_.find(prefix);
    if (itor == locators_.end()) {
        LocatorSeq locs = locators;
        std::sort(locs.begin(), locs.end(),
            [](const Locator_t& a, const Locator_t& b) {
                return uint32_t(a.kind - 5) < uint32_t(b.kind - 5);
            });
        locators_.insert({ prefix ,{locs, 1}});
    } else {
        itor->second.second++;
    }
}

void TransportGroup::DecreaseLocator(const GuidPrefix_t& prefix)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto itor = locators_.find(prefix);
    if (itor != locators_.end()) {
        itor->second.second--;
        if (itor->second.second == 0) {
            locators_.erase(itor);
        }
    }
}

void TransportGroup::RemoveLocator(const GuidPrefix_t& prefix)
{
    std::lock_guard<std::mutex> lock(mtx_);
    locators_.erase(prefix);
}

void TransportGroup::GetLocator(const std::set<GuidPrefix_t> prefixs, std::vector<LocatorSeq>& locators)
{
    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto& prefix : prefixs) {
        auto itor = locators_.find(prefix);
        if (itor != locators_.end()) {
            locators.push_back(itor->second.first);
        }
    }
}


