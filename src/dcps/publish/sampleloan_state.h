#ifndef TRAVODDS_SAMPLELOAN_STATE_H
#define TRAVODDS_SAMPLELOAN_STATE_H
#include "dcps/publish/sampleloan.h"
#include "common/memory/sharedpayloadpool.h"
TRAVODDS_NAMESPACE_BEGIN
struct SampleLoan::State {
    void* data = nullptr;
    std::shared_ptr<const void> writer;
#if defined(__linux__)
    std::shared_ptr<SharedPayloadPool> pool;
    SharedSampleToken token;
    ~State() { if (data && pool) pool->releaseWriter(token); }
#endif
};
TRAVODDS_NAMESPACE_END
#endif
