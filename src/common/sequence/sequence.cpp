#include "common/sequence/sequencebase.h"

int32_t SequenceBase::maximum() const
{
	return maximum_;
}

void* SequenceBase::operator [](uint32_t i)
{
	return elements_[i];
}

void** SequenceBase::buffer() const
{
	return elements_;
}

SequenceBase::~SequenceBase()
{

}
