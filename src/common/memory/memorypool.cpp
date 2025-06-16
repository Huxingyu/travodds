#include "common/memory/memorypool.h"

USING_AGIDDS_NAMESPACE;
MemoryPool::~MemoryPool()
{
	for (BufferNode* node : allocatedBuffers_) {
		delete node;
	}
}

std::shared_ptr<MemoryPool> MemoryPool::GetInstance()
{
	static std::shared_ptr<MemoryPool> instance_(new MemoryPool);
	return instance_;
}

bool MemoryPool::getBuffer(uint32_t size, char*& buffer)
{
	BufferNode* node = nullptr;
	std::unique_lock<std::mutex> lock(mutex_);
	if (freeBuffers_.empty()) {
		node = allocate(size);
		if (!node) {
			lock.unlock();
			buffer = nullptr;
			return false;
		}
	} else {
		node = findBestFit(size);
	}

	if (size > node->info().dataSize) {
		if (!node->resize(size)) {
			freeBuffers_.insert(node->info());
			lock.unlock();
			buffer = nullptr;
			return false;
		}
	}
	lock.unlock();
	node->info().onUsing = true;
	buffer = (char*)node->info().data;

	return true;
}
bool MemoryPool::releaseBuffer(char*& buffer)
{

	if (!buffer || false == BufferNode::info((octet*)buffer).onUsing) {
		return false;
	}
	uint32_t index = BufferNode::info((octet*)buffer).dataIndex;
	//memset(BufferNode::info((octet*)buffer).data, 0, BufferNode::info((octet*)buffer).dataSize);
	{
		std::lock_guard<std::mutex> lock(mutex_);
		//printf("releaseBuffer : %p\n", buffer);
		if (index >= allocatedBuffers_.size()) {
			return false;
		}
		BufferNode* node = allocatedBuffers_.at(index);
		freeBuffers_.insert(node->info());
		node->info().onUsing = false;
	}
	buffer = nullptr;
	return true;
}

BufferNode* MemoryPool::allocate(uint32_t size)
{
	if (allocatedBuffers_.size() >= maxPoolSize_) {
		return nullptr;
	}
	BufferNode* buffer = new (std::nothrow) BufferNode(size);
	if (buffer) {
		buffer->info().dataIndex = uint32_t(allocatedBuffers_.size());
		allocatedBuffers_.push_back(buffer);
	}
	return buffer;
}

BufferNode* MemoryPool::findBestFit(uint32_t size)
{
	// 使用 lower_bound 找到第一个大于或等于 size_ 的节点
	NodeInfo node = { size, 0 };
	auto it = freeBuffers_.lower_bound(node);

	if (it == freeBuffers_.end()) {
		it--;
	}
	uint32_t index = (*it).dataIndex;
	freeBuffers_.erase(it);
	return allocatedBuffers_.at(index);
}

BufferNode::BufferNode(uint32_t size)
{
	if (!size) {//==0
		buffer_ = (octet*)calloc(sizeof(NodeInfo), sizeof(octet));
	} else {
		buffer_ = (octet*)calloc(size + sizeof(NodeInfo) - 1, sizeof(octet));
	}
	if (buffer_ == nullptr) {
		throw std::bad_alloc();
	}
	NodeInfo* info = new (buffer_) NodeInfo;
	info->dataSize = size;
}

BufferNode::~BufferNode()
{
	info().~NodeInfo();
	free(buffer_);
}

bool BufferNode::resize(uint32_t size)
{
	octet* newBuffer = (octet*)realloc(buffer_, size_t(size) + offset_);
	if (!newBuffer) {
		return false;
	}
	buffer_ = newBuffer;
	memset(buffer_ + offset_ + info().dataSize, 0, (size - info().dataSize) * sizeof(octet));
	info().dataSize = size;
	return true;
}

NodeInfo& BufferNode::info()
{
	return *reinterpret_cast<NodeInfo*>(buffer_);
}

NodeInfo& BufferNode::info(octet* data)
{
	return *reinterpret_cast<NodeInfo*>(data - offset_);
}
