#include "rtps/transport/sharedmementity.h"

USING_TRAVODDS_NAMESPACE;
using namespace TRAVODDS::InterprocessType;

SharedMemEntity::SharedMemEntity(const char* sharedMemoryName, const char* objectName, const char* mutexName, size_t size)
	: sharedMemoryName_(sharedMemoryName), objectName_(objectName), mutexName_(mutexName), sharedMemSize_(size)
{
	try {
		segment_ = std::make_unique<managed_shared_memory>(
			open_or_create, sharedMemoryName_, sharedMemSize_);
		mutex_ = std::make_unique<named_mutex>(open_or_create, mutexName_);
	} catch (const std::exception& e) {
		static_cast<void>(e);
		CleanupResources();
		throw;
	}
}


SharedMemEntity::~SharedMemEntity()
{
	CleanupResources();
}


void SharedMemEntity::CleanupResources()
{
	try {
		segment_.reset();
		mutex_.reset();
		if (remaining_ == 0) {
			named_mutex::remove(mutexName_);
			shared_memory_object::remove(sharedMemoryName_);
			std::string shared_dir;
			ipcdetail::get_shared_dir(shared_dir);
			std::remove((shared_dir + "/" + mutexName_).c_str());
			std::remove((shared_dir + "/" + sharedMemoryName_).c_str());
		}

	} catch (...) {
	}
}