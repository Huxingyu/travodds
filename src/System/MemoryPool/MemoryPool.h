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
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H
#include <set>
#include "BasicType.h"

AGIDDS_NAMESPACE_BEGIN

struct NodeInfo
{
	unsigned int dataSize = 0;
	unsigned int dataIndex = 0;
	octet data[1];
	bool operator<(const NodeInfo& other) const
	{
		return dataSize < other.dataSize;
	}
};

class BufferNode {
public:
	BufferNode(unsigned int size);

	~BufferNode();

	bool resize(unsigned int size);

	NodeInfo& info();

	static NodeInfo& info(octet* data);

private:
	octet* buffer_ = nullptr;

	static constexpr unsigned int offset_ = offsetof(NodeInfo, data);
};

class MemoryPool {
public:
	~MemoryPool();

	static MemoryPool& GetInstance();

	bool getBuffer(unsigned int size, char*& buffer);

	bool releaseBuffer(char*& buffer);

protected:
	MemoryPool() = default;

private: /* yuguxu 此范围的方法都是调用域已经加锁 */
	BufferNode* allocate(unsigned int size);
	/* 查找最适合的节点 */
	BufferNode* findBestFit(unsigned int size);

private:
	uint32_t maxPoolSize_ = 5;

	std::multiset<NodeInfo> freeBuffers_;
	std::vector<BufferNode*> allocatedBuffers_;
	std::mutex mutex_;
};


AGIDDS_NAMESPACE_END
#endif
