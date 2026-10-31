# 无锁共享池原型协议独立审查（2026-10-29）

## 审查结论

**建议保留生产 SharedPayloadPool，先交付独立原型和确定性对照实验；不建议把现有 publish 的 Guard 删除或将几个字段逐一改 atomic 后直接替换生产后端。**候选原型采用单个 64 位 control 打包 generation、pending、loaned、Writer pin 和 WRITING，并以全段发布水位控制按序扫描。经源码审查与有限状态模型检查，该方向能够避免已识别的 pin 转移空窗、陈旧归还跨代清位和并发扫描乱序问题。

这不是“整个 DDS 已实现无锁”的结论。冻结核心的范围是单 Writer 提交 lane、固定预注册的最多 15 个 Reader、每 Reader 的 take/discard 串行、固定容量、32 位代次退休、外部初始化握手和映射寿命管理。后续独立 Controller 在核心外补充有限动态注册/关闭及操作租约，详见末节；死参与者回收、实际控制请求传输、replay 与 DDS/QoS 集成仍未覆盖。

## 审查对象和证据

- 生产基线：`/home/ubuntu/project/travodds`，`main@3da8e077b9902142cdb4549eafa5b43114bdf936`。
- 候选：`/home/ubuntu/project/travodds/.worktrees/datasharing-progress/experimental/lockfree_pool/shared_pool.hpp`（初审 252 行，复查包含 producer-only next_slot 轮转）、`mapped_pool.hpp`、`test_pool.cpp` 与独立 exec 验证入口。
- 最终协议复查为实验 layout version 2；两个核心头文件的审查快照 SHA-256 保存在 `/tmp/travodds-lockfree-review-source-hashes-20261029.json`，后续变更需重新核对。
- 已完整核对先前独立审查报告、下一步推进计划，以及生产布局、publish、recycle、registerReader、waitNext、releaseReader、close/removeReader、Loan 所有权路径。
- 轻量模型脚本：`/tmp/travodds-lockfree-model-20261029.py`；结果：`/tmp/travodds-lockfree-model-20261029-result.json`。模型运行耗时极短，没有编译或运行完整库/重负载测试。
- 本报告首先给出静态协议审查，不能代替父线程随后执行的实际 C++/跨进程/检查器验证；没有以历史测试数字证明新代码。

## 为什么不能对生产实现直接移锁

生产 v5 槽位使用独立的非原子 `generation/state/writer_held/pending/loaned`。metadata mutex 同时串行化：

1. WRITING → PUBLISHED 与 Writer/history 引用；
2. Reader pending → loaned 以及归还后复用；
3. Reader 注册 generation/accepting 与匹配目标快照；
4. late TRANSIENT_LOCAL replay 添加引用；
5. close/unregister/死亡回收删除引用与注册位重用。

只去掉 publish 的锁会与仍持锁的其他路径发生数据竞争；对每个字段分别 atomic 化也不能保存跨字段不变量。特别是“先清 pending，再设 loaned”，即使每次修改都是原子的，仍暴露暂时无 Reader 引用的状态，Writer 可在中间覆盖载荷。注册索引重用还需要注册 generation 与目标选择共同证明，不能仅靠一个槽位 generation 补齐。

通知移到锁外只影响唤醒后竞争及故障窗口，不消除元数据提交的锁依赖。当前强目标需要协议改造，而不是 notify 的局部位置调整。

## 候选 control 协议及关键证明义务

每个 control 为 64 位：高 32 位为 generation；低 15 位为 pending；再 15 位为 loaned；bit30 为 Writer/history pin；bit31 为 WRITING。静态 Reader 集合避免了本轮引入注册位重用。

### 1. 原子 pin 转移

Reader 成功 CAS 一次性执行 `pending_i -> loaned_i`，没有无引用空窗。Writer 只有从整个 flags 为 0 的 control 才能 reserve 下一代，因此 pending、活跃 Loan 或 Writer/history pin 任一存在时都不能覆写。

### 2. 归还与跨代复用

release 使用包含 generation 和 flags 的完整 control CAS。代次检查不是与清位分离的两个动作。若旧请求在读取旧 control 后暂停，新代已经复用，则旧 CAS 与当前完整 control 不相等，不能清除新代的 Loan bit。

不能将此 CAS 优化成“先读 generation，再 fetch_and 清 bit”。模型已给出具体反例：两个延迟归还请求读到旧代；第一个清位；Writer 复用为新代；第二个旧请求把新代引用误清。

### 3. 载荷发布可见性

Writer 在 WRITING 所有权下写 payload/size/sequence，随后以 release 语义 CAS 发布 control，再 release-store 全段 watermark。Reader acquire 采样 watermark，并且成功 acquire/acq_rel CAS 取得 loan pin 后，才读取非原子的 size/payload。

扫描阶段只读取 atomic control/sequence，不读取未 pin 的普通载荷元数据；这避免 Scanner 看见复用时对非原子载荷产生读写竞争。

### 4. 最后一次读取到下一次覆写

Reader 必须在完成最后一次载荷读取后 release 归还 pin。所有 control RMW 使用 acq_rel；Writer 下一次 reserve 以 acquire/acq_rel CAS 获取前代所有 pin 归还形成的 release sequence，然后才交还可写地址。该同步关系用于保证“不仅不同时覆盖，还存在上一代读完到下一代写入的 happens-before”。

该证明依赖每个共享 pin 有正确的本地所有权管理：不能有人在其他线程仍读取时提前归还同一 pin。多个本地 Loan alias 应由共享 owner 合并为最后一次归还，不能把单个共享 bit 当成任意多个独立引用计数。

### 5. Reader 扫描顺序

仅扫描各槽位并选择最小 sequence 仍可能乱序：扫描 A 时尚未发布，随后 Writer 发布 A(seq1)、B(seq2)，扫描 B 时看见 seq2，于是先返回 seq2。

原型通过先 acquire-snapshot `published_watermark`，再只选择 `sequence <= snapshot` 的槽位避免该问题。水位前已提交、且对该 Reader 仍 pending 的槽位被其自己的 pending bit 固定，扫描期间不会复用；水位后新样本被本轮排除。commit 先发布槽位 control，最后更新水位，水位是全段对 Reader 可见的提交门。

此证明还依赖该 Reader 的 take/discard 串行；如果允许同一个 Reader id 的另一条线程同时 discard pending，则“自己的 pending 一直固定候选槽位”这个证明不再成立。其他 Reader 与已取得 Loan 的归还线程只改变自身 pin，不破坏候选 Reader 的 pending。

## 有限状态模型结果

| 模型 | 检查规模 | 结果 |
|---|---:|---|
| 分离 generation 检查与延迟清位 | 20 个可达状态即找到反例 | 旧归还清除新代 pin |
| 完整 control CAS 的同一陈旧归还模型 | 19 个状态 | 未找到上述跨代错误 |
| 一个槽位、两个 Reader、两代生命周期 | 69 个状态、134 个转移 | pin 存在不覆写、仅取已提交载荷等安全断言通过 |
| 不设水位的按序扫描 | 35 种事件交错 | 2 种先返回 seq2 漏过 seq1 的反例 |
| 设置发布水位的相同扫描模型 | 35 种事件交错 | 未发现扫描乱序 |

这些模型使用顺序一致性原子抽象、有限代次和参与者数，不包含 C++ 弱内存、futex、崩溃、映射生命周期或动态注册。结果支持协议设计并能回归反例，但不是完整形式化证明，也不是性能结论。

## 阻止直接生产替换的条件

| 条件 / 限制 | 当前判断与后续要求 |
|---|---|
| 固定 Reader，最多 15 个 | 与生产最多 64 个 Reader、动态发现和注册语义不同。不能直接映射注册槽位后宣称兼容。 |
| 32 位 generation 不回绕，达到 UINT32_MAX 后退休 | 安全性优于回绕，但造成可用容量最终减少。一个热点槽位每秒复用 100 万次约 71.6 分钟可用尽代次；每秒 10 万次约 11.9 小时。初稿优先低索引的问题已通过 producer-only next_slot 轮转缓解，但不能消除有限代次寿命，仍应评估更多代次位或单独 epoch 设计。 |
| 单 Writer lane | reserve/commit/abort/close 必须串行。已发布 Writer pin 和 Reader Loan 可异线程 CAS 归还，但不能让 WRITING abort 与 commit 任意竞争而沿用当前证明。 |
| 固定 Reader 死亡 | 原型没有死亡检测/回收；死 pin 可以造成背压及耗尽。只要仍有空槽，不能由于死 Reader 锁住全池而阻止提交，这与无限容量/不中断服务不同。 |
| Writer 在 slot control 提交后、watermark 前死亡 | 可能留下尚未对 Reader 发布的 pin/槽位。安全拒绝暴露不完整提交，不是自动事务恢复；通常需要新 session。 |
| close_writer | 当前基础协议仅置 closed，不自动释放所有 history/reservation pin；与生产关闭流程不同。应验证关闭后可 drain 已提交 pending，已有 Loan 可以归还，映射何时结束由所有权保证。 |
| 映射与初始化寿命 | 共享 State 中没有进程本地指针。外部必须保证初始化完成后再 attach。已复查 MappedPool::Loan 的 process-local shared_ptr 保活映射，move 后原 Loan 清空；直接使用 core Sample 仍须调用方保活映射及恰好一次归还。 |
| token 来源与 Reader 身份 | 基础 low-level API 信任正确 Reader id；不能把它当作隔离恶意同用户进程的安全边界。DDS 层还需 Writer/Reader identity 和借用批次配对保护。 |
| 无锁范围 | commit 的核心状态更新可不取用户 mutex、不分配、不做 /proc 检测、不 syscall；futex 唤醒应作为独立适配层说明。整个 DDS write 仍有 Writer/history 等其他锁。 |

## 推荐验证门槛

1. 强制旧归还在读取旧 control 后暂停，完成新代复用后再恢复，验证精确 CAS 不清新代 bit；所有 Reader 组合分别覆盖。
2. Reader 已 pin 后 SIGSTOP：Writer 对其他可用槽位提交必须完成；被 pin 的槽位字节必须不变；耗尽时明确返回资源结果。
3. 水位前后插入确定性屏障，复现未门控版本乱序，并验证门控后只按本轮可见水位取最早序号。
4. 代次 UINT32_MAX-1 最后一次复用，以及退休后不再分配；最终 UINT64_MAX sequence 不回绕。
5. 分别验证共享对象在不同映射地址下工作、Writer 关闭/退出后的 Loan 保活、move-only Loan 的最后归还。
6. 普通版、ASan/UBSan、可用时 TSan；跨进程原子访问不能仅靠 TSan 是否报错判断。实际检查器失败要区分环境失败与协议错误。

只有固定集合原型与这些安全条件稳定后，才应设计动态控制面集成；生产后端整合仍需按注册、replay、QoS 与故障矩阵单独审查。

## 辅助层复查

`MappedPool::wait_take()` 先采样 changed 再调用 take 谓词，随后 futex 比较旧值；窗口内提交会使比较 EAGAIN。EINTR/超时后重查，原始总 deadline 不重置，单次睡眠最多 100 ms。notify/wake syscall 与 core commit 分离；未调用 wake 仍可周期检查，不保证即时唤醒延迟。Loan reset 先清共享 pin，再丢 process-local mapping owner，映射在归还过程中仍有效。

发现并已提醒主线程：新增轮转 cursor 后，exec 验证不能再断言“下次 reserve 必须立刻复用刚解锁的 first.slot”；需有界搜索该槽或将其他槽先保留，再验证其 generation 增长。这属于测试对分配策略的过强假设，不是 pin 协议失效。

## exec 验证与复用库脚本复查

后续再次复查 `exec_demo.cpp`，轮转假设已经修正：最多保留 4 个 reservation，找到原始槽位后验证 generation 增长及旧 token 归还失败，最后逐一 abort。验证入口使用真正 exec、命名文件重新打开和映射、FD_CLOEXEC 防止继承数据 fd，并确认子进程映射地址不同；精确 unlink 后继续保持映射和 Loan。Reader 0 在取得 Loan 后真实 SIGSTOP，Writer 在其他空槽提交 10,000 条仅给 Reader 1 的样本，Reader 1 完整按序消费并归还；恢复 Reader 0 后再验证其旧 Loan 内容。该范围足以支持固定 Reader 原型的目标场景，不能外推到所有新样本都发给暂停 Reader 时无限进展：后者会合理背压。

初次复查的 CMake 有独立 `lockfree_pool_test` 和 `lockfree_pool_exec_demo` 两个 target，C++17、警告视作错误、Threads 链接。后续加入 `lockfree_pool_controller_test`，三个 C++ CTest 均设置 30 秒 timeout；可用 Python 时再加入基础与注册两个有限模型，各 10 秒 timeout。没有把原型加入生产 DDS 构建或旧验收总数。

`tools/ospp/notification_progress.py` 明确将原型和 production boundary 检查分开，生产库标 `production_library_rebuilt=false`。复用库 SHA 与冻结 verification manifest 对比；对冻结清单中的 include/src 源逐个比对。审查者独立枚举了实际目录与清单：二者均为 292 个文件，无额外/缺失文件；唯一不一致是 `src/common/memory/payloadbuffer.cpp`，其当前字节精确加一个末尾 LF 后与冻结 SHA 相同。因此这项窄白名单有实际证据，不能被描述成无条件接受“空白差异”，也不应声称新生产库被重新构建。

没有发现阻断脚本执行或原型验证的缺陷。已建议父线程两项低成本加固：GNU timeout 加 `--kill-after` 作为忽略 SIGTERM/停住子进程的最终兜底；运行结束时重新核对 production 输入及所复用库的 SHA，以消除检查到使用之间并发变更的证据缺口（初稿结尾仅复核测试/原型输入）。TSan 的 setarch 禁止/运行时映射失败应记录为环境限制，不能记成协议 race，也不能记成 TSan 通过。

## 后续独立动态注册控制面审查

本节审查的是同一 worktree 中后续加入的实验 `controller.hpp`，冻结的 `shared_pool.hpp` layout v2 未修改。此前“固定 Reader 集合”的结论仍准确描述核心本身；新控制层在核心之外增加了有限动态注册，不表示 DDS 发现、重放、QoS 或故障恢复已经集成。审查快照：

| 文件 | SHA-256 |
|---|---|
| shared_pool.hpp | `6b6b1b03ceef8a1981ee18c7c392bd0919d098a81a48239a2213bc49f9acbbdf` |
| mapped_pool.hpp | `be4174e2b83f8155beafe812923e4e7d2b797fabacd486f867a4c829deb3379b` |
| controller.hpp | `be72b55973f7c2b0b9d663cf926e68031a0bf962a669012fc6c9ef58f6f0ca3e` |
| test_controller.cpp | `464db1a1f2a02e7b209cfff01bc5aae20709210a09adca96350bdc6c9e1f468d` |
| README.md | `766df1790b69242522c31199f2cb4b3b65f5d3d7a7b0ae3ac4454fc4b2c26424` |
| model_registration.py | `95094d2f016e046b1dede1c71663f9b1fa9ca84b422b0154b0253ddd50ed4879` |

**结论：未发现阻断独立实验验证的动态注册协议缺陷；不建议因此替换生产 SharedPayloadPool。**控制层以独立 64 位 gate 打包注册 generation、OPEN/CLOSING/FREE 和 inflight 计数，原子地取得操作租约。只检查 epoch、等待旧 Loan 位归零再复用身份不足以正确实现动态注册：

1. 旧 take 检查 epoch 后暂停，尚未取得任何 Loan。控制线程关闭、清 pending、重用注册位并发布新样本。旧 take 恢复后只凭数字 Reader id，可能消费新注册的数据。
2. 若未来加入对同一个历史样本 generation 的 replay，延迟旧 return 的完整 slot control 也可能与新注册的 Loan 控制字相同；仅有 slot generation 不排除注册 ABA。当前实现明确拒绝 replay，第二项是未来改造风险，不能报告成当前已经实现的 replay 缺陷。

`begin()` 的同一 gate CAS 将 epoch/phase 验证与 inflight 增加合并（controller.hpp:226）。`apply_close()` 先将 OPEN 改为 CLOSING 并保留计数（114），新 commit 只投递给 OPEN mask（145）。`poll_one()` 只有在 inflight=0 后清 pending，绝不清 loaned；所有对应 pin 都归零后，还必须成功 CAS 原始 CLOSING+count0 gate 到 FREE（241）。这阻止旧 take 尚未进入核心或旧 return 尚未完成时复用注册位。

归还可以在 CLOSING 期间进入。它可能恰好在清理者观察 count0 后取得 lease，但它只删除已有 Loan、不创建 pending；最后的精确 gate CAS 会拒绝在该归还仍持有 lease 时回收身份。若归还已完成、计数回到零，复用也已安全。关闭前取得的 take lease 可以在关闭后完成一次旧样本借用，这是在途操作语义；close 返回 Pending 时不等于旧操作全部停止。

`apply_open()` 还检查 FREE 身份在全部槽中确实没有 pending/loaned（92、220）。在 Writer/control 串行、所有发布均经 Controller、禁止绕过注册租约的约束下，这项防御检查与最终 gate CAS 没有新的身份复用窗口。注册 generation 及 inflight 达到上限时拒绝继续增加，不回绕。const Reader 方法复制一个只含共享 State 指针的 Core facade 后操作该共享状态；它没有复制数据或产生独立 cursor，也不改变核心 CAS 协议。

新增可执行模型 `experimental/lockfree_pool/model_registration.py`，实际轻量 Python 结果保存在 `/tmp/travodds-registration-model-20261029-result.json`：

| 注册复用模型 | 可达状态 / 转移 | 结果 |
|---|---:|---|
| 旧 take，epoch 检查与操作分离 | 19 / 23 | 找到跨注册消费反例 |
| 相同旧 take，epoch+inflight 原子 gate | 15 / 17 | 未找到反例 |
| 假设同样本 replay 的旧 return，无操作 gate | 35 / 51 | 找到跨注册清 Loan 反例 |
| 相同旧 return，有操作 gate | 30 / 41 | 未找到反例 |

这仍是有限顺序一致性抽象模型，未证明 C++ 弱内存、系统调用或死亡回收。模型特意保留不安全版本的反例，不应删除其与当前 replay=Unsupported 的范围说明。

必须保留的使用边界及最小后续工作：

- gate 是身份寿命保护，不是同一 Reader 的 consumer 互斥。take/discard 仍须按身份串行，否则不能沿用核心水位扫描证明。README 已明确该要求。
- Controller::Sample 是手工归还的底层令牌，Lease 也持 raw registry 指针；调用方必须保活数据与控制两个映射。它尚未像 MappedPool::Loan 一样以 RAII 自动持有全部映射。正式接口应增加双映射 owner 及 move-only Loan，再讨论 DDS 接口适配。
- 被暂停或死亡的 lease 可能永远占用自身注册位。其他 OPEN 身份在剩余容量内可继续提交；不能宣称无限进展或已具备死参与者安全回收。直接清 inflight 会重新引入旧操作恢复后的跨代风险。
- Request 是由单 Writer 应用的值；跨进程请求传输、发现、响应关联和认证尚未实现。replay、DDS/QoS、完整 Writer 重启/新 session 也未覆盖。
- test_controller.cpp 提供 8 个具名检查，涵盖旧 take lease、延迟归还、Loan 保留、真实子进程死亡、身份/计数耗尽、session 和独立 Reader 线程关闭/重开压力。实际编译运行由父线程统一完成，审查者随后只读核对其最终 normal 结果及日志，见末节。此前建议的 waitpid EINTR 假失败风险已加固：观察停止及最终回收均重试 EINTR。

新增线程压力检查（test_controller.cpp:201）使用 128 轮注册 generation 轮换，每轮保留 Reader 0 Loan，在 Writer close 后完成给 Reader 1 的 16 次提交及归还；每一次提交都通过 atomic counter 握手要求 Reader 0 再次读取并校验全部 64 字节旧载荷。stage ticket 含 round，不复用同一阶段值；初始 seq/generation 由 stage release/acquire 发布，exception_ptr 只在 join 后读取。Writer 等待 Returned 后才执行最终 poll/free 和下轮 open，仍满足同身份 take/discard 串行约束。stop flag、总 deadline 和异常 join 有退出路径。静态复查未发现测试侧数据竞争或阻断实验的协议缺陷；该检查覆盖线程级真实交错，不替代独立进程控制请求传输或死亡恢复实验。

下一步先统一运行新增控制面和模型，保存输入 SHA 与结果；其后优先建立所有权安全的注册 Loan facade 和具体 replay 语义，逐项重新审查，再评估生产接入。

## 生产接入的最小路径与前三个硬缺口

独立复核了生产 `sharedpayloadpool.{h,cpp}`、`sharedpayloadpool_layout.h`、`readerdatasharing.cpp`、`datawriterimpl.cpp` 和 `sampleloan_state.h`。以下排序针对“使实验后端可进入受限的真实 DDS 路径”，不是要求一次完成现后端的全部能力。

### 1. 运行时布局、能力协商与长期代次

生产布局是 version 5、最多 64 Readers、64 位 sample/registration generation（sharedpayloadpool_layout.h:11）；运行时配置 slots/payload/alignment，DataSharingQos 默认 max_readers=16，合法范围到 64（include/qos/datasharingqospolicy.h:26、src/qos/datasharingqospolicy.cpp:12）。Writer 将这些配置传入创建过程并广告到发现数据（datawriterimpl.cpp:175）。原型的 Pool 是编译期模板、最多 15 Readers、32 位代次退休，默认配置本身就不兼容，且热点长跑可能在小时尺度退休槽位。

最小改进：先定义独立 backend/layout 版本及发现兼容规则，在初始化时完成只读布局验证与受支持配置选择；对不支持的 16/64 Reader、超出模板范围的大小/alignment 及长跑寿命给出明确能力结果。不能重解释现有 v5，也不能把注册数静默截成 15。将来正式长期使用需给出代次寿命设计或可证明的 session 轮换，轮转 cursor 不能代替该设计。

### 2. 可执行的 Writer 控制通道、GUID 目标及 replay 语义

生产 Reader 工作线程在映射成功后直接 `registerReader(readerGuid, replay)`（readerdatasharing.cpp:97、109）；当双方 TRANSIENT_LOCAL 时 replay=true（78）。生产注册在 mutex 内向 Writer-held 历史样本补 pending（sharedpayloadpool.cpp:555、580），而 Writer 每次选择匹配 GUID 并在 history commit 时发布（datawriterimpl.cpp:520、549、561）。实验 Controller 只有请求值及 `apply_*`，没有跨进程请求/响应；它不管理 GUID，并明确拒绝 replay。

最小改进：先给 Writer lane 增加具体且有界的请求接收/响应协议，将 GUID→Registration→targets 与 open/close/commit 顺序纳入同一证明；增加独立进程注册、关闭、响应丢失与重复请求实验。第一版可以显式只支持 VOLATILE，保留当前后端处理 TRANSIENT_LOCAL；启用时必须准确协商并遵循 AUTO/ON 的失败语义。不能只把 `registerReader()` 替换成 `apply_open()`，更不能为“兼容”直接修改冻结 control 来加 replay 位。

### 3. 端到端 Loan 所有权、history 引用及故障会话边界

生产 SharedReadLoan 持 SharedPoolReader，再持 SharedPayloadPool，关闭或丢 Reader handle 后载荷仍活；reset 最后归还（sharedpayloadpool.h:71、sharedpayloadpool.cpp:724）。Writer SampleLoan::State 持 pool/token，history 的 nativePayload 使用 aliasing shared_ptr 延续 owner，nativeCommit 与 RTPS 序列化可能随后才发生（sampleloan_state.h:6、datawriterimpl.cpp:539、561、566）。Reader 收到 native 样本后再以共享 owner 挂到 history（readerdatasharing.cpp:190、200）。实验 MappedPool::Loan 只覆盖固定核心；Controller::Sample 尚为可复制裸令牌，既不自动持双映射，也不携带生产 SharedSampleMetadata/history 契约。

最小改进：增加只在本进程存在的双映射 owner、move-only registration Loan、history 共享别名及恰好一次归还；将 payload/size/sequence/metadata 发布为同一个可见生命周期；验证 Writer 关闭、Reader 删除、跨 history alias、迟归还和 RTPS 延迟序列化。旧 session 的 Loan 必须继续保活，不能因耗尽或切换后端撤销映射。暂停/死亡 lease 仍会占注册位，Writer 半提交仍需新 session；必须先定义资源耗尽、保守不回收与端点重建策略，不能沿用生产 mutex 的 removeReader/robust-owner-dead 代码而声称兼容。

建议接入路线为：保留生产实现，新后端显式 opt-in → 新版本映射与受限配置 → VOLATILE 的真实控制通道和目标选择 → 双映射 RAII Loan/history/元数据 → 进程级 DDS 实验及 AUTO/ON 回退矩阵 → 再扩展 replay、长期代次及死亡恢复。所有版本中都将“核心原子提交”“atomic+futex 通知”“完整 DDS write”分别报告；不能把局部无锁外推为完整调用链无锁。

本次生产接入审查没有改动生产源码，也没有运行生产构建。审查时工作树 HEAD 仍为 `3da8e077b9902142cdb4549eafa5b43114bdf936`，实验变更未提交。审查者实际运行的是上述轻量 Python 模型；C++ 构建、跨进程执行、检查器及生产测试由 root 串行统一执行。

## 最终冻结与普通构建证据核对

core、controller、测试与 README 最终冻结后，独立读取了 root 的 `/home/ubuntu/project/travodds/artifacts/notification-progress-20261029/normal-controller-final/results.json` 及首轮三份程序日志：核心 **14/14**，独立 exec **通过**，动态控制面 **8/8**（含新线程用例）。JSON 记录三类程序分别编译成功、各运行三轮 exit_code=0，`passed=true`；其源码 SHA 与本报告冻结的 core/controller/test/model 输入一致。完整源文件 hash 已更新在实验目录 `review-source-hashes.json`，包括 3 个头文件、3 个 C++ 测试/演示、2 个模型、CMakeLists 与 README。

该次命令清单只有原型，不含生产测试/全库构建，且 `production_library_rebuilt=false`，因此不能将上述结果解释为已重建生产库或完成整套 DDS 验收。本独立报告封存时 root 仍在执行后续 ASan/UBSan、TSan 与本机串行生产构建；这些后续结果须以各自最终日志为准，本报告不预先认定通过。普通构建通过支持继续实验推进，未消除前述生产接入硬缺口。
