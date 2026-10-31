# Boost.Interprocess 共享段后端

本次迁移复用 `src/rtps/transport/interprocesstype.h` 的命名空间和设备类型。默认 `/dev/shm`（按目录设备/inode 判定）使用 `shared_memory_object(create_only/open_only)`、`truncate`、`mapped_region` 和 `shared_memory_object::remove`；句柄关闭和解除映射由 Boost RAII 析构完成。生产共享池已无独立 openat/mmap/munmap 后端。

可配置的其他目录使用 Boost 1.88 的 `ipcdetail::file_wrapper` 设备与同一个 `mapped_region`。该设备也是 Boost managed_mapped_file 所用的底层设备，负责创建、打开、调整大小、关闭与删除。仓库原 Boost 子集缺少此头文件，现按官方 boost-1.88.0 原文补入，保留 Boost License，下载来源及 SHA256 在 `artifacts/boost-migration/boost-file-wrapper-origin.json`。这是版本固定的 Boost 内部接口依赖；升级 Boost 时必须一起核验。不会把全局 Boost 共享目录改掉影响旧 SHM。

选择比较：`managed_shared_memory` 适用于段内动态对象和 offset_ptr 容器，但其 segment manager/分配器带额外元数据与内部锁，进程异常时无法据名称推断修复语义。当前环形槽位已有静态预算和偏移，采用设备加 mapped_region 无需段内分配。所有共享字节仍由 Header、Slot 及按原生对齐取整的载荷组成，不保存普通指针；Boost 对象只存在进程本地。预算为实际文件大小，无隐藏 managed allocator 开销；最大对齐为页尺寸。当前共享布局为 v6，先校验前缀/版本/大小/会话/权限/PID namespace，再访问共享锁，拒绝历史版本。

POSIX 辅助调用仅用于目录 fd、flock 初始化屏障、fstat/pread 身份与权限校验、posix_fallocate 实际空间预留、随机会话、进程身份和同步。它们不是共享段创建/映射后端。目录先以 O_NOFOLLOW 固定；配置目录设备通过 /proc/self/fd 的路径交给 Boost，打开后验证名字仍为同一个普通文件，拒绝符号链接。关闭/孤儿清理仅删除匹配 inode 的确切会话。

当前数据面使用原子 slot/registration gate 与 persistent pin；预留、发布、借用、归还及 ready 注册快照不获取管理 mutex。通知发布增加 always-lock-free 的 32 位原子代次（release）；等待先采样代次再检查数据谓词，由 FUTEX_WAIT_BITSET 比较并休眠，FUTEX_WAKE 唤醒。管理注册与维护仍使用 PTHREAD_PROCESS_SHARED/ROBUST mutex，统计维护以 try-lock 忙返回 TIMEOUT。

控制锁暂停不阻止已有数据面，不等于整个 DDS write 路径形式化 lock-free：本地 Writer/history/发现锁和 futex 系统调用仍存在。管理锁持有者死亡，或死亡登记遗留不确定的短操作计数时，session 进入 CORRUPT，旧 Loan 保留映射和已发布字节，恢复需新会话。仅在确认 PID/start identity 死亡且短操作状态可安全回收时清 pin。详细协议见 [生产后端报告](DATASHARING-PRODUCTION-20261030.md)。

既有 InterprocessType::robust_mutex 仅探测 PID，未取得底层 mutex 就可能返回成功，且没有 EOWNERDEAD 事务处理，故不用于新共享池。无锁通知和 pthread robust 元数据锁与 Boost 映射职责分开。

迁移阶段历史验收入口为 `artifacts/completion-20261028/`，迁移前快照在 `artifacts/boost-migration/pre-boost/`。布局 v6 / discovery v2 的前阶段证据在 `artifacts/production-lockfree-20261030/`，可选 key-only 阶段的 484 项及 93.839% 见 [同日历史报告](DATASHARING-COMPLETION-20261030.md)。当前共享布局仍为 v6，发现能力升到 v3，带键 native 强制有效 key-only 元数据并移除原生全样本快照回退；新证据使用 `artifacts/strict-native-20261030/`，进度见 [严格准入报告](DATASHARING-STRICT-20261030.md)，官方对应关系见 [验收矩阵](DATASHARING-ACCEPTANCE.md)。每个结果只证明其冻结源码对应版本，旧覆盖率和性能不能代替当前测量。


补入头文件的可移植来源记录（不依赖本机artifacts）：

- 版本/路径：Boost.Interprocess `boost-1.88.0/include/boost/interprocess/detail/file_wrapper.hpp`。
- 原文：<https://raw.githubusercontent.com/boostorg/interprocess/boost-1.88.0/include/boost/interprocess/detail/file_wrapper.hpp>。
- SHA256：`3f72074a5edab39f87cd2b9f0a67f5fbe41a15356cabe3e321ab652d8d80058e`。
- 本地文件：`thirdparty/boost/interprocess/detail/file_wrapper.hpp`，保留上游Boost许可证。


当前严格版本（共享布局 v6 / discovery v3）：**500/500 回归、三个示例各 3 轮共 9 次执行通过，新增生产可执行行覆盖率 93.920%（2765/2944）**。本次独立完整插桩 ASan/UBSan 与 TSan focused 各 57/57 通过，最终汇总审计通过；319 项输入身份、库/结果哈希及检查范围见 [严格准入报告](DATASHARING-STRICT-20261030.md)；[同日旧报告](DATASHARING-COMPLETION-20261030.md) 仅属 discovery v2 历史。LSan 无结论，历史 WaitSet 崩溃根因仍未知。
