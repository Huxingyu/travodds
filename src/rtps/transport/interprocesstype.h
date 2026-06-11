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
#ifndef TRAVODDS_RTPS_TRANSPORT_INTERPROCESSTYPE_H
#define TRAVODDS_RTPS_TRANSPORT_INTERPROCESSTYPE_H

#include "travodds_dll.h"
#include "type/basictype.h"
#include "common/sys/sysinfo.h"

#include "boost/interprocess/allocators/allocator.hpp"
#include "boost/interprocess/containers/map.hpp"
#include "boost/interprocess/managed_shared_memory.hpp"
#include "boost/interprocess/sync/named_mutex.hpp"
#include "boost/interprocess/sync/named_condition.hpp"
#include "boost/interprocess/sync/scoped_lock.hpp"
#include "boost/interprocess/sync/interprocess_mutex.hpp"
#include "boost/interprocess/sync/interprocess_condition.hpp"
#include "boost/interprocess/sync/interprocess_condition_any.hpp"
TRAVODDS_NAMESPACE_BEGIN

namespace InterprocessType {
    namespace ipcdetail = boost::interprocess::ipcdetail;

    using interprocess_exception = boost::interprocess::interprocess_exception;

    using managed_shared_memory = boost::interprocess::managed_shared_memory;

    using named_condition = boost::interprocess::named_condition;

    using named_mutex = boost::interprocess::named_mutex;

	using shared_memory_object = boost::interprocess::shared_memory_object;

    using interprocess_mutex = boost::interprocess::interprocess_mutex;

    using interprocess_condition = boost::interprocess::interprocess_condition;

    using interprocess_condition_any = boost::interprocess::interprocess_condition_any;

    using boost::interprocess::scoped_lock;

    using boost::interprocess::create_only;

	using boost::interprocess::open_only;

    using boost::interprocess::open_or_create;

    struct robust_mutex {
        interprocess_mutex mutex;
        int32_t hold_pid = 0;      // 当前持有锁的进程ID

        // 尝试获取锁，崩溃转移
        bool try_lock() {
            if (mutex.try_lock_for(std::chrono::milliseconds(500)) ||
                !SysInfo::IsProcessExists(hold_pid)) {
                // 更新持有者信息
                hold_pid = SysInfo::GetProcessIDStatic();
                return true;
            }
            return false;
        }

        void lock() {
            while (!try_lock()) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }

        void unlock() {
            if (hold_pid == SysInfo::GetProcessIDStatic()) {
                hold_pid = 0;
                mutex.unlock();
            }
        }
    };


    // 共享map类型定义
    using SegmentManager = boost::interprocess::managed_shared_memory::segment_manager;
    using ShmMapAllocator = boost::interprocess::allocator<
        std::pair<const uint32_t, uint32_t>, SegmentManager>;
    using PortProcessMap = boost::interprocess::map<
        uint32_t, uint32_t, std::less<uint32_t>, ShmMapAllocator>;
    
    using PortIdentityMap = boost::interprocess::map<
        uint32_t, uint32_t,std::less<uint32_t>,
        ShmMapAllocator>;

    // 监听端口记录
    struct SharedList {
        robust_mutex mutex;
        uint32_t referenceCount;
        PortProcessMap portprocessidmap;
        SharedList(const ShmMapAllocator& alloc)
            : referenceCount(0), portprocessidmap(alloc) {
        }
    };
    //监听Socket记录
    struct SharedAcceptList {
        robust_mutex mutex;
        interprocess_condition_any condition;
        PortProcessMap sockets;
        PortIdentityMap srcPorts;
        bool shutdown;
        SharedAcceptList(const ShmMapAllocator& alloc)
            : sockets(alloc), srcPorts(alloc), shutdown(false) {
        }
    };
}

TRAVODDS_NAMESPACE_END

#endif