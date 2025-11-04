#include "rtps/transport/sharedmemsocket.h"
#include "rtps/transport/sharedmemaccept.h"
#include "common/sys/sysinfo.h"
#include "common/memory/memorypool.h"

USING_TRAVODDS_NAMESPACE;
using namespace TRAVODDS::InterprocessType;

SharedMemSocket::SharedMemSocket()
	: data_(nullptr)
	, port_(0)
	, pool_(1) {
}

SharedMemSocket::~SharedMemSocket()
{
	Close();
	data_ = nullptr;
}

bool SharedMemSocket::Connect(const std::string& srcport, const std::string& destport)
{
	acceptPort_ = destport;
	port_ = std::stoul(srcport);
	sharedMemoryName_ = "TravoddsTransportMemory" + srcport + "-" + destport;
	try {
		segment_ = std::make_unique<managed_shared_memory>(
			open_only, sharedMemoryName_.data());
		data_ = segment_->find<Data>(objectName_).first;

		return true;
	} catch (const interprocess_exception& e) {
		return false;
	}
	return false;
}

bool SharedMemSocket::Connect(const std::string& destport)
{
	try {
		std::string srcport;

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

			if (sharedAcceptList->sockets.empty()) {
				sharedAcceptList->sockets[sharedAcceptList->srcPort] = pocessId;
				sharedAcceptList->srcPort = sharedAcceptList->srcPort;
			}
			else {
				uint32_t expected = 0x10000;
				while(expected >= 0x10000)
				{
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
			
				sharedAcceptList->srcPort = expected;
			}
			port_ = sharedAcceptList->srcPort;
			acceptPort_ = destport;
			srcport = std::to_string(sharedAcceptList->srcPort);
			sharedMemoryName_ = "TravoddsTransportMemory" + srcport + "-" + destport;
			CleanupResources();
			segment_ = std::make_unique<managed_shared_memory>(
				create_only, sharedMemoryName_.data(), sharedMemSize_);

			data_ = segment_->construct<Data>(objectName_)();
		}
		sharedAcceptList->connecting = true;
		sharedAcceptList->condition.notify_one();
		return true;
	} catch (const interprocess_exception& e) {
		e.what();
	}
	return false;

}

void SharedMemSocket::Write(const char* message, size_t len)
{
	data_->refs.fetch_add(1, std::memory_order_release);
	if (!message || len == 0 || len > SHM_MAX_MESSAGE_SIZE) {
		data_->refs.fetch_sub(1, std::memory_order_release);
		throw std::invalid_argument("Invalid argument");
	}
	scoped_lock<interprocess_mutex>lock(data_->wmutex);
	bool waitsuc = data_->wcondition.wait_for(
		lock, 
		std::chrono::milliseconds(200),
		[len , this]() {
			return IsAllow(len) || data_->shutdown;
		}
	);
	if (!waitsuc) {
		data_->refs.fetch_sub(1, std::memory_order_release);
		throw std::runtime_error("Write operation timed out");
	}

	if (IsShutdown() || data_->shutdown) {
		shutdown_ = true;
		data_->refs.fetch_sub(1, std::memory_order_release);
		throw std::runtime_error("Socket is shutdown");
	}

	uint32_t currentwritepos = data_->writepos.load(std::memory_order_acquire);

	size_t buffersize = sizeof(Data::msg);

	WriteInLength(len, currentwritepos, buffersize);

	// 复制数据到缓冲区
	if (currentwritepos + len <= buffersize) {
		// 直接复制
		std::memcpy(data_->msg + currentwritepos, message, len);
	} else {
		// 分段复制（跨越缓冲区边界）
		size_t firstchunk = buffersize - currentwritepos;
		size_t secondchunk = len - firstchunk;

		std::memcpy(data_->msg + currentwritepos, message, firstchunk);
		std::memcpy(data_->msg, message + firstchunk, secondchunk);
	}
	int32_t newwritepos = (currentwritepos + len) % sizeof(Data::msg);
	data_->writepos.store(newwritepos, std::memory_order_release);
	data_->rcondition.notify_one();
	data_->refs.fetch_sub(1, std::memory_order_release);
}

void SharedMemSocket::Read(std::shared_ptr<MemoryPool> receivePool, std::vector<std::pair<char*, uint32_t>>& messages, char*& buffer)
{
	data_->refs.fetch_add(1, std::memory_order_release);


	scoped_lock<interprocess_mutex>lock(data_->rmutex);
	data_->rcondition.wait(lock,[this](){
		return  data_->readpos.load(std::memory_order_acquire) != data_->writepos.load(std::memory_order_acquire) || 
			data_->shutdown;
		}
	);
	// 先验本地，再验共享内存，保持一致
	if (IsShutdown() || data_->shutdown) {
		shutdown_ = true;
	}

	int32_t currentreadpos = data_->readpos.load(std::memory_order_acquire);
	int32_t currentwritepos = data_->writepos.load(std::memory_order_acquire);

	if (currentreadpos == currentwritepos) {
		data_->refs.fetch_sub(1, std::memory_order_release);
		return;
	}

	size_t buffersize = sizeof(Data::msg);

	size_t dataavailable = (currentwritepos >= currentreadpos)
		? (currentwritepos - currentreadpos)
		: (buffersize - currentreadpos + currentwritepos);

	bool ret = receivePool->getBuffer(dataavailable, buffer);
	if (!ret) {
		data_->refs.fetch_sub(1, std::memory_order_release);
		throw std::bad_alloc();
	}

	// 读取数据
	if (currentreadpos + dataavailable <= buffersize) {
		// 直接读取
		std::memcpy(buffer, data_->msg + currentreadpos, dataavailable);
	} else {
		// 分段读取（跨越缓冲区边界）
		size_t firstchunk = buffersize - currentreadpos;
		size_t secondchunk = dataavailable - firstchunk;

		std::memcpy(buffer, data_->msg + currentreadpos, firstchunk);
		std::memcpy(buffer + firstchunk, data_->msg, secondchunk);
	}

	int32_t newreadpos = (currentreadpos + dataavailable) % sizeof(Data::msg);
	data_->readpos.store(newreadpos, std::memory_order_release);
	data_->wcondition.notify_one();
	
	int32_t len = dataavailable;
	char* temp = buffer;
	while (len > 0) {
		Message_t&  msg = *reinterpret_cast<Message_t*>(temp);
		messages.push_back(std::pair<char*, uint32_t>(msg.data, msg.dataSize));

		temp += (msg.dataSize + sizeof(Message_t::dataSize));
		len -= (msg.dataSize + sizeof(Message_t::dataSize));
	}

	data_->refs.fetch_sub(1, std::memory_order_release);
}

void SharedMemSocket::AsyncRead(std::shared_ptr<MemoryPool> receivePool, std::function<void(const std::vector<std::pair<char*, uint32_t>>&, char*&, const std::exception*)> callback)
{
	asio::post(pool_,
		[this, receivePool, callback = std::move(callback)]() mutable {
			char* msg = nullptr;
			std::vector<std::pair<char*, uint32_t>> messages;
			try {
				Read(receivePool, messages, msg);
				callback(messages, msg, nullptr);
			} catch (const std::exception& e) {
				callback(messages, msg, &e);
			}
		}
	);
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
	}
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
	auto start = std::chrono::steady_clock::now();
	while (data_ && data_->refs.load(std::memory_order_acquire) != 0) {
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

		if (elapsed >= std::chrono::seconds(3)) {
			break; // 准备强制关闭
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
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

	}

	CleanupResources();
	isClosed_ = true;
}

void SharedMemSocket::WriteInLength(uint32_t len, uint32_t& currentwritepos, size_t buffersize)
{
	// 复制数据到缓冲区
	if (currentwritepos + sizeof(uint32_t) <= buffersize) {
		// 直接复制
		std::memcpy(data_->msg + currentwritepos, (char*)&len, sizeof(uint32_t));
	} else {
		// 分段复制（跨越缓冲区边界）
		size_t firstchunk = buffersize - currentwritepos;
		size_t secondchunk = sizeof(uint32_t) - firstchunk;

		std::memcpy(data_->msg + currentwritepos, (char*)&len, firstchunk);
		std::memcpy(data_->msg, (char*)&len + firstchunk, secondchunk);
	}

	currentwritepos = (currentwritepos + sizeof(uint32_t)) % sizeof(Data::msg);
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
	}
}



bool SharedMemSocket::IsAllow(const size_t& len) const
{
	if (len == 0 || len > SHM_MAX_MESSAGE_SIZE) {
		return false;
	}

	size_t currentreadpos = data_->readpos.load(std::memory_order_acquire);
	size_t currentwritepos = data_->writepos;

	size_t buffersize = sizeof(Data::msg);

	// 计算可用空间
	size_t availablespace = (currentwritepos >= currentreadpos)
		? buffersize - (currentwritepos - currentreadpos)
		: currentreadpos - currentwritepos;

	return (len + sizeof(uint32_t)) <= availablespace;
}
