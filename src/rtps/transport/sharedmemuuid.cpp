#include "rtps/transport/sharedmemuuid.h"
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_generators.hpp"
#include "boost/uuid/uuid_io.hpp"


USING_TRAVODDS_NAMESPACE;
using namespace TRAVODDS::InterprocessType;

SharedMemUUID::SharedMemUUID() 
    : SharedMemEntity("TravoddsGlobalUUIDSharedMemory", "TravoddsGlobalSharedUUID", "TravoddsGlobalUUIDMutex",1024) {
	try {
		std::unique_lock<named_mutex> lock(*mutex_, std::chrono::milliseconds(500));
		if (!lock.owns_lock()) {
			// 默认存在异常进程，出现死锁
			// 直接解锁，重新加锁
			mutex_->unlock();
			lock.lock();
		}

		auto ret = segment_->find<SharedUUID>(objectName_);

		if (ret.first && ret.second == 1) {
			shareduuid_ = ret.first;
			shareduuid_->referenceCount++;
		} else {
			boost::uuids::random_generator generator;
			boost::uuids::uuid uuid = generator();

			// 在共享内存中创建UUID对象
			shareduuid_ = segment_->construct<SharedUUID>(objectName_)();
			memcpy(shareduuid_->uuid.data(), uuid.data(), shareduuid_->uuid.size());
			shareduuid_->referenceCount = 1;
		}
	} catch (const std::exception& e) {
		CleanupResources();
		throw e;
	}
}

const SharedMemUUID& SharedMemUUID::GetInstance()
{
	static SharedMemUUID instance;
	return instance;
}

SharedMemUUID::~SharedMemUUID()
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
			if (shareduuid_) {
				remaining_ = --shareduuid_->referenceCount;
				if (remaining_ == 0) {
					if (segment_) {
						segment_->destroy<SharedUUID>(objectName_);
					}
				}
			}
			shareduuid_ = nullptr;
		}
	} catch (const std::exception& e) {
		throw e;
	}
}

std::array<char, 16> SharedMemUUID::GetUUID() const
{
	if (shareduuid_) {
		return shareduuid_->uuid;
	}
	return std::array<char, 16>();
}