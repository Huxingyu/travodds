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

/**
 * @brief NodeInfo结构体,内存池节点信息数据结构。
 * 与BufferNode类配合使用构成每个内存池的基本节点。
 */
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

/**
 * @brief BufferNode类,内存池基础节点。
 * 与NodeInfo结构体配合使用构成每个内存池的基本节点。
 */
class BufferNode {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] size 当前内存池节点需要申请的空间大小。
	 */
	BufferNode(uint32_t size);

	/**
	 * @brief 析构函数。
	 */
	~BufferNode();

	/**
	 * @brief 内存池节点空间大小重置函数。
	 *  根据输入的空间大小值重新申内存空间并将旧的内存复制到新的内存。
	 *
	 * @param [in] size  当前内存池节点需要申请的新空间大小。
	 *
	 * @return 重新设置成功返回true，否则返回false。
	 */
	bool resize(uint32_t size);

	/**
	 * @brief 从当前节点内存指针中获取内存节点信息。
	 *
	 * @return 内存节点信息。
	 */
	NodeInfo& info();

	/**
	 * @brief 获取内存节点data的内存节点信息。
	 *
	 * @param [in] data 内存池节点。
	 * 
	 * @return 内存节点信息。
	 */
	static NodeInfo& info(octet* data);

private:
	octet* buffer_ = nullptr;

	static constexpr uint32_t offset_ = offsetof(NodeInfo, data);
};

/**
 * @brief MemoryPool类,内存池类。
 * 由NodeInfo结构体和BufferNode类配合构成内存池。
 */
class MemoryPool {
public:
	/**
	 * @brief 构造函数。
	 *
	 * @param [in] maxSize 内存池可容纳内存节点最大值。
	 */
	MemoryPool(uint32_t maxSize = ~0) : maxPoolSize_(maxSize) {};
	
	/**
	 * @brief 析构函数。
	 */
	~MemoryPool();

	/**
	 * @brief 内存池类指针获取函数。
	 *  获取新的内存池指针。
	 *
	 * @return 内存池指针。
	 */
	static std::shared_ptr<MemoryPool> GetInstance();

	/**
	 * @brief 内存池节点分配函数。
	 *  根据输入的空间大小值寻找适合的空闲节点并将buffer指向该节点空间方便之后赋值。
	 *  当输入的空间大小无法满足时重新申请新的空间使用。
	 *
	 * @param [in]  size    当前内存池节点需要申请的新空间大小。
	 * @param [out] buffer  当前内存池节点指针。
	 *
	 * @return 设置成功返回true，否则返回false。
	 */
	bool getBuffer(uint32_t size, char*& buffer);

	/**
	 * @brief 内存池节点回收函数。
	 *  将buffer指针对应的内存节点空间回收到空闲节点列表中等待下次使用。
	 *
	 * @param [in] buffer  当前要回收的内存池节点指针。
	 *
	 * @return 回收成功返回true，否则返回false。
	 */
	bool releaseBuffer(char*& buffer);

private: 
	/**
	 * @brief 内存池节点申请函数。
	 * 
	 * 此范围的方法都是调用域已经加锁
	 *
	 * @param [in] size  当前要申请的内存池节点大小。
	 *
	 * @return 申请成功的内存节点指针。
	 */
	BufferNode* allocate(uint32_t size);
	/**
	 * @brief 查找最适合的节点。
	 *
	 * @param [in] size  当前要查找的内存池节点大小。
	 *
	 * @return 查找成功的内存节点指针。
	 */
	BufferNode* findBestFit(uint32_t size);

private:
	uint32_t maxPoolSize_ = 0;
	std::multiset<NodeInfo> freeBuffers_;
	std::vector<BufferNode*> allocatedBuffers_;
	std::mutex mutex_;
};


AGIDDS_NAMESPACE_END
#endif
