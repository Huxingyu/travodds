# Boost.Interprocess 共享段后端

本次迁移复用 `src/rtps/transport/interprocesstype.h` 的命名空间和设备类型。默认 `/dev/shm`（按目录设备/inode 判定）使用 `shared_memory_object(create_only/open_only)`、`truncate`、`mapped_region` 和 `shared_memory_object::remove`；句柄关闭和解除映射由 Boost RAII 析构完成。生产共享池已无独立 openat/mmap/munmap 后端。

可配置的其他目录使用 Boost 1.88 的 `ipcdetail::file_wrapper` 设备与同一个 `mapped_region`。该设备也是 Boost managed_mapped_file 所用的底层设备，负责创建、打开、调整大小、关闭与删除。仓库原 Boost 子集缺少此头文件，现按官方 boost-1.88.0 原文补入，保留 Boost License，下载来源及 SHA256 在 `artifacts/boost-migration/boost-file-wrapper-origin.json`。这是版本固定的 Boost 内部接口依赖；升级 Boost 时必须一起核验。不会把全局 Boost 共享目录改掉影响旧 SHM。

选择比较：`managed_shared_memory` 适用于段内动态对象和 offset_ptr 容器，但其 segment manager/分配器带额外元数据与内部锁，进程异常时无法据名称推断修复语义。当前固定槽位存储使用静态预算和偏移，采用设备加 mapped_region 无需段内分配；reservation 的绝对位置与实际 publish 历史分开管理，后者在共享 Slot 中形成闭合双向环，正常 Reader/replay 沿环读取。仍被 Writer/pending/Reader Loan pin 固定的项不能覆盖，取消 Loan 不加入历史。所有共享字节仍由 Header、Slot 及按类型对齐取整的载荷组成，不保存普通指针；Boost 对象只存在进程本地。当前共享布局 v7 先校验前缀/版本/大小/会话/权限/PID namespace，再访问同步对象，拒绝旧布局。

v7 将载荷区起点单独按页对齐，仅此 padding 最多增加 page_size-1 字节；新增环字段与全部载荷仍按实际文件大小计入预算，没有 managed allocator 隐藏开销。每个 Reader 的一个 Boost mapped_region 在校验后将完整 payload span 设为 PROT_READ，控制 metadata 保持 RW；Writer 创建的映射仍为 RW。保护失败拒绝该映射。生产只读页和测试 PROT_NONE 取证并非同一机制，完整不变量和待验证范围见 [环与载荷设计](DATASHARING-RING.md)。

POSIX 辅助调用仅用于目录 fd、flock 初始化屏障、fstat/pread 身份与权限校验、posix_fallocate 实际空间预留、随机会话、进程身份和同步。它们不是共享段创建/映射后端。目录先以 O_NOFOLLOW 固定；配置目录设备通过 /proc/self/fd 的路径交给 Boost，打开后验证名字仍为同一个普通文件，拒绝符号链接。关闭/孤儿清理仅删除匹配 inode 的确切会话。

当前数据面使用原子 slot/registration gate 与 persistent pin；预留、发布、借用、归还及 ready 注册快照不获取管理 mutex。通知发布增加 always-lock-free 的 32 位原子代次（release）；等待先采样代次再检查数据谓词，由 FUTEX_WAIT_BITSET 比较并休眠，FUTEX_WAKE 唤醒。管理注册与维护仍使用 PTHREAD_PROCESS_SHARED/ROBUST mutex，统计维护以 try-lock 忙返回 TIMEOUT。

控制锁暂停不阻止已有数据面，不等于整个 DDS write 路径形式化 lock-free：本地 Writer/history/发现锁和 futex 系统调用仍存在。管理锁持有者死亡，或死亡登记遗留不确定的短操作计数时，session 进入 CORRUPT，旧 Loan 保留映射和已发布字节，恢复需新会话。仅在确认 PID/start identity 死亡且短操作状态可安全回收时清 pin。详细协议见 [生产后端报告](DATASHARING-PRODUCTION-20261030.md)。

既有 InterprocessType::robust_mutex 仅探测 PID，未取得底层 mutex 就可能返回成功，且没有 EOWNERDEAD 事务处理，故不用于新共享池。无锁通知和 pthread robust 元数据锁与 Boost 映射职责分开。

迁移阶段历史验收入口为 `artifacts/completion-20261028/`，迁移前快照在 `artifacts/boost-migration/pre-boost/`。布局 v6 / discovery v2 的前阶段证据在 `artifacts/production-lockfree-20261030/`，可选 key-only 阶段的 484 项及 93.839% 见 [同日历史报告](DATASHARING-COMPLETION-20261030.md)。10/30 v6/v3 的强制带键 key-only 证据保留于 `artifacts/strict-native-20261030/`；10/31 前轮 v6/v4 无初始化准入结果保留于 [前轮报告](DATASHARING-PROGRESS-20261031.md)。当前 v7/v5 仍在验证，见 [本轮设计](DATASHARING-RING.md) 与 [验收矩阵](DATASHARING-ACCEPTANCE.md)。每个结果只证明其冻结源码对应版本，旧覆盖率和性能不能代替当前测量。


补入头文件的可移植来源记录（不依赖本机artifacts）：

- 版本/路径：Boost.Interprocess `boost-1.88.0/include/boost/interprocess/detail/file_wrapper.hpp`。
- 原文：<https://raw.githubusercontent.com/boostorg/interprocess/boost-1.88.0/include/boost/interprocess/detail/file_wrapper.hpp>。
- SHA256：`3f72074a5edab39f87cd2b9f0a67f5fbe41a15356cabe3e321ab652d8d80058e`。
- 本地文件：`thirdparty/boost/interprocess/detail/file_wrapper.hpp`，保留上游Boost许可证。


当前严格补全轮采用共享布局 v7 / discovery v5，新增真实发布历史环与 Reader 全载荷只读映射，已完成本轮验证，设计和实际证据见 [环形历史与载荷写入说明](DATASHARING-RING.md)。前轮 v6/v4 的 321 项冻结输入对应 529 项普通回归、9 次示例、93.936% 覆盖率和两个检查版各 86 项，原始范围保留于 [前轮报告](DATASHARING-PROGRESS-20261031.md)；这些数字不能作为本轮新源码通过结果。[10/30 报告](DATASHARING-STRICT-20261030.md) 继续保留更早 v3 的历史。LSan 未启用，历史 WaitSet 崩溃根因仍未知。
