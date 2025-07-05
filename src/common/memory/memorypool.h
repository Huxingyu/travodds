//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef AGIDDS_COMMON_MEMORY_MEMORYPOOL_H
#define AGIDDS_COMMON_MEMORY_MEMORYPOOL_H

#include "type/basictype.h"

AGIDDS_NAMESPACE_BEGIN

struct NodeInfo
{
	uint32_t dataSize = 0;
	uint32_t dataIndex = 0;
	bool onUsing = false;
	octet data[1];
	bool operator<(const NodeInfo& other) const
	{
		return dataSize < other.dataSize;
	}
};

class BufferNode {
public:
	BufferNode(uint32_t size);

	~BufferNode();

	bool resize(uint32_t size);

	NodeInfo& info();

	static NodeInfo& info(octet* data);

private:
	octet* buffer_ = nullptr;

	static constexpr uint32_t offset_ = offsetof(NodeInfo, data);
};

class MemoryPool {
public:
	MemoryPool(uint32_t maxSize = ~0) : maxPoolSize_(maxSize) {};

	~MemoryPool();

	static std::shared_ptr<MemoryPool> GetInstance();

	bool getBuffer(uint32_t size, char*& buffer);

	bool releaseBuffer(char*& buffer);

private: /* yuguxu 此范围的方法都是调用域已经加锁 */
	BufferNode* allocate(uint32_t size);
	/* 查找最适合的节点 */
	BufferNode* findBestFit(uint32_t size);

private:
	uint32_t maxPoolSize_ = 0;
	std::multiset<NodeInfo> freeBuffers_;
	std::vector<BufferNode*> allocatedBuffers_;
	std::mutex mutex_;
};


AGIDDS_NAMESPACE_END
#endif
