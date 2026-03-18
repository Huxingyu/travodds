#include "dcps/base/entityimpl.h"

USING_TRAVODDS_NAMESPACE;

EntityImpl::EntityImpl(const StatusMask& mask)
	: enable_(false),
	  statusMask_(mask),
	  statusCondition_(this),
	  handle_(HANDLE_NIL),
	  objectData_(nullptr)
{
}

StatusCondition& EntityImpl::get_statuscondition()
{
	return statusCondition_;
}

const StatusMask& EntityImpl::get_status_changes() const
{
	return statusCondition_.get_status();
}

ReturnCode_t EntityImpl::enable()
{
	enable_ = true;
	return RETCODE_OK;
}

const InstanceHandle_t& EntityImpl::get_instance_handle() const
{
	return handle_;
}

GUID_t EntityImpl::get_guid() const
{
	return Conversion::ToGuid(handle_);
}

void EntityImpl::set_object_data(void * data)
{
	objectData_ = data;
}

void * EntityImpl::get_object_data()
{
	return objectData_;
}

bool EntityImpl::IsEnable() const
{
	return enable_;
}

const StatusMask& EntityImpl::get_status_mask() const
{
	return statusMask_;
}


void EntityImpl::SetInstanceHandle(const InstanceHandle_t& handle)
{
	handle_ = handle;
}
