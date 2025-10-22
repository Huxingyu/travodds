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
#ifndef TRAVODDS_RTPS_TRANSPORT_SHAREDMEMSOCKET_H
#define TRAVODDS_RTPS_TRANSPORT_SHAREDMEMSOCKET_H

#include <functional>
#include <asio.hpp>
#include "rtps/transport/interprocesstype.h"

TRAVODDS_NAMESPACE_BEGIN

class SharedMemAccept;
class MemoryPool;

/*
 * yuguxu
 * 模拟点对点socket连接
 * 读和写的操作只会分别在两个节点
 * 不存在读或写的操作会发生在两个进程
 */
class SharedMemSocket {
public:
    SharedMemSocket();
    ~SharedMemSocket();

    bool Connect(const std::string& srcport, const std::string& destport);

    bool Connect(const std::string& destport);

    void Write(const char* message, size_t len);

    void Read(std::shared_ptr<MemoryPool> receivePool, std::vector<std::pair<char*, uint32_t>>& messages, char*& buffer);

    void AsyncRead(std::shared_ptr<MemoryPool> receivePool, std::function<void(const std::vector<std::pair<char*, uint32_t>>&, char*&,const std::exception*)> callback);

    void Shutdown();

    bool IsShutdown() const;

    void Close();
private:
    // 将长度先写入共享内存缓存
    void WriteInLength(uint32_t len, uint32_t& currentwritepos, size_t buffersize);
    void CleanupResources();

	bool IsAllow(const size_t& len) const;  

private:
    asio::thread_pool pool_;
    uint32_t port_;
    std::string acceptPort_;
    std::string sharedMemoryName_;
    const char* objectName_ = "TravoddsTransporType";
    // 与共享内存保持一致
    bool shutdown_ = false;
   
    std::unique_ptr<InterprocessType::managed_shared_memory> segment_;

    struct Message_t {
        uint32_t dataSize = 0;
        char data[1];
    };

    struct Data {
        // 读写锁、条件等待集
        InterprocessType::interprocess_mutex rmutex;
        InterprocessType::interprocess_condition rcondition;
        InterprocessType::interprocess_mutex wmutex;
        InterprocessType::interprocess_condition wcondition;

		// 当前读取位置
        std::atomic<uint32_t> readpos = 0;
		// 当前写入位置
        std::atomic<uint32_t> writepos = 0;
        // 读写引用记数
        std::atomic<uint32_t> refs = 0;
        bool shutdown = false;
        //缓存实际长度是允许单次写入的两倍
        char msg[SHM_MAX_MESSAGE_SIZE * 2];
        Data() : msg{0} {};
    };
    Data* data_;
    size_t sharedMemSize_ = 1024 + sizeof(Data);

    friend SharedMemAccept;
};

TRAVODDS_NAMESPACE_END



#endif
