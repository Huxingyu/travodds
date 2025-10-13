#include "rtps/transport/sharedmemaccept.h"
#include "rtps/transport/sharedmemsocket.h"
#include "common/sys/sysinfo.h"



USING_TRAVODDS_NAMESPACE;
using namespace TRAVODDS::InterprocessType;

SharedMemAccept::SharedMemAccept(const std::string& port)
	: port_(port)
	, sharedMemoryName_("TravoddsAcceptSharedMemory" + port)
	, objectName_("TravoddsAcceptsharedAcceptList")
    , pool_(1) {
	try {
		CleanupResources();
		segment_ = std::make_unique<managed_shared_memory>(
			create_only, sharedMemoryName_.data(), sharedMemSize_);	
		sharedAcceptList_ = segment_->construct<SharedAcceptList>(objectName_)(ShmemAllocator(segment_->get_segment_manager()));

	} catch (const interprocess_exception& e) {
		throw e;
	}
}

SharedMemAccept::~SharedMemAccept()
{
	Close();
	CleanupResources();	
}

void SharedMemAccept::Accept(std::shared_ptr<SharedMemSocket> socket)
{
	refs_.fetch_add(1, std::memory_order_release);
	try {
		scoped_lock<robust_mutex> lock(sharedAcceptList_->mutex);
		sharedAcceptList_->condition.wait(lock,
			[this](){
				return IsShutdown() || sharedAcceptList_->connecting;
			}
		);
		if (IsShutdown()) {
			throw std::runtime_error("Accept is shutdown.");
		}

		sharedAcceptList_->connecting = false;
		sockets_[sharedAcceptList_->srcPort] = socket;
		std::string srcport = std::to_string(sharedAcceptList_->srcPort);
		if (!socket->Connect(srcport, port_)) {
			throw std::runtime_error("Accept connect failed.");
		}
	} catch (const std::exception& e) {
		refs_.fetch_sub(1, std::memory_order_release);
		throw e;
	}
	refs_.fetch_sub(1, std::memory_order_release);
}

void SharedMemAccept::AsyncAccept(std::shared_ptr<SharedMemSocket> socket, std::function<void(const std::exception*)> callback)
{
	asio::post(pool_,
		[this, socket, callback = std::move(callback)]() mutable {
			try {
				Accept(socket);
				callback(nullptr);
			} catch (const std::exception& e) {
				callback(&e);
			}
		}
	);

}

void SharedMemAccept::Close() 
{
	Shutdown();

	auto start = std::chrono::steady_clock::now();
	while (refs_.load(std::memory_order_acquire) != 0) {
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

		if (elapsed >= std::chrono::seconds(3)) {
			break; // 准备强制关闭
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	scoped_lock<robust_mutex> lock(sharedAcceptList_->mutex);
	for (auto& it : sockets_) {
		it.second->Close();
	}
	sockets_.clear();
	sharedAcceptList_->sockets.clear();
}


void SharedMemAccept::Shutdown()
{
	if (IsShutdown()) {
		return;
	}
	shutdown_ = true;
	if(sharedAcceptList_) {
		sharedAcceptList_->shutdown = true;
		sharedAcceptList_->condition.notify_one();
	}
}

bool SharedMemAccept::IsShutdown() const
{
	return shutdown_;
}

void SharedMemAccept::CleanupResources()
{
	try {
		// 条件变量/信号量 → 互斥锁 → 共享内存
		shared_memory_object::remove(sharedMemoryName_.data());
		// 若是还能打开再进行二次删除
		managed_shared_memory segment(open_only, sharedMemoryName_.data());
		SharedAcceptList* sharedAcceptList = segment.find<SharedAcceptList>(objectName_).first;			
		if (sharedAcceptList != nullptr) {
			segment.destroy<SharedAcceptList>(objectName_);
			sharedAcceptList = nullptr;
		};

		std::string shared_dir;
		ipcdetail::get_shared_dir(shared_dir);
		std::remove((shared_dir + "/" + sharedMemoryName_).c_str());
	} catch (...) {
	}
}