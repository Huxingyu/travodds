#include "DDS/TopicDescriptionImpl.h"
#include "DDS/DomainParticipantImpl.h"

USING_AGIDDS_NAMESPACE;
TopicDescriptionImpl::TopicDescriptionImpl() : refs_(0)
{

}

TopicDescriptionImpl::~TopicDescriptionImpl()
{
}

DomainParticipant* TopicDescriptionImpl::get_participant() const
{
    return participant_;
}

const char* TopicDescriptionImpl::get_type_name() const
{
    return typeName_.c_str();
}

const char* TopicDescriptionImpl::get_name() const
{
    return name_.c_str();
}

void TopicDescriptionImpl::Reference()
{
    ++refs_;
}

void TopicDescriptionImpl::Dereference()
{
    --refs_;
}

bool TopicDescriptionImpl::IsReferenced()
{
    return refs_ != 0u;
}
