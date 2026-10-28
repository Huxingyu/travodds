#include "dcps/publish/sampleloan_state.h"
TRAVODDS_NAMESPACE_BEGIN
SampleLoan::~SampleLoan() = default;
SampleLoan::SampleLoan(SampleLoan&&) noexcept = default;
SampleLoan& SampleLoan::operator=(SampleLoan&&) noexcept = default;
void* SampleLoan::data() const { return state_ ? state_->data : nullptr; }
void SampleLoan::reset() noexcept { state_.reset(); }
TRAVODDS_NAMESPACE_END
