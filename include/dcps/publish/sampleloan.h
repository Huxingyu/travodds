// Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
#ifndef TRAVODDS_DCPS_PUBLISH_SAMPLELOAN_H
#define TRAVODDS_DCPS_PUBLISH_SAMPLELOAN_H
#include "travodds_dll.h"
#include <memory>

TRAVODDS_NAMESPACE_BEGIN
class DataWriterImpl;
// A move-only reservation. Destruction cancels an unsubmitted loan. A
// successful write consumes it; failed writes preserve it for retry/discard.
class DDS_DLL SampleLoan {
public:
    SampleLoan() = default;
    ~SampleLoan();
    SampleLoan(SampleLoan&&) noexcept;
    SampleLoan& operator=(SampleLoan&&) noexcept;
    SampleLoan(const SampleLoan&) = delete;
    SampleLoan& operator=(const SampleLoan&) = delete;
    void* data() const;
    explicit operator bool() const { return static_cast<bool>(state_); }
    void reset() noexcept;
private:
    struct State;
    std::shared_ptr<State> state_;
    friend class DataWriterImpl;
};
TRAVODDS_NAMESPACE_END
#endif
