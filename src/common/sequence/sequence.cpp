#include "common/sequence/sequencebase.h"
#include <limits>
#include <stdexcept>

uint32_t SequenceBase::maximum() const
{
	return maximum_;
}

void* SequenceBase::operator [](uint32_t i)
{
	if (i >= length_) throw std::out_of_range("sequence index out of range");
	return elements_[i];
}

bool SequenceBase::borrow(std::vector<void*> elements, std::shared_ptr<void> pin,
    std::shared_ptr<const void> owner, uint32_t role)
{
    if (maximum_ || length_ || has_loan() || !pin || !owner || elements.empty() ||
        elements.size() > std::numeric_limits<uint32_t>::max()) return false;
    for (auto* element : elements) if (!element) return false;
    loan_elements_ = std::move(elements);
    loan_pin_ = std::move(pin);
    loan_owner_ = std::move(owner);
    loan_role_ = role;
    length_ = static_cast<uint32_t>(loan_elements_.size());
    elements_ = loan_elements_.data();
    return true;
}

bool SequenceBase::matches_loan(const SequenceBase& other,
    const std::shared_ptr<const void>& owner, uint32_t ownRole, uint32_t otherRole) const
{
    return loan_pin_ && loan_pin_ == other.loan_pin_ && loan_owner_ == owner &&
        other.loan_owner_ == owner && loan_role_ == ownRole && other.loan_role_ == otherRole &&
        length_ == other.length_;
}

void SequenceBase::clear_loan()
{
    elements_ = nullptr;
    length_ = maximum_ = 0;
    loan_elements_.clear();
    loan_pin_.reset();
    loan_owner_.reset();
    loan_role_ = 0;
}

void** SequenceBase::buffer() const
{
	return elements_;
}

SequenceBase::~SequenceBase()
{

}
