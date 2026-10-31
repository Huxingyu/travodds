# 共享发布历史环与载荷写入边界

本页描述 2026-10-31 严格补全轮的设计与验收不变量。布局 **v7**、发现 **v5** 已完成静态复核及同源码的三轮验证：**542/542 完整普通回归、ASan/UBSan 99/99、TSan 99/99、9/9 有限示例、94.132% 新增生产可执行行覆盖率（2984/3170，143 个翻译单元）**。323 项冻结输入身份为 `42ee01a89f3cba2e11df8147ece3ea73ba2906c0652bcbd0c71e460262b6b3ea`。最终身份、原始证据重算与资源/清理核验以 [综合审计](../../../artifacts/strict-ring-20261031/final-verification.json) 为准；官方八项对应见 [验收矩阵](DATASHARING-ACCEPTANCE.md)。前轮 529/86/86 不作为本轮证据。

## 官方原文与验收范围

项目 261760062 的 programDesc 将方案描述为“按Writer GUID命名的共享环形历史段”，并在不足和改进中分别写明“DataSharingPayloadPool（环形段历史）”与“DataSharingPayloadPool（Writer命名环形段）”。因此需要真实的有界共享历史环；有限槽位的轮转扫描和环形空闲分配器本身不充分。

原文功能目标是“同主机plain/bounded类型数据读写全程物理内存仅写入1次，无任何用户态拷贝”。outputRequirement 第 5 项同时明确“支持跳过序列化/反序列化的零拷贝分支，同时保留RTPS降级回退机制”，第 7 项要求 Loan Sample 写发布与 Loaned Sample 读订阅示例。严格零拷贝验收因而针对双方兼容、实际选中的本机 native Writer Loan → Reader Loan 分支；普通 write、预分配复制输出和 RTPS 继续提供兼容行为。

原文留存于本轮证据根 `artifacts/strict-ring-20261031/requirements/` 的 official-programDesc.html、official-outputRequirement.json 和 acceptance-plan.json；来源是 10/29 已保存的官方响应，本轮没有重新联网取文。上述引文与后述可执行不变量对应，不把旧“固定槽位池”文档的措辞当作新实现通过证据。

## 分配位置与发布历史是两套顺序

| 结构 | 管理内容 | 顺序依据 |
|---|---|---|
| reservation 环 | 下一次尝试借出的固定共享槽位；Header.ring_tail 为不回绕的绝对位置，Slot.ring_position 记录归属 | 借出时的 reservation 位置，物理候选槽位对容量取模 |
| published history 环 | 已成功发布、尚被历史或投递/借阅引用保活的节点 | 实际 DDS publish sequence，由 history_head/tail/count 和 Slot 的 previous/next/linked 连接 |

例如先借出 A、再借出 B，应用先发布 B、再发布 A，历史必须按 B 的发布序列、A 的发布序列遍历。不能因 reservation 顺序先遇到 A 就阻塞已经成功发布的 B，也不能为了排列历史而移动载荷。环链接和固定大小控制元数据可以改变，槽位载荷的 backing extent 保持不变。

published history 是闭合双向环。非空时 tail.next=head、head.previous=tail；单节点的前后链接均指向自身。空环 head=tail=NO_SLOT、count=0。正常读取遍历恰好 count 个节点后回到 head，并验证最后节点为 tail；非空环不以 NO_SLOT 终止。

## 写侧事务与发布线性化

结构变更由 Publication 的 try-CAS lane 串行化；竞争方及时失败或按既有有限等待策略重试，不因管理 mutex 的持有者暂停而等待它释放。该 lane 只约束结构修改，不能据此宣称整个 DDS write 路径形式化 lock-free。

history_epoch 为 64 位结构版本：偶数表示可供读者验证的稳定快照，奇数表示一次 append/unlink 事务进行中。写侧取得 Publication lane 后把 epoch 变为奇数，更新相关原子链接、head/tail/count、节点阶段和发布水位，再以 release 操作提交新的偶数 epoch。Reader 用 acquire 读取 epoch，在遍历前后核对同一个偶数值。

需要区分数据发布与稳定环快照提交：先完成载荷/metadata、pending 和 PUBLISHED 阶段，再更新 last_sequence 发布水位，随后才提交偶数 epoch。Reader 只选择不超过该水位的已发布节点；last_sequence 更新是对 Reader 可见的数据发布点，偶数 epoch 是稳定环结构的提交点。通知用于提示重检谓词，不充当早于载荷完成的发布点。先看到奇数 epoch 的读者可能在安全 fallback 中取得已独立发布的 pending 节点；因此不能把“遍历时遇到奇数”误判成段已损坏。

回收已链接槽位时，先持有 Publication lane 并进入奇数 epoch，再取得槽位冻结权并复查全部 pin；成功 CAS 推进 reservation 尾位置后执行 unlink，随后才重置可重用的控制字段。稳定偶数快照不能合法包含一个已链接的 FROZEN 节点。正常失败路径必须恢复完整状态，不能留下丢失链接或已被错误消费的 reservation；若修改者异常死亡在奇数事务中，按后述故障路径处理，不能伪造一次正常偶数提交。

## 必须始终成立的不变量

1. 每个已链接节点只出现一次，count 不超过容量；previous/next 双向一致，首尾闭合。空环状态三字段一致。
2. 从 head 开始按 count 个节点观察到严格递增的 DDS publish sequence；跨越 tail 回到 head 才允许序列从最新回到最旧。Reader 的 after-sequence 与 TRANSIENT_LOCAL 回放使用这条历史。
3. 未发布的借出不在 published history 中。discard_loan 不发布数据；取消、失败和重试不制造虚假历史项或破坏其他已发布节点。
4. Writer 引用、未读 pending、Reader Loan pin 及短操作 gate 共同约束复用。仍有任一保活引用时不能覆盖载荷；节点移出历史也不自动取消独立应用 Loan 的保活。
5. token 仍绑定 session、slot 和 generation。旧 token 在回绕后不能释放、发布或访问新一代样本；generation、绝对 reservation 位置与 epoch 达到边界时拒绝继续变更，不允许 64 位回绕隐藏 ABA。
6. 稳定偶数 epoch 下发现越界、重复、断链、反向链接不一致、count/首尾不符或非递增序列，报告 CORRUPT，不用扫描结果把结构损坏伪装为正常环。
7. 发生竞争时可以重试或明确返回耗尽/超时/关闭；不能通过复制载荷、覆盖慢 Reader 或先丢失原 history 再尝试分配来制造成功。

## 正常遍历、回放与故障 fallback

正常 Reader 与晚加入回放沿 published history 环遍历；扫描所有槽位、选取最小 sequence 只保留为 epoch 不稳定、修改者暂停或死亡时的安全 fallback。不能使 fallback 成为稳定正常路径，再把未使用的环字段作为“环形历史已实现”的证据。

读者在结构读取前后确认 epoch；发生变化或观察到奇数时，丢弃该次结构快照。fallback 仍须重新校验 slot gate、session/generation、PUBLISHED、pending/loan 状态与读取水位，只借用已经完成发布的节点，不访问正在构造的载荷。管理者死亡或元数据无法安全归属时延续 CORRUPT/新会话恢复规则，不能自行猜测并清除未知 pin。

回放注册在稳定快照上选择已发布历史，竞争则重试；在登记 ready 之前完成对应历史引用的建立。已借出的 Loan、Writer 关闭和 Reader 删除之间的映射保活规则继续有效。多个 Writer 各有独立的环，不增加跨 Writer 全局 FIFO 要求。

## “只写一次”的软件命题与独立证据

可由 DDS 实现保证并验证的是：借出开始对象寿命而不初始化载荷；应用直接在最终共享槽位填写样本；随后 publish、通知、history 准入、read/take 与 return 不再写入该载荷，也不另建完整样本副本或进行 CDR 序列化/反序列化。应用可选择重复写字段，库提供“一次直接填写即可发布”的通路；受控验收样例使用单遍填写。

Writer 与 Reader 的虚拟地址可以不同。独立 exec 进程必须从各自实际 Loan 地址定位 `/proc/self/maps` 的完整区间，以 `VMA.file_offset + (loan_address - VMA.start)` 得到文件偏移，核对 device major/minor、inode、偏移、长度及 MAP_SHARED。路径名、相同 checksum 或进程内指针相等不足以替代这一证据。

新共享布局 v7 将载荷区起点向上按宿主页大小对齐，使控制元数据不与首个载荷页重叠；仅该页对齐 padding 最多增加 page_size-1 字节，新增环字段另按实际 Header/Slot 大小计入预算。两者都必须纳入 segment_bytes 和 max_segment_bytes 的溢出/预算检查。槽位 stride 仍按类型对齐取整，其他槽位可能共享一个载荷页；因此生产保护覆盖整个载荷 span，而非单个槽位页。

Reader 侧 SharedPayloadPool::open 保留一个 Boost mapped_region，在完成布局、文件和 header 校验后，对从载荷起点到载荷区末尾的全部页设置 PROT_READ；控制元数据页仍为 RW，供注册、pending 和 Loan pin 使用。Writer 侧 create 的映射仍为 RW，供应用直接填写 Loan。Reader 载荷保护失败必须拒绝该共享映射，不能悄悄留下可写载荷。发现能力同步隔离新旧布局，具体版本与结果在冻结清单中核对。

独立保护工具在上述生产只读保护之上使用整页隔离样本：无键样本在框架执行窗口对整个载荷设置 PROT_NONE；带键样本只开放单独 key 页为只读，其余 body 页为 PROT_NONE。应用读写通过受控窗口完成；常规 mprotect、文件映射和 mremap 受 seccomp TSYNC 限制，另以拒绝探针证明封锁生效。所有同 backing extent 的别名映射均须纳入保护，不能只保护返回给应用的一个虚拟地址。PROT_NONE 与受控窗口属于测试取证机制，生产 Reader 的保护是 PROT_READ，生产 Writer 的映射不是只读。

PROT_NONE 除了阻止写入，也阻止框架从该载荷读取后复制到另一个用户态缓冲。原始 XML、日志中的逐例 proof 记录、映射信息、保护状态、seccomp 拒绝探针和 TypeSupport 回调计数须一起审计；仅有一个最终 passed 字段不足。保护窗口和允许的应用窗口必须与实际 publish/read/return 阶段对应，带键 body 不能借 key 页开放而被读取。

已有文件描述符的 read/pread/write/pwrite 等数据 IO 可以绕过用户映射权限。因此还需审计所选生产稳态路径不存在通过该类 IO 搬运载荷，明确测试不是针对任意恶意代码的内存安全沙箱。固定大小的键、通知、索引和控制元数据可以复制，应列出范围，不扩大为“任何字节均不复制”。

这组不变量落实原文避免用户缓冲、CDR 和接收输出之间完整载荷搬运的目标。它不等价于 DRAM 总线恰好一次写事务：内核页清零、缓存写回、页迁移、ECC 及编译器 store 合并均不由 DDS API 控制。验收结论应使用“单次应用载荷初始化、零用户态载荷搬运”，同时保留原文引文和上述实际证据，不把硬件事务计数写成已测结果。

## 本轮验收清单

| 范围 | 必需证据 | 状态 |
|---|---|---|
| 真发布历史环 | 共享布局与实际 Reader/replay 路径静态复核；结构快照、空/单节点、多次 wrap、乱序借出/发布 | 静态复核、普通版及两个检查版均通过 |
| pin/cancel/回收 | 慢 Reader、乱序归还、取消、KEEP_LAST/KEEP_ALL、晚加入回放及旧 token | 普通版及两个检查版通过 |
| 竞争与故障 | 稳定坏环拒绝、奇数 epoch/暂停 fallback、代次与 epoch 边界、关闭/死亡保活 | 普通版及两个检查版通过 |
| 载荷访问保护 | NativePayloadWrites 逐例 XML/log、同 backing、PROT_NONE/key-RO、seccomp 拒绝探针、回调计数 | 普通版及两个检查版通过 |
| 正式交付 | 新冻结身份下的普通回归、独立检查器、示例、覆盖率、完整审计与便携包 | 542/99/99、9 示例及 94.132%；精确身份已绑定 |

保护测试为 `test/unit_payloadpool/test_native_payload_writes.cpp`，原始记录核验工具为 `tools/ospp/native_payload_writes.py`。本轮外部审计位于 `artifacts/strict-ring-20261031/final-audit.py`；三轮均通过后生成的 final-bindings.json 绑定精确 542/99/99 数量、同一源码和元数据 SHA-256；草稿绑定不能产生最终通过结论。

用户授权本轮继续在当前 VM 串行推进：jobs=1、nice=19、ionice best-effort 7、编译子进程地址空间 768 MiB、无新增 swap 或支出；资源计划保留于同证据根 resource-plan.json。磁盘允许时保留对象以便增量构建，本轮不默认启用 --trim-build-cache。正式命令和实际资源退出记录随最终冻结结果保存。

## 原始结果与复现

原始结果分别为 [coverage-atomic-final](../../../artifacts/strict-ring-20261031/coverage-atomic-final/results.json)、[asan-atomic-final](../../../artifacts/strict-ring-20261031/asan-atomic-final/results.json) 和 [tsan-atomic-final](../../../artifacts/strict-ring-20261031/tsan-atomic-final/results.json)。每轮包含四个 NativePayloadWrites 的实际 XML、日志和严格核验摘要；final audit 重新解析原始 proof，重建 focused driver，逐项核对私有库/安装头文件/检查器符号，从原始 gcov JSON 重新合并覆盖率，并核对九个独立进程示例的完整计数。五个伪造或不完整证据负控均被核验工具拒绝，记录位于 parser-negative-controls/。

在匹配源码根目录执行，使用全新的空证据与构建目录；三个命令必须依次运行。每个编译子进程限制为 768 MiB，运行检查器不受编译地址空间限制：

```bash
DS_EVIDENCE=/path/to/new-evidence
ionice -c 2 -n 7 nice -n 19 python3 tools/ospp/production_lockfree_verify.py \
  --output "$DS_EVIDENCE/coverage-atomic-final" --build-dir "$DS_EVIDENCE/build-coverage" \
  --mode coverage --core --examples 3 --compiler-memory-mib 768

ionice -c 2 -n 7 nice -n 19 python3 tools/ospp/production_lockfree_verify.py \
  --output "$DS_EVIDENCE/asan-atomic-final" --build-dir "$DS_EVIDENCE/build-asan" \
  --mode asan --scope focused --compiler-memory-mib 768 \
  --focus-test KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration \
  --focus-test KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource \
  --focus-test NativeReaderIntegration.KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion

ionice -c 2 -n 7 nice -n 19 python3 tools/ospp/production_lockfree_verify.py \
  --output "$DS_EVIDENCE/tsan-atomic-final" --build-dir "$DS_EVIDENCE/build-tsan" \
  --mode tsan --scope focused --compiler-memory-mib 768 \
  --focus-test KeyedNativeCaptureFailurePreservesLoanAndRollsBackRegistration \
  --focus-test KeyedNativeReaderCaptureFailureDoesNotConsumeInstanceCapacityOrRetainSource \
  --focus-test NativeReaderIntegration.KeyedNativeKeySnapshotSurvivesSingleSlotReuseAndWriterDeletion
```

上述检查器范围为完整共享池、无初始化/key-admission/Reader 普通池和四项真实 DDS focused，并非全部 542 项均运行于检查器。ASan/UBSan 使用 detect_leaks=0，未执行 LSan。首轮三个 harness 的功能检查通过，但首次综合审计发现非原子计数产生的三条负计数被旧收集器误算覆盖，因此该报告不作为最终覆盖率。修复正计数判定并启用 -fprofile-update=atomic 后，143 个旧 gcda 连同 notes/二进制/flags 已归档，计数清零后全量重建并重新采集；新数据中全部生产代码负计数为 0，三个新运行再以同一冻结身份通过。旧统计 94.101%、失败审计及完整诊断保留于 audit-attempt1-diagnostic/ 与 gcov-negative-attempt1/，不冒充当前结果。便携记录及源码包见 [交付索引](../../../artifacts/portable-evidence-20261031/native-v5-delivery.md)；便携校验验证记录哈希与源码一致性，不会重新执行测试或原机完整审计。
