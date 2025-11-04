#include "rtps/transport/sharedmemwatchdog.h"
#include "common/sys/sysinfo.h"

USING_TRAVODDS_NAMESPACE;
using namespace TRAVODDS::InterprocessType;

SharedMemWatchdog::SharedMemWatchdog()
	: SharedMemEntity("TravoddsWatchDogSharedMemory", "TravoddsWatchDogList", "TravoddsWatchDogUUIDMutex", 32 * 1024)
{
	try {
		std::unique_lock<named_mutex> lock(*mutex_, std::chrono::milliseconds(500));
		if (!lock.owns_lock()) {
			// 默认存在异常进程，出现死锁
			// 直接解锁，重新加锁
			mutex_->unlock();
			lock.lock();
		}
		auto ret = segment_->find<SharedList>(objectName_);

		if (ret.first && ret.second == 1) {
			sharedList_ = ret.first;
			sharedList_->referenceCount++;
		} else {
			sharedList_ = segment_->construct<SharedList>(objectName_)(ShmemAllocator(segment_->get_segment_manager()));
			sharedList_->referenceCount = 1;
		}
	} catch (const std::exception& e) {
		CleanupResources();
		throw e;
	}

}

const SharedMemWatchdog& SharedMemWatchdog::GetInstance()
{
	static SharedMemWatchdog instance;
	return instance;
}

SharedMemWatchdog::~SharedMemWatchdog()
{
	try {
		if (mutex_) {
			std::unique_lock<named_mutex> lock(*mutex_, std::chrono::milliseconds(500));
			if (!lock.owns_lock()) {
				// 默认存在异常进程，出现死锁
				// 直接解锁，重新加锁
				mutex_->unlock();
				lock.lock();
			}
			if (sharedList_) {
				remaining_ = --sharedList_->referenceCount;
				if (remaining_ == 0) {
					if (segment_) {
						segment_->destroy<SharedList>(objectName_);
					}
				}
			}
			sharedList_ = nullptr;
		}
	} catch (const std::exception& e) {
		throw e;
	}
}

ReturnCode_t SharedMemWatchdog::InsertPortProcess(const uint32_t& port) const
{
	try {
		scoped_lock<robust_mutex>  lock(sharedList_->mutex);
		auto it = sharedList_->portprocessidmap.find(port);

		if (it != sharedList_->portprocessidmap.end() && SysInfo::IsProcessExists(it->second)) {
			return RETCODE_PORT_IN_USE;
		}
		sharedList_->portprocessidmap[port] = SysInfo::GetProcessIDStatic();

	} catch (const std::exception& e) {
		return RETCODE_ERROR;
	}
	return RETCODE_OK;
}
