# Boost.Interprocess 共享段后端

本次迁移复用 `src/rtps/transport/interprocesstype.h` 的命名空间和设备类型。默认 `/dev/shm`（按目录设备/inode 判定）使用 `shared_memory_object(create_only/open_only)`、`truncate`、`mapped_region` 和 `shared_memory_object::remove`；句柄关闭和解除映射由 Boost RAII 析构完成。生产共享池已无独立 openat/mmap/munmap 后端。

可配置的其他目录使用 Boost 1.88 的 `ipcdetail::file_wrapper` 设备与同一个 `mapped_region`。该设备也是 Boost managed_mapped_file 所用的底层设备，负责创建、打开、调整大小、关闭与删除。仓库原 Boost 子集缺少此头文件，现按官方 boost-1.88.0 原文补入，保留 Boost License，下载来源及 SHA256 在 `artifacts/boost-migration/boost-file-wrapper-origin.json`。这是版本固定的 Boost 内部接口依赖；升级 Boost 时必须一起核验。不会把全局 Boost 共享目录改掉影响旧 SHM。

选择比较：`managed_shared_memory` 适用于段内动态对象和 offset_ptr 容器，但其 segment manager/分配器带额外元数据与内部锁，进程异常时无法据名称推断修复语义。当前环形槽位已有静态预算和偏移，采用设备加 mapped_region 无需段内分配。所有共享字节仍由 Header、Slot 及按原生对齐取整的载荷组成，不保存普通指针；Boost 对象只存在进程本地。预算为实际文件大小，无隐藏 managed allocator 开销；最大对齐为页尺寸。版本升至 5，先校验前缀/版本/大小/会话/权限/PID namespace，再访问共享锁，拒绝历史版本。

POSIX 辅助调用仅用于目录 fd、flock 初始化屏障、fstat/pread 身份与权限校验、posix_fallocate 实际空间预留、随机会话、进程身份和同步。它们不是共享段创建/映射后端。目录先以 O_NOFOLLOW 固定；配置目录设备通过 /proc/self/fd 的路径交给 Boost，打开后验证名字仍为同一个普通文件，拒绝符号链接。关闭/孤儿清理仅删除匹配 inode 的确切会话。

通知分三层：发布增加 always-lock-free 的 32 位原子代次（release）；等待在 metadata mutex 下 acquire 采样后解锁，内核 FUTEX_WAIT_BITSET 比较代次并休眠，FUTEX_WAKE 唤醒，避免解锁到休眠间丢通知；槽位与引用计数事务仍由 PTHREAD_PROCESS_SHARED/ROBUST mutex 保护。只称通知代次发布无锁，不称分配、等待或整个共享池无锁。持锁者死亡使段 CORRUPT，停止新借用；旧 Loan 继续固定映射。死 Reader 的引用仅在 PID 和启动身份确认死亡后回收。保留此前真实 SIGKILL 后条件变量挂死的故障回归。

既有 InterprocessType::robust_mutex 仅探测 PID，未取得底层 mutex 就可能返回成功，且没有 EOWNERDEAD 事务处理，故不用于新共享池。无锁通知和 pthread robust 元数据锁与 Boost 映射职责分开。

本轮最终验收入口为 `artifacts/completion-20261028/`；新结果只以冻结源码清单对应的日志和退出码为准。迁移前快照在 `artifacts/boost-migration/pre-boost/`，不得引用旧覆盖率或性能代表本后端。


补入头文件的可移植来源记录（不依赖本机artifacts）：

- 版本/路径：Boost.Interprocess `boost-1.88.0/include/boost/interprocess/detail/file_wrapper.hpp`。
- 原文：<https://raw.githubusercontent.com/boostorg/interprocess/boost-1.88.0/include/boost/interprocess/detail/file_wrapper.hpp>。
- SHA256：`3f72074a5edab39f87cd2b9f0a67f5fbe41a15356cabe3e321ab652d8d80058e`。
- 本地文件：`thirdparty/boost/interprocess/detail/file_wrapper.hpp`，保留上游Boost许可证。


本轮最终验收：普通与ASan/UBSan各468/468，两版各55项×30轮及三个有限示例各30轮通过；新增生产行覆盖率94.118%（2432/2584）。独立TSan限定路径通过，LSan未运行、历史WaitSet根因未知。当前完整证据与限制以 [本轮报告](DATASHARING-COMPLETION-20261028.md) 为准，早期数字只保留历史含义。
