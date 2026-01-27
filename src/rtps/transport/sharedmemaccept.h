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
#ifndef TRAVODDS_RTPS_TRANSPORT_SHAREDMEMACCEPT_H
#define TRAVODDS_RTPS_TRANSPORT_SHAREDMEMACCEPT_H

#include <asio.hpp>
#include <functional>
#include "rtps/transport/interprocesstype.h"


TRAVODDS_NAMESPACE_BEGIN
class SharedMemSocket;

class SharedMemAccept {
public:
    SharedMemAccept(const std::string& port);
    virtual ~SharedMemAccept();
    
	void Accept(std::shared_ptr<SharedMemSocket> socket);

    void AsyncAccept(std::shared_ptr<SharedMemSocket> socket, std::function<void(const std::exception*)> callback);

    void Close();

    void Shutdown();

    bool IsShutdown() const;

private:
    void CleanupResources();
private:
    asio::thread_pool pool_;
    std::string port_;
    std::string sharedMemoryName_;
    const char* objectName_;
    size_t sharedMemSize_ = 20 * 1024;

    bool shutdown_ = false;

    bool isClosed_ = false;

    std::atomic<uint32_t> refs_;

    std::unique_ptr<InterprocessType::managed_shared_memory> segment_;

    InterprocessType::SharedAcceptList* sharedAcceptList_;

    std::map<uint32_t, std::shared_ptr<SharedMemSocket>> sockets_;
};

TRAVODDS_NAMESPACE_END



#endif
