#include "SequenceBase.h"

int SequenceBase::maximum() const
{
	return maximum_;
}

int SequenceBase::length() const
{
	return length_;
}
void* SequenceBase::operator [](unsigned long i)
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