#include "rtps/transport/sharedmemsocket.h"
#include "rtps/transport/sharedmemaccept.h"
#include "common/sys/sysinfo.h"

#include <random>

static void Generate4byteID(char id[5])
{
	// 简单随机，不同设备或同一设备 二次申请到同一个端口，并生成同一个随机微乎其微
	// 即使同进程同一时间进来说明端口肯定不一致即使随机结果一致也可以忽略
	static const std::string chars =
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<> dis(0, chars.size() - 1);

	for (int i = 0; i < 4; ++i) {
		id[i] = chars[dis(gen)];
	}
	id[4] = '\0';
}

USING_TRAVODDS_NAMESPACE;

using namespace TRAVODDS::InterprocessType;

SharedMemSocket::SharedMemSocket()
	: pool_(0) , port_(0), data_(nullptr) {
}

SharedMemSocket::SharedMemSocket(std::function<void(char*, const uint32_t&)> callback)
	: callback_(std::move(callback)), pool_(1), port_(0), data_(nullptr) {
}

SharedMemSocket::~SharedMemSocket()
{
	Close();
	data_ = nullptr;
}

bool SharedMemSocket::Connect(const std::string& srcport, const std::string& destport)
{
	acceptPort_ = destport;
	port_ = std::stoul(srcport.substr(0, srcport.length() - 4));
	sharedMemoryName_ = "TravoddsTransportMemory" + srcport + "-" + destport;
	try {
		segment_ = std::make_unique<managed_shared_memory>(
			open_only, sharedMemoryName_.data());
		data_ = segment_->find<Data>(objectName_).first;
		asio::post(pool_, [this]() { this->ReadFromSharedMem(); });
		return true;
	} catch (const interprocess_exception& e) {
		static_cast<void>(e);
		return false;
	}
	return false;
}

bool SharedMemSocket::Connect(const std::string& destport)
{
	try {
		std::string sharedMemoryName = "TravoddsAcceptSharedMemory" + destport;
		const char* objectName = "TravoddsAcceptsharedAcceptList";

		managed_shared_memory segment(open_only, sharedMemoryName.data());

		SharedAcceptList* sharedAcceptList = segment.find<SharedAcceptList>(objectName).first;
		if (sharedAcceptList == nullptr) {
			return false;
		}
		{
			scoped_lock<robust_mutex> lock(sharedAcceptList->mutex);
			if (sharedAcceptList->shutdown) {
				return false;
			}

			int32_t pocessId = SysInfo::GetProcessIDStatic();

			uint32_t expected = 0x10000;

			if (sharedAcceptList->sockets.empty()) {
				sharedAcceptList->sockets[expected] = pocessId;
			}
			else {
				while(expected >= 0x10000)
				{
					auto srcit = sharedAcceptList->srcPorts.find(expected);
					if (srcit != sharedAcceptList->srcPorts.end())
					{
						expected++;
						continue;
					}
					auto it = sharedAcceptList->sockets.find(expected);
					if (it == sharedAcceptList->sockets.end()) {
						sharedAcceptList->sockets[expected] = pocessId;
						break;
					} else if (!SysInfo::IsProcessExists(it->second)) {
						it->second = pocessId;
						break;
					}
					expected++;
				}

			}
			port_ = expected;
			acceptPort_ = destport;

			Identity identity = {0};
		    Generate4byteID(identity.c);
			std::string srcport = std::to_string(expected) + std::string(identity.c);
			sharedMemoryName_ = "TravoddsTransportMemory" + srcport + "-" + destport;
			CleanupResources();
			segment_ = std::make_unique<managed_shared_memory>(
				create_only, sharedMemoryName_.data(), sharedMemSize_);

			data_ = segment_->construct<Data>(objectName_)();
			sharedAcceptList->srcPorts[expected] = identity.ui;
		}
		sharedAcceptList->condition.notify_one();
		return true;
	} catch (const interprocess_exception& e) {
		static_cast<void>(e);
	}
	return false;

}

void SharedMemSocket::Write(const char* message, uint32_t len)
{
	data_->refs.fetch_add(1, std::memory_order_relaxed);
	if (!message || len == 0 || len > SHM_MAX_MESSAGE_SIZE) {
		data_->refs.fetch_sub(1, std::memory_order_relaxed);
		throw std::invalid_argument("Invalid argument");
	}
	scoped_lock<interprocess_mutex>lock(data_->wmutex);
	bool waitsuc = data_->wcondition.wait_for(
		lock, 
		std::chrono::milliseconds(200),
		[&]() {
			return data_->shutdown || IsWriteSucfully(message, len);
		}
	);
	if (!waitsuc) {
		data_->refs.fetch_sub(1, std::memory_order_relaxed);
		throw std::runtime_error("Write operation timed out");
	}

	if (IsShutdown() || data_->shutdown) {
		shutdown_ = true;
		data_->refs.fetch_sub(1, std::memory_order_relaxed);
		throw std::runtime_error("Socket is shutdown");
	}

	data_->rcondition.notify_one();
	data_->refs.fetch_sub(1, std::memory_order_relaxed);
}

void SharedMemSocket::Shutdown()
{
	if (IsShutdown()) {
		return;
	}
	shutdown_ = true;
	if (data_) {
		data_->shutdown = true;
		data_->wcondition.notify_one();
		data_->rcondition.notify_one();
		uint32_t trycount = 0;
		while (data_->refs.load(std::memory_order_acquire) != 0 && trycount++ < 20) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	pool_.join();
}

bool SharedMemSocket::IsShutdown() const
{
	return shutdown_;
}

void SharedMemSocket::Close(bool haslock)
{
	if(isClosed_) {
		return;
	}
	Shutdown();
	CleanupResources();
	try {
		std::string sharedMemoryName = "TravoddsAcceptSharedMemory" + acceptPort_;
		const char* objectName = "TravoddsAcceptsharedAcceptList";

		managed_shared_memory segment(open_only, sharedMemoryName.data());

		SharedAcceptList* sharedAcceptList = segment.find<SharedAcceptList>(objectName).first;
		if(haslock == false && sharedAcceptList != nullptr) {
			scoped_lock<robust_mutex> lock(sharedAcceptList->mutex);
			sharedAcceptList->sockets.erase(port_);
		} else if (sharedAcceptList != nullptr) {
			sharedAcceptList->sockets.erase(port_);
		}
	} catch (const std::exception& e) {
		static_cast<void>(e);
	}
	isClosed_ = true;
}

void SharedMemSocket::CleanupResources()
{
	try {
		// 条件变量/信号量 → 互斥锁 → 共享内存
		shared_memory_object::remove(sharedMemoryName_.data());
		managed_shared_memory segment(open_only, sharedMemoryName_.data());
		Data* data = segment.find<Data>(objectName_).first;
		if (data != nullptr) {
			segment.destroy<Data>(objectName_);
			data = nullptr;
		}

		std::string shared_dir;
		ipcdetail::get_shared_dir(shared_dir);
		std::remove((shared_dir + "/" + sharedMemoryName_).c_str());
	} catch (const std::exception& e) {
		static_cast<void>(e);
	}
}



bool SharedMemSocket::IsWriteSucfully(const char* message, const uint32_t& len)
{
	uint32_t currentwritepos = data_->writepos;
	uint32_t currentreadpos = data_->readpos.load(std::memory_order_acquire);

	if (currentwritepos >= currentreadpos) {
		if (currentwritepos == currentreadpos) {
			if (data_->usedBytes.load(std::memory_order_acquire) != 0) {
				data_->rcondition.notify_one();
				return false;
			}
		}
		if ((buffersize_ - currentwritepos) < (len + sizeof(uint32_t))) {
			if (currentreadpos < (len + sizeof(uint32_t))) {
				data_->rcondition.notify_one();
				return false;
			} else {
				memset(data_->msg + currentwritepos, uint8_t(~0), sizeof(uint32_t));
				currentwritepos = 0;
			}
		}
	} else {
		if ((currentreadpos - currentwritepos) < (len + sizeof(uint32_t))) {
			data_->rcondition.notify_one();
			return false;
		}
	}
	std::memcpy(data_->msg + currentwritepos, (char*)&len, sizeof(uint32_t));
	std::memcpy(data_->msg + currentwritepos + sizeof(uint32_t), message, len);
	data_->writepos = currentwritepos + len + sizeof(uint32_t);
	data_->usedBytes.fetch_add(len + sizeof(uint32_t), std::memory_order_acq_rel);

	return true;
}

void SharedMemSocket::ReadFromSharedMem() {
	data_->refs.fetch_add(1, std::memory_order_relaxed);
	while (!shutdown_) {
		scoped_lock<interprocess_mutex>lock(data_->rmutex);
		data_->rcondition.wait(lock, [this]() {
			return (data_->shutdown ||data_->usedBytes.load(std::memory_order_acquire) != 0);
			}
		);

		// 先验本地，再验共享内存，保持一致
		if (IsShutdown() || data_->shutdown) {
			shutdown_ = true;
			continue;
		}
		uint32_t currentreadpos = data_->readpos.load(std::memory_order_acquire);
		uint32_t startpos = currentreadpos;
		char* temp = data_->msg + startpos;
		while (!shutdown_ && data_->usedBytes.load(std::memory_order_acquire) != 0) {
			Message_t& msg = *reinterpret_cast<Message_t*>(temp);
			if (msg.dataSize == uint32_t(~0)) {
				startpos = 0;
				temp = data_->msg;
			} else  {
				callback_(msg.data, msg.dataSize);
				data_->usedBytes.fetch_sub(msg.dataSize + sizeof(uint32_t), std::memory_order_acq_rel);
				startpos += (msg.dataSize + sizeof(uint32_t));
				temp = data_->msg + startpos;
			}
			data_->readpos.store(startpos, std::memory_order_release);
			data_->wcondition.notify_one();
		}
	}
	data_->refs.fetch_sub(1, std::memory_order_relaxed);
}

void SharedMemSocket::SetNull()
{
	shutdown_ = true;
	isClosed_ = true;
	port_ = 0;
	acceptPort_.clear();
	sharedMemoryName_.clear();
	segment_.reset();
	data_ = nullptr;
}
